#!/usr/bin/perl
# Wertet das Bot-Protokoll aus (g_botLog 1, botlog.log im Homeverzeichnis).
#
#   botlog.pl [-e] <botlog.log> [<botlog.log> ...]
#
# Mehrere Dateien stehen nebeneinander, eine Spalte je Lauf. -e listet dazu
# die laengsten Episoden jedes Laufs einzeln auf.
#
# Was gezaehlt wird, und warum so:
#
# Stillstand  der Bot lebt, steht auf dem Boden, laeuft langsamer als 20
#             Einheiten je Sekunde, mindestens eine halbe Sekunde am Stueck.
# Zappeln     in zwei Sekunden ueber 150 Einheiten Weg gemacht und dabei
#             weniger als 60 von der Stelle gekommen. Im Kampf ist das
#             gewollt (Umkreisen), deshalb steht der Kampf getrennt.
# Kehrtwende  die Laufrichtung dreht zwischen zwei Denkschritten um mehr als
#             120 Grad, bei ueber 50 Einheiten je Sekunde davor und danach.
# Zielwechsel ein neues Fernziel weniger als eine Sekunde nach dem letzten.
# Pingpong    Knoten A, dann B, dann wieder A, alles innerhalb einer Sekunde.
use strict;
use warnings;

my $episodes = 0;
if ( @ARGV && $ARGV[0] eq '-e' ) { $episodes = 1; shift @ARGV; }
die "botlog.pl [-e] <botlog.log> ...\n" unless @ARGV;

my %TT = ( 0 => '-', 2 => 'walk', 3 => 'crouch', 4 => 'barrier', 5 => 'jump', 6 => 'ladder',
	7 => 'ledge', 8 => 'swim', 9 => 'waterjump', 10 => 'teleport', 11 => 'elevator',
	12 => 'rocketjump', 13 => 'bfgjump', 14 => 'grapple', 18 => 'jumppad', 19 => 'funcbob' );
my @FLAG = ( [ 1, 'Bremse' ], [ 2, 'Weg gescheitert' ], [ 4, 'blockiert' ],
	[ 8, 'kein Ziel' ], [ 16, 'keine Kampfbewegung' ], [ 32, q(Ziel fallengelassen) ], [ 64, q(gestrandet) ], [ 128, q(ausgewichen) ], [ 256, q(in der Luft umgesteuert) ] );
my %IDLE = map { $_ => 1 } qw( NONE RESPAWN OBS INTER STAND );
my %FIGHT = map { $_ => 1 } qw( FIGHT BNBG RETREAT CHASE );

my @runs;
for my $file ( @ARGV ) {
	push @runs, analyse( $file );
}

report( @runs );
exit 0;

sub analyse {
	my ( $file ) = @_;
	my %r = ( file => $file );
	my ( %t, %s, %g );		# je Bot: Denkschritte, Knotenwechsel, Zielwahlen
	my @items;

	open my $in, '<', $file or die "$file: $!\n";
	while ( my $line = <$in> ) {
		$line =~ s/\r?\n$//;
		if ( $line =~ /^T (\d+) (\d+) (\S+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (-?\d+) (\d) (-?\d+) (-?\d+) (\d+) (-?\d+) (-?\d+) (\d+) (\d+) (\d+)$/ ) {
			push @{ $t{$2} }, { ms => $1, node => $3, x => $4, y => $5, z => $6, speed => $7,
				vz => $8, ground => $9, hp => $10, armor => $11, weapon => $12, enemy => $13,
				goal => $14, tt => $15, flags => $16, sw => $17 };
		} elsif ( $line =~ /^S (\d+) (\d+) (.+?) \| (.*)$/ ) {
			push @{ $s{$2} }, { ms => $1, node => $3, why => $4 };
		} elsif ( $line =~ /^G (\d+) (\d+) (\S) (\d+) (\d+) (-?\d+) (-?\d+) (-?\d+) (\S+) \| (.*)$/ ) {
			push @{ $g{$2} }, { ms => $1, kind => $3, num => $4, flags => $5, why => $9, name => $10 };
		} elsif ( $line =~ /^I (\d+) (\d+) (-?\d+) (\d+) (\S+)(?: (\d))?$/ ) {
			push @items, { ms => $1, ent => $2, who => $3, respawn => $4, class => $5, dropped => $6 // 0 };
		}
	}
	close $in;

	my ( $alive, $moving_sum, $moving_n ) = ( 0, 0, 0 );
	my ( %nodetime, %stall_node, %stall_flag, %dither_node, %dither_flag, %rev_node );
	my ( $stall, $stall_eps, $dither_fight, $dither_other, $dither_eps ) = ( 0, 0, 0, 0, 0 );
	my ( $rev_fight, $rev_other, $alive_fight, $alive_other ) = ( 0, 0, 0, 0 );
	my ( %flagcount, %brake_tt, %tt_time );
	my ( $air, @stall_list, @dither_list );

	for my $bot ( sort { $a <=> $b } keys %t ) {
		my @l = @{ $t{$bot} };
		my ( @run, @mark );
		for my $i ( 0 .. $#l ) {
			my $e = $l[$i];
			my $dt = $i < $#l ? ( $l[ $i + 1 ]{ms} - $e->{ms} ) / 1000 : 0.1;
			$dt = 0.1 if $dt <= 0 || $dt > 0.5;
			$e->{dt} = $dt;
			$e->{alive} = ( !$IDLE{ $e->{node} } && $e->{hp} > 0 ) ? 1 : 0;
			next unless $e->{alive};
			$alive += $dt;
			$nodetime{ $e->{node} } += $dt;
			$tt_time{ $e->{tt} } += $dt;
			$air += $dt unless $e->{ground};
			if ( $FIGHT{ $e->{node} } ) { $alive_fight += $dt } else { $alive_other += $dt }
			if ( $e->{speed} > 20 ) { $moving_sum += $e->{speed} * $dt; $moving_n += $dt; }
			for my $f ( @FLAG ) {
				$flagcount{ $f->[1] } += $dt if $e->{flags} & $f->[0];
			}
			$brake_tt{ $e->{tt} } += $dt if $e->{flags} & 1;
		}

		# Stillstand: Laeufe langsamer Denkschritte
		my $from = -1;
		for my $i ( 0 .. $#l + 1 ) {
			my $slow = $i <= $#l && $l[$i]{alive} && $l[$i]{ground} && $l[$i]{speed} < 20;
			if ( $slow ) { $from = $i if $from < 0; next; }
			if ( $from >= 0 ) {
				my $len = 0;
				$len += $l[$_]{dt} for $from .. $i - 1;
				if ( $len >= 0.5 ) {
					$stall += $len;
					$stall_eps++;
					my ( %n, $fl );
					$fl = 0;
					for ( $from .. $i - 1 ) { $n{ $l[$_]{node} } += $l[$_]{dt}; $fl |= $l[$_]{flags}; }
					my ( $node ) = sort { $n{$b} <=> $n{$a} } keys %n;
					$stall_node{$node} += $len;
					for my $f ( @FLAG ) { $stall_flag{ $f->[1] } += $len if $fl & $f->[0]; }
					# 32 ist keine Ursache, sondern eine Eigenschaft des Ziels
					$stall_flag{'ohne Flagge'} += $len unless $fl & ( 1 | 2 | 4 | 8 | 16 | 64 | 128 );
					push @stall_list, { bot => $bot, ms => $l[$from]{ms}, len => $len, node => $node,
						flags => $fl, x => $l[$from]{x}, y => $l[$from]{y}, z => $l[$from]{z},
						tt => $l[$from]{tt}, goal => $l[$from]{goal} };
				}
				$from = -1;
			}
		}

		# Zappeln: Fenster von zwei Sekunden, viel Weg und kaum Strecke
		my @cum = ( 0 );
		for my $i ( 1 .. $#l ) {
			my ( $a, $b ) = ( $l[ $i - 1 ], $l[$i] );
			my $d = sqrt( ( $b->{x} - $a->{x} )**2 + ( $b->{y} - $a->{y} )**2 );
			# ein Respawn oder Teleporter ist kein Weg
			$d = 0 if $d > 120 || !$a->{alive} || !$b->{alive};
			$cum[$i] = $cum[ $i - 1 ] + $d;
		}
		my @dith = ( 0 ) x @l;
		my $j = 0;
		for my $i ( 0 .. $#l ) {
			next unless $l[$i]{alive};
			$j = $i if $j < $i;
			$j++ while $j < $#l && $l[ $j + 1 ]{ms} - $l[$i]{ms} <= 2000 && $l[ $j + 1 ]{alive};
			next if $l[$j]{ms} - $l[$i]{ms} < 1800;
			my $path = $cum[$j] - $cum[$i];
			my $net = sqrt( ( $l[$j]{x} - $l[$i]{x} )**2 + ( $l[$j]{y} - $l[$i]{y} )**2 );
			if ( $path >= 150 && $net <= 60 ) { $dith[$_] = 1 for $i .. $j; }
		}
		$from = -1;
		for my $i ( 0 .. $#l + 1 ) {
			if ( $i <= $#l && $dith[$i] ) { $from = $i if $from < 0; next; }
			if ( $from >= 0 ) {
				my ( $len, %n, $fl ) = ( 0 );
				$fl = 0;
				for ( $from .. $i - 1 ) { $len += $l[$_]{dt}; $n{ $l[$_]{node} } += $l[$_]{dt}; $fl |= $l[$_]{flags}; }
				my ( $node ) = sort { $n{$b} <=> $n{$a} } keys %n;
				$dither_eps++;
				$dither_node{$node} += $len;
				if ( $FIGHT{$node} ) { $dither_fight += $len } else { $dither_other += $len }
				for my $f ( @FLAG ) { $dither_flag{ $f->[1] } += $len if $fl & $f->[0]; }
				push @dither_list, { bot => $bot, ms => $l[$from]{ms}, len => $len, node => $node,
					flags => $fl, x => $l[$from]{x}, y => $l[$from]{y}, z => $l[$from]{z},
					tt => $l[$from]{tt}, goal => $l[$from]{goal} };
				$from = -1;
			}
		}

		# Kehrtwenden
		for my $i ( 2 .. $#l ) {
			my ( $a, $b, $c ) = @l[ $i - 2, $i - 1, $i ];
			next unless $a->{alive} && $b->{alive} && $c->{alive};
			next unless $b->{speed} > 50 && $c->{speed} > 50 && $b->{ground} && $c->{ground};
			my ( $x1, $y1 ) = ( $b->{x} - $a->{x}, $b->{y} - $a->{y} );
			my ( $x2, $y2 ) = ( $c->{x} - $b->{x}, $c->{y} - $b->{y} );
			my $n1 = sqrt( $x1 * $x1 + $y1 * $y1 );
			my $n2 = sqrt( $x2 * $x2 + $y2 * $y2 );
			next if $n1 < 4 || $n2 < 4 || $n1 > 120 || $n2 > 120;
			if ( ( $x1 * $x2 + $y1 * $y2 ) / ( $n1 * $n2 ) < -0.5 ) {
				$rev_node{ $c->{node} }++;
				if ( $FIGHT{ $c->{node} } ) { $rev_fight++ } else { $rev_other++ }
			}
		}
	}

	# Zielwahl
	my ( $ltg, $ltg_fast, $ltg_none, $nbg, $ltg_dropped, $hunt, %why, %why_fast, %goalname ) = ( 0, 0, 0, 0, 0, 0 );
	for my $bot ( keys %g ) {
		my $last;
		for my $e ( @{ $g{$bot} } ) {
			if ( $e->{kind} eq 'N' ) { $nbg++; next; }
			# dem Laerm nach: kein Gegenstand, also weder Name noch Fernziel-Statistik
			if ( $e->{kind} eq 'H' ) { $hunt++; $last = $e->{ms}; next; }
			if ( $e->{kind} eq '-' ) { $ltg_none++; $why{ $e->{why} . ' (keins)' }++; next; }
			$ltg++;
			$why{ $e->{why} }++;
			$goalname{ $e->{name} }++;
			$ltg_dropped++ if $e->{flags} & 4;
			if ( defined $last && $e->{ms} - $last < 1000 ) { $ltg_fast++; $why_fast{ $e->{why} }++; }
			$last = $e->{ms};
		}
	}

	# Knotenwechsel
	my ( $switches, $pingpong, %sw_why, %pp_why ) = ( 0, 0 );
	for my $bot ( keys %s ) {
		my @l = @{ $s{$bot} };
		$switches += @l;
		for my $i ( 0 .. $#l ) {
			$sw_why{ "$l[$i]{node} <- $l[$i]{why}" }++;
			next if $i < 2;
			if ( $l[$i]{node} eq $l[ $i - 2 ]{node} && $l[$i]{node} ne $l[ $i - 1 ]{node}
				&& $l[$i]{ms} - $l[ $i - 2 ]{ms} <= 1000 ) {
				$pingpong++;
				$pp_why{ "$l[$i]{node} <-> $l[$i-1]{node}: $l[$i-1]{why} / $l[$i]{why}" }++;
			}
		}
	}

	# Gegenstaende: wie lange liegt ein grosser herum, bis ihn jemand holt
	my ( %taken, %wait, %picks );
	for my $e ( @items ) {
		$picks{ $e->{class} }++;
		# Was ein Toter fallen liess, kommt nicht wieder, und seine
		# Entitaetsnummer wird neu vergeben.
		if ( $e->{dropped} ) {
			delete $taken{ $e->{ent} };
			next;
		}
		my $p = $taken{ $e->{ent} };
		if ( $p && $p->{class} eq $e->{class} ) {
			my $w = ( $e->{ms} - $p->{ms} ) / 1000;
			# negativ heisst: eine andere Entitaet auf derselben Nummer, in
			# einem Protokoll von vor der Marke
			push @{ $wait{ $e->{class} } }, $w if $w >= 0;
		}
		$taken{ $e->{ent} } = { ms => $e->{ms} + $e->{respawn} * 1000, class => $e->{class} };
	}

	my $min = $alive / 60 || 1;
	%r = ( %r,
		bots => scalar( keys %t ), alive => $alive, min => $min,
		speed => $moving_n ? $moving_sum / $moving_n : 0,
		air => ( $air || 0 ) / ( $alive || 1 ),
		nodetime => \%nodetime, tt_time => \%tt_time,
		stall => $stall, stall_eps => $stall_eps, stall_node => \%stall_node, stall_flag => \%stall_flag,
		dither_fight => $dither_fight, dither_other => $dither_other, dither_eps => $dither_eps,
		dither_node => \%dither_node, dither_flag => \%dither_flag,
		rev_fight => $rev_fight, rev_other => $rev_other, rev_node => \%rev_node,
		alive_fight => $alive_fight, alive_other => $alive_other,
		flagcount => \%flagcount, brake_tt => \%brake_tt,
		ltg => $ltg, ltg_fast => $ltg_fast, ltg_none => $ltg_none, nbg => $nbg, ltg_dropped => $ltg_dropped,
		hunt => $hunt,
		why => \%why, why_fast => \%why_fast, goalname => \%goalname,
		switches => $switches, pingpong => $pingpong, sw_why => \%sw_why, pp_why => \%pp_why,
		picks => \%picks, wait => \%wait,
		stall_list => \@stall_list, dither_list => \@dither_list,
	);
	return \%r;
}

sub pct { my ( $a, $b ) = @_; return $b ? sprintf( '%.1f %%', 100 * $a / $b ) : '-'; }
sub num { my ( $a, $f ) = @_; return sprintf( $f // '%.1f', $a // 0 ); }

sub row {
	my ( $label, @cells ) = @_;
	printf "%-34s", $label;
	printf " %14s", $_ for @cells;
	print "\n";
}

sub section {
	my ( $title ) = @_;
	print "\n$title\n", '-' x length( $title ), "\n";
}

# eine Tabelle aus einem Hash je Lauf: alle Schluessel, die irgendwo vorkommen
sub hashrows {
	my ( $runs, $key, $fmt, $limit ) = @_;
	my %all;
	for my $r ( @$runs ) { $all{$_} += $r->{$key}{$_} for keys %{ $r->{$key} }; }
	my @keys = sort { $all{$b} <=> $all{$a} } keys %all;
	@keys = @keys[ 0 .. $limit - 1 ] if $limit && @keys > $limit;
	for my $k ( @keys ) {
		row( "  $k", map { $fmt->( $_, $_->{$key}{$k} // 0 ) } @$runs );
	}
}

sub report {
	my @r = @_;

	row( '', map { my $n = $_->{file}; $n =~ s{[/\\]baseq3[/\\]botlog\.log$}{}; $n =~ s{.*[/\\]}{}; $n } @r );
	row( 'Bots', map { $_->{bots} } @r );
	row( 'Lebenszeit aller Bots (min)', map { num( $_->{min} ) } @r );
	row( 'Tempo im Lauf (u/s)', map { num( $_->{speed}, '%.0f' ) } @r );
	row( 'in der Luft', map { pct( $_->{air}, 1 ) } @r );

	section( 'Wo die Zeit hingeht (Anteil der Lebenszeit)' );
	hashrows( \@r, 'nodetime', sub { pct( $_[1], $_[0]{alive} ) } );

	section( 'Stillstand' );
	row( 'Anteil der Lebenszeit', map { pct( $_->{stall}, $_->{alive} ) } @r );
	row( 'Episoden je Bot-Minute', map { num( $_->{stall_eps} / $_->{min}, '%.2f' ) } @r );
	print " nach Knoten (Anteil der Lebenszeit):\n";
	hashrows( \@r, 'stall_node', sub { pct( $_[1], $_[0]{alive} ) } );
	print " mit Flagge (Anteil des Stillstands):\n";
	hashrows( \@r, 'stall_flag', sub { pct( $_[1], $_[0]{stall} ) } );

	section( 'Zappeln' );
	row( 'ausserhalb des Kampfs', map { pct( $_->{dither_other}, $_->{alive_other} ) } @r );
	row( 'im Kampf', map { pct( $_->{dither_fight}, $_->{alive_fight} ) } @r );
	row( 'Episoden je Bot-Minute', map { num( $_->{dither_eps} / $_->{min}, '%.2f' ) } @r );
	print " nach Knoten (Anteil der Lebenszeit):\n";
	hashrows( \@r, 'dither_node', sub { pct( $_[1], $_[0]{alive} ) } );
	print " mit Flagge (Anteil des Zappelns):\n";
	hashrows( \@r, 'dither_flag', sub { pct( $_[1], $_[0]{dither_other} + $_[0]{dither_fight} ) } );

	section( 'Kehrtwenden je Minute' );
	row( 'ausserhalb des Kampfs', map { num( $_->{rev_other} / ( $_->{alive_other} / 60 || 1 ), '%.2f' ) } @r );
	row( 'im Kampf', map { num( $_->{rev_fight} / ( $_->{alive_fight} / 60 || 1 ), '%.2f' ) } @r );

	section( 'Zielwahl' );
	row( 'Fernziele je Bot-Minute', map { num( $_->{ltg} / $_->{min}, '%.2f' ) } @r );
	row( '  davon < 1 s nach dem letzten', map { pct( $_->{ltg_fast}, $_->{ltg} ) } @r );
	row( '  davon fallengelassen', map { pct( $_->{ltg_dropped}, $_->{ltg} ) } @r );
	row( 'Wahl ohne Ergebnis je Bot-Minute', map { num( $_->{ltg_none} / $_->{min}, '%.2f' ) } @r );
	row( 'Nahziele je Bot-Minute', map { num( $_->{nbg} / $_->{min}, '%.2f' ) } @r );
	row( 'dem Laerm nach, je Bot-Minute', map { num( $_->{hunt} / $_->{min}, '%.2f' ) } @r );
	print " Grund der Fernziel-Wahl (Anteil):\n";
	hashrows( \@r, 'why', sub { pct( $_[1], $_[0]{ltg} + $_[0]{ltg_none} ) } );
	print " Grund bei den schnellen Wechseln (Anteil):\n";
	hashrows( \@r, 'why_fast', sub { pct( $_[1], $_[0]{ltg_fast} ) } );
	print " gewaehlte Fernziele (Anteil):\n";
	hashrows( \@r, 'goalname', sub { pct( $_[1], $_[0]{ltg} ) } );

	section( 'Knotenwechsel' );
	row( 'je Bot-Minute', map { num( $_->{switches} / $_->{min}, '%.2f' ) } @r );
	row( 'Pingpong je Bot-Minute', map { num( $_->{pingpong} / $_->{min}, '%.2f' ) } @r );
	print " die haeufigsten Pingpong-Paare (je Bot-Minute):\n";
	hashrows( \@r, 'pp_why', sub { num( $_[1] / $_[0]{min}, '%.2f' ) }, 8 );

	section( 'Flaggen (Anteil der Lebenszeit)' );
	hashrows( \@r, 'flagcount', sub { pct( $_[1], $_[0]{alive} ) } );
	print " Bremse nach Reiseart (Sekunden):\n";
	for my $tt ( sort { $a <=> $b } keys %{ { map { %{ $_->{brake_tt} } } @r } } ) {
		row( '  ' . ( $TT{$tt} // $tt ), map { num( $_->{brake_tt}{$tt} // 0 ) } @r );
	}
	print " Reiseart (Anteil der Lebenszeit):\n";
	for my $tt ( sort { $a <=> $b } keys %{ { map { %{ $_->{tt_time} } } @r } } ) {
		row( '  ' . ( $TT{$tt} // $tt ), map { pct( $_->{tt_time}{$tt} // 0, $_->{alive} ) } @r );
	}

	section( 'Gegenstaende: aufgenommen, und wie lange sie vorher lagen (Median in s)' );
	my %classes;
	for my $r ( @r ) { $classes{$_} += $r->{picks}{$_} for keys %{ $r->{picks} }; }
	for my $c ( sort { $classes{$b} <=> $classes{$a} } keys %classes ) {
		row( "  $c", map {
			my @w = sort { $a <=> $b } @{ $_->{wait}{$c} // [] };
			sprintf( '%d / %s', $_->{picks}{$c} // 0, @w ? num( $w[ int( @w / 2 ) ] ) : '-' )
		} @r );
	}

	return unless $episodes;
	for my $r ( @r ) {
		for my $kind ( [ 'stall_list', 'Stillstand' ], [ 'dither_list', 'Zappeln' ] ) {
			section( "$r->{file}: laengste Episoden, $kind->[1]" );
			my @l = sort { $b->{len} <=> $a->{len} } @{ $r->{ $kind->[0] } };
			@l = @l[ 0 .. 24 ] if @l > 25;
			for my $e ( @l ) {
				my @f = map { $_->[1] } grep { $e->{flags} & $_->[0] } @FLAG;
				printf "  %6.1f s  Bot %2d  bei %7.1f s  %-8s (%5d %5d %5d)  Ziel %3d  %-10s %s\n",
					$e->{len}, $e->{bot}, $e->{ms} / 1000, $e->{node}, $e->{x}, $e->{y}, $e->{z},
					$e->{goal}, $TT{ $e->{tt} } // $e->{tt}, join( ', ', @f );
			}
		}
	}
}
