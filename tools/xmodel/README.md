# Call-of-Duty-Modelle nach Quake 3

Zwei Werkzeuge, beide in Perl, beide ohne Fremdbibliotheken.

| | |
| --- | --- |
| `xmodel2md3.pl` | `xmodelsurfs` (CoD 1, Version 14) → `.md3` |
| `dds2tga.pl` | DXT1-komprimiertes `.dds` → unkomprimiertes `.tga` |

```bash
perl xmodel2md3.pl MP401 --info
perl xmodel2md3.pl MP401 machinegun.md3 --shaders models/.../mp40_body,models/.../mp40_sight
perl dds2tga.pl "metal@MP40body.dds" mp40_body.tga
```

## Das Quellformat

Es ist nirgends dokumentiert. Zurückgerechnet wurde es aus sechs Dateien, und
die Probe darauf ist hart: **die Dateilänge muss aus den Zählern im Kopf exakt
aufgehen**, bei allen sechs, ohne ein einziges Byte Rest. Dazu kamen vier
Prüfungen, die kein Raten übersteht — jeder Dreiecksindex kleiner als die
Vertexzahl, UV-Koordinaten in [0,1], Normalen mit Länge 1, und die drei
Detailstufen derselben Waffe mit fast demselben Hüllquader.

```
Datei    u16 Version (14), u16 Zahl der Flächen
Fläche   u8 Flags, u16 Vertices, u16 Dreiecke, u16 Streifen, i16 Knochen
         bei Knochen == -1 folgen vier weitere Bytes, und die Vertices sind
         40 statt 32 Byte breit, mit Gewichten in einem Anhang von 18 Byte je
         zusätzlicher Bindung
Streifen je u8 Länge, dann so viele u16 Indizes — Dreiecksstreifen
Vertex   32 Byte starr: 3 float Normale, 2 float UV, 3 float Position
```

Drei Dinge daran sind nicht offensichtlich:

- Das Feld an +5 ist die Zahl der **Streifen**, nicht ein zweiter Vertexzähler.
  Es ist zugleich die Abbruchbedingung beim Lesen.
- Die **Wicklung** ist bei geradem k `(b,a,c)` und bei ungeradem `(a,b,c)`.
  Andersherum zeigen *alle* Dreiecke nach innen — geprüft an 2290 Dreiecken,
  100 % gegen 0 %.
- Die **entarteten Dreiecke** (zwei gleiche Indizes) sind die Nähte zwischen
  den Streifen und gehören weggeworfen. Erst dann stimmt die Dreieckszahl aus
  dem Kopf, und zwar in allen zwölf Flächen exakt.

Die Einheiten sind **Zoll**: die MP40 misst im Modell 24,85 Einheiten, das sind
63 cm — ihre wirkliche Länge mit eingeklapptem Schaft. Für Quake wird 1:1
übernommen; das Maschinengewehr dort ist 16,5 Einheiten lang, die MP40 ist also
sichtbar größer, und das ist richtig so.

## Achsen

Call of Duty legt die Höhe auf Y, Quake auf Z. `--axes xzy` (die Vorgabe) dreht
`(x,y,z)` zu `(x,-z,y)`; das ist eine echte Drehung mit Determinante +1, die
Wicklung bleibt also gültig. `--axes xyz` lässt alles, wie es ist.

## Was noch offen ist

Die **UV-Richtung**. MD3 zählt die zweite Texturkoordinate von oben, und ob
Call of Duty das genauso tut, steht in den Bytes nicht — `--flipv` dreht sie,
falls die Textur auf dem Kopf steht. Das lässt sich nur im Spiel sehen.

**Gewichtete Flächen** werden gelesen, aber die Knochenmatrizen stehen in
`xmodelparts` und werden nicht angewendet — für eine starr gehaltene Waffe
braucht es sie nicht. Alle Flächen der MP40 sind starr.
