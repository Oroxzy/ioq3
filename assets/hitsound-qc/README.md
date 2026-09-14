# Quake-Champions-Trefferton

Der Ton für `cl_hitSound 2`. `build_mingw64.bat` packt den Ordner `sound` hier
zu `baseq3/zz-hitsound-qc.pk3` - in einem pk3, weil der Server bei `sv_pure 1`
keine losen Dateien aus den Suchpfaden nimmt, und mit `zz-` im Namen, damit er
nach den `pak*.pk3` durchsucht wird.

| Datei | wofür |
| --- | --- |
| `hit_qc.wav` | was die Engine als `HIT_SOUND_QC` lädt, also `cl_hitSound 2` |
| `hit_qc_25/50/75/100.wav` | Varianten nach Rest-HP, über `cl_hitSoundFile` wählbar |
| `hit_qc_lowest/low/deep/high.wav` | dasselbe nach Tonhöhe benannt |

Mit `cl_hitPitch 1` verstimmt die Engine den geladenen Ton selbst nach der
Rest-HP des Getroffenen; die fertigen Varianten braucht dann nur, wer die
Abstufung lieber als eigene Dateien hätte.

Die wav-Dateien sind fremdes Material und stammen nicht aus diesem Projekt;
unter welcher Lizenz sie stehen, ist hier nicht festgehalten.
