#! /bin/bash

if [ ! 2 -eq "$#" ] 
then
	# echo "argument invalid"
	exit 1
fi

filesdir=$1
searchstr=$2

# echo $filesdir $searchstr
 

if [ ! -d "$filesdir" ]
then
	# echo "not same"
	exit 1
fi

echo "The number of files are $(find $filesdir -type f | wc -l) and the number of matching lines are $(grep -r $searchstr $filesdir | wc -l)"

