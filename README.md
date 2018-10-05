# Cateyes

Dynamic instrumentation toolkit for developers, reverse-engineers, and security
researchers.

Two ways to install—
===

## 1. Install from prebuilt binaries

This is the recommended way to get started. All you need to do is:

    pip install cateyes-tools # CLI tools
    pip install cateyes       # Python bindings
    npm install cateyes       # Node.js bindings

You may also download pre-built binaries for various operating systems from
[https://build.cateyes.re/cateyes/](https://build.cateyes.re/cateyes/).

## 2. Build your own binaries

### Dependencies

For running the Cateyes CLI tools, i.e. `cateyes`, `cateyes-ls-devices`, `cateyes-ps`,
`cateyes-kill`, `cateyes-trace`, and `cateyes-discover`, you need Python plus a
few packages:

    pip3 install colorama prompt-toolkit pygments

### Linux

    make

### macOS and iOS

First make a trusted code-signing certificate. You can use the guide at
https://sourceware.org/gdb/wiki/BuildingOnDarwin in the section
“Creating a certificate”. You can use the name `cateyes-cert` instead of
`gdb-cert` if you'd like.

Next export the name of the created certificate to the environment
variables `MAC_CERTID` and `IOS_CERTID` and run `make`:

    export MAC_CERTID=cateyes-cert
    export IOS_CERTID=cateyes-cert
    make

To ensure that macOS accepts the newly created certificate, restart the
`taskgated` daemon:

    sudo killall taskgated

### Windows

    cateyes.sln

(Requires Visual Studio 2017.)

See [https://www.cateyes.re/docs/building/](https://www.cateyes.re/docs/building/)
for details.

## Learn more

Have a look at our [documentation](https://www.cateyes.re/docs/home/).
