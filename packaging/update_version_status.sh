#!/bin/bash

set -e
set -x


cd ~/tint2.wiki
git reset --hard
git pull


./version_status.py > ./tint2.wiki/packaging.md


git commit -am 'Update packaging info'
git push
