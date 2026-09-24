#!/usr/bin/perl
use strict;
use warnings;

# Wandelt eine DXT1-komprimierte DDS-Datei in ein unkomprimiertes TGA.
#
#   perl dds2tga.pl <quelle.dds> <ziel.tga>
#
# DXT1 ist blockweise: je 4x4 Pixel acht Byte - zwei Farben in RGB565 und
# sechzehn Zwei-Bit-Indizes darauf. Ist die erste Farbe groesser als die
# zweite, wird zwischen beiden in Dritteln interpoliert; sonst liegt eine
# Zwischenfarbe in der Mitte und der vierte Index ist durchsichtig. Quake
# braucht kein Alpha an einer Waffe, also wird das schlicht schwarz.

my ($src, $dst) = @ARGV;
die "Aufruf: dds2tga.pl <quelle.dds> <ziel.tga>\n" unless defined $dst;

open my $f, "<:raw", $src or die "$src: $!";
my $d = do { local $/; <$f> };
close $f;

die "$src: kein DDS\n" unless substr($d, 0, 4) eq "DDS ";
my ($size, $flags, $h, $w, $pitch, $depth, $mips) = unpack("x4 V V V V V V V", $d);
my $fourcc = substr($d, 84, 4);
die "$src: $fourcc, nur DXT1 wird unterstuetzt\n" unless $fourcc eq "DXT1";

my $at = 128;
my @px;						# [y][x] = [r,g,b]

sub rgb565 {
	my ($c) = @_;
	return ( ( ( $c >> 11 ) & 31 ) * 255 / 31,
	         ( ( $c >>  5 ) & 63 ) * 255 / 63,
	         (   $c         & 31 ) * 255 / 31 );
}

for ( my $by = 0; $by < ( $h + 3 ) / 4; $by++ ) {
	for ( my $bx = 0; $bx < ( $w + 3 ) / 4; $bx++ ) {
		my ($c0, $c1, $bits) = unpack("v v V", substr($d, $at, 8));
		$at += 8;
		my @a = rgb565($c0);
		my @b = rgb565($c1);
		my @c;
		if ( $c0 > $c1 ) {
			@c = ( \@a, \@b,
			       [ map { ( 2 * $a[$_] + $b[$_] ) / 3 } 0 .. 2 ],
			       [ map { ( $a[$_] + 2 * $b[$_] ) / 3 } 0 .. 2 ] );
		} else {
			@c = ( \@a, \@b,
			       [ map { ( $a[$_] + $b[$_] ) / 2 } 0 .. 2 ],
			       [ 0, 0, 0 ] );
		}
		for my $y ( 0 .. 3 ) {
			for my $x ( 0 .. 3 ) {
				my $i = ( $bits >> ( 2 * ( 4 * $y + $x ) ) ) & 3;
				my ($px, $py) = ( $bx * 4 + $x, $by * 4 + $y );
				next if $px >= $w || $py >= $h;
				$px[$py][$px] = $c[$i];
			}
		}
	}
}

# TGA, unkomprimiert, 24 bit, von unten nach oben - so erwartet es Quake
my $out = pack("C C C v v C v v v v C C", 0, 0, 2, 0, 0, 0, 0, 0, $w, $h, 24, 0);
for ( my $y = $h - 1; $y >= 0; $y-- ) {
	for my $x ( 0 .. $w - 1 ) {
		my $p = $px[$y][$x] || [0, 0, 0];
		$out .= pack("C3", map { int($p->[$_] + 0.5) } 2, 1, 0);		# BGR
	}
}

open my $o, ">:raw", $dst or die "$dst: $!";
print $o $out;
close $o;
printf "%s geschrieben: %dx%d, %d Bytes\n", $dst, $w, $h, length $out;
