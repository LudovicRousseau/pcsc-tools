#/bin/sh -e

# $Id: create_distrib.sh,v 1.1 2001-10-16 07:33:22 rousseau Exp $

# $Log: not supported by cvs2svn $

dir=$(basename $(pwd))
if [ -d $dir ]
then
	rm -r $dir
fi
mkdir $dir

for i in $(cat MANIFEST)
do
	if [ $(echo $i | grep /) ]
	then
		idir=$dir/${i%/*}
		if [ ! -d $idir ]
		then
			echo "mkdir $idir"
			mkdir $idir
		fi
	fi
	echo "cp $i $dir/$i"
	cp -a $i $dir/$i
done

rcs2log > $dir/Changelog

tar czvf ../$dir.tar.gz $dir
rm -r $dir

