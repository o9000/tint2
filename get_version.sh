#!/bin/sh

MAJOR=0.12
DIRTY=""

git update-index -q --ignore-submodules --refresh
# Disallow unstaged changes in the working tree
if ! git diff-files --quiet --ignore-submodules --
then
    if [ "$1" = "--strict" ]
    then
        echo >&2 "Error: there are unstaged changes."
        git diff-files --name-status -r --ignore-submodules -- >&2
        exit 1
    else
        DIRTY="-dirty"
    fi
fi

# Disallow uncommitted changes in the index
if ! git diff-index --cached --quiet HEAD --ignore-submodules --
then
    if [ "$1" = "--strict" ]
    then
        echo >&2 "Error: there are uncommitted changes."
        git diff-index --cached --name-status -r --ignore-submodules HEAD -- >&2
        exit 1
    else
        DIRTY="-dirty"
    fi
fi

VERSION=$(git describe --exact-match 2>/dev/null || echo "$MAJOR-git$(git show -s --pretty=format:%cI.%h | tr -d ':' | tr -d '-' | tr '.' '-' | sed 's/T[0-9\+]*//g')")$DIRTY

echo '#define VERSION_STRING "'$VERSION'"' > version.h
echo $VERSION
