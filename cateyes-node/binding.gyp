{
  "variables": {
    "conditions": [
      ["OS=='win'", {
        "cateyes_host": "windows",
      }],
      ["OS=='mac' and target_arch=='ia32'", {
        "cateyes_host": "macos-x86",
      }],
      ["OS=='mac' and target_arch=='x64'", {
        "cateyes_host": "macos-x86_64",
      }],
      ["OS=='linux' and target_arch=='ia32'", {
        "cateyes_host": "linux-x86",
      }],
      ["OS=='linux' and target_arch=='x64'", {
        "cateyes_host": "linux-x86_64",
      }],
      ["OS=='linux' and target_arch=='arm'", {
        "cateyes_host": "linux-armhf",
      }],
    ],
    "cateyes_host_msvs": "unix",
    "build_v8_with_gn": 0,
  },
  "targets": [
    {
      "variables": {
        "conditions": [
          ["OS=='win' and target_arch=='ia32'", {
            "cateyes_host_msvs": "Win32-<(CONFIGURATION_NAME)",
          }],
          ["OS=='win' and target_arch=='x64'", {
            "cateyes_host_msvs": "x64-<(CONFIGURATION_NAME)",
          }],
        ],
      },
      "target_name": "cateyes_binding",
      "sources": [
        "src/addon.cc",
        "src/device_manager.cc",
        "src/device.cc",
        "src/application.cc",
        "src/process.cc",
        "src/spawn.cc",
        "src/child.cc",
        "src/icon.cc",
        "src/session.cc",
        "src/script.cc",
        "src/signals.cc",
        "src/glib_object.cc",
        "src/runtime.cc",
        "src/uv_context.cc",
        "src/glib_context.cc",
      ],
      "target_conditions": [
        ["OS=='win'", {
          "include_dirs": [
            "$(CATEYES)/build/tmp-windows/<(cateyes_host_msvs)/cateyes-core",
            "$(CATEYES)/build/sdk-windows/<(cateyes_host_msvs)/include/json-glib-1.0",
            "$(CATEYES)/build/sdk-windows/<(cateyes_host_msvs)/include/gee-0.8",
            "$(CATEYES)/build/sdk-windows/<(cateyes_host_msvs)/include/glib-2.0",
            "$(CATEYES)/build/sdk-windows/<(cateyes_host_msvs)/lib/glib-2.0/include",
            "<!(node -e \"require(\'nan\')\")",
          ],
          "library_dirs": [
            "$(CATEYES)/build/tmp-windows/<(cateyes_host_msvs)/cateyes-core",
            "$(CATEYES)/build/sdk-windows/<(cateyes_host_msvs)/lib",
          ],
          "libraries": [
            "-lcateyes-core.lib",
            "-ljson-glib-1.0.lib",
            "-lgee-0.8.lib",
            "-lgio-2.0.lib",
            "-lgthread-2.0.lib",
            "-lgobject-2.0.lib",
            "-lgmodule-2.0.lib",
            "-lglib-2.0.lib",
            "-lz.lib",
            "-lffi.lib",
            "-lintl.lib",
            "-ldnsapi.lib",
            "-liphlpapi.lib",
            "-lole32.lib",
            "-lpsapi.lib",
            "-lshlwapi.lib",
            "-lwinmm.lib",
            "-lws2_32.lib",
          ],
        }, {
          "include_dirs": [
            "$(CATEYES)/build/cateyes-<(cateyes_host)/include/cateyes-1.0",
            "$(CATEYES)/build/sdk-<(cateyes_host)/include/json-glib-1.0",
            "$(CATEYES)/build/sdk-<(cateyes_host)/include/glib-2.0",
            "$(CATEYES)/build/sdk-<(cateyes_host)/lib/glib-2.0/include",
            "<!(node -e \"require(\'nan\')\")",
          ],
          "library_dirs": [
            "$(CATEYES)/build/cateyes-<(cateyes_host)/lib",
            "$(CATEYES)/build/sdk-<(cateyes_host)/lib",
          ],
          "libraries": [
            "-lcateyes-core-1.0",
            "-ljson-glib-1.0",
            "-lcateyes-gum-1.0",
            "-lcapstone",
            "-lgee-0.8",
            "-lgio-2.0",
            "-lgthread-2.0",
            "-lgobject-2.0",
            "-lgmodule-2.0",
            "-lglib-2.0",
            "-lffi",
            "-lz",
          ],
        }],
        ["OS=='mac'", {
          "xcode_settings": {
            "OTHER_CFLAGS": [
              "-std=c++11",
              "-stdlib=libc++",
              "-mmacosx-version-min=10.9",
            ],
            "OTHER_LDFLAGS": [
              "-stdlib=libc++",
              "-Wl,-macosx_version_min,10.9",
              "-Wl,-dead_strip",
              "-Wl,-exported_symbols_list,binding.symbols",
            ],
          },
          "libraries": [
            "-lbsm",
            "-liconv",
            "-Wl,-framework -Wl,Foundation -Wl,-framework -Wl,AppKit",
            "-mmacosx-version-min=10.9",
          ],
        }],
        ["OS=='linux'", {
          "cflags": [
            "-std=c++11",
            "-ffunction-sections",
            "-fdata-sections",
          ],
          "ldflags": [
            "-Wl,--gc-sections",
            "-Wl,-z,noexecstack",
            "-Wl,--version-script",
            "-Wl,../binding.version",
          ],
          "library_dirs": [
            "$(CATEYES)/build/sdk-<(cateyes_host)/lib32",
            "$(CATEYES)/build/sdk-<(cateyes_host)/lib64",
          ],
        }],
      ],
    },
  ],
}
