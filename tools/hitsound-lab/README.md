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

Die Glättung dazwischen ist ein Ausgleich mit Mittelwert null — über alle
gezeichneten Bilder hebt sie sich auf. Über die **Schüsse** tat sie das nicht:
ein Waffentakt von hundert Millisekunden schwebt gegen einen Snapshot-Takt von
fünfzig und trifft immer wieder dieselbe Stelle der Sägezahnkurve, sodass das
Maschinengewehr mit einem festen Vorhalt von etwa sieben Millisekunden schoss —
immer vor das Ziel, nie dahinter, rund drei Einheiten. Der Ausgleich bleibt
jetzt auf **jedem** Schusskommando weg, nicht nur auf denen der Einzelschuss-
waffen. Und der Abstand, ab dem der eigene Splash die Hilfe aussetzen lässt,
richtet sich nach der Waffe: die früheren pauschalen 160 Einheiten passen zur
Rakete und sind für Plasma, dessen Splash zwanzig weit reicht, achtmal zu viel.

Sie ist **auf die lokale Verbindung und auf Bots begrenzt** und lässt sich
nicht gegen Menschen einsetzen: sie läuft nur über `NA_LOOPBACK`, und ein Ziel
kommt nur infrage, wenn der Server es in seinem Spieler-Configstring als Bot
ausweist. Das ist keine Einstellung, sondern steht so im Code.

**Sicht durch Wände.** Bots bekommen einen Drahtrahmen, Waffen und Powerups
einen Kasten mit Respawn-Zähler, dessen Farbe von Rot über Orange nach Grün
läuft, je näher das Ding am Zurückkommen ist.

**Rest-HP am Gegner.** Der Rahmen färbt sich nach dem, was der Bot beim letzten
Treffer noch hatte — grün unversehrt, über gelb und orange nach rot, wenn der
nächste Schuss reicht — und schreibt die Zahlen darüber (`45+20` heißt 45 Leben
und 20 Rüstung). Die Quelle ist dieselbe, aus der schon der Trefferton seine
Tonhöhe nimmt: der Server verrät, was der zuletzt Getroffene übrig hat.

Damit sind auch die Grenzen klar. Es steht nur bei Bots da, die **du selbst**
getroffen hast, es gilt nur für **den letzten** Treffer, und es verblasst über
zwölf Sekunden, weil sie inzwischen Health aufgesammelt haben können. Was der
Client nicht wissen kann, behauptet er auch nicht.

**Feuer halten.** Auf Wunsch bleibt der Abzug gesperrt, solange der Schuss
nicht durchkommt. Geprüft wird die Linie zu dem Punkt, auf den **wirklich
gezielt** wird — bei der Rakete also der vorgehaltene, nicht der Bot: einer
hinter einer Säule, dessen Vorhaltepunkt im Freien steht, ist ein Schuss wert;
einer im Freien, dessen Vorhaltepunkt hinter der Säule liegt, nicht. Dasselbe
gilt, wenn der eigene Splash einen erwischen würde.

Die Prüfung läuft von der Mündung, vierzehn Einheiten vor dem Auge, wie sie der
Server baut — und sie sieht auch **Türen und Aufzüge**, die der reine
Welt-Test durchlässt. Sie fällt, bevor der Schuss vorhergesagt wird, denn eine
Sperre löscht im Spiel den Waffentimer: eine Vorhersage, die den Schuss schon
gezählt hätte, liefe dem Server um einen Schuss voraus.

Zwei Dinge dazu, die man wissen muss. Gesperrt wird nur, **solange die
Zieltaste hält** — ohne Ziel gibt es kein „durchkommen", das man prüfen könnte.
Und eine Sperre **verzögert**, sie streicht nicht: das Spiel setzt den
Waffentimer auf null, sobald ein Befehl ohne Abzug ankommt, also feuert der
nächste Befehl mit Abzug sofort. Deshalb steht jede Sperre im Protokoll und
ihre Zahl oben im Tab „Trefferton" — vorher war eine wirkende Sperre von einer
toten Einstellung nicht zu unterscheiden.

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
| freie Sichtlinie | nur, worauf ein Schuss durchkommt — mit kurzer Nachwirkung |
| Nähe zum Fadenkreuz | wohin du ohnehin schon zielst |
| wer mich zuletzt traf | sofort zurückschlagen |
| Treffsicherheit | was die Waffe auf die Entfernung **gemessen** trifft |
| Nähe im Raum | der nächste Gegner zuerst |
| schon verwundet | wen ich selbst angeschlagen habe |
| Ziel behalten | nicht zwischen zwei Gegnern hin und her springen |
| trägt ein Powerup | Quad, Regeneration, Haste zuerst |
| in der Luft | fliegt berechenbar — ein bloßer Sprung zählt nicht |

Drei Dinge sind dabei erwähnenswert, weil sie nicht selbstverständlich sind:

*Freie Sichtlinie* hat als einzige Eigenschaft des Augenblicks eine Uhr, und
die hat sie sich verdient. Die Linie wird von einem Auge gezogen, das sich mit
jedem gezeichneten Bild bewegt, gegen einen Körper, der sich nur bewegt, wenn
ein Snapshot ankommt — an einer Kante kippt die Antwort deshalb mehrmals
**innerhalb eines Server-Frames**. Im Protokoll stand eine Zielwahl, die in
einer Viertelsekunde zehnmal zwischen zwei Bots hin und her sprang, und Schüsse
binnen einer Zehntelsekunde nach so einem Sprung waren **dreieinhalbmal
ungenauer** als der Rest — bei einem Sechstel aller Schüsse.

Eine Zehntelsekunde Gedächtnis beendet das. Das sind zwei Server-Frames, mehr
kann das Flackern nie überspannen, und länger ist es bewusst nicht: jede
Millisekunde davon ist eine, in der die Wahl an einem Ziel hängen könnte, das
wirklich hinter etwas verschwunden ist. Aus demselben Grund bekommt **nur das
Ziel, auf das gerade gezielt wird**, dieses Gedächtnis — sonst könnte die Hilfe
einen Bot hinter einer Wand aufgreifen, zu dem sie dann keinen Weg findet,
während ein erreichbarer im Freien steht. Nebenbei entscheidet das größte
Gewicht der Tabelle damit endlich etwas, statt jedem Bewerber dieselben hundert
Punkte zu addieren.

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

## Der Sprung und die Landung

Ein Spieler, der in Quake 3 in der Luft ist, hat keine Reibung und fast keine
Steuerung. Seine Bahn ist deshalb **exakt** vorhersagbar, nicht ungefähr: im
Protokoll lag die Vorhersage bei Zielen, die beim Einschlag noch flogen, im
Mittel **fünf Einheiten** daneben. Am Boden sind es hundert.

Der Haken war das Ende der Bahn. Eine Rakete fliegt hier rund eine Sekunde, ein
Sprung dauert zwei Drittel davon. Die Vorhersage trug den Bot danach einfach
mit Sprunggeschwindigkeit weiter, und das kostete 170 Einheiten. Die Zahl, an
der sich alles entscheidet, ist nicht „in der Luft" sondern **ob sich die
Fußlage während des Fluges ändert**:

| Zustand beim Schuss → beim Einschlag | Trefferquote |
| --- | --- |
| Boden → Boden | 59 % |
| Luft → Luft | 29 % |
| Luft → Boden | 21 % |
| Boden → Luft | 9 % |
| **unverändert** | **49 %** |
| **verändert** | **17 %** |

Deshalb wird der Moment, in dem die Bahn den Boden trifft, jetzt gelöst, und
was danach kommt, ist ein gedämpfter Lauf wie bei jedem anderen Läufer. Das
Feld `land` auf der Schusszeile sagt, wann die Hilfe die Landung erwartet hat,
damit sich am nächsten Protokoll nachprüfen lässt, ob es etwas gebracht hat.

Die Lernproben von Zielen in der Luft werden weiter **verworfen**, und das ist
richtig: gelernt wird allein die Haltedauer, und die kommt auf der Luftbahn gar
nicht vor. Eine solche Probe bestätigt sich immer selbst und würde die
Haltedauer nach oben ziehen, also den Vorhalt genau dort verlängern, wo er
schon zu lang ist.

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

Die **Streuung** eines Fachs ist der ganze Fehlschuss, nicht nur seine Hälfte.
Bis Dateifassung 3 zählte allein, wie weit das Ziel an seiner eigenen
Laufrichtung entlang danebenlag; was es **quer dazu** tat, ging verloren — und
das war der größere Teil: über eine Sitzung 22 Einheiten längs gegen 28
Einheiten quer. Die Streuung stand damit bei zwei Dritteln ihrer wahren Größe,
und ein gemessenes Fach wirkte zuverlässiger als ein ungemessenes. Eine ältere
`aimtune.cfg` wird deshalb verworfen statt weitergeschrieben.

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
| `aim hold:` | jede Sperre des Abzugs mit Grund und Entfernung, und wie lange sie hielt |
| `land` auf der Schusszeile | wann die Fußlage des Ziels wieder erwartet wird, oder −1 |
| `aim tune:` | der Stand eines Fachs nach jeder Änderung |
| `aim impact:` / `aim missile:` | jeder Einschlag mit den Stellungen aller Bots, jedes Geschoss beim Start |

Die Schusszeile hat in Fassung 5 aufgeräumt. `speed` war die **eigene**
Geschwindigkeit, obwohl die zweite Achse der Tabelle die des Ziels ist — jetzt
`pace` für das Ziel und `myspeed` für einen selbst, wie auf der Lernzeile.
`frame` war hier die Uhr des Befehls und auf jeder anderen Zeilenart die des
Snapshots, was jede Verknüpfung um bis zu ein Drittel Server-Frame verschob —
jetzt `cmd`, und `world` bleibt der Snapshot. `phase` ist ganz weg: die Zeile
wird nur auf einem Schusskommando geschrieben, und das bekommt den Ausgleich
nicht mehr, also hätte das Feld nur noch eine Zahl gedruckt, die auf nichts
angewandt wurde. Neu dazu kommt `swing` — wie weit die Sicht bis zum Schuss
kommen musste. Bei den schnappenden Waffen ist `error` bauartbedingt null, weil
die Sicht genau auf den Punkt gesetzt wird; erst `swing` sagt dort, wie viel
Arbeit das war.

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

Das Spiel kann sein `qconsole.log` nur neu schreiben, nie anhängen, also wäre
die vorige Runde weg, sobald die nächste beginnt. Beim Start wandert sie
deshalb mit ihrem Datum nach `baseq3\logs\`, und die jüngsten zwanzig bleiben
liegen. Eine Sitzung ist auf diese Weise schon verlorengegangen, bevor sie
ausgewertet war.
