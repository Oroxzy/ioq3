#!/bin/bash
# Ein Messlauf ohne Spieler: ioq3ded mit Bots, eigenes Homeverzeichnis, das
# Bot-Protokoll an. Danach liegt <out>/baseq3/botlog.log zum Auswerten da.
#
#   run.sh <name> <sekunden> <port> <cfg> [+set cvar wert ...]
#
# <cfg> ist eine Datei mit set-Zeilen, map und addbot. Das Spielmodul kommt
# lose aus build/cmake-mingw64/Release/baseq3/vm - deshalb sv_pure 0.
set -u
name="$1"; secs="$2"; port="$3"; cfg="$4"; shift 4

repo="$(cd "$(dirname "$0")/../.." && pwd)"
base="$repo/build/cmake-mingw64/Release"
out="$repo/build/botlog/$name"

rm -rf "$out"
mkdir -p "$out/baseq3"
# QVM=<datei> spielt mit einem anderen Spielmodul, etwa dem eingespielten:
# das Homeverzeichnis wird vor dem Basisverzeichnis durchsucht
if [ -n "${QVM:-}" ]; then
	mkdir -p "$out/baseq3/vm" && cp "$QVM" "$out/baseq3/vm/qagame.qvm" || exit 1
fi

# Die Engine setzt jedes "+set" der Kommandozeile VOR dem +exec, die
# set-Zeilen der Config gewinnen also. Deshalb wandern die Vorgaben und die
# Zusatzargumente als set-Zeilen in die Config, direkt vor "map".
over='set g_botLog 1
set fraglimit 0
set timelimit 0'
while [ $# -gt 0 ]; do
	if [ "$1" != "+set" ] || [ $# -lt 3 ]; then
		echo "run.sh: erwartet '+set cvar wert', bekam '$1'" >&2
		exit 1
	fi
	over="$over
set $2 \"$3\""
	shift 3
done
if ! grep -Eq '^[[:space:]]*(map|devmap)[[:space:]]' "$cfg"; then
	echo "run.sh: $cfg hat keine map-Zeile" >&2
	exit 1
fi
over="$over" awk '
	!done && /^[ \t]*(map|devmap)[ \t]/ { print ENVIRON["over"]; done = 1 }
	{ print }
' "$cfg" > "$out/baseq3/botrun.cfg"

# Windows-Pfade mit cygpath bauen, nie Rueckstriche aneinanderhaengen
cd "$base" || exit 1
timeout "$secs" ./ioq3ded.exe \
	+set dedicated 1 +set sv_maxclients 16 +set sv_pure 0 +set vm_game 2 \
	+set com_hunkMegs 256 +set com_zoneMegs 64 +set net_port "$port" \
	+set fs_basepath "$(cygpath -w "$base")" +set fs_homepath "$(cygpath -w "$out")" \
	+set logfile 2 \
	+exec botrun.cfg > "$out/stdout.txt" 2>&1 < /dev/null

log="$out/baseq3/botlog.log"
if [ -f "$log" ]; then
	echo "$name: $(grep -c '^T ' "$log") Denkschritte, $(grep -c '^Kill:' "$out/baseq3/qconsole.log" 2>/dev/null) Kills"
else
	echo "$name: kein botlog.log - siehe $out/stdout.txt"
fi
