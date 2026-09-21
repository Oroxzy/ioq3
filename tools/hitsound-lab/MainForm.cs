using System.Diagnostics;
using System.Text;

namespace HitsoundLab;

// Bedienoberflaeche fuer die Trefferton-Tests: schreibt eine Config, startet das
// Spiel damit und liest waehrenddessen dessen Konsolenprotokoll mit. Kein Zugriff
// auf den laufenden Prozess.
public class MainForm : Form, IMessageFilter {
	const string CfgName = "hitsoundlab.cfg";
	const int WmKeyDown = 0x0100;
	const int WmSysKeyDown = 0x0104;
	const int WmLeftButtonDown = 0x0201;
	const int WmRightButtonDown = 0x0204;
	const int WmMiddleButtonDown = 0x0207;
	const int WmMouseWheel = 0x020A;
	const int WmXButtonDown = 0x020B;

	static readonly string[] Maps = {
		"q3dm1", "q3dm2", "q3dm3", "q3dm4", "q3dm5", "q3dm6", "q3dm7", "q3dm8",
		"q3dm9", "q3dm10", "q3dm11", "q3dm12", "q3dm13", "q3dm14", "q3dm15",
		"q3dm16", "q3dm17", "q3dm18", "q3dm19",
		"q3tourney1", "q3tourney2", "q3tourney3", "q3tourney4", "q3tourney5", "q3tourney6",
	};

	static readonly string[] BotNames = {
		"Sarge", "Grunt", "Major", "Visor", "Klesk", "Anarki", "Bones", "Doom",
		"Hunter", "Keel", "Orbb", "Patriot", "Ranger", "Razor", "Slash", "Sorlag",
		"TankJr", "Uriel", "Xaero", "Mynx",
	};

	static readonly string[] HitSounds = {
		"Original", "Quake Champions", "Eigene Datei",
	};

	readonly TextBox gameDir = new() { Width = 258 };
	// com_maxfps ist archiviert, das Spiel merkt es sich also dauerhaft -
	// deshalb schreibt "unveraendert" hier nichts.
	readonly ComboBox maxFps = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 130 };
	static readonly int[] MaxFpsChoices = { 0, 333, 250, 125, 60 };

	// Bildschirm und Aufloesung. "unveraendert" schreibt nichts - diese Werte
	// sind archiviert, das Spiel merkt sie sich also dauerhaft, und ein Testlauf
	// hat hier schon einmal die Einstellung des Benutzers ueberschrieben.
	readonly ComboBox screenMode = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 170 };
	readonly ComboBox screenSize = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 170 };

	// Ueber r_mode -1 mit eigener Breite/Hoehe, das deckt jede Aufloesung ab -
	// die eingebaute Modus-Tabelle kennt kein 16:9.
	static readonly (string Name, int W, int H)[] Resolutions = {
		( "unverändert", 0, 0 ),
		( "2560 × 1440", 2560, 1440 ),
		( "1920 × 1080", 1920, 1080 ),
		( "1600 × 900", 1600, 900 ),
		( "1280 × 720", 1280, 720 ),
		( "1024 × 768", 1024, 768 ),
		( "800 × 600", 800, 600 ),
		( "640 × 480", 640, 480 ),
	};
	readonly ComboBox map = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 120 };
	readonly NumericUpDown bots = new() { Minimum = 0, Maximum = 10, Value = 3, Width = 60 };
	readonly NumericUpDown skill = new() { Minimum = 1, Maximum = 5, Value = 3, Width = 60 };

	readonly ComboBox hitSound = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 160 };
	readonly TextBox hitSoundFile = new() { Width = 240, Text = "sound/feedback/hit_qc_100.wav" };
	readonly CheckBox hitPitch = new() { Text = "Tonhöhe folgt der Rest-HP", Checked = true, AutoSize = true };
	readonly NumericUpDown pitchFull = new() { DecimalPlaces = 2, Increment = 0.05m, Minimum = 0.5m, Maximum = 2.0m, Value = 1.20m, Width = 70 };
	readonly NumericUpDown pitchEmpty = new() { DecimalPlaces = 2, Increment = 0.05m, Minimum = 0.5m, Maximum = 2.0m, Value = 0.80m, Width = 70 };
	readonly NumericUpDown pitchKill = new() { DecimalPlaces = 2, Increment = 0.05m, Minimum = 0.5m, Maximum = 2.0m, Value = 0.70m, Width = 70 };
	readonly NumericUpDown pitchStack = new() { Minimum = 1, Maximum = 999, Value = 200, Width = 70 };

	readonly CheckBox aimAssist = new() { Text = "Zielhilfe", Checked = true, AutoSize = true };
	readonly NumericUpDown aimStrength = new() { Minimum = 1, Maximum = 10, Value = 8, Width = 60 };
	readonly CheckBox botOutline = new() { Text = "Bot-Markierung anzeigen (durch Wände)", Checked = true, AutoSize = true };
	readonly CheckBox botDamage = new() { Text = "mit Rest-HP (Balken oder Zahl)", Checked = true, AutoSize = true };
	// Wie ein Bot markiert wird und in welcher Farbe. Kontur ist der echte Umriss
	// am Modell, Silhouette die gefüllte Form durch Wände.
	readonly ComboBox botStyle = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 200 };
	readonly ComboBox botBars = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 200 };
	readonly TextBox botColor = new() { Width = 90, Text = "255 0 220" };
	readonly Button botColorPick = new() { Text = "wählen…", AutoSize = true };
	readonly CheckBox botName = new() { Text = "Name über dem Kopf", Checked = true, AutoSize = true };
	// Der Index ist der Wert von cl_damagePlums: 0 aus, 1 über dem Getroffenen,
	// 2 immer am Fadenkreuz.
	readonly ComboBox damagePlums = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 190 };
	readonly CheckBox noSelfDamage = new() { Text = "kein Schaden an mir selbst", Checked = false, AutoSize = true };
	// Der Index ist der Wert von g_infiniteAmmo: 0 aus, 1 nur ich, 2 alle.
	// Aufgefuellt wird auf 999 und nicht auf "unendlich", weil die Bots ihre
	// Waffenwahl an den Munitionszahlen festmachen - siehe G_TopUpAmmo.
	readonly ComboBox infiniteAmmo = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 170 };
	// Nachladezeiten in Prozent der normalen, nur fuer Menschen. Zehn Prozent
	// ist der Boden; darunter bliebe der Zielhilfe kein Bild mehr, auf dem sie
	// den Schuss kommen sieht.
	readonly TrackBar weaponRate = new() {
		Minimum = 10, Maximum = 200, Value = 100, TickFrequency = 10,
		SmallChange = 5, LargeChange = 25, Width = 190,
	};
	readonly Label weaponRateValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	// Wen die Hilfe nimmt, entscheidet die Vorrangliste; dieser Haken sagt nur,
	// dass zum Angreifer ohne Einschwenken gesprungen wird
	readonly CheckBox aimAttacker = new() { Text = "zum Angreifer springen statt weich schwenken", Checked = true, AutoSize = true };
	readonly CheckBox itemOutline = new() { Text = "Waffen und Powerups mit Respawn-Zeit", Checked = true, AutoSize = true };
	readonly CheckBox itemOutlineAll = new() { Text = "auch Rüstung und Mega", Checked = true, AutoSize = true };
	// Wie weit die Kästen und ihre Zähler zu sehen sind. Voll bis zur Hälfte
	// dieser Strecke, dann rasch weg, damit die Karte nicht zugestellt ist.
	readonly TrackBar itemRange = new() {
		Minimum = 0, Maximum = 4000, Value = 1500, TickFrequency = 500,
		SmallChange = 50, LargeChange = 250, Width = 190,
	};
	readonly Label itemRangeValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	readonly TextBox aimKey = new() {
		Text = "MOUSE4", Width = 110, ReadOnly = true,
		BackColor = SystemColors.Window, Cursor = Cursors.Hand,
	};
	readonly NumericUpDown aimSmooth = new() { Minimum = 0, Maximum = 300, Increment = 10, Value = 0, Width = 60 };
	readonly NumericUpDown aimLead = new() { DecimalPlaces = 1, Increment = 0.1m, Minimum = 0.1m, Maximum = 5.0m, Value = 1.5m, Width = 70 };
	// Der Index ist der Wert von cl_aimAssistExact: 0 nie, 1 nur die
	// Einzelschuss-Waffen, 2 alle. Der Haken davor konnte nur 0 und 1, und 1 las
	// sich als "exakt im Schussmoment", meinte aber: nicht fuer MG, Plasma und
	// Blitz - die gingen mit nur 0,32 des Wegs zum Punkt raus (Befund F03).
	readonly ComboBox aimExact = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 170 };
	// Solange die Hilfe wirklich fuehrt, wird die eigene Maus verworfen. Ohne
	// das hat die Hand auf jedem Befehl, der nicht der exakte Schussbefehl ist,
	// noch ihren Anteil am Zielen - gemessen sind das bei MG, Plasma und Blitz
	// im Mittel vier bis fuenf Einheiten neben dem Punkt, den die Hilfe wollte.
	readonly CheckBox aimFreeze = new() { Text = "Maus sperren, solange die Hilfe führt", Checked = false, AutoSize = true };
	// Ob der Sturz über eine Plattformkante auch wirklich angelegt wird. Aus
	// heisst: nur erkennen und ins Protokoll schreiben, Zielpunkt unverändert.
	readonly CheckBox aimEdge = new() { Text = "Sturz über die Kante anlegen (F26)", Checked = false, AutoSize = true };
	readonly CheckBox aimLearn = new() { Text = "je Waffe und Entfernung nachmessen", Checked = true, AutoSize = true };
	readonly Label aimLearned = new() { AutoSize = true, ForeColor = Color.DimGray, Margin = new Padding( 6, 4, 0, 0 ) };

	readonly Label statHits = Number();
	readonly Label statFrames = Number();
	readonly Label statSounds = Number();
	readonly Label statMissed = Number();
	readonly TextBox logView = new() { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Font = new Font( "Consolas", 9 ), Dock = DockStyle.Fill };

	readonly Label statShots = Number();
	readonly Label statShotHits = Number();
	readonly Label statShotMiss = Number();
	readonly Label statShotRate = Number();
	readonly Label statShotError = Number();
	readonly Label statShotRateOff = Number();
	readonly Label statHold = Number();
	readonly ToolTip holdTip = new();
	readonly ListView shotView = new SmoothListView() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ),
	};

	readonly Label statTuneBoxes = Number();
	readonly Label statTuneSamples = Number();
	readonly CheckBox aimHoldFire = new() { Text = "nicht schießen, solange der Schuss nicht durchkommt", AutoSize = true };
	readonly TrackBar holdLottery = new() {
		Minimum = 0, Maximum = 60, Value = 0, TickFrequency = 10,
		SmallChange = 1, LargeChange = 5, Width = 190,
	};
	readonly Label holdLotteryValue = new() {
		AutoSize = false, Width = 210, Height = 20, TextAlign = ContentAlignment.MiddleLeft,
		ForeColor = Color.DimGray,
	};
	readonly CheckBox autoSwitch = new() { Text = "Waffe wechseln, wenn die Munition leer ist", Checked = true, AutoSize = true };
	// Dieselbe Reihenfolge wie die Vorgabe von cl_autoSwitchEmptyOrder in
	// code/client/cl_main.c, aus dem Gemessenen: Railgun 87 Prozent,
	// Schrotflinte 78, Maschinengewehr 74, Rakete 45 - aber 85 auf kurze Sicht.
	// Schmal genug fuer die linke Spalte: bei 420 lief das Feld aus der Gruppe
	// heraus und der Text dahinter war abgeschnitten.
	readonly TextBox autoSwitchOrder = new() {
		Width = 330,
		Text = "railgun rocket lightning plasma shotgun machinegun grenade bfg gauntlet",
	};

	// Was ein Ziel zum besseren Ziel macht. Die Reihenfolge ist das Gewicht:
	// oben zaehlt am meisten. Schluessel wie in cl_aimAssistPriority.
	// Life > 0: eine Regel ueber etwas, das geschehen ist - die verfaellt.
	// Life = 0: eine Eigenschaft des Augenblicks, die keine Uhr braucht.
	static readonly (string Key, string Name, string Effect, double Life)[] Priorities = {
		// Die Sichtlinie ist ein Tor, keine Auswahl: wer sichtbar ist, bekommt
		// ihre vollen Punkte, also alle dasselbe. Ihr Gewicht entscheidet
		// nichts, nur ihre Dauer tut es - sie traegt ein Ziel ueber ein
		// kurzes Verschwinden hinweg.
		( "sight",    "freie Sichtlinie",      "Tor, kein Vorzug: gibt allen Sichtbaren gleich viel", 0.1 ),
		( "cursor",   "Nähe zum Fadenkreuz",   "wohin du ohnehin schon zielst", 0 ),
		( "attacker", "wer mich zuletzt traf", "sofort zurückschlagen, solange es frisch ist", 6 ),
		( "sure",     "Treffsicherheit",       "was die Waffe auf die Entfernung gemessen trifft", 0 ),
		( "near",     "Nähe im Raum",          "der nächste Gegner zuerst", 0 ),
		( "wounded",  "schon verwundet",       "wen ich selbst angeschlagen habe", 12 ),
		( "keep",     "Ziel behalten",         "nicht zwischen zweien hin und her springen", 4 ),
		( "powerup",  "trägt ein Powerup",     "Quad, Regeneration, Haste zuerst", 0 ),
		( "air",      "in der Luft",           "fliegt berechenbar - ein bloßer Sprung zählt nicht", 0 ),
	};
	static readonly int[] PriorityDefault = { 100, 80, 100, 60, 40, 40, 30, 20, 0 };

	readonly Dictionary<string, int> prioWeight = new();
	readonly Dictionary<string, double> prioTime = new();

	// Was eine einzelne Waffe anders haben will. Nur die Abweichungen stehen
	// hier, alles andere folgt der Standardliste - neun volle Listen waeren
	// neunmal so viel zu verstellen, und gemessen werden pro Runde nur ein
	// paar Dutzend Proben.
	// Womit die Karte bestueckt wird. Geschrieben wird eine Liste nach
	// g_weaponSpawns, und zwar vor den map-Befehl; das Spiel belegt daraufhin
	// jeden Waffensockel und jede Munitionskiste reihum mit den genannten
	// Waffen neu - siehe G_SubstituteSpawnItem in code/game/g_items.c. Die
	// Munition muss hier nicht stehen, sie wird dort aus der Waffe abgeleitet.
	//
	// Der erste Versuch ging ueber disable_<classname>, was Quake 3 von Haus
	// aus kann und keine Aenderung am Spiel gebraucht haette. Es taugt aber
	// nicht: das entfernt die anderen Waffen, statt sie zu ersetzen, und bei
	// "nur MG" steht die Karte leer. Wegnehmen ist nicht einengen. Deshalb
	// braucht diese Einstellung ein passendes qagame - das Spiel laedt es als
	// vm/qagame.qvm aus zz-hitpitch.pk3, nicht als qagame.dll.
	//
	// Wofuer das da ist: eine Messreihe ist nur vergleichbar, wenn in jedem Lauf
	// dasselbe geschossen wird. Die Latenzreihe vom 19.09. ist genau daran
	// gescheitert - 40 / 25 / 80 Prozent MG-Anteil, und die Waffenmischung hat
	// mehr bewegt als die Latenz, die gemessen werden sollte.
	static readonly (string Name, string Weapon)[] SpawnItems = {
		( "Maschinengewehr",  "machinegun" ),
		( "Schrotflinte",     "shotgun" ),
		( "Granatwerfer",     "grenadelauncher" ),
		( "Raketenwerfer",    "rocketlauncher" ),
		( "Blitzwerfer",      "lightning" ),
		( "Railgun",          "railgun" ),
		( "Plasmagun",        "plasmagun" ),
		( "BFG",              "bfg" ),
		( "Gauntlet",         "gauntlet" ),
	};
	readonly CheckedListBox spawnWeapons = new() {
		Width = 200, Height = 152, CheckOnClick = true, IntegralHeight = false,
	};
	readonly Label spawnValue = new() { AutoSize = true, ForeColor = Color.DimGray };

	static readonly (string Key, string Name)[] Weapons = {
		( "rocket",     "Raketenwerfer" ),
		( "grenade",    "Granatwerfer" ),
		( "plasma",     "Plasmagun" ),
		( "shotgun",    "Schrotflinte" ),
		( "lightning",  "Blitzwerfer" ),
		( "railgun",    "Railgun" ),
		( "machinegun", "Maschinengewehr" ),
		( "bfg",        "BFG" ),
		( "gauntlet",   "Gauntlet" ),
		// Der Enterhaken gehoert dazu, weil die Engine ihn kennt: eine von
		// Hand geschriebene Abweichung dafuer wuerde sonst beim ersten
		// Speichern stillschweigend verschwinden.
		( "hook",       "Enterhaken" ),
	};
	// Was die Engine von Haus aus je Waffe anders haelt, in der Reihenfolge von
	// Priorities: sight, cursor, attacker, sure, near, wounded, keep, powerup,
	// air. Muss zu aimWeaponDefault in code/client/cl_input.c passen - die
	// beiden sind schon einmal auseinandergelaufen, also gehoeren Aenderungen
	// daran in denselben Commit.
	//
	// Kurz, woher die Zahlen kommen: "Treffsicherheit" ist bei Hitscan-Waffen
	// wirkungslos, weil ohne Flugzeit nichts gemessen wird, also steht sie dort
	// auf null. "Nähe im Raum" folgt der gemessenen Entfernungskurve jeder
	// Waffe - Railgun flach über die ganze Karte, Blitzwerfer bei 768 Einheiten
	// zu Ende, Granate bei etwa 660. "Nähe zum Fadenkreuz" trennt geschnappte
	// Waffen von geführten: geführte zahlen für den Schwenk, geschnappte nicht.
	static readonly Dictionary<string, int[]> WeaponDefault = new() {
		["gauntlet"]   = new[] { 100, 70, 60,  0, 100, 20, 25,  0,  0 },
		["machinegun"] = new[] { 100, 80,100,  0,  55, 35, 35, 15,  0 },
		["shotgun"]    = new[] { 100, 85, 90,  0,  75, 25, 20, 15,  0 },
		["grenade"]    = new[] { 100, 60,  0, 85, 100,  0, 30,  0,  0 },
		["rocket"]     = new[] { 100, 70,  0, 95,  80,  0, 30,  0, 35 },
		["lightning"]  = new[] { 100, 55, 85,  0, 100, 30, 45, 10,  0 },
		["railgun"]    = new[] { 100, 95, 80,  0,  10, 45, 15, 35,  0 },
		["plasma"]     = new[] { 100, 85, 90, 45,  80, 30, 35, 10,  0 },
		["bfg"]        = new[] { 100, 75,  0, 90,  45,  0, 30,  0, 25 },
	};
	// Nur zwei Waffen halten ihr Ziel kuerzer fest: die langsamen Einzelschuss-
	// waffen, deren Takt anderthalb Sekunden ist.
	static readonly Dictionary<string, double> WeaponKeepLife = new() {
		["gauntlet"] = 2, ["shotgun"] = 2, ["railgun"] = 2,
	};
	readonly Dictionary<string, Dictionary<string, int>> weaponWeight = new();
	readonly Dictionary<string, Dictionary<string, double>> weaponTime = new();
	readonly ComboBox prioWeapon = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 215 };
	readonly Button prioReset = new() { Text = "wie Standard", Width = 110 };
	// Eine cvar fasst 256 Zeichen. Laeuft die Liste der Abweichungen darueber,
	// schneidet die Engine sie stillschweigend ab - also muss man es sehen.
	readonly Label prioWarn = new() { AutoSize = true, ForeColor = Color.Firebrick };

	// null = die Standardliste, sonst der Schluessel der gewaehlten Waffe
	string? CurWeapon => prioWeapon.SelectedIndex <= 0 ? null
		: Weapons[prioWeapon.SelectedIndex - 1].Key;

	int Weight( string key ) {
		var w = CurWeapon;
		return w is not null && weaponWeight.TryGetValue( w, out var over )
			&& over.TryGetValue( key, out int v ) ? v : prioWeight[key];
	}

	double Life( string key ) {
		var w = CurWeapon;
		return w is not null && weaponTime.TryGetValue( w, out var over )
			&& over.TryGetValue( key, out double v ) ? v : prioTime[key];
	}

	bool Differs( string key ) {
		var w = CurWeapon;
		if ( w is null ) return false;
		return ( weaponWeight.TryGetValue( w, out var a ) && a.ContainsKey( key ) )
			|| ( weaponTime.TryGetValue( w, out var b ) && b.ContainsKey( key ) );
	}

	// Ein Wert, der dem Standard gleicht, ist keine Abweichung und darf auch
	// keine werden. Sonst legt schon ein Verschieben der Reihenfolge beim
	// Nachbarn einen Eintrag an, der nichts aendert, aber als abweichend
	// angezeigt und in die Zeichenkette geschrieben wird.
	void SetWeight( string key, int value ) {
		var w = CurWeapon;
		if ( w is null ) { prioWeight[key] = value; return; }
		if ( value == prioWeight[key] ) {
			if ( weaponWeight.TryGetValue( w, out var had ) ) {
				had.Remove( key );
				if ( had.Count == 0 ) weaponWeight.Remove( w );
			}
			return;
		}
		if ( !weaponWeight.TryGetValue( w, out var over ) ) weaponWeight[w] = over = new();
		over[key] = value;
	}

	void SetLife( string key, double value ) {
		var w = CurWeapon;
		if ( w is null ) { prioTime[key] = value; return; }
		if ( value == prioTime[key] ) {
			if ( weaponTime.TryGetValue( w, out var had ) ) {
				had.Remove( key );
				if ( had.Count == 0 ) weaponTime.Remove( w );
			}
			return;
		}
		if ( !weaponTime.TryGetValue( w, out var over ) ) weaponTime[w] = over = new();
		over[key] = value;
	}
	readonly ListView prioView = new SmoothListView() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true, CheckBoxes = true,
		GridLines = true, HideSelection = false, Font = new Font( "Segoe UI", 9 ),
	};
	readonly Button prioUp = new() { Text = "▲ höher", Width = 90 };
	readonly Button prioDown = new() { Text = "▼ tiefer", Width = 90 };
	readonly TrackBar prioBar = new() { Minimum = 0, Maximum = 100, TickFrequency = 10, Width = 200 };
	// Feste Breiten: sonst wandert die halbe Zeile mit, sobald sich die Zahl
	// von 9 auf 100 aendert oder aus "dauerhaft" "12.0 s" wird.
	readonly Label prioValue = new() {
		AutoSize = false, Width = 38, Height = 20, TextAlign = ContentAlignment.MiddleRight,
		Font = new Font( "Segoe UI", 9, FontStyle.Bold ),
	};
	// in Zehntelsekunden, damit auch die Nachwirkung der Sichtlinie einstellbar
	// ist - die liegt bei Bruchteilen einer Sekunde, nicht bei ganzen. Die
	// Reichweite bleibt dieselbe wie zuvor in ganzen Sekunden, sonst wuerde
	// ein geladener Wert darueber beim ersten Anfassen stillschweigend gekappt.
	readonly TrackBar prioLifeBar = new() { Minimum = 0, Maximum = 600, TickFrequency = 100, Width = 160 };
	readonly Label prioLifeValue = new() {
		AutoSize = false, Width = 118, Height = 20, TextAlign = ContentAlignment.MiddleLeft,
		ForeColor = Color.DimGray,
	};
	bool prioUpdating;
	bool prioClicked;			// ob der letzte Hakenwechsel von einem Klick kam
	// Beim Ziehen am Fenster feuert Resize hunderte Male. Jede Anpassung mass
	// bisher jede Ueberschrift neu und setzte jede Spaltenbreite einzeln, und
	// jede gesetzte Breite zeichnet die ganze Liste neu - mal sechs Listen, auch
	// die auf geschlossenen Karten. Also wird gesammelt und erst gerechnet, wenn
	// das Ziehen steht.
	readonly HashSet<ListView> fitPending = new();
	readonly System.Windows.Forms.Timer fitTimer = new() { Interval = 80 };
	bool fitBandsPending;
	// Die schmalste sinnvolle Breite je Spalte. Ueberschrift und Schrift aendern
	// sich nie, also wird einmal gemessen statt bei jedem Resize.
	readonly Dictionary<ColumnHeader, int> columnLeast = new();

	SplitContainer? splitMain;
	TabControl? settingsTabs;	// die Karten links; welche offen war, wird gemerkt
	int settingsTabSaved;		// zuletzt offene Karte, wird nach dem Aufbau gesetzt
	// Die Saetze, die frueher als graue Zeilen unter den Schaltern standen. Sie
	// werden einmal gelesen und nahmen dann dauerhaft ein Fuenftel der Hoehe weg.
	readonly ToolTip hintTip = new() { AutoPopDelay = 20000, InitialDelay = 400, ReshowDelay = 100 };
	Size windowSize;			// was zuletzt gespeichert wurde, leer beim ersten Start
	int splitterSaved;			// wo der Teiler stand, 0 wenn nie gespeichert

	readonly Label statBestWeapon = Number();
	readonly ListView rankView = new SmoothListView() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ), OwnerDraw = true,
	};
	readonly ListView tuneView = new SmoothListView() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ),
	};

	readonly Label statFixes = Number();
	readonly Label statFixUp = Number();
	readonly Label statFixDown = Number();
	readonly Label statFixFlat = Number();
	readonly Label histLast = new() {
		AutoSize = true, Font = new Font( "Segoe UI", 11, FontStyle.Bold ),
		Margin = new Padding( 4, 2, 4, 6 ),
	};
	readonly ComboBox histWeapon = new() {
		DropDownStyle = ComboBoxStyle.DropDownList, Width = 150,
		Margin = new Padding( 0, 8, 14, 0 ),
	};
	readonly Label histEmpty = new() {
		Dock = DockStyle.Fill, ForeColor = Color.DimGray, TextAlign = ContentAlignment.MiddleCenter,
		Visible = false,
	};
	readonly ListView histView = new SmoothListView() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ), OwnerDraw = true,
	};

	// Zurücksetzen ist selten und verwirft Gemessenes: darum klein und neben
	// den Zahlen, die es betrifft, nicht als Hauptknopf der Karteikarte.
	static Button ResetButton() => new() {
		Text = "zurücksetzen", Width = 110, Height = 26,
		Margin = new Padding( 18, 9, 0, 0 ), FlatStyle = FlatStyle.System,
	};
	readonly Button tuneReset = ResetButton();
	readonly Button rateReset = ResetButton();
	readonly ToolTip resetTip = new();

	readonly Label statBestRange = Number();
	readonly ListView rateView = new SmoothListView() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ), OwnerDraw = true,
	};
	readonly ToolTip rateTip = new() { AutoPopDelay = 20000, InitialDelay = 300 };

	// Die Protokollfassung, die dieses Werkzeug versteht. Schreibt das Spiel
	// eine andere, passen Zeilen und Auswertung nicht mehr sicher zusammen -
	// und dann soll das dastehen statt still falsch gerechnet zu werden.
	const int LogVersion = 14;
	readonly Label logVersion = new() { AutoSize = true, ForeColor = Color.DimGray };

	readonly Button start = new() { Text = "Spiel starten", Width = 140, Height = 34 };
	readonly Button save = new() { Text = "Speichern", Width = 100, Height = 34 };
	readonly Label status = new() { AutoSize = true, ForeColor = Color.DimGray };

	readonly System.Windows.Forms.Timer poll = new() { Interval = 500 };
	string logPath = "";
	string aimKeyBeforeCapture = "MOUSE4";
	bool capturingAimKey;
	string shotStamp = "";
	string rankStamp = "";
	string tuneStamp = "";
	Process? game;					// das von hier gestartete Spiel, solange es laeuft

	// Wann dieses Programm gebaut wurde, im Titel. Das Erstelldatum einer Datei
	// bleibt beim Ueberschreiben stehen, und wer an zwei Rechnern arbeitet,
	// sieht dem Ordner nicht an, welcher Stand darin liegt - der Titel schon.
	static string BuiltWhen() {
		try {
			var path = System.Reflection.Assembly.GetExecutingAssembly().Location;
			if ( path.Length > 0 && File.Exists( path ) ) {
				return File.GetLastWriteTime( path ).ToString( "dd.MM.yyyy HH:mm" );
			}
		} catch ( Exception ) {
		}
		return "unbekannt";
	}

	public MainForm() {
		Text = "Trefferton-Labor – Bau vom " + BuiltWhen();
		ClientSize = new Size( 1260, 820 );
		MinimumSize = new Size( 820, 560 );
		Font = new Font( "Segoe UI", 9 );

		gameDir.Text = FindGameDir();
		map.Items.AddRange( Maps );
		map.SelectedIndex = 0;
		hitSound.Items.AddRange( HitSounds );
		hitSound.SelectedIndex = 1;

		maxFps.Items.AddRange( new object[] { "unverändert", "333", "250", "125", "60" } );
		maxFps.SelectedIndex = 0;
		hintTip.SetToolTip( maxFps, "Wird beim Start als com_maxfps gesetzt. „unverändert“ fasst die Einstellung des Spiels nicht an." );
		screenMode.Items.AddRange( new object[] { "unverändert", "Fenster", "Vollbild" } );
		screenMode.SelectedIndex = 0;
		foreach ( var r in Resolutions ) screenSize.Items.Add( r.Name );
		screenSize.SelectedIndex = 0;
		hintTip.SetToolTip( screenMode, "Wird beim Start als +set an das Spiel übergeben, nicht über die"
			+ " Config – sonst greift es erst nach einem vid_restart. „unverändert“ fasst"
			+ " die Einstellung des Spiels nicht an." );
		hintTip.SetToolTip( screenSize, "Setzt r_mode -1 mit eigener Breite und Höhe, damit auch 16:9"
			+ " möglich ist. „unverändert“ lässt alles, wie es im Spiel steht." );

		foreach ( var s in SpawnItems ) spawnWeapons.Items.Add( s.Name, true );
		damagePlums.Items.AddRange( new object[] { "aus", "über dem Getroffenen", "immer am Fadenkreuz" } );
		damagePlums.SelectedIndex = 1;
		aimExact.Items.AddRange( new object[] { "nie", "nur Einzelschuss-Waffen", "alle Waffen" } );
		aimExact.SelectedIndex = 2;
		hintTip.SetToolTip( aimExact, "Ob die Sicht auf dem Feuerbefehl genau auf den vorhergesagten Punkt gesetzt"
			+ " wird. „nur Einzelschuss-Waffen“ = Shotgun, Granate, Rakete, Rail, BFG; MG, Plasma und Blitz"
			+ " folgten dann nur mit 0,32 des Wegs je Befehl – gemessen kostete das 1,5–3 Punkte MG-Trefferquote"
			+ " (72 % der MG-Schüsse). „nie“ ist für Vergleichsmessungen da." );
		botStyle.Items.AddRange( new object[] { "Drahtbox", "Silhouette (gefüllt)", "Kontur (Umriss)", "Kontur + Silhouette" } );
		botStyle.SelectedIndex = 2;
		botBars.Items.AddRange( new object[] { "Zahl", "HP-Balken", "HP + Rüstung" } );
		botBars.SelectedIndex = 2;
		botColorPick.Click += ( _, _ ) => PickBotColor();

		hitSound.SelectedIndexChanged += ( _, _ ) => hitSoundFile.Enabled = hitSound.SelectedIndex == 2;
		hitSoundFile.Enabled = false;
		aimKey.Click += ( _, _ ) => BeginAimKeyCapture();
		aimAssist.CheckedChanged += ( _, _ ) => UpdateAimEnabled();
		aimLearn.CheckedChanged += ( _, _ ) => UpdateAimEnabled();
		itemOutline.CheckedChanged += ( _, _ ) => UpdateItemEnabled();
		itemRange.ValueChanged += ( _, _ ) => ShowItemRange();
		weaponRate.ValueChanged += ( _, _ ) => ShowWeaponRate();
		infiniteAmmo.Items.AddRange( new object[] { "wie im Spiel", "unbegrenzt für mich", "unbegrenzt für alle" } );
		infiniteAmmo.SelectedIndex = 0;
		infiniteAmmo.SelectedIndexChanged += ( _, _ ) => UpdateSwitchEnabled();
		autoSwitch.CheckedChanged += ( _, _ ) => UpdateSwitchEnabled();
		holdLottery.ValueChanged += ( _, _ ) => ShowHoldLottery();
		ShowHoldLottery();
		// die Folge-Felder auf den Standard-Hakenstand bringen
		UpdateItemEnabled();
		ShowItemRange();
		ShowWeaponRate();
		UpdateSwitchEnabled();
		UpdateAimEnabled();

		// das eigene Icon der App, auch in der Titelleiste und der Taskleiste
		try {
			Icon = Icon.ExtractAssociatedIcon( Application.ExecutablePath );
		} catch {
		}
		shotView.Columns.Add( "Frame", 70 );
		shotView.Columns.Add( "Waffe", 90 );
		shotView.Columns.Add( "Ziel", 90 );
		shotView.Columns.Add( "Entfernung", 80, HorizontalAlignment.Right );
		// "ja, landet in N ms" trennt die Schuesse haerter als alles andere:
		// eine Rakete, deren Ziel unterwegs aufsetzt, trifft halb so oft
		shotView.Columns.Add( "in der Luft", 110 );
		shotView.Columns.Add( "Vorhalt", 70, HorizontalAlignment.Right );
		shotView.Columns.Add( "Fehler", 70, HorizontalAlignment.Right );
		// Bei geschnappten Waffen ist der Fehler bauartbedingt null, weil die
		// Sicht genau auf den Punkt gesetzt wird. Erst der Schwenk daneben
		// sagt, wie weit die Hilfe dafuer arbeiten musste.
		shotView.Columns.Add( "Schwenk", 70, HorizontalAlignment.Right );
		shotView.Columns.Add( "Tempo", 70, HorizontalAlignment.Right );
		shotView.Columns.Add( "Zielpunkt", 150 );
		shotView.Columns.Add( "Ergebnis", 80 );
		shotView.Columns.Add( "Hilfe", 60 );
		shotView.Columns.Add( "Fehlweite", 70, HorizontalAlignment.Right );
		shotView.Columns.Add( "Richtung", 110 );

		tuneView.Columns.Add( "Waffe", 110 );
		tuneView.Columns.Add( "Flugzeit ab", 90, HorizontalAlignment.Right );
		tuneView.Columns.Add( "Ziel-Tempo ab", 100, HorizontalAlignment.Right );
		tuneView.Columns.Add( "Proben", 70, HorizontalAlignment.Right );
		tuneView.Columns.Add( "Vorhalt-Faktor", 100, HorizontalAlignment.Right );
		tuneView.Columns.Add( "Streuung", 80, HorizontalAlignment.Right );
		tuneView.Columns.Add( "Wirkradius", 80, HorizontalAlignment.Right );
		tuneView.Columns.Add( "Aussicht", 110 );

		// Die Trefferquote ist eine Matrix, keine Liste: die Waffe nach unten,
		// die Entfernung nach rechts. So steht die Frage gezeichnet da, statt
		// in Prosa beantwortet zu werden.
		histView.Columns.Add( "#", 50, HorizontalAlignment.Right );
		histView.Columns.Add( "Zeit", 85, HorizontalAlignment.Right );
		histView.Columns.Add( "Waffe", 90 );
		histView.Columns.Add( "Topf", 150 );
		histView.Columns.Add( "Ziel", 80 );
		histView.Columns.Add( "erwartet", 75, HorizontalAlignment.Right );
		histView.Columns.Add( "gelaufen", 75, HorizontalAlignment.Right );
		histView.Columns.Add( "daneben", 75, HorizontalAlignment.Right );
		histView.Columns.Add( "Fach", 110, HorizontalAlignment.Right );
		histView.Columns.Add( "Änderung", 150 );
		histView.Columns.Add( "warum", 190 );

		// Die Antwort steht vorne: die Spalte, die die Frage beantwortet, soll
		// nicht die sein, die beim Schmalerziehen als erste leidet. Eine
		// Spalte "Proben" gibt es nicht - jedes Fach nennt seine Zahl selbst.
		rateView.Columns.Add( "Waffe", 170 );
		rateView.Columns.Add( "am besten", 110 );
		foreach ( var band in RangeNames ) {
			rateView.Columns.Add( band, 118, HorizontalAlignment.Center );
		}
		// Eigene Aufteilung statt FitColumns: in einer Matrix muessen die
		// Faecher gleich breit sein, sonst liest sich ein breiteres Fach wie
		// ein wichtigeres. Die beiden vorderen Spalten behalten ihr Mass, der
		// Rest wird zu gleichen Teilen auf die fuenf Entfernungen verteilt.
		rateView.Resize += ( _, _ ) => QueueBands();
		rateView.VisibleChanged += ( _, _ ) => { if ( rateView.Visible ) QueueBands(); };

		rankView.Columns.Add( "Waffe", 120 );
		rankView.Columns.Add( "Schüsse", 70, HorizontalAlignment.Right );
		rankView.Columns.Add( "Treffer", 70, HorizontalAlignment.Right );
		rankView.Columns.Add( "Quote", 260 );
		rankView.Columns.Add( "Fehlweite ø", 90, HorizontalAlignment.Right );

		// Eine schmale erste Spalte nur fuer das Zeichen, dass diese Zeile von
		// der Standardliste abweicht - so stehen die Namen darunter buendig,
		// statt um zwei Zeichen zu verrutschen.
		prioView.Columns.Add( "", 38, HorizontalAlignment.Center );
		prioView.Columns.Add( "Kriterium", 180 );
		prioView.Columns.Add( "Gewicht", 60, HorizontalAlignment.Right );
		prioView.Columns.Add( "", 96 );
		prioView.Columns.Add( "gilt", 60, HorizontalAlignment.Right );
		prioView.Columns.Add( "was es bewirkt", 380 );
		// Die Zeilenhoehe setzt jetzt SmoothListView fuer alle Listen gleich
		for ( int i = 0; i < Priorities.Length; i++ ) {
			prioWeight[Priorities[i].Key] = PriorityDefault[i];
			prioTime[Priorities[i].Key] = Priorities[i].Life;
		}
		// Dieselben Abweichungen, die die Engine von sich aus mitbringt. Ohne
		// sie wuerde ein Speichern ohne jede Aenderung eine leere Datei
		// schreiben und damit genau die Vorgaben loeschen, die gemessen wurden.
		// Nur was vom Standard abweicht wird gemerkt, sonst stuende in der
		// Liste ueberall ein Pfeil, der nichts bedeutet.
		foreach ( var w in Weapons ) {
			if ( !WeaponDefault.TryGetValue( w.Key, out var row ) ) continue;
			for ( int i = 0; i < Priorities.Length && i < row.Length; i++ ) {
				if ( row[i] == prioWeight[Priorities[i].Key] ) continue;
				if ( !weaponWeight.TryGetValue( w.Key, out var over ) ) weaponWeight[w.Key] = over = new();
				over[Priorities[i].Key] = row[i];
			}
			if ( WeaponKeepLife.TryGetValue( w.Key, out double life ) ) {
				weaponTime[w.Key] = new Dictionary<string, double> { ["keep"] = life };
			}
		}
		prioUp.Click += ( _, _ ) => MovePriority( -1 );
		prioDown.Click += ( _, _ ) => MovePriority( 1 );
		prioView.SelectedIndexChanged += ( _, _ ) => ShowPrioritySelection();
		// Beim Anlegen der Zeilen meldet Windows jeden Haken erst als aus und
		// dann als an. Das ist kein Klick, und ohne diese Unterscheidung
		// schreibt sich die ganze Liste beim ersten Anzeigen selbst um.
		prioView.MouseDown += ( _, _ ) => prioClicked = true;
		prioView.KeyDown += ( _, e ) => { if ( e.KeyCode == Keys.Space ) prioClicked = true; };
		prioView.ItemChecked += ( _, e ) => {
			if ( prioUpdating || !prioClicked || e.Item?.Tag is not string key ) return;
			prioClicked = false;
			// abgehakt heisst Gewicht null; beim Wiedereinschalten kommt ein
			// brauchbarer Wert zurueck, sonst bliebe die Zeile wirkungslos
			if ( !e.Item.Checked ) SetWeight( key, 0 );
			else if ( Weight( key ) == 0 ) SetWeight( key, 50 );
			BeginInvoke( () => FillPriorities( key ) );
		};
		prioBar.ValueChanged += ( _, _ ) => {
			if ( prioUpdating || prioView.SelectedItems.Count == 0 ) return;
			SetWeight( (string)prioView.SelectedItems[0].Tag!, prioBar.Value );
			FillPriorities( (string)prioView.SelectedItems[0].Tag! );
		};
		prioLifeBar.ValueChanged += ( _, _ ) => {
			if ( prioUpdating || prioView.SelectedItems.Count == 0 ) return;
			var key = (string)prioView.SelectedItems[0].Tag!;
			if ( !IsTimed( key ) ) return;
			SetLife( key, prioLifeBar.Value / 10.0 );
			FillPriorities( key );
		};

		// Mit der Tastatur: Plus und Minus verstellen das Gewicht der
		// gewaehlten Zeile um fuenf, Bild-auf und Bild-ab schieben sie in der
		// Reihenfolge. Fuer eine Feinabstimmung ist das schneller, als fuer
		// jede Zahl zum Schieber zu greifen.
		prioView.KeyDown += ( _, e ) => {
			if ( prioView.SelectedItems.Count == 0 ) return;
			var key = (string)prioView.SelectedItems[0].Tag!;
			int step = e.KeyCode switch {
				Keys.Add or Keys.Oemplus => 5,
				Keys.Subtract or Keys.OemMinus => -5,
				_ => 0,
			};
			if ( step != 0 ) {
				SetWeight( key, Math.Clamp( Weight( key ) + step, 0, 100 ) );
				FillPriorities( key );
				e.Handled = e.SuppressKeyPress = true;
				return;
			}
			if ( e.KeyCode == Keys.PageUp ) { MovePriority( -1 ); e.Handled = e.SuppressKeyPress = true; }
			else if ( e.KeyCode == Keys.PageDown ) { MovePriority( 1 ); e.Handled = e.SuppressKeyPress = true; }
		};

		prioWeapon.Items.Add( "Standard (alle Waffen)" );
		foreach ( var w in Weapons ) prioWeapon.Items.Add( w.Name );
		prioWeapon.SelectedIndex = 0;
		prioWeapon.SelectedIndexChanged += ( _, _ ) => FillPriorities();
		// Zurueck auf das, was diese Waffe von Haus aus will - nicht auf die
		// Standardliste. Sonst faellt die Rakete beim Zuruecksetzen auf eine
		// Nähe von 40, die fuer sie nie gemeint war.
		prioReset.Click += ( _, _ ) => {
			var w = CurWeapon;
			if ( w is null ) return;
			weaponWeight.Remove( w );
			weaponTime.Remove( w );
			if ( WeaponDefault.TryGetValue( w, out var row ) ) {
				for ( int i = 0; i < Priorities.Length && i < row.Length; i++ ) {
					if ( row[i] == prioWeight[Priorities[i].Key] ) continue;
					if ( !weaponWeight.TryGetValue( w, out var over ) ) weaponWeight[w] = over = new();
					over[Priorities[i].Key] = row[i];
				}
			}
			if ( WeaponKeepLife.TryGetValue( w, out double life ) ) {
				weaponTime[w] = new Dictionary<string, double> { ["keep"] = life };
			}
			FillPriorities();
		};
		// Die Quote bekommt einen Balken statt einer Zahl, der Rest bleibt Text
		rankView.DrawColumnHeader += ( _, e ) => e.DrawDefault = true;
		rankView.DrawItem += ( _, _ ) => { };
		rankView.DrawSubItem += ( _, e ) => {
			if ( e.ColumnIndex != RankBarColumn ) { e.DrawDefault = true; return; }
			e.DrawBackground();

			double share = e.Item?.Tag is double d ? d : 0;
			var bar = e.Bounds;
			bar.Inflate( -3, -3 );
			using ( var back = new SolidBrush( Color.FromArgb( 232, 232, 232 ) ) ) {
				e.Graphics.FillRectangle( back, bar );
			}
			using ( var fill = new SolidBrush( RateColour( share ) ) ) {
				e.Graphics.FillRectangle( fill, bar.X, bar.Y,
					(int)( bar.Width * Math.Clamp( share, 0, 1 ) ), bar.Height );
			}
			TextRenderer.DrawText( e.Graphics, e.SubItem?.Text ?? "", rankView.Font, bar,
				Color.Black, TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter );
		};

		// Dieselbe Zeichnung, nur fuenfmal je Zeile und mit drei Zustaenden:
		// ein Fach, das noch nichts sagen darf, sieht anders aus als eines,
		// das eine Zahl nennt, und beide anders als eines, das vergleichbar
		// ist. Ohne das liest sich ein Fach mit vier Proben wie ein Urteil.
		rateView.DrawColumnHeader += ( _, e ) => e.DrawDefault = true;
		rateView.DrawItem += ( _, _ ) => { };
		rateView.DrawSubItem += ( _, e ) => {
			int band = e.ColumnIndex - RateFirstBand;
			if ( band < 0 || band >= RangeNames.Length || e.Item?.Tag is not Cell[] cells ) {
				e.DrawDefault = true;
				return;
			}
			e.DrawBackground();

			var cell = cells[band];
			var bar = e.Bounds;
			bar.Inflate( -3, -3 );
			using ( var back = new SolidBrush( Color.FromArgb( 232, 232, 232 ) ) ) {
				e.Graphics.FillRectangle( back, bar );
			}

			int width = (int)( bar.Width * Math.Clamp( cell.Share, 0, 1 ) );
			if ( cell.Samples >= RateFirm ) {
				using var fill = new SolidBrush( RateColour( cell.Share ) );
				e.Graphics.FillRectangle( fill, bar.X, bar.Y, width, bar.Height );
			} else if ( cell.Samples >= RateSpeak ) {
				// Schraffiert heisst: das ist die Zahl, aber verlass dich nicht
				// darauf. Unter fuenfundzwanzig Proben ist das Wilson-Intervall
				// breiter als der Abstand zweier Nachbarfaecher.
				using var fill = new System.Drawing.Drawing2D.HatchBrush(
					System.Drawing.Drawing2D.HatchStyle.Percent50,
					RateColour( cell.Share ), Color.FromArgb( 232, 232, 232 ) );
				e.Graphics.FillRectangle( fill, bar.X, bar.Y, width, bar.Height );
			} else if ( cell.Samples > 0 ) {
				using var edge = new Pen( Color.DimGray );
				e.Graphics.DrawRectangle( edge, bar.X, bar.Y, bar.Width - 1, bar.Height - 1 );
			}

			// Der zweite, duenne Balken am unteren Rand: dieselbe Quote, aber
			// nur fuer die Schuesse, deren Ziel im Flug aufsetzen sollte. Fuer
			// Hitscan gibt es ihn nie, weil es dort keine Flugzeit gibt.
			if ( cell.LandWeight >= RateSpeak ) {
				using var fill = new SolidBrush( RateColour( cell.LandShare ) );
				e.Graphics.FillRectangle( fill, bar.X, bar.Bottom - 4,
					(int)( bar.Width * Math.Clamp( cell.LandShare, 0, 1 ) ), 4 );
			}

			TextRenderer.DrawText( e.Graphics, e.SubItem?.Text ?? "", rateView.Font, bar,
				cell.Samples >= RateSpeak ? Color.Black : Color.DimGray,
				TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter );
		};
		rateView.MouseMove += ( _, e ) => ShowRateTip( e.Location );

		// Der Balken der Korrekturen sitzt anders als die beiden anderen: er
		// waechst aus der Mitte nach beiden Seiten, weil eine Korrektur eine
		// Richtung hat und kein Urteil. Blau hinauf, orange hinunter - keine
		// Ampel, denn "staerker" ist nicht besser als "schwaecher".
		histView.DrawColumnHeader += ( _, e ) => e.DrawDefault = true;
		histView.DrawItem += ( _, _ ) => { };
		histView.DrawSubItem += ( _, e ) => {
			if ( e.ColumnIndex != HistBarColumn || e.Item?.Tag is not double moved ) {
				e.DrawDefault = true;
				return;
			}
			e.DrawBackground();

			var bar = e.Bounds;
			bar.Inflate( -3, -3 );
			using ( var back = new SolidBrush( Color.FromArgb( 232, 232, 232 ) ) ) {
				e.Graphics.FillRectangle( back, bar );
			}
			int middle = bar.X + bar.Width / 2;
			using ( var tick = new Pen( Color.FromArgb( 200, 200, 200 ) ) ) {
				e.Graphics.DrawLine( tick, middle, bar.Y, middle, bar.Bottom - 1 );
			}

			// Voller Ausschlag ist eine halbe Zehntelstelle. Gemessen liegt die
			// Haelfte aller Korrekturen unter 0,01 und ein Zwanzigstel darueber -
			// wer anschlaegt, bekommt eine Kerbe an der Spitze statt stiller
			// Kappung.
			double full = 0.05;
			int half = bar.Width / 2 - 1;
			int width = (int)( half * Math.Min( Math.Abs( moved ) / full, 1.0 ) );
			if ( Math.Abs( moved ) >= 0.005 ) {
				var colour = moved > 0 ? Color.FromArgb( 120, 160, 215 ) : Color.FromArgb( 215, 150, 110 );
				using var fill = new SolidBrush( colour );
				e.Graphics.FillRectangle( fill, moved > 0 ? middle : middle - width,
					bar.Y, width, bar.Height );
				if ( Math.Abs( moved ) > full ) {
					using var edge = new SolidBrush( Color.FromArgb( 90, 90, 90 ) );
					e.Graphics.FillRectangle( edge, moved > 0 ? middle + width - 3 : middle - width,
						bar.Y, 3, bar.Height );
				}
			}

			TextRenderer.DrawText( e.Graphics, e.SubItem?.Text ?? "", histView.Font, bar,
				Math.Abs( moved ) < 0.005 ? Color.DimGray : Color.Black,
				TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter );
		};
		histWeapon.SelectedIndexChanged += ( _, _ ) => {
			if ( histUpdating ) return;
			histStamp = "";
			RefreshStats();
		};

		// Die beiden gemessenen Tabellen von vorn anfangen lassen. Getrennt,
		// weil sie verschieden teuer sind: die Trefferquote ist reine Messung
		// und kostet beim Zurücksetzen nur Wartezeit, der Vorhalt steuert
		// dagegen mit - ohne ihn hält die Zielhilfe eine Weile schlechter vor.
		tuneReset.Click += ( _, _ ) => ResetTable( "aimtune.cfg", "Die Vorhalt-Messung",
			"Der Vorhalt fängt damit wieder beim Faktor 1,00 an – die Zielhilfe hält "
			+ "die ersten Runden also schlechter vor, bis wieder etwas gemessen ist. "
			+ "Diese Karte zeigt außerdem, was im Protokoll steht, und bleibt deshalb "
			+ "bis zum nächsten Spiel auf den alten Zahlen stehen." );
		resetTip.SetToolTip( tuneReset,
			"aimtune.cfg nach baseq3\\logs\\ legen und von vorn messen" );

		rateReset.Click += ( _, _ ) => ResetTable( "aimrate.cfg", "Die Trefferquoten-Messung",
			"Die Tabelle ist danach leer und füllt sich wieder ab dem nächsten Spiel. "
			+ "Am Zielen ändert das nichts – diese Tabelle misst nur, sie steuert nicht." );
		resetTip.SetToolTip( rateReset,
			"aimrate.cfg nach baseq3\\logs\\ legen und von vorn messen" );

		// auch mitlesen, wenn das Spiel von Hand gestartet wurde
		logPath = Path.Combine( HomePath, "qconsole.log" );
		start.Click += ( _, _ ) => StartGame();
		save.Click += ( _, _ ) => SaveSettings();
		poll.Tick += ( _, _ ) => RefreshStats();
		fitTimer.Tick += ( _, _ ) => {
			fitTimer.Stop();
			FlushFits();
		};

		Controls.Add( BuildLayout() );
		Application.AddMessageFilter( this );
		// Die eingestellte Breite ist zugleich das Gewicht, nach dem beim
		// Vergroessern verteilt wird. Ohne dieses Merken nimmt FitColumns die
		// jeweils aktuelle Breite als Gewicht, und die Verhaeltnisse wandern
		// bei jedem Ziehen am Fenster ein Stueck weiter.
		foreach ( var view in new[] { shotView, tuneView, rankView, histView, prioView } ) {
			foreach ( ColumnHeader c in view.Columns ) c.Tag = c.Width;
			FitOnResize( view );
		}
		FillPriorities();			// erst wenn die Liste im Fenster haengt
		LoadSettings();
		poll.Start();
	}

	protected override void OnLoad( EventArgs e ) {
		base.OnLoad( e );

		// Beim ersten Start gross aufmachen - fuenf Karten voller Tabellen
		// wollen Platz. Danach gilt, was der Benutzer zuletzt eingestellt hat.
		//
		// Auf den Bildschirm beschnitten: die gemerkte Groesse kommt von dem
		// Rechner, an dem zuletzt gearbeitet wurde, und der naechste kann
		// einen kleineren Schirm haben. Ohne das haengt das Fenster hinaus,
		// ohne dass es auffaellt - der Rahmen ist ja nicht zu sehen - und in
		// jeder Karteikarte fehlt die letzte Spalte.
		var room = Screen.FromPoint( Cursor.Position ).WorkingArea;
		if ( windowSize.Width > 400 && windowSize.Height > 300 ) {
			ClientSize = new Size(
				Math.Min( windowSize.Width, room.Width ),
				Math.Min( windowSize.Height, room.Height ) );
			CenterToScreen();
		} else {
			WindowState = FormWindowState.Maximized;
		}

		// 470 statt 430: die breiteste Zeile (Spielordner mit Knopf) braucht so
		// viel, sonst steht rechts etwas ueber den Rand hinaus
		if ( splitMain is not null && splitMain.Width > 800 ) {
			splitMain.Panel1MinSize = 470;
			splitMain.Panel2MinSize = 320;
			int want = splitterSaved > 0 ? splitterSaved : 540;
			splitMain.SplitterDistance = Math.Clamp( want, 470, splitMain.Width - 320 );
		}

		// Falls die Einstellungen vor dem Aufbau der Karten gelesen wurden
		if ( settingsTabs is not null && settingsTabSaved > 0
			&& settingsTabSaved < settingsTabs.TabPages.Count ) {
			settingsTabs.SelectedIndex = settingsTabSaved;
		}
	}

	// Eingestelltes bleibt eingestellt, ohne dass man daran denken muss. Der
	// Knopf bleibt, weil er zwischendurch sichert, aber er ist nicht mehr die
	// einzige Gelegenheit: eine Abendarbeit an den Vorranglisten war schon
	// verloren, weil das Fenster ohne Klick geschlossen wurde.
	protected override void OnFormClosing( FormClosingEventArgs e ) {
		SaveSettings();
		base.OnFormClosing( e );
	}

	protected override void OnFormClosed( FormClosedEventArgs e ) {
		Application.RemoveMessageFilter( this );
		base.OnFormClosed( e );
	}

	// Ohne Zielhilfe ist der Rest der Gruppe grau; der Vorhalt-Regler gehoert
	// dem Spiel, solange es ihn selbst optimiert
	void UpdateAimEnabled() {
		bool on = aimAssist.Checked;
		aimStrength.Enabled = on;
		aimKey.Enabled = on;
		aimAttacker.Enabled = on;
		aimFreeze.Enabled = on;
		aimSmooth.Enabled = on;
		aimExact.Enabled = on;
		aimLearn.Enabled = on;
		aimLead.Enabled = on && !aimLearn.Checked;
	}

	void BeginAimKeyCapture() {
		if ( capturingAimKey ) return;
		aimKeyBeforeCapture = aimKey.Text;
		aimKey.Text = "Taste drücken …";
		aimKey.SelectAll();
		capturingAimKey = true;
	}

	void FinishAimKeyCapture( string key ) {
		aimKey.Text = key;
		aimKey.SelectionLength = 0;
		capturingAimKey = false;
	}

	public bool PreFilterMessage( ref Message m ) {
		if ( !capturingAimKey ) return false;

		// Abbrechen zuerst, sonst waere Escape eine ganz normale Taste
		if ( m.Msg is WmKeyDown or WmSysKeyDown && (Keys)(int)m.WParam == Keys.Escape ) {
			FinishAimKeyCapture( aimKeyBeforeCapture );
			return true;
		}

		// Das Mausrad kennt kein Gedrückthalten, als Haltetaste also unbrauchbar
		if ( m.Msg == WmMouseWheel ) {
			return true;
		}

		string? key = m.Msg switch {
			WmLeftButtonDown => "MOUSE1",
			WmRightButtonDown => "MOUSE2",
			WmMiddleButtonDown => "MOUSE3",
			WmXButtonDown => ( ( m.WParam.ToInt64() >> 16 ) & 0xffff ) == 1 ? "MOUSE4" : "MOUSE5",
			WmKeyDown or WmSysKeyDown => QuakeKeyName( (Keys)(int)m.WParam, m.LParam.ToInt64() ),
			_ => null,
		};

		if ( key is null ) return false;

		FinishAimKeyCapture( key );
		return true;
	}

	// Wer wegklickt, hat die Taste nicht gewaehlt: sonst schluckt das Feld
	// den naechsten Klick irgendwo in der App.
	protected override void OnDeactivate( EventArgs e ) {
		if ( capturingAimKey ) FinishAimKeyCapture( aimKeyBeforeCapture );
		base.OnDeactivate( e );
	}

	static string? QuakeKeyName( Keys key, long lParam ) {
		// Bit 24 unterscheidet die Zusatztasten vom Ziffernblock
		bool extended = ( lParam & ( 1L << 24 ) ) != 0;

		key &= Keys.KeyCode;
		if ( key == Keys.Enter ) return extended ? "KP_ENTER" : "ENTER";
		if ( key >= Keys.A && key <= Keys.Z ) return ( (char)( 'a' + key - Keys.A ) ).ToString();
		if ( key >= Keys.D0 && key <= Keys.D9 ) return ( (char)( '0' + key - Keys.D0 ) ).ToString();
		if ( key >= Keys.F1 && key <= Keys.F15 ) return $"F{key - Keys.F1 + 1}";

		return key switch {
			Keys.Back => "BACKSPACE",
			Keys.Tab => "TAB",
			Keys.Enter => "ENTER",
			Keys.Space => "SPACE",
			Keys.Up => "UPARROW",
			Keys.Down => "DOWNARROW",
			Keys.Left => "LEFTARROW",
			Keys.Right => "RIGHTARROW",
			Keys.Menu => "ALT",
			Keys.ControlKey => "CTRL",
			Keys.ShiftKey => "SHIFT",
			Keys.CapsLock => "CAPSLOCK",
			Keys.Insert => "INS",
			Keys.Delete => "DEL",
			Keys.PageDown => "PGDN",
			Keys.PageUp => "PGUP",
			Keys.Home => "HOME",
			Keys.End => "END",
			Keys.Pause => "PAUSE",
			Keys.NumPad0 => "KP_INS",
			Keys.NumPad1 => "KP_END",
			Keys.NumPad2 => "KP_DOWNARROW",
			Keys.NumPad3 => "KP_PGDN",
			Keys.NumPad4 => "KP_LEFTARROW",
			Keys.NumPad5 => "KP_5",
			Keys.NumPad6 => "KP_RIGHTARROW",
			Keys.NumPad7 => "KP_HOME",
			Keys.NumPad8 => "KP_UPARROW",
			Keys.NumPad9 => "KP_PGUP",
			Keys.Decimal => "KP_DEL",
			Keys.Divide => "KP_SLASH",
			Keys.Subtract => "KP_MINUS",
			Keys.Add => "KP_PLUS",
			Keys.Multiply => "KP_STAR",
			Keys.NumLock => "KP_NUMLOCK",
			Keys.OemSemicolon => "SEMICOLON",
			Keys.Oemplus => "=",
			Keys.Oemcomma => ",",
			Keys.OemMinus => "-",
			Keys.OemPeriod => ".",
			Keys.OemQuestion => "/",
			Keys.Oemtilde => "`",
			Keys.OemOpenBrackets => "[",
			Keys.OemPipe => "\\",
			Keys.OemCloseBrackets => "]",
			Keys.OemQuotes => "'",
			_ => null,
		};
	}

	// Links wird eingestellt, rechts wird abgelesen. Der Teiler bleibt beim
	// Ziehen an der linken Spalte haengen, damit die Tabellen jede Breite
	// bekommen, die das Fenster hergibt.
	Control BuildLayout() {
		// Breiten erst setzen, wenn der Teiler eine hat: vor dem Einhaengen ist
		// er schmaler als seine eigenen Mindestbreiten und wehrt sich dagegen
		var split = new SplitContainer {
			Dock = DockStyle.Fill, Orientation = Orientation.Vertical,
			FixedPanel = FixedPanel.Panel1, SplitterWidth = 8,
		};
		splitMain = split;
		// Der Teiler laesst sich schon immer ziehen, sah aber aus wie eine Luecke.
		// Drei Punkte in der Mitte sagen, dass man ihn anfassen darf.
		split.Paint += ( _, e ) => {
			var bar = split.SplitterRectangle;
			int x = bar.X + bar.Width / 2 - 1;
			int y = bar.Y + bar.Height / 2;
			using var dot = new SolidBrush( Color.FromArgb( 150, 150, 150 ) );
			for ( int i = -2; i <= 2; i++ ) e.Graphics.FillRectangle( dot, x, y + i * 9, 2, 5 );
		};
		split.Panel1.Padding = new Padding( 12, 12, 6, 12 );
		// Die Karten scrollen selbst, das Panel darf es nicht auch noch tun
		split.Panel1.AutoScroll = false;
		split.Panel2.Padding = new Padding( 6, 12, 12, 12 );

		// Links oben bleibt, was man jederzeit braucht - starten, sichern, und
		// welcher Bau gerade laeuft. Darunter die Karten, damit nicht mehr alle
		// Gruppen gleichzeitig um Aufmerksamkeit bitten.
		var left = new TableLayoutPanel {
			Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 2,
		};
		left.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );
		left.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
		left.RowStyles.Add( new RowStyle( SizeType.Percent, 100 ) );
		left.Controls.Add( BuildHeader(), 0, 0 );
		left.Controls.Add( BuildSettingsTabs(), 0, 1 );

		split.Panel1.Controls.Add( left );
		split.Panel2.Controls.Add( BuildStatsBox() );
		return split;
	}

	// Der Hauptknopf gehoert nicht in eine Karte: sonst waere er weg, sobald man
	// woanders nachsieht.
	Control BuildHeader() {
		var head = new TableLayoutPanel {
			Dock = DockStyle.Top, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink,
			ColumnCount = 1, RowCount = 2, Margin = new Padding( 0, 0, 0, 8 ),
		};
		head.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );
		head.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
		head.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
		head.Controls.Add( Row( Pad( start ), Pad( save ), Pad( status ) ), 0, 0 );
		head.Controls.Add( Row( logVersion ), 0, 1 );
		return head;
	}

	TabControl BuildSettingsTabs() {
		var tabs = new TabControl { Dock = DockStyle.Fill };
		settingsTabs = tabs;
		tabs.TabPages.Add( SettingsPage( "Spiel", BuildMatchBox(), BuildSpawnBox() ) );
		tabs.TabPages.Add( SettingsPage( "Trefferton", BuildSoundBox() ) );
		tabs.TabPages.Add( SettingsPage( "Zielen", BuildAimBox(), BuildLeadBox(), BuildSwitchBox() ) );
		tabs.TabPages.Add( SettingsPage( "Anzeige", BuildBotBox(), BuildItemBox() ) );
		return tabs;
	}

	// Eine Karte voller Gruppen, gestapelt wie die linke Spalte vorher.
	static TabPage SettingsPage( string title, params Control[] groups ) {
		var page = new TabPage( title ) {
			Padding = new Padding( 10 ), BackColor = SystemColors.Control, AutoScroll = true,
		};
		var column = new TableLayoutPanel {
			Dock = DockStyle.Top, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink,
			ColumnCount = 1, RowCount = groups.Length,
		};
		column.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );
		for ( int i = 0; i < groups.Length; i++ ) {
			column.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
			column.Controls.Add( groups[i], 0, i );
		}
		page.Controls.Add( column );
		return page;
	}

	GroupBox BuildMatchBox() {
		var browse = new Button { Text = "…", Width = 34, Margin = new Padding( 0, 3, 14, 0 ) };
		browse.Click += ( _, _ ) => {
			using var dlg = new FolderBrowserDialog { SelectedPath = gameDir.Text };
			if ( dlg.ShowDialog() == DialogResult.OK ) gameDir.Text = dlg.SelectedPath;
		};

		// Starten/Speichern/Status und der Bau-Stempel sitzen jetzt oben fest,
		// ausserhalb der Karten - siehe BuildHeader.
		return Group( "Spiel",
			Row( Labelled( "Spielordner:", gameDir ), browse ),
			Row( Labelled( "Map:", map ), Labelled( "Bots:", bots ), Labelled( "Können:", skill ) ),
			Row( Pad( noSelfDamage ) ),
			Row( Labelled( "Munition:", infiniteAmmo ) ),
			Row( Labelled( "Nachladezeit:", weaponRate ) ),
			Row( Pad( weaponRateValue ) ),
			Row( Labelled( "Bildschirm:", screenMode ) ),
			Row( Labelled( "Auflösung:", screenSize ) ),
			Row( Labelled( "Bildrate:", maxFps ) ) );
	}

	// Womit die Karte bestückt wird. Die Sockel bleiben, wo sie sind - nur ihr
	// Inhalt wird auf die angehakten Waffen verteilt, Munitionskisten genauso.
	GroupBox BuildSpawnBox() {
		// "Karten-Standard" und nicht "alle": alles angehakt HEISST, dass nichts
		// umverteilt wird, und der Knopf soll sagen, was dabei herauskommt,
		// statt zu beschreiben, wie er es macht. Vorher hiess er "alle", und
		// dass das dasselbe ist wie "die Karte in Ruhe lassen", stand nur klein
		// in der Zeile darunter.
		var all = new Button { Text = "Karten-Standard", Width = 124, Margin = new Padding( 0, 0, 6, 0 ) };
		var mg = new Button { Text = "nur MG", Width = 74 };
		all.Click += ( _, _ ) => {
			for ( int i = 0; i < spawnWeapons.Items.Count; i++ ) spawnWeapons.SetItemChecked( i, true );
		};
		// Der haeufigste Fall: eine Messreihe, in der nur das Maschinengewehr
		// geschossen wird. Der Gauntlet bleibt an, er nimmt keinen Sockel weg.
		mg.Click += ( _, _ ) => {
			for ( int i = 0; i < spawnWeapons.Items.Count; i++ ) {
				spawnWeapons.SetItemChecked( i, SpawnItems[i].Weapon == "machinegun"
					|| SpawnItems[i].Weapon == "gauntlet" );
			}
		};
		spawnWeapons.ItemCheck += ( _, _ ) => { if ( IsHandleCreated ) BeginInvoke( ShowSpawn ); };
		ShowSpawn();

		hintTip.SetToolTip( spawnWeapons, "Alle Waffensockel und Munitionskisten der Karte werden reihum"
			+ " auf die angehakten Waffen verteilt – die Karte behält also ihre Dichte, nur der Inhalt"
			+ " wechselt. Gedacht für vergleichbare Messreihen: eine Waffe anhaken, dann wird in jedem"
			+ " Lauf dasselbe geschossen.\n\n„Karten-Standard“ hakt alles an, und das heißt: nichts"
			+ " umverteilen, die Karte bleibt, wie der Kartenbauer sie gesetzt hat. Nichts anzuhaken"
			+ " führt zum selben Ergebnis – die Zeile darunter sagt in jedem Fall, was wirklich"
			+ " passiert.\n\nDer Gauntlet bleibt liegen, wo die Karte ihn hat, rückt aber nie auf einen"
			+ " fremden Sockel nach.\n\nDu und die Bots starten unabhängig davon immer mit Gauntlet und"
			+ " Maschinengewehr – das ist Quake-3-Verhalten. Powerups, Rüstung und Medipacks bleiben"
			+ " unangetastet." );

		return Group( "Waffen auf der Karte",
			Row( spawnWeapons ),
			Row( all, mg ),
			Row( Pad( spawnValue ) ) );
	}

	// Die Liste für g_weaponSpawns, in Klassennamen. Leer heißt "Karte
	// unverändert" - und das gilt für drei Fälle: nichts angehakt, alles
	// angehakt, oder nur der Gauntlet, der nie nachrückt. In allen dreien gibt
	// es nichts umzuverteilen, und das Spiel soll das auch so sehen.
	string SpawnList() {
		var chosen = new List<string>();
		bool fills = false;
		for ( int i = 0; i < SpawnItems.Length; i++ ) {
			if ( !spawnWeapons.GetItemChecked( i ) ) continue;
			chosen.Add( SpawnItems[i].Weapon );
			if ( SpawnItems[i].Weapon != "gauntlet" ) fills = true;
		}
		if ( !fills || chosen.Count == SpawnItems.Length ) return "";
		return string.Join( " ", chosen );
	}

	void ShowSpawn() {
		int ticked = 0, n = 0;
		for ( int i = 0; i < SpawnItems.Length; i++ ) {
			if ( !spawnWeapons.GetItemChecked( i ) ) continue;
			ticked++;
			if ( SpawnItems[i].Weapon != "gauntlet" ) n++;
		}

		// Drei Wege fuehren zu "unveraendert", und sie sollen sich auch
		// unterscheiden lassen: alles angehakt ist die Absicht, nichts
		// angehakt ein Versehen, und nur der Gauntlet waere sinnlos.
		if ( SpawnList().Length == 0 ) {
			spawnValue.Text = ticked == SpawnItems.Length
				? "Karten-Standard – die Karte bleibt, wie der Kartenbauer sie gesetzt hat"
				: ticked == 0
					? "nichts gewählt – die Karte bleibt beim Standard"
					: "nur der Gauntlet – die Karte bleibt beim Standard";
			spawnValue.ForeColor = ticked == SpawnItems.Length ? Color.DimGray : Color.Firebrick;
			return;
		}
		spawnValue.ForeColor = Color.DimGray;
		spawnValue.Text = n == 1
			? "jeder Waffensockel und jede Munitionskiste wird zu dieser einen Waffe"
			: $"alle Waffensockel und Munitionskisten werden auf diese {n} Waffen verteilt";
	}

	GroupBox BuildSoundBox() {
		return Group( "Trefferton",
			Row( Labelled( "Ton:", hitSound ) ),
			Row( Labelled( "Datei:", hitSoundFile ) ),
			Row( Pad( hitPitch ) ),
			Row( Labelled( "volle HP:", pitchFull ), Labelled( "leer:", pitchEmpty ) ),
			Row( Labelled( "Kill:", pitchKill ), Labelled( "voll ab:", pitchStack ) ) );
	}

	// Wie gezielt wird
	GroupBox BuildAimBox() {
		hintTip.SetToolTip( aimAssist, "Wen die Hilfe nimmt, steht in der Karte „Vorrang“ rechts." );
		hintTip.SetToolTip( aimKey, "Die Taste zielt nur; geschossen wird mit der Feuertaste." );
		hintTip.SetToolTip( aimFreeze, "Die eigene Maus wird verworfen, solange die Hilfe wirklich auf ein Ziel führt."
 			+ " Der exakte Schussbefehl war ohnehin schon frei von ihr; das hier nimmt sie von allen"
 			+ " anderen Befehlen weg. Preis: man kann während des Haltens weder umsehen noch ein"
 			+ " anderes Ziel anvisieren - das Ziel wählt dann allein die Vorrangliste." );
		hintTip.SetToolTip( holdLottery, "Gilt, solange die Zieltaste hält, und zählt im Tab „Trefferton“ mit." );

		return Group( "Zielhilfe",
			Row( Pad( aimAssist ) ),
			Row( Labelled( "Halten:", aimKey ), Labelled( "Snap-Stärke:", aimStrength ) ),
			Row( Labelled( "Schussmoment exakt:", aimExact ) ),
			Row( Pad( aimFreeze ) ),
			Row( Pad( aimAttacker ) ),
			Row( Pad( aimHoldFire ) ),
			Row( Labelled( "auch aussichtslose:", holdLottery ) ),
			Row( Pad( holdLotteryValue ) ) );
	}

	// Der Waffenwechsel lag in der Zielhilfe, hat mit Zielen aber nichts zu tun.
	GroupBox BuildSwitchBox() {
		const string why = "Beste zuerst. Ohne dies merkt es das Spiel erst beim Klick auf die"
			+ " leere Waffe und greift zum Enterhaken.";

		hintTip.SetToolTip( autoSwitch, why );
		hintTip.SetToolTip( autoSwitchOrder, why );

		return Group( "Waffenwechsel",
			Row( Pad( autoSwitch ) ),
			Row( Labelled( "Reihenfolge:", autoSwitchOrder ) ) );
	}

	// Wie weit vorgehalten wird
	GroupBox BuildLeadBox() {
		hintTip.SetToolTip( aimEdge, "Läuft die vorhergesagte Strecke über eine Plattformkante, fällt der Zielpunkt"
 			+ " von dort. Aus ist die Voreinstellung: die Kante wird erkannt und protokolliert, der"
 			+ " Punkt bleibt stehen. Von den ersten vier nachprüfbaren Schüssen war einer gut, einer"
 			+ " ein Fehlalarm und zwei fielen zu tief - erst messen, dann anlegen." );
		return Group( "Vorhalt",
			Row( Labelled( "Glättung (ms):", aimSmooth ), Labelled( "Richtung halten (s):", aimLead ) ),
			Row( Pad( aimEdge ) ),
			Row( Pad( aimLearn ) ),
			Row( Pad( aimLearned ) ) );
	}

	// Was zu sehen ist - mit dem Zielen hat das nichts zu tun
	// Mit dauernd voller Munition wird keine Waffe je leer, und der Wechsel
	// darauf kann nicht ausloesen. Ein Haken, der sichtbar aktiv ist und nichts
	// tut, kostet spaeter eine Stunde Fehlersuche an der falschen Stelle.
	void UpdateSwitchEnabled() {
		bool possible = infiniteAmmo.SelectedIndex == 0;
		autoSwitch.Enabled = possible;
		autoSwitchOrder.Enabled = possible && autoSwitch.Checked;
	}

	void UpdateItemEnabled() {
		itemOutlineAll.Enabled = itemOutline.Checked;
		itemRange.Enabled = itemOutline.Checked;
		itemRangeValue.Enabled = itemOutline.Checked;
	}

	// Der Wert ist ein Vielfaches des Wirkradius, in Zehnteln eingestellt. Null
	// heisst: nie zurueckhalten. Die Zahl daneben nennt es fuer die Rakete in
	// Einheiten, damit es greifbar bleibt.
	void ShowHoldLottery() {
		double f = holdLottery.Value / 10.0;
		holdLotteryValue.Text = f <= 0 ? "aus – es wird immer geschossen"
			: $"ab {f:0.0}× Wirkradius, Rakete {f * 120:0} Einheiten";
	}

	// Prozent sagen wenig; die Millisekunden der Waffen, die hier gemessen
	// werden, sagen alles. Rakete und MG stehen stellvertretend fuer langsam
	// und schnell.
	void ShowWeaponRate() {
		int p = weaponRate.Value;
		// Fuenfzig Millisekunden sind der Boden, ein ganzes Server-Bild. Darunter
		// zaehlt die Trefferquoten-Tabelle nicht mehr mit und das halbe
		// Muendungsfeuer bleibt aus, also wird gar nicht erst schneller gefeuert.
		int rocket = Math.Max( 50, 800 * p / 100 );
		int mg = Math.Max( 50, 100 * p / 100 );
		weaponRateValue.Text = p == 100
			? "wie im Spiel – Rakete 800 ms, MG 100 ms"
			: $"{p} % – Rakete {rocket} ms, MG {mg} ms"
				+ ( mg > 100 * p / 100 ? "  (50 ms ist der Boden)" : "" );
		weaponRateValue.ForeColor = p == 100 ? Color.DimGray : Color.DarkGoldenrod;
	}

	void ShowItemRange() {
		itemRangeValue.Text = itemRange.Value == 0 ? "immer voll sichtbar"
			: $"voll bis {itemRange.Value / 2}, weg ab {itemRange.Value} Einheiten";
	}

	// Gegner und Gegenstaende lagen zusammen in einer Gruppe "Anzeige" und haben
	// miteinander nichts zu tun; getrennt liest sich beides schneller.
	GroupBox BuildBotBox() {
		hintTip.SetToolTip( botDamage, "Über dem Gegner steht bei Geschossen die Zeit bis zum"
			+ " Einschlag, gefärbt danach, was der Schuss taugt." );
		hintTip.SetToolTip( infiniteAmmo, "Füllt die Munition jedes Server-Bildes auf 999 auf, damit eine Messung"
 			+ " nicht daran endet, dass die Waffe leer ist. „Für alle“ versorgt auch die Bots -"
 			+ " dann laufen sie aber keine Munitionskiste mehr an, und genau diese Wege sind es,"
 			+ " an denen die Vorhersage gemessen wird." );
		hintTip.SetToolTip( weaponRate, "Die Nachladezeiten in Prozent der normalen, nur für dich – die Bots"
 			+ " schießen weiter im Originaltakt, sonst käme man vor lauter Einschlägen nicht zum"
 			+ " Messen. Die Zielhilfe rechnet denselben Takt mit, der exakte Griff im Schussmoment"
 			+ " greift also weiter richtig.\n\n"
 			+ "Was es NICHT bringt: mehr Lernproben. Der Lerner nimmt nur Projektilwaffen und"
 			+ " höchstens eine Probe je Ziel, solange die erste noch fliegt – schneller schießen"
 			+ " erhöht die Zahl der Schüsse, nicht die der Proben. Dafür braucht es mehr Bots.\n\n"
 			+ "Das Bild läuft nicht mit: die Bewegungsvorhersage im cgame kennt nur die"
 			+ " Originalzeiten, die Waffenanimation kann also zucken. Und eine Sitzung mit anderem"
 			+ " Takt ist mit den alten nicht direkt vergleichbar – der Wert steht im Protokollkopf." );
		hintTip.SetToolTip( noSelfDamage, "Ein Raketen- oder BFG-Sprung trägt genauso weit wie sonst – der Rückstoß"
			+ " wird im Spiel vor dem Schaden verrechnet –, kostet aber kein Leben mehr. Nur gegen dich"
			+ " selbst: wen dein Splash sonst noch erwischt, trifft er unverändert." );
		hintTip.SetToolTip( damagePlums, "Wieviel jeder deiner Treffer angerichtet hat, steigt als Zahl auf"
			+ " und verblasst – blass bei einem Streifschuss, leuchtend bei einem schweren.\n\n„über dem"
			+ " Getroffenen“ sucht sich den Gegner selbst: zuerst das, worauf die Zielhilfe gefeuert hat,"
			+ " sonst der Bot, der dem Blick am nächsten steht. Findet sich keiner im engen Kegel –"
			+ " etwa weil eine Rakete eine Sekunde unterwegs war und du längst woanders hinsiehst –,"
			+ " erscheint die Zahl neben dem Fadenkreuz statt über dem Falschen.\n\nDie Schrotflinte"
			+ " zählt zu wenig, weil jedes Korn einzeln verrechnet wird und nur das letzte in der"
			+ " Meldung landet. Eigener Schaden zählt nie mit." );
		return Group( "Gegner-Markierung",
			// eigene Zeilen: nebeneinander lief die zweite aus der Gruppe heraus
			Row( Pad( botOutline ) ),
			Row( Pad( botDamage ) ),
			Row( Labelled( "Markierung:", botStyle ) ),
			Row( Labelled( "Balken:", botBars ) ),
			Row( Labelled( "Farbe:", botColor ), Pad( botColorPick ) ),
			Row( Pad( botName ) ),
			Row( Labelled( "Schadenszahlen:", damagePlums ) ) );
	}

	GroupBox BuildItemBox() {
		hintTip.SetToolTip( itemRange, "Ferne Gegenstände werden blasser und verschwinden ganz." );

		return Group( "Gegenstände",
			Row( Pad( itemOutline ) ),
			Row( Pad( itemOutlineAll ) ),
			Row( Labelled( "Sichtweite:", itemRange ) ),
			Row( Pad( itemRangeValue ) ) );
	}

	// Gruppe aus festen Zeilen: nichts bricht um, nichts ueberlappt
	static GroupBox Group( string title, params Control[] rows ) {
		var stack = new TableLayoutPanel {
			Dock = DockStyle.Top, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink,
			ColumnCount = 1, RowCount = rows.Length,
		};
		stack.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );

		for ( int i = 0; i < rows.Length; i++ ) {
			stack.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
			rows[i].Dock = DockStyle.Top;
			stack.Controls.Add( rows[i], 0, i );
		}

		var box = new GroupBox {
			Text = title, Dock = DockStyle.Top, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink,
			Padding = new Padding( 10, 6, 10, 10 ), Margin = new Padding( 0, 0, 0, 10 ),
		};
		box.Controls.Add( stack );
		return box;
	}

	static Label Number() => new() {
		Text = "0", Font = new Font( "Segoe UI", 20, FontStyle.Bold ), AutoSize = true,
		Margin = new Padding( 4, 0, 24, 4 ),
	};

	static Control Counter( string text, Label value ) {
		var flow = new FlowLayoutPanel { AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, WrapContents = false };
		flow.Controls.Add( new Label { Text = text, AutoSize = true, Margin = new Padding( 0, 12, 2, 0 ) } );
		flow.Controls.Add( value );
		return flow;
	}

	// Ein senkrechter Strich, der zusammengehoerige Bedienelemente trennt.
	// Billiger und ruhiger als jede Gruppenbox in einer Werkzeugzeile.
	static Control Divider() => new Label {
		AutoSize = false, Width = 1, Height = 24, BorderStyle = BorderStyle.Fixed3D,
		Margin = new Padding( 6, 4, 10, 0 ),
	};

	static FlowLayoutPanel Row( params Control[] items ) {
		var flow = new FlowLayoutPanel { AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, WrapContents = false };
		flow.Controls.AddRange( items );
		return flow;
	}

	// Dasselbe untereinander. Der Kopf einer Karteikarte ist AutoSize, also
	// darf dort auch ein Stapel stehen und nicht nur eine Reihe.
	static FlowLayoutPanel Column( params Control[] items ) {
		var flow = new FlowLayoutPanel {
			AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink,
			FlowDirection = FlowDirection.TopDown, WrapContents = false,
		};
		flow.Controls.AddRange( items );
		return flow;
	}

	Control BuildStatsBox() {
		var tabs = new TabControl { Dock = DockStyle.Fill };
		tabs.TabPages.Add( Page( "Trefferton",
			Row( Counter( "Schaden:", statHits ), Counter( "Treffer:", statFrames ),
				Counter( "Sounds:", statSounds ), Counter( "ohne Ton:", statMissed ) ),
			logView ) );
		tabs.TabPages.Add( Page( "Zielhilfe",
			Row( Counter( "Schüsse:", statShots ), Counter( "getroffen:", statShotHits ),
				Counter( "daneben:", statShotMiss ), Counter( "Quote:", statShotRate ),
				Counter( "ohne Hilfe:", statShotRateOff ), Counter( "Fehler ø:", statShotError ),
				Counter( "Feuer gehalten:", statHold ) ),
			shotView ) );
		tabs.TabPages.Add( Page( "pro Waffe",
			Row( Counter( "Töpfe:", statTuneBoxes ), Counter( "Proben:", statTuneSamples ),
				tuneReset ),
			tuneView ) );
		tabs.TabPages.Add( Page( "Rangliste",
			Row( Counter( "beste Waffe:", statBestWeapon ) ),
			rankView ) );
		tabs.TabPages.Add( Page( "Trefferquote",
			Row( Counter( "am besten:", statBestRange ), rateReset ),
			rateView ) );

		// Zwei Zeilen Kopf: oben die Zaehler und die Waffenauswahl, darunter
		// die letzte Korrektur als ganzer Satz - so muss fuer die Frage
		// „was hat sich zuletzt geaendert" niemand die Liste lesen.
		var histBody = new Panel { Dock = DockStyle.Fill };
		histBody.Controls.Add( histView );
		histBody.Controls.Add( histEmpty );
		tabs.TabPages.Add( Page( "Korrekturen",
			Column(
				Row( Counter( "Korrekturen:", statFixes ), Counter( "stärker:", statFixUp ),
					Counter( "schwächer:", statFixDown ), Counter( "ohne Wirkung:", statFixFlat ),
					Divider(), Pad( new Label { Text = "Waffe:", AutoSize = true,
						Margin = new Padding( 0, 12, 4, 0 ) } ), histWeapon ),
				histLast ),
			histBody ) );
		// Eigene Karteikarte statt Page(): die Bedienung bekommt eine feste
		// Hoehe, sonst nimmt sie sich mit den Schiebern darin den ganzen Platz
		// und die Liste bleibt einen Pixel hoch.
		//
		// Zwei Zeilen statt einer, weil es zwei Fragen sind: oben WEN die
		// Liste betrifft, unten WAS an der gewaehlten Zeile verstellt wird.
		// In einer Reihe stand die Waffenauswahl neben dem Gewichtsschieber,
		// und dazwischen schwebte eine Meldung wie ein Fehler.
		var prioPage = new TabPage( "Vorrang" ) { Padding = new Padding( 10 ), BackColor = SystemColors.Control };
		var prioGrid = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 3 };
		prioGrid.RowStyles.Add( new RowStyle( SizeType.Absolute, 40 ) );
		prioGrid.RowStyles.Add( new RowStyle( SizeType.Absolute, 46 ) );
		prioGrid.RowStyles.Add( new RowStyle( SizeType.Percent, 100 ) );
		prioGrid.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );

		var prioWho = Row(
			Pad( new Label { Text = "Liste für:", AutoSize = true, Margin = new Padding( 0, 7, 6, 0 ) } ),
			Pad( prioWeapon ), Pad( prioReset ), Pad( prioWarn ) );
		prioWho.AutoSize = false;			// sonst streitet sich AutoSize mit Dock
		prioWho.Dock = DockStyle.Fill;
		prioWho.BackColor = Color.FromArgb( 244, 245, 248 );
		prioWho.Padding = new Padding( 8, 2, 8, 2 );

		var prioWhat = Row(
			Pad( prioUp ), Pad( prioDown ),
			Pad( Divider() ),
			Pad( new Label { Text = "Gewicht", AutoSize = true, Margin = new Padding( 0, 7, 6, 0 ) } ),
			Pad( prioBar ), Pad( prioValue ),
			Pad( Divider() ),
			Pad( new Label { Text = "gilt", AutoSize = true, Margin = new Padding( 0, 7, 6, 0 ) } ),
			Pad( prioLifeBar ), Pad( prioLifeValue ) );
		prioWhat.AutoSize = false;
		prioWhat.Dock = DockStyle.Fill;
		prioWhat.Padding = new Padding( 8, 4, 8, 0 );

		prioGrid.Controls.Add( prioWho, 0, 0 );
		prioGrid.Controls.Add( prioWhat, 0, 1 );
		prioGrid.Controls.Add( prioView, 0, 2 );
		prioPage.Controls.Add( prioGrid );
		tabs.TabPages.Add( prioPage );
		return tabs;
	}

	// Die Liste, nach Gewicht sortiert: oben zaehlt am meisten
	void FillPriorities( string? keep = null ) {
		if ( prioUpdating ) return;			// nicht aus sich selbst heraus
		prioUpdating = true;
		prioClicked = false;
		prioView.BeginUpdate();
		prioView.Items.Clear();

		foreach ( var p in Priorities.OrderByDescending( p => Weight( p.Key ) )
				.ThenBy( p => Array.FindIndex( Priorities, q => q.Key == p.Key ) ) ) {
			int weight = Weight( p.Key );
			// Ein Pfeil sagt, dass diese Zeile von der Standardliste abweicht,
			// damit auf einen Blick klar ist, was fuer diese Waffe eigens gilt
			var row = new ListViewItem( Differs( p.Key ) ? "▸" : "" ) {
				Tag = p.Key, Checked = weight > 0,
			};
			row.SubItems.Add( p.Name );
			row.SubItems.Add( weight.ToString() );
			row.SubItems.Add( WeightBar( weight ) );
			row.SubItems.Add( IsTimed( p.Key ) ? Life( p.Key ).ToString( "0.0" ) + " s" : "—" );
			row.SubItems.Add( Differs( p.Key )
				? $"{p.Effect}  (Standard {prioWeight[p.Key]})" : p.Effect );
			if ( weight == 0 ) row.ForeColor = Color.DimGray;
			else if ( Differs( p.Key ) ) row.ForeColor = Color.DarkSlateBlue;
			prioView.Items.Add( row );
			if ( keep is not null && p.Key == keep ) row.Selected = true;
		}

		prioReset.Enabled = CurWeapon is not null;
		// Keine Zeichengrenze mehr - die Listen gehen als Datei ins Spiel.
		// Stattdessen steht hier, wie viele Waffen eigene Regeln haben.
		// Die Liste ist keine Rangfolge, sondern ein Punktebudget: jede Regel
		// gibt einem Bewerber bis zu ihrem Gewicht, und alles wird addiert.
		// Deshalb koennen zwei Regeln dieselbe Zahl tragen - sie stehen nicht
		// auf demselben Platz, sie geben beide bis zu hundert Punkte. Ohne
		// diesen Satz sieht die nach Gewicht sortierte Liste wie ein Ranking
		// aus, und dann wirkt ein zweimal vergebenes Hundert wie ein Fehler.
		int eigen = Weapons.Count( w => weaponWeight.ContainsKey( w.Key ) || weaponTime.ContainsKey( w.Key ) );
		int summe = Priorities.Sum( p => Weight( p.Key ) );
		var teile = new List<string>();
		if ( eigen == 1 ) teile.Add( "1 Waffe weicht ab" );
		else if ( eigen > 1 ) teile.Add( $"{eigen} Waffen weichen ab" );
		teile.Add( $"die Gewichte addieren sich, höchstens {summe} Punkte" );
		prioWarn.Text = string.Join( "   ·   ", teile );
		prioWarn.ForeColor = Color.DimGray;
		prioView.EndUpdate();
		prioUpdating = false;
		ShowPrioritySelection();
	}

	// Das Gewicht als Balken: die Reihenfolge der Liste IST die Gewichtung,
	// und ein Balken sagt auf einen Blick, wie weit die Abstaende sind - zwei
	// Zeilen mit 80 und 70 stehen anders zueinander als 80 und 10.
	static string WeightBar( int weight ) {
		int full = Math.Clamp( ( weight + 5 ) / 10, 0, 10 );
		return new string( '█', full ) + new string( '·', 10 - full );
	}

	// Nur die drei Regeln ueber etwas Geschehenes haben eine Gueltigkeit
	static bool IsTimed( string key ) =>
		Array.Find( Priorities, p => p.Key == key ).Life > 0;

	void ShowPrioritySelection() {
		// Die Zahl bleibt eine Zahl: der Hinweis, dass nichts gewaehlt ist,
		// gehoert in das breite Feld daneben, nicht in das schmale fuer das
		// Gewicht, wo er auf drei Zeichen abgeschnitten wuerde.
		if ( prioView.SelectedItems.Count == 0 ) {
			prioValue.Text = "–";
			prioLifeValue.Text = "Zeile wählen";
			prioBar.Enabled = false;
			prioLifeBar.Enabled = false;
			prioUp.Enabled = prioDown.Enabled = false;
			return;
		}
		prioBar.Enabled = true;
		prioUp.Enabled = prioDown.Enabled = true;

		var key = (string)prioView.SelectedItems[0].Tag!;
		bool timed = IsTimed( key );
		prioUpdating = true;
		prioBar.Value = Math.Clamp( Weight( key ), prioBar.Minimum, prioBar.Maximum );
		prioLifeBar.Enabled = timed;
		prioLifeBar.Value = timed
			? (int)Math.Clamp( Math.Round( Life( key ) * 10 ), prioLifeBar.Minimum, prioLifeBar.Maximum ) : 0;
		prioUpdating = false;
		prioValue.Text = Weight( key ).ToString();
		prioLifeValue.Text = timed ? Life( key ).ToString( "0.0" ) + " s" : "dauerhaft";
	}

	// Hoeher oder tiefer heisst: das Gewicht mit dem Nachbarn tauschen, denn
	// das Gewicht ist die Reihenfolge
	void MovePriority( int step ) {
		if ( prioView.SelectedItems.Count == 0 ) return;
		int index = prioView.SelectedItems[0].Index, other = index + step;
		if ( other < 0 || other >= prioView.Items.Count ) return;

		var key = (string)prioView.Items[index].Tag!;
		var neighbour = (string)prioView.Items[other].Tag!;
		int mine = Weight( key ), theirs = Weight( neighbour );
		if ( mine == theirs ) {
			// gleich schwer: einen Schritt daran vorbei
			mine = Math.Clamp( theirs - step, 0, 100 );
		} else {
			( mine, theirs ) = ( theirs, mine );
		}
		SetWeight( key, mine );
		SetWeight( neighbour, theirs );
		FillPriorities( key );
	}

	// Der Punkt muss ein Punkt bleiben: die Engine liest die Zahl mit atof,
	// das ein deutsches Komma als Ende der Zahl nimmt und die Nachkommastellen
	// stillschweigend verschluckt.
	string PriorityString() => string.Join( " ", Priorities.Select( p => IsTimed( p.Key )
		? string.Format( System.Globalization.CultureInfo.InvariantCulture,
			"{0}:{1}:{2:0.##}", p.Key, prioWeight[p.Key], prioTime[p.Key] )
		: $"{p.Key}:{prioWeight[p.Key]}" ) );

	// Nur die Abweichungen, als "waffe.kriterium:gewicht". Eine cvar fasst 256
	// Zeichen, also waeren neun volle Listen gar nicht unterzubringen - was
	// hier steht, ist genau das, was anders gemeint war.
	string WeaponPriorityString() {
		var parts = new List<string>();
		foreach ( var w in Weapons ) {
			foreach ( var p in Priorities ) {
				bool hasW = weaponWeight.TryGetValue( w.Key, out var a ) && a.ContainsKey( p.Key );
				bool hasT = weaponTime.TryGetValue( w.Key, out var b ) && b.ContainsKey( p.Key );
				if ( !hasW && !hasT ) continue;
				int weight = hasW ? a![p.Key] : prioWeight[p.Key];
				double life = hasT ? b![p.Key] : prioTime[p.Key];
				parts.Add( IsTimed( p.Key )
					? string.Format( System.Globalization.CultureInfo.InvariantCulture,
						"{0}.{1}:{2}:{3:0.##}", w.Key, p.Key, weight, life )
					: $"{w.Key}.{p.Key}:{weight}" );
			}
		}
		return string.Join( " ", parts );
	}

	// Die Waffenlisten gehen als Datei neben die gelernte Tabelle, nicht als
	// Variable: eine cvar fasst 256 Zeichen, und schon zwei von Hand
	// abgestimmte Waffen brauchen 248 davon. Eine Zeile je Waffe, damit ein
	// Mensch sie lesen und von Hand aendern kann.
	void WriteWeaponPriorityFile() {
		var text = new StringBuilder();
		text.AppendLine( "format 1" );
		text.AppendLine( "// Was eine einzelne Waffe anders haelt als cl_aimAssistPriority." );
		text.AppendLine( "// waffe.kriterium:gewicht[:sekunden] - alles Ungenannte folgt der Standardliste." );
		foreach ( var w in Weapons ) {
			var parts = new List<string>();
			foreach ( var p in Priorities ) {
				bool hasW = weaponWeight.TryGetValue( w.Key, out var a ) && a.ContainsKey( p.Key );
				bool hasT = weaponTime.TryGetValue( w.Key, out var b ) && b.ContainsKey( p.Key );
				if ( !hasW && !hasT ) continue;
				int weight = hasW ? a![p.Key] : prioWeight[p.Key];
				double life = hasT ? b![p.Key] : prioTime[p.Key];
				parts.Add( IsTimed( p.Key )
					? string.Format( System.Globalization.CultureInfo.InvariantCulture,
						"{0}.{1}:{2}:{3:0.##}", w.Key, p.Key, weight, life )
					: $"{w.Key}.{p.Key}:{weight}" );
			}
			if ( parts.Count > 0 ) text.AppendLine( string.Join( " ", parts ) );
		}

		try {
			Directory.CreateDirectory( HomePath );
			File.WriteAllText( Path.Combine( HomePath, "aimprio.cfg" ), text.ToString() );
		} catch ( IOException ) {
		} catch ( UnauthorizedAccessException ) {
		}
	}

	void ApplyWeaponPriorityString( string text ) {
		weaponWeight.Clear();
		weaponTime.Clear();
		foreach ( var part in text.Split( ' ', StringSplitOptions.RemoveEmptyEntries ) ) {
			var f = part.Split( ':' );
			int dot = f[0].IndexOf( '.' );
			if ( f.Length < 2 || dot <= 0 ) continue;
			var weapon = f[0][..dot];
			var key = f[0][( dot + 1 )..];
			if ( Array.FindIndex( Weapons, x => x.Key == weapon ) < 0 ) continue;
			if ( !prioWeight.ContainsKey( key ) ) continue;

			if ( int.TryParse( f[1], out int w ) ) {
				if ( !weaponWeight.TryGetValue( weapon, out var over ) ) weaponWeight[weapon] = over = new();
				over[key] = Math.Clamp( w, 0, 100 );
			}
			if ( f.Length > 2 && IsTimed( key )
				&& double.TryParse( f[2], System.Globalization.NumberStyles.Any,
					System.Globalization.CultureInfo.InvariantCulture, out double life ) ) {
				if ( !weaponTime.TryGetValue( weapon, out var over ) ) weaponTime[weapon] = over = new();
				over[key] = Math.Clamp( life, 0, prioLifeBar.Maximum / 10.0 );
			}
		}
		FillPriorities();
	}

	void ApplyPriorityString( string text ) {
		foreach ( var part in text.Split( ' ', StringSplitOptions.RemoveEmptyEntries ) ) {
			var f = part.Split( ':' );
			if ( f.Length < 2 || !prioWeight.ContainsKey( f[0] ) ) continue;
			if ( int.TryParse( f[1], out int w ) ) prioWeight[f[0]] = Math.Clamp( w, 0, 100 );
			if ( f.Length > 2 && IsTimed( f[0] )
				&& double.TryParse( f[2], System.Globalization.NumberStyles.Any,
					System.Globalization.CultureInfo.InvariantCulture, out double life ) ) {
				// dieselbe Obergrenze wie der Schieber, damit ein geladener
				// Wert nicht groesser sein kann als das, was er anzeigt
				prioTime[f[0]] = Math.Clamp( life, 0, prioLifeBar.Maximum / 10.0 );
			}
		}
		FillPriorities();
	}

	// Karteikarte: eine Zeile Zaehler oben, darunter die Liste
	static TabPage Page( string title, Control counters, Control body ) {
		var page = new TabPage( title ) { Padding = new Padding( 10 ), BackColor = SystemColors.Control };
		var grid = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 2 };
		grid.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
		grid.RowStyles.Add( new RowStyle( SizeType.Percent, 100 ) );
		grid.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );
		grid.Controls.Add( counters, 0, 0 );
		grid.Controls.Add( body, 0, 1 );
		page.Controls.Add( grid );
		return page;
	}

	static Control Labelled( string text, Control inner ) {
		var panel = new FlowLayoutPanel { AutoSize = true, WrapContents = false, Margin = new Padding( 0, 0, 14, 0 ) };
		panel.Controls.Add( new Label { Text = text, AutoSize = true, Margin = new Padding( 0, 6, 4, 0 ) } );
		panel.Controls.Add( inner );
		return panel;
	}

	static Control Pad( Control inner ) {
		inner.Margin = new Padding( 0, 6, 14, 0 );
		return inner;
	}

	// Suchreihenfolge: der installierte Build, dann der uebliche Spielordner
	static string FindGameDir() {
		string[] candidates = {
			Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.MyDocuments ),
				"GitHub", "ioq3", "build", "release-mingw64-x86_64" ),
			Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.MyDocuments ), "ioQuake3" ),
		};

		foreach ( var dir in candidates ) {
			if ( File.Exists( Path.Combine( dir, "ioquake3.exe" ) ) ) return dir;
		}

		return candidates[0];
	}

	static string HomePath =>
		Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.ApplicationData ), "Quake3", "baseq3" );

	static string SettingsPath =>
		Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.ApplicationData ), "HitsoundLab", "settings.ini" );

	static string Dec( decimal v ) => v.ToString( System.Globalization.CultureInfo.InvariantCulture );

	// Alle Bedienelemente in eine schlichte Schluessel=Wert-Datei
	void SaveSettings() {
		var s = new StringBuilder();
		s.AppendLine( "gameDir=" + gameDir.Text );
		s.AppendLine( "map=" + map.Text );
		s.AppendLine( "bots=" + (int)bots.Value );
		s.AppendLine( "skill=" + (int)skill.Value );
		s.AppendLine( "hitSound=" + hitSound.SelectedIndex );
		s.AppendLine( "hitSoundFile=" + hitSoundFile.Text );
		s.AppendLine( "hitPitch=" + hitPitch.Checked );
		s.AppendLine( "pitchFull=" + Dec( pitchFull.Value ) );
		s.AppendLine( "pitchEmpty=" + Dec( pitchEmpty.Value ) );
		s.AppendLine( "pitchKill=" + Dec( pitchKill.Value ) );
		s.AppendLine( "pitchStack=" + (int)pitchStack.Value );
		s.AppendLine( "aimAssist=" + aimAssist.Checked );
		s.AppendLine( "aimStrength=" + (int)aimStrength.Value );
		s.AppendLine( "aimKey=" + aimKey.Text );
		s.AppendLine( "aimAttacker=" + aimAttacker.Checked );
		s.AppendLine( "aimFreeze=" + aimFreeze.Checked );
		s.AppendLine( "infiniteAmmo=" + infiniteAmmo.SelectedIndex );
		s.AppendLine( "weaponRate=" + weaponRate.Value );
		s.AppendLine( "aimEdge=" + aimEdge.Checked );
		s.AppendLine( "aimSmooth=" + (int)aimSmooth.Value );
		s.AppendLine( "aimLead=" + Dec( aimLead.Value ) );
		s.AppendLine( "aimExactMode=" + aimExact.SelectedIndex );
		s.AppendLine( "spawnWeapons=" + string.Concat(
			Enumerable.Range( 0, SpawnItems.Length ).Select( i => spawnWeapons.GetItemChecked( i ) ? "1" : "0" ) ) );
		s.AppendLine( "aimLearn=" + aimLearn.Checked );
		s.AppendLine( "aimHoldFire=" + aimHoldFire.Checked );
		s.AppendLine( "holdLottery=" + holdLottery.Value );
		s.AppendLine( "autoSwitch=" + autoSwitch.Checked );
		s.AppendLine( "autoSwitchOrder=" + autoSwitchOrder.Text );
		s.AppendLine( "aimPriority=" + PriorityString() );
		s.AppendLine( "aimPriorityWeapon=" + WeaponPriorityString() );
		// nur eine wiederherstellbare Groesse merken, kein maximiertes Fenster
		if ( WindowState == FormWindowState.Normal ) {
			s.AppendLine( "windowWidth=" + ClientSize.Width );
			s.AppendLine( "windowHeight=" + ClientSize.Height );
		}
		if ( splitMain is not null ) s.AppendLine( "splitter=" + splitMain.SplitterDistance );
		if ( settingsTabs is not null ) s.AppendLine( "settingsTab=" + settingsTabs.SelectedIndex );
		s.AppendLine( "botOutline=" + botOutline.Checked );
		s.AppendLine( "botDamage=" + botDamage.Checked );
		s.AppendLine( "botStyle=" + botStyle.SelectedIndex );
		s.AppendLine( "botBars=" + botBars.SelectedIndex );
		s.AppendLine( "botColor=" + botColor.Text );
		s.AppendLine( "botName=" + botName.Checked );
		s.AppendLine( "damagePlumsMode=" + damagePlums.SelectedIndex );
		s.AppendLine( "noSelfDamage=" + noSelfDamage.Checked );
		s.AppendLine( "maxFps=" + maxFps.SelectedIndex );
		s.AppendLine( "screenMode=" + screenMode.SelectedIndex );
		s.AppendLine( "screenSize=" + screenSize.SelectedIndex );
		s.AppendLine( "itemOutline=" + itemOutline.Checked );
		s.AppendLine( "itemOutlineAll=" + itemOutlineAll.Checked );
		s.AppendLine( "itemRange=" + itemRange.Value );

		try {
			Directory.CreateDirectory( Path.GetDirectoryName( SettingsPath )! );
			File.WriteAllText( SettingsPath, s.ToString() );
			status.Text = "Einstellungen gespeichert";
			status.ForeColor = Color.ForestGreen;
		} catch ( Exception ex ) {
			status.Text = "Speichern fehlgeschlagen: " + ex.Message;
			status.ForeColor = Color.Firebrick;
		}
	}

	void LoadSettings() {
		if ( !File.Exists( SettingsPath ) ) return;

		var v = new Dictionary<string, string>();
		try {
			foreach ( var line in File.ReadAllLines( SettingsPath ) ) {
				var eq = line.IndexOf( '=' );
				if ( eq > 0 ) v[line[..eq]] = line[( eq + 1 )..];
			}
		} catch ( IOException ) {
			return;
		}

		if ( v.TryGetValue( "gameDir", out var g ) && g.Length > 0 ) gameDir.Text = g;
		if ( v.TryGetValue( "map", out var m ) ) { int i = Array.IndexOf( Maps, m ); if ( i >= 0 ) map.SelectedIndex = i; }
		SetNum( bots, v, "bots" );
		SetNum( skill, v, "skill" );
		if ( v.TryGetValue( "hitSound", out var hs ) && int.TryParse( hs, out int hsi ) && hsi >= 0 && hsi < HitSounds.Length )
			hitSound.SelectedIndex = hsi;
		if ( v.TryGetValue( "hitSoundFile", out var hf ) ) hitSoundFile.Text = hf;
		SetBool( hitPitch, v, "hitPitch" );
		SetNum( pitchFull, v, "pitchFull" );
		SetNum( pitchEmpty, v, "pitchEmpty" );
		SetNum( pitchKill, v, "pitchKill" );
		SetNum( pitchStack, v, "pitchStack" );
		SetBool( aimAssist, v, "aimAssist" );
		SetNum( aimStrength, v, "aimStrength" );
		if ( v.TryGetValue( "aimKey", out var ak ) && ak.Length > 0 ) aimKey.Text = ak;
		SetBool( aimAttacker, v, "aimAttacker" );
		SetBool( aimFreeze, v, "aimFreeze" );
		SetIndex( infiniteAmmo, v, "infiniteAmmo" );
		SetBar( weaponRate, v, "weaponRate" );
		SetBool( aimEdge, v, "aimEdge" );
		SetNum( aimSmooth, v, "aimSmooth" );
		SetNum( aimLead, v, "aimLead" );
		// Der alte Haken: "aus" bleibt aus, "an" wird zum neuen Standard "alle"
		if ( v.ContainsKey( "aimExactMode" ) ) SetIndex( aimExact, v, "aimExactMode" );
		else if ( v.TryGetValue( "aimExact", out var oldExact ) && oldExact.Trim() == "False" ) aimExact.SelectedIndex = 0;
		// Eine Ziffer je Eintrag, in der Reihenfolge von SpawnItems. Eine Datei
		// aus einem aelteren Bau hat den Schluessel nicht: dann bleibt alles an,
		// und die Karte ist die, die sie immer war.
		if ( v.TryGetValue( "spawnWeapons", out var sw ) ) {
			sw = sw.Trim();
			for ( int i = 0; i < SpawnItems.Length && i < sw.Length; i++ ) {
				spawnWeapons.SetItemChecked( i, sw[i] == '1' );
			}
			ShowSpawn();
		}
		SetBool( aimLearn, v, "aimLearn" );
		SetBool( aimHoldFire, v, "aimHoldFire" );
		SetBar( holdLottery, v, "holdLottery" );
		SetBool( autoSwitch, v, "autoSwitch" );
		if ( v.TryGetValue( "autoSwitchOrder", out var swOrder ) && swOrder.Length > 0 ) autoSwitchOrder.Text = swOrder;
		if ( v.TryGetValue( "aimPriority", out var prio ) && prio.Length > 0 ) ApplyPriorityString( prio );
		if ( v.TryGetValue( "aimPriorityWeapon", out var wprio ) ) ApplyWeaponPriorityString( wprio );
		if ( v.TryGetValue( "windowWidth", out var ww ) && int.TryParse( ww, out int w2 )
			&& v.TryGetValue( "windowHeight", out var wh ) && int.TryParse( wh, out int h2 ) ) {
			windowSize = new Size( w2, h2 );
		}
		if ( v.TryGetValue( "splitter", out var sp ) && int.TryParse( sp, out int sd ) ) splitterSaved = sd;
		if ( v.TryGetValue( "settingsTab", out var st ) && int.TryParse( st, out int si ) ) {
			settingsTabSaved = si;
			if ( settingsTabs is not null && si >= 0 && si < settingsTabs.TabPages.Count ) {
				settingsTabs.SelectedIndex = si;
			}
		}
		SetBool( botOutline, v, "botOutline" );
		SetBool( botDamage, v, "botDamage" );
		SetIndex( botStyle, v, "botStyle" );
		SetIndex( botBars, v, "botBars" );
		if ( v.TryGetValue( "botColor", out var bc ) && bc.Trim().Length > 0 ) botColor.Text = bc.Trim();
		SetBool( botName, v, "botName" );
		// Der alte Haken: "aus" bleibt aus, "an" wird zu "ueber dem Getroffenen"
		if ( v.ContainsKey( "damagePlumsMode" ) ) SetIndex( damagePlums, v, "damagePlumsMode" );
		else if ( v.TryGetValue( "damagePlums", out var oldPlums ) && oldPlums.Trim() == "False" ) damagePlums.SelectedIndex = 0;
		SetBool( noSelfDamage, v, "noSelfDamage" );
		SetIndex( maxFps, v, "maxFps" );
		SetIndex( screenMode, v, "screenMode" );
		SetIndex( screenSize, v, "screenSize" );
		SetBool( itemOutline, v, "itemOutline" );
		SetBool( itemOutlineAll, v, "itemOutlineAll" );
		SetBar( itemRange, v, "itemRange" );
	}

	static void SetBool( CheckBox box, Dictionary<string, string> v, string key ) {
		if ( v.TryGetValue( key, out var s ) && bool.TryParse( s, out bool b ) ) box.Checked = b;
	}

	static void SetIndex( ComboBox box, Dictionary<string, string> v, string key ) {
		if ( v.TryGetValue( key, out var s ) && int.TryParse( s, out int i )
			&& i >= 0 && i < box.Items.Count ) box.SelectedIndex = i;
	}

	// The bot marker colour as the game wants it: "r g b", each 0..255. A bad
	// or empty field falls back to the magenta default.
	static int Clamp255( int x ) => x < 0 ? 0 : x > 255 ? 255 : x;

	static Color ParseBotColor( string s ) {
		var p = s.Split( new[] { ' ' }, StringSplitOptions.RemoveEmptyEntries );
		if ( p.Length == 3 && int.TryParse( p[0], out int r )
			&& int.TryParse( p[1], out int g ) && int.TryParse( p[2], out int b ) ) {
			return Color.FromArgb( Clamp255( r ), Clamp255( g ), Clamp255( b ) );
		}
		return Color.FromArgb( 255, 0, 220 );
	}

	void PickBotColor() {
		using var dlg = new ColorDialog { FullOpen = true, Color = ParseBotColor( botColor.Text ) };
		if ( dlg.ShowDialog( this ) == DialogResult.OK ) {
			botColor.Text = $"{dlg.Color.R} {dlg.Color.G} {dlg.Color.B}";
		}
	}

	// SetNum nimmt nur NumericUpDown, und dessen Wert ist decimal - ein
	// Schieber braucht einen eigenen. Das Begrenzen ist nicht Zierde: ein von
	// Hand geaenderter Wert ausserhalb des Bereichs wirft beim Setzen.
	static void SetBar( TrackBar bar, Dictionary<string, string> v, string key ) {
		if ( v.TryGetValue( key, out var s ) && int.TryParse( s, out int i ) ) {
			bar.Value = Math.Clamp( i, bar.Minimum, bar.Maximum );
		}
	}

	static void SetNum( NumericUpDown box, Dictionary<string, string> v, string key ) {
		if ( v.TryGetValue( key, out var s )
			&& decimal.TryParse( s, System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out decimal d )
			&& d >= box.Minimum && d <= box.Maximum ) {
			box.Value = d;
		}
	}

	string BuildConfig() {
		var cfg = new StringBuilder();
		cfg.AppendLine( "// von der Trefferton-App geschrieben, wird bei jedem Start ueberschrieben" );
		cfg.AppendLine( $"seta cl_hitSound {hitSound.SelectedIndex}" );
		cfg.AppendLine( $"seta cl_hitSoundFile \"{hitSoundFile.Text.Replace( '\\', '/' )}\"" );
		cfg.AppendLine( $"seta cl_hitPitch {( hitPitch.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_hitPitchFull {pitchFull.Value.ToString( System.Globalization.CultureInfo.InvariantCulture )}" );
		cfg.AppendLine( $"seta cl_hitPitchEmpty {pitchEmpty.Value.ToString( System.Globalization.CultureInfo.InvariantCulture )}" );
		cfg.AppendLine( $"seta cl_hitPitchKill {pitchKill.Value.ToString( System.Globalization.CultureInfo.InvariantCulture )}" );
		cfg.AppendLine( $"seta cl_hitPitchStack {(int)pitchStack.Value}" );
		cfg.AppendLine( "seta cl_hitSoundDebug 1" );
		cfg.AppendLine( "seta g_hitSoundDebug 1" );
		cfg.AppendLine( $"seta cl_aimAssist {( aimAssist.Checked ? (int)aimStrength.Value : 0 )}" );
		cfg.AppendLine( $"seta cl_itemOutline {( itemOutline.Checked ? ( itemOutlineAll.Checked ? 2 : 1 ) : 0 )}" );
		// immer geschrieben, auch bei abgeschalteten Kaesten: die Variable wird
		// archiviert, und ein hier ausgelassener Wert liesse einen alten aus
		// der q3config stehen - der Schieber wuerde dann etwas anderes zeigen,
		// als das Spiel wirklich benutzt
		cfg.AppendLine( $"seta cl_itemOutlineRange {itemRange.Value}" );
		cfg.AppendLine( $"seta cl_botOutline {( botOutline.Checked ? ( botDamage.Checked ? 2 : 1 ) : 0 )}" );
		cfg.AppendLine( $"seta cl_botOutlineStyle {botStyle.SelectedIndex}" );
		cfg.AppendLine( $"seta cl_botOutlineBars {botBars.SelectedIndex}" );
		{
			// Write the parsed value back into the field: otherwise a typo stays
			// on screen and gets saved, while the game quietly runs the default.
			var c = ParseBotColor( botColor.Text );
			botColor.Text = $"{c.R} {c.G} {c.B}";
			cfg.AppendLine( $"seta cl_botOutlineColor \"{c.R} {c.G} {c.B}\"" );
		}
		cfg.AppendLine( $"seta cl_botOutlineName {( botName.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_damagePlums {damagePlums.SelectedIndex}" );
		// com_maxfps ist archiviert, das Spiel merkt es sich also - deshalb nur
		// schreiben, wenn wirklich eine Bildrate gewaehlt wurde
		if ( maxFps.SelectedIndex > 0 && maxFps.SelectedIndex < MaxFpsChoices.Length ) {
			cfg.AppendLine( $"set com_maxfps {MaxFpsChoices[maxFps.SelectedIndex]}" );
		}
		cfg.AppendLine( $"seta cl_aimAssistAttacker {( aimAssist.Checked && aimAttacker.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistFreeze {( aimAssist.Checked && aimFreeze.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistEdge {( aimEdge.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistDebug {( aimAssist.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistKey \"{aimKey.Text.Replace( "\"", "" )}\"" );
		cfg.AppendLine( $"seta cl_aimAssistSmooth {(int)aimSmooth.Value}" );
		cfg.AppendLine( $"seta cl_aimAssistLead {Dec( aimLead.Value )}" );
		cfg.AppendLine( $"seta cl_aimAssistExact {aimExact.SelectedIndex}" );
		cfg.AppendLine( $"seta cl_aimAssistLearn {( aimAssist.Checked && aimLearn.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistHoldFire {( aimHoldFire.Checked ? 1 : 0 )}" );
		// Der Punkt muss ein Punkt bleiben, die Engine liest mit atof
		var lottery = ( holdLottery.Value / 10.0 ).ToString( "0.0",
			System.Globalization.CultureInfo.InvariantCulture );
		cfg.AppendLine( $"seta cl_aimAssistHoldLottery {lottery}" );
		cfg.AppendLine( $"seta cl_autoSwitchEmpty {( autoSwitch.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_autoSwitchEmptyOrder \"{autoSwitchOrder.Text}\"" );
		cfg.AppendLine( $"seta cl_aimAssistPriority \"{PriorityString()}\"" );
		// Leer, mit Absicht: die Waffenlisten stehen jetzt in aimprio.cfg, und
		// ein alter Wert aus der q3config wuerde die Datei sonst ueberstimmen,
		// weil die Engine die Variable zuletzt liest.
		cfg.AppendLine( "seta cl_aimAssistPriorityWeapon \"\"" );
		cfg.AppendLine( "set logfile 2" );
		cfg.AppendLine( "set bot_nochat 1" );
		// Die Engine begrenzt die Zielhilfe selbst auf localhost und privates LAN.
		// Menschliche Testziele sind zusaetzlich ein ausdruecklicher App-Haken.
		// Die Waffen, auf die alle Sockel und Munitionskisten verteilt werden.
		// Leer heisst: die Karte bleibt, wie sie ist - und das ist auch der
		// Fall, wenn alles angehakt ist, denn dann gibt es nichts zu ersetzen.
		// Kein seta: eine Laboreinstellung hat in der q3config des Spielers
		// nichts verloren, wo sie beim naechsten Spiel ohne dieses Werkzeug
		// still weiterwirken wuerde. Muss vor "map" stehen, weil das Spiel sie
		// beim Entstehen der Gegenstaende liest.
		// Aus: Raketen- und BFG-Spruenge tragen wie immer, tun aber nicht weh.
		// Kein seta - eine Laboreinstellung gehoert nicht in die q3config.
		cfg.AppendLine( $"set g_selfDamage {( noSelfDamage.Checked ? 0 : 1 )}" );
		// Nachladezeit und Munition. Ebenfalls kein seta, und ebenfalls vor
		// "map": die Nachladezeit liest das Spiel zwar bei jedem Schuss neu,
		// aber die Zielhilfe stempelt sie beim ersten Schnappschuss ins
		// Protokoll - steht sie dann schon, ist die Sitzung von Anfang an
		// richtig beschriftet.
		cfg.AppendLine( $"set g_weaponRate {weaponRate.Value}" );
		cfg.AppendLine( $"set g_infiniteAmmo {infiniteAmmo.SelectedIndex}" );
		cfg.AppendLine( $"set g_weaponSpawns \"{SpawnList()}\"" );
		cfg.AppendLine( $"map {map.Text}" );
		cfg.AppendLine( "wait 200" );

		for ( int i = 0; i < (int)bots.Value; i++ ) {
			cfg.AppendLine( $"addbot {BotNames[i % BotNames.Length]} {(int)skill.Value}" );
			cfg.AppendLine( "wait 20" );
		}

		return cfg.ToString();
	}

	// Das Spiel kann sein Protokoll nur neu schreiben, nie anhaengen, also ist
	// die Runde von vorhin weg, sobald die naechste beginnt. Eine ganze
	// Sitzung ging so schon verloren, bevor sie ausgewertet war. Sie wandert
	// jetzt mit ihrem Datum in einen Unterordner, und nur die juengsten
	// zwanzig bleiben liegen, damit der Ordner nicht ins Kraut schiesst.
	const int KeepLogs = 20;

	/*
	Eine gemessene Tabelle beiseitelegen, damit sauber von vorn gemessen werden
	kann - nach einem Umbau an der Zielhilfe zum Beispiel, wo die alten Proben
	etwas anderes gemessen haben als die neuen.

	Beiseite und nicht weg: die Datei wandert mit ihrem Datum nach baseq3\logs\,
	genau wie ein abgelaufenes Protokoll. Ein Abend Messung ist zu teuer, um ihn
	an einen Fehlklick zu verlieren, und zurueckholen ist dann ein Kopiervorgang.

	Waehrend das Spiel laeuft geht es nicht, und das ist kein Vorsichtsakt
	sondern Arithmetik: die Tabelle steht im Speicher des Spiels und wird von
	dort alle fuenfzehn Sekunden herausgeschrieben. Die Datei zu entfernen
	brachte also gar nichts - fuenfzehn Sekunden spaeter stuende sie wieder da,
	mit allen alten Proben.
	*/
	static bool GameRunning() {
		foreach ( var name in new[] { "ioquake3", "ioquake3.x86_64", "ioq3ded" } ) {
			try {
				if ( Process.GetProcessesByName( name ).Length > 0 ) return true;
			} catch ( InvalidOperationException ) {
			}
		}
		return false;
	}

	// Die Probenzahl steht in beiden gemessenen Dateien als letztes von sieben
	// Feldern - in aimtune.cfg hinter sum/weight/square, in aimrate.cfg hinter
	// shots/hits/landShots/landHits. Alles nach // ist Beiwerk.
	static int CountSamples( string path ) {
		int total = 0;
		try {
			foreach ( var raw in File.ReadAllLines( path ) ) {
				var line = raw;
				int remark = line.IndexOf( "//", StringComparison.Ordinal );
				if ( remark >= 0 ) line = line[..remark];
				var f = line.Split( new[] { ' ', '\t' }, StringSplitOptions.RemoveEmptyEntries );
				if ( f.Length == 7 && int.TryParse( f[6], out int n ) ) total += n;
			}
		} catch ( IOException ) {
		} catch ( UnauthorizedAccessException ) {
		}
		return total;
	}

	void ResetTable( string file, string what, string afterwards ) {
		string path = Path.Combine( HomePath, file );
		if ( !File.Exists( path ) ) {
			MessageBox.Show( this, $"{what} gibt es noch nicht – es wurde noch nichts gemessen.",
				"Nichts zurückzusetzen", MessageBoxButtons.OK, MessageBoxIcon.Information );
			return;
		}

		if ( GameRunning() ) {
			MessageBox.Show( this,
				"Das Spiel läuft gerade.\n\n"
				+ "Die Tabelle steht in seinem Speicher und wird von dort alle fünfzehn "
				+ "Sekunden herausgeschrieben – Zurücksetzen brächte also nichts, sie stünde "
				+ "gleich wieder da. Bitte das Spiel beenden und es dann noch einmal versuchen.",
				"Spiel läuft", MessageBoxButtons.OK, MessageBoxIcon.Warning );
			return;
		}

		int samples = CountSamples( path );
		var answer = MessageBox.Show( this,
			$"{what} zurücksetzen?\n\n"
			+ $"{samples} gemessene Proben gehen damit aus der laufenden Messung heraus. "
			+ "Gelöscht wird nichts: die Datei wandert mit ihrem Datum nach baseq3\\logs\\ "
			+ "und lässt sich von dort zurückkopieren.\n\n"
			+ afterwards,
			"Messung zurücksetzen", MessageBoxButtons.YesNo, MessageBoxIcon.Question,
			MessageBoxDefaultButton.Button2 );		// Enter verwirft nichts
		if ( answer != DialogResult.Yes ) return;

		try {
			var attic = Path.Combine( HomePath, "logs" );
			Directory.CreateDirectory( attic );
			var stamp = File.GetLastWriteTime( path ).ToString( "yyyyMMdd-HHmmss" );
			var target = Path.Combine( attic,
				Path.GetFileNameWithoutExtension( file ) + "-" + stamp + ".cfg" );
			if ( File.Exists( target ) ) File.Delete( target );
			File.Move( path, target );

			// der Zwischenspeicher der Trefferquote haengt am Datum der Datei,
			// und die gibt es gerade nicht mehr
			rateRead = DateTime.MinValue;
			rateCache = new Dictionary<string, Cell[]>();
			rateStamp = null;
			RefreshStats();

			status.Text = $"{what} zurückgesetzt – liegt als {Path.GetFileName( target )} in logs\\";
		} catch ( IOException e ) {
			MessageBox.Show( this, $"Ging nicht: {e.Message}", "Zurücksetzen fehlgeschlagen",
				MessageBoxButtons.OK, MessageBoxIcon.Error );
		} catch ( UnauthorizedAccessException e ) {
			MessageBox.Show( this, $"Ging nicht: {e.Message}", "Zurücksetzen fehlgeschlagen",
				MessageBoxButtons.OK, MessageBoxIcon.Error );
		}
	}

	void ArchiveLog() {
		try {
			if ( !File.Exists( logPath ) || new FileInfo( logPath ).Length == 0 ) return;

			var attic = Path.Combine( HomePath, "logs" );
			Directory.CreateDirectory( attic );
			var stamp = File.GetLastWriteTime( logPath ).ToString( "yyyyMMdd-HHmmss" );
			var target = Path.Combine( attic, $"qconsole-{stamp}.log" );
			if ( File.Exists( target ) ) File.Delete( target );
			File.Move( logPath, target );

			foreach ( var old in new DirectoryInfo( attic ).GetFiles( "qconsole-*.log" )
				.OrderByDescending( f => f.LastWriteTime ).Skip( KeepLogs ) ) {
				try { old.Delete(); } catch ( IOException ) { }
			}
		} catch ( IOException ) {
			// haengt noch ein Spiel daran, bleibt es eben stehen
		} catch ( UnauthorizedAccessException ) {
		}
	}

	// Bildschirm und Aufloesung gehoeren auf die Kommandozeile, nicht in die
	// Config: das Spiel liest +set noch vor dem Start des Renderers, waehrend
	// ein exec erst laeuft, wenn das Fenster laengst steht - dort gesetzt
	// braeuchte es ein vid_restart. Nicht Gewaehltes wird weggelassen, damit
	// die Einstellung des Spiels unangetastet bleibt.
	string VideoArguments() {
		var args = new StringBuilder();

		if ( screenMode.SelectedIndex == 1 ) args.Append( "+set r_fullscreen 0 " );
		else if ( screenMode.SelectedIndex == 2 ) args.Append( "+set r_fullscreen 1 " );

		int i = screenSize.SelectedIndex;
		if ( i > 0 && i < Resolutions.Length ) {
			var r = Resolutions[i];
			args.Append( $"+set r_mode -1 +set r_customwidth {r.W} +set r_customheight {r.H} " );
		}

		return args.ToString();
	}

	void StartGame() {
		var exe = Path.Combine( gameDir.Text, "ioquake3.exe" );
		if ( !File.Exists( exe ) ) {
			MessageBox.Show( this, $"{exe} gibt es nicht.\n\nBitte den Ordner wählen, in dem ioquake3.exe liegt.",
				"Spiel nicht gefunden", MessageBoxButtons.OK, MessageBoxIcon.Warning );
			return;
		}

		try {
			Directory.CreateDirectory( HomePath );
			File.WriteAllText( Path.Combine( HomePath, CfgName ), BuildConfig() );
			WriteWeaponPriorityFile();

			logPath = Path.Combine( HomePath, "qconsole.log" );
			ArchiveLog();
			shotStamp = "";
			aimLearned.Text = "";

			game = Process.Start( new ProcessStartInfo {
				FileName = exe,
				Arguments = VideoArguments() + $"+exec {CfgName}",
				WorkingDirectory = gameDir.Text,
				UseShellExecute = true,
			} );

			// Ein Start ist der Moment, in dem die Einstellung gemeint war:
			// sie ging gerade als Config an das Spiel, also gehoert sie auch
			// in die eigene Datei, und nicht nur dorthin.
			SaveSettings();
			status.Text = "gestartet, Mitschrift läuft";
			status.ForeColor = Color.ForestGreen;
		} catch ( Exception ex ) {
			MessageBox.Show( this, ex.Message, "Start fehlgeschlagen", MessageBoxButtons.OK, MessageBoxIcon.Error );
		}
	}

	// Das Spiel schreibt die Debug-Zeilen in qconsole.log, hier werden sie nur gelesen
	void RefreshStats() {
		// Ob das Spiel gerade zu Ende ist; sein Protokoll wird dann noch einmal
		// ganz gelesen, bevor der gelernte Vorhalt festgehalten wird
		bool exited;
		try { exited = game is { HasExited: true }; } catch ( InvalidOperationException ) { exited = false; }

		// Vor dem Protokoll, weil die Trefferquote nicht daran haengt: sie
		// steht in einer eigenen Datei und ueberlebt jedes Aufraeumen der
		// Protokolle.
		UpdateRates();

		if ( logPath.Length == 0 || !File.Exists( logPath ) ) {
			// ein Spiel, das zu Ende ist, ohne je ein Protokoll geschrieben zu
			// haben, darf kein spaeteres, von Hand gestartetes beglaubigen
			if ( exited ) game = null;
			return;
		}

		string text;
		try {
			using var stream = new FileStream( logPath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite );
			using var reader = new StreamReader( stream );
			text = reader.ReadToEnd();
		} catch ( IOException ) {
			return;		// das Spiel schreibt gerade, beim naechsten Mal wieder
		}

		int hits = 0, sounds = 0, holds = 0, heldMs = 0;
		var holdReason = new Dictionary<string, int>();
		var frames = new HashSet<string>();
		int frameRun = 0, lastDamageFrame = -1;		// Kartenwechsel trennen, siehe unten
		var recent = new List<string>();
		var damageFrames = new List<Damage>();
		var shots = new List<Shot>();
		var impacts = new List<Impact>();
		var missiles = new List<Missile>();
		var tunes = new Dictionary<string, Tune>();
		var corrections = new List<Correction>();
		Learn? pending = null;
		int segment = 0, clockFrom = 0, clockLast = 0;
		var learned = "";
		var stamp = "";
		// Nachladezeit und Munition koennen mitten in einer Sitzung umgestellt
		// werden; das Spiel stempelt dann neu. Wer die Zeilen davor und danach in
		// einen Topf wirft, vergleicht zwei Bedingungen und nennt es ein Ergebnis.
		var conditions = new HashSet<string>();

		foreach ( var line in text.Split( '\n' ) ) {
			var trimmed = line.TrimEnd( '\r' );

			if ( trimmed.StartsWith( "hit on " ) ) {
				hits++;
				// Treffer im selben Server-Frame beantwortet das Spiel mit einem Ton,
				// deshalb zaehlen die Frames und nicht die einzelnen Schadensereignisse
				var mark = trimmed.LastIndexOf( " frame ", StringComparison.Ordinal );
				if ( mark >= 0 && int.TryParse( trimmed[( mark + 7 )..], out int damageFrame ) ) {
					// Die Frame-Nummer faengt bei jedem Kartenwechsel wieder vorn an.
					// Ohne eigenen Abschnitt fielen zwei Treffer aus verschiedenen
					// Runden auf denselben Schluessel und zaehlten als einer - was
					// die Zahl der erwarteten Toene zu klein macht, also ausgerechnet
					// in Richtung "alles in Ordnung".
					if ( damageFrame < lastDamageFrame ) {
						frameRun++;
					}
					lastDamageFrame = damageFrame;
					frames.Add( frameRun + ":" + damageFrame );

					// wer getroffen wurde, steht zwischen "hit on " und dem Doppelpunkt
					var colon = trimmed.IndexOf( ':', 7 );
					damageFrames.Add( new Damage {
						Frame = damageFrame,
						Victim = colon > 7 ? trimmed[7..colon] : "",
					} );
				} else {
					frames.Add( "#" + hits );
				}
				recent.Add( trimmed );
			} else if ( trimmed.StartsWith( "hit sound: " ) ) {
				var rest = trimmed[11..];
				// "playing ..." und "counter resync ..." sind keine abgespielten Toene
				if ( rest.Length > 0 && !rest.StartsWith( "playing" ) && !rest.StartsWith( "counter" ) ) {
					sounds++;
					recent.Add( trimmed );
				}
			} else if ( trimmed.StartsWith( "aim shot: " ) ) {
				var shot = Shot.Parse( trimmed );
				if ( shot is not null ) shots.Add( shot );
			} else if ( trimmed.StartsWith( "aim impact: " ) ) {
				var impact = Impact.Parse( trimmed );
				if ( impact is not null ) impacts.Add( impact );
			} else if ( trimmed.StartsWith( "aim missile: " ) ) {
				var missile = Missile.Parse( trimmed );
				if ( missile is not null ) missiles.Add( missile );
			} else if ( trimmed.StartsWith( "aim hold: " ) ) {
				// "... released after N ms frame M" schliesst eine Sperre ab,
				// jede andere Zeile oeffnet eine
				var f = trimmed.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
				int at = Array.IndexOf( f, "after" );
				if ( at >= 0 && at + 1 < f.Length && int.TryParse( f[at + 1], out int ms ) ) {
					heldMs += ms;
				} else {
					holds++;
					var reason = string.Join( " ", f.Skip( 3 ).TakeWhile( x => x != "at" ) );
					if ( reason.Length > 0 ) {
						holdReason[reason] = holdReason.GetValueOrDefault( reason ) + 1;
					}
				}
			} else if ( trimmed.StartsWith( "aim learn: " ) ) {
				learned = trimmed;
				// Der Partner steht in der naechsten Zeile. Steht dort etwas
				// anderes, gehoert diese hier zu nichts und faellt weg.
				pending = Learn.Parse( trimmed[11..] );
				continue;
			} else if ( trimmed.StartsWith( "aim log: " ) ) {
				stamp = trimmed;
				conditions.Add( StampCondition( trimmed ) );
			} else if ( trimmed.StartsWith( "aim tune: " ) || trimmed.StartsWith( "aim table: " ) ) {
				bool fromLearn = trimmed[4] == 't' && trimmed[5] == 'u';
				var tune = Tune.Parse( trimmed[( fromLearn ? 10 : 11 )..] );
				// je Waffe und Flugzeitband zaehlt der zuletzt gemessene Stand
				if ( tune is not null ) tunes[tune.Weapon + "|" + tune.Band + "|" + tune.Pace] = tune;

				// Eine Korrektur ist das Paar aus beiden Zeilen. Der Abzug der
				// ganzen Tabelle heisst "aim table:" und ist keine.
				if ( fromLearn && tune is not null && pending is not null
					&& pending.Weapon == tune.Weapon ) {
					// Eine neue Runde stellt die Serveruhr zurueck; ab da laeuft
					// die Uhr in der Liste wieder von vorn. Verglichen wird mit
					// der vorigen Korrektur und nicht mit dem Anfang der Runde:
					// die Uhr faellt beim Kartenwechsel von neunhunderttausend
					// auf vierzigtausend, was immer noch weit ueber dem Anfang
					// liegt - so blieb ein Wechsel unbemerkt und zwei Zeilen
					// hintereinander lasen 14:50 und 0:32.
					if ( clockLast > 0 && tune.Frame < clockLast ) {
						segment++;
						clockFrom = tune.Frame;
					}
					if ( clockFrom == 0 ) clockFrom = tune.Frame;
					clockLast = tune.Frame;
					corrections.Add( new Correction {
						What = pending, Box = tune,
						Segment = segment, Since = tune.Frame - clockFrom,
					} );
				}
			}
			pending = null;
		}

		ShowLogVersion( stamp, conditions.Count );

		// Wie oft der Abzug gesperrt wurde und wie lange insgesamt. Ohne diese
		// Zeile war nicht zu unterscheiden, ob die Sperre nie zugriff oder ob
		// sie zugriff und man es nur nicht merkte.
		// Kurz halten: die Zeile ist die siebte Zahl in einer Reihe, die nicht
		// umbricht. Die Aufschlüsselung steht im Tooltip.
		if ( holds == 0 ) {
			statHold.Text = "–";
			holdTip.SetToolTip( statHold, "Der Abzug wurde nie gesperrt." );
		} else {
			statHold.Text = $"{holds}× / {heldMs} ms";
			holdTip.SetToolTip( statHold, string.Join( "\n", holdReason
				.OrderByDescending( x => x.Value ).Select( x => $"{x.Key}: {x.Value}" ) ) );
		}

		UpdateShots( shots, damageFrames, impacts, missiles );
		UpdateTune( tunes );
		UpdateHistory( corrections );
		ShowLearned( learned );

		// Das Spiel ist zu Ende und sein Protokoll gelesen: was es an Vorhalt
		// gelernt hat, festhalten, sonst waere die Optimierung beim naechsten
		// Start der App wieder weg
		if ( exited ) {
			game = null;
		}

		// Positiv heisst ein Treffer ohne Ton, negativ ein Ton zu viel. Vorher
		// war das auf null geklemmt: ein doppelter Ton war damit unsichtbar, und
		// die gruene Null stand auch dann da, wenn ueberhaupt noch nichts
		// gemessen war - sie las sich wie ein Beweis, dass alles stimmt.
		int missed = frames.Count - sounds;
		statHits.Text = hits.ToString();
		statFrames.Text = frames.Count.ToString();
		statSounds.Text = sounds.ToString();
		if ( frames.Count == 0 && sounds == 0 ) {
			statMissed.Text = "–";
			statMissed.ForeColor = Color.DimGray;
		} else if ( missed > 0 ) {
			statMissed.Text = missed.ToString();
			statMissed.ForeColor = Color.Firebrick;
		} else if ( missed < 0 ) {
			statMissed.Text = $"{-missed}× doppelt";
			statMissed.ForeColor = Color.Firebrick;
		} else {
			statMissed.Text = "0";
			statMissed.ForeColor = Color.ForestGreen;
		}

		var tail = string.Join( Environment.NewLine, recent.TakeLast( 200 ) );
		if ( logView.Text != tail ) {
			logView.Text = tail;
			logView.SelectionStart = logView.TextLength;
			logView.ScrollToCaret();
		}
	}

	static void ReadPoint( string[] f, int at, double[] point ) {
		for ( int k = 0; k < 3 && at + k < f.Length; k++ ) {
			double.TryParse( f[at + k], System.Globalization.CultureInfo.InvariantCulture, out point[k] );
		}
	}

	// Ein Einschlag, wie ihn der Client im Snapshot sieht, samt der Bots in
	// dem Moment - daraus wird die Fehlweite
	sealed class Impact {
		public string Kind = "";
		public int Num, Other, Client, Frame;
		public double[] At = new double[3];
		public Dictionary<string, double[]> Bots = new();

		public static Impact? Parse( string line ) {
			// aim impact: rail num 12 other 3 client 0 at 1 2 3 frame 555 | Sarge 10 20 30 air 0 | ...
			var parts = line.Split( " | " );
			var f = parts[0].Split( ' ', StringSplitOptions.RemoveEmptyEntries );
			var impact = new Impact();
			var ok = false;

			for ( int i = 0; i < f.Length - 1; i++ ) {
				switch ( f[i] ) {
					case "impact:": impact.Kind = f[i + 1]; break;
					case "num": int.TryParse( f[i + 1], out impact.Num ); break;
					case "other": int.TryParse( f[i + 1], out impact.Other ); break;
					case "client": int.TryParse( f[i + 1], out impact.Client ); break;
					case "at": ReadPoint( f, i + 1, impact.At ); break;
					case "frame": ok = int.TryParse( f[i + 1], out impact.Frame ); break;
				}
			}

			for ( int p = 1; p < parts.Length; p++ ) {
				var b = parts[p].Split( ' ', StringSplitOptions.RemoveEmptyEntries );
				if ( b.Length < 4 ) continue;
				var pos = new double[3];
				ReadPoint( b, 1, pos );
				impact.Bots[b[0]] = pos;
			}

			return ok ? impact : null;
		}
	}

	// Eine Rakete beim ersten Auftauchen: die Nummer verbindet sie mit ihrem Einschlag
	sealed class Missile {
		public int Num, Frame;
		public double[] At = new double[3];

		public static Missile? Parse( string line ) {
			var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
			var missile = new Missile();
			var ok = false;

			for ( int i = 0; i < f.Length - 1; i++ ) {
				switch ( f[i] ) {
					case "num": int.TryParse( f[i + 1], out missile.Num ); break;
					case "at": ReadPoint( f, i + 1, missile.At ); break;
					case "frame": ok = int.TryParse( f[i + 1], out missile.Frame ); break;
				}
			}

			return ok ? missile : null;
		}
	}

	static double Distance( double[] a, double[] b ) =>
		Math.Sqrt( ( a[0] - b[0] ) * ( a[0] - b[0] ) + ( a[1] - b[1] ) * ( a[1] - b[1] ) + ( a[2] - b[2] ) * ( a[2] - b[2] ) );

	static bool IsProjectile( string weapon ) =>
		weapon is "rocket" or "grenade" or "plasma" or "bfg" or "hook" or "nailgun" or "prox";

	// Eine Schadensmeldung des Servers
	sealed class Damage {
		public int Frame;
		public string Victim = "";
	}

	// Ob das Protokoll von einem Spiel stammt, dessen Zeilen dieses Werkzeug
	// kennt. Ohne Stempel ist es aelter als diese Pruefung.
	// Die Bedingung, unter der eine Sitzung lief: Nachladezeit und Munition.
	// Fehlen sie, ist das Protokoll aelter als diese Felder.
	static string StampCondition( string line ) {
		var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
		string rate = "", ammo = "";
		for ( int i = 0; i < f.Length - 1; i++ ) {
			if ( f[i] == "rate" ) rate = f[i + 1];
			else if ( f[i] == "ammo" ) ammo = f[i + 1];
		}
		return rate + "/" + ammo;
	}

	void ShowLogVersion( string line, int conditions = 1 ) {
		if ( line.Length == 0 ) {
			logVersion.Text = "Protokoll ohne Fassungsangabe – älter als dieses Werkzeug";
			logVersion.ForeColor = Color.DarkGoldenrod;
			return;
		}

		var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
		int found = 0;
		string built = "", rate = "", ammo = "";
		for ( int i = 0; i < f.Length - 1; i++ ) {
			if ( f[i] == "version" ) int.TryParse( f[i + 1], out found );
			else if ( f[i] == "built" && i + 3 < f.Length ) built = $"{f[i + 1]} {f[i + 2]} {f[i + 3]}";
			else if ( f[i] == "rate" ) rate = f[i + 1];
			else if ( f[i] == "ammo" ) ammo = f[i + 1];
		}

		// Unter welcher Bedingung gespielt wurde. Nur nennen, wenn sie vom
		// Normalfall abweicht - sonst steht auf jeder Zeile eine Null-Aussage.
		var how = "";
		if ( rate.Length > 0 && rate != "100" && rate != "0" ) how += $", Nachladezeit {rate} %";
		if ( ammo == "1" ) how += ", Munition unbegrenzt (nur ich)";
		else if ( ammo == "2" ) how += ", Munition unbegrenzt (alle)";

		if ( conditions > 1 ) {
			logVersion.Text = $"Protokoll Fassung {found}, Spiel vom {built}{how} – ACHTUNG: {conditions}"
				+ " verschiedene Bedingungen in einer Datei, die Zahlen unten mischen sie";
			logVersion.ForeColor = Color.Firebrick;
		} else if ( found == LogVersion ) {
			logVersion.Text = $"Protokoll Fassung {found}, Spiel vom {built}{how}";
			logVersion.ForeColor = how.Length > 0 ? Color.DarkGoldenrod : Color.DimGray;
		} else {
			logVersion.Text = found < LogVersion
				? $"Protokoll Fassung {found} – dieses Werkzeug erwartet {LogVersion}, bitte das Spiel neu bauen"
				: $"Protokoll Fassung {found} – neuer als dieses Werkzeug ({LogVersion}), bitte die App neu bauen";
			logVersion.ForeColor = Color.Firebrick;
		}
	}

	// Die letzte Zeile "aim learn:" - wie viele Schuesse bisher gemessen wurden.
	// Der Regler daneben wird davon nicht mehr bewegt: er gibt dem Vorhalt seine
	// Form und bleibt die Vorgabe des Benutzers, waehrend das Gemessene je Waffe
	// und Flugzeit in der Karte "pro Waffe" steht. Ein einzelner gelernter Wert
	// haette eine Korrektur von einer Entfernung auf alle anderen uebertragen.
	void ShowLearned( string line ) {
		if ( line.Length == 0 ) return;

		var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
		int count = 0;
		for ( int i = 0; i < f.Length - 1; i++ ) {
			if ( f[i] == "n" ) int.TryParse( f[i + 1], out count );
		}
		if ( count <= 0 ) return;

		aimLearned.Text = $"{count} Schüsse gemessen – siehe „pro Waffe“";
	}

	// Eine Zeile "aim shot:" aus dem Protokoll
	sealed class Shot {
		public int Frame, Lead, Distance;
		public int Me = -1;				// eigene Client-Nummer, um Einschlaege zuzuordnen
		public int World = -1;			// Server-Frame, gegen den der Schuss lief (neuere Protokolle)
		public bool InAir;
		public bool Assisted = true;	// aeltere Protokolle kennen das Feld nicht
		public double Error;
		public double Swing;			// wie weit die Sicht bis zum Schuss kommen musste
		public int Land = -1;			// ms bis zur Landung des Ziels, -1 = bleibt in der Luft
		public double Pace;				// Tempo des Ziels, die zweite Achse der Tabelle
		public double MySpeed;			// eigenes Tempo
		public double[] Eye = new double[3], Plain = new double[3];
		public string Weapon = "", Target = "", Position = "";

		public static Shot? Parse( string line ) {
			// aim shot: rocket target Sarge dist 612 air 1 lead 680 error 0.42 at 10 20 30 frame 16900
			var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
			var shot = new Shot();
			var ok = false;

			for ( int i = 0; i < f.Length - 1; i++ ) {
				switch ( f[i] ) {
					case "shot:": shot.Weapon = f[i + 1]; break;
					case "target": shot.Target = f[i + 1]; break;
					case "dist": int.TryParse( f[i + 1], out shot.Distance ); break;
					case "air": shot.InAir = f[i + 1] == "1"; break;
				case "assist": shot.Assisted = f[i + 1] == "1"; break;
				case "me": int.TryParse( f[i + 1], out shot.Me ); break;
				case "pace": double.TryParse( f[i + 1], System.Globalization.CultureInfo.InvariantCulture, out shot.Pace ); break;
				case "myspeed": double.TryParse( f[i + 1], System.Globalization.CultureInfo.InvariantCulture, out shot.MySpeed ); break;
				case "world": int.TryParse( f[i + 1], out shot.World ); break;
				case "eye": ReadPoint( f, i + 1, shot.Eye ); break;
				case "plain": ReadPoint( f, i + 1, shot.Plain ); break;
					case "lead": int.TryParse( f[i + 1], out shot.Lead ); break;
					case "error":
						double.TryParse( f[i + 1], System.Globalization.CultureInfo.InvariantCulture, out shot.Error );
						break;
					// Wie weit die Sicht kommen musste. Bei geschnappten Waffen
					// ist "error" bauartbedingt null, "swing" sagt dort alles.
					case "swing":
						double.TryParse( f[i + 1], System.Globalization.CultureInfo.InvariantCulture, out shot.Swing );
						break;
					// Wann die Fuesse des Ziels wieder aufkommen sollten, in ms,
					// oder -1 wenn es beim Einschlag noch in der Luft ist.
					// nur bei Erfolg zuweisen: TryParse schreibt sonst eine Null
					// in den Ausgabewert und macht aus "keine Landung" ein
					// "landet sofort"
					case "land":
						if ( int.TryParse( f[i + 1], out int land ) ) shot.Land = land;
						break;
					case "at":
						if ( i + 3 < f.Length ) shot.Position = $"{f[i + 1]} {f[i + 2]} {f[i + 3]}";
						break;
					// "cmd" ist der Takt des Befehls, "frame" hiess dasselbe in
					// Fassung 4 und frueher. Auf jeder anderen Zeilenart meint
					// "frame" dagegen den Snapshot - deshalb der neue Name.
					case "cmd":
					case "frame": ok = int.TryParse( f[i + 1], out shot.Frame ); break;
				}
			}

			return ok ? shot : null;
		}
	}

	// Ein gemeldeter Schaden gehoert genau einem Schuss: dem ersten, der auf
	// dasselbe Ziel ging und dessen Flugzeit passt. Sonst schreibt ein Treffer
	// jedem Schuss gut, dessen Fenster ihn zufaellig enthaelt.
	// Wo der eigene Schuss wirklich eingeschlagen ist: bei Hitscan der erste
	// Einschlag danach, den der Server mir zuschreibt (Kugeln nennen den
	// Schuetzen in "other", die Rail in "client"); bei Geschossen die Rakete,
	// die gleich nach dem Abzug neben meinem Auge auftaucht, und dann ihr
	// Einschlag unter derselben Nummer.
	static Impact? FindImpact( Shot shot, List<Impact> impacts, List<Missile> missiles ) {
		// Das Ereignis eines Schusses steht im naechsten Snapshot nach dem
		// Server-Frame, gegen den er lief; ohne "world" bleibt das weite Fenster
		int start = shot.World >= 0 ? shot.World : shot.Frame;
		int span = shot.World >= 0 ? 100 : 300;

		if ( IsProjectile( shot.Weapon ) ) {
			var mine = missiles.FirstOrDefault( m => m.Frame >= start && m.Frame <= start + 200
				&& Distance( m.At, shot.Eye ) < 150 );
			if ( mine is null ) return null;
			return impacts.FirstOrDefault( x => x.Num == mine.Num && x.Frame >= mine.Frame
				&& x.Kind.StartsWith( "missile" ) );
		}

		return impacts.FirstOrDefault( x => x.Frame >= start && x.Frame <= start + span
			&& ( x.Kind == "rail" ? x.Client == shot.Me : ( !x.Kind.StartsWith( "missile" ) && x.Other == shot.Me ) ) );
	}

	// Fehlweite und Richtung: wie weit das Ziel neben der Schusslinie (Auge bis
	// Einschlag) lag, gemessen auf Hoehe des Ziels. Ein Fehlschuss fliegt
	// vorbei und schlaegt irgendwo dahinter ein - der Einschlag selbst sagt
	// nichts, der Abstand der Linie zum Ziel dagegen alles. "kurz" heisst, der
	// Schuss ist vor dem Ziel im Boden oder einer Wand geblieben.
	// Wo das Ziel stand, als der Schuss aufgeloest wurde. Die Bot-Liste einer
	// Einschlagzeile ist dafuer einen Server-Frame zu spaet: das Ereignis kommt
	// erst mit dem naechsten Snapshot an. Bei Hitscan steht die richtige
	// Stellung ohnehin in der Schusszeile - "plain" ist das Ziel im Frame, gegen
	// den der Befehl lief, und stimmt damit auf die Einheit. Bei Geschossen
	// zaehlt der Ankunftszeitpunkt, also die Liste eines Frames davor.
	static double[]? ResolvePosition( Shot shot, Impact impact, List<Impact> impacts ) {
		if ( !IsProjectile( shot.Weapon ) && shot.World >= 0 ) return shot.Plain;

		var earlier = impacts.FirstOrDefault( x => x.Frame == impact.Frame - 50
			&& x.Bots.ContainsKey( shot.Target ) );
		if ( earlier is not null ) return earlier.Bots[shot.Target];

		return impact.Bots.TryGetValue( shot.Target, out var late ) ? late : null;
	}

	static string DescribeMiss( Shot shot, Impact impact, List<Impact> impacts, out double units ) {
		units = 0;
		var bot = ResolvePosition( shot, impact, impacts );
		if ( bot is null ) return "";

		double ux = impact.At[0] - shot.Eye[0], uy = impact.At[1] - shot.Eye[1], uz = impact.At[2] - shot.Eye[2];
		double len = Math.Sqrt( ux * ux + uy * uy + uz * uz );
		if ( len < 1 ) return "";
		ux /= len; uy /= len; uz /= len;

		// Koerpermitte des Ziels, und ihr naechster Punkt auf der Linie
		double tx = bot[0] - shot.Eye[0], ty = bot[1] - shot.Eye[1], tz = bot[2] + 8 - shot.Eye[2];
		double along = tx * ux + ty * uy + tz * uz;
		double ox = tx - ux * along, oy = ty - uy * along, oz = tz - uz * along;
		units = Math.Sqrt( ox * ox + oy * oy + oz * oz );

		// rechts von der Schusslinie aus gesehen
		double rx = uy, ry = -ux, rl = Math.Sqrt( rx * rx + ry * ry );
		if ( rl < 0.001 ) { rx = 1; ry = 0; } else { rx /= rl; ry /= rl; }
		double side = ox * rx + oy * ry;			// Ziel rechts der Linie: Schuss ging links

		var parts = new List<string>();
		if ( len < along - 30 ) parts.Add( "kurz" );
		if ( Math.Abs( side ) > 16 ) parts.Add( side > 0 ? "links" : "rechts" );
		if ( Math.Abs( oz ) > 16 ) parts.Add( oz > 0 ? "tief" : "hoch" );
		return parts.Count > 0 ? string.Join( "+", parts ) : "dran";
	}

	// Eine Zeile "aim tune:" - was das Spiel fuer eine Waffe in einem
	// Flugzeitband ueber sich selbst gemessen hat
	sealed class Tune {
		public string Weapon = "";
		public int Band, Pace, Samples, Frame;
		public double From, Above, Factor, Scatter, Reach;
		// Das Fach selbst, vor und nach diesem Schuss. Nur die Korrekturzeile
		// traegt sie; der Abzug der ganzen Tabelle nicht, denn dort hat sich
		// nichts bewegt. Minus eins heisst: steht nicht auf dieser Zeile.
		public double Was = -1, Now = -1;

		public static Tune? Parse( string payload ) {
			var f = payload.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
			if ( f.Length < 3 ) return null;

			var t = new Tune { Weapon = f[0], Scatter = -1 };
			for ( int i = 1; i < f.Length - 1; i++ ) {
				switch ( f[i] ) {
				case "band": int.TryParse( f[i + 1], out t.Band ); break;
				case "pace": int.TryParse( f[i + 1], out t.Pace ); break;
				case "samples": int.TryParse( f[i + 1], out t.Samples ); break;
				case "from": t.From = Num( f[i + 1] ); break;
				case "above": t.Above = Num( f[i + 1] ); break;
				case "factor": t.Factor = Num( f[i + 1] ); break;
				case "scatter": t.Scatter = Num( f[i + 1] ); break;
				case "reach": t.Reach = Num( f[i + 1] ); break;
				case "was": t.Was = Num( f[i + 1] ); break;
				case "now": t.Now = Num( f[i + 1] ); break;
				case "frame": int.TryParse( f[i + 1], out t.Frame ); break;
				}
			}
			return t.Samples > 0 ? t : null;
		}

		static double Num( string s ) =>
			double.TryParse( s, System.Globalization.NumberStyles.Any,
				System.Globalization.CultureInfo.InvariantCulture, out double v ) ? v : 0;
	}

	// Eine Zeile "aim learn:" - ein einzelner nachgeregelter Schuss. Die Zeile
	// darunter, "aim tune:", sagt in welches Fach er ging und wohin der Wert
	// dieses Faches sich dadurch bewegt hat; die beiden gehoeren zusammen.
	sealed class Learn {
		public string Weapon = "", Target = "";
		public int Index, Frame;		// Index: laufende Nummer des Schusses, KEIN Fach-Zaehler
		public double Ran, Straight, Expected, Aside, Error, Weight, Pace, MySpeed, Hold;

		// Genau die Groesse, die die Engine quadriert in das Fach legt: wie
		// weit der Schuss am Ende danebenlag, laengs und quer zusammen.
		public double Missed => Math.Sqrt( ( Ran - Expected ) * ( Ran - Expected ) + Aside * Aside );

		public static Learn? Parse( string payload ) {
			var f = payload.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
			if ( f.Length < 3 ) return null;

			var l = new Learn { Weapon = f[0] };
			for ( int i = 1; i < f.Length - 1; i++ ) {
				switch ( f[i] ) {
				case "target": l.Target = f[i + 1]; break;
				case "ran": l.Ran = Num( f[i + 1] ); break;
				case "of": l.Straight = Num( f[i + 1] ); break;
				case "expected": l.Expected = Num( f[i + 1] ); break;
				case "aside": l.Aside = Num( f[i + 1] ); break;
				case "error": l.Error = Num( f[i + 1] ); break;
				case "weight": l.Weight = Num( f[i + 1] ); break;
				case "pace": l.Pace = Num( f[i + 1] ); break;
				case "myspeed": l.MySpeed = Num( f[i + 1] ); break;
				case "hold": l.Hold = Num( f[i + 1] ); break;
				case "learned": int.TryParse( f[i + 1], out l.Index ); break;
				case "frame": int.TryParse( f[i + 1], out l.Frame ); break;
				}
			}
			return l.Index > 0 ? l : null;
		}

		static double Num( string s ) =>
			double.TryParse( s, System.Globalization.NumberStyles.Any,
				System.Globalization.CultureInfo.InvariantCulture, out double v ) ? v : 0;
	}

	sealed class Correction {
		public Learn What = null!;
		public Tune Box = null!;
		public int Segment;			// welcher Abschnitt des Abends, fuer die Uhr
		public int Since;			// ms seit dessen Anfang

		// Wie weit sich das Fach bewegt hat. Das ist der Eintrag im Hauptbuch -
		// nicht der verblendete Wert, den die Zielhilfe fuer diesen einen
		// Schuss gegeben haette.
		// Ein aelteres Protokoll kennt die beiden Felder nicht. Dann steht hier
		// nichts - und nicht etwa null, was wie "nichts bewegt" aussaehe.
		public bool Known => Box.Was >= 0 && Box.Now >= 0;
		public double Moved => Known ? Box.Now - Box.Was : 0;
	}

	// Die Tabelle, die das Spiel ueber sich selbst fuehrt: pro Waffe und
	// Flugzeit, wie viel vom Vorhalt wirklich eintrifft und wie weit das
	// Ergebnis danach noch streut. Die Streuung gegen den Wirkradius sagt,
	// ob der Schuss auf die Entfernung ueberhaupt zu machen ist.
	// Die Spalten sollen die Fensterbreite mitnehmen. Was beim Anlegen als
	// Breite dasteht, gilt dabei als Verhaeltnis: eine breite Spalte bekommt
	// von jeder zusaetzlichen Breite entsprechend mehr ab.
	void FitColumns( ListView view ) {
		var columns = view.Columns.Cast<ColumnHeader>().ToArray();
		var weights = columns.Select( c => c.Tag is int t ? t : c.Width ).ToArray();
		int total = weights.Sum();
		int room = view.ClientSize.Width - 4;
		if ( total <= 0 || room < 120 ) return;

		// Enger als die eigene Ueberschrift wird keine Spalte - lieber quer
		// scrollen als zwoelf Spalten, die alle "F..." heissen
		var least = columns.Select( ColumnLeast ).ToArray();

		// Jede gesetzte Breite zeichnet die Liste neu; gebuendelt ist es eine
		view.BeginUpdate();
		int used = 0;
		for ( int i = 0; i < columns.Length - 1; i++ ) {
			int w = Math.Max( least[i], room * weights[i] / total );
			if ( columns[i].Width != w ) columns[i].Width = w;
			used += w;
		}
		int rest = Math.Max( least[^1], room - used );
		if ( columns[^1].Width != rest ) columns[^1].Width = rest;
		view.EndUpdate();
	}

	int ColumnLeast( ColumnHeader column ) {
		if ( !columnLeast.TryGetValue( column, out int least ) ) {
			least = TextRenderer.MeasureText( column.Text, column.ListView?.Font ?? Font ).Width + 22;
			columnLeast[column] = least;
		}
		return least;
	}

	// Sammeln statt sofort rechnen: waehrend am Fenster gezogen wird, bleibt der
	// Wecker stehen und wird immer wieder neu gestellt.
	void QueueFit( ListView view ) {
		fitPending.Add( view );
		fitTimer.Stop();
		fitTimer.Start();
	}

	void QueueBands() {
		fitBandsPending = true;
		fitTimer.Stop();
		fitTimer.Start();
	}

	void FlushFits() {
		foreach ( var view in fitPending ) {
			// Eine Liste auf einer geschlossenen Karte hat keine Breite, mit der
			// sich rechnen laesst; sie wird nachgezogen, sobald ihre Karte kommt
			if ( view.IsHandleCreated && view.Visible ) FitColumns( view );
		}
		fitPending.Clear();

		if ( fitBandsPending ) {
			fitBandsPending = false;
			if ( rateView.IsHandleCreated && rateView.Visible ) FitBands();
		}
	}

	void FitOnResize( ListView view ) {
		foreach ( ColumnHeader column in view.Columns ) column.Tag = column.Width;
		view.Resize += ( _, _ ) => QueueFit( view );
		view.VisibleChanged += ( _, _ ) => { if ( view.Visible ) QueueFit( view ); };
		FitColumns( view );
	}

	// Welche Zeile gewaehlt ist, damit sie einen Neuaufbau ueberlebt: sonst
	// verliert man sie alle halbe Sekunde, sobald das Protokoll weiterlaeuft
	static string? SelectedKey( ListView view ) => view.SelectedItems.Count > 0
		? ( view.SelectedItems[0].Tag as string ?? view.SelectedItems[0].Text ) : null;

	static void Reselect( ListView view, string? key ) {
		if ( key is null ) return;
		foreach ( ListViewItem row in view.Items ) {
			if ( ( row.Tag as string ?? row.Text ) == key ) { row.Selected = true; return; }
		}
	}

	void UpdateTune( Dictionary<string, Tune> tunes ) {
		var rows = new List<ListViewItem>();
		int samples = 0;

		foreach ( var t in tunes.Values.OrderBy( t => t.Weapon ).ThenBy( t => t.Band ).ThenBy( t => t.Pace ) ) {
			samples += t.Samples;

			string outlook;
			Color colour;
			if ( t.Scatter < 0 ) {
				outlook = "misst noch";
				colour = Color.DimGray;
			} else if ( t.Scatter < t.Reach * 0.5 ) {
				outlook = "sicher";
				colour = Color.ForestGreen;
			} else if ( t.Scatter < t.Reach ) {
				outlook = "brauchbar";
				colour = Color.DarkGoldenrod;
			} else {
				outlook = "Lotterie";
				colour = Color.Firebrick;
			}

			var row = new ListViewItem( t.Weapon ) {
				ForeColor = colour, Tag = t.Weapon + "|" + t.Band + "|" + t.Pace,
			};
			row.SubItems.Add( t.From.ToString( "0.0", System.Globalization.CultureInfo.InvariantCulture ) + " s" );
			row.SubItems.Add( t.Above.ToString( "0" ) + " u/s" );
			row.SubItems.Add( t.Samples.ToString() );
			row.SubItems.Add( t.Factor.ToString( "0.00", System.Globalization.CultureInfo.InvariantCulture ) );
			row.SubItems.Add( t.Scatter < 0 ? "—" : t.Scatter.ToString( "0" ) );
			row.SubItems.Add( t.Reach.ToString( "0" ) );
			row.SubItems.Add( outlook );
			rows.Add( row );
		}

		statTuneBoxes.Text = rows.Count.ToString();
		statTuneSamples.Text = samples.ToString();

		// nur neu zeichnen, wenn sich wirklich etwas geaendert hat
		var stamp = string.Join( ";", rows.Select( r => r.Tag + ":" + r.SubItems[3].Text + ":" + r.SubItems[5].Text ) );
		if ( stamp == tuneStamp ) return;
		tuneStamp = stamp;

		var keep = SelectedKey( tuneView );
		tuneView.BeginUpdate();
		tuneView.Items.Clear();
		tuneView.Items.AddRange( rows.ToArray() );
		Reselect( tuneView, keep );
		tuneView.EndUpdate();
	}

	const int RankBarColumn = 3;
	const int HistBarColumn = 9;

	string histStamp = "";
	bool histUpdating;			// die Waffenliste wird gerade gefuellt, nicht gewaehlt

	/*
	Welche Nachregelung wann gemacht wurde, eine Zeile je Schuss, neueste oben.

	Eine Zeile ist das Paar aus "aim learn:" und der "aim tune:" darunter. Die
	Spalte "Fach" ist der eigentliche Eintrag: der Wert des Faches vor und nach
	diesem Schuss. Frueher stand dort der verblendete Wert an genau diesem
	Vorhalt, und der ist etwas anderes - er mischt bis zu vier Faecher, und
	darum summierten sich die einzelnen Korrekturen auch nicht zur Bewegung des
	Faches auf, in einem Topf sogar mit umgekehrtem Vorzeichen. Seit die Engine
	"was" und "now" mitschreibt, ist die Spalte das, wonach sie aussieht.

	Gemessen wird nur, was fliegt. Hitscan-Waffen haben keine Flugzeit und
	lehren diese Tabelle nichts - in viereinhalb Megabyte Protokoll stammen
	alle 359 Proben von der Rakete, gegen 2743 Schuesse mit dem Maschinengewehr,
	die null ergaben. Darum steht das auch so da, wenn die Liste leer ist:
	sonst liest sie sich wie ein Fehler.
	*/
	void UpdateHistory( List<Correction> corrections ) {
		string pick = histWeapon.SelectedItem as string ?? "alle";

		// Die Auswahl bietet nur an, was auch vorkommt. Verglichen wird der
		// Inhalt und nicht die Anzahl: nach einem Protokollwechsel koennen
		// genauso viele Waffen vorkommen wie vorher und trotzdem andere, und
		// dann stuende die Liste auf einer Waffe, die es nicht mehr gibt.
		var seen = corrections.Select( c => c.What.Weapon ).Distinct().OrderBy( w => w ).ToList();
		var have = histWeapon.Items.Cast<string>().Skip( 1 ).ToList();
		if ( histWeapon.Items.Count == 0 || !have.SequenceEqual( seen ) ) {
			// Das Fuellen loest SelectedIndexChanged aus, und dessen Behandler
			// ruft RefreshStats - also mitten in RefreshStats hinein, mit einem
			// zweiten vollstaendigen Durchlauf ueber ein megabytegrosses
			// Protokoll. Solange hier gebaut wird, schweigt er.
			histUpdating = true;
			try {
				histWeapon.Items.Clear();
				histWeapon.Items.Add( "alle" );
				foreach ( var w in seen ) histWeapon.Items.Add( w );
				histWeapon.SelectedItem = histWeapon.Items.Contains( pick ) ? pick : "alle";
			} finally {
				histUpdating = false;
			}
			pick = histWeapon.SelectedItem as string ?? "alle";
		}

		var shown = pick == "alle" ? corrections
			: corrections.Where( c => c.What.Weapon == pick ).ToList();

		// Die Zaehler zaehlen alles, gezeichnet werden die letzten fuenfhundert.
		// Vorher liefen beide ueber dieselbe gekuerzte Schleife, und dann stand
		// oben "Korrekturen: 800" ueber drei Zahlen, die sich zu 500 addierten.
		int up = 0, down = 0, flat = 0;
		foreach ( var c in shown ) {
			if ( !c.Known ) continue;
			if ( c.Moved >= 0.005 ) up++;
			else if ( c.Moved <= -0.005 ) down++;
			else flat++;
		}

		int rounds = shown.Count == 0 ? 0 : shown.Max( c => c.Segment ) + 1;
		var rows = new List<ListViewItem>();
		foreach ( var c in Enumerable.Reverse( shown ).Take( 500 ) ) {
			double moved = c.Moved;
			var row = new ListViewItem( c.What.Index.ToString() ) { Tag = moved };
			// Die Uhr laeuft je Runde; die Nummer steht nur davor, wenn es
			// mehr als eine gab. Ohne sie folgt in der Liste auf 0:32 ploetzlich
			// 14:50, und das sieht nach einem Fehler aus statt nach einem
			// Kartenwechsel.
			row.SubItems.Add( ( rounds > 1 ? $"{c.Segment + 1} · " : "" )
				+ $"{c.Since / 60000}:{c.Since / 1000 % 60:00}" );
			row.SubItems.Add( c.What.Weapon );
			row.SubItems.Add( BoxName( c.Box ) );
			row.SubItems.Add( c.What.Target );
			row.SubItems.Add( $"{c.What.Expected:0} u" );
			row.SubItems.Add( $"{c.What.Ran:0} u" );
			row.SubItems.Add( $"{c.What.Missed:0} u" );
			row.SubItems.Add( c.Known ? $"{c.Box.Was:0.00} → {c.Box.Now:0.00}" : "—" );
			row.SubItems.Add( !c.Known ? "—"
				: Math.Abs( moved ) < 0.005 ? "±0,00" : $"{moved:+0.00;−0.00}" );
			row.SubItems.Add( WhyText( c.What ) );

			// Wie weit der Schuss am Ende danebenlag, nach denselben Schwellen
			// gefaerbt, die die Engine selbst zum Gewichten benutzt
			var reach = c.Box.Reach > 0 ? c.Box.Reach : 120;
			row.SubItems[7].ForeColor = c.What.Missed < reach * 0.5 ? Color.ForestGreen
				: c.What.Missed < reach ? Color.DarkGoldenrod : Color.Firebrick;
			row.UseItemStyleForSubItems = false;
			rows.Add( row );
		}

		statFixes.Text = shown.Count.ToString();
		statFixUp.Text = up.ToString();
		statFixDown.Text = down.ToString();
		statFixFlat.Text = flat.ToString();

		if ( rows.Count == 0 ) {
			histLast.Text = "";
			histEmpty.Text = corrections.Count == 0
				? "Noch nichts nachgeregelt.\n\n"
					+ "Gemessen wird nur, was fliegt: Rakete, Granate, Plasma, BFG, Enterhaken.\n"
					+ "Hitscan-Waffen – Maschinengewehr, Railgun, Schrotflinte, Blitzwerfer –\n"
					+ "haben keine Flugzeit und lehren die Tabelle nichts."
				: $"Mit dieser Waffe wurde nichts nachgeregelt.";
			histEmpty.Visible = true;
			histView.Visible = false;
		} else {
			var top = shown[^1];
			histLast.Text = $"zuletzt #{top.What.Index} · {top.What.Weapon} · {BoxName( top.Box )}"
				+ $" · {top.What.Target} lief {top.What.Ran:0} statt {top.What.Expected:0} Einheiten"
				+ $" – {WhyText( top.What )}"
				+ ( top.Known ? $" · {top.Box.Was:0.00} → {top.Box.Now:0.00}" : "" );
			histEmpty.Visible = false;
			histView.Visible = true;
		}

		var stamp = rows.Count + "|" + pick + "|" + histLast.Text;
		if ( stamp == histStamp ) return;
		histStamp = stamp;

		var keep = SelectedKey( histView );
		histView.BeginUpdate();
		histView.Items.Clear();
		histView.Items.AddRange( rows.ToArray() );
		Reselect( histView, keep );
		histView.EndUpdate();
	}

	// Die Flugzeitbaender der Vorhalte-Tabelle. Muss zu CL_AimAssistBandStart
	// in code/client/cl_input.c passen, so wie RateWeapons zu
	// CL_AimAssistWeaponName - die untere Kante kommt zwar als "from" auf der
	// Zeile mit, die obere aber nicht, und die stand hier vorher als nackte
	// Zahl mitten im Ausdruck.
	static readonly double[] BandStart = { 0.0, 0.4, 0.8, 1.3 };

	static string BoxName( Tune t ) {
		string when = t.Band >= BandStart.Length - 1 ? $"ab {t.From:0.0} s"
			: $"{t.From:0.0}–{BandStart[t.Band + 1]:0.0} s";
		return when + ( t.Pace == 0 ? " · langsam" : " · schnell" );
	}

	// Warum dieser Schuss das Fach bewegt hat, in der Sprache der Sache. Die
	// Reihenfolge ist die der Engine: die beiden Anschlaege zuerst, denn sie
	// bedeuten etwas anderes als ein zu langer oder zu kurzer Vorhalt.
	static string WhyText( Learn l ) {
		string why = l.Error <= -0.999 ? "Ziel kehrte um"
			: l.Error >= 0.999 ? "Ziel zog davon"
			: l.Error <= -0.05 ? "zu weit geführt"
			: l.Error >= 0.05 ? "zu kurz geführt"
			: "Vorhalt saß";
		return l.Weight < 0.6 ? why + " · viel seitwärts" : why;
	}

	/*
	Die Trefferquote je Waffe und Entfernung.

	Gelesen wird baseq3/aimrate.cfg und nicht das Protokoll: die Engine
	schreibt die Datei alle fuenfzehn Sekunden, also steht dort der laufende
	Stand, ohne dass cl_aimAssistDebug an sein muss und ohne auf das Ende des
	Spiels zu warten. Die Datei traegt Waffennummern, keine Namen, darum die
	Tabelle darunter - sie muss zu CL_AimAssistWeaponName in
	code/client/cl_input.c passen, so wie WeaponDefault zu aimWeaponDefault.

	Die Grenzen 500/1000/1500/2000 stehen in CL_AimAssistRange und stecken in
	dem, was ein Fach bedeutet. Verschieben sie sich dort, gehoert die
	Formatnummer der Datei hochgezaehlt und die Beschriftung hier nachgezogen -
	beides, und im selben Commit. Einmal ist die Nummer bewusst stehen
	geblieben, naemlich fuer die fuenfzig Einheiten von 1450 auf 1500; warum,
	steht bei AIM_RATE_FORMAT in cl_input.c und nicht hier.
	*/
	static readonly string[] RateWeapons = {
		"none", "gauntlet", "machinegun", "shotgun", "grenade", "rocket",
		"lightning", "railgun", "plasma", "bfg", "hook",
	};
	static readonly string[] RangeNames = {
		"bis 500", "500–1000", "1000–1500", "1500–2000", "ab 2000",
	};
	// Die beiden Schranken der Engine, AIM_RATE_SPEAK und AIM_RATE_FIRM. Sie
	// kommen aus dem Wilson-Intervall bei p = 0,5: bei acht Proben faellt die
	// halbe Breite zum ersten Mal unter 30 Punkte, bei fuenfundzwanzig unter
	// 18 - und 18 Punkte ist der Abstand, ab dem sich zwei Nachbarfaecher
	// ueberhaupt unterscheiden lassen.
	const int RateSpeak = 8;
	const int RateFirm = 25;
	// Waffe, dann "am besten", dann erst die fuenf Faecher
	const int RateFirstBand = 2;

	// Wird auch bei jedem Auffrischen gerufen und nicht nur beim Groessern:
	// diese Karteikarte ist beim Start nicht die gewaehlte, und eine
	// Karteikarte, die noch nie zu sehen war, hat ihre Groesse noch nicht -
	// die Spalten standen darum einmal so breit, dass die letzten beiden
	// Entfernungen rechts aus dem Fenster fielen.
	void FitBands() {
		if ( rateView.Columns.Count < RateFirstBand + RangeNames.Length ) return;

		// ClientSize schliesst die senkrechte Bildlaufleiste schon aus; die
		// vier Pixel sind der Rahmen, den die Liste selbst noch braucht.
		int room = rateView.ClientSize.Width - 4
			- rateView.Columns[0].Width - rateView.Columns[1].Width;
		int each = room / RangeNames.Length;
		if ( each < 90 ) return;			// zu schmal: lieber quer scrollen als unlesbar

		for ( int i = 0; i < RangeNames.Length; i++ ) {
			// der Rest geht an das letzte Fach, sonst bleibt rechts eine Luecke
			int want = i == RangeNames.Length - 1
				? room - each * ( RangeNames.Length - 1 ) : each;
			var column = rateView.Columns[RateFirstBand + i];
			if ( column.Width != want ) column.Width = want;
		}
	}

	sealed class Cell {
		public double Shots, Hits, LandShots, LandHits;
		public int Samples;
		public double Share => Shots > 0 ? Hits / Shots : 0;
		public double LandShare => LandShots > 0 ? LandHits / LandShots : 0;

		// Kein Probenzaehler, sondern ein gealtertes Gewicht - die Datei fuehrt
		// fuer die aufsetzenden Schuesse keine ungealterte Zahl. Bis etwa
		// hundert Proben laeuft es fast mit: bei acht echten steht hier 7,7,
		// bei fuenfzig 39. Darueber laeuft es auseinander, aber die Schranke
		// darunter liegt bei acht, und da stimmt es noch. Der Name sagt das,
		// damit niemand es mit Samples verwechselt.
		public double LandWeight => LandShots;
	}

	// Die halbe Breite des Wilson-Intervalls in Punkten, damit neben einer
	// duennen Zahl steht, wie duenn sie ist. Ein Mittelwert braucht dafuer
	// keinen eigenen Rechenweg, ein Zaehler schon: bei acht Proben sind es
	// 28 Punkte, bei fuenfundzwanzig 18, bei hundertzwanzig 9.
	static double WilsonHalf( double p, int n ) {
		if ( n <= 0 ) return 100;
		const double z = 1.96;
		double denom = 1 + z * z / n;
		double half = z / denom * Math.Sqrt( p * ( 1 - p ) / n + z * z / ( 4.0 * n * n ) );
		return half * 100;
	}

	Dictionary<string, Cell[]> rateCache = new();
	DateTime rateRead = DateTime.MinValue;

	Dictionary<string, Cell[]> ReadRates() {
		var table = new Dictionary<string, Cell[]>();
		string path = Path.Combine( HomePath, "aimrate.cfg" );
		string[] lines;
		try {
			if ( !File.Exists( path ) ) return table;

			// Das Spiel schreibt die Datei alle fuenfzehn Sekunden, gelesen
			// wird zweimal pro Sekunde: neunundzwanzig von dreissig Durchgaengen
			// wuerden dieselben Bytes noch einmal zerlegen - und dabei jedes Mal
			// mit dem Schreiber des Spiels um die Datei streiten, weswegen
			// ueberhaupt FileShare und der Fang darunter dastehen.
			var written = File.GetLastWriteTimeUtc( path );
			if ( written == rateRead ) return rateCache;

			using var stream = new FileStream( path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite );
			using var reader = new StreamReader( stream );
			lines = reader.ReadToEnd().Split( '\n' );
			rateRead = written;
		} catch ( IOException ) {
			return rateCache;		// das Spiel schreibt gerade
		}

		foreach ( var raw in lines ) {
			var line = raw.Trim();
			// Alles nach // ist Beiwerk, und "format 1" ist keine Zeile mit
			// Zahlen darin. Stimmt die Fassung nicht, ist die Datei von einem
			// Bau mit anderen Grenzen und wird ganz verworfen.
			int remark = line.IndexOf( "//", StringComparison.Ordinal );
			if ( remark >= 0 ) line = line[..remark].Trim();
			if ( line.Length == 0 ) continue;
			if ( line.StartsWith( "format", StringComparison.Ordinal ) ) {
				if ( line != "format 1" ) return rateCache = new Dictionary<string, Cell[]>();
				continue;
			}

			var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
			if ( f.Length < 7 ) continue;
			if ( !int.TryParse( f[0], out int weapon ) || !int.TryParse( f[1], out int range ) ) continue;
			if ( weapon <= 0 || weapon >= RateWeapons.Length || range < 0 || range >= RangeNames.Length ) continue;

			var name = RateWeapons[weapon];
			if ( !table.TryGetValue( name, out var cells ) ) {
				cells = new Cell[RangeNames.Length];
				for ( int i = 0; i < cells.Length; i++ ) cells[i] = new Cell();
				table[name] = cells;
			}
			var cell = cells[range];
			cell.Shots = Num( f[2] );
			cell.Hits = Num( f[3] );
			cell.LandShots = Num( f[4] );
			cell.LandHits = Num( f[5] );
			int.TryParse( f[6], out cell.Samples );
		}

		return rateCache = table;
	}

	static double Num( string s ) =>
		double.TryParse( s, System.Globalization.NumberStyles.Any,
			System.Globalization.CultureInfo.InvariantCulture, out double v ) ? v : 0;

	// Null heisst "noch nie gezeichnet, also unbedingt neu aufbauen". Eine
	// leere Zeichenkette taugt dafuer nicht: die leere Tabelle hat selbst den
	// leeren Stempel, und dann sah das Zuruecksetzen wie "nichts geaendert"
	// aus - der Zaehler oben sprang auf "–", die Liste blieb auf den alten
	// Zahlen stehen.
	string? rateStamp;

	void UpdateRates() {
		FitBands();
		var table = ReadRates();
		var rows = new List<ListViewItem>();
		string best = "–";
		double bestShare = -1;

		// Die Reihenfolge ist die der Rangliste: erst die Waffe mit der besten
		// Quote ueber alles. Zwei Karteikarten, die dieselben Waffen anders
		// sortieren, lesen sich wie zwei verschiedene Messungen.
		foreach ( var entry in table.OrderByDescending( e => {
					double shots = e.Value.Sum( c => c.Shots );
					return shots > 0 ? e.Value.Sum( c => c.Hits ) / shots : -1;
				} ).ThenByDescending( e => e.Value.Sum( c => c.Shots ) ) ) {
			var cells = entry.Value;
			var row = new ListViewItem( WeaponName( entry.Key ) ) { Tag = cells };

			int firmBand = -1, worstBand = -1, firmCount = 0;
			double firmShare = -1, worstShare = 2;
			var texts = new string[cells.Length];
			for ( int i = 0; i < cells.Length; i++ ) {
				var cell = cells[i];
				if ( cell.Samples == 0 ) {
					texts[i] = "—";
				} else if ( cell.Samples < RateSpeak ) {
					texts[i] = $"misst noch ({cell.Samples})";
				} else if ( cell.Samples < RateFirm ) {
					texts[i] = $"{cell.Share * 100:0} % ±{WilsonHalf( cell.Share, cell.Samples ):0}";
				} else {
					texts[i] = $"{cell.Share * 100:0} % ({cell.Samples})";
					firmCount++;
					if ( cell.Share < worstShare ) { worstShare = cell.Share; worstBand = i; }
					if ( cell.Share > firmShare ) { firmShare = cell.Share; firmBand = i; }
				}
			}

			// Nur ein Fach mit genug Proben darf "am besten" heissen, sonst
			// gewinnt regelmaessig das Fach mit drei Schuessen darin.
			//
			// Und eine Waffe, die ueberall gleich gut trifft, bekommt keine
			// Lieblingsentfernung angedichtet. Ueberschneiden sich die
			// Vertrauensbereiche des besten und des schlechtesten Faches, ist
			// der Unterschied keiner - das trifft die Railgun, und dass sie
			// flach ist, ist der nuetzlichste Satz in dieser Tabelle.
			string bestText = "—";
			if ( firmBand >= 0 ) {
				bool flat = firmCount >= 2 && worstBand >= 0
					&& firmShare * 100 - WilsonHalf( firmShare, cells[firmBand].Samples )
						< worstShare * 100 + WilsonHalf( worstShare, cells[worstBand].Samples );
				bestText = flat ? "überall" : RangeNames[firmBand];
			}
			row.SubItems.Add( bestText );
			foreach ( var text in texts ) row.SubItems.Add( text );
			rows.Add( row );

			if ( firmBand >= 0 && firmShare > bestShare ) {
				bestShare = firmShare;
				best = $"{WeaponName( entry.Key )}, {bestText}";
			}
		}

		statBestRange.Text = best;

		var stamp = string.Join( ";", rows.Select( r => r.Text + ":"
			+ string.Join( ",", r.SubItems.Cast<ListViewItem.ListViewSubItem>().Select( s => s.Text ) ) ) );
		if ( rateStamp is not null && stamp == rateStamp ) return;
		rateStamp = stamp;

		var keep = SelectedKey( rateView );
		rateView.BeginUpdate();
		rateView.Items.Clear();
		rateView.Items.AddRange( rows.ToArray() );
		Reselect( rateView, keep );
		rateView.EndUpdate();
	}

	static string WeaponName( string key ) {
		foreach ( var w in Weapons ) {
			if ( w.Key == key ) return w.Name;
		}
		return key;
	}

	// Was in einem Fach steckt, wenn die Maus darauf steht. Der zweite,
	// duenne Balken hat keine Beschriftung - seine Zahl steht hier.
	string rateTipShown = "";

	void ShowRateTip( Point at ) {
		var hit = rateView.HitTest( at );
		int band = hit.Item is null || hit.SubItem is null
			? -1 : hit.Item.SubItems.IndexOf( hit.SubItem ) - RateFirstBand;
		if ( band < 0 || band >= RangeNames.Length || hit.Item?.Tag is not Cell[] cells ) {
			if ( rateTipShown.Length > 0 ) { rateTip.SetToolTip( rateView, "" ); rateTipShown = ""; }
			return;
		}

		var cell = cells[band];
		string text;
		if ( cell.Samples == 0 ) {
			text = $"{hit.Item.Text}, {RangeNames[band]}\nAuf diese Entfernung wurde noch nicht geschossen.";
		} else {
			text = $"{hit.Item.Text}, {RangeNames[band]}\n"
				+ $"{cell.Share * 100:0} % von {cell.Shots:0} gewerteten Schüssen"
				+ $" (±{WilsonHalf( cell.Share, cell.Samples ):0} Punkte)\n"
				+ $"{cell.Samples} Proben insgesamt, ältere zählen weniger";
			if ( cell.LandWeight >= RateSpeak ) {
				text += $"\nund {cell.LandShare * 100:0} %, wenn das Ziel im Flug aufsetzt"
					+ $" ({cell.LandShots:0} Schüsse)";
			} else if ( cell.LandShots >= 1 ) {
				text += "\nzu wenige Schüsse auf ein aufsetzendes Ziel, um das zu trennen";
			}
		}

		if ( text == rateTipShown ) return;
		rateTipShown = text;
		rateTip.SetToolTip( rateView, text );
	}

	// Ampel fuer eine Quote: was trifft, was geht so, was geht daneben
	static Color RateColour( double share ) {
		if ( share >= 0.7 ) return Color.FromArgb( 130, 195, 130 );
		if ( share >= 0.4 ) return Color.FromArgb( 235, 205, 115 );
		return Color.FromArgb( 228, 140, 130 );
	}

	// Was eine Waffe ueber die Sitzung geleistet hat
	sealed class Rank {
		public string Weapon = "";
		public int Shots, Hits, MissCount;
		public double MissSum;
		public double Share => Shots > 0 ? (double)Hits / Shots : 0;
	}

	// Welche Waffe am besten trifft, der Reihe nach. Die Quote bekommt einen
	// Balken, damit der Abstand zwischen den Waffen ins Auge faellt; die
	// Fehlweite daneben sagt, ob eine schwache Quote am Zielen liegt oder
	// daran, dass die Waffe auf die Entfernung nichts ausrichtet.
	void UpdateRanking( Dictionary<string, Rank> ranks ) {
		var order = ranks.Values.OrderByDescending( r => r.Share ).ThenByDescending( r => r.Shots ).ToList();
		var rows = new List<ListViewItem>();

		foreach ( var r in order ) {
			var row = new ListViewItem( r.Weapon ) { Tag = r.Share };
			row.SubItems.Add( r.Shots.ToString() );
			row.SubItems.Add( r.Hits.ToString() );
			row.SubItems.Add( ( 100 * r.Share ).ToString( "0" ) + " %" );
			row.SubItems.Add( r.MissCount > 0 ? ( r.MissSum / r.MissCount ).ToString( "0" ) : "—" );
			rows.Add( row );
		}

		// Eine Waffe mit drei Schuessen ist kein Sieger, nur ein Zufall
		var best = order.FirstOrDefault( r => r.Shots >= 5 ) ?? order.FirstOrDefault();
		statBestWeapon.Text = best is null ? "–" : $"{best.Weapon} {100 * best.Share:0} %";

		var stamp = string.Join( ";", order.Select( r => $"{r.Weapon}:{r.Shots}:{r.Hits}:{r.MissCount}" ) );
		if ( stamp == rankStamp ) return;
		rankStamp = stamp;

		var keep = SelectedKey( rankView );
		rankView.BeginUpdate();
		rankView.Items.Clear();
		rankView.Items.AddRange( rows.ToArray() );
		Reselect( rankView, keep );
		rankView.EndUpdate();
	}

	void UpdateShots( List<Shot> shots, List<Damage> damageFrames, List<Impact> impacts, List<Missile> missiles ) {
		int hit = 0, assisted = 0, assistedHit = 0, unassisted = 0, unassistedHit = 0;
		double errorSum = 0;
		var rows = new List<ListViewItem>();
		var ranks = new Dictionary<string, Rank>();
		var claimed = new bool[damageFrames.Count];

		foreach ( var shot in shots ) {
			// Ein Hitscan-Schuss trifft genau im Server-Frame, gegen den sein
			// Befehl lief - dem "world"-Frame der Zeile; so bekommt bei einer
			// Zeile pro Kugel jede Kugel nur ihren eigenen Treffer. Ein Geschoss
			// kommt nach dem Vorhalt an, plus dem Spielraum eines ausweichenden
			// Ziels. Aeltere Protokolle ohne "world" behalten das weite Fenster.
			int from = shot.Frame, until = shot.Frame + shot.Lead + 300;
			if ( shot.World >= 0 ) {
				from = shot.World;
				until = IsProjectile( shot.Weapon ) ? shot.World + shot.Lead + 400 : shot.World;
			}
			bool landed = false;

			for ( int i = 0; i < damageFrames.Count; i++ ) {
				var damage = damageFrames[i];
				if ( claimed[i] || damage.Victim != shot.Target ) continue;
				if ( damage.Frame < from || damage.Frame > until ) continue;

				claimed[i] = true;
				landed = true;
				break;
			}

			if ( landed ) hit++;
			errorSum += shot.Error;

			var impact = FindImpact( shot, impacts, missiles );
			string missText = "";
			double missUnits = 0;
			if ( impact is not null ) missText = DescribeMiss( shot, impact, impacts, out missUnits );

			// die Quote mit Hilfe sagt erst etwas, wenn die ohne daneben steht
			if ( shot.Assisted ) { assisted++; if ( landed ) assistedHit++; }
			else { unassisted++; if ( landed ) unassistedHit++; }

			if ( !ranks.TryGetValue( shot.Weapon, out var rank ) ) {
				ranks[shot.Weapon] = rank = new Rank { Weapon = shot.Weapon };
			}
			rank.Shots++;
			if ( landed ) rank.Hits++;
			if ( missText.Length > 0 ) { rank.MissSum += missUnits; rank.MissCount++; }

			rows.Add( new ListViewItem( new[] {
				shot.Frame.ToString(),
				shot.Weapon,
				shot.Target,
				shot.Distance.ToString(),
				shot.InAir ? ( shot.Land >= 0 ? $"ja, landet {shot.Land} ms" : "ja" ) : "nein",
				shot.Lead + " ms",
				shot.Error.ToString( "0.00" ) + "°",
				shot.Swing.ToString( "0.00" ) + "°",
				shot.Pace.ToString( "0" ) + "/" + shot.MySpeed.ToString( "0" ),
				shot.Position,
				landed ? "Treffer" : "daneben",
				shot.Assisted ? "ja" : "nein",
				missText.Length > 0 ? missUnits.ToString( "0" ) : "",
				missText,
			} ) { ForeColor = landed ? Color.ForestGreen : Color.Firebrick } );
		}

		statShots.Text = shots.Count.ToString();
		statShotHits.Text = hit.ToString();
		statShotMiss.Text = ( shots.Count - hit ).ToString();
		statShotRate.Text = assisted > 0 ? ( 100 * assistedHit / assisted ) + "%" : "–";
		statShotRateOff.Text = unassisted > 0 ? ( 100 * unassistedHit / unassisted ) + "%" : "–";
		statShotError.Text = shots.Count > 0 ? ( errorSum / shots.Count ).ToString( "0.00" ) + "°" : "–";
		statShotMiss.ForeColor = shots.Count > hit ? Color.Firebrick : Color.ForestGreen;
		UpdateRanking( ranks );

		// Neu zeichnen, sobald sich etwas geaendert hat: die Zeilenzahl allein
		// bleibt gleich, wenn ein Schuss nachtraeglich zum Treffer wird.
		var stamp = rows.Count + ":" + hit + ":" + ( shots.Count > 0 ? shots[^1].Frame : 0 );
		if ( stamp == shotStamp ) return;
		shotStamp = stamp;

		// Eine gewaehlte Zeile bleibt gewaehlt, und nur ohne Auswahl laeuft die
		// Liste dem neuesten Schuss hinterher - wer etwas ansieht, will nicht
		// bei jedem Schuss weggescrollt werden
		var keep = SelectedKey( shotView );
		shotView.BeginUpdate();
		shotView.Items.Clear();
		shotView.Items.AddRange( rows.TakeLast( 300 ).ToArray() );
		Reselect( shotView, keep );
		if ( keep is null && shotView.Items.Count > 0 ) shotView.EnsureVisible( shotView.Items.Count - 1 );
		shotView.EndUpdate();
	}
}

// WinForms zeichnet eine ListView ungepuffert: beim Ziehen am Fenster flackert
// sie sichtbar, und bei sechs Listen faellt das auf. Das Flag dagegen ist
// geschuetzt, also braucht es eine Ableitung.
//
// Die Bildliste ist der uebliche Kniff fuer die Zeilenhoehe: eine ListView
// richtet sich nach ihrer SmallImageList, nicht nach der Schrift. Ein Pixel
// breit, damit sie sonst nichts tut.
sealed class SmoothListView : ListView {
	public const int RowHeight = 24;

	public SmoothListView() {
		DoubleBuffered = true;
		SetStyle( ControlStyles.OptimizedDoubleBuffer | ControlStyles.AllPaintingInWmPaint, true );
		SmallImageList = new ImageList { ImageSize = new Size( 1, RowHeight ) };
	}
}
