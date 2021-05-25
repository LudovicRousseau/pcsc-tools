while read LINE
do
	grep -A 1 "$LINE" smartcard_list.txt
done < a
