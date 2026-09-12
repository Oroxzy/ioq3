using System.Diagnostics;
using System.Text;

namespace HitsoundLab;

// Bedienoberflaeche fuer die Trefferton-Tests: schreibt eine Config, startet das
// Spiel damit und liest waehrenddessen dessen Konsolenprotokoll mit. Kein Zugriff
// auf den laufenden Prozess.
public class MainForm : Form {
	const string CfgName = "hitsoundlab.cfg";

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

	readonly CheckBox aimAssist = new() { Text = "Zielhilfe auf Bots (nur mit Server-Cheats)", AutoSize = true };
	readonly NumericUpDown aimStrength = new() { Minimum = 1, Maximum = 10, Value = 5, Width = 60 };

	readonly Label statHits = Number();
	readonly Label statFrames = Number();
	readonly Label statSounds = Number();
	readonly Label statMissed = Number();
	readonly TextBox logView = new() { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Font = new Font( "Consolas", 9 ), Dock = DockStyle.Fill };

	readonly Button start = new() { Text = "Spiel starten", Width = 140, Height = 34 };
	readonly Label status = new() { AutoSize = true, ForeColor = Color.DimGray };

	readonly System.Windows.Forms.Timer poll = new() { Interval = 500 };
	string logPath = "";

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
		// auch mitlesen, wenn das Spiel von Hand gestartet wurde
		logPath = Path.Combine( HomePath, "qconsole.log" );
		start.Click += ( _, _ ) => StartGame();
		poll.Tick += ( _, _ ) => RefreshStats();

		Controls.Add( BuildLayout() );
		poll.Start();
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
				Pad( start ), Pad( status ) ) );
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
			Row( Pad( aimAssist ), Labelled( "Stärke:", aimStrength ) ),
			Row( new Label {
				Text = "greift nur auf Gegner, die der Server als Bot meldet, und nur wenn der Server Cheats erlaubt",
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

	GroupBox BuildStatsBox() {
		var box = new GroupBox { Text = "Mitschrift", Dock = DockStyle.Fill, Padding = new Padding( 10 ) };
		var grid = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = 1, RowCount = 2 };
		grid.RowStyles.Add( new RowStyle( SizeType.AutoSize ) );
		grid.RowStyles.Add( new RowStyle( SizeType.Percent, 100 ) );
		grid.ColumnStyles.Add( new ColumnStyle( SizeType.Percent, 100 ) );

		// eine Zeile statt Spalten: bleibt auch in einem schmalen Fenster lesbar
		grid.Controls.Add( Row(
			Counter( "Schaden:", statHits ), Counter( "Treffer:", statFrames ),
			Counter( "Sounds:", statSounds ), Counter( "ohne Ton:", statMissed ) ), 0, 0 );
		grid.Controls.Add( logView, 0, 1 );
		box.Controls.Add( grid );
		return box;
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
			Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.MyDocuments ), "ioQuake3" ),
			Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.MyDocuments ),
				"GitHub", "ioq3", "build", "release-mingw64-x86_64" ),
		};

		foreach ( var dir in candidates ) {
			if ( File.Exists( Path.Combine( dir, "ioquake3.exe" ) ) ) return dir;
		}

		return candidates[0];
	}

	static string HomePath =>
		Path.Combine( Environment.GetFolderPath( Environment.SpecialFolder.ApplicationData ), "Quake3", "baseq3" );

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
		cfg.AppendLine( "set logfile 2" );
		cfg.AppendLine( "set bot_nochat 1" );
		// devmap statt map: nur so erlaubt der eigene Server Cheats, und nur dann
		// laesst die Engine die Zielhilfe ueberhaupt zu
		cfg.AppendLine( $"devmap {map.Text}" );
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
			if ( File.Exists( logPath ) ) File.Delete( logPath );

			Process.Start( new ProcessStartInfo {
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
		if ( logPath.Length == 0 || !File.Exists( logPath ) ) return;

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

		foreach ( var line in text.Split( '\n' ) ) {
			var trimmed = line.TrimEnd( '\r' );

			if ( trimmed.StartsWith( "hit on " ) ) {
				hits++;
				// Treffer im selben Server-Frame beantwortet das Spiel mit einem Ton,
				// deshalb zaehlen die Frames und nicht die einzelnen Schadensereignisse
				var mark = trimmed.LastIndexOf( " frame ", StringComparison.Ordinal );
				frames.Add( mark >= 0 ? trimmed[( mark + 7 )..] : "#" + hits );
				recent.Add( trimmed );
			} else if ( trimmed.StartsWith( "hit sound: " ) ) {
				var rest = trimmed[11..];
				if ( rest.Length > 0 && ( char.IsDigit( rest[0] ) || rest.StartsWith( "kill" ) ) ) {
					sounds++;
					recent.Add( trimmed );
				}
			}
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
}
