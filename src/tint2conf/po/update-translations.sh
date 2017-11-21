#!/bin/bash

set -e
set -x

find .. -name '*.c' | sort -r | xargs xgettext --keyword=_ --language=C --output=tint2conf.pot -
sed -i "s/PACKAGE VERSION/tint2conf $(../../../get_version.sh | head -n1)/g" tint2conf.pot
sed -i "s/CHARSET/UTF-8/g" tint2conf.pot

for f in *.po
do
  lang=$(basename $f .po)
  echo $lang
  msgmerge -i -o $lang.pox $lang.po tint2conf.pot
  cat ${lang}.pox > ${lang}.po
  rm ${lang}.pox
done

set +e
set +x
echo "Status:"
for f in *.po
do
  lang=$(basename $f .po)
  fuzzy=$(cat ${lang}.po | grep -A2 "#, fuzzy")
  missing=$(cat ${lang}.po | grep -B1 'msgstr  ""')
  if [ -z "$fuzzy" ] && [ -z "$missing" ]
  then
    echo $lang ": Up to date"
  else
    count=$(echo -e "$fuzzy" "\n" "$missing" | grep "^--$" | wc -l)
    echo "${lang}: Translation incomplete: ${count} strings to be updated. See ${lang}.todo"
    echo "$fuzzy" > ${lang}.todo
    echo "$missing" >> ${lang}.todo
  fi
done
