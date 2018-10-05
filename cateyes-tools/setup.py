# -*- coding: utf-8 -*-

from setuptools import setup

setup(
    name="cateyes-tools",
    version="1.1.0",
    description="Cateyes CLI tools",
    long_description="CLI tools for [Cateyes](http://www.cateyes.re).",
    long_description_content_type="text/markdown",
    author="Cateyes Developers",
    author_email="oleavr@cateyes.re",
    url="https://www.cateyes.re",
    install_requires=[
        "colorama >= 0.2.7, < 1.0.0",
        "cateyes >= 12.0.0, < 13.0.0",
        "prompt-toolkit >= 0.57, < 2.0.0",
        "pygments >= 2.0.2, < 3.0.0"
    ],
    license="wxWindows Library Licence, Version 3.1",
    zip_safe=True,
    keywords="cateyes debugger dynamic instrumentation inject javascript windows macos linux ios iphone ipad android qnx",
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Environment :: Console",
        "Environment :: MacOS X",
        "Environment :: Win32 (MS Windows)",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved",
        "Natural Language :: English",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: Microsoft :: Windows",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 2",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.4",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: JavaScript",
        "Topic :: Software Development :: Debuggers",
        "Topic :: Software Development :: Libraries :: Python Modules"
    ],
    packages=['cateyes_tools'],
    entry_points={
        'console_scripts': [
            "cateyes = cateyes_tools.repl:main",
            "cateyes-ls-devices = cateyes_tools.lsd:main",
            "cateyes-ps = cateyes_tools.ps:main",
            "cateyes-kill = cateyes_tools.kill:main",
            "cateyes-discover = cateyes_tools.discoverer:main",
            "cateyes-trace = cateyes_tools.tracer:main"
        ]
    }
)
