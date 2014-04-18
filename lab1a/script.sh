#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid commands are processed correctly.

echo start of this script && echo Line 1 good || echo Line 1 failed
sort < testerFile > a.txt && echo Line 2 good || echo Line 2 failed
echo hello world
cat a.txt > b.txt
cmp a.txt b.txt || echo false
