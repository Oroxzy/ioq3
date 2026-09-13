namespace HitsoundLab;

static class Program {
	[STAThread]
	static void Main() {
		ApplicationConfiguration.Initialize();
		using var iconStream = typeof( Program ).Assembly.GetManifestResourceStream( "HitsoundLab.AppIcon" )
			?? throw new InvalidOperationException( "The application icon resource is missing." );
		using var icon = new Icon( iconStream );
		using var form = new MainForm { Icon = icon };
		Application.Run( form );
	}
}
