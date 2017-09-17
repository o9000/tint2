#!/usr/bin/env python

import os
import os.path
import tarfile
try:
  from StringIO import StringIO
except ImportError:
  from io import StringIO


class TarWriter:
  def __init__(self, out_name):
    print("Creating: " + out_name)
    self.tar = tarfile.open(out_name, "w")

  def add(self, path):
    print("Adding: " + path)
    if os.path.isfile(path):
      metadata = self.tar.gettarinfo(path)
      try:
        with open(path) as f:
          buf = f.read()
          fbuf = StringIO(buf)
          metadata.size = len(buf)
          self.tar.addfile(metadata, fbuf)
          fbuf.close()
      except:
        fbuf = StringIO()
        metadata.size = 0
        self.tar.addfile(metadata, fbuf)
        fbuf.close()

  def close(self):
    self.tar.close()


writer = TarWriter("battery.tar")
for root, dirs, files in os.walk("/sys/class/power_supply"):
  for device in dirs:
    for root2, dirs2, files2 in os.walk(root + "/" + device):
      for f in files2:
        writer.add(root2 + "/" + f)
writer.close()
print("Finished.")
