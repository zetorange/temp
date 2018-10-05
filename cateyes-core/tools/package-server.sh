#!/bin/sh

if [ -z "$CATEYES_TOOLCHAIN" ]; then
  echo "CATEYES_TOOLCHAIN must be set" > /dev/stderr
  exit 1
fi

if [ -z "$CATEYES_VERSION" ]; then
  echo "CATEYES_VERSION must be set" > /dev/stderr
  exit 2
fi

if [ $# -ne 2 ]; then
  echo "Usage: $0 cateyes-server output.deb" > /dev/stderr
  exit 3
fi
executable="$1"
if [ ! -f "$executable" ]; then
  echo "$executable: not found" > /dev/stderr
  exit 4
fi
output_deb="$2"

if file "$executable" | grep -q arm64; then
  pkg_id=re.cateyes.server
  pkg_name="Cateyes"
  pkg_conflicts=re.cateyes.server32
else
  pkg_id=re.cateyes.server32
  pkg_name="Cateyes for 32-bit devices"
  pkg_conflicts=re.cateyes.server
fi

tmpdir="$(mktemp -d /tmp/package-server.XXXXXX)"

mkdir -p "$tmpdir/usr/sbin/"
cp "$executable" "$tmpdir/usr/sbin/cateyes-server"
chmod 755 "$tmpdir/usr/sbin/cateyes-server"

mkdir -p "$tmpdir/Library/LaunchDaemons/"
cat >"$tmpdir/Library/LaunchDaemons/re.cateyes.server.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>Label</key>
	<string>re.cateyes.server</string>
	<key>Program</key>
	<string>/usr/sbin/cateyes-server</string>
	<key>ProgramArguments</key>
	<array>
		<string>/usr/sbin/cateyes-server</string>
	</array>
	<key>EnvironmentVariables</key>
	<dict>
		<key>_MSSafeMode</key>
		<string>1</string>
	</dict>
	<key>UserName</key>
	<string>root</string>
	<key>MachServices</key>
	<dict>
		<key>com.apple.uikit.viewservice.cateyes</key>
		<true/>
	</dict>
	<key>RunAtLoad</key>
	<true/>
	<key>KeepAlive</key>
	<true/>
	<key>ThrottleInterval</key>
	<integer>5</integer>
	<key>ExecuteAllowed</key>
	<true/>
</dict>
</plist>
EOF
chmod 644 "$tmpdir/Library/LaunchDaemons/re.cateyes.server.plist"

installed_size=$(du -sk "$tmpdir" | cut -f1)

mkdir -p "$tmpdir/DEBIAN/"
cat >"$tmpdir/DEBIAN/control" <<EOF
Package: $pkg_id
Name: $pkg_name
Version: $CATEYES_VERSION
Priority: optional
Size: 1337
Installed-Size: $installed_size
Architecture: iphoneos-arm
Description: Inject JavaScript to explore iOS apps over USB.
Homepage: https://www.cateyes.re/
Maintainer: Ole André Vadla Ravnås <oleavr@nowsecure.com>
Author: Cateyes Developers <oleavr@nowsecure.com>
Section: Development
Conflicts: $pkg_conflicts
EOF
chmod 644 "$tmpdir/DEBIAN/control"

cat >"$tmpdir/DEBIAN/extrainst_" <<EOF
#!/bin/sh

if [[ \$1 == upgrade ]]; then
  /bin/launchctl unload /Library/LaunchDaemons/re.cateyes.server.plist
fi

if [[ \$1 == install || \$1 == upgrade ]]; then
  /bin/launchctl load /Library/LaunchDaemons/re.cateyes.server.plist
fi

exit 0
EOF
chmod 755 "$tmpdir/DEBIAN/extrainst_"
cat >"$tmpdir/DEBIAN/prerm" <<EOF
#!/bin/sh

if [[ \$1 == remove || \$1 == purge ]]; then
  /bin/launchctl unload /Library/LaunchDaemons/re.cateyes.server.plist
fi

exit 0
EOF
chmod 755 "$tmpdir/DEBIAN/prerm"

$CATEYES_TOOLCHAIN/bin/dpkg-deb -b "$tmpdir" "$output_deb"
package_size=$(expr $(du -sk "$output_deb" | cut -f1) \* 1024)

sudo chown -R 0:0 "$tmpdir"
sudo sed \
  -i "" \
  -e "s,^Size: 1337$,Size: $package_size,g" \
  "$tmpdir/DEBIAN/control"
sudo $CATEYES_TOOLCHAIN/bin/dpkg-deb -b "$tmpdir" "$output_deb"
sudo chown -R $(whoami) "$tmpdir" "$output_deb"

rm -rf "$tmpdir"
