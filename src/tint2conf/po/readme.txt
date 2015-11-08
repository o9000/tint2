Updating pot file:

find .. -name '*.c' | sort -r | xargs xgettext --keyword=_ --language=C --output=updated.pot -

Followed by manual editing of updated.pot to make sure the header is OK. Then:

cat updated.pot > tint2conf.pot && rm -f updated.pot

Then update the po files:

for f in *.po ; do lang=$(basename $f .po); echo $lang ; msgmerge -i -o $lang.pox $lang.po tint2conf.pot ; cat ${lang}.pox > ${lang}.po ; rm ${lang}.pox ; done
