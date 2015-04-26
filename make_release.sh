#!/bin/bash

# Usage: ./make_release.sh
# Creates a tar.gz archive of the current tree.
#
# To bump the version number for the current commit (make sure you are in HEAD!), run manually:
#
# git tag -a v0.12 -m 'Version 0.12'
#
# To generate a release for an older tagged commit, first list the tags with:
#
# git tag
#
# then checkout the tagged commit with:
#
# git checkout tags/v0.1
#
# Finally, to revert to HEAD:
#
# git checkout master
#
# See more at https://gitlab.com/o9000/tint2/wikis/Development

VERSION=$(./get_version.sh --strict)
if [ ! $? -eq 0 ]
then
    echo >&2 "Error: get_version.sh failed!"
    exit 1
fi

DIR=tint2-$VERSION
ARCHIVE=$DIR.tar.gz
echo "Making release $DIR"
rm -rf $DIR $ARCHIVE

git checkout-index --prefix=$DIR/ -a

# Delete unneeded files
rm -f $DIR/make_release.sh

echo "echo \"#define VERSION_STRING \\\"$VERSION\\\"\" > version.h" > $DIR/get_version.sh

# Create tarball and remove the exported directory
tar -czf $ARCHIVE $DIR
rm -rf $DIR

sha1sum -b $ARCHIVE
sha256sum -b $ARCHIVE
