#!/usr/bin/perl
# Wie viel schiessen die Bots - und verfolgen sie, wen sie angeschossen haben?
#
#   fire.pl <maske> <laufordner> ...    (g_botLog 3; Maske wie g_botStockMask,
#                                        diese Clients zaehlen nicht mit)
#
# Feuer: aus den F-Zeilen, je Knoten (aus den T-Zeilen): Anteil der Bilder mit
# gedrueckter Feuertaste, mit sichtbarem Gegner, und mit sichtbarem Gegner,
# aber ohne Schuss.
#
# Verfolgen: eine "Gelegenheit" ist, wenn ein Bot einem Gegner in drei
# Sekunden mindestens 60 Schaden gemacht hat, der Gegner das ueberlebt und
# aus der Sicht verschwindet (die Treffer hoeren auf). Gezaehlt wird, wie oft
# derselbe Bot ihn in den zehn Sekunden danach erledigt, wie oft ein anderer,
# und wie oft er davonkommt - und was der Bot in der Zeit tat (Knoten).
use strict;
use warnings;

my $mask = shift;
my ( %fr, %vis, %att, %visnot, $frames, %whynot );
# Gruende aus BotCheckAttack (fire_why, Feld 18 der F-Zeile)
my %WHY = ( 0 => 'hat geschossen (Bild dazwischen)', 1 => 'Reaktionszeit', 2 => 'nach Teleport',
	3 => 'Waffenwechsel', 4 => 'Feuerpause (Charakter)', 5 => 'Gauntlet zu weit', 6 => 'nicht ausgerichtet',
	7 => 'Sicht verdeckt', 8 => 'Mitspieler im Weg', 9 => 'Splash zu nah', 10 => 'Taste loslassen',
	11 => 'Knoten fragt nicht' );
my ( %opp, $opps );
for my $dir ( @ARGV ) {
	my ( %node, @d, @k, %nodeat );
	open my $in, '<', "$dir/baseq3/botlog.log" or next;
	while ( <$in> ) {
		if ( /^T (\d+) (\d+) (\S+) / ) {
			$node{$2} = $3;
			push @{ $nodeat{$2} }, [ $1, $3 ];
		} elsif ( /^F \d+ (\d+) (?:\S+ ){12}(\d+) (\d) (\d)(?: (\d+))?\s*$/ ) {
			my ( $c, $pm, $a, $v, $why ) = ( $1, $2, $3, $4, $5 );
			next if ( $mask >> $c ) & 1 or $pm != 0 or !$node{$c};
			my $n = $node{$c};
			$n = 'Reise' if $n =~ /^(LTG|NBG|SEEK)$/;
			$fr{$n}++; $frames++;
			$att{$n}++ if $a;
			$vis{$n}++ if $v;
			if ( $v && !$a ) {
				$visnot{$n}++;
				$whynot{ $WHY{ $why // -1 } // "?$why" }++ if defined $why;
			}
		} elsif ( /^D (\d+) (\d+) (\d+) (\d+) (\d+) (\d+)/ ) {
			push @d, [ $1, $2, $3, $4 + $5 ] if $3 < 64 && $2 != $3 && $6 != 18;
		} elsif ( /^K (\d+) (\d+) (\d+) / ) {
			push @k, [ $1, $2, $3 ];
		}
	}
	close $in;
	# Gelegenheiten: Treffer je (Taeter, Opfer) zu Serien zusammenfassen
	my %series;
	for my $h ( @d ) {
		my ( $ms, $v, $a, $dmg ) = @$h;
		next if ( $mask >> $a ) & 1;
		my $key = "$a-$v";
		my $s = $series{$key};
		if ( $s && $ms - $s->{last} < 1500 ) { $s->{dmg} += $dmg; $s->{last} = $ms; next }
		close_series( $s, \@k, \%nodeat ) if $s;
		$series{$key} = { a => $a, v => $v, first => $ms, last => $ms, dmg => $dmg };
	}
	close_series( $_, \@k, \%nodeat ) for values %series;
}

sub close_series {
	my ( $s, $k, $nodeat ) = @_;
	return unless $s->{dmg} >= 60;
	# ueberlebt? (kein Tod des Opfers bis 0,3 s nach dem letzten Treffer)
	for my $x ( @$k ) {
		return if $x->[1] == $s->{v} && $x->[0] >= $s->{first} && $x->[0] <= $s->{last} + 300;
	}
	# Taeter selbst tot in dem Moment?
	for my $x ( @$k ) {
		return if $x->[1] == $s->{a} && $x->[0] >= $s->{last} - 200 && $x->[0] <= $s->{last} + 300;
	}
	$opps++;
	my $out = 'entkommt';
	for my $x ( @$k ) {
		next unless $x->[1] == $s->{v} && $x->[0] > $s->{last} + 300 && $x->[0] <= $s->{last} + 10000;
		$out = $x->[2] == $s->{a} ? 'vom Bot erledigt' : 'von einem anderen erledigt';
		last;
	}
	# und vom Taeter erschossen, bevor er nachsetzen konnte?
	for my $x ( @$k ) {
		next unless $x->[1] == $s->{a} && $x->[0] > $s->{last} + 300 && $x->[0] <= $s->{last} + 3000;
		$out = 'Bot selbst tot' if $out eq 'entkommt';
		last;
	}
	$opp{$out}++;
	# Knoten des Taeters in den 2 s danach
	my %n;
	for my $t ( @{ $nodeat->{ $s->{a} } // [] } ) {
		$n{ $t->[1] }++ if $t->[0] > $s->{last} && $t->[0] <= $s->{last} + 2000;
	}
	my ( $top ) = sort { $n{$b} <=> $n{$a} } keys %n;
	$opp{ "  danach meist " . ( $top // '?' ) }++;
}

printf "Feuer (Anteil der Bilder je Knoten):\n%-9s %7s %8s %9s %12s\n", 'Knoten', 'Zeit', 'schiesst', 'sieht ihn', 'sieht, still';
for my $n ( sort { $fr{$b} <=> $fr{$a} } keys %fr ) {
	next if $fr{$n} < $frames * 0.01;
	printf "%-9s %6.1f%% %7.1f%% %8.1f%% %11.1f%%\n", $n, 100 * $fr{$n} / $frames,
		100 * ( $att{$n} // 0 ) / $fr{$n}, 100 * ( $vis{$n} // 0 ) / $fr{$n}, 100 * ( $visnot{$n} // 0 ) / $fr{$n};
}
my ( $a, $v, $vn ) = ( 0, 0, 0 );
$a += $_ for values %att; $v += $_ for values %vis; $vn += $_ for values %visnot;
printf "%-9s %7s %7.1f%% %8.1f%% %11.1f%%\n", 'alle', '', 100 * $a / $frames, 100 * $v / $frames, 100 * $vn / $frames;
if ( %whynot ) {
	my $t = 0; $t += $_ for values %whynot;
	print "Warum still, obwohl er ihn sieht:\n";
	printf "  %-34s %5.1f %%\n", $_, 100 * $whynot{$_} / $t for sort { $whynot{$b} <=> $whynot{$a} } keys %whynot;
}
printf "\nVerfolgen: %d Gelegenheiten (mind. 60 Schaden, Gegner ueberlebt)\n", $opps // 0;
printf "  %-30s %5.1f %%\n", $_, 100 * $opp{$_} / $opps for sort keys %opp;
