#! /usr/bin/env python

import difflib
import pprint
pp = pprint.PrettyPrinter(indent=4)

ATRs = list()
for l in file("smartcard_list.txt").readlines():
    if l.endswith(" \n") or l.endswith("\t\n"):
        print "Trailing space in:", l

    if l == "\n":
        continue

    if l.startswith("#") or l.startswith("\t"):
        continue

    # check the ATR is all upper case
    if l.upper() != l:
        print "error:", l

    ATRs.append(l.strip())
#	if l.startswith("\t"):
#		ATRs.append([atr, l.strip()])
#	else:
#		atr = l.strip()

#pp.pprint(ATRs)
sorted_ATRs = list(ATRs)
sorted_ATRs.sort()

#pp.pprint(sorted_ATRs)

for l in difflib.context_diff(ATRs, sorted_ATRs):
    print l

# compte le nombre de nouveau ATR
from subprocess import Popen, PIPE

p1 = Popen(["git", "diff"], stdout=PIPE)
p2 = Popen(["grep", "+3[B,F]"], stdin=p1.stdout, stdout=PIPE)
p1.stdout.close()
output = p2.communicate()[0]

size = len(output.split("\n"))-1
if size >= 10:
    print
    print "********************"
    print "    %d new ATRs" % size
    print "********************"
    print
