# Trefferton-Labor

Ein Prüfstand für die Änderungen an dieser ioquake3-Gabelung. Die App startet
das Spiel mit einer erzeugten Config und liest mit, was es ins `qconsole.log`
schreibt. Sie greift **nicht** in das laufende Spiel ein — sie schreibt eine
Datei davor und liest eine Datei danach.

## Wofür

**Trefferton.** Original, Quake Champions oder eine eigene Datei, und die
Tonhöhe kann der Rest-HP des Getroffenen folgen (hoch bei voller Gesundheit,
tief kurz vor dem Kill). Der Tab „Trefferton" zählt mit, ob jeder Treffer
auch einen Ton bekommen hat.

**Zielhilfe auf Bots** — die Hauptfunktion. Eine Taste wird gehalten, und die
Sicht wird auf den Bot geführt, den die Prioritätenliste als besten ausweist;
geschossen wird weiter von Hand mit der Feuertaste. Die Hilfe sagt den
Waffentimer voraus und setzt auf genau dem Befehl, auf dem der Server feuert,
den Zielpunkt auf die Stelle, gegen die der Server den Schuss wirklich prüft.
Dazwischen folgt sie weich, damit das Bild nicht ruckelt.

Sie ist **auf die lokale Verbindung und auf Bots begrenzt** und lässt sich
nicht gegen Menschen einsetzen: sie läuft nur über `NA_LOOPBACK`, und ein Ziel
kommt nur infrage, wenn der Server es in seinem Spieler-Configstring als Bot
ausweist. Das ist keine Einstellung, sondern steht so im Code.

**Sicht durch Wände.** Bots bekommen einen Drahtrahmen, Waffen und Powerups
einen Kasten mit Respawn-Zähler, dessen Farbe von Rot über Orange nach Grün
läuft, je näher das Ding am Zurückkommen ist.

**Feuer halten.** Auf Wunsch bleibt der Abzug gesperrt, solange der Schuss
nicht durchkommt. Geprüft wird die Linie zu dem Punkt, auf den **wirklich
gezielt** wird — bei der Rakete also der vorgehaltene, nicht der Bot: einer
hinter einer Säule, dessen Vorhaltepunkt im Freien steht, ist ein Schuss wert;
einer im Freien, dessen Vorhaltepunkt hinter der Säule liegt, nicht.

## Die Tabs

| Tab | zeigt |
| --- | --- |
| Trefferton | Schaden, Treffer, gespielte Töne, fehlende Töne, und das Protokoll |
| Zielhilfe | jeden Schuss einzeln: Entfernung, Vorhalt, Ergebnis, Fehlweite und in welche Richtung er danebenging |
| pro Waffe | was das Spiel über sich selbst gemessen hat: je Waffe und Flugzeit der Vorhalt-Faktor, die Streuung und ob der Schuss sicher, brauchbar oder eine Lotterie ist |
| Rangliste | welche Waffe wirklich trifft, mit Balken und mittlerer Fehlweite |
| Vorrang | die Prioritätenliste der Zielwahl |

## Vorrang: was ein Ziel zum besseren Ziel macht

Neun Kriterien, jedes mit einem Gewicht von 0 bis 100. Die Liste ist nach
Gewicht sortiert — oben zählt am meisten —, `▲`/`▼` tauschen ein Gewicht mit
dem Nachbarn, der Schieber stellt es fein ein, und der Haken schaltet ein
Kriterium ganz ab. Jedes antwortet mit einem Wert zwischen null und eins; die
Gewichte machen daraus eine Zahl, und das höchste Ziel gewinnt.

| Kriterium | wofür |
| --- | --- |
| freie Sichtlinie | nur, worauf ein Schuss überhaupt durchkommt |
| Nähe zum Fadenkreuz | wohin du ohnehin schon zielst |
| wer mich zuletzt traf | sofort zurückschlagen |
| Treffsicherheit | was die Waffe auf die Entfernung **gemessen** trifft |
| Nähe im Raum | der nächste Gegner zuerst |
| schon verwundet | wen ich selbst angeschlagen habe |
| Ziel behalten | nicht zwischen zwei Gegnern hin und her springen |
| trägt ein Powerup | Quad, Regeneration, Haste zuerst |
| in der Luft | fliegt berechenbar — ein bloßer Sprung zählt nicht |

Zwei Dinge sind dabei erwähnenswert, weil sie nicht selbstverständlich sind:

*Treffsicherheit* stützt sich nicht auf eine Annahme, sondern auf die Tabelle
aus dem Tab „pro Waffe". Eine Rakete, deren gemessene Streuung breiter ist als
ihr Splash-Radius, ist auf diese Entfernung ein Glücksspiel — und die Zielwahl
weiß das, ohne irgendetwas über den Gegner zu wissen.

*Schon verwundet* geht so weit, wie die Engine es zulässt: Quake 3 sendet die
Gesundheit anderer Spieler **nie** an den Client. Was der Client weiß, ist, wie
viel derjenige noch hatte, den er selbst zuletzt getroffen hat. Genau das wird
gemerkt und nach einigen Sekunden wieder vergessen, weil er inzwischen Health
aufgesammelt haben dürfte. Eine echte „wenigste HP"-Auswahl über alle Gegner
ist nicht möglich.

## Was dabei gelernt wird

Mit „je Waffe und Entfernung nachmessen" prüft das Spiel nach jedem Geschoss,
wie weit der Bot bis zum Ankunfts-Frame wirklich in seine Laufrichtung gekommen
ist — und schreibt das Ergebnis **in das Fach seiner Waffe und seiner Flugzeit**,
sonst nirgendwohin.

Das ist der wichtige Teil. Früher gab es **einen** gelernten Wert, den jeder
Schuss bewegte. Ein Schwung weiter Raketen zog ihn herunter und verkürzte damit
den Vorhalt für nahes Plasma mit, wo nie etwas gemessen worden war. Was auf
einer Entfernung gilt, gehört auf diese Entfernung.

| | was es ist |
| --- | --- |
| `cl_aimAssistLead` („Richtung halten") | **deine Vorgabe**, nicht gelernt: sie gibt dem Vorhalt seine Form |
| `baseq3/aimtune.cfg` | **das Gemessene**: je Waffe × Flugzeitband ein Faktor und die Streuung |

Vier Bänder (bis 0,4 s / 0,8 s / 1,3 s / darüber). Geschrieben wird scharf in
ein Fach, **gelesen weich**: zwischen den Mitten zweier Fächer wird
überblendet, damit eine halbe Zehntelsekunde Unterschied nicht zu einer anderen
Antwort führt, bloß weil ein Schuss ins Nachbarfach fiel.

Die Datei überlebt die Sitzung, weil ein Fach sich langsam füllt — eine
Handvoll Schüsse pro Abend. Der Konsolenbefehl `aimtune` gibt die Tabelle
jederzeit aus, samt der Gewichte, wie die Engine sie verstanden hat.

Nichts davon weiß etwas über die mitgelieferten Bots. Gemessen wird, was vor
der Waffe steht — andere Bots oder unvorhersehbare Bewegung ergeben einfach
andere Zahlen.

## Was im Protokoll steht

Jede Sitzung stempelt sich mit der Fassung ihrer Zeilen (`aim log: version …`).
Passt sie nicht zu der, die dieses Werkzeug kennt, sagt es das oben im Fenster,
statt stillschweigend Felder zu lesen, die es damals nicht gab.

| Zeile | wofür |
| --- | --- |
| `aim shot:` | jeder Schuss: Ziel, Entfernung, Vorhalt, Zielpunkt, Restfehler, der gemessene Faktor und die erwartete Streuung, ob der Ersatzpunkt griff |
| `aim pick:` | jede Änderung der Zielwahl mit dem Beitrag **jeder** Priorität und den Punktzahlen der übrigen Bewerber |
| `aim skip:` | warum die Hilfe **nichts** tat: kein Ziel, kein Durchkommen, eigener Splash |
| `aim learn:` | eine gemessene Probe: wie weit der Bot wirklich lief gegen die Erwartung |
| `aim drop:` | warum eine Probe **nicht** zählte: Ziel in der Luft, kaum Bewegung, Vorhersage von Geometrie beschnitten, schon beobachtet, Ziel weg, teleportiert, kein Snapshot im Ankunfts-Frame |
| `aim tune:` | der Stand eines Fachs nach jeder Änderung |
| `aim impact:` / `aim missile:` | jeder Einschlag mit den Stellungen aller Bots, jedes Geschoss beim Start |

Die letzten beiden machen den Löwenanteil des Volumens aus — und genau aus
ihnen kommt die Fehlweiten-Messung.

**Ein Fallstrick beim Auswerten:** die Bot-Stellungen einer `aim impact:`-Zeile
sind einen Server-Frame **zu spät**, weil das Ereignis erst mit dem nächsten
Snapshot ankommt. Für Hitscan steht die richtige Stellung in der Schusszeile
selbst (`plain` — das Ziel in dem Frame, gegen den der Befehl lief). Wer gegen
die Einschlagliste misst, bläht jede Fehlweite um eine Frame-Bewegung auf.

## Bauen und starten

```
dotnet build -c Release
```

Braucht .NET 10 mit Windows Desktop. Die Engine wird mit `build_mingw64.bat`
im Wurzelverzeichnis gebaut; im Feld „Spielordner" steht der Ordner mit der
`ioquake3.exe` und `baseq3`.

„Speichern" legt alle Einstellungen unter `%AppData%\HitsoundLab\settings.ini`
ab und holt sie beim nächsten Start zurück.
