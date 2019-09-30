#! /usr/bin/env python
#------------------------------------------------------------------------------

import sys
import argparse
import os
import re
import subprocess
#------------------------------------------------------------------------------

def getDefaultIncludeDirs(cxx):
    cmd = [cxx, '-E', '-x', 'c++', '-v', '/dev/null']
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    m = re.findall('\n (/.*)', stderr)

    includes = []

    for x in m:
        if -1 != x.find('(framework directory)'):
            continue

        includes.append('-isystem%s' %(x))

    return includes
#------------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description='Description of your program')
    parser.add_argument('--insights',       help='C++ Insights binary',  required=True)
    parser.add_argument('--cxx',            help='C++ compiler to used', default='/usr/local/clang-current/bin/clang++')
    parser.add_argument('--std',            help='C++ Standard to used', default='c++17')
    parser.add_argument('args', nargs=argparse.REMAINDER)
    args = vars(parser.parse_args())

    insightsPath  = args['insights']
    remainingArgs = args['args']
    defaultCppStd = '-std=%s'% (args['std'])

    print(insightsPath)
    defaultIncludeDirs = getDefaultIncludeDirs(args['cxx'])

    for f in os.listdir('.'):
        if not f.startswith('opt-') or not f.endswith('.md'):
            continue

        optionName = os.path.splitext(f)[0][4:].strip().lower()

        data = open(f, 'r').read()

        cppFileName = 'cmdl-examples/%s.cpp' %(optionName)

        cpp = open(cppFileName, 'r').read().strip()

        data = data.replace('%s-source' %(optionName), cpp)

        cmd = [insightsPath, cppFileName, '--%s' %(optionName), '--', defaultCppStd, '-m64']
        p   = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()

        data = data.replace('%s-transformed' %(optionName), stdout)

        open(f, 'w').write(data)
#------------------------------------------------------------------------------

sys.exit(main())
#------------------------------------------------------------------------------

