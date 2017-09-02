#!/bin/bash

echo -e '\033kTerminal\033\\'

echo "" > ../../tint2.wiki/screenshots.md

for f in ../themes/*tint2rc
do
  reset
  echo $f
  name=$(basename -s .tint2rc $f)
  ( ( sleep 1 ; import -window tint2 ../../tint2.wiki/screenshots/${name}.png ; echo "Screenshot taken for ${name}!" ) &)
  ../build/tint2 -c $f
  sleep 1
  echo "![${name}](screenshots/${name})\n" >> ../../tint2.wiki/screenshots.md
done
