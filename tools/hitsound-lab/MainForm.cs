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

	readonly TextBox gameDir = new() { Width = 300 };
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
	readonly CheckBox aimPrefer = new() { Text = "kurze Waffen: nächstes Ziel zuerst", Checked = true, AutoSize = true };
	readonly CheckBox aimAttacker = new() { Text = "sofort auf den, der mich trifft", Checked = true, AutoSize = true };
	readonly CheckBox itemOutline = new() { Text = "Waffen und Powerups mit Respawn-Zeit", Checked = true, AutoSize = true };
	readonly CheckBox itemOutlineAll = new() { Text = "auch Rüstung und Mega", Checked = true, AutoSize = true };
	readonly TextBox aimKey = new() {
		Text = "MOUSE4", Width = 110, ReadOnly = true,
		BackColor = SystemColors.Window, Cursor = Cursors.Hand,
	};
	readonly NumericUpDown aimSmooth = new() { Minimum = 0, Maximum = 300, Increment = 10, Value = 0, Width = 60 };
	readonly NumericUpDown aimLead = new() { DecimalPlaces = 1, Increment = 0.1m, Minimum = 0.1m, Maximum = 5.0m, Value = 1.5m, Width = 70 };
	readonly CheckBox aimExact = new() { Text = "exakt im Schussmoment", Checked = true, AutoSize = true };
	readonly CheckBox aimLearn = new() { Text = "Vorhalt automatisch optimieren", Checked = true, AutoSize = true };
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
	readonly ListView shotView = new() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ),
	};

	readonly Label statTuneBoxes = Number();
	readonly Label statTuneSamples = Number();
	readonly CheckBox aimHoldFire = new() { Text = "nicht schießen, solange das Ziel hinter Deckung ist", AutoSize = true };

	// Was ein Ziel zum besseren Ziel macht. Die Reihenfolge ist das Gewicht:
	// oben zaehlt am meisten. Schluessel wie in cl_aimAssistPriority.
	static readonly (string Key, string Name, string Effect)[] Priorities = {
		( "sight",    "freie Sichtlinie",      "nur, worauf ein Schuss überhaupt durchkommt" ),
		( "cursor",   "Nähe zum Fadenkreuz",   "wohin du ohnehin schon zielst" ),
		( "attacker", "wer mich zuletzt traf", "sofort zurückschlagen" ),
		( "sure",     "Treffsicherheit",       "was die Waffe auf die Entfernung gemessen trifft" ),
		( "near",     "Nähe im Raum",          "der nächste Gegner zuerst" ),
		( "wounded",  "schon verwundet",       "wen ich selbst angeschlagen habe" ),
		( "keep",     "Ziel behalten",         "nicht zwischen zweien hin und her springen" ),
		( "powerup",  "trägt ein Powerup",     "Quad, Regeneration, Haste zuerst" ),
		( "air",      "in der Luft",           "fliegt berechenbar - ein bloßer Sprung zählt nicht" ),
	};
	static readonly int[] PriorityDefault = { 100, 80, 100, 60, 40, 40, 30, 20, 0 };

	readonly Dictionary<string, int> prioWeight = new();
	readonly ListView prioView = new() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true, CheckBoxes = true,
		GridLines = true, HideSelection = false, Font = new Font( "Segoe UI", 9 ),
	};
	readonly Button prioUp = new() { Text = "▲ höher", Width = 90 };
	readonly Button prioDown = new() { Text = "▼ tiefer", Width = 90 };
	readonly TrackBar prioBar = new() { Minimum = 0, Maximum = 100, TickFrequency = 10, Width = 260 };
	readonly Label prioValue = new() { AutoSize = true, ForeColor = Color.DimGray };
	bool prioUpdating;

	readonly Label statBestWeapon = Number();
	readonly ListView rankView = new() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ), OwnerDraw = true,
	};
	readonly ListView tuneView = new() {
		Dock = DockStyle.Fill, View = View.Details, FullRowSelect = true,
		GridLines = true, Font = new Font( "Consolas", 9 ),
	};

	readonly Button start = new() { Text = "Spiel starten", Width = 140, Height = 34 };
	readonly Button save = new() { Text = "Speichern", Width = 100, Height = 34 };
	readonly Label status = new() { AutoSize = true, ForeColor = Color.DimGray };

	readonly System.Windows.Forms.Timer poll = new() { Interval = 500 };
	string logPath = "";
	string aimKeyBeforeCapture = "MOUSE4";
	bool capturingAimKey;
	string shotStamp = "";
	Process? game;					// das von hier gestartete Spiel, solange es laeuft
	decimal leadAtStart;			// der Vorhalt, mit dem es gestartet wurde

	public MainForm() {
		Text = "Trefferton-Labor";
		ClientSize = new Size( 880, 720 );
		MinimumSize = new Size( 560, 420 );
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
		shotView.Columns.Add( "in der Luft", 80 );
		shotView.Columns.Add( "Vorhalt", 70, HorizontalAlignment.Right );
		shotView.Columns.Add( "Fehler", 70, HorizontalAlignment.Right );
		shotView.Columns.Add( "Zielpunkt", 150 );
		shotView.Columns.Add( "Ergebnis", 80 );
		shotView.Columns.Add( "Hilfe", 60 );
		shotView.Columns.Add( "Fehlweite", 70, HorizontalAlignment.Right );
		shotView.Columns.Add( "Richtung", 110 );

		tuneView.Columns.Add( "Waffe", 110 );
		tuneView.Columns.Add( "Flugzeit ab", 90, HorizontalAlignment.Right );
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
		prioView.Columns.Add( "was es bewirkt", 400 );
		for ( int i = 0; i < Priorities.Length; i++ ) prioWeight[Priorities[i].Key] = PriorityDefault[i];
		FillPriorities();
		prioUp.Click += ( _, _ ) => MovePriority( -1 );
		prioDown.Click += ( _, _ ) => MovePriority( 1 );
		prioView.SelectedIndexChanged += ( _, _ ) => ShowPrioritySelection();
		prioView.ItemChecked += ( _, e ) => {
			if ( prioUpdating ) return;
			// abgehakt heisst Gewicht null; beim Wiedereinschalten kommt ein
			// brauchbarer Wert zurueck, sonst bliebe die Zeile wirkungslos
			var key = (string)e.Item.Tag!;
			if ( !e.Item.Checked ) prioWeight[key] = 0;
			else if ( prioWeight[key] == 0 ) prioWeight[key] = 50;
			FillPriorities();
		};
		prioBar.ValueChanged += ( _, _ ) => {
			if ( prioUpdating || prioView.SelectedItems.Count == 0 ) return;
			prioWeight[(string)prioView.SelectedItems[0].Tag!] = prioBar.Value;
			FillPriorities( (string)prioView.SelectedItems[0].Tag! );
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
		LoadSettings();
		poll.Start();
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
		aimPrefer.Enabled = on;
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

	Control BuildLayout() {
		var root = new TableLayoutPanel {
			Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 4,
			Padding = new Padding( 12 ), AutoScroll = true,
		};
		root.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
		root.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
		root.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
		root.RowStyles.Add( new RowStyle( SizeType.Percent, 100 ) );
		root.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );

		root.Controls.Add( BuildMatchBox(), 0, 0 );
		root.Controls.Add( BuildSoundBox(), 0, 1 );
		root.Controls.Add( BuildAimBox(), 0, 2 );
		root.Controls.Add( BuildStatsBox(), 0, 3 );
		return root;
	}

	GroupBox BuildMatchBox() {
		var browse = new Button { Text = "…", Width = 34, Margin = new Padding( 0, 3, 14, 0 ) };
		browse.Click += ( _, _ ) => {
			using var dlg = new FolderBrowserDialog { SelectedPath = gameDir.Text };
			if ( dlg.ShowDialog() == DialogResult.OK ) gameDir.Text = dlg.SelectedPath;
		};

		return Group( "Spiel",
			Row( Labelled( "Spielordner:", gameDir ), browse ),
			Row( Labelled( "Map:", map ), Labelled( "Bots:", bots ), Labelled( "Können:", skill ),
				Pad( start ), Pad( save ), Pad( status ) ) );
	}

	GroupBox BuildSoundBox() {
		return Group( "Trefferton",
			Row( Labelled( "Ton:", hitSound ), Labelled( "Datei:", hitSoundFile ) ),
			Row( Pad( hitPitch ) ),
			Row( Labelled( "volle HP:", pitchFull ), Labelled( "leer:", pitchEmpty ),
				Labelled( "Kill:", pitchKill ), Labelled( "voll ab:", pitchStack ) ) );
	}

	GroupBox BuildAimBox() {
		return Group( "Zielhilfe",
			Row( Pad( aimAssist ), Labelled( "Halten:", aimKey ), Labelled( "Snap-Stärke:", aimStrength ) ),
			Row( Pad( aimAttacker ), Pad( aimPrefer ), Pad( aimExact ) ),
			Row( Pad( aimHoldFire ) ),
			Row( Labelled( "Glättung (ms):", aimSmooth ), Labelled( "Richtung halten (s):", aimLead ), Pad( aimLearn ), Pad( aimLearned ) ),
			Row( Pad( botOutline ) ),
			Row( Pad( itemOutline ), Pad( itemOutlineAll ) ),
			Row( new Label {
				Text = "Hold-Key zielt nur; geschossen wird separat mit der Feuertaste (10 = sofort)",
				AutoSize = true, ForeColor = Color.DimGray, Margin = new Padding( 0, 2, 0, 0 ),
			} ) );
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
				Counter( "ohne Hilfe:", statShotRateOff ), Counter( "Fehler ø:", statShotError ) ),
			shotView ) );
		tabs.TabPages.Add( Page( "pro Waffe",
			Row( Counter( "Töpfe:", statTuneBoxes ), Counter( "Proben:", statTuneSamples ) ),
			tuneView ) );
		tabs.TabPages.Add( Page( "Rangliste",
			Row( Counter( "beste Waffe:", statBestWeapon ) ),
			rankView ) );
		tabs.TabPages.Add( Page( "Vorrang",
			Row( Pad( prioUp ), Pad( prioDown ), Pad( new Label {
					Text = "Gewicht:", AutoSize = true, Margin = new Padding( 16, 8, 4, 0 ) } ),
				Pad( prioBar ), Pad( prioValue ) ),
			prioView ) );
		return tabs;
	}

	// Die Liste, nach Gewicht sortiert: oben zaehlt am meisten
	void FillPriorities( string? keep = null ) {
		prioUpdating = true;
		prioView.BeginUpdate();
		prioView.Items.Clear();

		foreach ( var p in Priorities.OrderByDescending( p => prioWeight[p.Key] )
				.ThenBy( p => Array.FindIndex( Priorities, q => q.Key == p.Key ) ) ) {
			int weight = prioWeight[p.Key];
			var row = new ListViewItem( p.Name ) { Tag = p.Key, Checked = weight > 0 };
			row.SubItems.Add( weight.ToString() );
			row.SubItems.Add( p.Effect );
			if ( weight == 0 ) row.ForeColor = Color.DimGray;
			prioView.Items.Add( row );
			if ( keep is not null && p.Key == keep ) row.Selected = true;
		}

		prioView.EndUpdate();
		prioUpdating = false;
		ShowPrioritySelection();
	}

	void ShowPrioritySelection() {
		if ( prioView.SelectedItems.Count == 0 ) { prioValue.Text = "(Zeile wählen)"; return; }
		var key = (string)prioView.SelectedItems[0].Tag!;
		prioUpdating = true;
		prioBar.Value = Math.Clamp( prioWeight[key], prioBar.Minimum, prioBar.Maximum );
		prioUpdating = false;
		prioValue.Text = prioWeight[key].ToString();
	}

	// Hoeher oder tiefer heisst: das Gewicht mit dem Nachbarn tauschen, denn
	// das Gewicht ist die Reihenfolge
	void MovePriority( int step ) {
		if ( prioView.SelectedItems.Count == 0 ) return;
		int index = prioView.SelectedItems[0].Index, other = index + step;
		if ( other < 0 || other >= prioView.Items.Count ) return;

		var key = (string)prioView.Items[index].Tag!;
		var neighbour = (string)prioView.Items[other].Tag!;
		int mine = prioWeight[key], theirs = prioWeight[neighbour];
		if ( mine == theirs ) {
			// gleich schwer: einen Schritt daran vorbei
			mine = Math.Clamp( theirs - step, 0, 100 );
		} else {
			( mine, theirs ) = ( theirs, mine );
		}
		prioWeight[key] = mine;
		prioWeight[neighbour] = theirs;
		FillPriorities( key );
	}

	string PriorityString() =>
		string.Join( " ", Priorities.Select( p => p.Key + ":" + prioWeight[p.Key] ) );

	void ApplyPriorityString( string text ) {
		foreach ( var part in text.Split( ' ', StringSplitOptions.RemoveEmptyEntries ) ) {
			var colon = part.IndexOf( ':' );
			if ( colon <= 0 ) continue;
			var key = part[..colon];
			if ( prioWeight.ContainsKey( key ) && int.TryParse( part[( colon + 1 )..], out int w ) ) {
				prioWeight[key] = Math.Clamp( w, 0, 100 );
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
		s.AppendLine( "aimPrefer=" + aimPrefer.Checked );
		s.AppendLine( "botOutline=" + botOutline.Checked );
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
		SetBool( aimPrefer, v, "aimPrefer" );
		SetBool( botOutline, v, "botOutline" );
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
		cfg.AppendLine( $"seta cl_botOutline {( botOutline.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistPrefer {( aimPrefer.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistAttacker {( aimAssist.Checked && aimAttacker.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistDebug {( aimAssist.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistKey \"{aimKey.Text.Replace( "\"", "" )}\"" );
		cfg.AppendLine( $"seta cl_aimAssistSmooth {(int)aimSmooth.Value}" );
		cfg.AppendLine( $"seta cl_aimAssistLead {Dec( aimLead.Value )}" );
		cfg.AppendLine( $"seta cl_aimAssistExact {( aimExact.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistLearn {( aimAssist.Checked && aimLearn.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistHoldFire {( aimHoldFire.Checked ? 1 : 0 )}" );
		cfg.AppendLine( $"seta cl_aimAssistPriority \"{PriorityString()}\"" );
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
			// Haengt noch ein Spiel am Protokoll, bleibt es eben stehen
			try {
				if ( File.Exists( logPath ) ) File.Delete( logPath );
			} catch ( IOException ) {
			} catch ( UnauthorizedAccessException ) {
			}
			shotStamp = "";
			aimLearned.Text = "";
			leadAtStart = aimLead.Value;

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

	// Nur den gelernten Vorhalt in die gespeicherten Einstellungen schreiben,
	// alles andere so lassen, wie es zuletzt gespeichert wurde - und wenn noch
	// nie gespeichert wurde, wenigstens diesen einen Wert
	void PersistLearnedLead() {
		try {
			var lines = File.Exists( SettingsPath )
				? File.ReadAllLines( SettingsPath ).Where( l => !l.StartsWith( "aimLead=" ) ).ToList()
				: new List<string>();
			lines.Add( "aimLead=" + Dec( aimLead.Value ) );
			Directory.CreateDirectory( Path.GetDirectoryName( SettingsPath )! );
			File.WriteAllLines( SettingsPath, lines );
		} catch ( Exception ) {
			// eine schreibgeschuetzte Datei oder ein gesperrter Ordner ist kein
			// Grund, den Takt mit einem Fehlerdialog anzuhalten
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

		int hits = 0, sounds = 0;
		var frames = new HashSet<string>();
		var recent = new List<string>();
		var damageFrames = new List<Damage>();
		var shots = new List<Shot>();
		var impacts = new List<Impact>();
		var missiles = new List<Missile>();
		var tunes = new Dictionary<string, Tune>();
		var learned = "";

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
			} else if ( trimmed.StartsWith( "aim learn: " ) ) {
				learned = trimmed;
			} else if ( trimmed.StartsWith( "aim tune: " ) ) {
				var tune = Tune.Parse( trimmed );
				// je Waffe und Flugzeitband zaehlt der zuletzt gemessene Stand
				if ( tune is not null ) tunes[tune.Weapon + "|" + tune.Band] = tune;
			}
		}

		UpdateShots( shots, damageFrames, impacts, missiles );
		UpdateTune( tunes );
		ShowLearned( learned );

		// Das Spiel ist zu Ende und sein Protokoll gelesen: was es an Vorhalt
		// gelernt hat, festhalten, sonst waere die Optimierung beim naechsten
		// Start der App wieder weg
		if ( exited ) {
			game = null;
			if ( aimLearn.Checked && aimLead.Value != leadAtStart ) PersistLearnedLead();
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

	// Die letzte Zeile "aim learn:" - der Vorhalt, den das Spiel gerade gelernt
	// hat. Solange das Spiel ihn selbst optimiert, folgt der Regler dem Spiel,
	// und beim naechsten Start geht der gelernte Wert wieder mit hinein.
	void ShowLearned( string line ) {
		if ( line.Length == 0 ) return;

		var f = line.Split( ' ', StringSplitOptions.RemoveEmptyEntries );
		decimal hold = 0;
		int count = 0;
		for ( int i = 0; i < f.Length - 1; i++ ) {
			if ( f[i] == "hold" ) {
				decimal.TryParse( f[i + 1], System.Globalization.NumberStyles.Any,
					System.Globalization.CultureInfo.InvariantCulture, out hold );
			} else if ( f[i] == "n" ) {
				int.TryParse( f[i + 1], out count );
			}
		}
		if ( hold <= 0 ) return;

		// Nur das Spiel, das von hier gestartet wurde, darf den Regler bewegen -
		// bis sein Protokoll nach dem Ende einmal ganz gelesen ist: ein altes
		// Protokoll wuerde sonst beim Start den gespeicherten Wert ueberschreiben
		if ( game is null ) return;

		aimLearned.Text = $"gelernt: {hold.ToString( "0.00", System.Globalization.CultureInfo.InvariantCulture )} s aus {count} Schüssen";
		var rounded = Math.Round( hold, 1 );
		if ( aimLearn.Checked && rounded >= aimLead.Minimum && rounded <= aimLead.Maximum && aimLead.Value != rounded ) {
			aimLead.Value = rounded;
		}
	}

	// Eine Zeile "aim shot:" aus dem Protokoll
	sealed class Shot {
		public int Frame, Lead, Distance;
		public int Me = -1;				// eigene Client-Nummer, um Einschlaege zuzuordnen
		public int World = -1;			// Server-Frame, gegen den der Schuss lief (neuere Protokolle)
		public bool InAir;
		public bool Assisted = true;	// aeltere Protokolle kennen das Feld nicht
		public double Error;
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
				case "world": int.TryParse( f[i + 1], out shot.World ); break;
				case "eye": ReadPoint( f, i + 1, shot.Eye ); break;
				case "plain": ReadPoint( f, i + 1, shot.Plain ); break;
					case "lead": int.TryParse( f[i + 1], out shot.Lead ); break;
					case "error":
						double.TryParse( f[i + 1], System.Globalization.CultureInfo.InvariantCulture, out shot.Error );
						break;
					case "at":
						if ( i + 3 < f.Length ) shot.Position = $"{f[i + 1]} {f[i + 2]} {f[i + 3]}";
						break;
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
	static string DescribeMiss( Shot shot, Impact impact, out double units ) {
		units = 0;
		if ( !impact.Bots.TryGetValue( shot.Target, out var bot ) ) return "";

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
		public int Band, Samples;
		public double From, Factor, Scatter, Reach;

		public static Tune? Parse( string line ) {
			var f = line[10..].Split( ' ', StringSplitOptions.RemoveEmptyEntries );
			if ( f.Length < 3 ) return null;

			var t = new Tune { Weapon = f[0], Scatter = -1 };
			for ( int i = 1; i < f.Length - 1; i++ ) {
				switch ( f[i] ) {
				case "band": int.TryParse( f[i + 1], out t.Band ); break;
				case "n": int.TryParse( f[i + 1], out t.Samples ); break;
				case "from": t.From = Num( f[i + 1] ); break;
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
	void UpdateTune( Dictionary<string, Tune> tunes ) {
		var rows = new List<ListViewItem>();
		int samples = 0;

		foreach ( var t in tunes.Values.OrderBy( t => t.Weapon ).ThenBy( t => t.Band ) ) {
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

			var row = new ListViewItem( t.Weapon ) { ForeColor = colour };
			row.SubItems.Add( t.From.ToString( "0.0", System.Globalization.CultureInfo.InvariantCulture ) + " s" );
			row.SubItems.Add( t.Samples.ToString() );
			row.SubItems.Add( t.Factor.ToString( "0.00", System.Globalization.CultureInfo.InvariantCulture ) );
			row.SubItems.Add( t.Scatter < 0 ? "—" : t.Scatter.ToString( "0" ) );
			row.SubItems.Add( t.Reach.ToString( "0" ) );
			row.SubItems.Add( outlook );
			rows.Add( row );
		}

		statTuneBoxes.Text = rows.Count.ToString();
		statTuneSamples.Text = samples.ToString();

		tuneView.BeginUpdate();
		tuneView.Items.Clear();
		tuneView.Items.AddRange( rows.ToArray() );
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

		rankView.BeginUpdate();
		rankView.Items.Clear();
		rankView.Items.AddRange( rows.ToArray() );
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
			if ( impact is not null ) missText = DescribeMiss( shot, impact, out missUnits );

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
				shot.InAir ? "ja" : "nein",
				shot.Lead + " ms",
				shot.Error.ToString( "0.00" ) + "°",
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

		shotView.BeginUpdate();
		shotView.Items.Clear();
		shotView.Items.AddRange( rows.TakeLast( 300 ).ToArray() );
		if ( shotView.Items.Count > 0 ) shotView.EnsureVisible( shotView.Items.Count - 1 );
		shotView.EndUpdate();
	}
}
