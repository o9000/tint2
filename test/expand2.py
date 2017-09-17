#!/usr/bin/env python2

# Creates directory tree printed by:
# bash -c "for d in /sys/class/power_supply/* ; do find $d/ -exec sh -c 'echo {} ; cat {} ' ';' ; done" 2>&1 | tee out.txt

import os
import sys


def flush(path, content):
  if not path or "/" not in path:
    return
  path = "./" + path
  dir_path, fname = path.rsplit("/", 1)
  try:
    os.makedirs("./" + dir_path)
  except:
    pass
  with open("./" + path, "w") as f:
    f.write(content)


with open(sys.argv[1], "r") as f:
  path = None
  content = ""
  for line in f:
    if "/" in line and ":" in line:
      flush(path, content)
      content = ""
      path, content = line.split(":", 1)
    else:
      content += line
  if content:
    flush(path, content)
