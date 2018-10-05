namespace Cateyes.SystemTest {
	public static void add_tests () {
		GLib.Test.add_func ("/System/enumerate-processes-returns-processes-with-icons", () => {
			var timer = new Timer ();
			var processes = System.enumerate_processes ();
			var time_spent_on_first_run = timer.elapsed ();

			assert (processes.length > 0);

			switch (Cateyes.Test.os ()) {
				case Cateyes.Test.OS.WINDOWS:
				case Cateyes.Test.OS.IOS:
					int num_icons_seen = 0;
					foreach (var p in processes) {
						if (p.small_icon.pixels != "" && p.large_icon.pixels != "")
							num_icons_seen++;
					}
					assert (num_icons_seen > 0);
					break;
			}

			timer.start ();
			processes = System.enumerate_processes ();
			var time_spent_on_second_run = timer.elapsed ();

			if (GLib.Test.verbose ())
				stdout.printf (" [spent %f and %f] ", time_spent_on_first_run, time_spent_on_second_run);

			if (Cateyes.Test.os () == Cateyes.Test.OS.IOS) {
				assert (time_spent_on_second_run <= time_spent_on_first_run / 2.0);
			}
		});
	}
}
