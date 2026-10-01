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

	// Die Reihenfolge ist der Wert von cl_hitSound. Quake 1 und 2 hatten gar
	// keinen Trefferton - die beiden Toene sind nachgebaut, nicht entnommen.
	static readonly string[] HitSounds = {
		"Original", "Quake Champions", "Eigene Datei", "Quake 1 (nachgebaut)", "Quake 2 (nachgebaut)",
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
	// Alles ab hier bis zum Bot-Protokoll steht auf der Karte „Bots“. Die
	// Beschriftungen sagen deshalb nicht mehr „Bots …“ - die Karte heißt schon
	// so. Jede Vorgabe ist der Stand „Standard Q3“: eine Einstellungsdatei, die
	// einen Regler noch nicht kennt, lässt ihn damit beim Original.
	//
	// Die Bots haben zwar eine Kantenpruefung, aber sie verweigert nur den
	// Befehl - gebremst wird nirgends, und bei dreihundertzwanzig Einheiten je
	// Sekunde braucht die Reibung rund fuenfzig Einheiten Weg. Siehe BotEdgeCare.
	readonly CheckBox botEdgeCare = new() { Text = "nicht in die Leere laufen", Checked = false, AutoSize = true };
	// Das Gegenstück in der Luft: wen ein Treffer über die Kante wirft, der hält
	// dagegen. Im Original steht an der Stelle ein FIXME. Siehe BotFooting.
	readonly CheckBox botAirControl = new() { Text = "in der Luft zurück ans Land steuern", Checked = false, AutoSize = true };
	readonly CheckBox botDodge = new() { Text = "Raketen ausweichen", Checked = false, AutoSize = true };
	// Ein Fleck ohne Wegnetz: von dort führt kein Weg zu irgendeinem Ziel, und
	// der Bot steht, bis ihn jemand abschießt.
	readonly CheckBox botUnstuck = new() { Text = "von Simsen ohne Wegnetz herunterkommen", Checked = false, AutoSize = true };
	// Zwei Cvars hinter einer Auswahl, weil es eine Frage ist: bot_rocketjump
	// sagt ob überhaupt, g_botRocketJump wer und unter welcher Bedingung. Als
	// Haken konnte das nur „öfter“ und nie „gar nicht“. Die Einträge und ihre
	// Reihenfolge stehen im Konstruktor, die Übersetzung in BuildConfig.
	readonly ComboBox botRocketJumpMode = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 220 };
	// Ohne Bodenreibung behaelt man beim Springen sein Tempo. Die Bots machen
	// das von sich aus nie - siehe BotSpeedJump.
	readonly CheckBox botJump = new() { Text = "hüpfen, um Tempo zu halten (gemessen: bringt nichts)", Checked = false, AutoSize = true };
	readonly NumericUpDown botJumper = CharValue();
	readonly NumericUpDown botCroucher = CharValue();
	// BotAggression gibt null zurück, sobald der Gegner 200 Einheiten höher
	// steht - auf einer Karte aus Plattformen also fast immer.
	readonly CheckBox botFightUp = new() { Text = "auch nach oben kämpfen", Checked = false, AutoSize = true };
	// Feste Schwellen - fünf Raketen, sechzig Leben - statt der Frage, ob der
	// Bot mit dem, was er in der Hand hat, kämpfen kann.
	readonly CheckBox botBrave = new() { Text = "kämpfen mit dem, was da ist", Checked = false, AutoSize = true };
	readonly CheckBox botHear = new() { Text = "Schüsse, Sprünge und Schritte hören", Checked = false, AutoSize = true };
	// In Sekunden. Null ist das Original: die Entscheidung kippt mit jedem
	// Denkschritt neu.
	readonly NumericUpDown botSteady = new() { DecimalPlaces = 1, Increment = 0.5m, Minimum = 0m, Maximum = 5m, Value = 0m, Width = 70 };
	// Eine Zahl, die ganze Leiter: unter 0,2 steht der Bot still, über 0,7
	// umkreist er mit Rhythmus. Früher ein Haken zusammen mit dem Lagern, der
	// nur 0,9 oder „Charakterdatei“ kannte.
	readonly NumericUpDown botAttackSkill = CharValue();
	// Die Reaktionszeit ist die einzige der Charakterzahlen in Sekunden, also
	// reicht sie über eins hinaus.
	readonly NumericUpDown botReaction = SnapToCharacter( new() { DecimalPlaces = 2, Increment = 0.1m, Minimum = -1m, Maximum = 5m, Value = -1m, Width = 70 } );
	readonly NumericUpDown botAimAccuracy = CharValue();
	readonly NumericUpDown botAimSkill = CharValue();
	readonly NumericUpDown botAlertness = CharValue();
	readonly NumericUpDown botFireThrottle = CharValue();
	readonly CheckBox botChallenge = new() { Text = "Herausforderung (bot_challenge)", Checked = false, AutoSize = true };
	// Das Fernziel ist zwanzig Sekunden gesperrt, und Schaden loest die Sperre
	// nirgends - ein Bot auf dreissig Leben holt weiter die Waffe statt Medipack.
	readonly CheckBox botRethink = new() { Text = "nach Treffern neu planen", Checked = false, AutoSize = true };
	// Das Spielmodul kennt jeden Respawn-Zeitpunkt und gibt ihn nie weiter.
	readonly CheckBox botTiming = new() { Text = "Respawn-Zeiten mitzählen", Checked = false, AutoSize = true };
	readonly CheckBox botGrab = new() { Text = "im Kampf Gegenstände mitnehmen", Checked = false, AutoSize = true };
	// Braucht das Hören: ohne botHear ist der Haken grau, und in die Config
	// geht 0 - siehe UpdateBotEnabled.
	readonly CheckBox botHunt = new() { Text = "ohne Gegner dorthin gehen, wo Lärm war", Checked = false, AutoSize = true };
	// Prozent der Zielwahlen, ganzzahlig.
	readonly NumericUpDown botVariety = new() { DecimalPlaces = 0, Increment = 5m, Minimum = 0m, Maximum = 100m, Value = 0m, Width = 70 };
	// Lagern ist Stillstand.
	readonly NumericUpDown botCamper = CharValue();
	// Tausend ist das Original, und das ist mehr als jedes Powerup: die Bots
	// laufen jeder Waffe nach, die ein Toter fallen lässt.
	readonly NumericUpDown botDroppedWeight = new() { DecimalPlaces = 0, Increment = 50m, Minimum = 0m, Maximum = 1000m, Value = 1000m, Width = 70 };
	// Jede Chatzeile kostet den Bot genau zwei Sekunden Stillstand - AINode_Stand
	// gibt gar keinen Bewegungsbefehl. Betrifft auch "Gegner tot" mitten im Kampf.
	readonly CheckBox botNoChat = new() { Text = "nicht quatschen (kostet 2 s Stillstand je Zeile)", Checked = false, AutoSize = true };
	// Das Protokoll ist ein Messgerät und keine Eigenschaft der Bots: die
	// beiden Knöpfe fassen es nicht an, und die Statuszeile zählt es nicht mit.
	readonly CheckBox botLog = new() { Text = "Bot-Protokoll schreiben (botlog.log)", Checked = false, AutoSize = true };
	readonly Label botPresetValue = new() { AutoSize = true, ForeColor = Color.DimGray };

	// Ein Wert aus der Charakterdatei, 0 bis 1 - oder -1 für „lass stehen, was
	// in der Datei steht“. Das Spiel nimmt jede Vorgabe ab null und liest bei
	// allem darunter die Datei, siehe BotChar in code/game/ai_dmq3.c.
	static NumericUpDown CharValue() => SnapToCharacter( new() {
		DecimalPlaces = 2, Increment = 0.05m, Minimum = -1m, Maximum = 1m, Value = -1m, Width = 70,
	} );

	// Zwischen -1 und 0 gibt es für das Spiel nichts: BotChar nimmt eine Vorgabe
	// ab null und liest bei allem darunter die Charakterdatei. Ein Feld auf
	// -0,5 hieße also dasselbe wie -1, und die Statuszeile meldete trotzdem
	// „eigene Einstellung“. Der Schritt von -1 nach oben landet deshalb auf 0,
	// der von 0 nach unten auf -1.
	static NumericUpDown SnapToCharacter( NumericUpDown box ) {
		decimal last = box.Value;
		box.ValueChanged += ( _, _ ) => {
			if ( box.Value > -1m && box.Value < 0m ) {
				// kommt mit einem gültigen Wert gleich wieder hier an
				box.Value = last < 0m ? 0m : -1m;
				return;
			}
			last = box.Value;
		};
		return box;
	}

	// Was die beiden Knöpfe stellen und woran die Statuszeile den Stand erkennt:
	// je Regler der Wert für „Standard Q3“ und der für „Menschlich“. Ein Haken
	// ist 0 oder 1, eine Auswahl ihr Index, ein Zahlenfeld sein Wert.
	//
	// EINE Tabelle für beides, mit Absicht. Stünden die Werte einmal im Knopf
	// und einmal im Vergleich, liefen die beiden beim ersten neuen Regler
	// auseinander - und die Zeile behauptete „eigene Einstellung“, direkt
	// nachdem man „Menschlich“ gedrückt hat.
	//
	// Nicht darin: bots und skill (die Besetzung ist keine Eigenschaft der
	// Bots) und botLog. Die Reihenfolge ist die der Karte und der Config.
	//
	// Als Eigenschaft und nicht als Feld, weil ein Feld die Regler bei seiner
	// Vorbelegung noch nicht nennen darf.
	(Control Box, decimal Stock, decimal Human)[]? botPresetTable;
	(Control Box, decimal Stock, decimal Human)[] BotPresets => botPresetTable ??= new (Control, decimal, decimal)[] {
		// Bewegung
		( botEdgeCare,       0,  1 ),
		( botAirControl,     0,  1 ),
		( botDodge,          0,  1 ),
		( botUnstuck,        0,  1 ),
		( botRocketJumpMode, 1,  2 ),
		// gemessen bringt das Hüpfen nichts, also bleibt es auch hier aus
		( botJump,           0,  0 ),
		( botJumper,        -1,  0.25m ),
		( botCroucher,      -1, -1 ),
		// Kampf
		( botFightUp,        0,  1 ),
		// gemessen macht das die Bots leichter: sie bleiben in Kämpfen, die sie
		// verlassen sollten - also bei „Menschlich“ aus, der Haken bleibt
		( botBrave,          0,  0 ),
		( botHear,           0,  1 ),
		( botSteady,         0,  1.5m ),
		( botAttackSkill,   -1,  0.9m ),
		// Zielen und Reagieren bleiben beim Charakter: gegen diese Bots wird
		// gemessen, und ein Bot, der besser trifft, ist nicht menschlicher
		( botReaction,      -1, -1 ),
		( botAimAccuracy,   -1, -1 ),
		( botAimSkill,      -1, -1 ),
		( botAlertness,     -1, -1 ),
		( botFireThrottle,  -1, -1 ),
		( botChallenge,      0,  0 ),
		// Ziele und Gegenstände
		( botRethink,        0,  1 ),
		( botTiming,         0,  1 ),
		( botGrab,           0,  1 ),
		( botHunt,           0,  1 ),
		( botVariety,        0,  25 ),
		( botCamper,        -1,  0 ),
		( botDroppedWeight, 1000, 100 ),
		// Sonstiges
		( botNoChat,         0,  1 ),
	};
	// Nachladezeiten in Prozent der normalen, nur fuer Menschen. Zehn Prozent
	// ist der Boden; darunter bliebe der Zielhilfe kein Bild mehr, auf dem sie
	// den Schuss kommen sieht.
	readonly TrackBar weaponRate = new() {
		Minimum = 10, Maximum = 200, Value = 100, TickFrequency = 10,
		SmallChange = 5, LargeChange = 25, Width = 190,
	};
	readonly Label weaponRateValue = new() { AutoSize = true, ForeColor = Color.DimGray };

	// Zielsuch-Raketen. Der Index ist der Wert von g_homingRockets - dieselbe
	// Zählung wie bei der Munition: nur ich, oder alle samt Bots.
	readonly ComboBox homingMode = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 190 };
	// Die Drehrate in Grad je Sekunde. Sie IST der Wendekreis - bei 900
	// Einheiten Fluggeschwindigkeit: Radius = 900 / (Grad · π / 180). Die
	// Anzeige daneben rechnet das aus, weil "wie eng" greifbarer ist als "wie
	// schnell dreht sie".
	readonly TrackBar homingTurn = new() {
		Minimum = 30, Maximum = 720, Value = 180, TickFrequency = 60,
		SmallChange = 15, LargeChange = 90, Width = 190,
	};
	readonly Label homingTurnValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	readonly Button homingTurnDefault = new() {
		Text = "Standard", Width = 84, Height = 24,
		Margin = new Padding( 12, 4, 0, 0 ), FlatStyle = FlatStyle.System,
	};
	readonly NumericUpDown homingCone = new() { DecimalPlaces = 0, Increment = 5m, Minimum = 10m, Maximum = 180m, Value = 45m, Width = 70 };
	// Festhalten oder umschwenken: mit dem Haken nimmt die Rakete jedes Bild neu
	// das naechste Ziel, auch mitten im Anflug auf ein anderes.
	readonly CheckBox homingRetarget = new() { Text = "jedes Bild neu wählen (auf ein besseres Ziel umschwenken)", Checked = false, AutoSize = true };
	// Lebensdauer in halben Sekunden, 1 bis 30 - der Schieber kennt nur ganze
	// Zahlen. 30 sind die 15 s, nach denen sich jede Rakete schon immer zerlegt.
	readonly TrackBar homingLife = new() {
		Minimum = 1, Maximum = 30, Value = 30, TickFrequency = 2,
		SmallChange = 1, LargeChange = 4, Width = 190,
	};
	readonly Label homingLifeValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	readonly Button homingLifeDefault = new() {
		Text = "Standard", Width = 84, Height = 24,
		Margin = new Padding( 12, 4, 0, 0 ), FlatStyle = FlatStyle.System,
	};
	// Die weiteren Regler der Zielsuche. Jede Vorgabe ist "wie bisher": wer
	// nichts anfasst, fliegt dieselbe Rakete wie vor diesen Reglern. Die
	// Indizes der Auswahllisten sind die Werte der Cvars, bis auf die Splitter.
	readonly ComboBox homingPick = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 240 };
	readonly ComboBox homingAir = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 240 };
	readonly ComboBox homingMissiles = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 240 };
	readonly TrackBar homingLead = new() {
		Minimum = 0, Maximum = 100, Value = 0, TickFrequency = 10,
		SmallChange = 5, LargeChange = 25, Width = 190,
	};
	readonly Label homingLeadValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	readonly NumericUpDown homingArm = new() { DecimalPlaces = 0, Increment = 50m, Minimum = 0m, Maximum = 1000m, Value = 0m, Width = 70 };
	readonly NumericUpDown homingFuel = new() { DecimalPlaces = 1, Increment = 0.5m, Minimum = 0m, Maximum = 15m, Value = 0m, Width = 70 };
	// Schärfzeit und Treibstoff je eine Zeile: zusammen waren sie breiter als
	// die Spalte bei der kleinsten Teilerstellung und wurden abgeschnitten.
	readonly Label homingSteerValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	readonly Label homingFuelValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	readonly NumericUpDown homingSpeedStart = new() { DecimalPlaces = 0, Increment = 50m, Minimum = 100m, Maximum = 3000m, Value = 900m, Width = 70 };
	readonly NumericUpDown homingSpeedEnd = new() { DecimalPlaces = 0, Increment = 50m, Minimum = 100m, Maximum = 3000m, Value = 900m, Width = 70 };
	readonly NumericUpDown homingSpeedRamp = new() { DecimalPlaces = 1, Increment = 0.1m, Minimum = 0.1m, Maximum = 10m, Value = 1.0m, Width = 60 };
	readonly Label homingSpeedValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	readonly TrackBar homingDrag = new() {
		Minimum = 0, Maximum = 100, Value = 0, TickFrequency = 10,
		SmallChange = 5, LargeChange = 25, Width = 190,
	};
	readonly Label homingDragValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	// Die Splitter: Index 0 heißt keine, danach 2, 3 und 4 - einer allein
	// wäre nur dieselbe Rakete mit halbem Schaden.
	readonly ComboBox homingSplit = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 190 };
	readonly NumericUpDown homingProx = new() { DecimalPlaces = 0, Increment = 8m, Minimum = 0m, Maximum = 300m, Value = 0m, Width = 70 };
	readonly Label homingProxValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	readonly CheckBox homingWarn = new() { Text = "Warnton, wenn mich eine Rakete verfolgt", Checked = false, AutoSize = true };
	// Hundert Prozent trifft man mit dem Schieber kaum genau; der Knopf daneben
	// ist der Weg zurück zum Spiel-Original.
	readonly Button weaponRateDefault = new() {
		Text = "Standard", Width = 84, Height = 24,
		Margin = new Padding( 12, 4, 0, 0 ), FlatStyle = FlatStyle.System,
	};
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
	// Der Abzug drückt selbst, sobald das Ziel sicher ist: nichts steht im Weg,
	// und die Sicht liegt schon innerhalb des Winkels, den der Körper auf dieser
	// Entfernung einnimmt. Das ist eine Bedingung der Sitzung wie Nachladezeit
	// und Munition, nicht nur eine Bequemlichkeit - deshalb steht es auch im
	// Protokollkopf.
	readonly CheckBox aimAutoFire = new() { Text = "mit der Zieltaste selbst abdrücken, wenn das Ziel sicher ist", Checked = false, AutoSize = true };
	// Ob der Sturz über eine Plattformkante auch wirklich angelegt wird. Aus
	// heisst: nur erkennen und ins Protokoll schreiben, Zielpunkt unverändert.
	readonly CheckBox aimEdge = new() { Text = "Sturz über die Kante anlegen (F26)", Checked = false, AutoSize = true };
	readonly CheckBox aimLearn = new() { Text = "je Waffe und Entfernung nachmessen", Checked = true, AutoSize = true };
	// Quake-Live-Bewegung, Stufe eins. Beide Schalter gehen als
	// CVAR_SYSTEMINFO an Spielmodul UND cgame; deshalb müssen nach einer
	// Änderung auch beide neu ausgeliefert werden.
	// ZTMs Flexible HUD, seit der Zusammenführung in unserem eigenen cgame.
	// Die Vorgaben stehen bewusst dort, wo deine Konfiguration sie schon
	// hatte - der Haken für das Seitenverhältnis kam auf 1, nicht auf die
	// Mod-Vorgabe 0, sonst schaltete das Werkzeug beim ersten Start die
	// Breitbild-Sicht ab, die du benutzt.
	readonly CheckBox hudFovAspect = new() { Text = "FOV aufs Seitenverhältnis umrechnen (Breitbild)", Checked = true, AutoSize = true };
	readonly CheckBox hudFovGun = new() { Text = "Waffe mit dem FOV mitführen", Checked = true, AutoSize = true };
	readonly CheckBox hudWeaponBar = new() { Text = "Waffenleiste", Checked = true, AutoSize = true };
	readonly CheckBox hudStatusHead = new() { Text = "Gesicht in der Statuszeile", Checked = true, AutoSize = true };
	readonly CheckBox hudPickups = new() { Text = "Aufgesammeltes einblenden", Checked = true, AutoSize = true };
	readonly CheckBox hudScores = new() { Text = "Punktestand einblenden", Checked = true, AutoSize = true };
	readonly CheckBox hudStretch = new() { Text = "HUD über die ganze Breite ziehen", Checked = false, AutoSize = true };
	readonly NumericUpDown hudStatusScale = new() { DecimalPlaces = 2, Increment = 0.05m, Minimum = 0.30m, Maximum = 2.00m, Value = 1.00m, Width = 70 };

	readonly CheckBox qlAutoHop = new() { Text = "Auto-Hop: gehaltene Sprungtaste springt weiter", Checked = false, AutoSize = true };
	readonly CheckBox qlWeaponSwitch = new() { Text = "Waffenwechsel wie Quake Live (400 statt 450 ms)", Checked = false, AutoSize = true };
	// Luftsteuerung, Rampensprung, Schritthöhe. Die Werte sind die aus id
	// Softwares eigenen Factories: Race setzt AirControl 1 und RampJump 1,
	// einzelne Spieltypen gehen bei der Schritthöhe auf 20 oder 28.
	readonly CheckBox qlAirControl = new() { Text = "Luftsteuerung (nur geradeaus, wie Race/PQL)", Checked = false, AutoSize = true };
	// Faktor auf die CPM-Stärke 150, die das Spiel seit Protokollfassung 20
	// selbst einrechnet: 1,00 ist Race. Mehr als 3 dreht den Schwung fast
	// sofort herum und ist keine Luftsteuerung mehr, sondern ein Lenkrad.
	readonly NumericUpDown qlAirControlValue = new() { DecimalPlaces = 2, Increment = 0.25m, Minimum = 0.25m, Maximum = 3.00m, Value = 1.00m, Width = 70 };
	// Das Feld zeigt den echten Wert, nicht "0 = Original": vorher stand es auf
	// 0, und ein Klick nach oben ergab 0,25 - eine SCHWÄCHERE Luftbeschleunigung
	// als Quake 3, obwohl man mehr wollte. Geschrieben wird trotzdem 0, solange
	// es auf 1,00 steht, damit der Protokollkopf den Normalfall als solchen zeigt.
	readonly NumericUpDown qlAirAccel = new() { DecimalPlaces = 2, Increment = 0.25m, Minimum = 0.25m, Maximum = 10.00m, Value = 1.00m, Width = 70 };
	readonly CheckBox qlRampJump = new() { Text = "Rampensprung: Aufwärtsschwung behalten", Checked = false, AutoSize = true };
	readonly NumericUpDown qlRampScale = new() { DecimalPlaces = 2, Increment = 0.05m, Minimum = 0.50m, Maximum = 3.00m, Value = 1.00m, Width = 70 };
	// Dasselbe hier, und hier war es gefährlich: 0 hieß "Original (18)", ein
	// Klick nach oben ergab 1 - eine Stufe von einer Einheit, und man kam auf
	// kein erhöhtes Sprungfeld mehr hinauf, ohne zu springen. Jetzt steht der
	// echte Wert da, und unter 16 geht es nicht: tiefer klettert man keine
	// normale Treppe mehr. Ein alter gespeicherter Wert darunter wird beim Laden
	// verworfen (SetNum nimmt nur, was in den Grenzen liegt) - die 1 heilt sich
	// also von selbst.
	readonly NumericUpDown qlStepHeight = new() { DecimalPlaces = 0, Increment = 1m, Minimum = 16m, Maximum = 40m, Value = 18m, Width = 70 };
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
	const int LogVersion = 20;
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
		weaponRateDefault.Click += ( _, _ ) => weaponRate.Value = 100;
		homingMode.Items.AddRange( new object[] { "aus", "nur meine Raketen", "alle (auch die Bots)" } );
		homingMode.SelectedIndex = 0;
		homingPick.Items.AddRange( new object[] {
			"das nächste", "das im Fadenkreuz", "am leichtesten zu töten", "wer mich zuletzt getroffen hat" } );
		homingPick.SelectedIndex = 0;
		homingAir.Items.AddRange( new object[] { "alle", "nur wer in der Luft ist", "nur wer am Boden steht" } );
		homingAir.SelectedIndex = 0;
		homingMissiles.Items.AddRange( new object[] {
			"nur Spieler", "Spieler und gegnerische Raketen", "nur Raketen (Abfangjäger)" } );
		homingMissiles.SelectedIndex = 0;
		homingSplit.Items.AddRange( new object[] { "zerlegen", "in 2 Splitter zerfallen", "in 3 Splitter zerfallen", "in 4 Splitter zerfallen" } );
		homingSplit.SelectedIndex = 0;
		homingMode.SelectedIndexChanged += ( _, _ ) => ShowHoming();
		homingTurn.ValueChanged += ( _, _ ) => ShowHoming();
		homingTurnDefault.Click += ( _, _ ) => homingTurn.Value = 180;
		homingLife.ValueChanged += ( _, _ ) => ShowHoming();
		homingLifeDefault.Click += ( _, _ ) => homingLife.Value = 30;
		homingLead.ValueChanged += ( _, _ ) => ShowHoming();
		homingArm.ValueChanged += ( _, _ ) => ShowHoming();
		homingFuel.ValueChanged += ( _, _ ) => ShowHoming();
		homingSpeedStart.ValueChanged += ( _, _ ) => ShowHoming();
		homingSpeedEnd.ValueChanged += ( _, _ ) => ShowHoming();
		homingSpeedRamp.ValueChanged += ( _, _ ) => ShowHoming();
		homingDrag.ValueChanged += ( _, _ ) => ShowHoming();
		homingProx.ValueChanged += ( _, _ ) => ShowHoming();
		homingMissiles.SelectedIndexChanged += ( _, _ ) => ShowHoming();
		infiniteAmmo.Items.AddRange( new object[] { "wie im Spiel", "unbegrenzt für mich", "unbegrenzt für alle" } );
		infiniteAmmo.SelectedIndex = 0;
		infiniteAmmo.SelectedIndexChanged += ( _, _ ) => UpdateSwitchEnabled();
		autoSwitch.CheckedChanged += ( _, _ ) => UpdateSwitchEnabled();
		// Das Original ist der Eintrag 1 und nicht 0: abgeschaltet ist der
		// Raketensprung in Quake 3 nur, wenn man es ausdrücklich verlangt.
		botRocketJumpMode.Items.AddRange( new object[] {
			"gar nicht", "wie der Charakter (Original)", "alle, mit Gesundheitsklausel", "alle, auch ohne Rüstung" } );
		botRocketJumpMode.SelectedIndex = 1;
		// Die Statuszeile rechnet sofort und nicht über BeginInvoke: LoadSettings
		// läuft, bevor das Fenster ein Handle hat, und dort wirft BeginInvoke.
		// Einen Aufschub braucht es hier auch nicht - anders als bei ItemCheck
		// steht der neue Wert schon im Regler, wenn das Ereignis kommt.
		foreach ( var p in BotPresets ) {
			if ( p.Box is CheckBox check ) check.CheckedChanged += ( _, _ ) => ShowBotPreset();
			else if ( p.Box is ComboBox list ) list.SelectedIndexChanged += ( _, _ ) => ShowBotPreset();
			else if ( p.Box is NumericUpDown number ) number.ValueChanged += ( _, _ ) => ShowBotPreset();
		}
		botHear.CheckedChanged += ( _, _ ) => UpdateBotEnabled();
		holdLottery.ValueChanged += ( _, _ ) => ShowHoldLottery();
		ShowHoldLottery();
		// die Folge-Felder auf den Standard-Hakenstand bringen
		UpdateBotEnabled();
		ShowBotPreset();
		UpdateItemEnabled();
		ShowItemRange();
		ShowWeaponRate();
		ShowHoming();
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
		aimAutoFire.Enabled = on;
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
		// Die Bots hatten acht Haken mitten in „Spiel“, zwischen Selbstschaden
		// und Munition, und es kommen achtzehn Regler dazu. Gleich an zweiter
		// Stelle, weil sie nach der Map das sind, was eine Sitzung ausmacht -
		// dass sich die Nummern der Karten dahinter um eins verschieben und ein
		// gemerkter settingsTab einmal daneben liegt, ist hingenommen.
		tabs.TabPages.Add( SettingsPage( "Bots", BuildBotPresetBox(), BuildBotCastBox(), BuildBotMoveBox(),
			BuildBotFightBox(), BuildBotGoalBox(), BuildBotMiscBox() ) );
		// Die Zielsuch-Raketen haben achtzehn Regler bekommen - mehr als jede
		// andere Gruppe. Auf "Spiel" hätten sie alles andere verdrängt.
		tabs.TabPages.Add( SettingsPage( "Raketen", BuildHomingBox(), BuildHomingFlightBox() ) );
		// Eigene Karte, obwohl erst zwei Haken darauf stehen: die Karte "Spiel"
		// traegt schon zweiundzwanzig Zeilen, doppelt so viel wie jede andere,
		// und diese Gruppe waechst von allen am staerksten - Luftsteuerung,
		// Doppelsprung und Crouch-Slide kommen mit eigenen Reglern.
		tabs.TabPages.Add( SettingsPage( "Quake Live", BuildQuakeLiveBox() ) );
		tabs.TabPages.Add( SettingsPage( "HUD", BuildHudBox() ) );
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
		// ausserhalb der Karten - siehe BuildHeader. Wie viele Bots und wie gut
		// sie sind, steht mit allem anderen über sie auf der Karte „Bots“.
		return Group( "Spiel",
			Row( Labelled( "Spielordner:", gameDir ), browse ),
			Row( Labelled( "Map:", map ) ),
			Row( Pad( noSelfDamage ) ),
			Row( Labelled( "Munition:", infiniteAmmo ) ),
			Row( Labelled( "Nachladezeit:", weaponRate ), weaponRateDefault ),
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

	// Die Karte „Bots“, oberste Gruppe: zwei Knöpfe, die alles auf einmal
	// stellen, und die Zeile, die sagt, welcher Stand gerade gilt. Die Zeile
	// steht unter den Knöpfen und nicht daneben - mit ihrem längsten Text wäre
	// die Reihe breiter als die Spalte bei der kleinsten Teilerstellung.
	GroupBox BuildBotPresetBox() {
		// Mit eigener Hoehe und Rand: als erste Zeile direkt unter der
		// Gruppenbeschriftung wurden die Knoepfe sonst unten abgeschnitten.
		var stock = new Button { Text = "Standard Q3", Width = 124, Height = 26, Margin = new Padding( 0, 4, 6, 4 ) };
		var human = new Button { Text = "Menschlich", Width = 124, Height = 26, Margin = new Padding( 0, 4, 0, 4 ) };
		stock.Click += ( _, _ ) => ApplyBotPreset( false );
		human.Click += ( _, _ ) => ApplyBotPreset( true );

		hintTip.SetToolTip( stock, "Stellt jeden Regler dieser Karte auf das Original: alle Haken aus, der"
			+ " Raketensprung wie der Charakter, jede Zahl aus der Charakterdatei. Bots, Können und das"
			+ " Bot-Protokoll bleiben, wie sie sind." );
		hintTip.SetToolTip( human, "Stellt jeden Regler dieser Karte auf den empfohlenen Wert. Zielen und"
			+ " Reagieren bleiben dabei beim Charakter – ein Bot, der besser trifft, ist nicht"
			+ " menschlicher. Bots, Können und das Bot-Protokoll bleiben, wie sie sind.\n\n"
			+ "Achtung für die Messung: fast jeder dieser Regler ändert, wie sich die Bots bewegen –"
			+ " also das, wogegen die Vorhersage gemessen wird. Neue Basislinie nötig, bevor du gegen"
			+ " alte Zahlen vergleichst." );

		return Group( "Voreinstellung",
			Row( stock, human ),
			Row( Pad( botPresetValue ) ) );
	}

	// Wie viele und wie gut. Die Knöpfe fassen beides nicht an: die Besetzung
	// gehört zur Sitzung und nicht zum Verhalten, und wer zehn Bots auf Stufe
	// fünf eingestellt hat, will sie nach „Menschlich“ noch haben.
	GroupBox BuildBotCastBox() {
		return Group( "Besetzung",
			Row( Labelled( "Bots:", bots ), Labelled( "Können:", skill ) ) );
	}

	// Die Zahlenfelder je eine Zeile: zwei nebeneinander wären mit diesen
	// Beschriftungen breiter als die Spalte.
	GroupBox BuildBotMoveBox() {
		hintTip.SetToolTip( botEdgeCare, "In jedem Bild wird vorausgerechnet, wo der Bot mit seinem Befehl in"
			+ " den nächsten vier Zehntelsekunden landet – mit Reibung, Schwung und Schwerkraft wie"
			+ " im Spiel. Ist das Leere, Lava, Schleim oder eine Todeszone, bekommt er das Mildeste,"
			+ " was hilft: kein Sprung, Schritttempo, stehenbleiben, gegensteuern. Auf seinem Weg"
			+ " nach der Karte wird er nie angehalten, nur gebremst, wenn er damit wirklich landet.\n\n"
			+ "Gemessen auf den sechs Karten mit Grube oder Lava, zwölf Bots, je Lauf vier Minuten:"
			+ " 40 Tode durch die Karte statt 72 beim Original." );
		hintTip.SetToolTip( botAirControl, "Wirft ein Treffer den Bot über eine Kante oder stößt er im Flug mit"
			+ " jemandem zusammen, steuert er dorthin, wo die Rechnung wieder eine Landung findet –"
			+ " zuerst zurück zum letzten sicheren Stand. Das Original lässt sich fallen. Einen"
			+ " geplanten Flug (Sprungfeld, Sprung über eine Lücke) lässt das in Ruhe." );
		hintTip.SetToolTip( botDodge, "Fliegt eine Rakete auf den Bot zu und er sieht sie, macht er einen"
			+ " Schritt quer zur Flugbahn – aber nie über eine Kante. Nicht jeder sieht jede"
			+ " rechtzeitig: auf Könnensstufe 1 weicht er 44 von 100 aus, auf Stufe 5 allen."
			+ " Gemessen mit zehn Bots und nur Raketenwerfern, je drei Läufe: 148 statt 171 Tode in"
			+ " fünf Minuten, und nicht mehr Stürze." );
		hintTip.SetToolTip( botUnstuck, "Ein Bot kennt die Karte nur als Netz von Feldern. Landet er daneben –"
			+ " auf einem Sims, einem Zierrat –, führt von dort kein Weg zu irgendeinem Ziel, und er"
			+ " steht, bis ihn jemand abschießt; gemessen einmal achtzig Sekunden lang. Mit Haken sucht"
			+ " er nach einer Sekunde den nächsten Boden, auf dem es weitergeht, und geht hin – auch"
			+ " über die Kante." );
		// Der Text des alten Hakens, auf die vier Einträge umgeschrieben. Die 55
		// Leben, die dort standen, gibt es nicht mehr: die gelockerte Klausel hat
		// die Stürze verdoppelt und ist im Spiel wieder auf 60 - siehe
		// BotCanAndWantsToRocketJump in code/game/ai_dmq3.c.
		hintTip.SetToolTip( botRocketJumpMode, "Auf q3dm17 liegen 225 Raketensprung-Verbindungen, und der"
			+ " Ausführer ist vollständig – was sie verhindert, ist eine Klausel: mindestens 60 Leben,"
			+ " und unter 90 zusätzlich 40 Rüstung. Dazu kommt die Sprungfreude aus der Charakterdatei"
			+ " (sieben Charaktere liegen darunter).\n\n"
			+ "„alle, mit Gesundheitsklausel“ übergeht nur die Sprungfreude. „alle, auch ohne Rüstung“"
			+ " lässt zusätzlich die Rüstung fallen; die 60 Leben bleiben in jedem Fall, denn ein"
			+ " Raketensprung kostet um die fünfzig. „gar nicht“ schaltet ihn für alle ab.\n\n"
			+ "Gemessen, ehrlich: auf q3dm17 springt trotzdem keiner, in keinem Lauf. Die Wegsuche"
			+ " rechnet einen Raketensprung mit 300 und ein Sprungfeld mit 200, und auf dem Rückzug –"
			+ " zwei Drittel der Zeit – erlaubt das Original ihn gar nicht. Der Regler wirkt nur dort,"
			+ " wo ein Raketensprung der einzige Weg ist." );
		hintTip.SetToolTip( botJump, "Springt ein Bot beim Laufen, verliert er kein Tempo an die Bodenreibung."
			+ " Gesprungen wird nur geradeaus, schon schnell und mit Boden voraus – nicht beim"
			+ " Ausweichen im Gefecht, denn ein Bot in der Luft fliegt eine Wurfparabel und ist"
			+ " damit leichter zu treffen, nicht schwerer.\n\n"
			+ "Achtung für die Messung: mehr springende Bots heißt mehr Ziele in der Luft, und"
			+ " die Vorhersage trifft die viel besser als laufende. Sitzungen mit und ohne diesen"
			+ " Haken sind nicht direkt vergleichbar." );
		hintTip.SetToolTip( botJumper, "Wie gern ein Bot im Kampf springt: bei jedem Denkschritt ein Würfelwurf"
			+ " gegen diese Zahl, höchstens ein Sprung je Sekunde. " + FromCharacter );
		hintTip.SetToolTip( botCroucher, "Wie gern ein Bot sich im Kampf duckt, und wie lange: fünf Sekunden"
			+ " mal diese Zahl. " + FromCharacter );

		return Group( "Bewegung",
			Row( Pad( botEdgeCare ) ),
			Row( Pad( botAirControl ) ),
			Row( Pad( botDodge ) ),
			Row( Pad( botUnstuck ) ),
			Row( Labelled( "Raketensprung:", botRocketJumpMode ) ),
			Row( Pad( botJump ) ),
			Row( Labelled( "Sprungfreude im Kampf:", botJumper ) ),
			Row( Labelled( "Ducken im Kampf:", botCroucher ) ) );
	}

	GroupBox BuildBotFightBox() {
		hintTip.SetToolTip( botFightUp, "BotAggression gibt null zurück, sobald der Gegner mehr als 200 Einheiten"
			+ " höher steht – noch bevor Waffe oder Munition angesehen werden – und der Bot zieht"
			+ " sich zurück. Auf q3dm17 ist das fast jeder Kampf. Mit dem Haken gilt die Grenze nur"
			+ " noch für Waffen, mit denen nach oben nichts auszurichten ist." );
		hintTip.SetToolTip( botBrave, "Im Original zieht sich ein Bot zurück, sobald er fünf Raketen oder"
			+ " weniger hat oder unter 60 Leben fällt – gemessen ist er drei Viertel der Zeit auf dem"
			+ " Rückzug. Mit Haken zählt, ob er eine Waffe mit Munition hat und ob Leben und Rüstung"
			+ " zusammen reichen (70, die Rüstung zu zwei Dritteln gerechnet). Wer das Quad trägt, dem"
			+ " stellt sich keiner, der es nicht selbst hat.\n\n"
			+ "Gemessen macht das die Bots leichter: auf q3dm17, Stufe 5, kam ein starker Spieler"
			+ " gegen zehn solche Bots auf mehr Abschüsse je Tod als ohne. Deshalb bei „Menschlich“ aus." );
		hintTip.SetToolTip( botHear, "Die Bots sind im Original taub: jedes Geräusch fällt in einen leeren"
			+ " Zweig. Mit Haken bemerkt ein Bot, wer in Hörweite schießt, springt oder landet – auch"
			+ " hinter seinem Rücken. Sehen muss er ihn trotzdem können." );
		hintTip.SetToolTip( botSteady, "Kampf, Rückzug und Verfolgung hängen im Original an derselben"
			+ " Schwelle und kippen bei jedem Denkschritt neu – gemessen 17 Hin-und-Her je Bot und"
			+ " Minute, mit 1,5 Sekunden noch 5. Eine getroffene Entscheidung gilt so viele Sekunden –"
			+ " ein Kampf aber nur, solange der Bot dabei nicht getroffen wird, und ein Rückzug nur, bis"
			+ " er deutlich stärker geworden ist (25 Leben und Rüstung). Verschwindet der Gegner"
			+ " hinter einer Ecke, bleibt der Bot vier Zehntelsekunden beim Kampf, statt sofort die"
			+ " Verfolgung aufzunehmen. 0 ist das Original." );
		// Der Text des alten Hakens „Bots beweglicher“, ohne dessen ersten Satz:
		// der setzte diese Zahl und das Lagern zusammen, jetzt hat jedes sein Feld.
		hintTip.SetToolTip( botAttackSkill, "Diese eine Zahl ist die ganze Leiter der Kampfbewegung: unter 0,2"
			+ " steht der Bot still, bis 0,4 läuft er nur geradeaus vor und zurück, erst darüber"
			+ " umkreist er, und erst über 0,7 mit dem zufälligen Rhythmus, den ein Mensch hat. "
			+ FromCharacter + "\n\n"
			+ "Achtung: diese Zahl und das Lagern ändern, wie sich die Bots bewegen – also das,"
			+ " wogegen die Vorhersage gemessen wird. Neue Basislinie nötig, bevor du gegen alte"
			+ " Zahlen vergleichst." );
		hintTip.SetToolTip( botReaction, "So lange nach dem ersten Blick auf den Gegner schießt ein Bot noch"
			+ " nicht, in Sekunden. " + FromCharacter );
		hintTip.SetToolTip( botAimAccuracy, "Wie genau ein Bot den Punkt trifft, auf den er zielt: je weiter"
			+ " unter 1, desto mehr streut er. " + FromCharacter );
		hintTip.SetToolTip( botAimSkill, "Wie gut ein Bot vorhält, also die Bewegung des Gegners und die"
			+ " Flugzeit des Geschosses einrechnet. " + FromCharacter );
		hintTip.SetToolTip( botAlertness, "Wie weit ein Bot einen Gegner bemerkt: 900 Einheiten und dazu 4000"
			+ " mal diese Zahl. " + FromCharacter );
		hintTip.SetToolTip( botFireThrottle, "Wie ein Bot sein Feuer einteilt: Salven und Pausen dazwischen."
			+ " Am längsten sind die Pausen bei 0,5; bei 0 und bei 1 schießt er durch. " + FromCharacter );
		hintTip.SetToolTip( botChallenge, "Der Schalter aus dem Original: die Sicht schwenkt ohne Überschwingen,"
			+ " und ein Bot mit hoher Zielgenauigkeit, der den Gegner eine Sekunde im Blick hat, setzt"
			+ " sie geradewegs auf den Punkt." );

		return Group( "Kampf",
			Row( Pad( botFightUp ) ),
			Row( Pad( botBrave ) ),
			Row( Pad( botHear ) ),
			Row( Labelled( "Entscheidung halten (s):", botSteady ) ),
			Row( Labelled( "Kampfbewegung:", botAttackSkill ) ),
			Row( Labelled( "Reaktionszeit (s):", botReaction ) ),
			Row( Labelled( "Zielgenauigkeit:", botAimAccuracy ) ),
			Row( Labelled( "Zielkönnen (Vorhalt):", botAimSkill ) ),
			Row( Labelled( "Wachsamkeit:", botAlertness ) ),
			Row( Labelled( "Feuerdisziplin:", botFireThrottle ) ),
			Row( Pad( botChallenge ) ) );
	}

	GroupBox BuildBotGoalBox() {
		hintTip.SetToolTip( botRethink, "Ein Bot sperrt sein Fernziel für zwanzig Sekunden, und Schaden löst die"
			+ " Sperre nirgends – wer von hundert auf dreißig fällt, holt weiter die Waffe, die sein"
			+ " gesundes Ich ausgesucht hat. Die Gewichte sind sehr wohl gesundheitsabhängig, sie"
			+ " werden nur nie neu ausgewertet. Ab 25 Schaden wird die Sperre gelöst, höchstens alle"
			+ " zwei Sekunden." );
		hintTip.SetToolTip( botTiming, "Das Spielmodul kennt den Wiederkehr-Zeitpunkt jedes Gegenstands auf die"
			+ " Millisekunde und gibt ihn nie weiter. Ein Bot merkt sich nur, was er SELBST genommen"
			+ " hat – nimmst du das Quad, laufen sie weiter zu der leeren Stelle, und wenn es"
			+ " wiederkommt, steht keiner dort. Mit dem Haken erfährt jeder Bot jede Aufnahme.\n\n"
			+ "Auf Powerups, Rüstung, die großen Medipacks und eine Waffe, die er noch nicht hat, geht"
			+ " er zwei Sekunden vorher los und wartet dort – das Original hält eine leere Stelle für"
			+ " „Ziel erledigt“ und dreht ab. Und was ein Toter fallen ließ, gilt als weg, sobald es"
			+ " jemand genommen hat." );
		hintTip.SetToolTip( botGrab, "Im reinen Kampf nimmt ein Bot im Original nichts auf, auch nicht die"
			+ " Rüstung zwei Schritte neben ihm." );
		hintTip.SetToolTip( botHunt, "Hat ein Bot keinen Gegner und ist gut ausgestattet, geht er dorthin, wo"
			+ " er zuletzt etwas gehört hat, statt zur nächsten Munitionskiste. Braucht das Hören." );
		hintTip.SetToolTip( botVariety, "Die Zielwahl ist im Original streng: bestes Gewicht durch Wegzeit,"
			+ " immer. Alle Bots laufen deshalb dieselben Wege. Mit 25 % nimmt ein Bot jedes vierte Mal"
			+ " das zweitbeste Ziel." );
		hintTip.SetToolTip( botCamper, "Wie gern ein Bot an einer Stelle lagert, statt zu laufen – und Lagern"
			+ " ist Stillstand. Unter 0,1 lagert er nie. " + FromCharacter );
		hintTip.SetToolTip( botDroppedWeight, "Was einem Bot ein Gegenstand zusätzlich wert ist, den ein Toter"
			+ " fallen ließ. Im Original tausend Punkte – ein Quad wiegt vierhundert –, und deshalb"
			+ " läuft jeder Bot jeder fallengelassenen Waffe nach, auch wenn er sie schon hat. Gemessen"
			+ " gingen damit zwei von drei Zielwahlen an Fallengelassenes. Bei 0 zählt nur noch, was der"
			+ " Gegenstand selbst wert ist. Wird beim Laden der Karte gelesen." );

		return Group( "Ziele und Gegenstände",
			Row( Pad( botRethink ) ),
			Row( Pad( botTiming ) ),
			Row( Pad( botGrab ) ),
			Row( Pad( botHunt ) ),
			Row( Labelled( "zweitbestes Ziel nehmen (%):", botVariety ) ),
			Row( Labelled( "Lagern:", botCamper ) ),
			Row( Labelled( "Aufschlag für Fallengelassenes:", botDroppedWeight ) ) );
	}

	GroupBox BuildBotMiscBox() {
		hintTip.SetToolTip( botNoChat, "Ein Bot, der etwas sagt, steht dafür genau zwei Sekunden völlig still –"
			+ " AINode_Stand gibt keinen einzigen Bewegungsbefehl. Ausgelöst wird das unter"
			+ " anderem durch \"Gegner tot\" mitten im Gefecht, durch Treffer und durch reinen"
			+ " Zufall. Mit diesem Haken reden sie nicht mehr und bleiben in Bewegung." );
		hintTip.SetToolTip( botLog, "Schreibt je Denkschritt und Bot eine Zeile nach botlog.log im"
			+ " Homeverzeichnis (baseq3). Ausgewertet wird mit tools/botlog/botlog.pl. Zehn Bots"
			+ " schreiben rund 400 Kilobyte je Minute." );

		return Group( "Sonstiges",
			Row( Pad( botNoChat ) ),
			Row( Pad( botLog ) ) );
	}

	// Der Zusatz, der an jedem Charakterwert hängt. Einmal hier, damit er
	// überall gleich lautet - das Minus ist das echte, nicht der Bindestrich.
	const string FromCharacter = "−1 lässt den Wert aus der Charakterdatei stehen.";

	// Ohne das Hören gibt es keinen Lärm, zu dem ein Bot gehen könnte. Der
	// Haken wird grau und behält seinen Stand, damit er wieder da ist, wenn das
	// Hören zurückkommt; in die Config geht so lange 0.
	void UpdateBotEnabled() {
		botHunt.Enabled = botHear.Checked;
	}

	bool BotHuntOn => botHear.Checked && botHunt.Checked;

	// Was ein Regler gerade gilt, in der Zählung der Tabelle. Beim Hingehen zum
	// Lärm ist das nicht der Haken, sondern das, was in die Config geht: ein
	// angehakter, aber grauer Haken ist aus.
	decimal BotValue( Control box ) => box switch {
		CheckBox check => ( check == botHunt ? BotHuntOn : check.Checked ) ? 1 : 0,
		ComboBox list => list.SelectedIndex,
		NumericUpDown number => number.Minimum < 0m && number.Value < 0m ? -1m : number.Value,
		_ => 0,
	};

	// Die Tabelle steht in der Reihenfolge der Karte, das Hören also vor dem
	// Hingehen zum Lärm - und die Zeile rechnet nach jedem einzelnen Regler
	// neu, zeigt unterwegs also Zwischenstände. Gezeichnet wird keiner davon,
	// das Fenster kommt erst nach dem letzten wieder an die Reihe.
	void ApplyBotPreset( bool human ) {
		foreach ( var p in BotPresets ) {
			decimal want = human ? p.Human : p.Stock;
			if ( p.Box is CheckBox check ) check.Checked = want != 0;
			else if ( p.Box is ComboBox list ) list.SelectedIndex = (int)want;
			// Value wirft außerhalb der Grenzen des Felds
			else if ( p.Box is NumericUpDown number ) number.Value = Math.Clamp( want, number.Minimum, number.Maximum );
		}
	}

	// Welcher Stand gerade gilt. Rechnet direkt und ohne BeginInvoke, weil das
	// auch aus LoadSettings heraus läuft, bevor das Fenster ein Handle hat.
	void ShowBotPreset() {
		bool stock = true, human = true;
		foreach ( var p in BotPresets ) {
			decimal now = BotValue( p.Box );
			if ( now != p.Stock ) stock = false;
			if ( now != p.Human ) human = false;
		}
		botPresetValue.Text = stock ? "Standard Q3 – die Bots wie im Original"
			: human ? "Menschlich – die empfohlenen Werte"
			: "eigene Einstellung";
		// Goldgelb wie die anderen Zeilen dieser Art, sobald etwas vom Spiel
		// abweicht - eine Sitzung mit solchen Bots ist mit den alten nicht
		// direkt vergleichbar.
		botPresetValue.ForeColor = stock ? Color.DimGray : Color.DarkGoldenrod;
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
		hintTip.SetToolTip( aimAutoFire, "Solange die Zieltaste hält, drückt der Abzug von selbst ab, sobald zweierlei"
 			+ " stimmt: es steht nichts zwischen Mündung und Zielpunkt, und die Sicht liegt bereits innerhalb"
 			+ " des Winkels, den der Körper auf dieser Entfernung einnimmt - sie schwenkt also nicht mehr"
 			+ " dorthin, sie ist da. Auf 500 Einheiten sind das 1,7 Grad für Hitscan, für Rakete und Granate"
 			+ " entsprechend mehr, weil der Splash zählt. Die Entfernung begrenzt er NICHT - gemessen trafen"
 			+ " jenseits von 1600 Einheiten zwar nur 8 % der Raketen, aber eine Grenze darauf ließ nur noch"
 			+ " 6 von 55 Gelegenheiten übrig, und das war spürbar schlechter. Wer sie doch will, nimmt den"
 			+ " Regler „auch aussichtslose“ darunter: 3,0 schneidet ab etwa 1600 Einheiten ab, 2,0 ab 1200."
 			+ " „Nicht ins Leere schießen“ darf den Schuss weiter"
 			+ " wegnehmen; dieser Haken gibt ihn nur. Achtung für die Messung: damit werden schlechte"
 			+ " Gelegenheiten gar nicht erst abgedrückt, die Trefferquote steigt also schon deshalb."
 			+ " Das Protokoll vermerkt es im Kopf." );

		return Group( "Zielhilfe",
			Row( Pad( aimAssist ) ),
			Row( Labelled( "Halten:", aimKey ), Labelled( "Snap-Stärke:", aimStrength ) ),
			Row( Labelled( "Schussmoment exakt:", aimExact ) ),
			Row( Pad( aimFreeze ) ),
			Row( Pad( aimAttacker ) ),
			Row( Pad( aimAutoFire ) ),
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

	// Zielsuch-Raketen, nach Anup Shindes Mod von 2007 - aber neu gebaut, weil
	// dessen Code abstürzt (Raketen ohne Selbstzünder, Kartenschützen ohne
	// client). Eine Spielregel für die Welt; mit allen Reglern eine eigene Karte.
	// Diese Gruppe sagt, WAS die Rakete sucht und wie sie lenkt.
	GroupBox BuildHomingBox() {
		hintTip.SetToolTip( homingMode, "Raketen suchen sich ein Ziel im Blickkegel und drehen darauf zu."
 			+ " „Nur meine“ heißt: die Raketen der Menschen, Bots schießen normal. „Alle“ heißt, auch"
 			+ " die Bots schießen Zielsuch-Raketen – auf dich." );
		hintTip.SetToolTip( homingTurn, "Wie schnell die Rakete drehen kann, in Grad je Sekunde. Das ist der"
 			+ " Wendekreis: je höher, desto enger – und desto schwerer auszuweichen. Mit dem Kurvenverlust"
 			+ " unten kostet jede Drehung Tempo." );
		hintTip.SetToolTip( homingCone, "Wie weit die Rakete zur Seite schaut, um ein Ziel zu finden – der"
 			+ " halbe Öffnungswinkel. 45° ist ein Blick nach vorn, 180° sieht auch nach hinten." );
		hintTip.SetToolTip( homingPick, "Welches Ziel sie nimmt, wenn mehrere im Blickkegel sind. „Im"
 			+ " Fadenkreuz“ ist der kleinste Winkel zur Flugrichtung – beim Abschuss also, wohin du"
 			+ " gezielt hast. „Am leichtesten zu töten“ rechnet Leben und Rüstung zusammen, so wie"
 			+ " Quake 3 den Schaden verteilt. „Wer mich zuletzt getroffen hat“ zählt nur Gegner –"
 			+ " eigener Splash, ein Raketensprung oder ein Sturz löschen die Erinnerung nicht." );
		hintTip.SetToolTip( homingAir, "Luftabwehr: nur wer gerade springt oder fliegt – gegen"
 			+ " Raketenspringer. Gilt beim Aussuchen; wer schon verfolgt wird und kurz aufsetzt,"
 			+ " bleibt verfolgt." );
		hintTip.SetToolTip( homingMissiles, "Raketen jagen Raketen. Zwei, die sich näher als 40 Einheiten"
 			+ " kommen, platzen beide – mit Näherungszünder auch weiter, aber höchstens 120, so weit die"
 			+ " eigene Explosion reicht. Das ist die einzige Art, eine Rakete abzuschießen, denn Raketen"
 			+ " haben keinen Körper. Die eigenen und die der Mitspieler sind nie Ziel." );
		hintTip.SetToolTip( homingRetarget, "Ohne Haken hält die Rakete ihr Ziel, solange sie es sieht. Mit"
 			+ " Haken sucht sie jedes Bild neu und schwenkt auf ein besseres um." );
		hintTip.SetToolTip( homingLead, "0 % fliegt dem Ziel hinterher. 100 % fliegt dorthin, wo sich"
 			+ " Rakete und Ziel treffen, wenn beide so weiterlaufen – seitliches Ausweichen hilft dann"
 			+ " kaum noch. Wer springt, fällt in der Rechnung mit." );
		hintTip.SetToolTip( homingArm, "So lange fliegt sie nach dem Abschuss geradeaus: keine Zielsuche,"
 			+ " kein Näherungszünder. Aus der Nähe muss man dann noch selbst zielen." );
		hintTip.SetToolTip( homingFuel, "Die Lenkkraft nimmt gleichmäßig ab und ist nach so vielen"
 			+ " Sekunden weg; danach fliegt sie geradeaus. Wer lange genug davonläuft, entkommt."
 			+ " 0 heißt: lenkt, bis sie platzt." );

		return Group( "Zielsuch-Raketen: Ziel und Lenkung",
			Row( Labelled( "Zielsuche:", homingMode ) ),
			Row( Labelled( "Drehrate:", homingTurn ), homingTurnDefault ),
			Row( Pad( homingTurnValue ) ),
			Row( Labelled( "Blickkegel (°):", homingCone ) ),
			Row( Labelled( "Ziel:", homingPick ) ),
			Row( Labelled( "Wer:", homingAir ) ),
			Row( Labelled( "Jagt:", homingMissiles ) ),
			Row( Pad( homingRetarget ) ),
			Row( Labelled( "Vorhalt:", homingLead ) ),
			Row( Pad( homingLeadValue ) ),
			Row( Labelled( "Schärfzeit (ms):", homingArm ), Labelled( "Treibstoff (s):", homingFuel ) ),
			Row( Pad( homingSteerValue ) ),
			Row( Pad( homingFuelValue ) ) );
	}

	// Diese Gruppe sagt, WIE sie fliegt und wie sie endet.
	GroupBox BuildHomingFlightBox() {
		hintTip.SetToolTip( homingSpeedStart, "Tempo beim Abschuss. 900 ist jede Rakete in Quake 3." );
		hintTip.SetToolTip( homingSpeedEnd, "Tempo, auf das sie in der angegebenen Zeit gleichmäßig"
 			+ " kommt. Start und Ende gleich heißt: festes Tempo." );
		hintTip.SetToolTip( homingDrag, "Enge Kurven kosten Tempo, der Motor holt es danach wieder auf."
 			+ " Bei 100 % halbiert eine Vierteldrehung das Tempo, wenn nichts nachkommt. Wer scharf"
 			+ " ausweicht, bremst die Rakete aus." );
		hintTip.SetToolTip( homingLife, "Wie lange eine Zielsuch-Rakete fliegt, bevor sie sich in der Luft"
 			+ " zerlegt – mit vollem Splash, dort wo sie gerade ist. 15 s ist der Selbstzünder, den jede"
 			+ " Rakete schon immer hat. Kürzer heißt: wer lange genug ausweicht, überlebt sie." );
		hintTip.SetToolTip( homingSplit, "Statt sich am Ende zu zerlegen, zerfällt sie in kleine Raketen,"
 			+ " die reihum 25° aus der Flugrichtung weiterfliegen und selbst suchen. Jeder Splitter hat"
 			+ " den halben Schaden; Splitter zerfallen nicht noch einmal." );
		hintTip.SetToolTip( homingProx, "Zündet im Vorbeiflug, sobald die Rakete der Körpermitte ihres Ziels"
 			+ " so nahe kommt – auch ohne Volltreffer, mit dem Splash. Der reicht 120 Einheiten ab dem"
 			+ " Körperrand, seitlich also gut 136 ab der Mitte. 0 heißt aus. Vorsicht ohne Schärfzeit:"
 			+ " steht ein Gegner direkt vor dir, platzt sie in deinem Gesicht." );
		hintTip.SetToolTip( homingWarn, "Ein Piepen, das nur der Verfolgte hört – dichter, je näher die"
 			+ " Rakete kommt. Wichtig, wenn die Bots Zielsuch-Raketen schießen." );

		return Group( "Zielsuch-Raketen: Flug und Zünder",
			Row( Labelled( "Tempo Start:", homingSpeedStart ), Labelled( "Ende:", homingSpeedEnd ) ),
			Row( Labelled( "Übergang (s):", homingSpeedRamp ) ),
			Row( Pad( homingSpeedValue ) ),
			Row( Labelled( "Kurvenverlust:", homingDrag ) ),
			Row( Pad( homingDragValue ) ),
			Row( Labelled( "Lebensdauer:", homingLife ), homingLifeDefault ),
			Row( Pad( homingLifeValue ) ),
			Row( Labelled( "Am Ende:", homingSplit ) ),
			Row( Labelled( "Näherungszünder:", homingProx ) ),
			Row( Pad( homingProxValue ) ),
			Row( Pad( homingWarn ) ) );
	}

	// Der Wert von g_homingSplit zur Auswahl: keine, dann 2, 3, 4.
	int HomingSplitCount => homingSplit.SelectedIndex <= 0 ? 0 : homingSplit.SelectedIndex + 1;

	// Wie weit sie in t Sekunden kommt: das Tempo steigt über die Rampe
	// gleichmäßig von Start auf Ende, danach bleibt es - Fläche unter der Kurve.
	double HomingDistance( double t ) {
		double v0 = (double)homingSpeedStart.Value, v1 = (double)homingSpeedEnd.Value;
		double ramp = (double)homingSpeedRamp.Value;
		return t <= ramp
			? v0 * t + ( v1 - v0 ) * t * t / ( 2.0 * ramp )
			: ( v0 + v1 ) * ramp / 2.0 + v1 * ( t - ramp );
	}

	// Wo sie nach t Sekunden wirklich ist: fire_rocket schickt jede Rakete mit
	// 50 ms Vorlauf los (MISSILE_PRESTEP_TIME), beim Starttempo.
	const double HomingPrestep = 0.05, HomingServerFrame = 0.05;
	double HomingFlown( double t ) => (double)homingSpeedStart.Value * HomingPrestep + HomingDistance( t );

	// Grad je Sekunde sagen wenig; der Wendekreis sagt, ob man ausweichen kann.
	// Ein Mensch läuft 320 Einheiten je Sekunde - ist der Radius deutlich
	// kleiner als das, kommt man einer Rakete kaum noch davon. Der Radius ist
	// Tempo durch Drehrate, bei einem Tempoprofil also zwei Zahlen.
	void ShowHoming() {
		bool on = homingMode.SelectedIndex > 0;
		Color live = on ? Color.DarkGoldenrod : Color.DimGray;
		int deg = homingTurn.Value;
		double v0 = (double)homingSpeedStart.Value, v1 = (double)homingSpeedEnd.Value;
		double rad = deg * Math.PI / 180.0;
		double r0 = v0 / rad, r1 = v1 / rad, radius = Math.Max( r0, r1 );
		string feel = radius > 500 ? "weit, gut auszuweichen"
			: radius > 200 ? "mittel"
			: radius > 100 ? "eng, schwer auszuweichen"
			: "sehr eng, kaum zu entkommen";
		homingTurnValue.Text = v0 == v1
			? $"{deg} °/s – Wendekreis {r0:0} Einheiten ({feel})"
			: $"{deg} °/s – Wendekreis {r0:0} bis {r1:0} Einheiten ({feel})";

		int lead = homingLead.Value;
		homingLeadValue.Text = lead == 0 ? "0 % – fliegt dem Ziel hinterher"
			: lead == 100 ? "100 % – auf den Treffpunkt"
			: $"{lead} % – zwischen Ziel und Treffpunkt";

		// Die Schärfzeit prüft das Spiel einmal je Serverbild nach der Bewegung,
		// also greift sie erst im ersten Bild, das sie erreicht hat.
		double arm = (double)homingArm.Value / 1000.0, fuel = (double)homingFuel.Value;
		double armAt = Math.Ceiling( arm / HomingServerFrame - 1e-9 ) * HomingServerFrame;
		homingSteerValue.Text = arm > 0
			? $"Schärfzeit: die ersten {HomingFlown( armAt ):0} Einheiten geradeaus"
			: "Schärfzeit: keine, sucht sofort";
		homingFuelValue.Text = fuel > 0
			? $"Treibstoff: halbe Lenkkraft nach {fuel / 2:0.0} s, keine nach {fuel:0.0} s"
			: "Treibstoff: lenkt bis zum Ende";

		double ramp = (double)homingSpeedRamp.Value;
		homingSpeedValue.Text = v0 == v1
			? ( v0 == 900 ? "900 u/s – wie jede Rakete" : $"fest {v0:0} u/s, jede Rakete sonst 900" )
			: $"{v0:0} → {v1:0} u/s in {ramp:0.0} s" + ( v1 > v0 ? ", wird schneller" : ", wird langsamer" );

		int drag = homingDrag.Value;
		homingDragValue.Text = drag == 0 ? "0 % – Kurven kosten nichts"
			: $"{drag} % – eine Vierteldrehung kostet ohne Nachschub"
				+ $" {100 - 100 * Math.Exp( -0.4413 * drag / 100.0 * Math.PI / 2 ):0} % Tempo";

		// Die Strecke bis zum Zerlegen, über das Tempoprofil gerechnet.
		double life = homingLife.Value * 0.5;
		homingLifeValue.Text = life >= 15.0
			? "15,0 s – wie immer, zerlegt sich praktisch nie in der Luft"
			: $"{life:0.0} s – höchstens {HomingFlown( life ):0} Einheiten Flug";

		// Der Zünder misst zur Körpermitte, der Splash (120) zum Rand der Box -
		// seitlich 16 Einheiten davor. Was bei einem Vorbeiflug auf Körperhöhe
		// noch ankommt, so wie G_RadiusDamage es rechnet.
		int prox = (int)homingProx.Value;
		double edge = Math.Max( 0, prox - 16 );
		int sideDamage = edge >= 120 ? 0 : (int)( 100 * ( 1 - edge / 120.0 ) );
		homingProxValue.Text = prox == 0
			? ( homingMissiles.SelectedIndex > 0
				? "aus – gegen Raketen trotzdem ab 40 Einheiten"
				: "aus – nur Volltreffer und Wände" )
			: sideDamage > 0
				? $"{prox} vor der Körpermitte, seitlich noch ~{sideDamage} Schaden"
				: $"{prox} vor der Körpermitte – zu weit, kein Schaden";

		foreach ( var l in new[] { homingTurnValue, homingLeadValue, homingSteerValue, homingFuelValue,
				homingSpeedValue, homingDragValue, homingLifeValue, homingProxValue } ) {
			l.ForeColor = live;
		}
		foreach ( var c in new Control[] { homingTurn, homingTurnDefault, homingCone, homingRetarget,
				homingLife, homingLifeDefault, homingPick, homingAir, homingMissiles, homingLead,
				homingArm, homingFuel, homingSpeedStart, homingSpeedEnd, homingSpeedRamp, homingDrag,
				homingSplit, homingProx, homingWarn } ) {
			c.Enabled = on;
		}
	}

	// ZTMs Flexible HUD. Seit der Zusammenführung steckt er in unserem eigenen
	// cgame, das aus zz-hitpitch.pk3 geladen wird - vorher gewann das cgame aus
	// ztm-flexible-hud-r8-baseq3.pk3 und diese Karte hätte Cvars angeboten, die
	// unser Modul gar nicht kennt.
	GroupBox BuildHudBox() {
		hintTip.SetToolTip( hudFovAspect, "Rechnet das Sichtfeld aufs Seitenverhältnis um, statt es zu strecken."
 			+ " Auf einem Breitbildschirm siehst du damit links und rechts mehr, statt oben und unten"
 			+ " beschnitten zu werden - die eigentliche Hauptsache des Mods. Deine Konfiguration hat das"
 			+ " bereits an; aus heißt Quake-3-Original." );
		hintTip.SetToolTip( hudFovGun, "Bei hohem Sichtfeld rutscht die Waffe tiefer, bei niedrigem weiter nach"
 			+ " vorn. Ohne das wandert sie beim Zoomen aus dem Bild." );
		hintTip.SetToolTip( hudStretch, "Zieht die Anzeige über die volle Breite statt sie im 4:3-Bereich zu"
 			+ " verankern. Meist unerwünscht auf Breitbild - deshalb aus." );
		hintTip.SetToolTip( hudStatusScale, "Größe der Zahlen für Leben, Rüstung und Munition. 1,00 ist das"
 			+ " Original; kleiner macht Platz, größer liest sich auf Abstand besser." );
		hintTip.SetToolTip( hudWeaponBar, "Die Leiste mit den besessenen Waffen unten in der Mitte." );
		hintTip.SetToolTip( hudStatusHead, "Der Kopf des eigenen Modells zwischen Leben und Rüstung." );

		return Group( "HUD (ZTM Flexible HUD r8)",
			Row( Pad( hudFovAspect ) ),
			Row( Pad( hudFovGun ) ),
			Row( Pad( hudWeaponBar ) ),
			Row( Pad( hudStatusHead ) ),
			Row( Pad( hudPickups ) ),
			Row( Pad( hudScores ) ),
			Row( Pad( hudStretch ) ),
			Row( Labelled( "Größe der Statuszahlen:", hudStatusScale ) ) );
	}

	// Bewegung und Spielgefühl von Quake Live. Beides geht über
	// CVAR_SYSTEMINFO an beide Module, damit die Vorhersage des Clients mit
	// dem Server rechnet - sonst ruckelte es bei jedem Sprung.
	GroupBox BuildQuakeLiveBox() {
		hintTip.SetToolTip( qlAutoHop, "Gehaltene Sprungtaste springt weiter, statt auf das Loslassen zu warten."
 			+ " So macht es Quake Live (in den Regelsätzen, die es anbieten) und CPMA. Betrifft alle"
 			+ " gleich, Bots eingeschlossen - es ist die Physik der Welt, keine Messeinstellung." );
		hintTip.SetToolTip( qlWeaponSwitch, "Quake 3 steckt in 200 ms weg und nimmt in 250 ms hoch, zusammen 450."
 			+ " Quake Live nimmt in 200 hoch, also 400. Der Unterschied sitzt allein im Hochnehmen." );

		hintTip.SetToolTip( qlAirControl, "Dreht den Schwung im Sprung in die Blickrichtung, statt zu"
 			+ " beschleunigen - der Betrag bleibt, die Richtung wandert. Wirkt nur, solange du geradeaus"
 			+ " oder gerade rückwärts drückst; seitwärts bleibt Strafejump unverändert. In id Softwares"
 			+ " eigenen Factories setzt das NUR Race, kein Duell und kein Clan Arena." );
		hintTip.SetToolTip( qlAirControlValue, "Die Stärke als Faktor auf die von CPMA. 1,00 ist der Wert aus"
 			+ " den Race-Factories: bei 400 u/s dreht der Schwung bis gut 260 Grad je Sekunde." );
		hintTip.SetToolTip( qlAirAccel, "Luftbeschleunigung. 1,00 ist Quake 3; PQL geht auf 2. Gilt auch ohne"
 			+ " den Haken Luftsteuerung - sie beschleunigt, die Luftsteuerung dreht." );
		hintTip.SetToolTip( qlRampJump, "Quake 3 überschreibt beim Sprung die Aufwärtsgeschwindigkeit - ein"
 			+ " Sprung von einer Schräge frisst also genau den Schwung, den die Schräge gerade gegeben"
 			+ " hat. Damit wird er stattdessen behalten und der Sprung darauf gelegt, gedeckelt bei 700." );
		hintTip.SetToolTip( qlRampScale, "Womit der vorhandene Aufwärtsschwung vorher multipliziert wird."
 			+ " Die Factories benutzen 1,25 und 1,75." );
		hintTip.SetToolTip( qlStepHeight, "Wie hohe Stufen ohne Sprung genommen werden. 18 ist Quake 3"
 			+ ". Quake Live geht in einzelnen Spieltypen auf 20 oder 28; unter 16 nimmt man keine normale Treppe mehr." );

		return Group( "Quake Live",
			Row( Pad( qlAutoHop ) ),
			Row( Pad( qlWeaponSwitch ) ),
			Row( Pad( qlAirControl ) ),
			Row( Labelled( "Stärke:", qlAirControlValue ), Labelled( "Luftbeschleunigung:", qlAirAccel ) ),
			Row( Pad( qlRampJump ) ),
			Row( Labelled( "Faktor:", qlRampScale ), Labelled( "Schritthöhe:", qlStepHeight ) ) );
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
		s.AppendLine( "aimAutoFire=" + aimAutoFire.Checked );
		s.AppendLine( "infiniteAmmo=" + infiniteAmmo.SelectedIndex );
		s.AppendLine( "qlAutoHop=" + qlAutoHop.Checked );
		s.AppendLine( "qlWeaponSwitch=" + qlWeaponSwitch.Checked );
		s.AppendLine( "qlAirControl=" + qlAirControl.Checked );
		s.AppendLine( "qlAirControlValue=" + Dec( qlAirControlValue.Value ) );
		s.AppendLine( "qlAirAccel=" + Dec( qlAirAccel.Value ) );
		s.AppendLine( "qlRampJump=" + qlRampJump.Checked );
		s.AppendLine( "qlRampScale=" + Dec( qlRampScale.Value ) );
		s.AppendLine( "qlStepHeight=" + Dec( qlStepHeight.Value ) );
		s.AppendLine( "hudFovAspect=" + hudFovAspect.Checked );
		s.AppendLine( "hudFovGun=" + hudFovGun.Checked );
		s.AppendLine( "hudWeaponBar=" + hudWeaponBar.Checked );
		s.AppendLine( "hudStatusHead=" + hudStatusHead.Checked );
		s.AppendLine( "hudPickups=" + hudPickups.Checked );
		s.AppendLine( "hudScores=" + hudScores.Checked );
		s.AppendLine( "hudStretch=" + hudStretch.Checked );
		s.AppendLine( "hudStatusScale=" + Dec( hudStatusScale.Value ) );
		// Die Karte „Bots“ in ihrer Reihenfolge, ein Schlüssel je Regler. Die
		// alten Schlüssel botMoveSkill und botRocketJump werden nicht mehr
		// geschrieben; gelesen werden sie noch, solange die neuen fehlen.
		s.AppendLine( "botEdgeCare=" + botEdgeCare.Checked );
		s.AppendLine( "botAirControl=" + botAirControl.Checked );
		s.AppendLine( "botDodge=" + botDodge.Checked );
		s.AppendLine( "botUnstuck=" + botUnstuck.Checked );
		s.AppendLine( "botRocketJumpMode=" + botRocketJumpMode.SelectedIndex );
		s.AppendLine( "botJump=" + botJump.Checked );
		s.AppendLine( "botJumper=" + Dec( botJumper.Value ) );
		s.AppendLine( "botCroucher=" + Dec( botCroucher.Value ) );
		s.AppendLine( "botFightUp=" + botFightUp.Checked );
		s.AppendLine( "botBrave=" + botBrave.Checked );
		s.AppendLine( "botHear=" + botHear.Checked );
		s.AppendLine( "botSteady=" + Dec( botSteady.Value ) );
		s.AppendLine( "botAttackSkill=" + Dec( botAttackSkill.Value ) );
		s.AppendLine( "botReaction=" + Dec( botReaction.Value ) );
		s.AppendLine( "botAimAccuracy=" + Dec( botAimAccuracy.Value ) );
		s.AppendLine( "botAimSkill=" + Dec( botAimSkill.Value ) );
		s.AppendLine( "botAlertness=" + Dec( botAlertness.Value ) );
		s.AppendLine( "botFireThrottle=" + Dec( botFireThrottle.Value ) );
		s.AppendLine( "botChallenge=" + botChallenge.Checked );
		s.AppendLine( "botRethink=" + botRethink.Checked );
		s.AppendLine( "botTiming=" + botTiming.Checked );
		s.AppendLine( "botGrab=" + botGrab.Checked );
		// der Haken selbst, nicht was davon gilt: er soll wieder dastehen,
		// wenn das Hören zurückkommt
		s.AppendLine( "botHunt=" + botHunt.Checked );
		s.AppendLine( "botVariety=" + (int)botVariety.Value );
		s.AppendLine( "botCamper=" + Dec( botCamper.Value ) );
		s.AppendLine( "botDroppedWeight=" + (int)botDroppedWeight.Value );
		s.AppendLine( "botNoChat=" + botNoChat.Checked );
		s.AppendLine( "botLog=" + botLog.Checked );
		s.AppendLine( "weaponRate=" + weaponRate.Value );
		s.AppendLine( "homingMode=" + homingMode.SelectedIndex );
		s.AppendLine( "homingTurn=" + homingTurn.Value );
		s.AppendLine( "homingCone=" + Dec( homingCone.Value ) );
		s.AppendLine( "homingRetarget=" + homingRetarget.Checked );
		s.AppendLine( "homingLife=" + homingLife.Value );
		s.AppendLine( "homingPick=" + homingPick.SelectedIndex );
		s.AppendLine( "homingAir=" + homingAir.SelectedIndex );
		s.AppendLine( "homingMissiles=" + homingMissiles.SelectedIndex );
		s.AppendLine( "homingLead=" + homingLead.Value );
		s.AppendLine( "homingArm=" + Dec( homingArm.Value ) );
		s.AppendLine( "homingFuel=" + Dec( homingFuel.Value ) );
		s.AppendLine( "homingSpeedStart=" + Dec( homingSpeedStart.Value ) );
		s.AppendLine( "homingSpeedEnd=" + Dec( homingSpeedEnd.Value ) );
		s.AppendLine( "homingSpeedRamp=" + Dec( homingSpeedRamp.Value ) );
		s.AppendLine( "homingDrag=" + homingDrag.Value );
		s.AppendLine( "homingSplit=" + homingSplit.SelectedIndex );
		s.AppendLine( "homingProx=" + Dec( homingProx.Value ) );
		s.AppendLine( "homingWarn=" + homingWarn.Checked );
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
		SetBool( aimAutoFire, v, "aimAutoFire" );
		SetIndex( infiniteAmmo, v, "infiniteAmmo" );
		SetBool( qlAutoHop, v, "qlAutoHop" );
		SetBool( qlWeaponSwitch, v, "qlWeaponSwitch" );
		SetBool( qlAirControl, v, "qlAirControl" );
		SetNum( qlAirControlValue, v, "qlAirControlValue" );
		SetNum( qlAirAccel, v, "qlAirAccel" );
		SetBool( qlRampJump, v, "qlRampJump" );
		SetNum( qlRampScale, v, "qlRampScale" );
		SetNum( qlStepHeight, v, "qlStepHeight" );
		SetBool( hudFovAspect, v, "hudFovAspect" );
		SetBool( hudFovGun, v, "hudFovGun" );
		SetBool( hudWeaponBar, v, "hudWeaponBar" );
		SetBool( hudStatusHead, v, "hudStatusHead" );
		SetBool( hudPickups, v, "hudPickups" );
		SetBool( hudScores, v, "hudScores" );
		SetBool( hudStretch, v, "hudStretch" );
		SetNum( hudStatusScale, v, "hudStatusScale" );
		// Die Karte „Bots“. Ein Schlüssel, der fehlt, lässt seinen Regler auf
		// der Vorgabe, und die ist „Standard Q3“ - eine Datei aus einem älteren
		// Bau macht aus den neuen Reglern also nichts, was vorher nicht galt.
		SetBool( botEdgeCare, v, "botEdgeCare" );
		SetBool( botAirControl, v, "botAirControl" );
		SetBool( botDodge, v, "botDodge" );
		SetBool( botUnstuck, v, "botUnstuck" );
		// Der alte Haken „öfter Raketensprünge“: an war g_botRocketJump 1, aus
		// das Original. „gar nicht“ und „auch ohne Rüstung“ kannte er nicht.
		if ( v.ContainsKey( "botRocketJumpMode" ) ) SetIndex( botRocketJumpMode, v, "botRocketJumpMode" );
		else if ( v.TryGetValue( "botRocketJump", out var oldJump ) ) {
			if ( oldJump.Trim() == "True" ) botRocketJumpMode.SelectedIndex = 2;
			else if ( oldJump.Trim() == "False" ) botRocketJumpMode.SelectedIndex = 1;
		}
		SetBool( botJump, v, "botJump" );
		SetNum( botJumper, v, "botJumper" );
		SetNum( botCroucher, v, "botCroucher" );
		SetBool( botFightUp, v, "botFightUp" );
		SetBool( botBrave, v, "botBrave" );
		SetBool( botHear, v, "botHear" );
		SetNum( botSteady, v, "botSteady" );
		SetNum( botAttackSkill, v, "botAttackSkill" );
		SetNum( botReaction, v, "botReaction" );
		SetNum( botAimAccuracy, v, "botAimAccuracy" );
		SetNum( botAimSkill, v, "botAimSkill" );
		SetNum( botAlertness, v, "botAlertness" );
		SetNum( botFireThrottle, v, "botFireThrottle" );
		SetBool( botChallenge, v, "botChallenge" );
		SetBool( botRethink, v, "botRethink" );
		SetBool( botTiming, v, "botTiming" );
		SetBool( botGrab, v, "botGrab" );
		SetBool( botHunt, v, "botHunt" );
		SetNum( botVariety, v, "botVariety" );
		SetNum( botCamper, v, "botCamper" );
		SetNum( botDroppedWeight, v, "botDroppedWeight" );
		// Der alte Haken „Bots beweglicher“ stellte zwei Zahlen auf einmal:
		// Kampfbewegung 0,9 und Lagern 0. Ohne Haken bleibt beides bei -1, und
		// das ist die Vorgabe. Nach den beiden SetNum, damit es gilt, sobald
		// botAttackSkill fehlt - daran ist die alte Datei zu erkennen.
		if ( !v.ContainsKey( "botAttackSkill" )
			&& v.TryGetValue( "botMoveSkill", out var oldMove ) && oldMove.Trim() == "True" ) {
			botAttackSkill.Value = 0.9m;
			botCamper.Value = 0m;
		}
		SetBool( botNoChat, v, "botNoChat" );
		SetBool( botLog, v, "botLog" );
		SetBar( weaponRate, v, "weaponRate" );
		SetIndex( homingMode, v, "homingMode" );
		SetBar( homingTurn, v, "homingTurn" );
		SetNum( homingCone, v, "homingCone" );
		SetBool( homingRetarget, v, "homingRetarget" );
		SetBar( homingLife, v, "homingLife" );
		SetIndex( homingPick, v, "homingPick" );
		SetIndex( homingAir, v, "homingAir" );
		SetIndex( homingMissiles, v, "homingMissiles" );
		SetBar( homingLead, v, "homingLead" );
		SetNum( homingArm, v, "homingArm" );
		SetNum( homingFuel, v, "homingFuel" );
		SetNum( homingSpeedStart, v, "homingSpeedStart" );
		SetNum( homingSpeedEnd, v, "homingSpeedEnd" );
		SetNum( homingSpeedRamp, v, "homingSpeedRamp" );
		SetBar( homingDrag, v, "homingDrag" );
		SetIndex( homingSplit, v, "homingSplit" );
		SetNum( homingProx, v, "homingProx" );
		SetBool( homingWarn, v, "homingWarn" );
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
			// Nur die Charakterwerte reichen unter null, und dort heißt alles
			// Negative „aus der Charakterdatei“ - eine Datei mit -0,95 lädt als
			// -1 und nicht, vom Feld nach oben gerundet, als Vorgabe 0.
			if ( box.Minimum < 0m && d < 0m ) d = box.Minimum;
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
		// An die Zielhilfe gebunden wie die anderen: ohne sie gibt es kein
		// gefuehrtes Ziel, und ohne gefuehrtes Ziel darf nichts von selbst
		// abdruecken.
		cfg.AppendLine( $"seta cl_aimAssistAutoFire {( aimAssist.Checked && aimAutoFire.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistEdge {( aimEdge.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistDebug {( aimAssist.Checked ? 1 : 0 )}" );
		// Quake-Live-Bewegung. Null heisst bei den Zeiten ausdruecklich
		// "Original", nicht "null Millisekunden" - so liest es bg_pmove.c.
		//
		// Kein seta: die Bewegung ist eine Laboreinstellung wie g_selfDamage und
		// gehoert nicht in die q3config. Mit seta blieb sie dort stehen, und ein
		// Spiel ohne das Labor hatte trotzdem Auto-Hop und Luftsteuerung - und
		// auf einem fremden Server sagte das cgame Bewegungen voraus, die der
		// Server nie machte. Das unset davor raeumt ein Archiv-Flag weg, das ein
		// aelteres Labor dort hinterlassen hat: vor "map" hat noch kein Modul die
		// Cvar registriert, sie ist also noch vom Benutzer angelegt und darf
		// geloescht werden; ein set allein behielte das Flag.
		foreach ( var (name, val) in new[] {
			( "pmove_AutoHop", qlAutoHop.Checked ? "1" : "0" ),
			( "pmove_WeaponDropTime", qlWeaponSwitch.Checked ? "200" : "0" ),
			( "pmove_WeaponRaiseTime", qlWeaponSwitch.Checked ? "200" : "0" ),
			// Die Stärke steht nur dann, wenn der Haken sie freigibt - sonst
			// wäre eine Zahl gesetzt, die nichts tut, und im Protokollkopf
			// stünde eine Bedingung, die nie galt.
			( "pmove_AirControl", qlAirControl.Checked ? Dec( qlAirControlValue.Value ) : "0" ),
			// Beim Originalwert wird 0 geschrieben, nicht die Zahl: die Engine
			// liest 0 als "Original", und der Protokollkopf zeigt dann keine
			// Bedingung an, wo keine ist.
			( "pmove_AirAccel", qlAirAccel.Value == 1.00m ? "0" : Dec( qlAirAccel.Value ) ),
			( "pmove_RampJump", qlRampJump.Checked ? "1" : "0" ),
			( "pmove_RampJumpScale", Dec( qlRampScale.Value ) ),
			( "pmove_StepHeight", qlStepHeight.Value == 18m ? "0" : Dec( qlStepHeight.Value ) ),
		} ) {
			cfg.AppendLine( $"unset {name}" );
			cfg.AppendLine( $"set {name} {val}" );
		}
		// ZTMs Flexible HUD. Alle acht sind CVAR_ARCHIVE, stehen also auch in
		// der q3config - diese Zeilen laufen danach und gewinnen damit.
		cfg.AppendLine( $"seta cg_fovAspectAdjust {( hudFovAspect.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cg_fovGunAdjust {( hudFovGun.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cg_drawWeaponBar {( hudWeaponBar.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cg_drawStatusHead {( hudStatusHead.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cg_drawPickups {( hudPickups.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cg_drawScores {( hudScores.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cg_stretch {( hudStretch.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cg_statusScale {Dec( hudStatusScale.Value )}" );
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
		cfg.AppendLine( $"set g_homingRockets {homingMode.SelectedIndex}" );
		cfg.AppendLine( $"set g_homingTurn {homingTurn.Value}" );
		cfg.AppendLine( $"set g_homingCone {Dec( homingCone.Value )}" );
		cfg.AppendLine( $"set g_homingRetarget {( homingRetarget.Checked ? 1 : 0 )}" );
		cfg.AppendLine( "set g_homingLifetime " + ( homingLife.Value * 0.5 ).ToString( "0.0", System.Globalization.CultureInfo.InvariantCulture ) );
		cfg.AppendLine( $"set g_homingPick {homingPick.SelectedIndex}" );
		cfg.AppendLine( $"set g_homingAir {homingAir.SelectedIndex}" );
		cfg.AppendLine( $"set g_homingMissiles {homingMissiles.SelectedIndex}" );
		cfg.AppendLine( $"set g_homingLead {homingLead.Value}" );
		cfg.AppendLine( $"set g_homingArm {Dec( homingArm.Value )}" );
		cfg.AppendLine( $"set g_homingFuel {Dec( homingFuel.Value )}" );
		cfg.AppendLine( $"set g_homingSpeedStart {Dec( homingSpeedStart.Value )}" );
		cfg.AppendLine( $"set g_homingSpeedEnd {Dec( homingSpeedEnd.Value )}" );
		cfg.AppendLine( $"set g_homingSpeedRamp {Dec( homingSpeedRamp.Value )}" );
		cfg.AppendLine( $"set g_homingDrag {homingDrag.Value}" );
		cfg.AppendLine( $"set g_homingSplit {HomingSplitCount}" );
		cfg.AppendLine( $"set g_homingProximity {Dec( homingProx.Value )}" );
		cfg.AppendLine( $"set g_homingWarn {( homingWarn.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_infiniteAmmo {infiniteAmmo.SelectedIndex}" );
		// Die Karte „Bots“, je Regler eine Zeile und in der Reihenfolge der
		// Karte. Kein seta, wie bei allem hier unten. Jede Zeile wird immer
		// geschrieben, auch beim Original: ein Wert aus der Runde davor bliebe
		// sonst im laufenden Spiel stehen, und „Standard Q3“ wäre keiner.
		cfg.AppendLine( $"set g_botEdgeCare {( botEdgeCare.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botAirControl {( botAirControl.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botDodge {( botDodge.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botUnstuck {( botUnstuck.Checked ? 1 : 0 )}" );
		{
			// Eine Auswahl, zwei Cvars: der Eintrag 0 schaltet den Raketensprung
			// ab, und ab dem Eintrag 1 zählt g_botRocketJump von null an. Ohne
			// Auswahl gilt das Original.
			int mode = botRocketJumpMode.SelectedIndex < 0 ? 1 : botRocketJumpMode.SelectedIndex;
			cfg.AppendLine( $"set bot_rocketjump {( mode > 0 ? 1 : 0 )}" );
			cfg.AppendLine( $"set g_botRocketJump {Math.Max( mode - 1, 0 )}" );
		}
		cfg.AppendLine( $"set g_botJump {( botJump.Checked ? 1 : 0 )}" );
		// -1 lässt einen Wert aus der Charakterdatei stehen. Mit Dec, weil die
		// deutsche Schreibweise ein Komma setzt und das Spiel mit atof liest.
		cfg.AppendLine( $"set g_botJumper {Dec( botJumper.Value )}" );
		cfg.AppendLine( $"set g_botCroucher {Dec( botCroucher.Value )}" );
		cfg.AppendLine( $"set g_botFightUp {( botFightUp.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botBrave {( botBrave.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botHear {( botHear.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botSteady {Dec( botSteady.Value )}" );
		cfg.AppendLine( $"set g_botAttackSkill {Dec( botAttackSkill.Value )}" );
		cfg.AppendLine( $"set g_botReaction {Dec( botReaction.Value )}" );
		cfg.AppendLine( $"set g_botAimAccuracy {Dec( botAimAccuracy.Value )}" );
		cfg.AppendLine( $"set g_botAimSkill {Dec( botAimSkill.Value )}" );
		cfg.AppendLine( $"set g_botAlertness {Dec( botAlertness.Value )}" );
		cfg.AppendLine( $"set g_botFireThrottle {Dec( botFireThrottle.Value )}" );
		cfg.AppendLine( $"set bot_challenge {( botChallenge.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botRethink {( botRethink.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botTiming {( botTiming.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botGrab {( botGrab.Checked ? 1 : 0 )}" );
		// ohne das Hören 0, auch wenn der graue Haken noch gesetzt ist
		cfg.AppendLine( $"set g_botHunt {( BotHuntOn ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botVariety {(int)botVariety.Value}" );
		cfg.AppendLine( $"set g_botCamper {Dec( botCamper.Value )}" );
		cfg.AppendLine( $"set g_botDroppedWeight {(int)botDroppedWeight.Value}" );
		cfg.AppendLine( $"set bot_nochat {( botNoChat.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"set g_botLog {( botLog.Checked ? 1 : 0 )}" );
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

	void ArchiveLog( string path, string prefix ) {
		try {
			if ( !File.Exists( path ) || new FileInfo( path ).Length == 0 ) return;

			var attic = Path.Combine( HomePath, "logs" );
			Directory.CreateDirectory( attic );
			var stamp = File.GetLastWriteTime( path ).ToString( "yyyyMMdd-HHmmss" );
			var target = Path.Combine( attic, $"{prefix}-{stamp}.log" );
			if ( File.Exists( target ) ) File.Delete( target );
			File.Move( path, target );

			foreach ( var old in new DirectoryInfo( attic ).GetFiles( prefix + "-*.log" )
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
			ArchiveLog( logPath, "qconsole" );
			// Das Spiel hängt an botlog.log an, und seine Uhr beginnt mit jedem
			// Start von vorn - ohne das hier lägen zwei Sitzungen in einer
			// Datei, und die Auswertung hielte sie für einen Lauf.
			ArchiveLog( Path.Combine( HomePath, "botlog.log" ), "botlog" );
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
	// Die Bedingung, unter der eine Sitzung lief: Nachladezeit, Munition und ob
	// der Abzug von selbst gedrueckt hat. Fehlen sie, ist das Protokoll aelter
	// als diese Felder. Der selbsttaetige Abzug gehoert dazu, weil er nicht nur
	// besser zielt, sondern die schlechten Gelegenheiten gar nicht erst
	// abdrueckt - Schuesse mit und ohne ihn sind nicht dieselbe Stichprobe.
	static string StampCondition( string line ) {
		var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
		string rate = "", ammo = "", autoFire = "", hop = "", wraise = "", air = "", airAcc = "", ramp = "", rampSc = "", step = "", homing = "";
		for ( int i = 0; i < f.Length - 1; i++ ) {
			if ( f[i] == "rate" ) rate = f[i + 1];
			else if ( f[i] == "ammo" ) ammo = f[i + 1];
			else if ( f[i] == "autofire" ) autoFire = f[i + 1];
			else if ( f[i] == "autohop" ) hop = f[i + 1];
			else if ( f[i] == "wraise" ) wraise = f[i + 1];
			else if ( f[i] == "air" ) air = f[i + 1];
			else if ( f[i] == "airaccel" ) airAcc = f[i + 1];
			else if ( f[i] == "ramp" ) ramp = f[i + 1];
			else if ( f[i] == "rampscale" ) rampSc = f[i + 1];
			else if ( f[i] == "step" ) step = f[i + 1];
			else if ( f[i] == "homing" ) homing = f[i + 1];
		}
		// Was Pmove wie das Original nimmt, zählt als Original - ab Fassung 20
		// schreibt das Spiel es schon so, ältere Protokolle stempelten roh: eine
		// Luftbeschleunigung von 0 oder 1000, Schritthöhe 18, Hochnehmen in 250.
		// Der Rampenfaktor wirkt nur mit dem Rampensprung.
		if ( hop != "-1" ) {
			if ( int.TryParse( airAcc, out int aa ) && ( aa <= 0 || aa == 1000 ) ) airAcc = "";
			if ( step == "18" ) step = "0";
			if ( wraise == "250" ) wraise = "0";
		}
		string rampKey = ramp == "1"
			? "1:" + ( int.TryParse( rampSc, out int rs ) && rs > 0 ? rs : 1000 )
			: ramp;
		// Die Wegsteckzeit bleibt aus dem Schlüssel heraus: sie ist in Quake 3
		// und Quake Live dieselbe, und wer sie von Hand verstellt, sieht es an
		// der Zeile darüber. Das Hochnehmen ist der Wert, der sich unterscheidet.
		// Die Regler der Zielsuche gehören hinein, wenn sie an ist: eine Rakete
		// mit Vorhalt und Zünder ist eine andere Waffe als eine ohne. Ist sie aus,
		// zählen sie nicht - sonst trennte ein verschobener Regler zwei
		// Sitzungen, in denen er gar nicht gewirkt hat.
		string key = rate + "/" + ammo + "/" + autoFire + "/" + hop + "/" + wraise + "/" + air
			+ "/" + airAcc + "/" + rampKey + "/" + step + "/" + homing;
		if ( homing == "1" || homing == "2" ) {
			var h = HomingStamp( f );
			// Die Rampe wirkt nur, wenn Start und Ende verschieden sind; sonst
			// trennte ein verschobener Regler Sitzungen, in denen er nichts tat.
			foreach ( var name in HomingStampFields ) {
				key += "/" + ( name == "hramp" && h["hv0"] == h["hv1"] ? "" : h[name] );
			}
		}
		return key;
	}

	// Die Felder der Zielsuch-Raketen im Stempel, in der Reihenfolge, in der
	// das Spiel sie schreibt (aimLogHomingFields in cl_input.c).
	static readonly string[] HomingStampFields = {
		"hturn", "hcone", "hnear", "hlife", "hprox", "hlead", "harm", "hfuel",
		"hv0", "hv1", "hramp", "hdrag", "hpick", "hair", "hwarn", "hsplit", "hmiss",
	};

	// Jedes dieser Felder mit seinem Wert, leer wenn die Zeile es nicht hat -
	// ein älteres Protokoll kennt nur einen Teil davon.
	static Dictionary<string, string> HomingStamp( string[] f ) {
		var h = new Dictionary<string, string>();
		foreach ( var name in HomingStampFields ) h[name] = "";
		for ( int i = 0; i < f.Length - 1; i++ ) {
			if ( h.ContainsKey( f[i] ) ) h[f[i]] = f[i + 1];
		}
		return h;
	}

	void ShowLogVersion( string line, int conditions = 1 ) {
		if ( line.Length == 0 ) {
			logVersion.Text = "Protokoll ohne Fassungsangabe – älter als dieses Werkzeug";
			logVersion.ForeColor = Color.DarkGoldenrod;
			return;
		}

		var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
		int found = 0;
		string built = "", rate = "", ammo = "", autoFire = "", hop = "", wdrop = "", wraise = "", air = "", airAcc = "", ramp = "", rampSc = "", step = "", homing = "";
		for ( int i = 0; i < f.Length - 1; i++ ) {
			if ( f[i] == "version" ) int.TryParse( f[i + 1], out found );
			else if ( f[i] == "built" && i + 3 < f.Length ) built = $"{f[i + 1]} {f[i + 2]} {f[i + 3]}";
			else if ( f[i] == "rate" ) rate = f[i + 1];
			else if ( f[i] == "ammo" ) ammo = f[i + 1];
			else if ( f[i] == "autofire" ) autoFire = f[i + 1];
			else if ( f[i] == "autohop" ) hop = f[i + 1];
			else if ( f[i] == "wdrop" ) wdrop = f[i + 1];
			else if ( f[i] == "wraise" ) wraise = f[i + 1];
			else if ( f[i] == "air" ) air = f[i + 1];
			else if ( f[i] == "airaccel" ) airAcc = f[i + 1];
			else if ( f[i] == "ramp" ) ramp = f[i + 1];
			else if ( f[i] == "rampscale" ) rampSc = f[i + 1];
			else if ( f[i] == "step" ) step = f[i + 1];
			else if ( f[i] == "homing" ) homing = f[i + 1];
		}

		// Unter welcher Bedingung gespielt wurde. Nur nennen, wenn sie vom
		// Normalfall abweicht - sonst steht auf jeder Zeile eine Null-Aussage.
		var how = "";
		if ( rate.Length > 0 && rate != "100" && rate != "0" ) how += $", Nachladezeit {rate} %";
		if ( ammo == "1" ) how += ", Munition unbegrenzt (nur ich)";
		else if ( ammo == "2" ) how += ", Munition unbegrenzt (alle)";
		// Der Hinweis gehoert auch dann hin, wenn sonst nichts abweicht: eine
		// Trefferquote aus selbst abgedrueckten Schuessen ist mit einer aus
		// handgedrueckten nicht vergleichbar, und das soll man sehen, ohne es
		// wissen zu muessen.
		if ( autoFire == "1" ) how += ", Abzug selbsttätig (nur sichere Ziele)";

		// Die Bewegung. "-1" heißt: das Spielmodul kannte diese Physik nicht,
		// die Zahlen daneben hätten also nie gegolten - das ist eine eigene
		// Meldung wert, keine stille Null.
		if ( hop == "-1" ) {
			how += ", Bewegung unbekannt (altes Spielmodul)";
		} else {
			if ( hop == "1" ) how += ", Auto-Hop";
			if ( wraise.Length > 0 && wraise != "0" && wraise != "250" ) how += $", Waffe hoch in {wraise} ms";
			if ( wdrop.Length > 0 && wdrop != "0" && wdrop != "200" ) how += $", Waffe weg in {wdrop} ms";
			// Die Bruchzahlen stehen in Tausendsteln in der Zeile. Vor Fassung 20
			// rechnete das Spiel die Luftsteuerung ohne die CPM-Stärke - dort
			// drehte sie so gut wie nichts, und das soll hier auch stehen.
			if ( int.TryParse( air, out var airMilli ) && airMilli > 0 )
				how += found > 0 && found < 20
					? $", Luftsteuerung {airMilli / 1000.0:0.##} (vor Fassung 20: wirkungslos schwach)"
					: $", Luftsteuerung {airMilli / 1000.0:0.##}";
			if ( int.TryParse( airAcc, out var accMilli ) && accMilli > 0 && accMilli != 1000 )
				how += $", Luftbeschleunigung {accMilli / 1000.0:0.##}";
			if ( ramp == "1" )
				how += int.TryParse( rampSc, out var scMilli ) && scMilli > 0 && scMilli != 1000
					? $", Rampensprung ×{scMilli / 1000.0:0.##}" : ", Rampensprung";
			if ( int.TryParse( step, out var stepUnits ) && stepUnits > 0 && stepUnits != 18 )
				how += $", Schritthöhe {stepUnits}";
		}
		// Zielsuch-Raketen stehen außerhalb der Bewegung: sie hängen an einer
		// eigenen Cvar des Spielmoduls, nicht an pmove_qlActive. Genannt wird,
		// was von der Vorgabe abweicht - die Drehrate immer, sie ist die Waffe.
		if ( homing == "1" || homing == "2" ) {
			var h = HomingStamp( f );
			int N( string key, int standard ) => int.TryParse( h[key], out int n ) ? n : standard;
			var parts = new List<string> { homing == "1" ? "nur meine" : "alle", $"{N( "hturn", 180 )} °/s" };
			if ( N( "hcone", 45 ) != 45 ) parts.Add( $"Kegel {N( "hcone", 45 )}°" );
			if ( N( "hnear", 0 ) == 1 ) parts.Add( "wählt jedes Bild neu" );
			if ( N( "hlife", 15000 ) is > 0 and < 15000 ) parts.Add( $"zerlegt nach {N( "hlife", 15000 ) / 1000.0:0.0} s" );
			if ( N( "hsplit", 0 ) > 0 ) parts.Add( $"{N( "hsplit", 0 )} Splitter" );
			if ( N( "hprox", 0 ) > 0 ) parts.Add( $"Zünder {N( "hprox", 0 )} u" );
			if ( N( "hlead", 0 ) > 0 ) parts.Add( $"Vorhalt {N( "hlead", 0 )} %" );
			if ( N( "harm", 0 ) > 0 ) parts.Add( $"scharf nach {N( "harm", 0 )} ms" );
			if ( N( "hfuel", 0 ) > 0 ) parts.Add( $"Treibstoff {N( "hfuel", 0 ) / 1000.0:0.0} s" );
			int v0 = N( "hv0", 900 ), v1 = N( "hv1", 900 );
			if ( v0 != 900 || v1 != 900 ) {
				parts.Add( v0 == v1 ? $"{v0} u/s" : $"{v0}→{v1} u/s in {N( "hramp", 1000 ) / 1000.0:0.0#} s" );
			}
			if ( N( "hdrag", 0 ) > 0 ) parts.Add( $"Kurvenverlust {N( "hdrag", 0 )} %" );
			switch ( N( "hpick", 0 ) ) {
				case 1: parts.Add( "Ziel im Fadenkreuz" ); break;
				case 2: parts.Add( "leichteste Beute" ); break;
				case 3: parts.Add( "wer mich zuletzt traf" ); break;
			}
			if ( N( "hair", 0 ) == 1 ) parts.Add( "nur in der Luft" );
			else if ( N( "hair", 0 ) == 2 ) parts.Add( "nur am Boden" );
			if ( N( "hmiss", 0 ) == 1 ) parts.Add( "jagt auch Raketen" );
			else if ( N( "hmiss", 0 ) == 2 ) parts.Add( "jagt nur Raketen" );
			if ( N( "hwarn", 0 ) == 1 ) parts.Add( "Warnton" );
			how += ", Zielsuch-Raketen (" + string.Join( ", ", parts ) + ")";
		}

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
