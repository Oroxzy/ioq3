#!/usr/bin/perl
# Eine Zeile je Lauf, fuer den Vergleich vieler Karten.
#
#   sweep.pl <laufordner> ...      (jeder mit baseq3/botlog.log und qconsole.log)
#
# Spalten: Kills je Bot-Minute, Tode durch Karte (Grube, Lava, Schleim,
# Sturz, Quetschen) in Prozent aller Tode, Stillstand, kein Ziel, gestrandet,
# blockiert, Weg gescheitert (Anteile der Lebenszeit), Zappeln ausserhalb des
# Kampfs, Pingpong je Bot-Minute, Fernziele je Bot-Minute.
use strict;
use warnings;

my %IDLE = map { $_ => 1 } qw( NONE RESPAWN OBS INTER STAND );
my %FIGHT = map { $_ => 1 } qw( FIGHT BNBG RETREAT CHASE );

printf "%-24s %6s %6s %6s %6s %6s %6s %6s %6s %6s %6s %5s\n",
	'Lauf', 'K/min', 'Karte%', 'still', 'kZiel', 'gestr', 'block', 'Wegx', 'zappl', 'pingp', 'ferns', 'Bots';
for my $dir ( @ARGV ) {
	my $log = "$dir/baseq3/botlog.log";
	my $con = "$dir/baseq3/qconsole.log";
	( my $name = $dir ) =~ s{/+$}{}; $name =~ s{.*/}{};
	unless ( -f $log ) { printf "%-24s  kein botlog.log\n", $name; next; }

	my ( $kills, $deaths, $world ) = ( 0, 0, 0 );
	if ( open my $c, '<', $con ) {
		while ( <$c> ) {
			next unless /^Kill: (\d+) (\d+) (\d+): .* by (MOD_\w+)/;
			$deaths++;
			my $mod = $4;
			if ( $mod =~ /^MOD_(TRIGGER_HURT|LAVA|SLIME|FALLING|CRUSH)$/ ) { $world++ }
			else { $kills++ }
		}
		close $c;
	}

	my ( %t, %s, %g );
	open my $in, '<', $log or die;
	while ( <$in> ) {
		if ( /^T (\d+) (\d+) (\S+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (\d) (-?\d+) (-?\d+) (\d+) (-?\d+) (-?\d+) (\d+) (\d+)/ ) {
			push @{ $t{$2} }, { ms => $1, node => $3, x => $4, y => $5, speed => $7, ground => $9, hp => $10, flags => $16 };
		} elsif ( /^S (\d+) (\d+) (.+?) \|/ ) {
			push @{ $s{$2} }, { ms => $1, node => $3 };
		} elsif ( /^G (\d+) (\d+) ([LH]) / ) {
			$g{$2}++;
		}
	}
	close $in;

	my ( $alive, $still, $nogoal, $stranded, $blocked, $fail, $dither ) = ( 0 ) x 7;
	for my $b ( keys %t ) {
		my @l = @{ $t{$b} };
		my @cum = ( 0 );
		for my $i ( 1 .. $#l ) {
			my $d = sqrt( ( $l[$i]{x} - $l[ $i - 1 ]{x} )**2 + ( $l[$i]{y} - $l[ $i - 1 ]{y} )**2 );
			$d = 0 if $d > 120;
			$cum[$i] = $cum[ $i - 1 ] + $d;
		}
		my ( $j, @dith ) = ( 0 );
		for my $i ( 0 .. $#l ) {
			my $e = $l[$i];
			next if $IDLE{ $e->{node} } || $e->{hp} <= 0;
			$alive++;
			$still++ if $e->{ground} && $e->{speed} < 20;
			$nogoal++ if $e->{flags} & 8;
			$stranded++ if $e->{flags} & 64;
			$blocked++ if $e->{flags} & 4;
			$fail++ if $e->{flags} & 2;
			next if $FIGHT{ $e->{node} };
			$j = $i if $j < $i;
			$j++ while $j < $#l && $l[ $j + 1 ]{ms} - $e->{ms} <= 2000;
			next if $l[$j]{ms} - $e->{ms} < 1800;
			my $path = $cum[$j] - $cum[$i];
			my $net = sqrt( ( $l[$j]{x} - $e->{x} )**2 + ( $l[$j]{y} - $e->{y} )**2 );
			$dither++ if $path >= 150 && $net <= 60;
		}
	}
	my $pingpong = 0;
	for my $b ( keys %s ) {
		my @l = @{ $s{$b} };
		for my $i ( 2 .. $#l ) {
			$pingpong++ if $l[$i]{node} eq $l[ $i - 2 ]{node} && $l[$i]{node} ne $l[ $i - 1 ]{node}
				&& $l[$i]{ms} - $l[ $i - 2 ]{ms} <= 1000;
		}
	}
	my $ltg = 0; $ltg += $_ for values %g;
	my $min = $alive / 600 || 1;		# Denkschritte zu Bot-Minuten
	printf "%-24s %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %6.1f %5d\n",
		$name, $kills / $min, $deaths ? 100 * $world / $deaths : 0,
		100 * $still / $alive, 100 * $nogoal / $alive, 100 * $stranded / $alive,
		100 * $blocked / $alive, 100 * $fail / $alive, 100 * $dither / $alive,
		$pingpong / $min, $ltg / $min, scalar keys %t;
}
