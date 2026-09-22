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
