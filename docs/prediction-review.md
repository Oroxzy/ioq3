# Trajektorienvorhersage des Aim-Assists: Was noch verbessert werden kann

> Stand 20.09.2026: F02, F09 und F15 (das Nach-Landung-Paket) sind umgesetzt. Nachmessung über 111 Landeschüsse des ganzen Korpus (Ziel bei Ankunft bekannt): der Lauf nach der Landung ist bei jeder Flugzeit zweigeteilt — etwa die Hälfte läuft weiter, die Hälfte dreht um — und kein fester Faktor ändert den Splash-Anteil messbar (−0,5: 34 %, 0: 32 %, 0,4: 30 %, 1: 29 %). Die in F02 erwarteten +2 Punkte bestätigen sich nicht; die Kappung, die Fußhöhe (F09) und das Messen (F15) bleiben richtig. Die Lernfächer werden sich nahe 0 einpendeln; ein besserer Hebel für diese Gruppe ist das Abwarten bis zur Landung, nicht die Vorhersage.

## 1. Kurzfassung

Der Kern der Vorhersage stimmt: Flugzeit, Mündung, Zeitbasis (Prestep + Frame), Parabel für Ziele in der Luft und die Lead-0-Regel für Hitscan wurden von allen Lesern gegen Spielcode und Logs geprüft und passen bis auf Rundung (Rakete taucht in 808/808 Fällen bei world+50 auf; Rail-Einschläge p50 1–2 u neben der Linie; Ziele, die in der Luft bleiben, werden auf median 1–3 u xy vorhergesagt).

Messbar schwach ist alles, was nach einer Bodenberührung oder auf dem Boden passiert: Ziele, die vor dem Einschlag landen (18–21 % der Raketenschüsse, Trefferquote 9–11 % gegen 49–67 % bei reinen Luftzielen), Jump-Pads, die der Bodenpfad nicht sieht, ein Bodenziel, das über eine Kante läuft, und der Fallback bei blockierter Linie (8 % der Raketen, 3 % Treffer). Dazu kommt die Bodenlauf-Modellierung (Abbiegen, Bremsen), deren Fehler mit dem Lead wächst (xy-Fehler median 24/88/189 u bei <400/400–750/≥800 ms), wo aber alle Gegenmaßnahmen im Impact-Test wenig bringen, weil das Laufverhalten der Bots bimodal ist.

Die eine wertvollste Änderung: das Nach-Landung-Modell (F02 mit F09 und F15 als Paket) — Post-Landing-Lauf auf 320 u/s kappen, deutlich stärker dämpfen (gemessen läuft der Bot nach der Landung ~0–0,5 des erwarteten Wegs), die Füße statt des Körpers anzielen und die Landegruppe endlich lernen lassen. Alle drei Prüflinsen haben diese drei Befunde bestätigt.

## 2. Bestätigte Befunde

Sortiert nach erwarteter Wirkung auf den Einschlagpunkt. "Linsen" = Code / Daten / Impact; ein Dissens ist jeweils benannt.

### Rang 1 — F02: Post-Landing-Lauf mit Luftgeschwindigkeit, Trust 1 und Boden-Faktor
- **Wo:** code/client/cl_input.c:1994-2007 (fall ≥ 0: `sideways = fall + Sideways(rest)*Trust*Tune`, `end = trBase + motion*sideways` mit `motion` = Luftgeschwindigkeit aus :751-758), :797-812 (Trust = 1.0 für jedes Luftziel); Spiel: bg_pmove.c:199-200 (30 %/Frame Reibung), :239-257 (max. 320 u/s zurück).
- **Was:** Ein Ziel, das vor dem Einschlag landet, wird nach der Landung mit der in der Luft gemessenen Geschwindigkeit und Richtung weitergeführt. Das Spiel nimmt pro 50-ms-Frame 30 % weg und füllt höchstens bis 320 auf; Pad-Lander mit 500–800 u/s sind nach 2–3 Frames bei ≤320, und die Bots biegen beim Landen ab oder bleiben stehen. Der Punkt liegt weit vor dem Ziel.
- **Beleg:** Trefferanteil Landegruppe 9,5 % (12/126) bzw. 11,4 % (n=202) gegen Boden 18–19 % und Luft-ohne-Landung 49–56 %. Entlang der Luftrichtung liegt das Ziel median −73 u hinter dem Punkt (Pace >320: −199 u; n=133); Post-Landing-Lauf gemessen ~0,3 des erwarteten (n=27). Sensitivität (n=35): innerhalb 120 u heute 54 %, mit Kappung 320 und Faktor 0,5 66 %, Faktor 0,25 69 %.
- **Behebung:** Nach der Landung horizontale Geschwindigkeit auf cl.snap.ps.speed (320) kappen, den Rest-Lauf mit einem eigenen Faktor (~0,25–0,5 oder gelernt, siehe F15) dämpfen statt mit den Bodenläufer-Boxen.
- **Wirkung:** ~+2 Punkte aller Raketenschüsse innerhalb des Splash; Direkttreffer der Gruppe (12,7 %) sollten sich der Bodenquote (18,8 %) nähern.
- **Aufwand:** S–M. **Linsen:** alle drei bestätigt (0,7/0,8/0,65).

### Rang 2 — F01: Jump-Pads (ET_PUSH_TRIGGER) fehlen im Bodenpfad
- **Wo:** cl_input.c:2038-2087 (Pfad nur gegen MASK_PLAYERSOLID), :2146-2199; Spiel: g_trigger.c:184-212, bg_misc.c:1453 (BG_TouchJumpPad ersetzt die Geschwindigkeit), cg_predict.c:372-373 (cgame macht genau diese Prüfung schon für den eigenen Spieler).
- **Was:** Der Bodenlauf wird nur gegen feste Geometrie gestuft. Kreuzt der Lauf innerhalb der Flugzeit ein Pad, steht der Punkt 16–20 u über dem Boden, während das Ziel hunderte Einheiten hoch und weg ist. Sobald das Ziel auf der Pad-Parabel ist, ist das Luftmodell nahezu exakt.
- **Beleg:** 20 von 233 verknüpfbaren Boden-Raketenschüssen (8,6 %) haben das Ziel beim Eintreffen in der Luft und >46 u über `plain` (15 davon >160 u = Pads); |actual−at| median ~390 u, 1/18 im Splash. Diese Schüsse treffen 6/63 gegen 44/184 bei Bodenzielen. Luftmodell auf der Parabel: median 1,4–2,2 u.
- **Behebung:** Die gestufte Hülle zusätzlich gegen jedes ET_PUSH_TRIGGER-Inline-Modell prüfen (CM_InlineModel wird bei :2688 schon benutzt) und ab dem Kontaktframe mit velocity = origin2 durch den Luftzweig weiterrechnen. Lernproben mit großem `aside` und Anstieg verwerfen.
- **Wirkung:** Behauptet +2–3 Punkte Raketen im Splash. **Dissens Impact (0,6):** nur 10 der 20 Launches kreuzen ein Pad entlang des tatsächlich gezielten Laufs (12 entlang der Vollgeschwindigkeitslinie), die übrigen Bots gehen erst später aufs Pad; realistisch also etwa die Hälfte.
- **Aufwand:** M.

### Rang 3 — F03: Maschinengewehr wird auf dem Feuerkommando nie auf den Punkt gesetzt
(Steuerung, nicht Vorhersage — aber der größte gemessene Effekt im Korpus.)
- **Wo:** cl_input.c:4733-4734 (exact nur für SingleShot-Waffen), :3804-3807 (MG/Plasma/LG ausgeschlossen), :4839-4856 (blend = 8·8/200 = 0,32 pro Kommando).
- **Was:** Mit cl_aimAssist 8, Exact 1, 125 fps bewegt sich die Sicht pro Kommando nur 0,32 des Wegs zum Punkt, ohne Vorhalt der Punktbewegung. Der Schuss geht mit dem geloggten `error` raus; nach einem Zielwechsel startet der Filter neu.
- **Beleg:** 1532/2142 MG-Schüsse (72 %) geblendet. Trefferquote geblendet vs. gesnapped: 400–800 u 0,885 vs 1,000 (n=339/159), 800–1200 u 0,844 vs 0,911. Schüsse mit ≥2° Rest (10,7 %) treffen 30 %. Winkelrate <10°/s: median 0,18° Rest, ≥100°/s: 1,19°.
- **Behebung:** Auf dem Feuerkommando jede Waffe snappen (SingleShot-Bedingung bei :4734 fallen lassen bzw. Exact-2-Semantik) oder Feed-Forward ω·(1−b)/b; das geblendete Folgen zwischen den Schüssen bleibt.
- **Wirkung:** +3 Punkte MG-Trefferquote (~70 Treffer je 2142), unabhängige Schätzung +1,5–2. Vorbehalt aller Linsen: die gesnappte Gruppe ist der aktuelle Angreifer, evtl. näher.
- **Aufwand:** S. **Linsen:** alle drei bestätigt (0,85).

### Rang 4 — F15: Die Landegruppe lernt nie, liest aber die Bodenläufer-Boxen
- **Wo:** cl_input.c:4140-4143 (Remember: `target in the air` → return vor jeder Probe), :1996-1998 (Post-Landing-Lauf liest `CL_AimAssistTune(weapon, rest, pace)`), :1252 (Box ohne Luft/Lande-Achse).
- **Was:** Kein Schuss auf ein Luftziel erreicht Learn/TuneUpdate; der Post-Landing-Faktor kommt aus Boxen, die nur Bodenläufer füllen (~1,0), obwohl die Gruppe einen Faktor um 0,5 oder darunter bräuchte. Keine Rückkopplung kann das korrigieren.
- **Beleg:** 0 von 391 Luft-Raketenschüssen haben eine learn-Zeile (275 von 406 Bodenschüsse haben eine); 546–557 Drops `target in the air`. Post-Landing-Anteil des Laufs: median −0,01 des erwarteten, lernergewichtet 0,43–0,48 (n=41–75); mit Faktor 0,5 fällt der median |Längsfehler| von 114 auf 68 u, 70 % der 69 verknüpften Schüsse verbessern sich.
- **Behebung:** Landeziele registrieren (Ursprung = Landepunkt, Richtung = Luftkurs, erwartet = Post-Landing-Lauf) in einer eigenen Achse/Box, nicht in die Bodenläufer-Box mischen (sonst F07-artige Vermischung).
- **Wirkung:** etwa die Hälfte der korrigierbaren ~84 u auf 18 % der Projektilschüsse. **Einschränkung Daten-Linse:** die Verteilung ist bimodal, der beste feste Skalar liegt bei ~0 (rms 171 u) statt 0,5 (rms 194); ein gelernter Skalar nimmt nur einen Teil des Fehlers weg.
- **Aufwand:** M. **Linsen:** alle drei bestätigt (0,8/0,8/0,75).

### Rang 5 — F10: Blockierte Linie → Fallback auf den Körper ohne Lead
- **Wo:** cl_input.c:4753-4763 (`VectorCopy(trBase)`, +BodyHeight, plain = qtrue), :2792-2801 (Hold lässt den Schuss zu, weil der Körper frei ist).
- **Was:** Fällt die Eye→Punkt-Spur durch, zielt die Steuerung auf die aktuelle Körperposition, ohne einen Zwischenpunkt auf dem vorhergesagten Pfad zu versuchen, obwohl die Flugzeit ~1,1 s ist.
- **Beleg:** 72 von 883 Raketenschüssen (8,2 %) sind Fallback; Treffer 1/33 (3,0 %) gegen 187/670 (27,9 %) geführt; Daten-Linse: 1/46 vs 315/1067, bei Pace ≥200 0/33. 48/72 Luftziele, 55/72 Pace ≥200, mittlere Distanz 1073 u.
- **Behebung:** Den Lead frameweise verkürzen, bis CL_AimAssistShotClear passt (letzter freier Punkt auf dem vorhergesagten Pfad); für Bodenziele zuerst denselben geführten Punkt in Körperhöhe (+4) statt Füße−20 versuchen, weil die 4 u über dem Boden bei flachen Linien klippen.
- **Wirkung:** ~+1 Punkt Raketen-Trefferanteil (+5–8 Treffer je ~1100). Korrektur aus der Prüfung: "der Record speichert lead 0" stimmt nicht — Remember kehrt bei lead ≤ 0 sofort zurück, die Schüsse fehlen im Lerner ganz.
- **Aufwand:** S–M. **Linsen:** alle drei bestätigt (0,8/0,6/0,7).

### Rang 6 — F09: Füße-/Körperhöhe nach aktuellem Stand statt nach Ankunft
- **Wo:** cl_input.c:2284-2292 (Impact: −20 nur wenn `groundEntityNum != ENTITYNUM_NONE`), Aufrufer :2459-2464 und :2570-2575 mit dem Live-Entity, obwohl Predict `pinned`/fall ≥ 0 gesetzt hat.
- **Was:** Ein Ziel, das laut Vorhersage beim Eintreffen steht, wird auf Ursprung+4 (28 u über dem Boden) gezielt statt auf Ursprung−20 (4 u). Die Rakete fliegt durch den Punkt und platzt hinter dem Ziel statt daneben am Boden.
- **Beleg:** Fehlschüsse der Landegruppe: Ankunft −world−lead +50:46, +100:33, +150:12, bis +1050 (fliegt weiter) gegen Bodenziele 0:157, +50:205; Explosionsabstand vom Zielpunkt median 43 u mit vert −27 in jedem Quartil (n=66); Boden 9 u.
- **Behebung:** Die Füße wählen, wenn Predict den Punkt auf einen Boden gepinnt hat.
- **Wirkung:** ~35 u Burst-Offset weg auf 18–21 % der Raketenschüsse; allein +1–2 Punkte in dieser Gruppe, begrenzt durch den xy-Fehler (median 188 u heute), mehr nach F02.
- **Aufwand:** S. **Linsen:** alle drei bestätigt (0,8/0,8/0,75).

### Rang 7 — F26: Ein Bodenziel, das über eine Kante läuft, fällt nie
- **Wo:** cl_input.c:2006-2007 (Höhe flach entlang der Geschwindigkeit), :2024-2026 (Schwerkraft nur bei !grounded), :2093-2108 (Bodenspur senkt nur ≤ STEPSIZE), :1974-1975 (Landing nur für Luftziele); Spiel: bg_pmove.c:1124-1129 (freier Fall ab dem Frame, in dem die Box den Boden verlässt), Schwerkraft bg_slidemove.c:65-68 (im Befund falsch als :21-25 zitiert).
- **Was:** Verlässt der vorhergesagte Lauf die Plattform, findet die Endspur einen Boden weit unten, der STEPSIZE-Test schlägt fehl, und der Punkt bleibt auf Plattformhöhe über der Leere; ein Fall-Modell für Bodenziele gibt es nicht.
- **Beleg:** 23 von 449 verknüpften Boden-Raketenschüssen (5,1 %) mit Ziel >20 u unter dem Punkt, median dz 222 u (25–328), 11 davon mit dxy ≤ 60 (saubere Fehlschüsse nur wegen der Höhe); Daten-Linse 18/510 (3,5 %), 5 mit dz ≥100 und dxy ≤37, alle missile-miss. Code-Linse strenger: 6–7 echte Walk-offs von 268 (2,2–2,6 %); einer der fünf zitierten "Fehlschüsse" (lead 350, dz 25) war ein Treffer.
- **Behebung:** Findet die Endspur Boden >STEPSIZE tiefer, die Kante entlang des Laufs mit wenigen Box-Spuren einschachteln, ab dort den Fall integrieren und den Rest an CL_AimAssistLanding geben. Hinweis der Daten-Linse: die Bots bremsen in der Luft Richtung Landepunkt (Lauf 0,25–0,5 von pace·flight), die bisherige gedämpfte Horizontale lag in diesen Fällen schon auf 17–37 u — nur die Höhe braucht den Fall.
- **Wirkung:** 2,5–5 % der Boden-Raketenschüsse, davon etwa die Hälfte saubere Höhen-Fehlschüsse → Splash.
- **Aufwand:** M. **Linsen:** alle drei bestätigt (0,8/0,8/0,85).

### Rang 8 — F05: Trust-Dämpfung schneidet den Lead stärker als die Ziele zurückbleiben
- **Wo:** cl_input.c:850-852 (spread = |perp dv|/speed·√(time/interval), trust = 1/(1+spread)), angewandt :1996-2002, im Lerner :4198.
- **Was:** Eine einzige Querbeschleunigung aus einem Snapshot-Paar wird mit √(lead/50 ms) hochskaliert und multipliziert den ganzen Längs-Lead. Ziele mit trust <0,8 laufen median +28–30 u weiter als geführt; das Signal korreliert mit dem Querfehler, nicht mit dem Lauf.
- **Beleg:** trust <0,8 auf 34–38 % der Boden-Raketen; ran−expected median +28 (n=163/164); Idealfaktor 0,76–0,85 gegen benutzt 0,52–0,68; corr(spread, Lauf) −0,02 (n=205). Replay: k=0 statt k=1 hebt den Anteil innerhalb 60 u um +3–7 Punkte bei 0,4–0,8 s. Gegenrechnung auf Schüssen: +4/153 innerhalb 120 u.
- **Behebung:** Dämpfung halbieren oder weglassen (k<1 in 1/(1+k·spread)), das √(time/interval)-Wachstum streichen.
- **Wirkung:** ~2 Punkte der trust<0,8-Schüsse, unter 1 Punkt insgesamt. **Dissens Daten (0,6):** in den Spieldaten ändert das Wegnehmen der Dämpfung den 120-u-Anteil bei 400–750 ms gar nicht (123 vs 123).
- **Aufwand:** S.

### Rang 9 — F04: Abbiegerichtung wird gemessen und weggeworfen
- **Wo:** cl_input.c:834-852 (nur |perp dv|), :2001-2005 (Kurs fest auf aktuelles trDelta); Spiel: bg_pmove.c:199-200, :245-256.
- **Was:** Der Bodenzug ist bei bekannter Wunschrichtung deterministisch (0,7 der Geschwindigkeit pro Frame, +≤160 u/s Richtung w). Ein Bot, der zwischen zwei Snapshots abbog, biegt weiter; w ≈ normalize(v_now − 0,49·v_prev) ist aus den Daten rekonstruierbar, die der Trust-Code schon liest.
- **Beleg:** Vorzeichen der Querverschiebung 300 ms später stimmt 159:39 mit dv_perp überein. Recursion-Kurs + Hold-Betrag (D0): Median auf abbiegenden Zielen 69,5→54,4 u (0,4 s), 108,9→89,8 (0,6 s); Anteil innerhalb 60 u gesamt 60,8→65,1 %, 34,9→44,7 %. Der volle Rekursion als Positionsmodell verliert ab 0,6 s.
- **Behebung:** Nur den Kurs aus zwei bg_pmove-Frames Richtung w nehmen, den Betrag beim Hold-Modell lassen.
- **Wirkung:** 15–19 u median auf abbiegenden Zielen (ein Drittel), verblasst über 600 ms. **Dissens Impact (0,7):** im eigenen Replay über alle Boden-Trajektorien ist der Kurswechsel auf Schussniveau fast nichts wert; Daten-Linse: Effekt überzeichnet.
- **Aufwand:** M.

### Rang 10 — F07: Trust steckt im Lerner-Nenner, ist aber keine Tabellenachse
- **Wo:** cl_input.c:4198 (base = speed·Sideways·trust), :1256-1263 (Box nur weapon/band/pace).
- **Was:** Eine Box mittelt Verhältnisse zweier Populationen mit gegenläufigem Korrekturbedarf und landet bei ~1,0 — der trust-abhängige Fehler von F05 ist für die einzige gelernte Korrektur unsichtbar.
- **Beleg:** gewichtetes Verhältnis trust<0,8: 1,33/1,14/1,22/1,18 pro Band gegen trust≥0,8: 0,99/0,97/0,77/0,62 (n=414). Zusatz [log-learning]: die pace<200-Boxen sind clamp-dominiert (60–82 % der Proben bei 0 oder 2; b3 p0 roher Mittelwert 11,5) und speisen über den Pace-Blend 36–59 % in jede Abfrage bei 200–300 u/s ein (12 % der Boden-Raketen).
- **Behebung:** Verhältnis gegen das un-getrustete Modell messen (Trust erst nach der Tabelle) oder Trust als dritte Achse; robuste Statistik (Median) für die langsamen Boxen.
- **Wirkung:** behauptet +33 u median auf ~30 % Low-Trust-Schüsse. **Dissens Impact (0,8):** Simulation der Achse (in-sample und leave-one-out) bringt keine Probe zusätzlich in den Splash; überwiegend Stabilität.
- **Aufwand:** M.

### Rang 11 — F12: Bremsen entlang des Kurses wird per Design ignoriert
- **Wo:** cl_input.c:831-844 (Längsanteil der Änderung wird herausprojiziert), :1996-2004; Spiel: bg_pmove.c:199-200, :245-256; botlib be_ai_move.c:1360-1367, :1404, :2974 (Bots fahren vor Zielen/Kanten absichtlich unter 320).
- **Was:** Ein bremsendes Ziel behält trust ~1 und wird geführt, als liefe es weiter; das Spiel stoppt einen 320er in 7 Frames auf 31,6 u (nicht 53 u wie im Befund).
- **Beleg:** Rein längs bremsende Paare (pace ≥200): Laufanteil 0,45 gegen 0,90 (steady) bei 300 ms, 0,17 gegen 0,71 bei 600 ms; Trust dieser Paare im Mittel 0,933. Unter der Kappe: beschleunigende Paare laufen 1,64×, bremsende 0,08× der Geraden. Reines Bremsen ist aber nur 3,5 % der Bodenpaare (nicht 9 %).
- **Behebung:** Vorzeichen der Längsänderung lesen: bremsend → Lead auf den Reibungsrest (~32 u) kürzen, beschleunigend → mit 320·Sideways führen. Die Über-Kappe-Hälfte gehört zu F13.
- **Wirkung:** klein. **Dissens Impact (0,8):** am Schuss selbst bremsen nur 5 von 270 Boden-Raketenzielen (1,9 %), mit Leads von 9–64 u; ~0,5 % der Schüsse. Erste Runde: Code und Daten refutiert, zweite Runde beide bestätigt.
- **Aufwand:** S–M.

### Rang 12 — F13: Bodengeschwindigkeit über 320 u/s wird als dauerhaft geführt
- **Wo:** cl_input.c:1961-2004 mit :751-758 (trDelta ungekappt); Spiel: bg_pmove.c:199-200, :245-248, g_combat.c:901-912 (Knockback setzt die Reibung ≤200 ms aus).
- **Was:** Landungs-/Knockback-Impuls >400 u/s wird über den ganzen Lead multipliziert; die Ziele laufen entlang dieses Kurses ~0 oder rückwärts.
- **Beleg:** 9 von 644 Boden-Raketen >400 u/s, alle 6 messbaren um 157–902 u verfehlt (ran/expected −0,05); 47 Schüsse bei 341–400 verhalten sich wie Läufer (Anteil 0,78–0,91, unter-geführt).
- **Behebung:** Über-Kappe-Geschwindigkeit nach zwei Frames als verbraucht behandeln (auf 320 kappen, Rest ohne Trust).
- **Wirkung:** 4–6 von 644 (0,6–0,9 %). **Dissens Impact (0,85):** zu selten.
- **Aufwand:** S.

### Rang 13 — F08: Landesuche hält am ersten Boden fest / gibt auf
- **Wo:** cl_input.c:2166-2193 (:2175-2176 und :2184-2185 brechen ab und behalten `answer` der vorigen Runde), konsumiert :1994-2007.
- **Was (verschärft, [vertical]):** Runde 1 sondiert unter dem aktuellen xy; findet Runde 2 (am xy, wo der Bogen diese Höhe kreuzt) nichts oder nur den Kartenboden, ist das der Beweis, dass das Ziel an der Plattform vorbeifliegt — die Schleife behält trotzdem Runde 1. Spiegelbild: findet Runde 1 nichts, wird der Lauf mit voller Luftgeschwindigkeit weitergeführt und nur die Höhe vom Boden-Snap gerettet.
- **Beleg:** 4 verknüpfte Raketenschüsse mit Phantomlandung, Ziel im freien Fall (z innerhalb 1–2 u des Bogens, z. B. 153301 world 169250: Land 248 ms auf z 348, Ziel bei −90); Plattformkanten aus geerdeten Bot-Positionen bestätigen die zweite Sonde neben der Plattform. Daten-Linse: falscher Zweig 11/222 (5 %) der verknüpften Luftschüsse, ~2,4 % der Raketen. Erste Runde: alle drei Linsen refutiert; mit dem Zweig-Nachweis Code/Daten bestätigt.
- **Behebung:** Ist die Sonde der Runde 2 tiefer oder leer, Runde 1 verwerfen und den Bogen ab dieser Zeit fortsetzen (nächste Kreuzung sondieren); generell entlang des Bogens sondieren.
- **Wirkung:** klein auf q3dm17 (Ziel stirbt meist in der Leere). **Dissens Impact (0,8):** 7 von 5209 Schüssen (0,13 %), alle Fehlschüsse, das zitierte 153301-Beispiel war `assist 0`.
- **Aufwand:** M.

### Rang 14 — F24: Luftsegment als Sehne statt Bogen geclippt
- **Wo:** cl_input.c:2034-2040 (eine gerade Spur von trBase zum Bogen-/Landeende), :2060, :2079 (Settle scheitert bei Luftstart), :2103-2107 (Snap nur ≤STEPSIZE).
- **Was:** Die Sehne eines nach unten gekrümmten Bogens liegt unter dem Bogen und schneidet bei Landezielen diagonal zum Boden; Plattformlippen, an denen das Ziel vorbeisegelt, stoppen die Spur, der Punkt bleibt auf der Sehne hängen.
- **Beleg:** 24 von 265 gelösten Landungen >4 u über dem Boden, 12 >30 u, 4 >100 u, alle 24 exakt auf der Sehne (cos 1,000); zwei Punkte sitzen genau 24 über dem 352er-Boden (Lippe). Verknüpft: bei 4 von 7 stand das Ziel auf dem gelösten Boden, Punkt 25–141 u darüber. Die Nicht-Lande-Hälfte des Befunds hält nicht: dort rettet der Sehnenstopp F08-verpasste Landungen (8/16 dz=0), ein reiner Bogen-Trace wäre schlechter.
- **Behebung:** Luftteil in wenigen Segmenten entlang der Parabel bis zum Landepunkt spuren, dann Boden-/Stufenlogik.
- **Wirkung:** ≤5 % der Lande-Raketen, real weniger. **Dissens Impact (0,75):** 0,84 % der exakten Raketen; auf 16 prüfbaren Schüssen macht der Bogen 3 besser, 8 schlechter.
- **Aufwand:** M.

### Rang 15 — F27: Raketen auf Ziele, die von der Karte fallen; Kartenboden als Landung
- **Wo:** cl_input.c:2172-2173 (8192 u nach unten, jeder Treffer gilt), :2523 (Lead-Kappe 2950 ms), :2719-2800 (kein Hold-Grund), :4870-4876 (als Landeschuss in der Rate-Tabelle gebucht); Spiel: g_trigger.c:354-390 (trigger_hurt bei z −2434..−1730).
- **Was:** Der Boden bei z ≈ −2398 liegt innerhalb des trigger_hurt-Volumens; kein Bot landet dort je (tiefster geerdeter Bot z 24, lebend bis −1697). Trotzdem wird eine Landung 1,7–2,9 s voraus gebucht und geschossen.
- **Beleg:** 22 von 665 exakten Luft-Raketen mit Zielpunkt z < −100 (3,3 %); 9 Ziele starben vor Ankunft in der Leere, 13 standen beim Eintreffen auf einer Plattform 1300–2200 u über dem Punkt; aimrate.cfg Zeile `5 4`: landShots 13,92, landHits 0.
- **Behebung:** Boden unter dem tiefsten begehbaren Boden (oder Fall >~1,5 s ohne Näheres) als keine Landung; Hold-Grund für einen Bogen, der unter jedem Boden im Flug endet.
- **Wirkung:** keine Einschlagänderung, nur gesparte Schüsse. **Dissens Impact (0,75):** 11 echte Fälle (0,2 % aller Schüsse), Rest sind Pad-Flüge (F08/F01).
- **Aufwand:** S.

### Rang 16 — F16: Kill-Frame fehlt in Lerner und Record
- **Wo:** cl_input.c:4238-4247, :4481 (EF_DEAD übersprungen); verschärft: :4399-4412 CL_AimAssistWatch löscht die Probe still, bevor Learn läuft; Spiel: bg_misc.c:1469 (Gib → ET_INVISIBLE), g_combat.c:616-618, g_missile.c:412-418 (Gib-Kill wird als missile-miss geloggt).
- **Was:** Der Record kann Kills nicht messen, und 57 % der direkten Raketen-Kills (178/311) stehen als `missile-miss … other 0` im Log — jede Trefferquote aus missile-hit-Zeilen unterzählt Direkttreffer um 34 %.
- **Beleg:** 311 direkte Kills gegen 211 nicht-tödliche Treffer; Ziel auf 0/311 Kill-Frames im Suffix; 133 Kills mit missile-hit ohne Ziel. Faktorverschiebung durch die Zensur ~0,015 (Daten) bzw. 0,0007 (Impact).
- **Behebung:** Auf dem Ankunftsframe ein Entity mit passender Nummer akzeptieren, das ET_PLAYER+EF_DEAD oder ET_INVISIBLE ist — in Watch (:4404) und Learn (:4240); missile-miss mit gleichzeitigem `hit on … MOD_ROCKET`-Kill als Treffer labeln.
- **Wirkung:** Record: +33 % der Direkttreffer messbar; Lead: ~0,1–2 u. **Dissens Impact (0,85):** keine messbare Aim-Änderung.
- **Aufwand:** S.

### Rang 17 — F14: Das Vorzeichen des Längsrests hängt von Statistik und Trust-Bin ab; "früh" ist Geometrie
- **Wo:** cl_input.c:2284-2292 (Füße−20), :2472 (fester 14+15-u-Rand), :2525-2532 (Framewahl), :4245-4285 (Learn misst am Ankunftsframe, nie am Einschlag).
- **Was:** Mittelwert negativ (22–73 % der Ziele stoppen/kehren um bei ≥800 ms), Median entlang des Kurses ~0 für trust≥0,8 <800 ms und positiv für trust<0,8; frühe Treffer (32–40 % steil geerdet vs 8 % Luft) kommen von der einseitigen Körpersäule, gleichmäßig über nähernde, querende und fliehende Ziele — nicht von Über-Lead.
- **Beleg:** trust<0,8 Mediane +58/+44 u (400–750/800–1250 ms); Lerner-Verhältnis trust<0,8 1,18 gegen 0,85/0,75; frühe Treffer treten median +18 u über dem Ursprung in die Box ein, 16–169 u vor dem Punkt.
- **Behebung:** Keine Codeänderung — ein Richtungsschutz: Low-Trust-Faktor anheben, nicht den Lead aus dem Mittelwert kürzen; nichts aus der Ankunftsstatistik tunen.
- **Wirkung:** keine direkt; verhindert eine Verschlechterung um 15–38 u auf ~450 Schüssen. **Dissens Impact (0,6):** kein Defekt, Inhalt steckt in F05/F11/F19.
- **Aufwand:** S.

### Rang 18 — F32: Decay 0,98 lässt den Faktor auf Rauschen laufen
- **Wo:** cl_input.c:993 (AIM_TUNE_DECAY 0,98 → 99 effektive Proben), :1263-1277, :1134-1138; Vergleich :1443-1456 (Rate-Tabelle wählte 0,99 mit derselben Begründung).
- **Was:** Pro-Proben-sd 0,46–0,90 gegen einen Rauschboden von ±0,046–0,09; die Differenz zwischen Bändern (1,06→0,81) ist etwa 2–3,5 σ; die Sessions sind homogen (ANOVA F 0,41–1,24, lag-1-Autokorrelation ~0).
- **Beleg:** Beobachtete `now`-Spanne 0,05/0,09/0,13/0,11 (b0–b3 p1) liegt innerhalb der reinen Rausch-Simulation (0,09/0,14/0,16/0,14); Box 1/0 fiel 1,26→0,95 (Konvergenz, nicht Drift).
- **Behebung:** Decay 0,99 oder 0,995 bzw. zeitbasiertes Vergessen.
- **Wirkung:** Stabilität. **Dissens Impact (0,85):** realisierte sd des Faktors 0,014–0,029, Zielpunkt-Wanderung median 0,4–4,6 u, Basis 190 u statt 335; unter der 5-u-Schwelle.
- **Aufwand:** S.

### Rang 19 — F30: Probengewicht korreliert mit dem gemessenen Fehler; Scatter 15–30 % zu klein
- **Wo:** cl_input.c:4267 (weight = straight/(straight+lateral)), :1272-1277 (miss² · weight), :1229; Konsumenten :2251-2254 (Grade), :3614-3616 (Prioritätsteil SURE), :2769-2772 (Lottery, aus).
- **Beleg:** corr(weight, miss) −0,56…−0,72 in jeder Box; RMS ungewichtet/gewichtet 134/94 (1/1), 278/234 (2/1), 412/355 (3/1); mit Decay 102 vs 83 (1/1). Nur abgebogene, nicht gestoppte Proben werden abgewertet.
- **Behebung:** Gewicht nur im Faktor, ungewichtetes mittleres Quadrat für den Scatter.
- **Wirkung:** keine auf den Zielpunkt (Scatter geht nie in Predict); HUD-Grade und Zielwahl. **Dissens Impact (0,8).**
- **Aufwand:** S.

### Rang 20 — F31: Scatter scharf gelesen, Faktor geblendet
- **Wo:** cl_input.c:1224 vs :1150-1203; :2251-2254, :3594-3616.
- **Beleg:** aimtune.cfg Rakete p1: 46/84/251/337 u — bei 0,79→0,80 s springt reach 1,42→0,48 (Grade 3→1), SURE-Teil 55,8→30,7 von 95; 5,4 % der Raketen (Pace ≥200) liegen ±50 ms an der 0,8-s-Kante; die learn-Fehler steigen dort glatt (104/124/147 u median).
- **Behebung:** Scatter mit denselben Vier-Box-Gewichten blenden.
- **Wirkung:** keine auf den Einschlag; 0 von 550 Pick-Wechseln waren nachweislich Kantenflips. **Dissens Impact (0,8).**
- **Aufwand:** S.

### Rang 21 — F23: Exakte Framesuche endet bei 59 Frames (2,95 s)
- **Wo:** cl_input.c:2525-2532 (kein Fallthrough-Check).
- **Beleg:** 24 von 1469 exakten Projektilschüssen bei lead 2950, echte Flugzeit 2,96–4,80 s; learn/drop-Zeilen liegen bei world+2950, nicht bei der Ankunft.
- **Behebung:** Kappe 3,0–3,5 s oder "keine Lösung".
- **Wirkung:** keine messbare (0,5–96 u auf 19 Luft-Pad-Flüge >3100 u). **Dissens Impact (0,85).**
- **Aufwand:** S.

### Rang 22 — F34: Tune-Datei kennt weder Hold noch Skill/Map
- **Wo:** cl_input.c:1110-1112, :1075-1077, :4198, :943-952, :1002.
- **Beleg:** Alle 21 Logs `hold 1.50`; Header nur `format 3`; Sideways(1,5 s) 0,948 bei Hold 1,5 gegen 0,777 bei Hold 1,0. Daten-Linse: Log 125255 mischt bereits q3dm13/q3dm14 und 7 Bots in die Tabelle.
- **Behebung:** Hold/Skill/Map in den Header, Tabelle bei Abweichung verwerfen.
- **Wirkung:** heute keine; verhindert einen 6–18 % Lead-Fehler beim ersten Umstellen (Relearn dauert Sessions). **Dissens Impact (0,85).**
- **Aufwand:** S.

### Rang 23 — F21: Pad-Touch-Snapshot (air 0, großes vz)
- **Wo:** cl_input.c:2006-2007, :2024-2026; Spiel: bg_misc.c:1453, bg_pmove.c:1132.
- **Beleg:** 2 Raketenschüsse in 17 Logs (153301 world 114800: Punkt 309 u in x und 353 u in z neben dem exakten Pad-Bogen); der zweite (vz 223) ist laut Daten-Linse kein Pad.
- **Behebung:** Grounded + vz ≥ ~100–200 auf ebenem Boden als Luftziel behandeln (fällt mit F01 ab); vor der Über-Kappe-Regel von F13 prüfen.
- **Wirkung:** 0,02–0,04 % aller Schüsse. **Dissens Impact (0,92).**
- **Aufwand:** S.

## 3. Geprüft und verworfen

- **F06 (Faktor-Mittelwert sitzt in der Lücke der bimodalen Verteilung):** Die Faktorwerte im Befund sind veraltet (aimtune.cfg 0,937/0,912, nicht 0,88); median ran/expected liegt in den genannten Bändern bei ~1,0; der Replay-Gewinn (B_f0,9) ist in den Spieldaten nicht sichtbar (Code- und Impact-Linse refutiert, Daten-Linse hielt nur die Bimodalität).
- **F11 (Füße-Offset nach Sinkwinkel):** Rechnung reproduziert, aber mit dem Join über die eigene Rakete (519 Schüsse) sind die 5 umkippenden Steil-Schüsse solche, die bei −20 schon ~95 Splash-Schaden machten; kein Gewinn.
- **F17 (Ankunftsposition nur auf Impact-Frames):** Die learn-Zeile liefert den signierten Längslauf bereits für 55 % der Boden-Raketen (Vereinigung 64 %); der Unterschied der Populationen kommt von den Drop-Regeln des Lerners, nicht vom Sampling; reine Instrumentierung.
- **F18 (kein v_prev auf der Schusszeile):** Das Intervall ist konstant 50 ms und |perp dv| ist aus trust invertierbar; nur das Abbiege-Vorzeichen fehlt (Teilaspekt von F04); Logging-only.
- **F19 (15-u-Rand ohne Pitch):** Die Richtung ist falsch — der feste Rand ist pessimistisch, nicht optimistisch; mit der tatsächlichen Zielposition ist der Frame in 48/63 Boden-Direkttreffern richtig gegen 46 mit 15/cos; Azimut zählt so viel wie Pitch.
- **F20 (Über-Kappe-Proben ziehen die Läufer-Box runter):** Die 341–400-Proben gehen geklemmt mit 1,34 ein und ziehen die Box hoch; nur die 6 Proben >400 wirken (−0,005…−0,013); der vorgeschlagene Clamp auf 320 ändert die Box nicht.
- **F22 (Missile-Zeile loggt nur den Startpunkt):** Für TR_LINEAR ist der Flug aus Startpunkt, Richtung, 900 u/s und Zeitbasis vollständig rekonstruierbar (93–95 % innerhalb des Ein-Frame-Fensters); Logging-only.
- **F25 (Landesonde startet auf aktueller Höhe):** Mechanismus richtig, aber 1 auswertbarer Fall von 842 (0,12 %); nur zwei Linsen.
- **F28 (Air Control 320 u/s²):** Physik richtig, aber "Bots nutzen es nicht" ist falsch (BotAirControl in be_ai_move.c:1660-1690); gemessen median 1–3 u, p90 12–17 u — nicht modellierenswert.
- **F29 (Least-Squares-Faktor statt Ratio-Mittel):** Der LS-Faktor landet weniger Proben im Splash (56,3 % vs 57,0 %), das Optimum liegt über dem gespeicherten Faktor; langsame Boxen ≤15 u.
- **F33 (NaN-Sum in der Tune-Datei):** NaN erreicht cl.viewangles nie (AngleNormalize360 castet NaN auf 0); Folge wäre ein stilles Nicht-Steuern, nur bei handeditierter Datei; nie aufgetreten.
- **F35 (Granaten-Fallback ohne Bogenlösung):** n=1, und dieser Schuss traf mit 64 Splash-Schaden (Bounce); "~200 u kurz" ist falsch (121 u).
- **F36 (Granate auf Körpermitte statt Füße):** Der Bounce deckt die kurze Seite; Simulation zeigt mehr Direkttreffer mit der Fußhöhe; 7 Granatenschüsse im ganzen Korpus (0,13 %).

## 4. Ohne ausreichendes Urteil

Keiner. Jeder Befund hat mindestens zwei Urteile; F25 hat nur zwei (Code, Impact) und ist verworfen.

## 5. Was die Nachleser gesehen haben

- **Core:** Flugzeitkern, Mündungsmodell, Zeitbasis und Hitscan-Lag sind gegen Server und Logs verifiziert (Missile bei world+50 in 808/808; Rail p50 1–2 u; Bullet-Flesh-Punkte in der Box zum Snapshot 450:9 gegen +50 ms; Direkttreffer auf dem vorhergesagten Frame 61/77). q3dm17 hat keine Mover, 13 Pads; 0 Granaten-/Plasmaschüsse im Korpus, also Arc und Smooth-Zweig ungetestet. Verbesserungen liegen in den Bodenlauf-Befunden, nicht im Kern.
- **Vertical:** Parabel und Landezeitformel exakt (z-Fehler median 0 bei 59 Luftschüssen, 22/22 saubere Landungen mit dz=0). Schwach ist die Geometrie drumherum: Sehne statt Bogen (F24), Sonde sieht keinen höheren Boden (F25), kein Kantenfall (F26), zweite Sonde kann die erste nicht kippen (F08), Schüsse auf fallende Ziele (F27). Nicht geschafft: Rampen-Kickoff, Duck-Höhe bei der Landung, Zuordnung der 14–25-u-Lifts über dem 352er-Boden.
- **Table:** Lerner liest das Ziel zum richtigen Frame (102/113 Luft, 47/75 Boden exakt), Fehlschüsse lehren mit, Blend ist stetig, Speichergenauigkeit verlustfrei. Scatter geht nie in den Zielpunkt. Nicht geschafft: Granaten/BFG-Boxen, Plasma (keine Daten), Hit-Rate-Replay alternativer Schätzer, Einfluss von SURE auf die Zielwahl.
- **Arc:** FireDelay/Firing-Modell, Mündung (Residuum 2,0 u auf 723 Schüssen), Granatenbogen (0,2 u an sechs Schüssen), Frame-Reihenfolge Clients vor Missiles — alles verifiziert. Nur 7 Granaten, 33 BFG, 0 Plasma. Nicht geschafft: Re-Trace blockierter Punkte (kein BSP-Zugriff), Bounce-Modell, BFG-Analyse.
- **Log-Learning:** aimtune.cfg stimmt exakt mit den letzten tune-Zeilen überein; die 11 % "weder drop noch learn" sind genau die `assist 0`-Schüsse; keine Session- oder Versionsänderung in irgendeiner Box nachweisbar. Nicht geschafft: Join Schuss→Einschlag für die Kosten des Faktorrauschens, Vorgeschichte der p0-Boxen.

## 6. Was die Daten nicht hergeben

- **Ankunftsposition je Schuss:** Nur ~ein Drittel der Schüsse hat das Ziel auf einem Impact-Frame; Kill-Frames fehlen ganz (F16), Direkttreffer werden zu 34 % als missile-miss geloggt. Alle Zellen der Trust×Lead-Tabellen sind dünn (n=3–37). Ein Lauf bräuchte eine `aim arrive:`-Zeile je gemerktem Schuss (Zielursprung, air, signierte Querabweichung, auch für tote/gegibbte Ziele) und das Labeln von Gib-Kills.
- **Abbiegerichtung:** Die Schusszeile hat kein v_prev; F04-Replays laufen auf Bot-Trajektorien, nicht auf den Schüssen. Ein Lauf bräuchte das vorige trDelta (oder signiertes perp/along dv) auf der Schusszeile.
- **Blockierte Punkte:** Ohne BSP-Trace ist nicht sagbar, welcher Brush eine Sehne stoppt (F24) oder ob die zweite Sonde wirklich neben der Plattform lag (F08). Ein Lauf bräuchte das Trace-Ergebnis (endpos, Brush) im Log.
- **Ein Setting, eine Karte, drei Bots:** Alle Logs mit hold 1,50 auf q3dm17 (bis auf einen Log mit q3dm13/14); ob Faktoren, Trust-Signal oder Post-Landing-Verhalten auf anderen Karten oder Skills halten, ist nicht prüfbar. Ein Lauf bräuchte Sessions mit anderem Hold, anderer Karte, anderem Skill — und den Header-Guard aus F34, damit die Tabelle sie nicht vermischt.
- **Waffen:** 7 Granaten, 0 Plasma: Bogenlösung, Bounce und Plasma-Lead sind ungetestet. Ein Lauf bräuchte gezielte Granaten-/Plasma-Sessions.
- **Wirkung auf Treffer:** Für F04, F05, F07, F12, F15 gibt es nur Replays und Proxy-Anteile, keinen In-Game-A/B-Vergleich; die Impact-Linse hat genau deshalb dissentiert. Ein Lauf bräuchte je Änderung eine Session mit umschaltbarem Cvar und die Trefferquote aus den `hit on`-Zeilen des Servers (nicht aus missile-hit).
- **HUD-Grade und Zielwahl:** Grade-Toggles werden nicht geloggt; ob F30/F31 je einen Pick geändert haben, ist nur indirekt (Pick-Margen) sichtbar.
