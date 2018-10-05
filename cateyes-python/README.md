# cateyes-python

Python bindings for [Cateyes](http://www.cateyes.re).

# Some tips during development

To build and test your own egg, do something along the following lines:

```
set CATEYES_VERSION=12.0.0.10.gd7c36fc # from C:\src\cateyes\build\tmp-windows\cateyes-version.h
set CATEYES_EXTENSION=C:\src\cateyes\build\cateyes-windows\Win32-Debug\lib\python2.7\site-packages\_cateyes.pyd
cd C:\src\cateyes\cateyes-python\
python setup.py bdist_egg
pip uninstall cateyes
easy_install dist\cateyes-12.0.0.10.gd7c36fc-py2.7-win32.egg
```
