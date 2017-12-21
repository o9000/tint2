#!/bin/sh

SCRIPT_DIR=$(dirname "$0")
DIRTY=""
VERSION=""

OLD_DIR=$(pwd)
cd ${SCRIPT_DIR}

if [ -d .git ] && git status 1>/dev/null 2>/dev/null
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
    if git describe 1>/dev/null 2>/dev/null
    then
        VERSION=$(git describe 2>/dev/null)$DIRTY
    elif git log -n 1 1>/dev/null 2>/dev/null
    then
        VERSION=$(head -n 1 "ChangeLog" | cut -d ' ' -f 2)
        if [ "$VERSION" = "master" ]
        then
            PREVIOUS=$(grep '^2' "ChangeLog" | head -n 2 | tail -n 1 | cut -d ' ' -f 2)
            HASH=$(git log -n 1 --pretty=format:%cI.%ct.%h | tr -d ':' | tr -d '-' | tr '.' '-' | sed 's/T[0-9\+]*//g' 2>/dev/null)
            VERSION=$PREVIOUS-next-$HASH
        fi
    fi
fi

if [ -z "$VERSION" ]
then
    VERSION=$(head -n 1 "ChangeLog" | cut -d ' ' -f 2)
    if [ "$VERSION" = "master" ]
    then
        VERSION=$VERSION-$(head -n 1 "ChangeLog" | cut -d ' ' -f 1)
    fi
fi

cd "${OLD_DIR}"

VERSION=$(echo "$VERSION" | sed 's/^v//')

echo '#define VERSION_STRING "'$VERSION'"' > version.h
echo $VERSION
