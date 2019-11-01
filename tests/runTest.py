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
    cmd = [args['cxx'], cppStd, '-m64', '-D__cxa_guard_acquire(x)=true', '-D__cxa_guard_release(x)', '-D__cxa_guard_abort(x)']

    # GCC seems to dislike empty ''
    if '-std=c++98' == cppStd:
        cmd += ['-Dalignas(x)=']

    cmd += ['-c', tmpFileName]

    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    compileErrorFile = os.path.join(mypath, fileName + '.cerr')
    if 0 != p.returncode:
        if os.path.isfile(compileErrorFile):
            ce = open(compileErrorFile, 'r').read()
            stderr = stderr.replace(tmpFileName, '.tmp.cpp')
            # Replace paths, as for example, the STL path differs from a local build to Travis-CI at least for macOS
            stderr = re.sub('/(.*)/(.*?:[0-9]+):', '... \\2:', stderr)

            if ce == stderr:
                print '[PASSED] Compile: %s' %(f)
                return True, None

        compileErrorFile = os.path.join(mypath, fileName + '.ccerr')
        if os.path.isfile(compileErrorFile):
                ce = open(compileErrorFile, 'r').read()
                stderr = stderr.replace(tmpFileName, '.tmp.cpp')

                if ce == stderr:
                    print '[PASSED] Compile: %s' %(f)
                    return True, None

        print '[ERROR] Compile failed: %s' %(f)
        print stderr
    else:
        if os.path.isfile(compileErrorFile):
            print 'unused file: %s' %(compileErrorFile)

        objFileName = '%s.o' %(os.path.splitext(os.path.basename(tmpFileName))[0])
        os.remove(objFileName)

        print '[PASSED] Compile: %s' %(f)
        return True, None

    return False, stderr
#------------------------------------------------------------------------------


def main():
    parser = argparse.ArgumentParser(description='Description of your program')
    parser.add_argument('--insights',       help='C++ Insights binary',  required=True)
    parser.add_argument('--cxx',            help='C++ compiler to used', default='/usr/local/clang-current/bin/clang++')
    parser.add_argument('--failure-is-ok',  help='Failing tests are ok', default=False, action='store_true')
    parser.add_argument('--update-tests',   help='Update failing tests', default=False, action='store_true')
    parser.add_argument('--std',            help='C++ Standard to used', default='c++17')
    parser.add_argument('--use-libcpp',     help='Use libst++',          default=False, action='store_true')
    parser.add_argument('args', nargs=argparse.REMAINDER)
    args = vars(parser.parse_args())

    insightsPath  = args['insights']
    remainingArgs = args['args']
    bFailureIsOk  = args['failure_is_ok']
    bUpdateTests  = args['update_tests']
    defaultCppStd = '-std=%s'% (args['std'])

    if 0 == len(remainingArgs):
        cppFiles = [f for f in os.listdir(mypath) if (os.path.isfile(os.path.join(mypath, f)) and f.endswith('.cpp'))]
    else:
        cppFiles = remainingArgs

    filesPassed     = 0
    missingExpected = 0
    ret             = 0

    regEx         = re.compile('.*cmdline:(.*)')
    regExInsights = re.compile('.*cmdlineinsights:(.*)')

    for f in sorted(cppFiles):
        fileName     = os.path.splitext(f)[0]
        expectFile   = os.path.join(mypath, fileName + '.expect')
        ignoreFile   = os.path.join(mypath, fileName + '.ignore')
        cppStd       = defaultCppStd
        insightsOpts = ''

        fileHeader = open(f, 'r').readline().strip()
        m = regEx.match(fileHeader)
        if None != m:
            cppStd = m.group(1)

        m = regExInsights.match(fileHeader)
        if None != m:
            insightsOpts = m.group(1)

        if not os.path.isfile(expectFile) and not os.path.isfile(ignoreFile):
            print 'Missing expect/ignore for: %s' %(f)
            missingExpected += 1
            continue

        cmd = [insightsPath, f]

        if args['use_libcpp']:
            cmd.append('-use-libc++')


        if '' != insightsOpts:
            cmd.append(insightsOpts)

        cmd.extend(['--', cppStd, '-m64'])

        begin = datetime.datetime.now()
        p   = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        end   = datetime.datetime.now()
        stdout, stderr = p.communicate()

        if 0 != p.returncode:
            compileErrorFile = os.path.join(mypath, fileName + '.cerr')
            if os.path.isfile(compileErrorFile):
                ce = open(compileErrorFile, 'r').read()

                # Linker errors name the tmp file and not the .tmp.cpp, replace the name here to be able to suppress
                # these errors.
                ce = re.sub('(.*).cpp:', '.tmp:', ce)
                ce = re.sub('(.*).cpp.', '.tmp:', ce)
                stderr = re.sub('(.*).cpp:', '.tmp:', stderr)
                stderr = re.sub('(Error while processing.*.cpp.)', '', stderr)
                # Replace paths, as for example, the STL path differs from a local build to Travis-CI at least for macOS
                stderr = re.sub('/(.*)/(.*?:[0-9]+):', '... \\2:', stderr)

                # The cerr output matches and the return code says that we hit a compile error, accept it as passed
                if (ce == stderr) and (1 == p.returncode):
                    print '[PASSED] Compile: %s' %(f)
                    filesPassed += 1
                    continue
                else:
                    print '[ERROR] Compile: %s' %(f)
                    ret = 1


            print 'Insight crashed for: %s with: %d' %(f, p.returncode)
            print stderr

            if not bUpdateTests:
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
            bCompiles, stderr = testCompile(tmpFileName, f, args, fileName, cppStd)
            compileErrorFile = os.path.join(mypath, fileName + '.cerr')


            if bCompiles and equal:
                filesPassed += 1
            elif bUpdateTests:
                if bCompiles and not equal:
                    open(expectFile, 'w').write(stdout)
                    print('Updating test')
                elif not bCompiles and os.path.exists(compileErrorFile):
                    open(expectFile, 'w').write(stdout)
                    open(compileErrorFile, 'w').write(stderr)
                    print('Updating test cerr')


        finally:
            os.remove(tmpFileName)



    expectedToPass = len(cppFiles)-missingExpected
    print '-----------------------------------------------------------------'
    print 'Tests passed: %d/%d' %(filesPassed, expectedToPass)

    if bFailureIsOk:
        return 0

    if missingExpected:
        print('Missing expected files: %d' %(missingExpected))

    passed = (0 == missingExpected) and (expectedToPass == filesPassed)

    return (False == passed)  # note bash expects 0 for ok
#------------------------------------------------------------------------------


sys.exit(main())
#------------------------------------------------------------------------------

