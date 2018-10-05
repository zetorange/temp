namespace Cateyes.GadgetTest {
	public static void add_tests () {
		GLib.Test.add_func ("/Gadget/Standalone/load-script", Standalone.load_script);
	}

	namespace Standalone {
		private static void load_script () {
			if (!GLib.Test.slow ()) {
				stdout.printf ("<skipping, run in slow mode> ");
				return;
			}

			var cateyes_root_dir = Path.get_dirname (Path.get_dirname (Cateyes.Test.Process.current.filename));
			var gadget_filename = Path.build_filename (cateyes_root_dir, "lib", "gadget", ".libs", "libcateyes-gadget" + Cateyes.Test.os_library_suffix ());

			var tests_dir = Path.get_dirname (Cateyes.Test.Process.current.filename);
			var data_dir = Path.build_filename (tests_dir, "data");
			var script_file = File.new_for_path (Path.build_filename (data_dir, "test-gadget-standalone.js"));

			var envp = new string[] {
				"DYLD_INSERT_LIBRARIES=" + gadget_filename,
				"CATEYES_GADGET_SCRIPT=" + script_file.get_path ()
			};

			try {
				var process = Cateyes.Test.Process.start (Cateyes.Test.Labrats.path_to_executable ("sleeper"), null, envp);
				var exitcode = process.join (5000);
				assert (exitcode == 123);
			} catch (Error e) {
				printerr ("\nFAIL: %s\n\n", e.message);
				assert_not_reached ();
			}
		}
	}
}
