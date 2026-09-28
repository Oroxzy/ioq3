# Warnton der Zielsuch-Raketen

Für `g_homingWarn 1`. `build_mingw64.bat` packt den Ordner `sound` zu
`baseq3/zz-homing.pk3` – in einem pk3, weil der Server bei `sv_pure 1` keine
losen Dateien aus den Suchpfaden nimmt.

| Datei | für |
| --- | --- |
| `sound/homing/lock.wav` | der Piepton, den nur der Verfolgte hört |

Mit einem kleinen Perl-Skript erzeugt, nicht aus einem Spiel entnommen: 1600 Hz
mit einer Oktave darüber, 60 Millisekunden, 3 ms Anstieg und 20 ms Ausklang.
Mono, 22050 Hz, 16 bit.

Kurz, weil der Server ihn wiederholt: alle 800 ms, wenn die Rakete 2000 Einheiten
oder weiter weg ist, dichter mit jeder Einheit, die sie näher kommt, bis alle
120 ms. Ein längerer Ton würde sich aus der Nähe selbst überlappen.

Fehlt das pk3, meldet das Spiel beim ersten Piepen, dass es den Ton nicht findet,
und spielt ihn nicht – die Rakete sucht trotzdem.
