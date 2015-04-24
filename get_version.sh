#!/bin/sh

git update-index -q --ignore-submodules --refresh
# Disallow unstaged changes in the working tree
if ! git diff-files --quiet --ignore-submodules --
then
    echo >&2 "Error: there are unstaged changes."
    git diff-files --name-status -r --ignore-submodules -- >&2
    exit 1
fi

# Disallow uncommitted changes in the index
if ! git diff-index --cached --quiet HEAD --ignore-submodules --
then
    echo >&2 "Error: there are uncommitted changes."
    git diff-index --cached --name-status -r --ignore-submodules HEAD -- >&2
    exit 1
fi

git describe --exact-match 2>/dev/null || echo "0.11-git$(git show -s --pretty=format:%cI.%h | tr -d ':' | tr -d '-' | tr '.' '-' | sed 's/T[0-9\+]*//g')"
