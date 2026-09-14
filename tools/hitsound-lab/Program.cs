namespace HitsoundLab;

static class Program {
	// Wohin ein Absturz geschrieben wird, damit er nachlesbar ist statt nur
	// in einem Dialog zu stehen, den man wegklickt
	static string CrashPath => Path.Combine(
		Environment.GetFolderPath( Environment.SpecialFolder.ApplicationData ), "HitsoundLab", "error.log" );

	static void Note( string where, object? problem ) {
		try {
			Directory.CreateDirectory( Path.GetDirectoryName( CrashPath )! );
			File.AppendAllText( CrashPath, $"--- {DateTime.Now:yyyy-MM-dd HH:mm:ss} {where}\n{problem}\n\n" );
		} catch ( Exception ) {
		}
	}

	[STAThread]
	static void Main() {
		Application.ThreadException += ( _, e ) => Note( "Oberflaeche", e.Exception );
		AppDomain.CurrentDomain.UnhandledException += ( _, e ) => Note( "Hintergrund", e.ExceptionObject );

		ApplicationConfiguration.Initialize();
		using var iconStream = typeof( Program ).Assembly.GetManifestResourceStream( "HitsoundLab.AppIcon" )
			?? throw new InvalidOperationException( "The application icon resource is missing." );
		using var icon = new Icon( iconStream );
		using var form = new MainForm { Icon = icon };
		Application.Run( form );
	}
}
