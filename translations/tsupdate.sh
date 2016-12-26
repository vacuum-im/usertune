#!/bin/sh

LOPTIONS="-no-obsolete -locations none -source-language en"

lupdate=lupdate
if which -s lupdate-qt5; then
	lupdate=lupdate-qt5
fi

lupdate="${lupdate} ${LOPTIONS}"

find . -name '*.pro' -exec ${lupdate} {} \;

