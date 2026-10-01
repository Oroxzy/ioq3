#!/bin/bash
# Alle Karten hintereinander, mehrere Server gleichzeitig.
#
#   sweep.sh <arm> <cfg> <bots> <sekunden> <parallel> [karte ...]
#
# <cfg> ist eine Config wie die des Labors (set-Zeilen, map, addbot); die
# map-Zeile und die addbot-Zeilen werden je Karte ersetzt. Ohne Karten laufen
# die 25 Standardkarten. Jeder Lauf heisst <arm>-<karte>. Laufen mehrere
# Sweeps zugleich, braucht jeder seinen eigenen Portbereich: PORTBASE=29100 ...
# Zusaetzliche Argumente fuer run.sh stehen in EXTRA, etwa EXTRA="+set g_botLog 3".
set -u
arm="$1"; cfg="$2"; bots="$3"; secs="$4"; par="$5"; shift 5
here="$(cd "$(dirname "$0")" && pwd)"
repo="$(cd "$here/../.." && pwd)"
maps=( "$@" )
if [ ${#maps[@]} -eq 0 ]; then
	maps=( q3dm1 q3dm2 q3dm3 q3dm4 q3dm5 q3dm6 q3dm7 q3dm8 q3dm9 q3dm10 q3dm11 q3dm12 q3dm13 q3dm14 q3dm15 q3dm16 q3dm17 q3dm18 q3dm19 q3tourney1 q3tourney2 q3tourney3 q3tourney4 q3tourney5 q3tourney6 )
fi
names=( Sarge Grunt Major Visor Klesk Anarki Bones Doom Hunter Keel Orbb Patriot Ranger Razor Slash Sorlag )

mkdir -p "$repo/build/botlog/cfg"
port=${PORTBASE:-29000}
list=""
for m in "${maps[@]}"; do
	out="$repo/build/botlog/cfg/$arm-$m.cfg"
	# alles ausser map, wait und addbot uebernehmen
	grep -Ev '^[[:space:]]*(map|devmap|addbot|wait)[[:space:]]' "$cfg" > "$out"
	echo "map $m" >> "$out"
	echo "wait 200" >> "$out"
	for ((i = 0; i < bots; i++)); do
		echo "addbot ${names[i % 16]} 3" >> "$out"
		echo "wait 20" >> "$out"
	done
	port=$((port + 1))
	list="$list$arm-$m $secs $port $out${EXTRA:+ $EXTRA}"$'\n'
done
printf '%s' "$list" | xargs -P "$par" -L 1 bash "$here/run.sh"
