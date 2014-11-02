#!/bin/sh

QAKE=../../qake

set -ex

set_up () {
    export RELAY_DEBUG=
}

case_full_build () {
    rm -rf build
    $QAKE 2>&1 | sort > log
    cat full_build.log | sort > full_build.log.sorted
    diff -q log full_build.log.sorted
}

case_null_build () {
    rm -rf build
    $QAKE >/dev/null 2>&1
    $QAKE | sort > log
    cat null_build.log | sort > null_build.log.sorted
    diff -q log null_build.log.sorted
}

case_meaningless_change_build () {
    rm -rf build
    $QAKE >/dev/null 2>&1
    echo // >> src/irc.c
    $QAKE | sort > log
    cat meaningless_change_build.log | sort > meaningless_change_build.log.sorted
    diff -q log meaningless_change_build.log.sorted

    git checkout src/irc.c
}

case_meaningful_change_build () {
    rm -rf build
    $QAKE >/dev/null 2>&1
    echo 'void test(void) { printf ("Test\\n"); }' >> src/irc.c
    $QAKE | sort > log
    cat meaningful_change_build.log | sort > meaningful_change_build.log.sorted
    diff -q log meaningful_change_build.log.sorted

    git checkout src/irc.c
}

case_command_change_build () {
    rm -rf build
    cp Makefile Makefile.tmp
    QAKE_FILE="$QAKE -f Makefile.tmp"
    $QAKE_FILE >/dev/null 2>&1
    sed -i 's|  -rdynamic \\||g' Makefile.tmp
    $QAKE_FILE | sort > log
    cat command_change_build.log | sort > command_change_build.log.sorted
    diff -q log command_change_build.log.sorted
}


set_up
case_full_build
case_null_build
case_meaningless_change_build
case_meaningful_change_build
case_command_change_build
