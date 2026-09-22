#!/usr/bin/perl
use strict; use warnings;
my ($pak, $mode, $want, $out) = @ARGV;
# Listet oder entpackt eine Quake-1/2-PAK-Datei.
#   perl pak.pl <pak> list [Regex]
#   perl pak.pl <pak> get  <Pfad im pak> <Zieldatei>
# Das Format ist schlicht: "PACK", Verzeichnis-Offset und -Laenge, danach
# Eintraege zu je 64 Byte - Name (56), Offset, Laenge.
open my $f, "<:raw", $pak or die "$pak: $!";
read $f, my $h, 12;
my ($magic, $off, $len) = unpack("A4 V V", $h);
die "kein PACK: $pak\n" unless $magic eq "PACK";
seek $f, $off, 0;
read $f, my $dir, $len;
for my $i (0 .. $len/64 - 1) {
	my ($name, $fpos, $flen) = unpack("Z56 V V", substr($dir, $i*64, 64));
	if ($mode eq "list") {
		printf "%-42s %7d\n", $name, $flen if !$want || $name =~ /$want/;
	} elsif ($mode eq "get" && $name eq $want) {
		seek $f, $fpos, 0; read $f, my $data, $flen;
		open my $o, ">:raw", $out or die; print $o $data; close $o;
		printf "%s -> %s (%d Bytes)\n", $name, $out, $flen;
		exit 0;
	}
}
exit($mode eq "get" ? 1 : 0);
