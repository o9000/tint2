#!/bin/bash

# Usage: ./make_release.sh
# Creates a tar.bz2 archive of the current tree.
#
# To bump the version number for the current commit (make sure you are in HEAD!), run manually:
#
# git tag -a v0.12 -m 'Version 0.12'
#
# To generate a release for an older tagged commit, first list the tags with:
#
# git tags
#
# then checkout the tagged commit with:
#
# git checkout tags/v0.1
#
# Finally, to revert to HEAD:
#
# git checkout master

VERSION=$(./get_version.sh --strict)
if [ ! $? -eq 0 ]
then
    echo >&2 "Error: get_version.sh failed!"
    exit 1
fi

DIR=tint2-$VERSION
echo "Making release $DIR"
rm -rf $DIR

git checkout-index --prefix=$DIR/ -a

# Delete unneeded files
rm -f $DIR/make_release.sh

echo "echo \"#define VERSION_STRING \\\"$VERSION\\\"\" > version.h" > $DIR/get_version.sh

# Create tarball and remove the exported directory
tar -cjf $DIR.tar.bz2 $DIR
rm -rf $DIR

sha1sum -b $DIR.tar.bz2
sha256sum -b $DIR.tar.bz2
