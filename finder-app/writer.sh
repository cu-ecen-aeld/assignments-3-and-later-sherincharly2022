#! /bin/bash

if [ ! 2 -eq $# ]
then
	exit 1
fi

writefile=$1
writestr=$2

if [ ! -e $writefile ]
then
	mkdir -p "$(dirname $writefile)" && touch $writefile
fi

if [ ! 0 -eq $? ]
then
	exit 1
fi

echo $writestr > $writefile

if [ ! 0 -eq "$?" ]
then
	exit 1
fi
 

