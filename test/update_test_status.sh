#!/bin/bash

set -e
set -x

[ "${FLOCKER}" != "$0" ] && exec env FLOCKER="$0" flock -en "$0" "$0" "$@" || :

exec > ~/tint2.runner-test.log
exec 2>&1

cd ~/tint2
git reset --hard
git pull


cd ~/tint2.wiki
git reset --hard
git pull


cd ~/tint2/test
~/tint2/test/regression.py > ~/tint2.wiki/tests.tmp.md
cat ~/tint2.wiki/tests.tmp.md > ~/tint2.wiki/tests.md
rm ~/tint2.wiki/tests.tmp.md

cd ~/tint2.wiki
git add tests.md
git commit -am 'Update test results'
git push origin master
