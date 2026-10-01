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

**Zielhilfe** — die Hauptfunktion. Eine Taste wird gehalten, und die Sicht wird
auf das Ziel geführt, das die Prioritätenliste als bestes ausweist;
geschossen wird weiter von Hand mit der Feuertaste. Die Hilfe sagt den
Waffentimer voraus und setzt auf genau dem Befehl, auf dem der Server feuert,
den Zielpunkt auf die Stelle, gegen die der Server den Schuss wirklich prüft.
Dazwischen folgt sie weich, damit das Bild nicht ruckelt.

Ob die Sicht auf dem Feuerbefehl wirklich **auf den Punkt gesetzt** wird,
stellt „Schussmoment exakt“ ein (`cl_aimAssistExact`): „nie“, „nur
Einzelschuss-Waffen“ (Shotgun, Granate, Rakete, Rail, BFG) oder „alle
Waffen“. Bis zum 20.09.2026 konnte das Tool nur die mittlere Stufe setzen, und
die las sich wie „exakt“, hieß aber: Maschinengewehr, Plasma und Blitz folgten
auch im Schussmoment nur mit 0,32 des Wegs je Befehl. Gemessen kostete das
1,5–3 Punkte MG-Trefferquote auf 72 % der MG-Schüsse (Befund F03 in
`docs/prediction-review.md`); der Standard ist jetzt „alle Waffen“, und „nie“
bleibt für Vergleichsmessungen.

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

Sie ist **auf die eigene Partie begrenzt und zielt nur auf Bots**. Die Grenze
steht im Quelltext, nicht in dieser Datei: `CL_AimAssistSteer` und
`CL_AimAssistSnapshot` in `code/client/cl_input.c` kehren sofort zurück, wenn
die Verbindung nicht `NA_LOOPBACK` ist — also nicht der Server im selben
Programm —, und als Ziel kommt nur in Frage, wessen Configstring ein `skill`
trägt, was ein Mensch nie tut. Bis zum 20.09.2026 stimmte das nur halb: der
Kommentar behauptete die Grenze, die Prüfung fehlte, und ein Cvar
(`cl_aimAssistHumanTargets`) samt Haken im Werkzeug konnte Menschen als Ziele
freischalten. Beides ist weg, und ein LAN gilt ausdrücklich nicht als „eigene
Partie“: `Sys_IsLANAddress` nimmt jedes Subnetz einer lokalen Schnittstelle an,
und über ein VPN sitzen dort Fremde.

**Waffen auf der Karte.** In der Karteikarte „Spiel“ steht, womit die Karte
bestückt wird. Die Sockel bleiben, wo der Kartenbauer sie hingesetzt hat — nur
ihr Inhalt wird reihum auf die angehakten Waffen verteilt, und die
Munitionskisten genauso auf deren Munition. Die Karte behält damit ihre Dichte
und ihre Wege, es liegt nur überall dasselbe.

Wozu: eine Messreihe ist nur vergleichbar, wenn in jedem Lauf dasselbe
geschossen wird. Die Latenzreihe vom 19.09. ist genau daran gescheitert — 40,
25 und 80 Prozent MG-Anteil in den drei Läufen, und die Waffenmischung bewegte
die Zahlen mehr als die Latenz, die gemessen werden sollte.

Der erste Versuch dafür war falsch gebaut und ist es eine Stunde lang auch
geblieben: die nicht gewählten Waffen wurden über `disable_<classname>`
weggelassen, was Quake 3 von Haus aus kann. Das räumt die Karte aber aus — bei
„nur MG“ stehen elf Sockel leer und auf einem liegt etwas. Wegnehmen ist nicht
dasselbe wie einengen. Seitdem wird ersetzt statt entfernt:
`G_SubstituteSpawnItem` in `code/game/g_items.c` gibt zu jedem Fund der Karte
zurück, was stattdessen dort liegen soll, und `G_CallSpawn` in `g_spawn.c`
fragt es genau einmal — auf dem Weg, der von der Karte kommt. Was ein Spieler
fallen lässt und was `give` erzeugt, geht direkt an `G_SpawnItem` und bleibt,
was es ist.

Die Liste steht in `g_weaponSpawns`, durch Leerzeichen getrennt, mit oder ohne
`weapon_` davor. Das Werkzeug schreibt sie vor den `map`-Befehl, weil sie beim
Entstehen der Gegenstände gelesen wird, und mit `set` statt `seta`: eine
Laboreinstellung hat in der `q3config` nichts verloren. Leer heißt „Karte
unverändert“, und das Werkzeug schreibt auch dann leer, wenn **alles**
angehakt ist — dann gibt es nichts umzuverteilen.

Nachgeprüft am 20.09. mit zwei Bot-Matches über je 95 Sekunden auf q3dm17,
gleich bis auf die Haken. Unverändert: 461 Einschläge in sieben Arten, darunter
25 Rail, 35 Schrot, 55 Geschosse. Nur Railgun angehakt: 534 Einschläge in drei
Arten — **103 Rail statt 25**, kein Schrot, kein einziges Geschoss. Die Railgun
ist also nicht bloß übriggeblieben, sie hat die anderen Sockel übernommen, und
die Munitionskisten liefern die Slugs dafür.

Zwei Dinge lässt das bewusst in Ruhe. **Der Gauntlet** bleibt liegen, wo die
Karte ihn hat, rückt aber nie auf einen fremden Sockel nach — man trägt ihn
ohnehin immer bei sich, ein zweiter wäre ein verlorener Sockel. Und
**Powerups, Rüstung und Medipacks** werden nicht angefasst.

Eines lässt sich damit nicht abstellen: **du und die Bots starten immer mit
Gauntlet und Maschinengewehr**. Das ist Quake-3-Verhalten und hängt nicht an
den Gegenständen auf der Karte — in einer Railgun-Runde fällt es als
Kugel-Einschläge im Protokoll auf.

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
gilt, wenn der eigene Splash einen erwischen würde – aber nur, solange es
Eigenschaden gibt (siehe *Kein Schaden an dir selbst* weiter unten).

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
| pro Waffe | was das Spiel über seinen **Vorhalt** gemessen hat: je Waffe und Flugzeit der Faktor, die Streuung und ob der Schuss sicher, brauchbar oder eine Lotterie ist |
| Rangliste | welche Waffe wirklich trifft, mit Balken und mittlerer Fehlweite |
| Trefferquote | welche Waffe auf welche **Entfernung** ankommt — eine Matrix, Waffe nach unten, fünf Entfernungsfächer nach rechts |
| Korrekturen | jede einzelne Nachregelung: welches Fach, warum, und wohin sein Wert sich dadurch bewegt hat |
| Vorrang | die Prioritätenliste der Zielwahl |

Die beiden mittleren beantworten verschiedene Fragen und sind deshalb zwei.
„pro Waffe" misst, **wie weit** vorgehalten werden muss; „Trefferquote" misst,
**ob der Schuss ankommt**. Beides hängt an verschiedenen Größen, und für drei
der vier meistbenutzten Waffen gibt es die erste Tabelle gar nicht — Hitscan
hat keine Flugzeit und lernt nie etwas.

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

## Jede Waffe darf abweichen

Über der Liste steht, **für wen** sie gilt: „Standard" oder eine einzelne
Waffe. Was bei einer Waffe anders eingestellt ist, bekommt einen Pfeil und den
Standardwert daneben, „wie Standard" nimmt alles wieder zurück. Gespeichert
werden nur die **Abweichungen**, nicht neun volle Listen.

Der Grund steht im Protokoll. Ein Schuss, der fliegen muss, verliert mit der
Entfernung; einer, der sofort ankommt, nicht:

| Entfernung | Plasma | Rakete | Maschinengewehr |
| --- | --- | --- | --- |
| bis 400 | 72 % | 85 % | 94–96 % |
| 400–800 | 52 % | 45 % | 81–92 % |
| 800–1200 | 18 % | 33 % | 52 % |

Deshalb sind die Vorgaben so gesetzt: Blitzwerfer 95, Granatwerfer 85,
Schrotflinte 80, Rakete 70, Plasma 60 bei „Nähe im Raum", gegen 25 beim
Maschinengewehr und 10 bei der Railgun. Eine Waffe mit Flugzeit soll den nahen
Gegner nehmen, eine ohne den, auf den das Fadenkreuz ohnehin zeigt.

Nicht nur Gewichte lassen sich so trennen, auch die Dauern: `rocket.keep:30:2.5`
heißt, dass die Rakete ihr Ziel nur 2,5 Sekunden lang bevorzugt behält.

Eine Engine-Variable fasst 256 Zeichen. Das reicht für rund zwanzig
Abweichungen, und wenn es eng wird, steht die Zahl neben dem Knopf. Wird es zu
lang, schneidet die Engine stillschweigend ab — deshalb die Warnung.

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
| `baseq3/aimprio.cfg` | **deine Listen**: die allgemeine Vorrangliste und jede Abweichung je Waffe |
| `baseq3/aimtune.cfg` | **das Gemessene**: je Waffe × Flugzeitband ein Faktor und die Streuung |
| `baseq3/aimrate.cfg` | **das Gemessene**: je Waffe × Entfernungsfach Schüsse und Treffer |

Die beiden gemessenen Dateien werden alle fünfzehn Sekunden geschrieben, nicht
erst am Rundenende: ein Fach füllt sich mit einer Handvoll Schüssen pro Abend,
und ein Spiel, das anders als sauber endet, nahm vorher den ganzen Abend mit.

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

Der Konsolenbefehl `aimtune` gibt die Tabelle
jederzeit aus, samt der Gewichte, wie die Engine sie verstanden hat — die
Standardliste, und darunter je eine Zeile für jede Waffe, die davon abweicht.
Steht dort nichts, wurde auch nichts anders verstanden: ein Tippfehler im
Waffennamen fällt genau dadurch auf.

Nichts davon weiß etwas über die mitgelieferten Bots. Gemessen wird, was vor
der Waffe steht — andere Bots oder unvorhersehbare Bewegung ergeben einfach
andere Zahlen.

## Welche Waffe auf welche Entfernung ankommt

Die Tabelle darüber sagt nichts darüber, ob ein Schuss trifft — und kann es für
die halbe Waffenkammer auch nie sagen. Dafür gibt es eine zweite, `aimrate.cfg`,
und sie nimmt **jede** Waffe.

Gemessen wurde an rund viereinhalb Megabyte eigener Protokolle, 4182
unterstützten Schüssen aus sieben Sitzungen, was davon überhaupt etwas erklärt.
Ergebnis: die Entfernung, und sonst fast nichts.

| Waffe | bis 500 | 500–1000 | 1000–1500 | 1500–2000 | ab 2000 |
| --- | --- | --- | --- | --- | --- |
| Railgun | 92 % | 77 % | 82 % | 80 % | 88 % |
| Schrotflinte | 98 % | 77 % | 35 % | 38 % | – |
| Maschinengewehr | 90 % | 80 % | 67 % | 48 % | 26 % |
| Rakete | 73 % | 41 % | 9 % | 0 % | 0 % |

Dass die Railgun flach ist, ist der nützlichste einzelne Satz darin — und der
Grund, warum die Waffe eine eigene Achse braucht: auf 1000 bis 1500 Einheiten
steht die Rakete bei 9 und die Railgun bei 82 Prozent. Der Tab schreibt einer
solchen Waffe darum auch keine Lieblingsentfernung zu, sondern „überall",
sobald sich die Vertrauensbereiche ihres besten und schlechtesten Fachs
überschneiden.

**Was nicht zählt.** Das Tempo des Ziels — die zweite Achse der
Vorhalte-Tabelle — trennt hier nichts: am dortigen Schnitt bei 200 u/s ist die
schnelle Hälfte sogar 4,5 Punkte besser, und oberhalb von 400 sind es 11,7
Punkte bei einem Fehler von 5,1. Das eigene Tempo ist null, geduckt kommt in 13
von 2524 Schüssen vor. Alles das wurde gemessen und verworfen, statt geraten.

**Eines zählt doch**, und steht als zweites Zählerpaar im selben Fach statt als
dritte Achse: ob das Ziel während des Fluges aufsetzt. Das sind 23 Punkte, nach
Entfernung bereinigt. Als eigene Dimension würde es das dünnste Fach von 184
Schüssen auf 31 kürzen; als Zählerpaar kostet es acht Byte und keine Probe. Im
Tab ist es der dünne zweite Balken am unteren Rand einer Zelle — bei
Hitscan-Waffen gibt es ihn nie, und dass er fehlt, ist auch eine Aussage.

**Die Grenzen sind fest verdrahtet.** Gesucht wurden sie mit einer
Rasterschätzung über alle Schnitte in Hundertern: 500/1000/1500 kommt sowohl
für die Rakete allein als auch für alle vier Waffen zusammen heraus, und jedes
weitere Fach bringt danach immer gleich viel — das Kennzeichen dafür, dass nur
noch Rauschen angepasst wird. Eine mitwandernde Grenze liefe diesem Rauschen
nach und würde bei jedem Schritt alle gespeicherten Fächer still umbenennen.

**Drei Zustände je Zelle**, und sie sehen mit Absicht verschieden aus:

| Proben | Zelle |
| --- | --- |
| keine | leerer Kasten, „–" — nie getestet ist nicht dasselbe wie schlecht |
| 1–7 | Umriss, „misst noch (n)" — darunter ist das Wilson-Intervall breiter als 30 Punkte |
| 8–24 | schraffierter Balken, „NN % ±XX" — die Zahl gilt, ein Rang daraus nicht |
| ab 25 | voller Balken, „NN % (n)" |

Ein Fach behält 0,99 von sich je gebuchtem Schuss, langsamer als die 0,98 der
Vorhalte-Tabelle: ein Zähler braucht mehr Proben als ein Mittelwert, weil jede
Probe nur ein Bit trägt. Je gebuchtem Schuss und nicht je Sekunde — so behalten
die fünf Schüsse, die die Railgun pro Abend in ein Fach legt, vier Abende
Geschichte, während das Maschinengewehr den letzten beiden folgt. Eine
gemessene Notwendigkeit ist das Altern nicht: über sieben Sitzungen und vier
Baustände steht das Fach 500–1000 der Rakete bei 40/38/41/41/41/43/41 Prozent.
Es ist eine Versicherung gegen einen anderen Gegner oder einen anderen Server,
und darum darf es langsam sein.

**Was sich nicht ehrlich zuordnen lässt**, steht im Quelltext neben dem Code,
der damit lebt, und soll auch hier stehen:

* Zwei Raketen auf dasselbe Ziel — bei knapp fünfzehn Prozent kommt eine zweite
  innerhalb von hundert Millisekunden an. Gebucht wird trotzdem die ältere:
  93 Prozent dieser Paare liegen im selben Fach, das Raten kostet also rund ein
  Prozent falsch einsortierte Schüsse, beide wegzuwerfen kostete fünfzehn.
* Splash auf einen Umstehenden — `PERS_ATTACKEE_REMAINING` nennt nur die
  Gesundheit des zuletzt Verletzten, keinen Namen. Die Raketenzeile steht damit
  etwa fünf Prozent zu hoch, und daran ist nichts zu machen.
* Die einzelne MG-Kugel ist nicht zuzuordnen, bei keinem Versatz — fast neun
  von zehn liegen in einer Salve. Das **Fach** stimmt trotzdem: zwei
  aufeinanderfolgende Kugeln liegen dreißig Einheiten auseinander, tief in einem
  fünfhundert Einheiten breiten Fach.

Der Konsolenbefehl `aimrate` gibt die Tabelle jederzeit aus. Der Tab liest aber
die Datei und nicht das Protokoll — sie wird alle fünfzehn Sekunden
geschrieben, steht also live, braucht `cl_aimAssistDebug` nicht und überlebt
jedes Aufräumen der Protokolle.

## Welche Korrektur wann gemacht wurde

Der Tab „Korrekturen" zeigt jede einzelne Nachregelung: in welches Fach der
Schuss ging, was das Ziel tun sollte, was es stattdessen tat, wie weit der
Schuss am Ende danebenlag — und den Wert des Fachs **vor und nach** diesem
Schuss.

Dieses Paar ist der Grund, warum die Engine `was` und `now` mitschreibt. Der
`factor` auf derselben Zeile ist etwas anderes: der über bis zu vier Fächer
verblendete Wert an genau diesem Vorhalt, also was die Zielhilfe für diesen
einen Schuss gegeben hätte. Die einzelnen Korrekturen summierten sich deshalb
nie zur Bewegung des Fachs auf, in einem Topf sogar mit umgekehrtem Vorzeichen.

Der Balken wächst aus der Mitte statt von links, weil eine Korrektur eine
Richtung hat und kein Urteil: blau stärker, orange schwächer, unter einem
halben Hundertstel gar kein Balken — eine eingeschwungene Tabelle soll ruhig
aussehen, und ein Viertel aller Korrekturen ist so klein. Die Uhr läuft je
Runde, mit der Rundennummer davor, sobald es mehr als eine gab.

Für die meisten Waffen bleibt der Tab leer, und er sagt auch warum: gemessen
wird nur, was fliegt. In viereinhalb Megabyte Protokoll stammen **alle** 359
Proben von der Rakete, gegen 2743 Schüsse mit dem Maschinengewehr, die null
ergaben.

## Was im Protokoll steht

Jede Sitzung stempelt sich mit der Fassung ihrer Zeilen (`aim log: version …`).
Passt sie nicht zu der, die dieses Werkzeug kennt, sagt es das oben im Fenster,
statt stillschweigend Felder zu lesen, die es damals nicht gab.

Im Stempel stehen auch die Bedingungen, unter denen gespielt wurde: `rate` die
Nachladezeit in Prozent, `ammo` die unbegrenzte Munition, `autofire` der
selbsttätige Abzug, und `autohop`, `wdrop`, `wraise`, `air`, `airaccel`, `ramp`, `rampscale` und `step` die Quake-Live-Bewegung (die Bruchzahlen in Tausendsteln).
Ändert sich eine davon mitten in der Sitzung, wird neu gestempelt, und das
Werkzeug warnt in Rot, wenn eine Datei mehrere Bedingungen enthält – sonst
mischte die Auswertung unten Stichproben, die nicht dieselben sind.

Die Bewegung gehört in diese Reihe, weil sie die Messung genauso verändert wie
die Nachladezeit: Auto-Hop ändert, wie sich Bots und Spieler bewegen, also
Entfernungen, Flugzeiten und damit jede Trefferquote; die Waffenwechselzeit
ändert, wie oft im Gefecht überhaupt geschossen wird.

Steht bei den drei Bewegungsfeldern **`-1`**, hat das geladene Spielmodul diese
Physik gar nicht gekannt. Die Engine prüft das an `pmove_qlActive`, einer Cvar,
die nur ein Modul anlegt, das die Werte auch ausliest – dieselbe Sperre wie
`g_weaponRateActive`. Ohne sie stünde im Kopf eine Bedingung, die nie galt,
wenn Engine und Spielmodul getrennt veralten.

## Quake-Live-Bewegung

Eigene Karte im Werkzeug. Die Cvar-Namen sind die von Quake Live selbst, mit
großem Anfangsbuchstaben – genau so stehen sie in den **Factories**, mit denen
echte QL-Server ihre Spieltypen einstellen
([quakelive-server-standards](https://github.com/quakelive-server-standards/quakelive-server-standards)).
Damit lässt sich eine QL-Konfiguration hier unverändert übernehmen, statt sie zu
übersetzen.

| Schalter | Cvar | was er tut |
| --- | --- | --- |
| Auto-Hop | `pmove_AutoHop` | gehaltene Sprungtaste springt weiter, statt auf das Loslassen zu warten |
| Waffenwechsel | `pmove_WeaponDropTime` / `pmove_WeaponRaiseTime` | 200/200 statt 200/250 ms |
| Luftsteuerung | `pmove_AirControl`, `pmove_AirAccel` | dreht den Schwung im Sprung in die Blickrichtung |
| Rampensprung | `pmove_RampJump`, `pmove_RampJumpScale` | Aufwärtsschwung behalten statt überschreiben |
| Schritthöhe | `pmove_StepHeight` | wie hohe Stufen ohne Sprung genommen werden |

**Was id Software selbst wo einstellt** – aus den Factories abgelesen, nicht
geraten: `pmove_AirControl 1` und `pmove_RampJump 1` setzt **nur Race**. Duell,
Clan Arena, TDM und der Rest lassen beides aus. Die Waffenwechselzeiten ändert
ebenfalls nur Race, und zwar auf **10/10** – also praktisch sofort, nicht auf die
200/200, die hier als „wie Quake Live“ stehen. Die 200/200 sind die *Vorgabe* von
Quake Live, nicht das, was du auf einem Race-Server erlebst.

Alle sind `CVAR_SYSTEMINFO`-Cvars, genau wie `pmove_fixed` in ioquake3: der
Server besitzt den Wert, die Systeminfo trägt ihn zum Client, und **beide**
Module legen ihn in `pmove_t` – das Spielmodul in `ClientThink_real`, der cgame
in `CG_PredictPlayerState`. Das ist nicht Kosmetik: `bg_pmove.c` steckt in
beiden, und sagt der Client etwas anderes voraus, als der Server rechnet, zieht
es den Spieler bei jedem Sprung zurecht. **Nach einer Änderung an der Bewegung
muss deshalb `zz-hitpitch.pk3` neu ausgeliefert werden** – seit der
Zusammenführung mit dem Flexible HUD steckt das cgame mit darin.

Der cgame liest die Werte **aus der Systeminfo des Servers**
(`CG_ParseSysteminfo`), nicht aus seinen eigenen Cvars. Die Engine setzt nur die
Schlüssel, die ein Server schickt; ein Server ohne dieses Spielmodul schickt
keinen davon, und die Vorhersage rechnete sonst mit dem, was zuletzt eingestellt
war – Auto-Hop und Luftsteuerung, die dieser Server nie macht. Fehlt ein
Schlüssel, gilt das Original. Auch freie Zuschauer bewegt der Server mit
derselben Schritthöhe, die der cgame vorhersagt (`G_SetPmoveQL` für jeden
`Pmove`-Aufruf).

Das Werkzeug schreibt die acht Cvars mit `unset` und `set`, **nicht** mit `seta`:
sie sind Laboreinstellungen wie `g_selfDamage`. Mit `seta` blieben sie in der
q3config stehen, und ein Spiel ohne das Labor hatte trotzdem Quake-Live-Bewegung.
Das `unset` davor räumt ein Archiv-Flag weg, das ein älteres Labor dort
hinterlassen hat – beim nächsten Beenden verschwinden die Zeilen aus der q3config.

Bei allen Zahlen heißt `0` ausdrücklich *Original*, nicht null: 200/250 ms bei
den Waffenzeiten, 1,0 bei der Luftbeschleunigung, 18 Einheiten bei der
Schritthöhe. So muss keine Einstellung wissen, was Quake 3 vorgibt.

**Auto-Hop** braucht keinen Takt und kein neues Feld: gesprungen wird ohnehin nur
vom Boden, der nächste Sprung kann also erst nach dem Aufsetzen kommen. Quake
Live hat zusätzlich 100 ms Mindestabstand, die aber zu seinem Chain-Jump gehören.

**Die Luftsteuerung** dreht den vorhandenen Schwung in die Blickrichtung, statt
ihn zu beschleunigen – der Betrag bleibt, die Richtung wandert. Zwei
Einschränkungen sind das Wesentliche daran: sie wirkt nur bei `movementDir` 0
oder 4, also geradeaus oder gerade rückwärts, damit Strafejump unverändert
bleibt; und das Quadrat des Skalarprodukts geht ein, sodass ein Blick quer zur
Bewegung gar nichts tut.

Die Stärke ist die von CPM: `cpm_pm_aircontrol` steht im Promode-Code auf 150,
Xonotics CPMA-Profil nimmt dieselbe Zahl (`PM_AIRCONTROL_STRENGTH`).
`pmove_AirControl` ist wie in Quake Live nur der Schalter und Faktor darauf – 1 ist
Race und dreht den Schwung bei 400 u/s bis gut 260 Grad je Sekunde. **Bis
Protokollfassung 19 fehlte die 150**: `pmove_AirControl 1` drehte kaum zwei Grad
je Sekunde, weniger als die gewöhnliche Luftbeschleunigung ohnehin. Sitzungen mit
„Luftsteuerung“ aus dieser Zeit sind also Sitzungen **ohne** spürbare
Luftsteuerung; die Kopfzeile sagt das dazu.

**Der Rampensprung**: Quake 3 überschreibt beim Sprung die
Aufwärtsgeschwindigkeit (`velocity[2] = JUMP_VELOCITY`), ein Sprung von einer
Schräge frisst also genau den Schwung, den die Schräge gerade gegeben hat. Mit
`pmove_RampJump` wird er behalten, mit `pmove_RampJumpScale` multipliziert und
der Sprung darauf gelegt – nie weniger als ein normaler Sprung, höchstens 700
(`PM_RAMPJUMP_MAX`, in Quake Live `pmove_JumpVelocityMax`).

**Die Schritthöhe** ersetzt `STEPSIZE` in `PM_StepSlideMove`. Beide Stellen dort
nehmen dieselbe Zahl – die eine tastet nach unten, die andere hebt an; wären sie
verschieden, stiege man Stufen hinauf, die man nicht gesehen hat. Über 18 steigt
man mehr als 16 Einheiten auf einmal, und das Stufen-Ereignis kannte nur 4, 8, 12
und 16 – der Rest war ein Ruck der Kamera. Die wahre Höhe fährt jetzt im
`eventParm` mit, und der cgame glättet sie ganz. Auch die Vorhersage der
Zielhilfe steigt so hoch wie das Spiel; vorher endete ihr Vorhalt am Fuß jeder
Stufe, die ein Bot mit 28 einfach hinaufging.

### Der Abzug, der von selbst drückt

`cl_aimAssistAutoFire` (Haken: *mit der Zieltaste selbst abdrücken, wenn das Ziel
sicher ist*) drückt den Abzug, solange die Zieltaste hält und zweierlei stimmt:

1. **Nichts spricht gegen den Schuss.** Das ist genau dieselbe Prüfung, die
   *Nicht ins Leere schießen* benutzt, um den Abzug festzuhalten – gefragt wird
   einmal, gelesen von beiden. Der eine nimmt den Schuss weg, wenn etwas
   dagegen spricht, der andere gibt ihn, wenn nichts dagegen spricht. Bei
   Widerspruch gewinnt das Festhalten: der Haken hier fügt den Abzug nur hinzu.
2. **Die Sicht liegt schon auf dem Ziel.** Der Restfehler des vorigen Befehls
   muss kleiner sein als der Winkel, den der Körper auf dieser Entfernung
   überhaupt einnimmt – `atan(Trefferradius / Entfernung)`, also 1,7° für
   Hitscan auf 500 Einheiten und entsprechend mehr für Rakete und Granate, wo
   der Splash zählt. Ohne diese zweite Bedingung drückte er mitten im Schwenk
   ab, und das wäre kein sicheres Ziel, sondern nur ein früher Schuss.
Mehr verlangt er **nicht**, und das ist eine Entscheidung mit Vorgeschichte.

### Die Entfernung, und warum sie trotzdem nicht begrenzt wird

Die erste gemessene Sitzung mit dem selbsttätigen Abzug, 55 Raketen:

| Entfernung | Schüsse | Treffer | Flugzeit | gemessene Streuung |
| --- | --- | --- | --- | --- |
| 0–400 u | 6 | 50 % | 250 ms | 60 u |
| 400–800 u | 13 | 31 % | 600 ms | 139 u |
| 800–1200 u | 12 | 17 % | 1050 ms | 222 u |
| 1200–1600 u | 12 | 33 % | 1500 ms | 381 u |
| ab 1600 u | 12 | **8 %** | 2250 ms | 381 u |

Daraufhin stand hier eine vierte Bedingung: die gemessene Streuung musste in den
Wirkradius passen (120 Einheiten für die Rakete). Gemessen war das richtig, im
Spiel war es trotzdem falsch – durch kam nur noch die Klasse unter 400 Einheiten,
also **6 von 55** Gelegenheiten. Eine Waffe, die fast nie mehr schießt, ist keine
Verbesserung, auch wenn ihre Quote steigt. Die Bedingung ist wieder draußen.

Wer die aussichtslosen doch abschneiden will, hat dafür schon einen Regler:
*auch aussichtslose* (`cl_aimAssistHoldLottery`) als Vielfaches des Wirkradius.
Der erzeugt einen Grund, und ein Grund verbietet dem selbsttätigen Abzug den
Schuss ohnehin – es braucht dafür nichts Eigenes. Nach der Tabelle oben: **3,0**
schneidet ungefähr ab 1600 Einheiten ab, **2,0** ab etwa 1200, **1,0** lässt nur
noch die kurzen Raketen zu.

Warum der Rest vom *vorigen* Befehl: gedrückt werden muss, bevor der Waffentakt
den Schuss einbucht, geführt wird aber erst danach. Ein Befehl Verzug bei 60 bis
120 Befehlen je Sekunde ist weniger, als eine Hand je treffen könnte.

Drei Ausstiege setzen den Rest ausdrücklich zurück: kein Ziel, gar kein
Durchkommen, und Ziel so nah, dass die eigene Explosion mitginge. Ohne das
berief sich der Abzug noch zwei Bilder lang auf eine Zahl, die nicht mehr gilt –
im letzten Fall mit einer Rakete vor den eigenen Füßen. Die Gauntlet ist außen
vor: sie hat keine Linie, über die sich etwas entscheiden ließe.

**Für die Messung wichtig:** damit werden schlechte Gelegenheiten gar nicht erst
abgedrückt. Die Trefferquote steigt schon deshalb, ohne dass ein einziger Schuss
besser gezielt wäre. Wer mit und ohne vergleichen will, muss die Zahl der
abgegebenen Schüsse danebenlegen – der Stempel `autofire` im Protokollkopf ist
genau dafür da.

| Zeile | wofür |
| --- | --- |
| `aim shot:` | jeder Schuss: Ziel, Entfernung, Vorhalt, Zielpunkt, Restfehler, der gemessene Faktor und die erwartete Streuung, ob der Ersatzpunkt griff |
| `aim pick:` | jede Änderung der Zielwahl mit dem Beitrag **jeder** Priorität und den Punktzahlen der übrigen Bewerber |
| `aim skip:` | warum die Hilfe **nichts** tat: kein Ziel, kein Durchkommen, eigener Splash |
| `aim learn:` | eine gemessene Probe: wie weit der Bot wirklich lief gegen die Erwartung |
| `aim drop:` | warum eine Probe **nicht** zählte: Ziel in der Luft, kaum Bewegung, Vorhersage von Geometrie beschnitten, schon beobachtet, Ziel weg, teleportiert, kein Snapshot im Ankunfts-Frame, landet zu spät zum Laufen |
| `aim hold:` | jede Sperre des Abzugs mit Grund und Entfernung, und wie lange sie hielt |
| `land` auf der Schusszeile | wann die Fußlage des Ziels wieder erwartet wird, oder −1 |
| `aim tune:` | die **Korrektur**, die auf die Lernzeile darüber folgt: welches Fach, und mit `was`/`now` sein Wert davor und danach |
| `aim table:` | der **Abzug** der ganzen Vorhalte-Tabelle, vom Befehl `aimtune` und beim Verbindungsende |
| `aim land:` | eine gemessene **Landeprobe**: der Lauf ab dem Landepunkt entlang der Anflugrichtung, gegen die Erwartung des Lande-Fachs; `pace` wie angekommen, `run` wie geführt (nach der 320er-Kappe), `fall`/`rest` in ms, `landed` zählt diese Proben getrennt |
| `aim landtable:` | der Abzug der Lande-Fächer, zusammen mit `aim table:` |
| `aim rate:` | der Abzug der Trefferquoten-Tabelle, vom Befehl `aimrate` — je Fach Schüsse, Treffer, das Paar für aufsetzende Ziele und wieviel es sagen darf |
| `aim rated:` | ein abgeschlossener Schuss: Waffe, Entfernungsfach, getroffen oder daneben |
| `aim impact:` / `aim missile:` | jeder Einschlag mit den Stellungen aller Bots, jedes Geschoss beim Start |

Fassung 9 hat dem Aufsetzen eigene Fächer gegeben. Ein Ziel, das vor dem
Einschlag landet, wurde bis dahin mit seiner Fluggeschwindigkeit weitergeführt
und vom Läufer-Fach gedämpft, das nie von solchen Schüssen gelernt hatte. Jetzt
wird die Geschwindigkeit auf 320 gekappt, der Restlauf kommt aus `aim land:`-
Proben, und `tune` auf der Schusszeile ist bei `land` ≥ 0 dieser Lande-Faktor.
In `aimtune.cfg` stehen die Fächer auf `land`-Zeilen, die ein älterer Bau
überliest.

Fassung 7 hat die beiden `aim tune:`-Arten getrennt. Bis dahin sahen eine
Korrektur und ein Abzug der ganzen Tabelle gleich aus, und kein Feld sagte,
welche von beiden es war: `frame 0` als Kennzeichen zu nehmen ist für die 74
Abzüge falsch, die bei stehender Verbindung gemacht wurden, und die Nachbarschaft
zur Lernzeile stimmte nur, weil die zwei `Com_Printf` im selben Schleifendurchlauf
nebeneinanderstehen. Das hatte niemand aufgeschrieben, und die erste Zeile
zwischen den beiden hätte jeden Leser still kaputtgemacht. Jetzt heißt der Abzug
`aim table:` und die Frage stellt sich nicht.

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

**Hier stand ein Fallstrick, den es nicht gibt.** Bis hierher behauptete diese
Datei, die Bot-Stellungen einer `aim impact:`-Zeile seien einen Server-Frame zu
spät, weil das Ereignis erst mit dem nächsten Snapshot ankomme — wer dagegen
messe, blähe jede Fehlweite um eine Frame-Bewegung auf. Nachgemessen stimmt das
nicht.

Der Test: 529 Raketen aus einer Sitzung, `aim missile:` über die Geschossnummer
mit `aim impact:` verbunden, und die Flugzeit gegen die einkompilierten
900 u/s gehalten. Läge die Einschlagzeile einen Frame zu spät, stünde dort ein
Gipfel bei +50 ms. Es steht keiner da:

| Versatz | Raketen |
| --- | --- |
| **0 ms** | 391 |
| −50 ms | 132 |
| +50 ms | **0** |

Die sechs Ausreißer darüber sind wiederverwendete Geschossnummern. Nebenbei
bestätigt dasselbe Bild die 900 u/s: der Gipfel sitzt genau auf null.

Wer der alten Warnung folgte und um einen Frame verschob, hat sich damit
15–19 Einheiten Fehler pro Schuss eingehandelt — in genau der Auswertung, für
die diese Datei da ist. Für Hitscan bleibt `plain` auf der Schusszeile
trotzdem die bequemere Quelle, weil dort keine Verknüpfung nötig ist.

## Zielsuch-Raketen

Eigene Karte *Raketen*, zwei Gruppen: *Ziel und Lenkung* und *Flug und Zünder*.
Nachgebaut nach Anup Shindes Mod von
2007 ([Artikel](https://www.anupshinde.com/modifying-quake3/)), aber **nicht aus
dessen Code**. Der Mod liefert vier ganze Dateien zum Überschreiben
(`g_client.c`, `g_cmds.c`, `g_local.h`, `g_missile.c`) – das hätte alles gelöscht,
was hier in genau diesen Dateien steckt, und er hat Fehler, die mitgekommen wären:

| im Mod von 2007 | Folge | hier |
| --- | --- | --- |
| Lenkung über `think`, `nextthink` jedes Bild neu | der 15-s-Selbstzünder ist weg; jeder Fehlschuss ins All fliegt ewig, bis `G_Spawn: no free entities` | Lenkung in `G_RunMissile`, `think` bleibt |
| `self->client` ohne Prüfung | Absturz auf jeder Karte mit `shooter_rocket` | nur Spieler bekommen Zielsuche |
| `VectorMA( forward, 0.05, targetdir )` mit ungenormtem `targetdir` | auf 500 Einheiten wiegt das Ziel 25-mal die Flugrichtung: weit weg schnappt sie herum, nah lenkt sie sanft – verkehrt herum | feste Drehrate in °/s, ein echter Wendekreis |
| „Hysterese“ beim Zielwechsel | bevorzugt Ziele bis 100 Einheiten *weiter* weg, merkt sich nichts | Ziel wird gehalten, solange es gültig und sichtbar ist |
| Sicht über `contents & CONTENTS_SOLID` zu den Füßen | Treppenstufen verdecken Ziele | Spur zur Körpermitte |

Aus dem Mod nicht übernommen: die *variable Geschwindigkeit* (sie verglich
normierte Weltpositionen, also Winkel vom Kartenursprung aus – die schnellen
Stufen griffen nie) und das *Feuerwerk* (schoss aus der Lenkung weitere Raketen
ab, über eine globale Uhr für alle, ohne Grenze). Beides gibt es hier neu und
begrenzt: ein Tempoprofil über das Alter der Rakete und Splitter, die selbst
nicht mehr zerfallen.

| Regler | Cvar | Vorgabe | was er tut |
| --- | --- | --- | --- |
| Zielsuche | `g_homingRockets` | 0 | 0 aus, 1 nur die Raketen der Menschen, 2 alle samt Bots |
| Drehrate | `g_homingTurn` | 180 °/s | Wendekreis = 900 / (Rate in Bogenmaß): 180 °/s ≈ 290 Einheiten, 720 °/s ≈ 70 |
| Lebensdauer | `g_homingLifetime` | 15 s | danach zerlegt sie sich in der Luft, mit vollem Splash |
| Blickkegel | `g_homingCone` | 45° | halber Öffnungswinkel, in dem ein Ziel gesehen wird; 180° sieht nach hinten |
| jedes Bild neu wählen | `g_homingRetarget` | 0 | 1: jedes Bild neu nach der Zielwahl unten, auch mitten im Anflug |
| Ziel | `g_homingPick` | 0 | 0 das nächste, 1 der kleinste Winkel zur Flugrichtung – bis zur ersten Wahl vom Abschusspunkt aus gemessen, also wohin du gezielt hast –, 2 am leichtesten zu töten, 3 der Gegner, der dich zuletzt getroffen hat – auch über deinen Tod hinaus, der Rachefall ist genau der, der dich eben getötet hat |
| Wer | `g_homingAir` | 0 | 1 nur wer in der Luft ist (Luftabwehr), 2 nur wer am Boden steht – gilt beim Aussuchen; wer schon verfolgt wird, bleibt es beim Aufsetzen, auch mit „jedes Bild neu“ |
| Jagt | `g_homingMissiles` | 0 | 1 auch gegnerische Raketen, 2 nur Raketen (Abfangjäger) |
| Vorhalt | `g_homingLead` | 0 % | 100 %: auf den Treffpunkt statt hinter dem Ziel her; wer springt, fällt in der Rechnung mit – bis zum Boden unter ihm |
| Schärfzeit | `g_homingArm` | 0 ms | so lange geradeaus, ohne Zielsuche und ohne Zünder |
| Treibstoff | `g_homingFuel` | 0 s | die Lenkkraft nimmt gleichmäßig ab und ist danach weg; 0 = lenkt bis zum Ende; höchstens 60 s |
| Tempo | `g_homingSpeedStart`, `…End`, `…Ramp` | 900, 900, 1 s | gleichmäßig von Start auf Ende über die Rampe |
| Kurvenverlust | `g_homingDrag` | 0 % | enge Kurven kosten Tempo; 100 %: eine Vierteldrehung halbiert es ohne Nachschub, der Motor holt 900 u/s² gegenüber dem Tempoprofil auf |
| Am Ende | `g_homingSplit` | 0 | 2–4: statt zu zerplatzen in so viele Splitter, 25° gefächert, halber Schaden, suchen selbst mit vollem Treibstoff, zerfallen nicht noch einmal |
| Näherungszünder | `g_homingProximity` | 0 | zündet im Vorbeiflug so viele Einheiten vor der Körpermitte des Ziels; der Splash reicht 120 ab dem Körperrand |
| Warnton | `g_homingWarn` | 0 | ein Piepen nur für den Verfolgten, alle 800 ms auf 2000 Einheiten bis alle 120 ms aus der Nähe (`zz-homing.pk3`) |

**Raketen gegen Raketen.** Raketen haben keinen Körper – die Spur, mit der
`G_RunMissile` Einschläge findet, geht durch eine andere Rakete einfach durch.
Abgeschossen wird deshalb über den Abstand: kommen sich Jäger und gejagte Rakete
bis zum nächsten Bild näher als 40 Einheiten, platzen beide – mit Näherungszünder
auch weiter, aber höchstens 120, so weit die eigene Explosion reicht; eine Rakete
außerhalb davon „abzuschießen“ hieße, sie ohne Explosion verschwinden zu lassen.
Gerechnet mit der
Relativbewegung, denn zwei Raketen, die sich entgegenfliegen, sind in einem Bild
90 Einheiten weiter und würden sich zwischen zwei Prüfungen sonst verfehlen. Die
eigenen Raketen und die der Mitspieler sind nie Ziel. Dieselbe Rechnung dient
dem Näherungszünder gegen Spieler. Hätte die gejagte Rakete auf dem Weg zum
Treffpunkt eine Wand oder einen Spieler getroffen, wird nicht abgefangen – sonst
platzte sie auf der falschen Seite der Wand. Jeder Abschuss steht im games.log
als `Intercept: <Jäger> <Gejagter>: X shot down a rocket of Y`, im Stil der
`Kill:`-Zeilen – auch darin, dass ein Kartenschütze als `1022 <world>` dasteht.
Suchende Raketen bleiben aus der Trefferquoten-Tabelle (`aimrate.cfg`) heraus,
wie eine veränderte Nachladezeit: ihr Treffer landet nicht im Zeitfenster der
geraden Rakete.

**Der Vorhalt** löst |d + v·t| = Tempo·t exakt nach t auf. Eine schrittweise
Näherung liefe bei zwei gleich schnellen Raketen, die sich entgegenkommen, im
Kreis; die exakte Lösung trifft sich dort genau in der Mitte. Mit einem
Tempoprofil wird ein zweites Mal gerechnet, mit dem mittleren Tempo über genau
diese Flugzeit – eine Rakete, die von 300 auf 2000 beschleunigt, hielte sonst
viel zu weit vor. Wer springt, fällt in der Rechnung nur bis zum Boden unter
ihm; ohne das lag der Punkt bei einem Hasensprung Hunderte Einheiten unter dem
Boden, und die Rakete schlug davor ein.

Mit allen Reglern auf der Vorgabe wird die Rakete beim Lenken **nicht
schneller** – nur die Richtung wandert, 900 u/s bleiben. Mitspieler,
Unsichtbare, Tote, Zuschauer und der Schütze selbst sind nie Ziel. Die
Lebensdauer gilt nur für Zielsuch-Raketen: eine gerade fliegende trifft lange
vorher eine Wand, eine kreisende sonst erst nach einer Viertelminute. Geht ein
Schütze vom Server, zerfallen seine Raketen nicht mehr in Splitter – die
gehörten sonst dem, der seinen Platz als Nächster belegt.

Das Protokoll stempelt alles davon (seit Fassung 19: `homing` – 0, 1 oder 2, so
wie `fire_rocket` den Wert liest – und danach `hturn hcone
hnear hlife hprox hlead harm hfuel hv0 hv1 hramp hdrag hpick hair hwarn hsplit
hmiss`, Sekunden als Millisekunden, jeder Wert so gelesen, wie das Spielmodul
ihn liest – auch die Schalter: `g_homingPick 4` ist im Modul „das nächste“ und
steht deshalb als 0 da, nicht als 3). `g_homingActive` legt nur ein Spielmodul
an, das die Zielsuche auch fliegt – fehlt es, steht `homing -1`. Ein Regler, den
das Werkzeug per `set` angelegt, das geladene Modul aber nie registriert hat,
steht mit seiner Vorgabe da: ein älteres Modul fliegt ihn nicht. Ist die
Zielsuche an, gehört jeder dieser Regler zur Bedingung der Sitzung: eine Rakete
mit Vorhalt und Zünder ist eine andere Waffe als eine ohne, und die Auswertung
warnt, wenn beide in einer Datei stehen.

Die Engine-Zielhilfe rechnet ihren Vorhalt weiter mit 900 u/s. Mit einem anderen
Tempoprofil liegt ihr Vorhalt daneben – bei einer suchenden Rakete zählt er
aber wenig.

**Gemessen, fünf Bots auf q3dm17, je 4 Minuten** (`g_homingRockets 2`, sonst
Vorgaben): ohne Zielsuche 67–75 Kills, davon 13–15 durch direkte Raketen; mit
117–141 Kills, davon 66–90 direkt. Die Lebensdauer, je zwei Läufe à 3 Minuten:

| Lebensdauer | Kills | direkte Rakete | Raketen-Splash |
| --- | --- | --- | --- |
| 15 s | 91–96 | 62–65 | 9 |
| 1 s | 76–79 | 33–43 | 14 |
| 0,5 s | 65–70 | 17–26 | 15–20 |

Der Splash steigt, weil sich die Raketen jetzt neben den Zielen zerlegen. Kein
Absturz, keine Entitäten-Warnung, auch nicht bei 3600 °/s und 180° Kegel.

**Die weiteren Regler, je 150 s** (gleicher Aufbau, Zielsuche für alle). Die
Streuung zwischen zwei gleichen Läufen ist groß – ohne jeden Regler lagen die
direkten Raketen-Kills zwischen 39 und 50 –, also zählen nur deutliche
Unterschiede:

| Einstellung | Kills | direkt | Splash |
| --- | --- | --- | --- |
| Vorgabe (4 Läufe) | 77–86 | 39–50 | 5–12 |
| Näherungszünder 64 | 88 | 1 | 70 |
| Vorhalt 100 % (4 Läufe; drei davon 55–64 direkt, einer 34) | 68–84 | 34–64 | 1–3 |
| Schärfzeit 500 ms | 67 | 29 | 10 |
| Treibstoff 1 s | 68 | 25 | 10 |
| Tempo 400 → 1600 in 1,5 s | 74 | 43 | 10 |
| nur Raketen jagen | 42 | 9 | 9 |
| alle Regler zugleich | 77 | 2 | 62 |

Der Näherungszünder nimmt jedem Volltreffer den Platz – die Rakete platzt,
bevor sie den Körper berührt, und der Schaden kommt als Splash. Abschüsse von
Raketen in 120 s: „auch Raketen“ 5, „nur Raketen“ 11, „nur Raketen“ mit 180°
Kegel und 720 °/s 54; ohne den Regler keiner. Ein Chaos-Lauf mit vierfacher
Feuerrate, unbegrenzter Munition, vier Splittern alle 0,5 s und Raketenjagd lief
ebenfalls ohne Entitäten-Warnung durch.

Vor dem Commit haben vier unabhängige Durchsichten (Absturzsicherheit,
Mathematik, Werkzeug und Stempel, Versprechen) den Code gelesen und jede
Beanstandung von einem zweiten Leser widerlegen lassen; 24 blieben stehen und
sind behoben. Die wichtigsten: Splitter erbten mit dem Alter auch den
verbrauchten Treibstoff und konnten nie lenken; der Vorhalt ließ Springer durch
den Boden fallen; „wer mich zuletzt traf“ vergaß den Gegner nach jedem eigenen
Raketensprung, weil `lasthurt_client` auch eigenen Splash und Stürze zählt – es
gibt dafür jetzt `lastEnemyHurtClient`.

Zum Testaufbau selbst: `ioq3ded.exe` stürzte mit umgeleiteter Ausgabe bei gut der
Hälfte der Läufe ab, mit und ohne Zielsuche. Das war `CON_Show` in
`code/sys/con_win32.c`: ohne echte Konsole schlägt
`GetConsoleScreenBufferInfo` fehl, und die ungeprüfte Puffergröße schob den
Schreibzeiger irgendwohin. Mit der Prüfung liefen alle Läufe durch.

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

**Reichweite der Waffe.** Der Blitzwerfer kommt 768 Einheiten weit und dahinter
gar nicht — `LIGHTNING_RANGE` in `code/game/bg_public.h`. Bis zum 20.09.2026
stand dem nur ein hohes `Nähe`-Gewicht gegenüber, und das ist etwas anderes: es
zieht nahe Ziele vor, schließt ferne aber nicht aus. Stand nichts Näheres zur
Auswahl, wurde der Blitzwerfer weiter auf anderthalbtausend Einheiten geführt,
wo der Strahl nie ankommt. Der Schuss ist dort nicht unwahrscheinlich, sondern
unmöglich, und er kostet doppelt: die Hilfe steht auf einem Gegner, auf den
nichts geht, und der Schuss bucht in der Trefferquoten-Tabelle als Fehlschuss —
er drückt die gemessene Kurve der Waffe aus einem Grund, der mit Zielen nichts
zu tun hat. `CL_AimAssistWeaponReach` nimmt solche Ziele jetzt aus der Auswahl.

Der **Gauntlet steht dort ausdrücklich nicht drin**, obwohl er nur 46 Einheiten
weit schlägt: bei ihm ist das Hinterherlaufen der Sinn der Sache — man wird auf
den Gegner geführt, während man ihn einholt, und ob der Schlag ankommt,
entscheidet `CL_AimAssistInReach` beim Zuschlagen. Die **Granate** ebenso
wenig: sie fällt zwar nach etwa 660 Einheiten zu Boden, aber das ist der Bogen
und keine Wand — höher gezielt kommt sie weiter.

**Kein Schaden an dir selbst.** In der Karteikarte „Spiel“ abschaltbar
(`g_selfDamage`). Ein Raketen- oder BFG-Sprung trägt dann genauso weit wie
sonst und kostet nichts mehr: der Rückstoß wird im Spiel **vor** dem Schaden
verrechnet (`g_combat.c`, der Kommentar dort sagt es ausdrücklich — „calculated
after knockback, so rocket jumping works“), und genau hinter dieser Stelle
steigt der Schaden jetzt aus. Kein Schmerz-Ruckler, kein roter Blitz, kein
Leben weg — der Sprung selbst bleibt unverändert.

Damit fallen auch die beiden Splash-Sperren der Zielhilfe weg: ohne
Eigenschaden lässt die Lenkung einen Gegner innerhalb der eigenen Splash-Reichweite
(Rakete 160 Einheiten) nicht mehr los, und *Feuer halten* nimmt dir den Schuss
unter 120 Einheiten nicht mehr weg – auch der selbsttätige Abzug drückt dort
wieder. Beide Sperren schützten nur vor dem eigenen Schaden; mit
`g_selfDamage 0` verhinderten sie nichts als den Schuss auf den, der direkt vor
dir steht. Gemerkt an einem Abend, an dem fast jede Sitzung ein
`aim skip: rocket own splash` im Protokoll hatte. Der Rückstoß bleibt: eine
Rakete aus nächster Nähe schiebt dich weg wie ein Raketensprung.

Nur gegen dich selbst: wen dein Splash sonst noch erwischt, trifft er wie immer.
Mit den Trefferzählern hat das ohnehin nichts zu tun — die stehen hinter
`targ != attacker` und haben Selbstschaden noch nie gezählt.

**Sprungfelder in der Vorhersage.** Der Bodenpfad spurte nur gegen feste
Geometrie, und ein Sprungfeld ist keine — es ist ein Auslöser, durch den die
Spur hindurchgeht, als wäre dort nichts. Also lief die Vorhersage unbeirrt am
Boden weiter, während das Ziel hundert Einheiten hoch und fort war. Im
Protokoll waren das 8,6 % der verknüpfbaren Boden-Raketen mit einem mittleren
Fehler von rund 390 Einheiten; sie trafen 6 von 63 gegen 44 von 184 bei den
übrigen (Befund F01 in `docs/prediction-review.md`).

Jetzt wird der Lauf zusätzlich gegen jedes `ET_PUSH_TRIGGER` geprüft. Der
Server schickt die Felder ausdrücklich an die Clients, ihr Volumen ist das
Inline-Modell und `origin2` die Geschwindigkeit, die `BG_TouchJumpPad` dem
Getroffenen gibt — das cgame prüft für den eigenen Spieler genau so. Ab dem
Kontakt rechnet die Vorhersage die Wurfparabel statt des Laufs.

Gesucht wird entlang der **vollen** Laufstrecke, nicht der gedämpften: die
Dämpfung sagt, dass ein Bot die Richtung wechseln könnte, nicht dass er
langsamer liefe. Wer geradeaus läuft, ist zu der Zeit dort.

Auf der Schusszeile steht dafür `pad` — eins, wenn der Zielpunkt von einem
Wurf kommt. Ohne diese Spalte ließe sich nicht nachsehen, ob der Pfad
überhaupt je greift, und eine Änderung, die sich nicht nachmessen lässt, ist
eine Behauptung. q3dm17 hat dreizehn solcher Felder.

**Über die Kante gelaufen.** Das Gegenstück zum Sprungfeld: dort ging es
unerwartet nach oben, hier nach unten. Lief die vorhergesagte Strecke über eine
Plattformkante hinaus, fand der abschließende Boden-Trace zwar Boden — nur
hunderte Einheiten tiefer. Die Stufen-Prüfung (`STEPSIZE`, achtzehn Einheiten)
schlug fehl, und dann geschah **gar nichts**: der Zielpunkt blieb auf
Plattformhöhe über der Leere stehen, während der Bot längst darunter war.
Schwerkraft gab es nur für Ziele, die schon flogen.

Jetzt sucht `CL_AimAssistEdge` mit acht Sonden, wo die Plattform aufhört, und
ab dort wird gefallen. Nachgetragen wird **nur die Höhe** — die Bots bremsen im
Fall zum Landepunkt hin, sodass die gedämpfte Waagerechte in diesen Fällen
schon auf 17 bis 37 Einheiten stimmte; sie weiterzuschieben machte es
schlechter. Auf der Schusszeile steht `edge` dafür (Befund F26).

Auf q3dm17 ist das kein Randfall — die Karte ist eine Ansammlung von
Plattformen über dem Nichts. Die drei Prüfungen schätzten den Anteil
unterschiedlich (2,2 bis 5,1 % der Boden-Raketen) bei einem mittleren
Höhenfehler von 222 Einheiten.

**Und dann die ersten Daten.** In der nächsten Sitzung feuerten fünf Schüsse
mit `edge 1`, vier davon nachprüfbar: einer traf die Vorhersage auf zwanzig
Einheiten, einer war ein glatter Fehlalarm (der Bot stand beim Eintreffen noch
auf der Plattform), und zwei fielen um rund 350 Einheiten zu tief — einer
davon, weil der Bot die Lücke gesprungen statt hineingefallen ist. Mittlerer
Fehler 399 Einheiten gegen 127 bei den übrigen Raketen derselben Sitzung.

Das ist zu wenig, um es zu entscheiden, aber genug, um es nicht anzulassen:
bei einem langen Vorhalt reicht die Restzeit fast immer aus, um bis zum Boden
darunter zu fallen, sodass der Zweig aus „irgendwo über eine Kante“ ein
„steht unten“ macht. Deshalb ist `cl_aimAssistEdge` (Haken „Sturz über die
Kante anlegen“) **aus** voreingestellt: die Kante wird erkannt und als
`edge 2` protokolliert, der Zielpunkt bleibt aber stehen. So lässt sich an
einer Sitzung auszählen, wie oft der Bot beim Eintreffen wirklich unten war,
ohne dafür einen einzigen Schuss zu bezahlen. `edge 1` heißt angelegt.

**Die eigene Hand.** Während die Zieltaste hält, bewegt die Maus die Sicht
weiter — das ist kein Versehen, aber es gehört gemessen, bevor man es beurteilt.
Der Befehl wird in dieser Reihenfolge gebaut: erst Tastatur, dann Maus, dann
Joystick, und **danach** greift die Hilfe (`CL_CreateCmd`). Sie legt ihre
Korrektur also auf eine Sicht, die die Hand schon verschoben hat.

Auf dem **Schussbefehl** macht das nachweislich nichts. Dort setzt die Hilfe
die Sicht ganz auf den Punkt (Mischfaktor 1), und der Anteil der Hand kürzt
sich heraus — auch aus der Neunzig-Grad-Klammer, die den Schritt begrenzt. Im
Bestand vom 20. September: **3295 exakte Schüsse, kein einziger mit einem Rest
über 0,01°** — Rakete, Rail, BFG, Granate, Schrot und MG im exakten Modus.

Auf allen **anderen** Befehlen bleibt sie drin, und bei `cl_aimAssistExact 1`
sind das für MG, Plasma und Blitz auch die Schussbefehle. Gemessen an denselben
Sitzungen verlässt ein solcher Schuss den Lauf im Mittel 4 bis 5 Einheiten
neben dem Punkt, den die Hilfe wollte; knapp ein Drittel über 9 Einheiten, gut
ein Achtel über 18 — eine halbe Körperbreite. Wieviel davon die Maus ist und
wieviel der Mischfaktor, sagte das Protokoll bisher nicht.

Zwei Dinge also. Die Schusszeile führt jetzt `own` — die Gradzahl, die die
eigene Hand auf diesem Befehl beigesteuert hat, aus `oldAngles` gegen die Sicht
nach der Eingabe. Und der Haken **„Maus sperren, solange die Hilfe führt"**
(`cl_aimAssistFreeze`) verwirft sie: in `CL_CreateCmd` werden Nick und Gier
nach der Eingabe auf den Stand vor ihr zurückgesetzt. Eine Stelle für Maus,
Tastatur und Joystick zugleich; die Maus-Puffer werden normal geleert, also
schnappt die Sicht beim Loslassen nicht nach.

Gesperrt wird **nur, während die Hilfe wirklich auf ein Ziel führt** — nicht
schon, wenn die Taste hält. Sonst stünde die Sicht auch dann fest, wenn die
Hilfe gerade gar nichts tut (kein Ziel, tot, Zwischenstand), und dieselbe
Prüfung wie beim Zielen selbst hängt davor, damit die Taste auf einem fremden
Server niemals irgendetwas einfriert.

Der Preis steht dazu: wer die Maus sperrt, kann während des Haltens weder
umsehen noch ein anderes Ziel anvisieren — und weil die Laufrichtung in Quake
an der Sicht hängt, läuft man dorthin, wohin die Hilfe blickt. Das Fadenkreuz
ist der einzige Posten der Vorrangliste, der die Sicht überhaupt liest
(`cursor`, beim MG 80 von 100), und die gewählte Waffe wie die eigene Position
bleiben als Kanäle bestehen; das Anvisieren selbst fällt aber an die Liste.
Und die Liste hat beim geführten Griff überhaupt keinen Sichtkegel — die
dreißig Grad im Quelltext gelten nur für den Vergleichsgriff des Protokolls. Für eine saubere Messung ist das richtig, zum Spielen
nicht — deshalb ist der Haken aus voreingestellt. Mit ihm an muss `own` auf
jeder Zeile 0,00 stehen; das ist die Probe, dass er greift.

**Munition und Nachladezeit.** Zwei Stellschrauben, die nichts messen, sondern
das Messen beschleunigen: eine Messung soll nicht daran enden, dass die Waffe
leer ist, und wer hundert Raketen braucht, will nicht achthundert Millisekunden
je Schuss warten.

*Munition* füllt jedes Server-Bild auf **200** auf — nicht auf „unendlich".
Das Spiel kennt −1 als unendlich (`PM_Weapon` zieht dann nichts ab), aber
`BotUpdateInventory` kopiert `ps.ammo[]` roh in die Bot-Inventur, und die
Fuzzy-Logik der Waffenwahl vergleicht diese Zahlen gegen Schwellen. Eine −1
läge unter jeder davon: der Bot hätte unendlich Raketen und würde den
Raketenwerfer nie mehr nehmen. 200 heißt für beide Seiten dasselbe — und es
ist die Zahl, die der Rest des Spiels für „voll“ hält: `Add_Ammo` klemmt dort
ab, sodass ein Aufsammeln den Wert nicht mehr zurücksetzt. Dass
Munitionskisten damit liegen bleiben, ist keine Panne, sondern was „immer
voll“ bedeutet — und der Haken „Waffe wechseln, wenn die Munition leer ist“
wird ausgegraut, weil er nie auslösen könnte.
„Für alle" versorgt auch die Bots — dann laufen sie aber keine Munitionskiste
mehr an, und **genau diese Wege sind es, an denen die Vorhersage gemessen
wird**. Mehr noch: die Entscheidungen der Bot-Bibliothek zwischen Angreifen,
Fliehen, Verfolgen und Lagern hängen an Munitionsschwellen zwischen fünf und
fünfzig. Sind die dauerhaft erfüllt, greifen die Bots an, statt sich
abzusetzen. Eine Sitzung in diesem Modus misst also andere Bots als jede
davor; „nur für mich" ist der gemeinte Normalfall.

*Nachladezeit* ist ein Prozentsatz der normalen Zeiten und gilt **nur für
Menschen**: schössen die Bots mit, käme man vor lauter Einschlägen nicht zum
Messen. Sie steht in `PM_Weapon`, und dieselbe Rechnung — dieselbe Reihenfolge,
derselbe Boden von zehn Millisekunden — steht ein zweites Mal in
`CL_AimAssistFireDelay`. Das ist kein Versehen: die Zielhilfe sagt voraus, auf
**welchem** Befehl der Schuss rausgeht, und setzt die Sicht auf genau diesem
Befehl exakt auf den Punkt. Laufen die beiden Rechnungen auseinander, schnappt
sie auf dem falschen Befehl, und der ganze Gewinn von Befund F03 wäre weg.

**Der Boden liegt bei einem Server-Bild**, fünfzig Millisekunden, und das ist
keine Vorsicht, sondern eine Bedingung. Unterhalb davon fällt dreierlei
zusammen: der Spielerzustand trägt nur **zwei** Ereignisse je Bild, also gingen
Mündungsfeuer und Schussgeräusch verloren — und mit ihnen Schritt- und
Landegeräusche aus denselben zwei Plätzen; der Trefferton kommt einmal je
Schnappschuss; und die **Trefferquoten-Tabelle bucht höchstens einen Schuss je
Waffe und Bild**. Sie ruht darauf, dass keine Waffe schneller feuert als der
Blitzwerfer, und der trifft mit seinen fünfzig Millisekunden genau ein Bild.
Ohne den Boden hätte die Tabelle von sechs Schüssen einen gebucht und einen
Treffer gutgeschrieben — also gemessen, ob *irgendeiner* von sechs traf, und
das als Trefferquote in eine Datei geschrieben, die Sitzungen überdauert.
Fünfzig lässt der Rakete immer noch das Sechzehnfache und der Railgun das
Dreißigfache; Blitz und MG sind schlicht schon am Boden.

Drei Dinge noch, der Reihe nach ehrlich:

*Die Engine glaubt nicht der Wunschzahl.* `CL_AimAssistFireDelay` liest nicht
`g_weaponRate`, sondern `g_weaponRateActive` — eine Cvar, die **nur ein
Spielmodul anlegt, das sie auch anwendet**. Engine und Modul sind zwei Dateien,
die einzeln veralten können; würde nur die Engine neu eingespielt, rechnete
sie sonst mit einem Takt, nach dem auf dem Server niemand schießt. Damit ist
die Kopfzeile des Protokolls zugleich die Probe, dass beide zusammenpassen.

*Das Bild läuft nicht mit.* Das cgame sagt die Bewegung mit den Originalzeiten
voraus und setzt das Feld gar nicht — auch unser eigenes nicht, es braucht
keine Schusstakte, und das hier geladene kommt ohnehin aus einem fremden pk3.
Die Waffenanimation kann also zucken. Über den Schuss entscheidet der Server,
und die Zielhilfe liest dessen `weaponTime` aus dem Schnappschuss.

*Mehr Schüsse sind nicht mehr Lernproben.* Der Vorhalt-Lerner nimmt nur
Projektilwaffen und höchstens **eine offene Probe je Ziel** — zwei Proben auf
denselben Lauf wären derselbe Lauf, zweimal gezählt. Schneller schießen
erhöht dort gar nichts; dafür braucht es **mehr Bots**. Die
Trefferquoten-Tabelle dagegen bucht je Waffe und Bild und nimmt die
zusätzlichen Schüsse sehr wohl — bis zu zwanzig in der Sekunde.

Und weil eine Sitzung mit anderem Takt mit den alten nicht direkt vergleichbar
ist, stehen `rate` und `ammo` im Kopf des Protokolls, wird neu gestempelt,
sobald sich eines ändert, und die Werkbank schreibt die Bedingung neben die
Fassungsangabe — in Rot, wenn eine Datei mehrere davon enthält.

**Was die Nachladezeit der Trefferquoten-Tabelle antut.** Gemessen an vier
Sitzungen bei zwölf Prozent: die gebuchte Trefferquote der Rakete im ersten
Entfernungsfach fiel von 88 auf **22 Prozent** — während dieselben Schüsse
genauso genau zielten wie vorher (52,3 gegen 53,3 % innerhalb von 120 Einheiten
am Einschlag gemessen). Die Quote war also falsch, nicht das Zielen.

Der Grund sitzt auf der Gutschriftseite. `PERS_HITS` ist der einzige Zähler,
den der Server über eigene Treffer schickt, und die Zahl der *Schadensereignisse*
ist nicht die Zahl der Treffer: eine direkt einschlagende Rakete steppt ihn
zweimal, eine Schrotladung elfmal. Deshalb vergibt die Tabelle **höchstens
einen Treffer je Schnappschuss**. Das stimmt, solange eine Waffe langsamer
feuert als ein Server-Bild — bei zwölf Prozent fliegen aber zehn Raketen
gleichzeitig, zwei landen im selben Bild, und eine davon bucht als Fehlschuss.

Die Tabelle überdauert die Sitzung und verfällt nur langsam (0,99 je Probe,
also rund hundert wirksame Proben je Fach). Sie darf solche Zahlen gar nicht
erst sehen: **bei jeder Nachladezeit außer 100 % lernt sie nichts** und sagt
das einmal je Sitzung als `aim rateskip`. Eine schnelle Sitzung ist damit gut
für Vorhersage-Messungen am Einschlag und für Statistik über einzelne Schüsse,
aber sie trägt nichts zur Trefferquoten-Kurve bei.

## Warum die Bots in die Leere laufen

> **Stand 29.09.2026:** die Bremse, die hier beschrieben ist, hat die Bots an
> Kanten festgehalten und ist umgebaut — siehe „Warum die Bots zappelten“
> weiter unten. Die Ursachenforschung in diesem Abschnitt gilt weiter.

Die Klage ist berechtigt und lässt sich beziffern. Aus `games.log`, rund 6700
Tode: **937 MOD_TRIGGER_HURT** — das ist die Grube unter q3dm17 — plus 238
MOD_FALLING. Die zweite Zahl gehört allerdings nicht dazu: `EV_FALL_FAR` macht
zehn Schaden, das sind also Bots, die einen tiefen Sturz überlebt hätten, wenn
sie nicht ohnehin fast tot gewesen wären. Bleiben **14 % aller Tode**, die in
der Grube enden. Der Mensch liegt mit 21 % nicht besser — das ist ein Hinweis,
dass ein Teil davon die Karte ist und nicht die KI.

**Wer fällt warum.** Aus dem Protokoll der Zielhilfe ließen sich 46 Stürze
rekonstruieren (Positionen und Boden-Flag aller Bots je Einschlagsbild):

| | |
|---|---|
| Einschlag im Wirkradius in den 400 ms davor | **19** |
| selbst gesprungen und nicht angekommen | 6 |
| gelaufen, Tempo messbar (Median 322 u/s) | 6 |
| langsam heruntergelaufen | 2 |
| eigener Antrieb, Tempo nicht messbar | 13 |

Die ersten neunzehn sind **kein KI-Problem**. `G_Damage` addiert den vollen
Rückstoß auf die Geschwindigkeit und setzt `PMF_TIME_KNOCKBACK` für 50 bis 200
Millisekunden; in diesem Fenster lässt `PM_Friction` die Bodenreibung komplett
aus und `PM_WalkMove` fällt auf `pm_airaccelerate` zurück. Ein direkter
Raketentreffer gibt rund 500 Einheiten je Sekunde zur Seite, gegen die der Bot
physikalisch nicht anbremsen kann. Das ist derselbe Schubs, den du selbst
bekommst — deshalb liegen eure Quoten übereinander.

**Wo die Lücke ist.** Die Bots *haben* eine Kantenprüfung, aber nur auf dem
freien Weg (`BotWalkInDirection`, also Ausweichen im Kampf), sie schaut beim
Gehen zwei Bilder voraus — und vor allem: sie **verweigert nur den Befehl**.
Ein Bot mit 320 Einheiten je Sekunde bleibt davon nicht stehen; die Reibung
braucht rund fünfzig Einheiten Weg. Gebremst wird an keiner Stelle. Dem Weg
nach der Karte (`BotMoveToGoal`) fehlt selbst das: `TRAVEL_WALK` benutzt die
Lückenprüfung nur als Tempodrossel und läuft weiter, `TRAVEL_WALKOFFLEDGE` hat
gar keine.

Nachgezählt im `.aas` von q3dm17: von 2602 Verbindungen sind 365 „geh über die
Kante" und 225 „schieß dich mit der Rakete rüber". Die Karte ist so gebaut.

**Was der Haken tut.** `BotEdgeCare` sitzt in `BotUpdateInput`, zwischen dem
Abholen der Bot-Eingabe und dem Umbau in einen `usercmd` — die letzte Stelle,
an der die Bewegung noch ein Richtungsvektor in Weltkoordinaten ist. Läuft ein
Bot auf dem Boden schneller als hundert Einheiten je Sekunde und ist am Ende
seines Bremswegs auf tausend Einheiten nach unten **nichts**, wird rückwärts
gedrückt statt weiter vorwärts. Gemessen: in siebzig Sekunden mit sechs Bots
greift das 112-mal, jeweils bei 320 bis 327 Einheiten je Sekunde.

Drei Einschränkungen stehen absichtlich drin. **Springen bleibt seine Sache** —
über die Lücke zu springen ist auf dieser Karte die normale Art, sich zu
bewegen, also wird bei gesetztem `ACTION_JUMP` nichts angefasst. **Tausend
Einheiten**, weil ein gewollter Absatz auf q3dm17 im Mittel 174 tief ist; so
kann der Griff keine Strecke sperren, die der Bot wirklich gehen wollte. Und
die vier Richtungsflaggen werden mitgelöscht, weil `BotInputToUserCommand`
`forwardmove` und `rightmove` sonst stumpf überschreibt — ohne das wäre die
Bremse je nach Laune der KI wirkungslos.

**Und der zweite Haken: hüpfen.** In Quake läuft man beim Springen ohne
Bodenreibung weiter — wer hüpft, behält sein Tempo, statt es in jedem Bild ein
Stück zu verlieren. Die Bots machen das von sich aus nie; sie springen nur, wo
die Karte es verlangt (`TRAVEL_JUMP`, Sprungfeld, Hindernis) oder zufällig im
Gefecht.

`BotSpeedJump` sitzt hinter derselben Stelle wie die Bremse und drückt Sprung,
wenn vier Dinge zugleich gelten: der Bot steht auf dem Boden, läuft schon
schneller als zweihundert Einheiten je Sekunde, will weiter **in dieselbe
Richtung** (Skalarprodukt über 0,9), und vor ihm ist Boden. Beim Ausweichen im
Gefecht wird ausdrücklich nicht gesprungen — ein Bot in der Luft fliegt eine
Wurfparabel und ist damit *leichter* zu treffen, nicht schwerer.

Dass daraus überhaupt ein Hüpfen wird und nicht ein einzelner Sprung, liegt an
`PM_CheckJump`: die Taste muss zwischendurch los sein (`PMF_JUMP_HELD`). Weil
hier nur auf dem Boden gedrückt wird und in der Luft nicht, löst sich das von
selbst — beim Absprung gesetzt, während des Flugs nicht, bei der Landung
wieder.

**Für die Messung wichtig:** mehr springende Bots heißt mehr Ziele in der Luft,
und die trifft die Vorhersage deutlich besser als laufende (im Bestand 49–56 %
gegen 18–19 %). Eine Sitzung mit diesem Haken ist mit einer ohne **nicht**
direkt vergleichbar. Beide Haken sind aus voreingestellt.

## Warum die Bots herumstehen

Drei Gründe, alle im Original-Quelltext, alle nachgelesen.

**Erstens: jede Chatzeile kostet genau zwei Sekunden Stillstand.** `BotChatTime`
gibt stur `2.0` zurück, und `AINode_Stand` gibt in dieser Zeit **keinen
einzigen** Bewegungsbefehl — der Bot steht, hebt die Sprechblase, wartet. Die
Auslöser sind genau die falschen Momente: `AIEnter_Stand(bs, "battle fight:
enemy dead")` friert ihn direkt nach einem Kill ein, mitten im Gefecht. Dazu
„getroffen worden", „Zufallsgeplauder", „ins Spiel gekommen". Jedes `BotChat_*`
beginnt mit `if (bot_nochat.integer) return qfalse;` — der Haken **„Bots nicht
quatschen lassen"** setzt genau diese Cvar, mehr braucht es nicht.

**Zweitens, und das ist das Hängenbleiben auf Plattformen:** `BotAttackMove`
hat exakt **zwei** Versuche, sich zu bewegen — seitwärts, und bei Misserfolg
seitwärts andersrum. An einer Plattformkante führen beide über die Leere,
`BotWalkInDirection` lehnt beide ab, *ohne je einen Bewegungsbefehl zu geben*,
und die Funktion fällt mit einem Nullsatz heraus: `failure 0, blocked 0`. Der
KI-Knoten sieht also keinen Fehler, `BotAIBlocked` steigt in der ersten Zeile
wieder aus, und niemand merkt etwas. Der Bot steht einen ganzen Denkschritt —
hundert Millisekunden, nicht ein Bild — und wieder, solange er dort steht.

Die Rettung dafür steht seit id Software im Code, auskommentiert:

```c
//bot couldn't do any useful movement
//	bs->attackchase_time = AAS_Time() + 6;
```

`attackchase_time` schickt den Bot stattdessen den Weg nach der Karte zum
Gegner. Weil die Zeile nie läuft, ist der Zweig, der sie liest, toter Code.
Am Haken `g_botEdgeCare` wird sie jetzt gesetzt — mit **einer halben Sekunde
statt sechs**, denn sechs machen aus jedem Scharmützel eine Verfolgungsjagd.

**Drittens, hausgemacht:** die erste Fassung der Bremse hat die Bots aus ihren
eigenen Sprüngen gebremst. botlib kündigt einen Sprung über eine Lücke mit
`EA_DelayedJump` an und drückt erst ein Bild später wirklich; während des
Anlaufs steht nur `ACTION_DELAYEDJUMP`, und vor dem Bot ist naturgemäß nichts.
Geprüft wurde aber nur auf `ACTION_JUMP`. Auf einer Karte, auf der Springen die
Fortbewegung *ist*, heißt das festgenagelt. Beide Flaggen werden jetzt
respektiert — und die schönen Sturzzahlen der ersten Sitzung sind mit Vorsicht
zu lesen: ein Teil davon waren Bots, die schlicht nicht mehr hinübergegangen
sind.

Drei weitere Befunde stehen noch offen: „blockiert" heißt im ganzen Bot-Code
nur *eine andere Entität in drei Einheiten Abstand* — Weltgeometrie und
Plattformecken sind dem Ausweichsystem unsichtbar; zwei Bots auf einer schmalen
Plattform blockieren sich gegenseitig ohne dritten Versuch; und bei
Kampf-Können ≤ 0,4 gibt ein Bot gar keinen Bewegungsbefehl, solange der Gegner
in seiner Wunschentfernung steht.

## Menschlichere Bots, erste Stufe

> **Stand 29.09.2026:** die Haken dieses Abschnitts stehen jetzt auf der Karte
> „Bots“. „Bots beweglicher“ ist in zwei Zahlenfelder zerlegt (Kampfbewegung,
> Lagern), der Raketensprung ist eine Auswahl mit vier Einträgen.

Zwei Haken, beide aus voreingestellt, beide im Spielmodul — kein pak0, keine
Engine.

**„Bots auch nach oben kämpfen lassen"** (`g_botFightUp`). Im Original steht in
`BotAggression`:

```c
if (bs->inventory[ENEMY_HEIGHT] > 200) return 0;
```

Steht der Gegner mehr als zweihundert Einheiten höher, ist die Aggression
**null** — noch bevor Waffe oder Munition überhaupt angesehen werden. Und
`BotWantsToRetreat` feuert unter fünfzig, der Bot geht also weg. Auf q3dm17,
einer Karte, die aus nichts als Höhenunterschieden besteht, heißt das: kein
Kampf nach oben, und damit kein Kampf auf der halben Karte. Ein Mensch macht
das Gegenteil — das ist genau der Moment, in dem er Raketen auf den
Plattformboden schießt. Mit dem Haken gilt die Grenze nur noch für Waffen, mit
denen nach oben wirklich nichts auszurichten ist; wer Rakete, Rail, Blitz,
Plasma oder BFG mit Munition hat, darf hinauf.

Dazu ein zweiter, stiller Fehler derselben Ecke: `ENEMY_HEIGHT` und
`ENEMY_HORIZONTAL_DIST` werden nur in `BotUpdateBattleInventory` gesetzt und
**nirgends gelöscht**. `AINode_Seek_LTG` setzt `bs->enemy = -1` und ruft direkt
danach `BotWantsToCamp`, das wieder `BotAggression` liest — die
Lager-Entscheidung fällt also anhand der Höhe eines Gegners, der dreißig
Sekunden tot sein kann. An allen drei Stellen werden beide Werte jetzt
mitgelöscht.

**„Bots beweglicher"** (`g_botAttackSkill 0.9`, `g_botCamper 0`). Möglich wird
das durch eine Beobachtung, die alles aufschließt: `grep -rn "CHARACTERISTIC_"
code/botlib/*.c` liefert **nichts**. Jede Charaktereigenschaft wird
ausschließlich im Spielmodul gelesen — die Dateien in pak0 bleiben unberührt,
wir klemmen den Wert auf dem Weg nach draußen ab (`BotChar`, −1 lässt den
Originalwert stehen).

`CHARACTERISTIC_ATTACK_SKILL` ist dabei die ganze Leiter der Kampfbewegung:

| Wert | Verhalten |
|---|---|
| < 0,2 | der Bot gibt **gar keinen** Bewegungsbefehl |
| ≤ 0,4 | nur geradeaus auf den Gegner zu und zurück |
| > 0,4 | Umkreisen |
| > 0,7 | Umkreisen mit zufälligem Rhythmus — das, was ein Mensch tut |

Und `CHARACTERISTIC_CAMPER` auf null lässt `BotWantsToCamp` sofort aussteigen.
Lagern ist Stillstand, und Stillstand ist für diese Werkbank Gift.

**Beides ändert, wie sich die Bots bewegen** — also das, wogegen die Vorhersage
gemessen wird. Nach dem Einschalten braucht es eine neue Basislinie, bevor
gegen alte Zahlen verglichen wird.

**„Bots nach Treffern neu planen lassen"** (`g_botRethink`). Ein Bot wählt sein
Fernziel und sperrt es für **zwanzig Sekunden**:

```c
bs->ltg_time = FloatTime() + 20;   // ai_dmnet.c:308
```

Neu gewählt wird nur, wenn die Sperre abläuft, das Ziel erreicht ist oder die
Bewegung scheitert. **Schaden setzt sie nirgends zurück.** Ein Bot, der von
hundert auf dreißig fällt, läuft also bis zu zwanzig Sekunden weiter zu dem
Ziel, das sein gesundes Ich ausgesucht hat — meist zu einer Waffe, an der
Gesundheit vorbei.

Das Ärgerliche daran: die Gewichte in den Botdateien **sind** von Gesundheit
und Rüstung abhängig. Sie werden nur nicht neu ausgewertet, weil der
Wahlvorgang gar nicht stattfindet. Es genügt deshalb, die Sperre zu lösen — die
richtige Entscheidung trifft der Bot dann von allein.

Schwelle fünfundzwanzig Schaden (eine MG-Kugel macht sieben, ein Raketensplash
das Vielfache), und höchstens alle zwei Sekunden: sonst plant ein Bot unter
Dauerfeuer in einem fort neu, statt irgendwo anzukommen.

**„Bots die Respawn-Zeiten mitzählen lassen"** (`g_botTiming`). Das
Spielmodul kennt den Wiederkehr-Zeitpunkt jedes Gegenstands auf die
Millisekunde — `ent->nextthink = level.time + respawn * 1000`, mit
`RESPAWN_POWERUP 120`, `RESPAWN_ARMOR 25`, `RESPAWN_HEALTH 35` — und gibt ihn
**nie weiter**.

Die einzige Respawn-Kenntnis eines Bots ist seine private Vermeidungsliste, und
die wird nur scharf, wenn **er** den Gegenstand angefasst oder ausgewählt hat.
Nimmst *du* das Quad, bewertet jeder andere Bot es weiterhin, als läge es da:
die Anwesenheitsprüfung in `BotChooseLTGItem` fragt `li->entitynum` ab, und das
wird nach dem Verknüpfen nie wieder gelöscht. Die Bots laufen also zu einer
leeren Stelle — und wenn der Gegenstand nach zwei Minuten wiederkommt, steht
keiner dort. Item-Timing ist das Kennzeichen des guten Quake-Spielers, und der
Zugang dazu lag die ganze Zeit offen.

`BotItemTaken` schließt das: jede Aufnahme setzt bei **allen** Bots die
Vermeidungszeit auf die echte Respawn-Dauer, minus zwei Sekunden Vorlauf, damit
einer sich rechtzeitig auf den Weg macht statt erst loszugehen, wenn das Ding
schon wieder liegt. Die Level-Item-Nummer wird über den Aufsammelnamen
(`item->pickup_name`, also „Quad Damage") gesucht — die Bot-Gegenstandsliste
kennt keine Klassennamen.

**„Bots öfter Raketensprünge machen lassen"** (`g_botRocketJump`). Auf q3dm17
liegen **225** Raketensprung-Verbindungen, und der Ausführer in botlib ist
vollständig. Was sie in der Praxis verhindert, ist nicht die Charakterdatei —
26 der 33 Charaktere bestehen die Prüfung —, sondern die Klausel darüber:
mindestens 60 Leben, und unter 90 Leben zusätzlich 40 Rüstung. Mit dem Haken
reichen 55 Leben (so viel, dass der Sprung selbst den Bot nicht umbringt), und
die Sprungfreude aus der Charakterdatei wird übergangen.

Ehrlich dazu: dass die Klausel *die* Ursache der Seltenheit ist, ist belegt;
wie oft sie danach wirklich springen, ist Schätzung. Das muss eine Sitzung
zeigen.

**Nachgemessen: das Hüpfen bringt nichts.** Zwei headless-Läufe von je zehn
Minuten, sechs Bots, alle übrigen Haken an, nur `g_botJump` unterschiedlich:

| | Tode | in der Grube | |
|---|---|---|---|
| Hüpfen **an** | 151 | 18 | **11,9 %** |
| Hüpfen **aus** | 177 | 10 | **5,6 %** |

Der Anteil verdoppelt sich, und die Bots töten insgesamt seltener. Rund zwei
Sigma — nicht in Stein gemeißelt, aber die Richtung stimmt mit allem anderen
überein. Aus den Spielprotokollen dazu: mit Hüpfen waren die Bots **73 % der
Zeit in der Luft** statt 33–36 %, und ihr Tempo lag bei **291 u/s statt
310–312** — also *langsamer*.

Der Grund ist Quake-Physik, und er war vorhersehbar: Springen **erhält** das
Tempo, es erhöht es nicht. Der Gewinn beim echten Strafe-Jumping kommt aus dem
Zusammenspiel von Blickrichtung und seitlicher Beschleunigung in der Luft —
und die Blickrichtung des Bots gehört dem Zielen. Geradeaus hüpfen hat also nur
die Nachteile: eine Wurfparabel ist leichter zu treffen, und wer öfter in der
Luft ist, verpasst öfter die Landung.

Der Haken bleibt drin, damit man es selbst sehen kann. Empfohlen ist er nicht.

## Die Karte „Bots"

Alles, was die Bots betrifft, steht auf einer eigenen Karte, gleich nach
„Spiel". Oben zwei Knöpfe: **„Standard Q3"** stellt jeden Regler auf das
Original, **„Menschlich"** auf die empfohlenen Werte. Die Zeile darunter sagt,
welcher Stand gerade gilt. Wie viele Bots mitspielen und wie gut sie sind,
fassen die Knöpfe nicht an — das gehört zur Sitzung, nicht zum Verhalten.

| Regler | Cvar | Standard Q3 | Menschlich |
|---|---|---|---|
| nicht in die Leere laufen | `g_botEdgeCare` | 0 | 1 |
| in der Luft zurück ans Land steuern | `g_botAirControl` | 0 | 1 |
| Raketen ausweichen | `g_botDodge` | 0 | 1 |
| von Simsen ohne Wegnetz herunterkommen | `g_botUnstuck` | 0 | 1 |
| Raketensprung | `bot_rocketjump`, `g_botRocketJump` | wie der Charakter | alle, mit Gesundheitsklausel (gemessen ohne Wirkung, siehe unten) |
| hüpfen, um Tempo zu halten | `g_botJump` | 0 | 0 |
| Sprungfreude im Kampf | `g_botJumper` | −1 | 0,25 |
| Ducken im Kampf | `g_botCroucher` | −1 | −1 |
| auch nach oben kämpfen | `g_botFightUp` | 0 | 1 |
| kämpfen mit dem, was da ist | `g_botBrave` | 0 | 0 (gemessen schwächer, siehe unten) |
| Schüsse, Sprünge und Schritte hören | `g_botHear` | 0 | 1 |
| Entscheidung halten (s) | `g_botSteady` | 0 | 1,5 |
| Kampfbewegung | `g_botAttackSkill` | −1 | 0,9 |
| Reaktionszeit, Zielgenauigkeit, Zielkönnen, Wachsamkeit, Feuerdisziplin | `g_botReaction`, `g_botAimAccuracy`, `g_botAimSkill`, `g_botAlertness`, `g_botFireThrottle` | −1 | −1 |
| Herausforderung | `bot_challenge` | 0 | 0 |
| nach Treffern neu planen | `g_botRethink` | 0 | 1 |
| Respawn-Zeiten mitzählen | `g_botTiming` | 0 | 1 |
| im Kampf Gegenstände mitnehmen | `g_botGrab` | 0 | 1 |
| ohne Gegner dorthin gehen, wo Lärm war | `g_botHunt` | 0 | 1 |
| zweitbestes Ziel nehmen (%) | `g_botVariety` | 0 | 25 |
| Lagern | `g_botCamper` | −1 | 0 |
| Aufschlag für Fallengelassenes | `g_botDroppedWeight` | 1000 | 100 |
| nicht quatschen | `bot_nochat` | 0 | 1 |

**−1 heißt überall „aus der Charakterdatei".** Zielen und Reagieren bleiben
auch bei „Menschlich" beim Charakter: gegen diese Bots wird gemessen, und ein
Bot, der besser trifft, ist nicht menschlicher.

**Was „Standard Q3" nicht zurückdreht**, damit niemand es für das unberührte
Original hält: die beiden Enemy-Werte, die der Seek-Knoten seit der ersten
Stufe mitlöscht (ein liegengebliebener Wert, kein Verhalten), und das
Umwidmen ersetzter Sockel — das greift aber nur, wenn unter „Waffen auf der
Karte" eingeengt wird, und eine eingeengte Karte ist ohnehin nicht das
Original.

## Das Bot-Protokoll

`g_botLog 1` (Haken „Bot-Protokoll schreiben") legt im Homeverzeichnis neben
`qconsole.log` eine `botlog.log` an. Zehn Bots schreiben rund 400 Kilobyte je
Minute. Die Zeilenarten, jede mit Spielzeit in Millisekunden und Clientnummer:

| | |
|---|---|
| `T` | ein Denkschritt: Knoten, Ort, Tempo, Boden, Leben, Rüstung, Waffe, Gegner, Ziel, Reiseart, Flaggen, Knotenwechsel |
| `S` | ein Knotenwechsel mit dem Grund aus dem Quelltext |
| `G` | eine Zielwahl: `L` Fernziel, `N` Nahziel, `H` dem Lärm nach, `-` nichts gefunden — und warum das alte Ziel aufgegeben wurde |
| `I` | ein Gegenstand wurde genommen, von wem, und wann er wiederkommt |
| `K` | ein Tod: Opfer, Täter (1022 ist die Karte), Todesart |
| `B` | der Tritt hat am Boden eingegriffen: Ort, Tempo, Schwung, Befehl, Reiseart, Art des Eingriffs (1 ohne Sprung, 2 Schritttempo, 3 stehen, 4 gegen den Schwung, 5 zurück), und wo die Rechnung den Bot sonst enden sah |
| `A` | der Tritt hat in der Luft umgesteuert: Ort, Tempo, wohin |
| `U` | gestrandet, und wohin es hinausgeht |
| `P` | Abschuss von einem Sprungfeld: welches, was die Rechnung vorhersagt (landet ja/nein, warum, wo) |
| `M` | einem Pendel, einer Plattform oder einer Quetschfalle ausgewichen: welche, wie (1 warten, 2 zurück, 3 weg) |
| `D` | ein Treffer: Opfer, Täter, Schaden, von der Rüstung geschluckt, Waffe |

`g_botLog 2` schreibt zusätzlich `J` (ein Sprung, den die Rechnung für sicher
hielt, samt vorhergesagter Landung) und `B` mit Art 0 (keine Landung gesehen,
aber auch kein Ausweg). `g_botLog 3` dazu `F`: jedes Serverbild je Bot, mit Ort,
Tempo, Blickrichtung, Befehl und Eingriff — daraus sucht `tools/botlog/jitter.pl`
Zittern, das schneller ist als ein Denkschritt.

Weitere Auswertungen in `tools/botlog/`: `deaths.pl` (Tode durch die Karte nach
dem, was der Bot davor tat), `proxy.pl` (ein Stellvertreter gegen das Feld),
`dmg.pl` (Schaden im gemischten Spiel mit `g_botStockMask`), `sweep.sh` mit
`EXTRA="+set g_botLog 3"` für Zusatzargumente, und `run.sh` mit `QVM=<datei>`
für ein anderes Spielmodul, etwa das eingespielte zum Vergleich.

Ausgewertet wird mit `tools/botlog/botlog.pl`, mehrere Dateien stehen als
Spalten nebeneinander:

```
perl tools/botlog/botlog.pl lauf1/baseq3/botlog.log lauf2/baseq3/botlog.log
```

Und `tools/botlog/run.sh` fährt einen Lauf **ohne Spieler**: `ioq3ded` mit
eigenem Homeverzeichnis, Bots aus einer Config, nach einer festen Zeit ist
Schluss. Acht solche Läufe nebeneinander sind auf diesem Rechner kein Problem,
jeder braucht nur seinen eigenen Port.

Was gezählt wird:

| | |
|---|---|
| Stillstand | lebt, steht auf dem Boden, unter 20 u/s, mindestens eine halbe Sekunde |
| Zappeln | in zwei Sekunden über 150 Einheiten gelaufen und keine 60 von der Stelle gekommen |
| Kehrtwende | die Laufrichtung dreht zwischen zwei Denkschritten um mehr als 120° |
| Pingpong | Knoten A, dann B, dann wieder A, innerhalb einer Sekunde |

„Zappeln" im Kampf ist zum Teil gewollt — wer einen Gegner umkreist, macht
viel Weg und kommt nicht von der Stelle. Deshalb steht der Kampf in der
Auswertung getrennt, und deshalb ist die Kehrtwende das schärfere Maß.

## Warum die Bots zappelten

Gemessen, nicht vermutet. Alle Zahlen: q3dm17, zehn Bots auf Stufe drei, nur
Raketenwerfer auf der Karte, Läufe von fünf Minuten ohne Spieler.

**Es war die Kantenbremse — also hausgemacht.** Ein Bot stand 8,7 Sekunden an
einer Stelle und pendelte um zehn Einheiten, die Bremse griff bei jedem zweiten
Denkschritt:

```
T 106650 0 RETREAT -221 322 24 170 ... Bremse
T 106750 0 RETREAT -223 311 24 170 ...
T 106850 0 RETREAT -221 322 24 170 ... Bremse
T 106950 0 RETREAT -223 311 24 171 ...
```

Zwei Fehler, beide in `BotEdgeCare`:

1. **Der Boden wurde mit einem Punkt gesucht.** Ein Punkt fällt durch jede
   Lücke eines Gitterbodens. Nachgemessen an 316 Bremsungen: bei 190 fand
   dieselbe Probe mit der Standfläche des Körpers Boden, meist keine vierzig
   Einheiten tiefer. Sechzig Prozent der Bremsungen waren schlicht falsch.
2. **Gebremst wurde auch, wer genau dorthin wollte.** Von den übrigen 126
   zeigte bei 62 der eigene Bewegungsbefehl des Bots in dieselbe Richtung wie
   sein Schwung: sein Weg führte dicht an der Kante vorbei, und die Bremse
   hielt ihn vor jeder solchen Stelle fest — anlaufen, gebremst werden, wieder
   anlaufen.

Jetzt sucht die Probe mit dem Körper, und dem eigenen Weg wird getraut. Das
ist kein Leichtsinn: der Weg nach der Karte drosselt vor einer Lücke von sich
aus das Tempo (`BotTravel_Walk`), und die freie Bewegung rechnet zwei Bilder
voraus. Die Bremse ist für das da, was keiner von beiden sieht — Schwung nach
einem Rückstoß, einem Ausweichschritt, einer zu schnell genommenen Kurve.

| | Original | alte Bremse | neue Bremse |
|---|---|---|---|
| Tode in der Grube | 15,9 % | 9,7 % | **9,1 %** |
| Kehrtwenden im Kampf, je Minute | 5,4–6,3 | 16,2 | **6,2–7,2** |
| Bremse greift (Anteil der Lebenszeit) | – | 2,1 % | 0,3–0,6 % |

Die Stürze bleiben fast halbiert, die Kehrtwenden sind wieder beim Original.

**Was sich nicht bestätigt hat**, damit es niemand noch einmal sucht:

- *„Ein Bot ohne lohnendes Ziel steht herum."* In keinem einzigen Lauf blieb
  eine Zielwahl ohne Ergebnis (0,00 je Bot-Minute). Stillstand gab es bei den
  Original-Bots auf q3dm6 zu sechs bis zehn Prozent — das ist das Quatschen.
- *„Die Bremse hält Sprungfelder über der Leere für die Leere."* Bei 8 von 686
  Bremsungen lag ein Sprungfeld voraus. Stimmt im Prinzip, spielt keine Rolle.
- *„Ein kreisender Bot muss nur langsamer werden."* Gebaut (`g_botSteer`),
  gemessen, wieder entfernt: der Bot kreiste langsam weiter, und die Erkennung
  hielt jeden Ausweichschritt für einen Kreis. Das Kreisen selbst gibt es —
  im Original ein halbes Prozent der Lebenszeit, ein Bot verfehlt eine schmale
  Rampe und läuft im Bogen zurück an ihren Anfang — und es steht noch offen.

## Menschlichere Bots, zweite Stufe

Nach der Bremse blieben drei Dinge, die das Protokoll zeigt und die ein
Zuschauer als „gescriptet" oder „unentschlossen" liest.

**Die Bots sind drei Viertel der Zeit auf dem Rückzug.** `BotAggression` misst
an festen Schwellen — mehr als fünf Raketen, mindestens sechzig Leben, unter
achtzig zusätzlich vierzig Rüstung — und wer darunter liegt, läuft seine
Besorgungen ab und schießt nebenher. **`g_botBrave`** fragt stattdessen, ob
der Bot eine Waffe mit Munition hat und ob Leben und Rüstung zusammen siebzig
ergeben (die Rüstung zu zwei Dritteln gerechnet). Wer das Quad trägt, dem
stellt sich keiner, der es nicht selbst hat.

**Kampf, Rückzug und Verfolgung kippen bei jedem Denkschritt neu.** Alle drei
hängen an derselben Zahl, und die ändert sich mit jedem Treffer. Dazu zwei
feste Schleifen: der Rückzug schickt den Bot bei jedem Gegnerwechsel in den
Kampfknoten, der ihn im selben Denkschritt zurückschickt; und der Kampfknoten
nimmt die Verfolgung auf, sobald der Gegner für ein Bild hinter einer Ecke
verschwindet. **`g_botSteady`** lässt eine Entscheidung so viele Sekunden
gelten (wer seither dreißig Leben verloren hat, darf sofort neu), hält den Bot
beim Gegnerwechsel auf dem Rückzug und gibt einem verschwundenen Gegner vier
Zehntelsekunden.

**Zwei von drei Zielwahlen gelten etwas, das nicht da ist.** Der Grund jeder
Wahl steht im Protokoll, und 68 bis 81 Prozent lauten „gone": der Bot sieht
die leere Stelle und wählt neu. Drei Ursachen, drei Korrekturen:

- *Fallengelassenes.* botlib führt eine Waffe, die ein Toter fallen ließ,
  dreißig Sekunden lang in seiner Liste, auch wenn sie längst jemand genommen
  hat, und gibt ihr tausend Punkte Aufschlag. Mit `g_botTiming` erfahren jetzt
  alle Bots, dass sie weg ist; **`g_botDroppedWeight`** stellt den Aufschlag.
- *Zu früh da.* Das Mitzählen schickt einen Bot zwei Sekunden vor der
  Wiederkehr los, und das Original hält die leere Stelle dann für „Ziel
  erledigt". Jetzt wartet er — aber nur auf das, worauf auch ein Mensch
  wartet: Powerups, Rüstung, die großen Medipacks, eine Waffe, die er noch
  nicht hat. Für alles andere gibt es keinen Vorlauf mehr.
- *Ersetzte Sockel*, siehe unten.

**Und einer stand achtzig Sekunden auf einem Sims.** Im letzten Messsatz
fand ein Bot 517-mal hintereinander kein Ziel: ein Kampfsprung hatte ihn von
der oberen Plattform auf ein Sims an der Außenwand gebracht, ein Zierrat,
den der Kartenbauer nie zum Betreten gedacht hat. Ein Bot kennt die Karte
nur als Netz von Feldern, und von dort führt kein Weg zu irgendeinem Ziel —
also stand er, schoss von dort auf alles, was vorbeikam, und wartete, bis ihn
jemand traf. Das ist der Fall, den ein Zuschauer als „weiß nicht, was er tun
soll" liest, und er tritt selten auf (einmal in rund fünfundzwanzig Läufen),
dann aber lange.

**`g_botUnstuck`**: hat ein Bot zehn Denkschritte lang kein Ziel oder keinen
Weg gefunden, sucht er in sechzehn Richtungen den nächsten Boden, von dem aus
das Wegnetz wieder dorthin führt, wo er zuletzt einen Weg hatte — und geht
hin, über die Kante. Das Sims selbst zählt nicht: es *hat* Verbindungen im
Wegnetz, sie führen nur nirgends hin, weshalb die erste Fassung genau dort
nichts tat. Und die Grube zählt nicht, sie hat auch einen Boden. Findet er
nichts, springt er nach drei Sekunden irgendwohin — und sei es in die Grube.
Das ist, was ein Mensch auf einem Sims auch tut. Geprüft, indem ein Bot im
Test dorthin gesetzt wurde: ohne den Haken blieb er, mit ihm war er nach vier
Sekunden unten.

Dazu fünf Dinge, die das Original gar nicht kann:

| | |
|---|---|
| **`g_botHear`** | Die Bots sind taub; jedes Geräusch fällt in einen leeren Zweig unter id Softwares eigenem FIXME. Jetzt bemerkt ein Bot, wer in Hörweite schießt (1500), springt (600) oder läuft (400) — auch im Rücken. Sehen muss er ihn trotzdem können, und wer geht oder geduckt läuft, bleibt lautlos. |
| **`g_botHunt`** | Ohne Gegner und gut ausgestattet geht der Bot dem letzten Geräusch nach statt zur nächsten Kiste. |
| **`g_botDodge`** | Ein Schritt quer zur Flugbahn, wenn eine Rakete in unter 0,6 s näher als 90 Einheiten vorbeikommt. Nie über eine Kante. |
| **`g_botGrab`** | Im reinen Kampf nimmt ein Bot im Original nichts auf. Jetzt fragt auch dieser Knoten nach nahen Gegenständen. |
| **`g_botVariety`** | Die Zielwahl ist streng — Gewicht durch Wegzeit, das höchste gewinnt, immer. Mit 25 nimmt der Bot jedes vierte Mal das Zweitbeste. |

**Gemessen**, dieselbe Aufstellung wie oben:

| | Original | Menschlich |
|---|---|---|
| Pingpong je Bot und Minute | 16,7–18,9 | **4,7–6,0** |
| Knotenwechsel je Bot und Minute | 80–88 | 51–54 |
| Fernziel-Wahlen je Bot und Minute | 51 | 26 |
| im Kampf / auf dem Rückzug | 11 % / 76 % | 22–26 % / 61–65 % |
| Kehrtwenden im Kampf, je Minute | 5,4–6,3 | 6,9–7,0 |
| Tode in der Grube, Anteil | 15,9 % | 11,4 % |
| Tode in der Grube, je Lauf | 27 | 17 |

Der Anteil liegt höher als mit der Bremse allein (9,1 %), die Zahl nicht (17
gegen 16 je Lauf): es wird insgesamt weniger gestorben, seit die Bots Raketen
ausweichen.

Und das Ausweichen für sich, je drei Läufe, sonst alles gleich: **148 statt
171 Tode** in fünf Minuten, davon 79 statt 100 durch Raketen, 14 statt 19 in
der Grube.

Auf q3dm6 mit vier Bots — eine Karte, auf der die Bots die halbe Zeit keinen
Gegner sehen: Pingpong 7,3–8,1 statt 11,9–12,9, kein Stillstand statt sechs
bis zehn Prozent, und rund neun Gänge „dem Lärm nach" je Bot in fünf Minuten.

Der Preis, damit er nicht überrascht: außerhalb des Kampfs wechseln die Bots
dort öfter die Richtung als das Original — 5,0 bis 7,6 Kehrtwenden je Minute
gegen 3,0 bis 3,7. Je zwei Läufe mit einem Regler weniger: ohne die Jagd 4,3,
ohne die Abwechslung 4,6 bis 5,7, ohne beides und ohne das Hören 4,1 bis 4,3.
Den größten Teil macht also die Jagd: wer dem Lärm nachgeht, dreht um, wenn
der Lärm woanders ist. Ob das lebendig wirkt oder unentschlossen, muss das
Spielen zeigen — der Haken ist einzeln abschaltbar. Auf q3dm17 mit zehn Bots
spielt es keine Rolle, dort ist ein Bot ein Prozent seiner Zeit ohne Gegner.


**Raketensprünge gibt es trotzdem nicht.** In keinem Lauf – Original oder
„Menschlich", q3dm17 oder q3dm6 – kam ein einziger Denkschritt mit der
Reiseart Raketensprung vor. Die Klausel aus der ersten Stufe war nie das
einzige Hindernis: q3dm17.aas rechnet einen Raketensprung mit Wegzeit 300 und
ein Sprungfeld mit 200, also gewinnt bei der Wegsuche immer das Feld; und auf
dem Rückzug, wo die Bots zwei Drittel ihrer Zeit sind, erlaubt das Original
den Sprung gar nicht (`AINode_Battle_Retreat` setzt `TFL_ROCKETJUMP` nie).
Der Regler bleibt, weil er auf Karten wirkt, auf denen der Raketensprung der
einzige Weg ist – hier ist er ohne Wirkung.

**Was das für die Messung heißt:** fast jeder dieser Regler ändert, wie sich
die Bots bewegen — also das, wogegen die Vorhersage gemessen wird. Nach dem
Umstellen braucht es eine neue Basislinie.

## Alle Karten, und der Tritt

Der Stand „Menschlich" war auf q3dm17 und q3dm6 gemessen. Der Lauf über alle
25 Karten (zwölf Bots, vier Minuten, ohne Spieler: `tools/botlog/sweep.sh`)
hat gezeigt, was dabei unterging: das Pingpong war überall halbiert, aber auf
den Karten mit Grube oder Lava starben die Bots **öfter** durch die Karte als
das Original — auf den sechs auffälligsten 92 Tode je Lauf gegen 72.

Die Todesursache steht seitdem im Bot-Protokoll (`K`), und `tools/botlog/`
kann damit nachsehen, was ein Bot in den Sekunden vor seinem Sturz getan hat
(`deaths.pl` nach Klassen, `trace.pl` Zeile für Zeile). Drei Ursachen:

- **Die Bremse sah nur den Schwung.** Auf q3dm9 biegt der Weg an einer
  Plattformecke auf einen Steg ab. Der Bot hat noch Schwung geradeaus, dort
  ist Leere, die Bremse schiebt ihn zurück — und er tritt einen halben Schritt
  neben dem Steg von der Kante. Fünf von acht starben so; die zwei, bei denen
  die Bremse nicht griff, kamen heil hinunter.
- **Der eigene Weg galt als sicher.** Auf q3dm18 laufen die Bots (auch die des
  Originals) mit vollem Tempo auf eine Treppe aus schwebenden Absätzen und
  fliegen über den ersten hinaus.
- **Die Kampfbewegung prüft mit dem Wegnetz**, und das kennt keine Todeszonen:
  der Boden einer Grube ist dort ein Landeplatz wie jeder andere.

`BotFooting` (in `code/game/ai_main.c`) rät deshalb nicht mehr, sondern
rechnet: in jedem Bild des Servers die nächsten vier Zehntelsekunden am Boden
mit Reibung und Beschleunigung wie im Spiel, Stufen und Rampen eingeschlossen,
und wenn der Bot dabei den Boden verliert, den Flug bis zur Landung. Keine
Landung sind Leere, Lava, Schleim, eine Todeszone, ein Aufprall, den er nicht
überlebt — und die bloße Kante einer Plattform. Dann bekommt er das Mildeste,
was hilft: kein Sprung, Schritttempo, stehenbleiben, gegen den Schwung.
**Auf seinem Weg nach der Karte wird er nie angehalten** — die Zwischenfassung
tat das, und die Bots standen auf q3dm9 je Lauf fünfzig Sekunden an Kanten,
über die ihr Weg führte.

In der Luft (`g_botAirControl`) steuert er dorthin, wo die Rechnung wieder
eine Landung findet, aber nur, wenn der Flug nicht der geplante ist: ein
Treffer hat ihn geworfen, oder er ist im Flug mit jemandem zusammengestoßen.
Ein Sprungfeld bleibt in Ruhe — das auf q3dm9 wirft durch einen Torbogen, und
sechzehn Einheiten Korrektur reichten, um mit dem Kopf anzuschlagen.

Gemessen, sechs Karten mit Grube oder Lava (q3dm7, 9, 10, 13, 15, 18), Tode
durch die Karte je Lauf:

| | Original | alte Bremse | Tritt |
|---|---|---|---|
| Summe | 72 | 92 | 40 |
| im Kampf gesprungen | 13 | 15 | 0,3 |
| im Kampf gelaufen | 6 | 8 | 3 |
| auf dem Weg gelaufen | 18 | 26 | 9 |
| vom Treffer hinausgeworfen | 16 | 19 | 6 |

Über alle 25 Karten: 205 Tode durch die Karte statt 428, Pingpong 12 statt 26
je Bot-Minute, Stillstand 2,7 statt 2,9 Prozent. Kein Absturz, kein Bot, der
hängt.

**Und wer gewinnt?** `g_botStockMask` lässt im selben Spiel einen Teil der
Bots als Original laufen (ein Bit je Clientnummer). Sechs gegen sechs auf neun
Karten, je zwei Läufe mit vertauschten Rollen, damit die Charaktere sich
aufheben (`tools/botlog/duel.pl`): die Menschlichen erwischen die Originale
906-mal, umgekehrt 829-mal (1,09 zu 1), sterben 48-mal durch die Karte statt
126-mal, und kommen auf 1,02 Abschüsse je Tod gegen 0,89.

Was damals offen blieb — Pendel, schwebende Plattformen, die Sprungfelder auf
q3tourney6 — steht weiter unten unter „Sprungfelder, Pendel und Plattformen".

## Stufe 5, und wer eigentlich leichter ist

Der Einwand aus dem Spiel: auf „Menschlich" zittern Bots herum, und sie sind
leichter als die originalen. Beides ließ sich klären.

**Gespielt wurde ein alter Bau.** Das Spiel lädt sein Spielmodul aus
`Documents\ioQuake3\baseq3\zz-hitpitch.pk3` (steht in `qconsole.log`), und dort
lag der Stand vom 29.09. — vor der Überarbeitung der Kantenbremse, die in genau
dieser Fassung Bots an Kanten festhielt (8,7 Sekunden auf zehn Einheiten). Die
meisten Schalter der Karte „Bots" kannte dieses Modul noch gar nicht. Das Tool
schreibt die Cvars trotzdem; ein Modul, das sie nicht kennt, übergeht sie
stillschweigend.

**Gemessen in genau diesem Aufbau**: q3dm17, zehn Bots auf Stufe 5,
zielsuchende Raketen für alle, dieselbe `hitsoundlab.cfg`. Mit `g_botLog 3`
schreibt das Protokoll jedes Serverbild mit (`F`), dazu jeden Treffer (`D`);
`tools/botlog/jitter.pl` sucht darin Zittern, das schneller ist als ein
Denkschritt. Ergebnis für den neuen Bau: 0,03 Sekunden je Bot-Minute, beim
Original 0,01. Stehen ohne Grund: 0,09 Sekunden je Bot-Minute, gleich viel.

**Wer leichter ist, misst ein Stellvertreter.** Bot gegen Bot sagt wenig über
Mensch gegen Bots, also spielt ein starker Original-Bot (Xaero, Stufe 5) allein
gegen die zehn — seine Abschüsse je Tod zeigen, wie leicht das Feld ist
(`tools/botlog/proxy.pl`). Läufe von fünf Minuten; ein einzelner Lauf streut
um ±0,2, deshalb die Anzahl dazu:

| Feld | Abschüsse je Tod des Stellvertreters | Läufe |
|---|---|---|
| Original | 1,85 ± 0,06 | 12 |
| Menschlich, bis heute | 1,65 ± 0,05 | 21 |
| … ohne Raketen ausweichen | 1,98 | 6 |
| … ohne „kämpfen mit dem, was da ist" | 1,58 | 6 |
| … ohne „Entscheidung halten" | 1,52 | 6 |
| **Menschlich, neu** | **1,55 ± 0,05** | 27 |

Im gemischten Spiel (fünf gegen fünf, Rollen getauscht, `tools/botlog/dmg.pl`)
teilt das neue Menschlich 14 Prozent mehr Schaden aus, als es einsteckt, und
stirbt 53-mal durch die Karte statt 157-mal.

Das Ausweichen ist das Stärkste an „Menschlich" — ohne ist das Feld leichter
als das Original. Zwei Schalter dagegen machten die Bots leichter, weil sie in
Kämpfen blieben, die sie verlassen sollten: „kämpfen mit dem, was da ist"
(Kampf schon ab 40 Leben, mit einer einzigen Rakete) und das Halten einer
Entscheidung (ein Kampf galt, bis 30 Leben verloren waren). Das erste ist bei
„Menschlich" jetzt aus, das zweite hält einen Kampf nur noch, solange der Bot
dabei nicht getroffen wird, und einen Rückzug nur, bis er deutlich stärker
geworden ist. Das Hin und Her bleibt trotzdem halbiert (21 statt 35 je
Bot-Minute).

Nachgezählt auch die Sprungfreude: bei Stufe 5 liegt sie in den Charakterdateien
meist bei 1,0, „Menschlich" setzt 0,25. Mit den Werten der Charaktere war das
Feld leichter (1,85), nicht schwerer — ein springender Bot fliegt eine
Wurfparabel, und die ist leicht vorherzusagen. Die 0,25 bleiben.

## Sprungfelder, Pendel und Plattformen

Nachgemessen mit dem Bild-für-Bild-Protokoll, zwölf Bots, je drei Läufe von
vier Minuten. Drei Befunde, drei Änderungen.

**Die „Zusammenstöße" über den Sprungfeldern waren keine.** Auf q3tourney6
bricht bei 22 von 22 tödlichen Flügen mit plötzlichem Tempoverlust der Flug an
einem festen Hindernis ab, nie an einem anderen Bot — immer an denselben zwei
Stellen. Dort fahren die Rohre der Quetschfalle herunter, wenn jemand den Knopf
trifft. Zwei Sprungfelder hängen hintereinander, und erst das zweite wirft in
den Raum darunter (dort liegt die BFG).

Die Rechnung des Tritts (`BotFooting`) hielt jedes Sprungfeld für sicher — „es
trägt ja". Jetzt rechnet sie den Flug mit, den das Feld gibt (`s.origin2`, genau
das setzt `BG_TouchJumpPad`), durch alles, was gerade im Weg steht, und folgt
auch einem zweiten Feld im Flug. Führt der Flug in eine Todeszone oder in die
Tiefe, wartet der Bot davor, höchstens sechs Sekunden (die Falle bleibt fünf
unten). Beim Abschuss schreibt das Protokoll, was die Rechnung vorhergesagt hat
(`P`); verglichen mit dem, was dann geschah, stimmt sie.

Zwei Fehlalarme mussten dafür weg, beide zuerst im Protokoll aufgefallen:
- die Rechnung hielt andere Bots für Wände, dort wo sie gerade stehen — im
  echten Flug sind sie längst weg (sie sieht jetzt durch Spieler hindurch);
- „keine Landung" hieß oft nur, dass die Rechnung an einer Kante hängenblieb
  (sie rechnet eine Wand je Bild, das Spiel bis zu vier). Als Gefahr zählt es
  nur noch, wenn sie den Bot dabei tief fallen sieht. Und wer mit zehn Leben
  eine harte Landung nicht übersteht, wartet nicht: davon wird er nicht gesünder.

In der ersten Fassung standen die Bots auf q3dm17 deshalb je Lauf sieben
Sekunden vor Sprungfeldern. Jetzt: null.

**Pendel und schwebende Plattformen** (`func_pendulum`, `func_bobbing`) töten
sofort, was sie nicht wegschieben können. Auf q3dm15 starben zehn Bots unter
den drei Pendeln des Gangs, alle einfach durchgelaufen; auf q3dm19 standen sie
dort, wo botlib auf die Plattform wartet, halb unter deren Rand.
`BotMoverGuard` rechnet eine halbe Sekunde voraus, wo die Falle sein wird und
wo der Bot mit seinem Befehl ist — getestet mit dem wirklichen Modell der Falle
(`trap_EntityContact`), in deren Bezugssystem zurückgerechnet. Trifft es, wartet
er, geht zurück oder weg von der Falle. Wie ein Mensch, der den Takt abpasst.
Im Protokoll: `M`.

**Nach einer schnellen Landung** darf der Tritt jetzt auch auf dem eigenen Weg
gegen den Schwung halten, wenn der Bot schneller ist, als sein Befehl ihn
machen kann: auf q3tourney6 landeten Bots mit 700 u/s und rutschten über die
Kante.

| je drei Läufe | vorher | jetzt |
|---|---|---|
| q3tourney6, Tode durch die Karte | 321 | 221 |
| q3tourney6, davon zerquetscht | 97 | 81 |
| q3dm19, Tode durch die Karte | 93 | 67 |
| q3dm19, davon zerquetscht | 6 | 1 |
| q3dm15, davon zerquetscht | 12 | 2 |

Auf q3dm17 — deinem Aufbau, Stufe 5 — ändert sich an der Stärke nichts: der
Stellvertreter kommt gegen das alte Feld auf 1,52 Abschüsse je Tod (41 Läufe),
gegen das neue auf 1,53 (18 Läufe).

**Was bleibt.** Unter der Quetschfalle auf q3tourney6 liegen die BFG und zwei
Medipacks; wer dort ist, wenn jemand den Knopf trifft, stirbt — die Falle fährt
mit tausend Einheiten je Sekunde, dem entkommt niemand. Das ist die Falle der
Karte, beim Original genauso. Auf q3dm19 warten die Bots an den schwebenden
Plattformen etwas länger als vorher (Stehen 5,7 statt 4,4 Sekunden je Bot und
Minute, fast alles Warten auf die Plattform, wie im Original). Und der Anlauf
zu einem Sprung über eine Lücke bleibt botlib überlassen: dort den Absprung zu
verschieben, macht ihn schlechter.

## Ersetzte Sockel

Wer unter „Waffen auf der Karte" einengt, legt auf den Sockel der Schrotflinte
einen Raketenwerfer. botlib liest die Gegenstände einer Karte aber selbst aus
dem BSP und erkennt sie am Modell: der Eintrag der Schrotflinte blieb für
immer ohne Entität, und der Raketenwerfer galt als **fallengelassen** — zehn
statt dreißig Sekunden Sperre nach der Wahl, keine Umwegprüfung beim Nahziel,
und alle dreißig Sekunden vergessen und neu entdeckt. Auf q3dm17 mit „nur
Raketenwerfer" betraf das drei der fünf Waffensockel und sechs der acht
Munitionskisten.

`BotRelinkSocket` (in `code/botlib/be_ai_goal.c`) gibt dem Eintrag des Sockels
das, was wirklich daliegt — nur in den ersten drei Sekunden nach dem Laden der
Karte, denn später könnte es eine Waffe sein, die ein Toter genau neben einem
leeren Sockel fallen ließ. Nachgesehen im Protokoll: die neun Sockel tragen
jetzt ihre Kartennummer und die Flagge eines Kartengegenstands.

**Das ist eine Änderung an der Engine.** botlib steckt in `ioquake3.exe`,
nicht im Spielmodul. Das Spielmodul fragt beim Start, ob die Bibliothek das
Umwidmen kennt; bei einer älteren `.exe` nimmt es wie bisher den Aufschlag für
Fallengelassenes ganz weg und schreibt das in die Konsole.

## Die Trefferton von Quake 1 und Quake 2

Neu in der Auswahl: `cl_hitSound 3` und `4`. Eine Warnung vorweg, damit kein
falscher Eindruck entsteht: **Quake 1 und Quake 2 hatten überhaupt keinen
Trefferton.** Es gibt also nichts, was man hätte entnehmen können — was unter
diesen Namen kursiert, sind entweder andere Töne aus den Spielen, die Mods
zweckentfremdet haben, oder Nachbauten.

Deshalb zwei Wege, und beide funktionieren gleichzeitig:

**Mitgeliefert (nachgebaut).** `assets/hitsound-retro/` enthält zwei mit einem
Perl-Skript erzeugte Töne, die nur den Klangcharakter der jeweiligen Zeit
nachahmen — Quake 1 dunkel und körnig mit Tiefpass, Quake 2 heller und
metallisch mit einem Sprung nach oben. `build_mingw64.bat` packt sie zu
`zz-hitsound-retro.pk3`. Damit läuft die Auswahl auf jedem Rechner, auch ohne
die alten Spiele.

**Echt, aus den eigenen Spieldateien.** Wer Quake 1 und 2 besitzt, holt sich die
Töne heraus, die die Mods von damals benutzt haben:

| Spiel | Datei im pak | was es ist |
| --- | --- | --- |
| Quake 1 | `sound/misc/talk.wav` | der Nachrichten-Piep, der ikonische Blip |
| Quake 1 | `sound/weapons/tink1.wav` | der Querschläger, als Trefferton oft schöner |
| Quake 2 | `sound/misc/talk1.wav` | genau das, was Q2-Mods als Hitsound nahmen |

`tools/pak/pak.pl` liest das PAK-Format der beiden Spiele:

```bash
perl tools/pak/pak.pl "…/id1/pak0.pak" list 'sound/misc'
perl tools/pak/pak.pl "…/id1/pak0.pak" get sound/misc/talk.wav hit_q1.wav
```

Die Dateien dann als `sound/feedback/hit_q1.wav` und `hit_q2.wav` in ein pk3
packen, das **nach** `zz-hitsound-retro.pk3` sortiert — etwa
`zzz-hitsound-q12.pk3`. Die Engine sucht die später einsortierten zuerst, also
gewinnen die echten Töne, und der nächste Bau überschreibt sie nicht.

Die so entnommenen Dateien gehören ins eigene Spielverzeichnis, **nicht** in
dieses Repository.
