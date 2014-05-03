#! /bin/sh

# UCLA CS 111 Lab 1 - Test that valid commands are processed correctly.


#testing exec
exec echo goodbye world 
exec ls -a > ls.txt
exec cat ls.txt > lscopy.txt
cat < a.txt > c.txt

ls -a > ls.txt
echo start of this script && echo Line 1 good || echo Line 1 failed
sort < testerFile > a.txt && echo Line 2 good || echo Line 2 failed
cat a.txt > b.txt
cmp a.txt b.txt || echo false

#testing for subcommands
echo line in a.txt > a.txt
echo line in b.txt > b.txt
echo so far so good
(cat a.txt) > b.txt
cmp a.txt b.txt || echo false
echo LINE IN B.TXT > b.txt
(cat > a.txt) < b.txt
cmp a.txt b.txt && echo true > true.txt
echo LINE IN A.TXT > a.txt


(cat a.txt) < b.txt > c.txt # should output A.TXT to c.txt here
cmp a.txt c.txt && echo true

(cat) < b.txt > a.txt  # shoud output B.TXT to a.txt here 
cmp b.txt a.txt && echo All tests were successful > d.txt
echo Hello World

echo Should be dependent on Node 1 and 4 > c.txt

echo first

echo second

echo third

echo fourth: testing order of execution for non-dependent nodes

echo fifth
