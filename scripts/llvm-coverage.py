#! /usr/bin/env python3
#------------------------------------------------------------------------------

import os
import sys
import argparse
import glob
import subprocess
#------------------------------------------------------------------------------

def runCmd(cmd, data=None):
    if input is None:
        p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
    else:
        p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate(input=data)

    return stdout.decode('utf-8'), stderr.decode('utf-8'), p.returncode
#------------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description='llvm-coverage')
    parser.add_argument('--insights',       help='C++ Insights binary',      required=True)
    parser.add_argument('--llvm-prof-dir',  help='LLVM profiles data dir',   default='')
    parser.add_argument('--format',         help='Output format: html/text', default='text')
    parser.add_argument('--output',         help='Output filename',          required=True)
    parser.add_argument('args', nargs=argparse.REMAINDER)
    args = vars(parser.parse_args())

    insightsPath     = args['insights']
    rawProfiles      = glob.glob(os.path.join(args['llvm_prof_dir'], "*.profraw"))
    profilesManifest = os.path.join(args['llvm_prof_dir'], 'profiles.manifest')
    profilesData     = os.path.join(args['llvm_prof_dir'], 'insights.profdata')

    with open(profilesManifest, "w") as manifest:
        manifest.write("\n".join(rawProfiles))

    cmd = ['llvm-profdata', 'merge', '-sparse', '-f', profilesManifest, '-o', profilesData]
    stdout, stderr, returncode = runCmd(cmd)
    print(stderr)

    cmd = ['llvm-cov', 'show', insightsPath, f'-instr-profile={profilesData}', f'--format={args["format"]}', '-ignore-filename-regex=build/']
    stdout, stderr, returncode = runCmd(cmd)
    print(stderr)

    open(args['output'], 'w').write(stdout)
#------------------------------------------------------------------------------


sys.exit(main())
#------------------------------------------------------------------------------


