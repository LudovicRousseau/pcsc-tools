#! /usr/bin/env python

import difflib
import pprint
pp = pprint.PrettyPrinter(indent=4)

ATRs = list()
for l in file("smartcard_list.txt").readlines():
	if l == "\n" or l.startswith("#") or l.startswith("\t"):
		continue
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
