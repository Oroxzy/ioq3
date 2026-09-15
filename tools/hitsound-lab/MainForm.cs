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

	readonly CheckBox aimAssist = new() { Text = "Zielhilfe auf Bots", Checked = true, AutoSize = true };
	readonly NumericUpDown aimStrength = new() { Minimum = 1, Maximum = 10, Value = 8, Width = 60 };
	readonly CheckBox botOutline = new() { Text = "Bots durch Wände umranden", Checked = true, AutoSize = true };
	readonly CheckBox botDamage = new() { Text = "mit Rest-HP (Farbe und Zahl)", Checked = true, AutoSize = true };
	// Wen die Hilfe nimmt, entscheidet die Vorrangliste; dieser Haken sagt nur,
	// dass zum Angreifer ohne Einschwenken gesprungen wird
	readonly CheckBox aimAttacker = new() { Text = "zum Angreifer springen statt weich schwenken", Checked = true, AutoSize = true };
	readonly CheckBox itemOutline = new() { Text = "Waffen und Powerups mit Respawn-Zeit", Checked = true, AutoSize = true };
	readonly CheckBox itemOutlineAll = new() { Text = "auch Rüstung und Mega", Checked = true, AutoSize = true };
	readonly TextBox aimKey = new() {
		Text = "MOUSE4", Width = 110, ReadOnly = true,
		BackColor = SystemColors.Window, Cursor = Cursors.Hand,
	};
	readonly NumericUpDown aimSmooth = new() { Minimum = 0, Maximum = 300, Increment = 10, Value = 0, Width = 60 };
	readonly NumericUpDown aimLead = new() { DecimalPlaces = 1, Increment = 0.1m, Minimum = 0.1m, Maximum = 5.0m, Value = 1.5m, Width = 70 };
	readonly CheckBox aimExact = new() { Text = "exakt im Schussmoment", Checked = true, AutoSize = true };
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
	readonly ListView shotView = new() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ),
	};

	readonly Label statTuneBoxes = Number();
	readonly Label statTuneSamples = Number();
	readonly CheckBox aimHoldFire = new() { Text = "nicht schießen, solange der Schuss nicht durchkommt", AutoSize = true };

	// Was ein Ziel zum besseren Ziel macht. Die Reihenfolge ist das Gewicht:
	// oben zaehlt am meisten. Schluessel wie in cl_aimAssistPriority.
	// Life > 0: eine Regel ueber etwas, das geschehen ist - die verfaellt.
	// Life = 0: eine Eigenschaft des Augenblicks, die keine Uhr braucht.
	static readonly (string Key, string Name, string Effect, double Life)[] Priorities = {
		( "sight",    "freie Sichtlinie",      "nur, worauf ein Schuss durchkommt – plus Nachwirkung", 0.1 ),
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
	};
	readonly Dictionary<string, Dictionary<string, int>> weaponWeight = new();
	readonly Dictionary<string, Dictionary<string, double>> weaponTime = new();
	readonly ComboBox prioWeapon = new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 170 };
	readonly Button prioReset = new() { Text = "wie Standard", Width = 110 };

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

	void SetWeight( string key, int value ) {
		var w = CurWeapon;
		if ( w is null ) { prioWeight[key] = value; return; }
		if ( !weaponWeight.TryGetValue( w, out var over ) ) weaponWeight[w] = over = new();
		over[key] = value;
	}

	void SetLife( string key, double value ) {
		var w = CurWeapon;
		if ( w is null ) { prioTime[key] = value; return; }
		if ( !weaponTime.TryGetValue( w, out var over ) ) weaponTime[w] = over = new();
		over[key] = value;
	}
	readonly ListView prioView = new() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true, CheckBoxes = true,
		GridLines = true, HideSelection = false, Font = new Font( "Segoe UI", 9 ),
	};
	readonly Button prioUp = new() { Text = "▲ höher", Width = 90 };
	readonly Button prioDown = new() { Text = "▼ tiefer", Width = 90 };
	readonly TrackBar prioBar = new() { Minimum = 0, Maximum = 100, TickFrequency = 10, Width = 200 };
	readonly Label prioValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	// in Zehntelsekunden, damit auch die Nachwirkung der Sichtlinie einstellbar
	// ist - die liegt bei Bruchteilen einer Sekunde, nicht bei ganzen. Die
	// Reichweite bleibt dieselbe wie zuvor in ganzen Sekunden, sonst wuerde
	// ein geladener Wert darueber beim ersten Anfassen stillschweigend gekappt.
	readonly TrackBar prioLifeBar = new() { Minimum = 0, Maximum = 600, TickFrequency = 100, Width = 160 };
	readonly Label prioLifeValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	bool prioUpdating;
	bool prioClicked;			// ob der letzte Hakenwechsel von einem Klick kam
	SplitContainer? splitMain;
	Size windowSize;			// was zuletzt gespeichert wurde, leer beim ersten Start
	int splitterSaved;			// wo der Teiler stand, 0 wenn nie gespeichert

	readonly Label statBestWeapon = Number();
	readonly ListView rankView = new() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ), OwnerDraw = true,
	};
	readonly ListView tuneView = new() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ),
	};

	// Die Protokollfassung, die dieses Werkzeug versteht. Schreibt das Spiel
	// eine andere, passen Zeilen und Auswertung nicht mehr sicher zusammen -
	// und dann soll das dastehen statt still falsch gerechnet zu werden.
	const int LogVersion = 6;
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

	public MainForm() {
		Text = "Trefferton-Labor";
		ClientSize = new Size( 1260, 820 );
		MinimumSize = new Size( 820, 560 );
		Font = new Font( "Segoe UI", 9 );

		gameDir.Text = FindGameDir();
		map.Items.AddRange( Maps );
		map.SelectedIndex = 0;
		hitSound.Items.AddRange( HitSounds );
		hitSound.SelectedIndex = 1;

		hitSound.SelectedIndexChanged += ( _, _ ) => hitSoundFile.Enabled = hitSound.SelectedIndex == 2;
		hitSoundFile.Enabled = false;
		aimKey.Click += ( _, _ ) => BeginAimKeyCapture();
		aimAssist.CheckedChanged += ( _, _ ) => UpdateAimEnabled();
		aimLearn.CheckedChanged += ( _, _ ) => UpdateAimEnabled();
		itemOutline.CheckedChanged += ( _, _ ) => itemOutlineAll.Enabled = itemOutline.Checked;
		// die Folge-Felder auf den Standard-Hakenstand bringen
		itemOutlineAll.Enabled = itemOutline.Checked;
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

		rankView.Columns.Add( "Waffe", 120 );
		rankView.Columns.Add( "Schüsse", 70, HorizontalAlignment.Right );
		rankView.Columns.Add( "Treffer", 70, HorizontalAlignment.Right );
		rankView.Columns.Add( "Quote", 260 );
		rankView.Columns.Add( "Fehlweite ø", 90, HorizontalAlignment.Right );

		prioView.Columns.Add( "Kriterium", 190 );
		prioView.Columns.Add( "Gewicht", 70, HorizontalAlignment.Right );
		prioView.Columns.Add( "gilt", 70, HorizontalAlignment.Right );
		prioView.Columns.Add( "was es bewirkt", 400 );
		for ( int i = 0; i < Priorities.Length; i++ ) {
			prioWeight[Priorities[i].Key] = PriorityDefault[i];
			prioTime[Priorities[i].Key] = Priorities[i].Life;
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

		prioWeapon.Items.Add( "Standard (alle Waffen)" );
		foreach ( var w in Weapons ) prioWeapon.Items.Add( w.Name );
		prioWeapon.SelectedIndex = 0;
		prioWeapon.SelectedIndexChanged += ( _, _ ) => FillPriorities();
		prioReset.Click += ( _, _ ) => {
			var w = CurWeapon;
			if ( w is null ) return;
			weaponWeight.Remove( w );
			weaponTime.Remove( w );
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

		// auch mitlesen, wenn das Spiel von Hand gestartet wurde
		logPath = Path.Combine( HomePath, "qconsole.log" );
		start.Click += ( _, _ ) => StartGame();
		save.Click += ( _, _ ) => SaveSettings();
		poll.Tick += ( _, _ ) => RefreshStats();

		Controls.Add( BuildLayout() );
		Application.AddMessageFilter( this );
		foreach ( var view in new[] { shotView, tuneView, rankView, prioView } ) FitOnResize( view );
		FillPriorities();			// erst wenn die Liste im Fenster haengt
		LoadSettings();
		poll.Start();
	}

	protected override void OnLoad( EventArgs e ) {
		base.OnLoad( e );

		// Beim ersten Start gross aufmachen - fuenf Karten voller Tabellen
		// wollen Platz. Danach gilt, was der Benutzer zuletzt eingestellt hat.
		if ( windowSize.Width > 400 && windowSize.Height > 300 ) {
			ClientSize = windowSize;
			CenterToScreen();
		} else {
			WindowState = FormWindowState.Maximized;
		}

		if ( splitMain is not null && splitMain.Width > 760 ) {
			splitMain.Panel1MinSize = 430;
			splitMain.Panel2MinSize = 320;
			int want = splitterSaved > 0 ? splitterSaved : 500;
			splitMain.SplitterDistance = Math.Clamp( want, 430, splitMain.Width - 320 );
		}
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
		split.Panel1.Padding = new Padding( 12, 12, 6, 12 );
		split.Panel1.AutoScroll = true;
		split.Panel2.Padding = new Padding( 6, 12, 12, 12 );

		var boxes = new[] { BuildMatchBox(), BuildSoundBox(), BuildAimBox(), BuildLeadBox(), BuildViewBox() };
		var column = new TableLayoutPanel {
			Dock = DockStyle.Top, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink,
			ColumnCount = 1, RowCount = boxes.Length,
		};
		column.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );
		for ( int i = 0; i < boxes.Length; i++ ) {
			column.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
			column.Controls.Add( boxes[i], 0, i );
		}

		split.Panel1.Controls.Add( column );
		split.Panel2.Controls.Add( BuildStatsBox() );
		return split;
	}

	GroupBox BuildMatchBox() {
		var browse = new Button { Text = "…", Width = 34, Margin = new Padding( 0, 3, 14, 0 ) };
		browse.Click += ( _, _ ) => {
			using var dlg = new FolderBrowserDialog { SelectedPath = gameDir.Text };
			if ( dlg.ShowDialog() == DialogResult.OK ) gameDir.Text = dlg.SelectedPath;
		};

		return Group( "Spiel",
			Row( Labelled( "Spielordner:", gameDir ), browse ),
			Row( Labelled( "Map:", map ), Labelled( "Bots:", bots ), Labelled( "Können:", skill ) ),
			Row( Pad( start ), Pad( save ), Pad( status ) ),
			Row( logVersion ) );
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
		return Group( "Zielhilfe",
			Row( Pad( aimAssist ) ),
			Row( Labelled( "Halten:", aimKey ), Labelled( "Snap-Stärke:", aimStrength ) ),
			Row( Pad( aimExact ) ),
			Row( Pad( aimAttacker ) ),
			Row( Pad( aimHoldFire ) ),
			Row( Hint( "Gilt, solange die Zieltaste hält, und zählt im Tab „Trefferton“ mit." ) ),
			Row( Hint( "Die Taste zielt nur; geschossen wird mit der Feuertaste." ) ),
			Row( Hint( "Wen sie nimmt, steht in der Karte „Vorrang“ rechts." ) ) );
	}

	// Wie weit vorgehalten wird
	GroupBox BuildLeadBox() {
		return Group( "Vorhalt",
			Row( Labelled( "Glättung (ms):", aimSmooth ), Labelled( "Richtung halten (s):", aimLead ) ),
			Row( Pad( aimLearn ) ),
			Row( Pad( aimLearned ) ) );
	}

	// Was zu sehen ist - mit dem Zielen hat das nichts zu tun
	GroupBox BuildViewBox() {
		return Group( "Anzeige",
			Row( Pad( botOutline ), Pad( botDamage ) ),
			Row( Pad( itemOutline ) ),
			Row( Pad( itemOutlineAll ) ) );
	}

	static Label Hint( string text ) => new() {
		Text = text, AutoSize = true, ForeColor = Color.DimGray,
		Margin = new Padding( 0, 2, 0, 0 ),
	};

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

	static Control Row( params Control[] items ) {
		var flow = new FlowLayoutPanel { AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, WrapContents = false };
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
			Row( Counter( "Töpfe:", statTuneBoxes ), Counter( "Proben:", statTuneSamples ) ),
			tuneView ) );
		tabs.TabPages.Add( Page( "Rangliste",
			Row( Counter( "beste Waffe:", statBestWeapon ) ),
			rankView ) );
		// Eigene Karteikarte statt Page(): die Bedienzeile bekommt eine feste
		// Hoehe, sonst nimmt sie sich mit dem Schieber darin den ganzen Platz
		// und die Liste bleibt einen Pixel hoch
		var prioPage = new TabPage( "Vorrang" ) { Padding = new Padding( 10 ), BackColor = SystemColors.Control };
		var prioGrid = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 2 };
		prioGrid.RowStyles.Add( new RowStyle( SizeType.Absolute, 56 ) );
		prioGrid.RowStyles.Add( new RowStyle( SizeType.Percent, 100 ) );
		prioGrid.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );
		prioGrid.Controls.Add( Row(
			Pad( new Label { Text = "für:", AutoSize = true, Margin = new Padding( 0, 10, 4, 0 ) } ),
			Pad( prioWeapon ), Pad( prioReset ),
			Pad( prioUp ), Pad( prioDown ),
			Pad( new Label { Text = "Gewicht:", AutoSize = true, Margin = new Padding( 16, 10, 4, 0 ) } ),
			Pad( prioBar ), Pad( prioValue ),
			Pad( new Label { Text = "gilt (s):", AutoSize = true, Margin = new Padding( 16, 10, 4, 0 ) } ),
			Pad( prioLifeBar ), Pad( prioLifeValue ) ), 0, 0 );
		prioGrid.Controls.Add( prioView, 0, 1 );
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
			var row = new ListViewItem( ( Differs( p.Key ) ? "▸ " : "" ) + p.Name ) {
				Tag = p.Key, Checked = weight > 0,
			};
			row.SubItems.Add( weight.ToString() );
			row.SubItems.Add( IsTimed( p.Key ) ? Life( p.Key ).ToString( "0.0" ) + " s" : "—" );
			row.SubItems.Add( Differs( p.Key )
				? $"{p.Effect}  (Standard {prioWeight[p.Key]})" : p.Effect );
			if ( weight == 0 ) row.ForeColor = Color.DimGray;
			else if ( Differs( p.Key ) ) row.ForeColor = Color.DarkSlateBlue;
			prioView.Items.Add( row );
			if ( keep is not null && p.Key == keep ) row.Selected = true;
		}

		prioReset.Enabled = CurWeapon is not null;
		prioView.EndUpdate();
		prioUpdating = false;
		ShowPrioritySelection();
	}

	// Nur die drei Regeln ueber etwas Geschehenes haben eine Gueltigkeit
	static bool IsTimed( string key ) =>
		Array.Find( Priorities, p => p.Key == key ).Life > 0;

	void ShowPrioritySelection() {
		if ( prioView.SelectedItems.Count == 0 ) {
			prioValue.Text = "(Zeile wählen)";
			prioLifeValue.Text = "";
			prioLifeBar.Enabled = false;
			return;
		}

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
		s.AppendLine( "aimSmooth=" + (int)aimSmooth.Value );
		s.AppendLine( "aimLead=" + Dec( aimLead.Value ) );
		s.AppendLine( "aimExact=" + aimExact.Checked );
		s.AppendLine( "aimLearn=" + aimLearn.Checked );
		s.AppendLine( "aimHoldFire=" + aimHoldFire.Checked );
		s.AppendLine( "aimPriority=" + PriorityString() );
		s.AppendLine( "aimPriorityWeapon=" + WeaponPriorityString() );
		// nur eine wiederherstellbare Groesse merken, kein maximiertes Fenster
		if ( WindowState == FormWindowState.Normal ) {
			s.AppendLine( "windowWidth=" + ClientSize.Width );
			s.AppendLine( "windowHeight=" + ClientSize.Height );
		}
		if ( splitMain is not null ) s.AppendLine( "splitter=" + splitMain.SplitterDistance );
		s.AppendLine( "botOutline=" + botOutline.Checked );
		s.AppendLine( "botDamage=" + botDamage.Checked );
		s.AppendLine( "itemOutline=" + itemOutline.Checked );
		s.AppendLine( "itemOutlineAll=" + itemOutlineAll.Checked );

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
		SetNum( aimSmooth, v, "aimSmooth" );
		SetNum( aimLead, v, "aimLead" );
		SetBool( aimExact, v, "aimExact" );
		SetBool( aimLearn, v, "aimLearn" );
		SetBool( aimHoldFire, v, "aimHoldFire" );
		if ( v.TryGetValue( "aimPriority", out var prio ) && prio.Length > 0 ) ApplyPriorityString( prio );
		if ( v.TryGetValue( "aimPriorityWeapon", out var wprio ) ) ApplyWeaponPriorityString( wprio );
		if ( v.TryGetValue( "windowWidth", out var ww ) && int.TryParse( ww, out int w2 )
			&& v.TryGetValue( "windowHeight", out var wh ) && int.TryParse( wh, out int h2 ) ) {
			windowSize = new Size( w2, h2 );
		}
		if ( v.TryGetValue( "splitter", out var sp ) && int.TryParse( sp, out int sd ) ) splitterSaved = sd;
		SetBool( botOutline, v, "botOutline" );
		SetBool( botDamage, v, "botDamage" );
		SetBool( itemOutline, v, "itemOutline" );
		SetBool( itemOutlineAll, v, "itemOutlineAll" );
	}

	static void SetBool( CheckBox box, Dictionary<string, string> v, string key ) {
		if ( v.TryGetValue( key, out var s ) && bool.TryParse( s, out bool b ) ) box.Checked = b;
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
		cfg.AppendLine( $"seta cl_botOutline {( botOutline.Checked ? ( botDamage.Checked ? 2 : 1 ) : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistAttacker {( aimAssist.Checked && aimAttacker.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistDebug {( aimAssist.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistKey \"{aimKey.Text.Replace( "\"", "" )}\"" );
		cfg.AppendLine( $"seta cl_aimAssistSmooth {(int)aimSmooth.Value}" );
		cfg.AppendLine( $"seta cl_aimAssistLead {Dec( aimLead.Value )}" );
		cfg.AppendLine( $"seta cl_aimAssistExact {( aimExact.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistLearn {( aimAssist.Checked && aimLearn.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistHoldFire {( aimHoldFire.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistPriority \"{PriorityString()}\"" );
		cfg.AppendLine( $"seta cl_aimAssistPriorityWeapon \"{WeaponPriorityString()}\"" );
		cfg.AppendLine( "set logfile 2" );
		cfg.AppendLine( "set bot_nochat 1" );
		// Die Engine begrenzt die Zielhilfe selbst auf lokale Bot-Partien. Der
		// Trefferton-Test braucht deshalb keine allgemeinen Server-Cheats.
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

			logPath = Path.Combine( HomePath, "qconsole.log" );
			ArchiveLog();
			shotStamp = "";
			aimLearned.Text = "";

			game = Process.Start( new ProcessStartInfo {
				FileName = exe,
				Arguments = $"+exec {CfgName}",
				WorkingDirectory = gameDir.Text,
				UseShellExecute = true,
			} );

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
		var recent = new List<string>();
		var damageFrames = new List<Damage>();
		var shots = new List<Shot>();
		var impacts = new List<Impact>();
		var missiles = new List<Missile>();
		var tunes = new Dictionary<string, Tune>();
		var learned = "";
		var stamp = "";

		foreach ( var line in text.Split( '\n' ) ) {
			var trimmed = line.TrimEnd( '\r' );

			if ( trimmed.StartsWith( "hit on " ) ) {
				hits++;
				// Treffer im selben Server-Frame beantwortet das Spiel mit einem Ton,
				// deshalb zaehlen die Frames und nicht die einzelnen Schadensereignisse
				var mark = trimmed.LastIndexOf( " frame ", StringComparison.Ordinal );
				frames.Add( mark >= 0 ? trimmed[( mark + 7 )..] : "#" + hits );
				if ( mark >= 0 && int.TryParse( trimmed[( mark + 7 )..], out int damageFrame ) ) {
					// wer getroffen wurde, steht zwischen "hit on " und dem Doppelpunkt
					var colon = trimmed.IndexOf( ':', 7 );
					damageFrames.Add( new Damage {
						Frame = damageFrame,
						Victim = colon > 7 ? trimmed[7..colon] : "",
					} );
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
			} else if ( trimmed.StartsWith( "aim log: " ) ) {
				stamp = trimmed;
			} else if ( trimmed.StartsWith( "aim tune: " ) ) {
				var tune = Tune.Parse( trimmed );
				// je Waffe und Flugzeitband zaehlt der zuletzt gemessene Stand
				if ( tune is not null ) tunes[tune.Weapon + "|" + tune.Band + "|" + tune.Pace] = tune;
			}
		}

		ShowLogVersion( stamp );

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
		ShowLearned( learned );

		// Das Spiel ist zu Ende und sein Protokoll gelesen: was es an Vorhalt
		// gelernt hat, festhalten, sonst waere die Optimierung beim naechsten
		// Start der App wieder weg
		if ( exited ) {
			game = null;
		}

		int missed = Math.Max( 0, frames.Count - sounds );
		statHits.Text = hits.ToString();
		statFrames.Text = frames.Count.ToString();
		statSounds.Text = sounds.ToString();
		statMissed.Text = missed.ToString();
		statMissed.ForeColor = missed > 0 ? Color.Firebrick : Color.ForestGreen;

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
	void ShowLogVersion( string line ) {
		if ( line.Length == 0 ) {
			logVersion.Text = "Protokoll ohne Fassungsangabe – älter als dieses Werkzeug";
			logVersion.ForeColor = Color.DarkGoldenrod;
			return;
		}

		var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
		int found = 0;
		string built = "";
		for ( int i = 0; i < f.Length - 1; i++ ) {
			if ( f[i] == "version" ) int.TryParse( f[i + 1], out found );
			else if ( f[i] == "built" && i + 3 < f.Length ) built = $"{f[i + 1]} {f[i + 2]} {f[i + 3]}";
		}

		if ( found == LogVersion ) {
			logVersion.Text = $"Protokoll Fassung {found}, Spiel vom {built}";
			logVersion.ForeColor = Color.DimGray;
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
					case "land": int.TryParse( f[i + 1], out shot.Land ); break;
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
		public int Band, Pace, Samples;
		public double From, Above, Factor, Scatter, Reach;

		public static Tune? Parse( string line ) {
			var f = line[10..].Split( ' ', StringSplitOptions.RemoveEmptyEntries );
			if ( f.Length < 3 ) return null;

			var t = new Tune { Weapon = f[0], Scatter = -1 };
			for ( int i = 1; i < f.Length - 1; i++ ) {
				switch ( f[i] ) {
				case "band": int.TryParse( f[i + 1], out t.Band ); break;
				case "pace": int.TryParse( f[i + 1], out t.Pace ); break;
				case "n": int.TryParse( f[i + 1], out t.Samples ); break;
				case "from": t.From = Num( f[i + 1] ); break;
				case "above": t.Above = Num( f[i + 1] ); break;
				case "factor": t.Factor = Num( f[i + 1] ); break;
				case "scatter": t.Scatter = Num( f[i + 1] ); break;
				case "reach": t.Reach = Num( f[i + 1] ); break;
				}
			}
			return t.Samples > 0 ? t : null;
		}

		static double Num( string s ) =>
			double.TryParse( s, System.Globalization.NumberStyles.Any,
				System.Globalization.CultureInfo.InvariantCulture, out double v ) ? v : 0;
	}

	// Die Tabelle, die das Spiel ueber sich selbst fuehrt: pro Waffe und
	// Flugzeit, wie viel vom Vorhalt wirklich eintrifft und wie weit das
	// Ergebnis danach noch streut. Die Streuung gegen den Wirkradius sagt,
	// ob der Schuss auf die Entfernung ueberhaupt zu machen ist.
	// Die Spalten sollen die Fensterbreite mitnehmen. Was beim Anlegen als
	// Breite dasteht, gilt dabei als Verhaeltnis: eine breite Spalte bekommt
	// von jeder zusaetzlichen Breite entsprechend mehr ab.
	static void FitColumns( ListView view ) {
		var columns = view.Columns.Cast<ColumnHeader>().ToArray();
		var weights = columns.Select( c => c.Tag is int t ? t : c.Width ).ToArray();
		int total = weights.Sum();
		int room = view.ClientSize.Width - 4;
		if ( total <= 0 || room < 120 ) return;

		// Enger als die eigene Ueberschrift wird keine Spalte - lieber quer
		// scrollen als zwoelf Spalten, die alle "F..." heissen
		var least = columns.Select( c => TextRenderer.MeasureText( c.Text, view.Font ).Width + 22 ).ToArray();

		int used = 0;
		for ( int i = 0; i < columns.Length - 1; i++ ) {
			int w = Math.Max( least[i], room * weights[i] / total );
			columns[i].Width = w;
			used += w;
		}
		columns[^1].Width = Math.Max( least[^1], room - used );
	}

	static void FitOnResize( ListView view ) {
		foreach ( ColumnHeader column in view.Columns ) column.Tag = column.Width;
		view.Resize += ( _, _ ) => FitColumns( view );
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
