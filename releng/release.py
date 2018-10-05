#!/usr/bin/env python3
from __future__ import print_function

if __name__ == '__main__':
    from contextlib import contextmanager
    from devkit import generate_devkit
    from distutils.spawn import find_executable
    import codecs
    import glob
    import os
    import platform
    import re
    import shutil
    import subprocess
    import sys
    import tempfile

    system = platform.system()
    slave = sys.argv[1]

    build_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
    if system == 'Darwin':
        build_os = 'macos-x86_64'
    else:
        build_os = system.lower()
    toolchain_dir = os.path.join(build_dir, "build", "toolchain-" + build_os)
    cateyes_core_dir = os.path.join(build_dir, "cateyes-core")
    cateyes_python_dir = os.path.join(build_dir, "cateyes-python")
    cateyes_node_dir = os.path.join(build_dir, "cateyes-node")
    cateyes_tools_dir = os.path.join(build_dir, "cateyes-tools")

    if system == 'Windows':
        szip = r"C:\Program Files\7-Zip\7z.exe"
        ssh = r"C:\Program Files (x86)\PuTTY\plink.exe"
        scp = r"C:\Program Files (x86)\PuTTY\pscp.exe"
    else:
        szip = "7z"
        ssh = "ssh"
        scp = "scp"

    raw_version = subprocess.check_output(["git", "describe", "--tags", "--always", "--long"], cwd=build_dir).decode('utf-8').strip().replace("-", ".")
    (major, minor, micro, nano, commit) = raw_version.split(".")
    version = "%d.%d.%d" % (int(major), int(minor), int(micro))
    tag_name = str(version)

    def upload_python_bindings_to_pypi(interpreter, extension, extra_env = {}, sdist = False):
        env = {}
        env.update(os.environ)
        env.update({
            'CATEYES_VERSION': version,
            'CATEYES_EXTENSION': extension
        })
        env.update(extra_env)

        targets = []
        if sdist:
            targets.append("sdist")
        targets.extend(["bdist_egg", "upload"])

        subprocess.call([interpreter, "setup.py"] + targets, cwd=cateyes_python_dir, env=env)

    def upload_python_debs(distro_name, package_name_prefix, interpreter, extension, upload):
        env = {}
        env.update(os.environ)
        env.update({
            'CATEYES_VERSION': version,
            'CATEYES_EXTENSION': extension
        })

        for module_dir in [cateyes_python_dir, cateyes_tools_dir]:
            subprocess.check_call([
                "fpm",
                "--iteration=1." + distro_name,
                "--maintainer=Ole André Vadla Ravnås <oleavr@cateyes.re>",
                "--vendor=Cateyes",
                "--category=Libraries",
                "--python-bin=" + interpreter,
                "--python-package-name-prefix=" + package_name_prefix,
                "--python-install-bin=/usr/bin",
                "--python-install-lib=/usr/lib/{}/dist-packages".format(os.path.basename(interpreter)),
                "-s", "python",
                "-t", "deb",
                "setup.py"
            ], cwd=module_dir, env=env)

        packages = glob.glob(os.path.join(cateyes_python_dir, "*.deb"))
        try:
            for package in packages:
                with open(package, "rb") as f:
                    upload(os.path.basename(package), "application/x-deb", f)
        finally:
            for package in packages:
                os.unlink(package)

    def upload_python_rpms(distro_name, package_name_prefix, interpreter, extension, upload):
        env = {}
        env.update(os.environ)
        env.update({
            'CATEYES_VERSION': version,
            'CATEYES_EXTENSION': extension
        })

        for module_dir in [cateyes_python_dir, cateyes_tools_dir]:
            subprocess.check_call([
                "fpm",
                "--iteration=1." + distro_name,
                "--maintainer=Ole André Vadla Ravnås <oleavr@cateyes.re>",
                "--vendor=Cateyes",
                "--python-bin=" + interpreter,
                "--python-package-name-prefix=" + package_name_prefix,
                "-s", "python",
                "-t", "rpm",
                "setup.py"
            ], cwd=module_dir, env=env)

        subprocess.check_call([
            "fpm",
            "--name={}-prompt-toolkit".format(package_name_prefix),
            "--version=1.0.15",
            "--iteration=1." + distro_name,
            "--maintainer=Ole André Vadla Ravnås <oleavr@cateyes.re>",
            "--vendor=Cateyes",
            "--python-bin=" + interpreter,
            "--python-package-name-prefix=" + package_name_prefix,
            "-s", "python",
            "-t", "rpm",
            "prompt-toolkit"
        ], cwd=cateyes_python_dir)

        packages = glob.glob(os.path.join(cateyes_python_dir, "*.rpm"))
        try:
            for package in packages:
                with open(package, "rb") as f:
                    upload(os.path.basename(package), "application/x-rpm", f)
        finally:
            for package in packages:
                os.unlink(package)

    def upload_node_bindings_to_npm(node, upload_to_github, publish, python2_interpreter=None, extra_build_args=[], extra_build_env=None):
        node_bin_dir = os.path.dirname(node)
        npm = os.path.join(node_bin_dir, "npm")
        if system == 'Windows':
            npm += '.cmd'

        env = dict(os.environ)
        env.update({
            'PATH': node_bin_dir + os.pathsep + os.getenv('PATH'),
            'CATEYES': build_dir
        })
        if python2_interpreter is not None:
            env['PYTHON'] = python2_interpreter

        def do(args, **kwargs):
            quoted_args = []
            for arg in args:
                if " " in arg:
                    # Assumes none of our arguments contain quotes
                    quoted_args.append('"{}"'.format(arg))
                else:
                    quoted_args.append(arg)
            command = " ".join(quoted_args)
            exit_code = subprocess.call(command, cwd=cateyes_node_dir, env=env, shell=True, **kwargs)
            if exit_code != 0:
                raise RuntimeError("Failed to run: " + command)
        def do_build_command(args):
            env_args = [". " + extra_build_env, "&&"] if extra_build_env is not None else []
            do(env_args + args + extra_build_args)
        def reset():
            do(["git", "clean", "-xffd"])
        reset()
        with package_version_temporarily_set_to(version, os.path.join(cateyes_node_dir, "package.json")):
            do_build_command([npm, "install"])
            if publish:
                do([npm, "publish"])
            do_build_command([npm, "run", "prebuild", "--", "-t", "8.0.0", "-t", "9.0.0", "-t", "10.0.0"])
            do_build_command([npm, "run", "prebuild", "--", "-t", "2.0.0", "-r", "electron"])
            packages = glob.glob(os.path.join(cateyes_node_dir, "prebuilds", "*.tar.gz"))
            for package in packages:
                with open(package, 'rb') as package_file:
                    upload_to_github(os.path.basename(package), "application/gzip", package_file.read())
        reset()

    def upload_meta_modules_to_npm(node):
        for module in ["cateyes-gadget-ios"]:
            upload_meta_module_to_npm(node, module)

    def upload_meta_module_to_npm(node, module_name):
        module_dir = os.path.join(build_dir, "releng", "modules", module_name)
        with package_version_temporarily_set_to(version, os.path.join(module_dir, "package.json")):
            subprocess.check_call(["npm", "publish"], cwd=module_dir)

    @contextmanager
    def package_version_temporarily_set_to(version, package_json_path):
        with codecs.open(package_json_path, "rb", 'utf-8') as f:
            package_json_original = f.read()

        package_json_versioned = re.sub(r'"version": "(.+)",', r'"version": "{}",'.format(version), package_json_original)
        with codecs.open(package_json_path, "wb", 'utf-8') as f:
            f.write(package_json_versioned)

        try:
            yield
        finally:
            with codecs.open(package_json_path, "wb", 'utf-8') as f:
                f.write(package_json_original)

    def upload_ios_deb(name, server):
        env = {
            'CATEYES_VERSION': version,
            'CATEYES_TOOLCHAIN': toolchain_dir
        }
        env.update(os.environ)
        deb = os.path.join(build_dir, "{}_{}_iphoneos-arm.deb".format(name, version))
        subprocess.call([os.path.join(cateyes_core_dir, "tools", "package-server.sh"), server, deb], env=env)
        subprocess.call([scp, deb, "cateyes@build.cateyes.re:/home/cateyes/public_html/debs/"])
        subprocess.call([ssh, "cateyes@build.cateyes.re", "cd /home/cateyes/public_html" +
            " && reprepro -Vb . --confdir /home/cateyes/.reprepo --ignore=forbiddenchar includedeb stable debs/" + os.path.basename(deb) +
            " && cp dists/stable/main/binary-iphoneos-arm/Packages.gz ."])
        os.unlink(deb)

    def get_github_uploader():
        from agithub.GitHub import GitHub
        import requests

        with open(os.path.expanduser("~/.cateyes-release-github-token"), "r") as f:
            token = f.read().strip()

        g = GitHub(token=token)
        def repo():
            return g.repos.cateyes.cateyes

        status, data = repo().releases.tags[tag_name].get()
        if status != 200:
            if status == 404:
                status, data = repo().releases.post(body={
                    'tag_name': tag_name,
                    'name': "Cateyes {}".format(version),
                    'body': "See http://www.cateyes.re/news/ for details.",
                })
            else:
                raise RuntimeError("Unexpected error trying to get current release; status={} data={}".format(status, data))

        upload_url = data['upload_url']
        upload_url = upload_url[:upload_url.index("{")]

        def upload(name, mimetype, data):
            try:
                r = requests.post(
                    url=upload_url,
                    params={
                        "name": name,
                    },
                    headers={
                        "Authorization": "Token {}".format(token),
                        "Content-Type": mimetype,
                    },
                    data=data)
                r.raise_for_status()
                print("Uploaded", name)
            except Exception as e:
                print("Skipping {}: {}".format(name, e))

        return upload

    def upload_file(name_template, path, upload):
        if system == 'Windows':
            asset_filename = (name_template + ".xz").format(version=version)
            data = subprocess.check_output([szip, "a", "-txz", "-so", asset_filename, path])
        else:
            asset_filename = (name_template + ".xz").format(version=version)
            data = subprocess.check_output(["xz", "-z", "-c", "-T", "0", path])
        upload(asset_filename, "application/x-xz", data)

    def upload_directory(name_template, path, upload):
        tarball_filename = (name_template + ".tar").format(version=version)
        asset_filename = tarball_filename + ".xz"

        output_dir = tempfile.mkdtemp(prefix="cateyes-release")
        try:
            dist_dir = os.path.join(output_dir, "dist")
            shutil.copytree(path, dist_dir)
            subprocess.check_call(["tar", "cf", "../" + tarball_filename, "."], cwd=dist_dir)
            subprocess.check_call(["xz", "-T", "0", tarball_filename], cwd=output_dir)
            with open(os.path.join(output_dir, asset_filename), 'rb') as f:
                tarball = f.read()
        finally:
            shutil.rmtree(output_dir)

        upload(asset_filename, "application/x-xz", tarball)

    def upload_devkits(host, upload):
        kits = [
            "cateyes-gum",
            "cateyes-gumjs",
            "cateyes-core",
        ]

        for kit in kits:
            if host.startswith("windows-"):
                asset_filename = "{}-devkit-{}-{}.exe".format(kit, version, host)
                asset_mimetype = "application/octet-stream"
            else:
                tarball_filename = "{}-devkit-{}-{}.tar".format(kit, version, host)
                asset_filename = tarball_filename + ".xz"
                asset_mimetype = "application/x-xz"

            output_dir = tempfile.mkdtemp(prefix="cateyes-release")
            try:
                try:
                    filenames = generate_devkit(kit, host, output_dir)
                except Exception as e:
                    print("Skipping {}: {}".format(asset_filename, e))
                    continue
                if host.startswith("windows-"):
                    subprocess.check_call([szip, "a", "-sfx7zCon.sfx", "-r", asset_filename, "."], cwd=output_dir)
                else:
                    subprocess.check_call(["tar", "cf", tarball_filename] + filenames, cwd=output_dir)
                    subprocess.check_call(["xz", "-T", "0", tarball_filename], cwd=output_dir)
                with open(os.path.join(output_dir, asset_filename), 'rb') as f:
                    asset_data = f.read()
            finally:
                shutil.rmtree(output_dir)

            upload(asset_filename, asset_mimetype, asset_data)

    if int(nano) == 0:
        if slave == 'windows':
            upload = get_github_uploader()

            upload_devkits("windows-x86", upload)
            upload_devkits("windows-x86_64", upload)

            upload_file("cateyes-server-{version}-windows-x86.exe", os.path.join(build_dir, "build", "cateyes-windows", "Win32-Release", "bin", "cateyes-server.exe"), upload)
            upload_file("cateyes-server-{version}-windows-x86_64.exe", os.path.join(build_dir, "build", "cateyes-windows", "x64-Release", "bin", "cateyes-server.exe"), upload)

            upload_file("cateyes-gadget-{version}-windows-x86.dll", os.path.join(build_dir, "build", "cateyes-windows", "Win32-Release", "bin", "cateyes-gadget.dll"), upload)
            upload_file("cateyes-gadget-{version}-windows-x86_64.dll", os.path.join(build_dir, "build", "cateyes-windows", "x64-Release", "bin", "cateyes-gadget.dll"), upload)

            upload_python_bindings_to_pypi(r"C:\Program Files (x86)\Python 2.7\python.exe",
                os.path.join(build_dir, "build", "cateyes-windows", "Win32-Release", "lib", "python2.7", "site-packages", "_cateyes.pyd"))
            upload_python_bindings_to_pypi(r"C:\Program Files\Python 2.7\python.exe",
                os.path.join(build_dir, "build", "cateyes-windows", "x64-Release", "lib", "python2.7", "site-packages", "_cateyes.pyd"))
            upload_python_bindings_to_pypi(r"C:\Program Files (x86)\Python 3.6\python.exe",
                os.path.join(build_dir, "build", "cateyes-windows", "Win32-Release", "lib", "python3.6", "site-packages", "_cateyes.pyd"))
            upload_python_bindings_to_pypi(r"C:\Program Files\Python 3.6\python.exe",
                os.path.join(build_dir, "build", "cateyes-windows", "x64-Release", "lib", "python3.6", "site-packages", "_cateyes.pyd"), sdist=True)

            python2_interpreter=r"C:\Program Files\Python 2.7\python.exe"
            upload_node_bindings_to_npm(r"C:\Program Files (x86)\nodejs\node.exe", upload, publish=False, python2_interpreter=python2_interpreter)
            upload_node_bindings_to_npm(r"C:\Program Files\nodejs\node.exe", upload, publish=False, python2_interpreter=python2_interpreter)
        elif slave == 'macos':
            upload = get_github_uploader()

            upload_devkits("macos-x86", upload)
            upload_devkits("macos-x86_64", upload)
            upload_devkits("ios-x86", upload)
            upload_devkits("ios-x86_64", upload)
            upload_devkits("ios-arm", upload)
            upload_devkits("ios-arm64", upload)

            upload_file("cateyes-server-{version}-macos-x86_64", os.path.join(build_dir, "build", "cateyes-macos-x86_64", "bin", "cateyes-server"), upload)
            upload_file("cateyes-server-{version}-ios-arm", os.path.join(build_dir, "build", "cateyes-ios-arm", "bin", "cateyes-server"), upload)
            upload_file("cateyes-server-{version}-ios-arm64", os.path.join(build_dir, "build", "cateyes-ios-arm64", "bin", "cateyes-server"), upload)

            upload_file("cateyes-gadget-{version}-macos-universal.dylib", os.path.join(build_dir, "build", "cateyes-macos-universal", "lib", "CateyesGadget.dylib"), upload)
            upload_file("cateyes-gadget-{version}-ios-universal.dylib", os.path.join(build_dir, "build", "cateyes-ios-universal", "lib", "CateyesGadget.dylib"), upload)

            upload_directory("cateyes-swift-{version}-macos-x86_64", os.path.join(build_dir, "cateyes-swift", "build", "Release"), upload)

            upload_directory("cateyes-qml-{version}-macos-x86_64", os.path.join(build_dir, "build", "cateyes-macos-x86_64", "lib", "qt5", "qml"), upload)

            for osx_minor in range(9, 13):
                upload_python_bindings_to_pypi("/usr/bin/python2.7",
                    os.path.join(build_dir, "build", "cateyes-macos-universal", "lib", "python2.7", "site-packages", "_cateyes.so"),
                    { '_PYTHON_HOST_PLATFORM': "macosx-10.%d-intel" % osx_minor })
            upload_python_bindings_to_pypi("/usr/local/bin/python3.6",
                os.path.join(build_dir, "build", "cateyes-macos-universal", "lib", "python3.6", "site-packages", "_cateyes.so"))

            upload_node_bindings_to_npm("/opt/node-64/bin/node", upload, publish=True)
            upload_meta_modules_to_npm("/opt/node-64/bin/node")

            upload_ios_deb("cateyes", os.path.join(build_dir, "build", "cateyes-ios-arm64", "bin", "cateyes-server"))
            upload_ios_deb("cateyes32", os.path.join(build_dir, "build", "cateyes-ios-arm", "bin", "cateyes-server"))
        elif slave == 'linux':
            upload = get_github_uploader()

            upload_devkits("linux-x86", upload)
            upload_devkits("linux-x86_64", upload)

            upload_file("cateyes-server-{version}-linux-x86", os.path.join(build_dir, "build", "cateyes-linux-x86", "bin", "cateyes-server"), upload)
            upload_file("cateyes-server-{version}-linux-x86_64", os.path.join(build_dir, "build", "cateyes-linux-x86_64", "bin", "cateyes-server"), upload)

            upload_file("cateyes-gadget-{version}-linux-x86.so", os.path.join(build_dir, "build", "cateyes-linux-x86", "lib", "cateyes-gadget.so"), upload)
            upload_file("cateyes-gadget-{version}-linux-x86_64.so", os.path.join(build_dir, "build", "cateyes-linux-x86_64", "lib", "cateyes-gadget.so"), upload)

            upload_python_bindings_to_pypi("/opt/python27-32/bin/python2.7",
                os.path.join(build_dir, "build", "cateyes-linux-x86", "lib", "python2.7", "site-packages", "_cateyes.so"),
                { 'LD_LIBRARY_PATH': "/opt/python27-32/lib", '_PYTHON_HOST_PLATFORM': "linux-i686" })
            upload_python_bindings_to_pypi("/opt/python27-64/bin/python2.7",
                os.path.join(build_dir, "build", "cateyes-linux-x86_64", "lib", "python2.7", "site-packages", "_cateyes.so"),
                { 'LD_LIBRARY_PATH': "/opt/python27-64/lib", '_PYTHON_HOST_PLATFORM': "linux-x86_64" })
            upload_python_bindings_to_pypi("/opt/python36-32/bin/python3.6",
                os.path.join(build_dir, "build", "cateyes-linux-x86", "lib", "python3.6", "site-packages", "_cateyes.so"),
                { 'LD_LIBRARY_PATH': "/opt/python36-32/lib", '_PYTHON_HOST_PLATFORM': "linux-i686" })
            upload_python_bindings_to_pypi("/opt/python36-64/bin/python3.6",
                os.path.join(build_dir, "build", "cateyes-linux-x86_64", "lib", "python3.6", "site-packages", "_cateyes.so"),
                { 'LD_LIBRARY_PATH': "/opt/python36-64/lib", '_PYTHON_HOST_PLATFORM': "linux-x86_64" })

            upload_node_bindings_to_npm("/opt/node-32/bin/node", upload, publish=False)
            upload_node_bindings_to_npm("/opt/node-64/bin/node", upload, publish=False)
        elif slave == 'ubuntu_16_04-x86_64':
            upload = get_github_uploader()

            upload_python_debs("ubuntu-xenial", "python", "/usr/bin/python2.7",
                os.path.join(build_dir, "build", "cateyes-linux-x86_64", "lib", "python2.7", "site-packages", "_cateyes.so"),
                upload)
            upload_python_debs("ubuntu-xenial", "python3", "/usr/bin/python3.5",
                os.path.join(build_dir, "build", "cateyes-linux-x86_64", "lib", "python3.5", "site-packages", "_cateyes.so"),
                upload)
        elif slave == 'ubuntu_18_04-x86_64':
            upload = get_github_uploader()

            upload_python_debs("ubuntu-bionic", "python", "/usr/bin/python2.7",
                os.path.join(build_dir, "build", "cateyes_thin-linux-x86_64", "lib", "python2.7", "site-packages", "_cateyes.so"),
                upload)
            upload_python_debs("ubuntu-bionic", "python3", "/usr/bin/python3.6",
                os.path.join(build_dir, "build", "cateyes_thin-linux-x86_64", "lib", "python3.6", "site-packages", "_cateyes.so"),
                upload)
        elif slave == 'fedora_28-x86_64':
            upload = get_github_uploader()

            upload_python_rpms("fc28", "python2", "/usr/bin/python2.7",
                os.path.join(build_dir, "build", "cateyes-linux-x86_64", "lib", "python2.7", "site-packages", "_cateyes.so"),
                upload)
            upload_python_rpms("fc28", "python3", "/usr/bin/python3.6",
                os.path.join(build_dir, "build", "cateyes-linux-x86_64", "lib", "python3.6", "site-packages", "_cateyes.so"),
                upload)
        elif slave == 'pi':
            upload = get_github_uploader()

            upload_node_bindings_to_npm(find_executable("node"), upload, publish=False,
                    extra_build_args=["--arch=arm"],
                    extra_build_env=os.path.join(build_dir, "build", "cateyes-env-linux-armhf.rc"))
        elif slave == 'android':
            upload = get_github_uploader()

            upload_devkits("android-x86", upload)
            upload_devkits("android-x86_64", upload)
            upload_devkits("android-arm", upload)
            upload_devkits("android-arm64", upload)

            upload_file("cateyes-server-{version}-android-x86", os.path.join(build_dir, "build", "cateyes-android-x86", "bin", "cateyes-server"), upload)
            upload_file("cateyes-server-{version}-android-x86_64", os.path.join(build_dir, "build", "cateyes-android-x86_64", "bin", "cateyes-server"), upload)
            upload_file("cateyes-server-{version}-android-arm", os.path.join(build_dir, "build", "cateyes-android-arm", "bin", "cateyes-server"), upload)
            upload_file("cateyes-server-{version}-android-arm64", os.path.join(build_dir, "build", "cateyes-android-arm64", "bin", "cateyes-server"), upload)

            upload_file("cateyes-inject-{version}-android-x86", os.path.join(build_dir, "build", "cateyes-android-x86", "bin", "cateyes-inject"), upload)
            upload_file("cateyes-inject-{version}-android-x86_64", os.path.join(build_dir, "build", "cateyes-android-x86_64", "bin", "cateyes-inject"), upload)
            upload_file("cateyes-inject-{version}-android-arm", os.path.join(build_dir, "build", "cateyes-android-arm", "bin", "cateyes-inject"), upload)
            upload_file("cateyes-inject-{version}-android-arm64", os.path.join(build_dir, "build", "cateyes-android-arm64", "bin", "cateyes-inject"), upload)

            upload_file("cateyes-gadget-{version}-android-x86.so", os.path.join(build_dir, "build", "cateyes-android-x86", "lib", "cateyes-gadget.so"), upload)
            upload_file("cateyes-gadget-{version}-android-x86_64.so", os.path.join(build_dir, "build", "cateyes-android-x86_64", "lib", "cateyes-gadget.so"), upload)
            upload_file("cateyes-gadget-{version}-android-arm.so", os.path.join(build_dir, "build", "cateyes-android-arm", "lib", "cateyes-gadget.so"), upload)
            upload_file("cateyes-gadget-{version}-android-arm64.so", os.path.join(build_dir, "build", "cateyes-android-arm64", "lib", "cateyes-gadget.so"), upload)
        elif slave == 'arm':
            upload = get_github_uploader()

            upload_devkits("linux-arm", upload)
            upload_devkits("linux-armhf", upload)

            upload_file("cateyes-server-{version}-linux-arm", os.path.join(build_dir, "build", "cateyes-linux-arm", "bin", "cateyes-server"), upload)
            upload_file("cateyes-server-{version}-linux-armhf", os.path.join(build_dir, "build", "cateyes-linux-armhf", "bin", "cateyes-server"), upload)

            upload_file("cateyes-gadget-{version}-linux-arm.so", os.path.join(build_dir, "build", "cateyes-linux-arm", "lib", "cateyes-gadget.so"), upload)
            upload_file("cateyes-gadget-{version}-linux-armhf.so", os.path.join(build_dir, "build", "cateyes-linux-armhf", "lib", "cateyes-gadget.so"), upload)
        elif slave == 'mips':
            upload = get_github_uploader()

            upload_devkits("linux-mipsel", upload)

            upload_file("cateyes-server-{version}-linux-mipsel", os.path.join(build_dir, "build", "cateyes-linux-mipsel", "bin", "cateyes-server"), upload)

            upload_file("cateyes-gadget-{version}-linux-mipsel.so", os.path.join(build_dir, "build", "cateyes-linux-mipsel", "lib", "cateyes-gadget.so"), upload)
        elif slave == 'qnx-arm':
            upload = get_github_uploader()

            upload_devkits("qnx-arm", upload)
            upload_devkits("qnx-armeabi", upload)

            upload_file("cateyes-server-{version}-qnx-arm", os.path.join(build_dir, "build", "cateyes-qnx-arm", "bin", "cateyes-server"), upload)
            upload_file("cateyes-server-{version}-qnx-armeabi", os.path.join(build_dir, "build", "cateyes-qnx-armeabi", "bin", "cateyes-server"), upload)

            upload_file("cateyes-gadget-{version}-qnx-arm.so", os.path.join(build_dir, "build", "cateyes-qnx-arm", "lib", "cateyes-gadget.so"), upload)
            upload_file("cateyes-gadget-{version}-qnx-armeabi.so", os.path.join(build_dir, "build", "cateyes-qnx-armeabi", "lib", "cateyes-gadget.so"), upload)
