#!/bin/sh

subdirs="core proxy util test"

for i in $subdirs; do
	(cd $i && make $1 && cd ..) || exit;
done

