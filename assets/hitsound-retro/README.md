# Trefferton im Stil von Quake 1 und Quake 2

Für `cl_hitSound 3` (Quake 1) und `cl_hitSound 4` (Quake 2). `build_mingw64.bat`
packt den Ordner `sound` zu `baseq3/zz-hitsound-retro.pk3` — in einem pk3, weil
der Server bei `sv_pure 1` keine losen Dateien aus den Suchpfaden nimmt.

| Datei | für |
| --- | --- |
| `hit_q1.wav` | `cl_hitSound 3` |
| `hit_q2.wav` | `cl_hitSound 4` |

**Wichtig, damit hier kein Missverständnis entsteht:** Quake 1 und Quake 2 hatten
überhaupt **keinen Trefferton**. Es gibt also nichts, was man hätte entnehmen
können, und entnommen ist hier auch nichts. Die beiden Dateien sind mit einem
kleinen Perl-Skript erzeugt und ahmen nur den *Klangcharakter* der jeweiligen
Zeit nach:

- **Quake 1**: dunkel und körnig — ein tiefer Ton bei 420 Hz mit einer Quinte
  darüber, Rauschanteil, sehr schnelle Hüllkurve, und danach durch einen
  einfachen Tiefpass. So klang die 11-kHz-Ausgabe von damals.
- **Quake 2**: heller und metallischer — zwei Töne kurz hintereinander (900 Hz,
  dann 1350 Hz), mit einer Oberwelle und kaum Dämpfung. Der typische Blip der
  22-kHz-Ära.

Beide sind mono, 22050 Hz, 16 bit, und 75 bzw. 90 Millisekunden lang. Wer die
echten Töne aus einem Mod von damals lieber mag, legt sie irgendwo ab und
zeigt mit `cl_hitSound 2` und `cl_hitSoundFile` darauf.

Mit `cl_hitPitch 1` verstimmt die Engine auch diese Töne nach der Rest-HP des
Getroffenen.
