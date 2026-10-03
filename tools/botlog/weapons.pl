#!/usr/bin/perl
# Mit welcher Waffe sind die Bots unterwegs, und wie schnell haben sie nach dem
# Wiederbeleben eine richtige?
#
#   weapons.pl <maske> <laufordner> ...   (Maske wie g_botStockMask: diese Clients nicht)
#
# Aus den T-Zeilen (Feld Waffe = was er in der Hand hat; BotChooseWeapon nimmt
# immer die beste, also ist "Maschinengewehr in der Hand" praktisch "nichts
# Besseres dabei"). Und aus den K-Zeilen, womit sie Abschuesse holen.
use strict;
use warnings;

my $mask = shift;
my %WP = ( 1 => 'Gauntlet', 2 => 'MG', 3 => 'SG', 4 => 'GL', 5 => 'RL', 6 => 'LG', 7 => 'RG', 8 => 'PG', 9 => 'BFG' );
my ( %time, $alive, @first, $never, %kills, $kt );
for my $dir ( @ARGV ) {
	my ( %spawn, %got );
	open my $in, '<', "$dir/baseq3/botlog.log" or next;
	while ( <$in> ) {
		if ( /^T (\d+) (\d+) (\S+) (?:\S+ ){6}(-?\d+) \S+ (\d+)/ ) {
			my ( $ms, $c, $node, $hp, $w ) = ( $1, $2, $3, $4, $5 );
			next if ( $mask >> $c ) & 1;
			if ( $node eq 'RESPAWN' || $hp <= 0 ) {
				if ( defined $spawn{$c} && !$got{$c} ) { $never++ }
				undef $spawn{$c}; $got{$c} = 0;
				next;
			}
			if ( !defined $spawn{$c} ) { $spawn{$c} = $ms; $got{$c} = 0 }
			$alive++;
			$time{ $WP{$w} // $w }++;
			if ( !$got{$c} && $w >= 3 ) {
				push @first, $ms - $spawn{$c};
				$got{$c} = 1;
			}
		} elsif ( /^K \d+ (\d+) (\d+) (MOD_\w+)/ ) {
			next if $2 >= 64 || ( $mask >> $2 ) & 1 || $1 == $2;
			$kills{$3}++; $kt++;
		}
	}
	close $in;
}
printf "In der Hand (Anteil der Lebenszeit):\n";
printf "  %-9s %5.1f %%\n", $_, 100 * $time{$_} / $alive for sort { $time{$b} <=> $time{$a} } keys %time;
@first = sort { $a <=> $b } @first;
printf "Bis zur ersten richtigen Waffe (nicht Gauntlet, nicht MG): Median %.1f s, 75 %% bis %.1f s; nie in diesem Leben: %d von %d Leben\n",
	@first ? $first[ int( @first / 2 ) ] / 1000 : 0, @first ? $first[ int( @first * 0.75 ) ] / 1000 : 0, $never // 0, scalar( @first ) + ( $never // 0 );
printf "Abschuesse mit:\n";
printf "  %-22s %5.1f %%\n", $_, 100 * $kills{$_} / $kt for ( sort { $kills{$b} <=> $kills{$a} } keys %kills )[ 0 .. 5 ];
