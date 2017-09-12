#!/bin/bash

exec > ~/tint2.runner-version.log
exec 2>&1

set -e
set -x


cd ~/tint2.wiki
git reset --hard
git pull


~/tint2/packaging/version_status.py > packaging.tmp.md
cat packaging.tmp.md > packaging.md
rm packaging.tmp.md

git commit -am 'Update packaging info'
git push
