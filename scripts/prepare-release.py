#! /usr/bin/env python3
#
#
# C++ Insights, copyright (C) by Andreas Fertig
# Distributed under an MIT license. See LICENSE for details
#
#------------------------------------------------------------------------------

import re
import os
import subprocess
#------------------------------------------------------------------------------

def main():
    versionH = open('version.h.in', 'r').read()

    oldClangStable = '12'
    newClangStable = '13'
    newInsightsVersion = '0.8'
    oldInsightsVersion = re.search('INSIGHTS_VERSION\s+"(.*?)"', versionH, re.DOTALL | re.MULTILINE).group(1)


    print('Preparing a new release:')
    print(' Current Clang stable      : %s' %(oldClangStable))
    print(' New Clang stable          : %s' %(newClangStable))
    print(' Current Insights version  : %s' %(oldInsightsVersion))
    print(' New Insights version      : %s' %(newInsightsVersion))

    print('  - Updating .github/workflows/ci.yml')
    travis = open('.github/workflows/ci.yml', 'r').read()

    regEx = re.compile('[clang|llvm]-([0-9]+)')

    travis = re.sub('(clang|llvm|clang\+\+|llvm-config|llvm-toolchain-bionic|clang-format|clang-tidy|llvm-toolchain-trusty)(-%s)' %(oldClangStable), '\\1-%s' %(newClangStable) , travis)
    travis = re.sub('(clang|Clang|llvm|LLVM) (%s)' %(oldClangStable), '\\1 %s' %(newClangStable) , travis)
    travis = re.sub(r"(LLVM_VERSION=)('%s)" %(oldClangStable), r"\1'%s" %(newClangStable) , travis)
    travis = re.sub(r"(LLVM_VERSION:)\s*(%s.0.0)" %(oldClangStable), r"\1 %s.0.0" %(newClangStable) , travis)
    travis = re.sub(r'(llvm_version:\s*)("%s)(.0.0",)' %(oldClangStable), '\\1"%s.0.0",' %(newClangStable), travis)
    travis = re.sub(r"clang(%s)0" %(oldClangStable), r"clang%s0" %(newClangStable) , travis)
    travis = re.sub(r"(llvm-toolchain-xenial)-(%s)" %(oldClangStable), r"\1-%s" %(newClangStable) , travis)
    travis = re.sub(r"(./llvm.sh) (%s)" %(oldClangStable), r"\1 %s" %(newClangStable) , travis)

#    print(travis)
    open('.github/workflows/ci.yml', 'w').write(travis)


    print('  - Updating CMakeLists.txt')
    cmake = open('CMakeLists.txt', 'r').read()
    cmake = re.sub('(set\(INSIGHTS_MIN_LLVM_MAJOR_VERSION)( %s)\)' %(oldClangStable), '\\1 %s)' %(newClangStable) , cmake)
    open('CMakeLists.txt', 'w').write(cmake)


    print('  - Updating version.h %s -> %s' %(oldInsightsVersion, newInsightsVersion))
    version = open('version.h.in', 'r').read()
    version = re.sub('(INSIGHTS_VERSION )"(.*)"', '\\1"%s"' %(newInsightsVersion) , version)
    open('version.h.in', 'w').write(version)


    print('  - Generating CHANGELOG.md')
    os.system('gren changelog --override --username=andreasfertig --repo=cppinsights %s...continous' %(oldInsightsVersion))

    cmd = ['gren', 'changelog', '--override', '--username=andreasfertig', '--repo=cppinsights', '%s...continous' %(oldInsightsVersion)]
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    if 0 != p.returncode:
        print('ERR: gren failed')
        print(stderr)
        return 1

    changeLog = open('CHANGELOG.md', 'r').read()
    changeLog = re.sub('(## Continuous build.*?---)\n', '', changeLog, flags=re.DOTALL)
    open('CHANGELOG.md', 'w').write(changeLog)

    gitTag = 'v_%s' %(oldInsightsVersion)
    print('  - Tagging %s' %(gitTag))

    #cmd = ['git', 'tag', '-a', '-m', gitTag, gitTag, 'main']
    cmd = ['git', 'tag', gitTag, 'main']
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdout, stderr = p.communicate()

    if 0 != p.returncode:
        print('ERR: git tag failed!')
        print(stderr)
        return 1

    cppInsightsDockerBaseFile = '../cppinsights-docker-base/Dockerfile'

    print('  - Updating cppinsights-docker-base (%s)' %(cppInsightsDockerBaseFile))

    dockerFile = open(cppInsightsDockerBaseFile, 'r').read()
    dockerFile = re.sub('(ENV\s+CLANG_VERSION=)([0-9]+)', r'\g<1>%s' %(newClangStable), dockerFile)
    open(cppInsightsDockerBaseFile, 'w').write(dockerFile)
#------------------------------------------------------------------------------

main()
#------------------------------------------------------------------------------

