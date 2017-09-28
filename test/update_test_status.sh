#!/bin/bash

set -e
set -x

[ "${FLOCKER}" != "$0" ] && exec env FLOCKER="$0" flock -en "$0" "$0" "$@" || :

exec > ~/tint2.runner-version.log
exec 2>&1

cd ~/tint2.wiki
git reset --hard
git pull


~/tint2/test/regression.py > tests.tmp.md
cat tests.tmp.md > tests.md
rm tests.tmp.md

git add tests.md
git commit -am 'Update test results'
git push origin master
