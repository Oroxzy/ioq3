#!/usr/bin/perl
# Zittern, Bild fuer Bild (F-Zeilen, g_botLog 3).
#
#   jitter.pl <botlog.log> ...        mehrere Dateien werden zusammengezaehlt
#   SHOW=5 jitter.pl <botlog.log>     dazu die fuenf laengsten Stellen
#
# Gezaehlt wird, was ein Zuschauer als Zittern sieht: in einer Sekunde dreht
# die Laufrichtung mindestens dreimal um mehr als 120 Grad, und der Bot kommt
# dabei keine 64 Einheiten von der Stelle. Auf dem Boden, lebend. Dazu
# "steht": eine Sekunde unter 20 u/s ohne Gegner in Sicht ist nicht zu sehen,
# also nur mit Protokollknoten - die F-Zeile kennt den Knoten nicht, die
# T-Zeile schon; beide werden ueber die Zeit verbunden.
use strict;
use warnings;

my ( %tot, @show );
for my $file ( @ARGV ) {
	my ( %f, %node );
	open my $in, '<', $file or die "$file: $!\n";
	while ( <$in> ) {
		if ( /^F (\d+) (\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (\S+) (\S+) (\S+) (\d+) (\d) (\d+) (\d+)/ ) {
			push @{ $f{$2} }, [ $1, $3, $4, $5, $6, $7, $8, $12, $13, $14, $15 ];
		} elsif ( /^T (\d+) (\d+) (\S+) / ) {
			push @{ $node{$2} }, [ $1, $3 ];
		}
	}
	close $in;
	for my $bot ( keys %f ) {
		my @l = @{ $f{$bot} };
		my @n = @{ $node{$bot} // [] };
		my $ni = 0;
		my ( $jit, $alive, $guard ) = ( 0, 0, 0 );
		# Richtungswechsel je Bild: die Geschwindigkeit dreht um mehr als 120 Grad
		my @flip = (0) x @l;
		for my $i ( 1 .. $#l ) {
			my ( $a, $b ) = ( $l[ $i - 1 ], $l[$i] );
			my $la = sqrt( $a->[4]**2 + $a->[5]**2 );
			my $lb = sqrt( $b->[4]**2 + $b->[5]**2 );
			next if $la < 40 || $lb < 40;
			my $cos = ( $a->[4] * $b->[4] + $a->[5] * $b->[5] ) / ( $la * $lb );
			$flip[$i] = 1 if $cos < -0.5;
		}
		# Fenster von einer Sekunde (20 Bilder), Schritt ein Bild
		my $mark = -1;
		for my $i ( 20 .. $#l ) {
			my ( $a, $b ) = ( $l[ $i - 20 ], $l[$i] );
			next if $b->[0] - $a->[0] > 1100;		# Luecke (tot, Neustart)
			next if $b->[10] != 0;				# nicht PM_NORMAL
			$alive++;
			my $fl = 0;
			$fl += $flip[$_] for $i - 19 .. $i;
			my $ground = 0;
			$ground += $l[$_][8] for $i - 19 .. $i;
			my $d = sqrt( ( $b->[1] - $a->[1] )**2 + ( $b->[2] - $a->[2] )**2 );
			$guard++ if $b->[7];
			next unless $fl >= 3 && $d < 64 && $ground >= 15;
			$jit++;
			$ni++ while $ni < $#n && $n[ $ni + 1 ][0] <= $b->[0];
			my $nd = @n ? $n[$ni][1] : '?';
			$tot{node}{$nd}++;
			my $g = 0;
			$g++ for grep { $l[$_][7] } $i - 19 .. $i;
			$tot{guarded}++ if $g;
			if ( $i > $mark ) {
				push @show, [ $file, $bot, $a->[0], $nd, $a->[1], $a->[2], $a->[3], $fl, $g ];
			}
			$mark = $i + 20;
		}
		$tot{jit} += $jit;
		$tot{alive} += $alive;
		$tot{guard} += $guard;
	}
}
my $min = ( $tot{alive} // 0 ) / 20 / 60;
printf "Bot-Minuten %.1f  Zittern %.2f s je Bot-Minute  (davon mit Tritt %.2f)  Tritt greift in %.2f %% der Bilder\n",
	$min, $min ? ( $tot{jit} // 0 ) / 20 / $min : 0, $min ? ( $tot{guarded} // 0 ) / 20 / $min : 0,
	$tot{alive} ? 100 * ( $tot{guard} // 0 ) / $tot{alive} : 0;
printf "  im Knoten %-8s %.2f s je Bot-Minute\n", $_, $tot{node}{$_} / 20 / $min
	for sort { $tot{node}{$b} <=> $tot{node}{$a} } keys %{ $tot{node} // {} };
if ( $ENV{SHOW} ) {
	for my $s ( ( sort { $b->[7] <=> $a->[7] } @show )[ 0 .. $ENV{SHOW} - 1 ] ) {
		next unless $s;
		printf "  %s bot %d @ %d %s bei %d %d %d, %d Wechsel, Tritt %d\n", @$s;
	}
}
