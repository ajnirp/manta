#!/usr/bin/python
#
# Simple script to run all base tests
# The test scripts are named test_XXXX_description.py
# They are assumed to be in ascending order or complexity.
#
# Rough ordering:
# 0xxx tests are very basic (mostly single operator calls)
# 1xxx is for 2d sims
# 2xxx for "real" 3d sims
# 

import os
import shutil
import sys
import re
from subprocess import check_output

# note - todo, right now this script assumes the test_X.py files are
# in the current working directory... this should be changed at some 
# point.

# debugging, print outputs from all manta calls
printAllOutpus = 1
filePrefix = "test_"

if(len(sys.argv)<2):
	print "Usage runTests.py <manta-executable>"
	exit(1)

manta = sys.argv[1]
print "Using mantaflow executable '" + manta + "' " 

files = os.popen("ls "+str(filePrefix)+"????_*.py").read() 
#print "Debug, found test files: "+files


genRefFiles = int(os.getenv('MANTA_GEN_TEST_DATA', 0))
if (genRefFiles>0):
	print "\nNote - generating test data for all tests!"
	print "Tests results will not be evaluated...\n"

num = 0
numOks = 0
numFail = 0

files = files.split('\n')
for file in files:
	if ( len(file) < 1):
		continue
	num += 1
	print "Running '" + file + "' "
	result = os.popen(manta + " " + file + " 2>&1 ").read() 

	oks = re.findall(r"\nOK!", result)
	#print oks
	numOks += len(oks)

	fails = re.findall(r"\nFAIL!", result)
	# also check for "Errors"
	if (len(fails)==0) :
		# note - if there are errors, the overall count of tests won't be valid anymore... 
		fails = re.findall(r"Error", result) 
	#print fails
	numFail += len(fails)

	if (len(fails)>0) or (printAllOutpus==1):
		print
		print "Full output: " + result
		print


if (genRefFiles==1):
	print "Test data generated"
	exit(0)

print
print "Test summary, " +str(num) + " runs, " + str(numOks) + " passed, " + str(numFail) + " failed."

if (numFail==0) and (numOks==0):
	print "Failure! Perhaps manta executable didnt work, or runTests not called in test directory \n"
	exit(1)
elif (numFail==0) and (numOks>0):
	print "All good :) \n"
	exit(0)
else:
	print "Some tests failed :( \n"
	exit(2)


