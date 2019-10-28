#! /bin/bash

set -e

# Temporary fix for image: xcode11 to use GCC.
# Source: https://github.com/Farwaykorse/fwkSudoku/commit/9dff745e37390abcfacd6e4f8da650e7db54444e
# backup the date
ORG_DATE=`date +%m%d%H%M%Y`
sudo date 101918002019
date
sudo installer -pkg /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_10.14.pkg -allowUntrusted -target /
# set the date to the inital value
sudo date ${ORG_DATE}
