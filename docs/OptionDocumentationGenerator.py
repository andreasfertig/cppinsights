#! /usr/bin/env python
#------------------------------------------------------------------------------

import sys
import argparse
import os
import re
import subprocess
import base64
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

def replaceSource(match):
    fileName = match.group(2)

    cpp = '<!-- source:%s.cpp -->\n' %(fileName)
    cpp += '```{.cpp}\n'
    cpp += open('examples/%s.cpp' %(fileName), 'r').read().strip()
    cpp += '\n```\n'
    cpp += '<!-- source-end:%s.cpp -->' %(fileName)

    return cpp
#------------------------------------------------------------------------------

def cppinsightsLink(code, std='2a', options=''):
    # currently 20 is not a thing
    if '20' == std:
        print('Replacing 20 by 2a')
        std = '2a'

    # per default use latest standard
    if '' == std:
        std = '2a'

    std = 'cpp' + std

    if options:
        options += ',' + std
    else:
        options = std

    return('https://cppinsights.io/lnk?code=%s&insightsOptions=%s&rev=1.0' %(base64.b64encode(code).decode('utf-8'), options))
#------------------------------------------------------------------------------

def replaceInsights(match, parser, args):
    cppFileName = match.group(2) + '.cpp'

    insightsPath  = args['insights']
    remainingArgs = args['args']
    defaultCppStd = '-std=%s'% (args['std'])

    defaultIncludeDirs = getDefaultIncludeDirs(args['cxx'])
    cpp = '<!-- transformed:%s -->\n' %(cppFileName)
    cpp += 'Here is the transformed code:\n'
    cpp += '```{.cpp}\n'

    cmd = [insightsPath, 'examples/%s' %(cppFileName), '--', defaultCppStd, '-m64']
    p   = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    cpp += stdout

    cppData = open('examples/%s' %(cppFileName), 'r').read().strip()


    cpp += '\n```\n'
    cpp += '[Live view](%s)\n' %(cppinsightsLink(cppData))
    cpp += '<!-- transformed-end:%s -->' %(cppFileName)

    return cpp
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

        optionName = os.path.splitext(f)[0][4:].strip()

        data = open(f, 'r').read()

        cppFileName = 'cmdl-examples/%s.cpp' %(optionName)

        cpp = open(cppFileName, 'r').read().strip()

        data = data.replace('%s-source' %(optionName), cpp)

        cmd = [insightsPath, cppFileName, '--%s' %(optionName), '--', defaultCppStd, '-m64']
        p   = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()

        data = data.replace('%s-transformed' %(optionName), stdout)

        open(f, 'w').write(data)

    regEx = re.compile('(<!-- source:(.*?).cpp -->(.*?)<!-- source-end:(.*?) -->)', re.DOTALL)
    regExIns = re.compile('(<!-- transformed:(.*?).cpp -->(.*?)<!-- transformed-end:(.*?) -->)', re.DOTALL)

    for f in os.listdir('examples'):
        if not f.endswith('.md'):
            continue

        exampleName = os.path.splitext(f)[0].strip()

        mdFileName = os.path.join('examples', '%s.md' %(exampleName))

        mdData = open(mdFileName, 'r').read()

        mdData = regEx.sub(replaceSource, mdData)

        rpl = lambda match : replaceInsights(match, parser, args)
        mdData = regExIns.sub(rpl, mdData)

        open(mdFileName, 'w').write(mdData)

#------------------------------------------------------------------------------

sys.exit(main())
#------------------------------------------------------------------------------

