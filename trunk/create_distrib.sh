#/bin/sh -e

# $Id: create_distrib.sh,v 1.2 2001-10-17 09:22:11 rousseau Exp $

# $Log: not supported by cvs2svn $
# Revision 1.1  2001/10/16 07:33:22  rousseau
# script to export a clean archive
#

# create a new directory named after the current directory name
# the directory name should be in the form foo-bar.x.y.z
# the use of "_" is not recommanded since it is a problem for Debian

dir=$(basename $(pwd))

echo -e "Using $dir as directory name\n"

rv=$(echo $dir | sed -e 's/.*-[0-9]\.[0-9]\.[0-9]/ok/')
if [ $rv != "ok" ]
then
	echo "ERROR: The directory name should be in the form foo-bar.x.y.z"
	exit
fi

if [ -e $dir ]
then
	echo -e "ERROR: $dir already exists\nremove it and restart"
	exit
fi
mkdir $dir

rcs2log > $dir/Changelog

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

tar czvf ../$dir.tar.gz $dir
rm -r $dir

