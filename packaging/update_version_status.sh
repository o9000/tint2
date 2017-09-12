#!/bin/bash

exec > ~/tint2.runner-version.log
exec 2>&1

set -e
set -x


cd ~/tint2.wiki
git reset --hard
git pull


./version_status.py > ./tint2.wiki/packaging.md


git commit -am 'Update packaging info'
git push
