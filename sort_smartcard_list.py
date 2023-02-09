#! /usr/bin/env python3

import difflib
import pprint
import sys

pp = pprint.PrettyPrinter(indent=4)


def check_format():
    ret_value = False
    ATRs = list()
    with open("smartcard_list.txt", "r") as f:
        for line in f.readlines():
            if line.endswith(" \n") or line.endswith("\t\n"):
                print("Trailing space in:", line)
                ret_value = True

            if line == "\n":
                continue

            if line.startswith("#") or line.startswith("\t"):
                continue

            # check the ATR is all upper case
            if line.upper() != line:
                print("error:", line)
                ret_value = True

            ATRs.append(line.strip())
    # 	if line.startswith("\t"):
    # 		ATRs.append([atr, line.strip()])
    # 	else:
    # 		atr = line.strip()

    # pp.pprint(ATRs)
    sorted_ATRs = list(ATRs)
    sorted_ATRs.sort()
    uniq_ATRs = sorted(set(sorted_ATRs))

    # pp.pprint(sorted_ATRs)
    # pp.pprint(uniq_ATRs)

    # sorted
    for line in difflib.context_diff(ATRs, sorted_ATRs):
        print(line)
        ret_value = True

    # uniq
    for line in difflib.context_diff(sorted_ATRs, uniq_ATRs):
        print(line)
        ret_value = True

    return ret_value


def count_cards():
    # compte le nombre de nouveau ATR
    from subprocess import Popen, PIPE

    p1 = Popen(["git", "diff"], stdout=PIPE)
    p2 = Popen(["grep", "+3[B,F]"], stdin=p1.stdout, stdout=PIPE)
    p1.stdout.close()
    output = p2.communicate()[0]

    output = output.decode("utf-8")

    size = len(output.split("\n")) - 1
    if size >= 10:
        print()
        print("********************")
        print("    %d new ATRs" % size)
        print("********************")
        print()
    else:
        print("only", size, "ATR")


if __name__ == "__main__":
    if check_format():
        sys.exit(1)
    count_cards()
