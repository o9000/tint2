#!/bin/bash

# You can install md2man with gem install md2man. You need gem and ruby-dev.

md2man-roff tint2.md > tint2.1

cat header.html > manual.html
cat tint2.md | sed 's/^# TINT2 .*$/# TINT2/g' | md2man-html >> manual.html
cat footer.html >> manual.html

cat header.html > readme.html
cat ../README.md | sed 's|doc/tint2.md|manual.html|g' | md2man-html >> readme.html
cat footer.html >> readme.html
