#!/bin/bash

# Usage: ./make_release.sh
# Creates a tar.gz archive of the current tree.

VERSION=$(./get_version.sh --strict)
if [ ! $? -eq 0 ]
then
    echo >&2 "Error: get_version.sh failed!"
    exit 1
fi

ARCHIVE=tint2-$VERSION.tar.gz

echo "Making release tint2-$VERSION"
git archive --format=tar.gz --prefix=tint2-$VERSION/ v$VERSION >$ARCHIVE

sha1sum -b $ARCHIVE
sha256sum -b $ARCHIVE
