#!/bin/sh

FALLBACK=\"0.11-svn\"

if [ $# -eq 0 ]; then
  DIR=.
else
  DIR=$1
fi

if [ -f version.h ]; then
  REV_OLD=$(cat version.h  | cut -d" " -f3)
else
  REV_OLD=\"\"
fi

if [ -x "$(which svnversion 2>/dev/null)" -a -d "${DIR}/.svn" ] ; then
  REV=\"$(svnversion -n ${DIR})\"
else
  REV=${FALLBACK}
fi

if [ ${REV_OLD} != ${REV} ]; then
  echo "Building new version.h"
  echo "Rev_old: ${REV_OLD} Rev: ${REV}"
  echo "#define VERSION_STRING ${REV}" > version.h
fi
