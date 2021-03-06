#!/usr/bin/env zsh

:<<'_'
 The MIT License (MIT)

 Copyright (c) 2015 Alexander Droste

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
_

# script to setup mpi-checker

# check for internet connectivity
ping -c 1 www.google.com
if [[ $? -eq 0 ]]; then
    echo "internet connectivity established"

    echo "––––––––––MPI-Checker––––––––––––"
    #download –––––––––––––––––––––––––––––––––––––––––––––––––––––––
    # clone mpi-checker project
    cd tools/clang/lib/StaticAnalyzer/Checkers
    git clone https://github.com/0ax1/MPI-Checker.git

    #config ––––––––––––––––––––––––––––––––––––––––––––––––––––––––
    # pipe checker registration
    cat MPI-Checker/setup/checkerTd.txt >> Checkers.td

    # use gsed if available (osx, homebrew, gnu-sed)
    sed=sed
    hash gsed 2>/dev/null
    if [[ $? -eq 0 ]]; then
        sed=gsed
    fi

    # add sources to cmake
    lineNo=$(grep -nr add_clang_library CMakeLists.txt | sed -E 's/[^0-9]*//g')

    $sed -i "${lineNo}i FILE (GLOB MPI-CHECKER MPI-Checker/src/*.cpp)" \
        CMakeLists.txt
    ((lineNo += 2))
    $sed -i "${lineNo}i \ \ \${MPI-CHECKER}" CMakeLists.txt

    # symlink test source
    abspath() {
        [[ $1 = /* ]] && echo "$1" || echo "$PWD/${1#./}"
    }
    ln -s `abspath MPI-Checker/tests/MPICheckerTest.c` \
        `abspath ../../../test/Analysis/MPICheckerTest.c`

else
    # echo as error (pipe stdout to stderr)
    echo "no internet connectivity" 1>&2
fi
