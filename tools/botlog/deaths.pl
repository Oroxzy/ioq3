#!/usr/bin/perl
# Was ein Bot getan hat, bevor ihn die Karte umgebracht hat.
#
#   deaths.pl <botlog.log> ...
#   SHOW='FIGHT Sprung' N=5 deaths.pl <botlog.log>     (die Spuren dazu)
#
# Je Tod durch Grube, Lava, Schleim, Sturz oder Quetschen: wo der Bot zuletzt
# festen Boden hatte, in welchem Knoten, und wie er ihn verlassen hat. Mehrere
# Dateien werden zusammengezaehlt.
#
#   Reise / FIGHT   unterwegs zu einem Ziel (LTG, NBG, RETREAT, CHASE) oder im Kampf
#   gelaufen        ueber die Kante gelaufen
#   Sprung          abgesprungen, ohne Sprungfeld
#   getroffen       in den 0,7 Sekunden davor mindestens zehn Leben verloren
#   Sprungfeld      vom Sprungfeld geworfen - mit "Zusammenstoss", wenn er im
#                   Flug schlagartig mehr als die Haelfte des Tempos verliert
#   Weg / frei      ob ihn in dem Moment ein Weg nach der Karte gefuehrt hat
#   Bremse          der Tritt (g_botEdgeCare) hatte kurz davor eingegriffen
use strict;
use warnings;

my ( %c, $shown );
for my $file ( @ARGV ) {
	my ( %t, @k );
	open my $in, '<', $file or die "$file: $!\n";
	while ( <$in> ) {
		if ( /^T (\d+) (\d+) (\S+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (\d) (-?\d+) (-?\d+) (\d+) (-?\d+) (-?\d+) (\d+) (\d+)/ ) {
			push @{ $t{$2} }, { raw => $_, ms => $1, node => $3, sp => $7, vz => $8, ground => $9, hp => $10, tt => $15, flags => $16 };
		} elsif ( /^K (\d+) (\d+) (\d+) (MOD_(?:TRIGGER_HURT|LAVA|SLIME|FALLING|CRUSH))/ ) {
			push @k, { ms => $1, bot => $2, mod => $4 };
		}
	}
	close $in;
	for my $k ( @k ) {
		my @l = grep { $_->{ms} <= $k->{ms} && $_->{ms} >= $k->{ms} - 6000 && $_->{hp} > 0 } @{ $t{ $k->{bot} } // [] };
		next unless @l;
		# der letzte Denkschritt auf festem Boden, vor dem letzten Flug
		my $last = $#l;
		$last-- while $last > 0 && !$l[$last]{ground};
		my $g = $l[$last];
		my @before = grep { $_->{ms} >= $g->{ms} - 700 && $_->{ms} <= $g->{ms} } @l;
		my @after = grep { $_->{ms} > $g->{ms} } @l;
		my $fl = 0;
		$fl |= $_->{flags} for @before, ( @after ? $after[0] : () );
		my $hit = ( $before[0]{hp} - $g->{hp} >= 10 ) || ( @after && $g->{hp} - $after[0]{hp} >= 10 );
		my ( $hitair, $coll ) = ( 0, 0 );
		for my $i ( 1 .. $#after ) {
			$hitair = 1 if $after[ $i - 1 ]{hp} - $after[$i]{hp} >= 10;
			$coll = 1 if $after[ $i - 1 ]{sp} > 300 && $after[$i]{sp} < $after[ $i - 1 ]{sp} * 0.5;
		}
		my $how = !$g->{ground} ? 'nur Flug'
			: $hit ? 'getroffen'
			: ( @after && ( $after[0]{vz} > 300 || $after[0]{sp} > 500 || $g->{tt} == 18 || $g->{vz} > 300 ) )
				? 'Sprungfeld' . ( $coll ? ' Zusammenstoss' : $hitair ? ' im Flug getroffen' : '' )
			: ( $fl & 128 ) ? 'Ausweichschritt'
			: ( @after && $after[0]{vz} > 120 ) ? 'Sprung'
			: $k->{mod} eq 'MOD_CRUSH' ? 'zerquetscht' : 'gelaufen';
		my $route = $g->{tt} ? 'Weg' : 'frei';
		$route = '' if $how =~ /Sprungfeld|nur Flug|zerquetscht/;
		my $nd = $g->{node};
		$nd = 'Reise' if $nd =~ /^(LTG|NBG|BNBG|RETREAT|CHASE)$/;
		if ( $ENV{SHOW} && "$nd $how $route" =~ /$ENV{SHOW}/ && $shown++ < ( $ENV{N} // 3 ) ) {
			print "== $file $k->{mod} bot $k->{bot} \@ $k->{ms}: $nd $how\n";
			print "   $_->{raw}" for grep { $_->{ms} >= $g->{ms} - 800 && $_->{ms} <= $g->{ms} + 1200 } @l;
		}
		$c{ sprintf '%-6s %-30s %-5s%s', $nd, $how, $route, ( $fl & 1 ) ? ' Bremse' : '' }++;
	}
}
my $total = 0;
$total += $_ for values %c;
printf "%d Tode durch die Karte\n", $total;
printf "%4d  %s\n", $c{$_}, $_ for sort { $c{$b} <=> $c{$a} } keys %c;
