#!/bin/bash

set -e
set -x

./sort_smartcard_list.py

scp ~/Documents/sc/costa/pcsc-tools/smartcard_list.txt vps2:Serveurs_web/pcsc-tools.apdu.fr
