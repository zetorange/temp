import sys
sys.path.insert(0, "/Users/oleavr/src/cateyes/build/cateyes-macos-universal/lib/python2.7/site-packages")
import AppDelegate
import Capture
import MainWindowController
import ProcessList

if __name__ == "__main__":
    from PyObjCTools import AppHelper
    AppHelper.runEventLoop()
