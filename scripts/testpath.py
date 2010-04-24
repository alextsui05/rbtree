#!/usr/bin/python
# IF YOU ARE NOT GETTING THE RESULTS YOU EXPECT WHILE TESTING
# THIS IS THE LIKELY CAUSE
# :: Use distutils to modify the pythonpath for inplace testing
import sys, os
from distutils.util import get_platform


def getTestingPath():
        plat_specifier = ".%s-%s" % (get_platform(), sys.version[0:3])
        build_platlib = os.path.join("build", 'lib' + plat_specifier)
        test_lib = os.path.join(os.path.abspath("."), build_platlib) + '/'
        #assert os.path.exists(test_lib)
        return test_lib

def modTestPath():
        path = getTestingPath()
        sys.path.insert(0, path)
        return path
# END PATH ADJUSTMENT CODE

if __name__ == "__main__":
        print modTestPath()
