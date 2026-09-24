#!/usr/bin/perl
use strict;
use warnings;

# Wandelt ein Call-of-Duty-1-Modell (xmodelsurfs, Version 14) in ein Quake-3-MD3.
#
#   perl xmodel2md3.pl <xmodelsurfs> <ziel.md3> [--shader PFAD] [--scale F]
#                      [--axes xzy|xyz] [--flipv] [--info]
#
# Das Quellformat ist nirgends dokumentiert; es wurde aus sechs Dateien
# zurueckgerechnet, und die Probe darauf ist, dass die Dateilaenge bei allen
# sechs exakt aufgeht. Aufbau, alles little endian:
#
#   Datei:   u16 Version (14), u16 Zahl der Flaechen
#   Flaeche: u8 Flags, u16 Vertices, u16 Dreiecke, u16 Streifen, i16 Knochen
#            steht dort -1, folgen vier weitere Bytes und die Vertices sind
#            40 statt 32 Byte breit, mit Gewichten in einem Anhang
#   Streifen: je u8 Laenge, dann so viele u16 Indizes (Dreiecksstreifen)
#   Vertex (starr, 32 Byte): 3 float Normale, 2 float UV, 3 float Position
#
# Die Wicklung ist bei geradem k (b,a,c) und bei ungeradem (a,b,c) - anders
# herum zeigen alle Dreiecke nach innen. Entartete Dreiecke (zwei gleiche
# Indizes) sind die Naehte zwischen den Streifen und fallen weg; erst dann
# stimmt die Zahl im Kopf.

my ($src, $dst, %opt);
{
	my @a = @ARGV;
	while (@a) {
		my $t = shift @a;
		if    ($t eq "--shader") { $opt{shader} = shift @a }
		elsif ($t eq "--shaders"){ @{$opt{shaders}} = split /,/, shift @a }
		elsif ($t eq "--scale")  { $opt{scale}  = shift @a }
		elsif ($t eq "--axes")   { $opt{axes}   = shift @a }
		elsif ($t eq "--flipv")  { $opt{flipv}  = 1 }
		elsif ($t eq "--info")   { $opt{info}   = 1 }
		elsif (!defined $src)    { $src = $t }
		else                     { $dst = $t }
	}
}
die "Aufruf: xmodel2md3.pl <xmodelsurfs> <ziel.md3> [Optionen]\n" unless defined $src;
$opt{shader} //= "models/weapons2/machinegun/machinegun";
$opt{scale}  //= 1.0;
$opt{axes}   //= "xzy";

open my $fh, "<:raw", $src or die "$src: $!";
my $d = do { local $/; <$fh> };
close $fh;

my ($ver, $nSurf) = unpack("v v", $d);
die "$src: Version $ver, erwartet 14\n" unless $ver == 14;
my $at = 4;
my @surfaces;

for my $s (0 .. $nSurf - 1) {
	my ($flags, $nVert, $nTri, $nStrip, $bone) = unpack("C v v v s<", substr($d, $at, 9));
	$at += 9;
	my $weighted = ($bone == -1);
	$at += 4 if $weighted;

	# Dreiecksstreifen
	my @tris;
	for my $i (1 .. $nStrip) {
		my $n = unpack("C", substr($d, $at, 1));
		$at += 1;
		my @idx = unpack("v$n", substr($d, $at, 2 * $n));
		$at += 2 * $n;
		for my $k (0 .. $n - 3) {
			my ($a, $b, $c) = @idx[$k, $k + 1, $k + 2];
			next if $a == $b || $b == $c || $a == $c;		# Naht zwischen Streifen
			push @tris, ($k % 2 == 0) ? [$b, $a, $c] : [$a, $b, $c];
		}
	}

	# Vertices
	my (@pos, @nrm, @uv, $extra);
	$extra = 0;
	for my $i (1 .. $nVert) {
		if ($weighted) {
			my @v = unpack("f<3 f<2 v v f<3 f<", substr($d, $at, 40));
			$at += 40;
			push @nrm, [@v[0,1,2]];
			push @uv,  [@v[3,4]];
			push @pos, [@v[7,8,9]];			# Knochenraum; fuer starre Modelle irrelevant
			$extra += $v[5];
		} else {
			my @v = unpack("f<3 f<2 f<3", substr($d, $at, 32));
			$at += 32;
			push @nrm, [@v[0,1,2]];
			push @uv,  [@v[3,4]];
			push @pos, [@v[5,6,7]];
		}
	}
	$at += 18 * $extra if $weighted;

	die "$src: Flaeche $s meldet $nTri Dreiecke, gelesen wurden " . scalar(@tris) . "\n"
		unless @tris == $nTri;
	push @surfaces, { name => "surf$s", pos => \@pos, nrm => \@nrm, uv => \@uv,
	                  tris => \@tris, weighted => $weighted, bone => $bone };
}

die "$src: " . (length($d) - $at) . " Bytes uebrig - die Deutung stimmt nicht\n"
	unless $at == length $d;

# --- Achsen von Call of Duty nach Quake drehen
# Quake: x vorn, y links, z oben. Bei "xzy" wird (x,y,z) zu (x,-z,y), eine
# echte Drehung (Determinante +1), die Wicklung bleibt also gueltig.
sub turn {
	my ($v) = @_;
	return $opt{axes} eq "xyz" ? [ @$v ] : [ $v->[0], -$v->[2], $v->[1] ];
}

my (@lo, @hi);
for my $s (@surfaces) {
	for my $i (0 .. $#{$s->{pos}}) {
		$s->{pos}[$i] = turn($s->{pos}[$i]);
		$s->{nrm}[$i] = turn($s->{nrm}[$i]);
		$_ *= $opt{scale} for @{$s->{pos}[$i]};
		for my $k (0 .. 2) {
			$lo[$k] = $s->{pos}[$i][$k] if !defined $lo[$k] || $s->{pos}[$i][$k] < $lo[$k];
			$hi[$k] = $s->{pos}[$i][$k] if !defined $hi[$k] || $s->{pos}[$i][$k] > $hi[$k];
		}
	}
	if ( $opt{flipv} ) {
		$s->{uv}[$_][1] = 1.0 - $s->{uv}[$_][1] for 0 .. $#{$s->{uv}};
	}
}

if ($opt{info}) {
	printf "%s: Version %d, %d Flaechen\n", $src, $ver, $nSurf;
	for my $s (@surfaces) {
		printf "  %-8s %5d Vertices %5d Dreiecke  %s\n", $s->{name},
			scalar @{$s->{pos}}, scalar @{$s->{tris}},
			$s->{weighted} ? "gewichtet" : "starr an Knochen $s->{bone}";
	}
	printf "  Huellquader  %.2f %.2f %.2f  bis  %.2f %.2f %.2f   (%.2f x %.2f x %.2f)\n",
		@lo, @hi, map { $hi[$_] - $lo[$_] } 0 .. 2;
	exit 0 unless defined $dst;
}
die "Kein Ziel angegeben\n" unless defined $dst;

# --- MD3 schreiben
use constant XYZ_SCALE => 64.0;
my $PI = 4 * atan2(1, 1);

sub packNormal {			# Normale als Breiten-/Laengengrad, wie Quake es speichert
	my ($n) = @_;
	my $len = sqrt($n->[0]**2 + $n->[1]**2 + $n->[2]**2) || 1;
	my @u = map { $_ / $len } @$n;
	my $lat = atan2(sqrt($u[0]**2 + $u[1]**2), $u[2]) * 255 / (2 * $PI);
	my $lng = atan2($u[1], $u[0]) * 255 / (2 * $PI);
	return ((int($lat) & 255) << 8) | (int($lng) & 255);
}

my $radius = 0;
for my $k (0 .. 2) {
	my $m = abs($lo[$k]) > abs($hi[$k]) ? abs($lo[$k]) : abs($hi[$k]);
	$radius += $m * $m;
}
$radius = sqrt($radius);

# Die drei Anhaengepunkte, die eine Quake-Waffe kennt. tag_flash sitzt vorn an
# der Muendung, sonst haengt das Muendungsfeuer im Griff.
my @tags = (
	[ "tag_weapon", 0, 0, 0 ],
	[ "tag_barrel", $hi[0] * 0.5, 0, 0 ],
	[ "tag_flash",  $hi[0], 0, 0 ],
);

# md3Frame_t: sechs Float Huellquader, drei Float Ursprung, Radius, Name[16]
my $frameLump = pack("f<10 Z16", @lo, @hi, 0, 0, 0, $radius, "MP40");
my $tagLump = "";
$tagLump .= pack("Z64 f<12", $_->[0], $_->[1], $_->[2], $_->[3], 1,0,0, 0,1,0, 0,0,1) for @tags;

my $surfLump = "";
my $surfIndex = 0;
for my $s (@surfaces) {
	my $nV = scalar @{$s->{pos}};
	my $nT = scalar @{$s->{tris}};
	my $shaderName = $opt{shader};
	if ( $opt{shaders} && @{$opt{shaders}} ) {
		$shaderName = $opt{shaders}[$surfIndex] // $opt{shaders}[-1];
	}
	$surfIndex++;
	my $shaders = pack("Z64 l<", $shaderName, 0);
	my $tris = join "", map { pack("l<3", @$_) } @{$s->{tris}};
	my $st   = join "", map { pack("f<2", @$_) } @{$s->{uv}};
	my $xyz  = "";
	for my $i (0 .. $nV - 1) {
		$xyz .= pack("s<3 v",
			map({ int($s->{pos}[$i][$_] * XYZ_SCALE + ($s->{pos}[$i][$_] < 0 ? -0.5 : 0.5)) } 0 .. 2),
			packNormal($s->{nrm}[$i]));
	}
	my $hdrSize = 108;
	my $oTris = $hdrSize;
	my $oShaders = $oTris + length $tris;
	my $oSt = $oShaders + length $shaders;
	my $oXyz = $oSt + length $st;
	my $end = $oXyz + length $xyz;
	# md3Surface_t: Kennung, Name[64], dann zehn int - zusammen 108 Byte
	$surfLump .= pack("a4 Z64 l<10", "IDP3", $s->{name}, 0, 1, 1, $nV, $nT,
		$oTris, $oShaders, $oSt, $oXyz, $end) . $tris . $shaders . $st . $xyz;
}

my $hdrSize = 108;
my $oFrames = $hdrSize;
my $oTags = $oFrames + length $frameLump;
my $oSurfs = $oTags + length $tagLump;
my $oEnd = $oSurfs + length $surfLump;
# md3Header_t: Kennung, Version, Name[64], dann neun int - zusammen 108 Byte
my $md3 = pack("a4 l< Z64 l<9", "IDP3", 15, "mp40", 0, 1, scalar @tags, scalar @surfaces, 0,
	$oFrames, $oTags, $oSurfs, $oEnd) . $frameLump . $tagLump . $surfLump;

open my $out, ">:raw", $dst or die "$dst: $!";
print $out $md3;
close $out;
printf "%s geschrieben: %d Bytes, %d Flaechen, %d Vertices, %d Dreiecke\n",
	$dst, length $md3, scalar @surfaces,
	eval { my $n = 0; $n += scalar @{$_->{pos}} for @surfaces; $n },
	eval { my $n = 0; $n += scalar @{$_->{tris}} for @surfaces; $n };
