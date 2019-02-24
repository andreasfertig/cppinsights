#! /usr/bin/python
#------------------------------------------------------------------------------

import os
import sys
import subprocess
import re
import argparse
import tempfile
import datetime
#------------------------------------------------------------------------------

mypath = '.'


def testCompare(tmpFileName, stdout, expectFile, f, args, time):
    expect = open(expectFile, 'r').read()

    if args['docker']:
        expect = re.sub( r'instantiated from: .*?.cpp:', r'instantiated from: x.cpp:', expect)

    if stdout != expect:
        print '[FAILED] %s - %s' %(f, time)
        cmd = ['/usr/bin/diff', expectFile, tmpFileName]
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()

        print stdout
    else:
        print '[PASSED] %s - %s' %(f, time)
        return True

    return False
#------------------------------------------------------------------------------

def testCompile(tmpFileName, f, args, fileName, cppStd):
    cmd = [args['cxx'], cppStd, '-m64', '-c', tmpFileName]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    compileErrorFile = os.path.join(mypath, fileName + '.cerr')
    if 0 != p.returncode:
        if os.path.isfile(compileErrorFile):
            ce = open(compileErrorFile, 'r').read()
            stderr = stderr.replace(tmpFileName, '.tmp.cpp')

            if ce == stderr:
                print '[PASSED] Compile: %s' %(f)
                return True

        compileErrorFile = os.path.join(mypath, fileName + '.ccerr')
        if os.path.isfile(compileErrorFile):
                ce = open(compileErrorFile, 'r').read()
                stderr = stderr.replace(tmpFileName, '.tmp.cpp')

                if ce == stderr:
                    print '[PASSED] Compile: %s' %(f)
                    return True

        print '[ERROR] Compile failed: %s' %(f)
        print stderr
        ret = 1
    else:
        if os.path.isfile(compileErrorFile):
            print 'unused file: %s' %(compileErrorFile)

        objFileName = '%s.o' %(os.path.splitext(os.path.basename(tmpFileName))[0])
        os.remove(objFileName)

        print '[PASSED] Compile: %s' %(f)
        return True

    return False
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
    parser.add_argument('--docker',         help='Run tests in docker container', action='store_true')
    parser.add_argument('--docker-image',   help='Docker image name', default='cppinsights-runtime')
    parser.add_argument('--failure-is-ok',  help='Failing tests are ok', default=False, action='store_true')
    parser.add_argument('--std',            help='C++ Standard to used', default='c++1z')
    parser.add_argument('args', nargs=argparse.REMAINDER)
    args = vars(parser.parse_args())

    insightsPath  = args['insights']
    remainingArgs = args['args']
    bFailureIsOk  = args['failure_is_ok']
    defaultCppStd = '-std=%s'% (args['std'])

    if 0 == len(remainingArgs):
        cppFiles = [f for f in os.listdir(mypath) if (os.path.isfile(os.path.join(mypath, f)) and f.endswith('.cpp'))]
    else:
        cppFiles = remainingArgs

    if args['docker']:
        print 'Running tests in docker'

    filesPassed     = 0
    missingExpected = 0
    ret             = 0

    defaultIncludeDirs = getDefaultIncludeDirs(args['cxx'])

    regEx = re.compile('.*cmdline:(.*)')
    for f in sorted(cppFiles):
        fileName   = os.path.splitext(f)[0]
        expectFile = os.path.join(mypath, fileName + '.expect')
        cppStd     = defaultCppStd

        fileHeader = open(f, 'r').readline().strip()
        m = regEx.match(fileHeader)
        if None != m:
            cppStd = m.group(1)

        if not os.path.isfile(expectFile):
            print 'Missing expect for: %s' %(f)
            missingExpected += 1
            continue

        if args['docker']:
                data = open(f, 'r').read()
                cmd = ['docker', 'run', '-i', args['docker_image'], insightsPath, '-stdin', 'x.cpp', '--', '-std=c++1z', '-isystem/usr/include/c++/v1/']
                p   = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                stdout, stderr = p.communicate(input=data)
        else:
                cmd   = [insightsPath, f, '--', cppStd, '-m64'] + defaultIncludeDirs
                begin = datetime.datetime.now()
                p     = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
                end   = datetime.datetime.now()
                stdout, stderr = p.communicate()

        if 0 != p.returncode:
            compileErrorFile = os.path.join(mypath, fileName + '.cerr')
            if os.path.isfile(compileErrorFile):
                ce = open(compileErrorFile, 'r').read()

                # Linker errors name the tmp file and not the .tmp.cpp, replace the name here to be able to suppress
                # these errors.
                ce = re.sub('(.*).cpp:', '.tmp:', ce)

                if ce == stderr:
                    print '[PASSED] Compile: %s' %(f)
                    filesPassed += 1
                    continue
                else:
                    print '[ERROR] Compile: %s' %(f)
                    ret = 1


            print 'Insight crashed for: %s with: %d' %(f, p.returncode)
            print stderr

            continue

        fd, tmpFileName = tempfile.mkstemp('.cpp')
        try:
            with os.fdopen(fd, 'w') as tmp:
                # stupid replacements for clang 6.0. With 7.0 they added a 1.
                stdout = stdout.replace('__range ', '__range1 ')
                stdout = stdout.replace('__range.', '__range1.')
                stdout = stdout.replace('__range)', '__range1)')
                stdout = stdout.replace('__range;', '__range1;')
                stdout = stdout.replace('__begin ', '__begin1 ')
                stdout = stdout.replace('__begin.', '__begin1.')
                stdout = stdout.replace('__begin,', '__begin1,')
                stdout = stdout.replace('__begin;', '__begin1;')
                stdout = stdout.replace('__end ', '__end1 ')
                stdout = stdout.replace('__end.', '__end1.')
                stdout = stdout.replace('__end;', '__end1;')
                stdout = stdout.replace('__end)', '__end1)')

                # write the data to the temp file
                tmp.write(stdout)

            equal = testCompare(tmpFileName, stdout, expectFile, f, args, end-begin)

            if testCompile(tmpFileName, f, args, fileName, cppStd) and equal:
                filesPassed += 1

        finally:
            os.remove(tmpFileName)



    expectedToPass = len(cppFiles)-missingExpected
    print '-----------------------------------------------------------------'
    print 'Tests passed: %d/%d' %(filesPassed, expectedToPass)

    if bFailureIsOk:
        return 0

    return expectedToPass != filesPassed # note bash expects 0 for ok
#------------------------------------------------------------------------------


sys.exit(main())
#------------------------------------------------------------------------------

