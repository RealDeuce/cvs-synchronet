#!/bin/sh

git remote add -f $1 ../cvs/$1
git merge -s ours $1/master
git read-tree --prefix=$1 -u $1/master
git commit -m "Merge cvs-synchronet-$1 as $1"

