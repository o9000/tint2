#!/bin/sh

MAJOR=0.14
DIRTY=""

if git status 1>/dev/null 2>/dev/null
then
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
    VERSION=$(git describe --exact-match 2>/dev/null || echo "$MAJOR-git$(git show -s --pretty=format:%ci | cut -d ' ' -f 1 | tr -d '-').$(git show -s --pretty=format:%h)")$DIRTY
else
    VERSION=$(head -n 1 ChangeLog | cut -d ' ' -f 2)
    if [ $VERSION = "master" ]
    then
        VERSION=$VERSION-$(head -n 1 ChangeLog | cut -d ' ' -f 1)
    fi
fi


VERSION=$(echo "$VERSION" | sed 's/^v//')

echo '#define VERSION_STRING "'$VERSION'"' > version.h
echo $VERSION
