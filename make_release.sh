#!/bin/bash

# usage: ./make_release.sh RELEASE_VERSION_NUMBER

if [[ $# -ne 1 ]]; then
  echo "usage: $0 RELEASE_VERSION_NUMBER"
  exit
fi

DIR=tint2-${1}
echo "Making release ${DIR}"
rm -Rf ${DIR}
svn export . ${DIR} > /dev/null

# delete unneeded files
rm -f ${DIR}/configure ${DIR}/make_release.sh

# replace get_svnrev.sh by a simple echo command
echo "echo \"#define VERSION_STRING \\\"${1}\\\"\" > version.h" > ${DIR}/get_svnrev.sh

# create tarball and remove the exported directory
tar -cjf ${DIR}.tar.bz2 ${DIR}
rm -Rf ${DIR}
