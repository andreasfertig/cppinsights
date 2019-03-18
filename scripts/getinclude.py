#! /usr/bin/env python
#
#
# C++ Insights, copyright (C) by Andreas Fertig
# Distributed under an MIT license. See LICENSE for details
#
#------------------------------------------------------------------------------

import os
import sys
import subprocess
import re

def main():
    cxx = 'g++'
    if 2 == len(sys.argv):
        cxx = sys.argv[1]

    cmd = [cxx, '-E', '-x', 'c++', '-v', '/dev/null']
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    m = re.findall(b'\n (/.*)', stderr)

    includes = ''

    for x in m:
        if -1 != x.find(b'(framework directory)'):
            continue

        includes += '-isystem%s ' % os.path.normpath(x.decode())

    print(includes)

    return 1
#------------------------------------------------------------------------------

sys.exit(main())
#------------------------------------------------------------------------------
