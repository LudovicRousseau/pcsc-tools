#!/bin/bash

while true
do
	read atr
	atr=$(echo $atr | sed -e "s/ //g")
	echo $atr
	ATR_analysis $atr
done
