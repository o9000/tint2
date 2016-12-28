#!/usr/bin/env python2

# Creates directory tree printed by:
# bash -c "for d in /sys/class/power_supply/* ; do find $d/ -exec sh -c 'echo {} ; cat {} ' ';' ; done" 2>&1 | tee out.txt

import os
import sys


def flush(path, content):
  if not path:
    return
  if content.startswith("cat: %s: Is a directory" % path):
    try:
      os.makedirs("./" + path)
    except:
      pass
  elif content.startswith("cat: %s:" % path):
    with open("./" + path, "w") as f:
      pass
  else:
    with open("./" + path, "w") as f:
      f.write(content)


with open(sys.argv[1], "r") as f:
  path = None
  content = ""
  for line in f:
    if line.startswith("/"):
      flush(path, content)
      content = ""
      path = line.strip()
    else:
      content += line
  if content:
    flush(path, content)
