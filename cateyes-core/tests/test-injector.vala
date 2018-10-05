namespace Cateyes.InjectorTest {
	public static void add_tests () {
		GLib.Test.add_func ("/Injector/inject-dynamic-current-arch", () => {
			test_dynamic_injection (Cateyes.Test.Arch.CURRENT);
		});

		GLib.Test.add_func ("/Injector/inject-dynamic-other-arch", () => {
			test_dynamic_injection (Cateyes.Test.Arch.OTHER);
		});

		GLib.Test.add_func ("/Injector/inject-resident-current-arch", () => {
			test_resident_injection (Cateyes.Test.Arch.CURRENT);
		});

		GLib.Test.add_func ("/Injector/inject-resident-other-arch", () => {
			test_resident_injection (Cateyes.Test.Arch.OTHER);
		});

		GLib.Test.add_func ("/Injector/resource-leaks", test_resource_leaks);

#if DARWIN
		GLib.Test.add_func ("/Injector/suspended-injection-current-arch", () => {
			test_suspended_injection (Cateyes.Test.Arch.CURRENT);
		});

		GLib.Test.add_func ("/Injector/suspended-injection-other-arch", () => {
			test_suspended_injection (Cateyes.Test.Arch.OTHER);
		});
#endif
	}

	private static void test_dynamic_injection (Cateyes.Test.Arch arch) {
		var logfile = File.new_for_path (Cateyes.Test.path_to_temporary_file ("test-dynamic-injection.log"));
		try {
			logfile.delete ();
		} catch (GLib.Error delete_error) {
		}
		var envp = new string[] {
			"CATEYES_LABRAT_LOGFILE=" + logfile.get_path ()
		};

		var rat = new Labrat ("sleeper", envp, arch);

		rat.inject ("simple-agent", "", arch);
		rat.wait_for_uninject ();
		assert (content_of (logfile) == ">m<");

		var requested_exit_code = 43;
		rat.inject ("simple-agent", requested_exit_code.to_string (), arch);
		rat.wait_for_uninject ();

		switch (Cateyes.Test.os ()) {
			case Cateyes.Test.OS.MACOS:   // Gum.Darwin.Mapper
			case Cateyes.Test.OS.IOS:     // Gum.Darwin.Mapper
			case Cateyes.Test.OS.ANDROID: // Bionic's behavior
				assert (content_of (logfile) == ">m<>m");
				break;
			case Cateyes.Test.OS.LINUX:
				if (Cateyes.Test.libc () == Cateyes.Test.Libc.UCLIBC) {
					assert (content_of (logfile) == ">m<>m");
				} else {
					assert (content_of (logfile) == ">m<>m<");
				}
				break;
			default:
				assert (content_of (logfile) == ">m<>m<");
				break;
		}

		var exit_code = rat.wait_for_process_to_exit ();
		assert (exit_code == requested_exit_code);

		try {
			logfile.delete ();
		} catch (GLib.Error delete_error) {
			assert_not_reached ();
		}

		rat.close ();
	}

	private static void test_resident_injection (Cateyes.Test.Arch arch) {
		var logfile = File.new_for_path (Cateyes.Test.path_to_temporary_file ("test-resident-injection.log"));
		try {
			logfile.delete ();
		} catch (GLib.Error delete_error) {
		}
		var envp = new string[] {
			"CATEYES_LABRAT_LOGFILE=" + logfile.get_path ()
		};

		var rat = new Labrat ("sleeper", envp, arch);

		rat.inject ("resident-agent", "", arch);
		assert (!rat.try_wait_for_uninject (500));
		assert (content_of (logfile) == ">m");

		try {
			rat.process.kill ();
		} catch (Error e) {
			assert_not_reached ();
		}

		assert (!rat.try_wait_for_uninject (500));

		try {
			logfile.delete ();
		} catch (GLib.Error delete_error) {
			assert_not_reached ();
		}
	}

	private static void test_resource_leaks () {
		var logfile = File.new_for_path (Cateyes.Test.path_to_temporary_file ("test-leaks.log"));
		var envp = new string[] {
			"CATEYES_LABRAT_LOGFILE=" + logfile.get_path ()
		};

		var rat = new Labrat ("sleeper", envp);

		/* Warm up static allocations */
		rat.inject ("simple-agent", "");
		rat.wait_for_uninject ();
		rat.wait_for_cleanup ();

		var usage_before = rat.process.snapshot_resource_usage ();

		rat.inject ("simple-agent", "");
		rat.wait_for_uninject ();
		rat.wait_for_cleanup ();

		var usage_after = rat.process.snapshot_resource_usage ();

		usage_after.assert_equals (usage_before);

		rat.inject ("simple-agent", "0");
		rat.wait_for_uninject ();
		rat.wait_for_process_to_exit ();

		rat.close ();
	}

#if DARWIN
	private static void test_suspended_injection (Cateyes.Test.Arch arch) {
		var logfile = File.new_for_path (Cateyes.Test.path_to_temporary_file ("test-suspended-injection.log"));
		try {
			logfile.delete ();
		} catch (GLib.Error delete_error) {
		}
		var envp = new string[] {
			"CATEYES_LABRAT_LOGFILE=" + logfile.get_path ()
		};

		var rat = new Labrat.suspended ("sleeper", envp, arch);

		rat.inject ("simple-agent", "", arch);
		rat.wait_for_uninject ();
		assert (content_of (logfile) == ">m<");

		rat.close ();
	}
#endif

	private static string content_of (File file) {
		try {
			uint8[] contents;
			file.load_contents (null, out contents, null);
			unowned string str = (string) contents;
			return str;
		} catch (GLib.Error load_error) {
			stderr.printf ("%s: %s\n", file.get_path (), load_error.message);
			assert_not_reached ();
		}
	}

	private class Labrat {
		public Cateyes.Test.Process? process {
			get;
			private set;
		}

		private Injector injector;

		public Labrat (string name, string[] envp, Cateyes.Test.Arch arch = Cateyes.Test.Arch.CURRENT) {
			try {
				process = Cateyes.Test.Process.start (Cateyes.Test.Labrats.path_to_executable (name), null, envp, arch);
			} catch (Error e) {
				printerr ("\nFAIL: %s\n\n", e.message);
				assert_not_reached ();
			}

			/* TODO: improve injectors to handle injection into a process that hasn't yet finished initializing */
			Thread.usleep (50000);
		}

		public Labrat.suspended (string name, string[] envp, Cateyes.Test.Arch arch = Cateyes.Test.Arch.CURRENT) {
			try {
				process = Cateyes.Test.Process.create (Cateyes.Test.Labrats.path_to_executable (name), null, envp, arch);
			} catch (Error e) {
				printerr ("\nFAIL: %s\n\n", e.message);
				assert_not_reached ();
			}
		}

		public void close () {
			var loop = new MainLoop ();
			Idle.add (() => {
				do_close.begin (loop);
				return false;
			});
			loop.run ();
		}

		private async void do_close (MainLoop loop) {
			if (injector != null) {
				yield injector.close ();
				injector = null;
			}
			process = null;

			/* Queue an idle handler, allowing MainContext to perform any outstanding completions, in turn cleaning up resources */
			Idle.add (() => {
				loop.quit ();
				return false;
			});
		}

		public void inject (string name, string data, Cateyes.Test.Arch arch = Cateyes.Test.Arch.CURRENT) {
			var loop = new MainLoop ();
			Idle.add (() => {
				perform_injection.begin (name, data, arch, loop);
				return false;
			});
			loop.run ();
		}

		private async void perform_injection (string name, string data, Cateyes.Test.Arch arch, MainLoop loop) {
			if (injector == null)
				injector = Injector.new ();

			try {
				var path = Cateyes.Test.Labrats.path_to_library (name, arch);
				assert (FileUtils.test (path, FileTest.EXISTS));

				yield injector.inject_library_file (process.id, path, "cateyes_agent_main", data);
			} catch (Error e) {
				printerr ("\nFAIL: %s\n\n", e.message);
				assert_not_reached ();
			}

			loop.quit ();
		}

		public void wait_for_uninject () {
			var success = try_wait_for_uninject (1000);
			assert (success);
		}

		public bool try_wait_for_uninject (uint timeout) {
			var loop = new MainLoop ();

			var handler_id = injector.uninjected.connect ((id) => {
				loop.quit ();
			});

			var timed_out = false;
			var timeout_id = Timeout.add (timeout, () => {
				timed_out = true;
				loop.quit ();
				return false;
			});

			loop.run ();

			if (!timed_out)
				Source.remove (timeout_id);
			injector.disconnect (handler_id);

			return !timed_out;
		}

		public void wait_for_cleanup () {
			var loop = new MainLoop ();

			/* The Darwin injector does cleanup 50ms after detecting that the remote thread is dead */
			Timeout.add (100, () => {
				loop.quit ();
				return false;
			});

			loop.run ();
		}

		public int wait_for_process_to_exit () {
			int exitcode = -1;

			try {
				exitcode = process.join (1000);
			} catch (Error e) {
				stdout.printf ("\n\nunexpected error: %s\n", e.message);
				assert_not_reached ();
			}

			return exitcode;
		}
	}
}
