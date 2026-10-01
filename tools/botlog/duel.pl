#!/usr/bin/perl
# Wer gewinnt, wenn ein Teil der Bots als Original spielt (g_botStockMask)?
#
#   duel.pl <maske> <laufordner> [<maske> <laufordner> ...]
#
# Die Maske ist die von g_botStockMask: ein Bit je Clientnummer, gesetzt heisst
# Original. Gezaehlt wird aus qconsole.log, wer wen erwischt hat und wen die
# Karte. Zwei Laeufe mit vertauschten Masken heben auf, dass die Charaktere
# verschieden gut sind.
use strict;
use warnings;

my ( %sum, @rows );
while ( @ARGV >= 2 ) {
	my ( $mask, $dir ) = splice @ARGV, 0, 2;
	$mask = oct( $mask ) if $mask =~ /^0/;
	my %c = map { $_ => 0 } qw( hs sh hh ss hw sw );
	open my $in, '<', "$dir/baseq3/qconsole.log" or do { warn "$dir: $!\n"; next };
	while ( <$in> ) {
		next unless /^Kill: (\d+) (\d+) (\d+):/;
		my ( $k, $v ) = ( $1, $2 );
		my $vs = ( $mask >> $v ) & 1;
		if ( $k >= 64 || $k == $v ) { $c{ $vs ? 'sw' : 'hw' }++; next; }
		my $ks = ( $mask >> $k ) & 1;
		$c{ ( $ks ? 's' : 'h' ) . ( $vs ? 's' : 'h' ) }++;
	}
	close $in;
	( my $name = $dir ) =~ s{/+$}{}; $name =~ s{.*/}{};
	push @rows, [ $name, \%c ];
	$sum{$_} += $c{$_} for keys %c;
}

sub line {
	my ( $name, $c ) = @_;
	my $hk = $c->{hs} + $c->{hh};			# Abschuesse der Menschlichen
	my $sk = $c->{sh} + $c->{ss};
	my $hd = $c->{sh} + $c->{hh} + $c->{hw};	# ihre Tode
	my $sd = $c->{hs} + $c->{ss} + $c->{sw};
	printf "%-22s %5d %5d  %5d %5d  %5d %5d   %5.2f %5.2f   %5.2f\n", $name,
		$c->{hs}, $c->{sh}, $hk, $sk, $c->{hw}, $c->{sw},
		$hd ? $hk / $hd : 0, $sd ? $sk / $sd : 0,
		$c->{sh} ? $c->{hs} / $c->{sh} : 0;
}

printf "%-22s %5s %5s  %5s %5s  %5s %5s   %5s %5s   %5s\n", 'Lauf',
	'M>O', 'O>M', 'M ab', 'O ab', 'M Krt', 'O Krt', 'M K/D', 'O K/D', 'M:O';
line( @$_ ) for @rows;
line( 'Summe', \%sum ) if @rows > 1;
print "\nM>O: Menschliche erwischen Originale, O>M umgekehrt. ab: alle Abschuesse.\n";
print "Krt: Tode durch die Karte. M:O: das direkte Verhaeltnis, ueber 1 gewinnen die Menschlichen.\n";
