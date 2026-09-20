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
