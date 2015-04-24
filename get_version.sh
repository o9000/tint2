#!/bin/sh

git describe --exact-match 2>/dev/null || echo "0.11-git$(git show -s --pretty=format:%cI.%h | tr -d ':' | tr -d '-' | tr '.' '-' | sed 's/T[0-9\+]*//g')"
