#!/usr/bin/perl
# Die letzten Sekunden vor jedem Tod durch die Karte, Zeile fuer Zeile.
#
#   trace.pl <botlog.log> [<sekunden>] [<filter>]
#
# Der Filter ist ein regulaerer Ausdruck auf "<MOD> <Knoten>", etwa
# 'LAVA' oder 'TRIGGER_HURT FIGHT'. Gezeigt werden die T-Zeilen des Opfers
# und seine B- (Bremse) und U-Zeilen (gestrandet).
use strict;
use warnings;

my ( $file, $secs, $filter ) = @ARGV;
$secs ||= 2.5;
my ( %l, @k );
open my $in, '<', $file or die "$file: $!\n";
while ( <$in> ) {
	chomp; s/\r$//;
	if ( /^([TBU]) (\d+) (\d+) / ) { push @{ $l{$3} }, [ $2, $_ ]; }
	elsif ( /^K (\d+) (\d+) (\d+) (MOD_(?:TRIGGER_HURT|LAVA|SLIME|FALLING|CRUSH))/ ) { push @k, [ $1, $2, $4 ]; }
}
close $in;
for my $k ( @k ) {
	my ( $ms, $bot, $mod ) = @$k;
	my @w = grep { $_->[0] <= $ms && $_->[0] >= $ms - $secs * 1000 } @{ $l{$bot} // [] };
	my ( $node ) = map { $_->[1] =~ /^T \d+ \d+ (\S+)/ ? $1 : () } reverse @w;
	next if $filter && "$mod " . ( $node // '' ) !~ /$filter/;
	print "== $mod bot $bot bei $ms\n";
	print "   $_->[1]\n" for @w;
}
