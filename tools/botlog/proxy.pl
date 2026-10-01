#!/usr/bin/perl
# Ein Spieler gegen das Feld: wie geht es Client 0 gegen die anderen Bots?
#
#   proxy.pl <laufordner> ...      (Client 0 ist der Stellvertreter, g_botStockMask 1)
#
# Das ist die Lage eines Menschen gegen zehn Bots, soweit ein Bot sie
# nachstellen kann: je leichter das Feld, desto besser steht der Stellvertreter
# da. Gezaehlt aus D- und K-Zeilen, Telefrags nicht.
use strict;
use warnings;

my ( %s, @rows );
for my $dir ( @ARGV ) {
	my %c = map { $_ => 0 } qw( kill death world dealt taken ms );
	my ( $first, $last );
	open my $in, '<', "$dir/baseq3/botlog.log" or do { warn "$dir: $!\n"; next };
	while ( <$in> ) {
		if ( /^T (\d+) 0 / ) { $first //= $1; $last = $1; next }
		if ( /^D \d+ (\d+) (\d+) (\d+) (\d+) (\d+)/ ) {
			next if $5 == 18;
			my ( $v, $a, $d ) = ( $1, $2, $3 + $4 );
			next if $v == $a;
			$c{dealt} += $d if $a == 0 && $v < 64;
			$c{taken} += $d if $v == 0 && $a < 64;
		} elsif ( /^K \d+ (\d+) (\d+) / ) {
			my ( $v, $a ) = ( $1, $2 );
			if ( $v == 0 ) { ( $a >= 64 || $a == 0 ) ? $c{world}++ : $c{death}++ }
			elsif ( $a == 0 ) { $c{kill}++ }
		}
	}
	close $in;
	$c{ms} = ( $last // 0 ) - ( $first // 0 );
	( my $name = $dir ) =~ s{/+$}{};
	$name =~ s{.*/}{};
	push @rows, [ $name, \%c ];
	( my $arm = $name ) =~ s/-\d+$//;
	$s{$arm}{$_} += $c{$_} for keys %c;
	$s{$arm}{runs}++;
}
sub line {
	my ( $n, $c ) = @_;
	my $min = $c->{ms} / 60000 || 1;
	printf "%-14s %6.2f %6.2f %6.2f  %6.2f   %5.0f %5.0f %6.2f\n", $n, $c->{kill} / $min, $c->{death} / $min,
		$c->{world} / $min, $c->{death} ? $c->{kill} / $c->{death} : 0, $c->{dealt} / $min, $c->{taken} / $min,
		$c->{taken} ? $c->{dealt} / $c->{taken} : 0;
}
printf "%-14s %6s %6s %6s  %6s   %5s %5s %6s\n", 'Lauf', 'K/min', 'T/min', 'Kt/min', 'K/T', 'Sch+', 'Sch-', '+/-';
line( @$_ ) for @rows;
print "\n";
line( "$_ (" . $s{$_}{runs} . ')', $s{$_} ) for sort keys %s;
print "\nK: Abschuesse des Stellvertreters, T: seine Tode durch Bots, Kt: durch die Karte.\n";
print "Sch+/-: Schaden, den er austeilt und einsteckt, je Minute. Je hoeher K/T und +/-, desto leichter das Feld.\n";
