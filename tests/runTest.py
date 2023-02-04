#! /usr/bin/env python3
#------------------------------------------------------------------------------

import os
import sys
import subprocess
import re
import argparse
import tempfile
import datetime
import difflib
#------------------------------------------------------------------------------

mypath = '.'

def runCmd(cmd, data=None):
    if None == input:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
    else:
        p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate(input=data)

    return stdout.decode('utf-8'), stderr.decode('utf-8'), p.returncode
#------------------------------------------------------------------------------

def cleanStderr(stderr, fileName=None):
    if None != fileName:
        stderr = stderr.replace(fileName, '.tmp.cpp')
    else:
        stderr = re.sub('(.*).cpp:', '.tmp:', stderr)
#    stderr = re.sub('[\n ](.*).cpp:', '\\1%s:' %(fileName), stderr)

    # Replace paths, as for example, the STL path differs from a local build to Travis-CI at least for macOS
    stderr = re.sub('/(.*)/(.*?:[0-9]+):', '... \\2:', stderr)
    stderr = re.sub('RecoveryExpr 0x[a-f0-9]+ ', 'RecoveryExpr ', stderr)

    return stderr
#------------------------------------------------------------------------------

def testCompare(tmpFileName, stdout, expectFile, f, args, time):
    expect = open(expectFile, 'r', encoding='utf-8').read()

    # align line-endings under Windows to Unix-style
    if os.name == 'nt':
        stdout = stdout.replace('\r\n', '\n')

    if stdout != expect:
        print('[FAILED] %s - %s' %(f, time))

        for line in difflib.unified_diff(expect.splitlines(keepends=True), stdout.splitlines(keepends=True), fromfile=expectFile, tofile='stdout', n=3):
            print('%s' %((line[1:] if line.startswith(' ') else line) ), end='')
    else:
        print('[PASSED] %-50s - %s' %(f, time))
        return True

    return False
#------------------------------------------------------------------------------

def testCompile(tmpFileName, f, args, fileName, cppStd):
    if os.name == 'nt':
        cppStd = cppStd.replace('-std=', '/std:')
        cppStd = cppStd.replace('2a', 'latest')

    cmd = [args['cxx'], cppStd, '-D__cxa_guard_acquire(x)=true', '-D__cxa_guard_release(x)', '-D__cxa_guard_abort(x)']

    if os.name != 'nt':
        cmd.append('-m64')
    else:
        cmd.extend(['/nologo', '/EHsc', '/IGNORE:C4335']) # C4335: mac file format detected. EHsc assume only C++ functions throw exceptions.

    # GCC seems to dislike empty ''
    if '-std=c++98' == cppStd:
        cmd += ['-Dalignas(x)=']

    cmd += ['-c', tmpFileName]

    stdout, stderr, returncode = runCmd(cmd)

    compileErrorFile = os.path.join(mypath, fileName + '.cerr')
    if 0 != returncode:
        if os.path.isfile(compileErrorFile):
            ce = open(compileErrorFile, 'r', encoding='utf-8').read()
            stderr = cleanStderr(stderr, tmpFileName)

            if ce == stderr:
                print('[PASSED] Compile: %s' %(f))
                return True, None

        compileErrorFile = os.path.join(mypath, fileName + '.ccerr')
        if os.path.isfile(compileErrorFile):
                ce = open(compileErrorFile, 'r', encoding='utf-8').read()
                stderr = stderr.replace(tmpFileName, '.tmp.cpp')

                if ce == stderr:
                    print('[PASSED] Compile: %s' %(f))
                    return True, None

        print('[ERROR] Compile failed: %s' %(f))
        print(stderr)
    else:
        if os.path.isfile(compileErrorFile):
            print('unused file: %s' %(compileErrorFile))

        ext = 'obj' if os.name == 'nt' else 'o'

        objFileName = '%s.%s' %(os.path.splitext(os.path.basename(tmpFileName))[0], ext)
        os.remove(objFileName)

        print('[PASSED] Compile: %s' %(f))
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
    crashes         = 0
    ret             = 0

    regEx         = re.compile('.*cmdline:(.*)')
    regExInsights = re.compile('.*cmdlineinsights:(.*)')

    for f in sorted(cppFiles):
        fileName     = os.path.splitext(f)[0]
        expectFile   = os.path.join(mypath, fileName + '.expect')
        ignoreFile   = os.path.join(mypath, fileName + '.ignore')
        cppStd       = defaultCppStd
        insightsOpts = ''

        fh = open(f, 'r', encoding='utf-8')
        fileHeader = fh.readline()
        fileHeader += fh.readline()
        m = regEx.search(fileHeader)
        if None != m:
            cppStd = m.group(1)

        m = regExInsights.search(fileHeader)
        if None != m:
            insightsOpts = m.group(1)

        if not os.path.isfile(expectFile) and not os.path.isfile(ignoreFile):
            print('Missing expect/ignore for: %s' %(f))
            missingExpected += 1
            continue

        if os.path.isfile(ignoreFile):
            print('Ignoring: %s' %(f))
            filesPassed += 1
            continue

        cmd = [insightsPath, f]

        if args['use_libcpp']:
            cmd.append('-use-libc++')


        if '' != insightsOpts:
            cmd.append(insightsOpts)

        cmd.extend(['--', cppStd, '-m64'])

        begin = datetime.datetime.now()
        stdout, stderr, returncode = runCmd(cmd)
        end   = datetime.datetime.now()

        if 0 != returncode:
            compileErrorFile = os.path.join(mypath, fileName + '.cerr')
            if os.path.isfile(compileErrorFile):
                ce = open(compileErrorFile, 'r', encoding='utf-8').read()

                # Linker errors name the tmp file and not the .tmp.cpp, replace the name here to be able to suppress
                # these errors.
                ce = re.sub('(.*).cpp:', '.tmp:', ce)
                ce = re.sub('(.*).cpp.', '.tmp:', ce)
                stderr = re.sub('(Error while processing.*.cpp.)', '', stderr)
                stderr = cleanStderr(stderr)

                # The cerr output matches and the return code says that we hit a compile error, accept it as passed
                if (ce == stderr) and (1 == returncode):
                    print('[PASSED] Compile: %s' %(f))
                    filesPassed += 1
                    continue
                else:
                    print('[ERROR] Compile: %s' %(f))
                    ret = 1


            crashes += 1
            print('Insight crashed for: %s with: %d' %(f, returncode))
            print(stderr)

            if not bUpdateTests:
                continue

        fd, tmpFileName = tempfile.mkstemp('.cpp')
        try:
            with os.fdopen(fd, 'w', encoding='utf-8') as tmp:
                # write the data to the temp file
                tmp.write(stdout)

            equal = testCompare(tmpFileName, stdout, expectFile, f, args, end-begin)
            bCompiles, stderr = testCompile(tmpFileName, f, args, fileName, cppStd)
            compileErrorFile = os.path.join(mypath, fileName + '.cerr')


            if bCompiles and equal:
                filesPassed += 1
            elif bUpdateTests:
                if bCompiles and not equal:
                    open(expectFile, 'w', encoding='utf-8').write(stdout)
                    print('Updating test')
                elif not bCompiles and os.path.exists(compileErrorFile):
                    open(expectFile, 'w', encoding='utf-8').write(stdout)
                    open(compileErrorFile, 'w', encoding='utf-8').write(stderr)
                    print('Updating test cerr')


        finally:
            os.remove(tmpFileName)



    expectedToPass = len(cppFiles)-missingExpected
    print('-----------------------------------------------------------------')
    print('Tests passed: %d/%d' %(filesPassed, expectedToPass))

    if bFailureIsOk:
        return 0

    print('Insights crashed: %d' %(crashes))
    print('Missing expected files: %d' %(missingExpected))

    passed = (0 == missingExpected) and (expectedToPass == filesPassed)

    return (False == passed)  # note bash expects 0 for ok
#------------------------------------------------------------------------------


sys.exit(main())
#------------------------------------------------------------------------------

