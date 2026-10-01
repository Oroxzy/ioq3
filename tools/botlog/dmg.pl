#!/usr/bin/perl
# Wer ist staerker? Schaden und Abschuesse im gemischten Spiel (g_botStockMask).
#
#   dmg.pl <maske> <laufordner> [<maske> <laufordner> ...]
#
# Gezaehlt aus den D- und K-Zeilen des Bot-Protokolls: Schaden, der ankam
# (Leben plus Ruestung), zwischen Menschlichen (M) und Originalen (O), und
# die Abschuesse. Schaden gibt es zwanzigmal so oft wie Abschuesse - das
# Verhaeltnis ist darum schon aus einem Lauf brauchbar.
use strict;
use warnings;

my ( %sum, @rows );
while ( @ARGV >= 2 ) {
	my ( $mask, $dir ) = splice @ARGV, 0, 2;
	my %c = map { $_ => 0 } qw( dMO dOM dMM dOO dMw dOw kMO kOM kMw kOw );
	open my $in, '<', "$dir/baseq3/botlog.log" or do { warn "$dir: $!\n"; next };
	while ( <$in> ) {
		if ( /^D \d+ (\d+) (\d+) (\d+) (\d+) (\d+)/ ) {
			next if $5 == 18;		# Telefrag: 100000 Schaden, kein Kampf
			my ( $v, $a, $d ) = ( $1, $2, $3 + $4 );
			next if $v >= 64;
			my $vs = ( ( $mask >> $v ) & 1 ) ? 'O' : 'M';
			if ( $a >= 64 || $a == $v ) { $c{"d${vs}w"} += $d; next }
			my $as = ( ( $mask >> $a ) & 1 ) ? 'O' : 'M';
			$c{"d$as$vs"} += $d;
		} elsif ( /^K \d+ (\d+) (\d+) / ) {
			my ( $v, $a ) = ( $1, $2 );
			next if $v >= 64;
			my $vs = ( ( $mask >> $v ) & 1 ) ? 'O' : 'M';
			if ( $a >= 64 || $a == $v ) { $c{"k${vs}w"}++; next }
			my $as = ( ( $mask >> $a ) & 1 ) ? 'O' : 'M';
			$c{"k$as$vs"}++ if $as ne $vs;
		}
	}
	close $in;
	( my $name = $dir ) =~ s{/+$}{};
	$name =~ s{.*/}{};
	push @rows, [ $name, \%c ];
	$sum{$_} += $c{$_} for keys %c;
}
sub line {
	my ( $n, $c ) = @_;
	printf "%-20s %7d %7d %6.3f   %5d %5d %6.3f   %4d %4d\n", $n, $c->{dMO}, $c->{dOM},
		$c->{dOM} ? $c->{dMO} / $c->{dOM} : 0, $c->{kMO}, $c->{kOM}, $c->{kOM} ? $c->{kMO} / $c->{kOM} : 0,
		$c->{kMw}, $c->{kOw};
}
printf "%-20s %7s %7s %6s   %5s %5s %6s   %4s %4s\n", 'Lauf', 'Sch M>O', 'Sch O>M', 'M:O', 'K M>O', 'K O>M', 'M:O', 'M Kt', 'O Kt';
line( @$_ ) for @rows;
line( 'Summe', \%sum ) if @rows > 1;
