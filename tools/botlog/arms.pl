#!/usr/bin/perl
# Mehrere Laeufe je Arm zusammenfassen: der Stellvertreter (Client 0) gegen das Feld.
#
#   arms.pl <laufordner> ...       Laeufe heissen <praefix>-<arm>-<nummer>
#
# Je Arm Mittel und Standardfehler seiner Abschuesse je Tod und des Verhaeltnisses
# von ausgeteiltem zu eingestecktem Schaden. Je niedriger, desto schwerer das Feld.
# Ein einzelner Lauf streut um etwa 0,25 - unter zwoelf Laeufen je Arm ist ein
# Unterschied von 0,1 nichts wert.
use strict;
use warnings;

my %arm;
for my $dir ( @ARGV ) {
	( my $name = $dir ) =~ s{/+$}{};
	$name =~ s{.*/}{};
	my ( $a ) = $name =~ /^[^-]+-(.+)-\d+$/ or next;
	my %c = map { $_ => 0 } qw( kill death dealt taken );
	open my $in, '<', "$dir/baseq3/botlog.log" or next;
	while ( <$in> ) {
		if ( /^D \d+ (\d+) (\d+) (\d+) (\d+) (\d+)/ ) {
			next if $5 == 18 || $1 == $2;
			$c{dealt} += $3 + $4 if $2 == 0 && $1 < 64;
			$c{taken} += $3 + $4 if $1 == 0 && $2 < 64;
		} elsif ( /^K \d+ (\d+) (\d+) / ) {
			if ( $1 == 0 && $2 < 64 && $2 != 0 ) { $c{death}++ }
			elsif ( $2 == 0 && $1 != 0 ) { $c{kill}++ }
		}
	}
	close $in;
	next unless $c{death} && $c{taken};
	push @{ $arm{$a}{kt} }, $c{kill} / $c{death};
	push @{ $arm{$a}{dr} }, $c{dealt} / $c{taken};
}
sub armstat {
	my @x = @_;
	my $n = @x;
	my $m = 0; $m += $_ for @x; $m /= $n;
	my $q = 0; $q += ( $_ - $m )**2 for @x;
	return ( $m, $n > 1 ? sqrt( $q / ( $n - 1 ) / $n ) : 0, $n );
}
printf "%-14s %3s  %13s  %13s\n", 'Arm', 'n', 'K/T', 'Schaden +/-';
for my $a ( sort { ( armstat( @{ $arm{$a}{kt} } ) )[0] <=> ( armstat( @{ $arm{$b}{kt} } ) )[0] } keys %arm ) {
	my ( $m, $se, $n ) = armstat( @{ $arm{$a}{kt} } );
	my ( $dm, $dse ) = armstat( @{ $arm{$a}{dr} } );
	printf "%-14s %3d  %5.2f +- %.2f  %5.2f +- %.2f\n", $a, $n, $m, $se, $dm, $dse;
}
