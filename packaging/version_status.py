#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import sys
reload(sys)
sys.setdefaultencoding('utf8')
import datetime
import xml.etree.ElementTree as ET
import ftplib
import gzip
import json
import re
from StringIO import StringIO
import urllib2


# Returns true if `value` is an integer represented as a string.
def is_int(value):
  # type: (str) -> bool
  try:
    value = int(value)
  except ValueError:
    return False
  return True


# Returns a new string with all instances of multiple whitespace
# replaced with a single space.
def collapse_multiple_spaces(line):
  # type: (str) -> str
  return " ".join(line.split())


# Extracts the file name from a line of an FTP listing.
# The input must be a valid directory entry (starting with "-" or "d").
def ftp_file_name_from_listing(line):
  # type: (str) -> str
  line = collapse_multiple_spaces(line)
  return line.split(" ", 8)[-1]


# Extracts a list of the directories and a list of the files
# from an FTP listing.
def ftp_list_dir_process_listing(lines):
  # type: (List[str]) -> List[str], List[str]
  dirs = []
  files = []
  for line in lines:
    if line.startswith("d"):
      dirs.append(ftp_file_name_from_listing(line))
    elif line.startswith("-"):
      files.append(ftp_file_name_from_listing(line))
  return dirs, files


# Lists the remote FTP directory located at `path`.
# Returns a list of the directories and a list of the files.
def ftp_list_dir(ftp, path):
  # type: (ftplib.FTP, str) -> List[str], List[str]
  ftp.cwd(path)
  lines = []
  ftp.retrlines("LIST", lines.append)
  return ftp_list_dir_process_listing(lines)


# Downloads a binary file to a string.
# Returns the string.
def ftp_download(ftp, path):
  # type: (ftplib.FTP, str) -> str
  blocks = []
  ftp.retrbinary("RETR {0}".format(path), blocks.append)
  return "".join(blocks)


# Extracts the list of links from an HTML string.
def http_links_from_listing(html):
  # type: (str) -> List[str]
  pattern = re.compile(r"""href=['"]+([^'"]+)['"]+""")
  return re.findall(pattern, html)


# Extracts the list of paths (relative links, except to ../*) from an HTML string.
def http_paths_from_listing(html):
  # type: (str) -> List[str]
  paths = []
  for link in http_links_from_listing(html):
    if link.startswith(".."):
      continue
    if link == "./" or link == "/":
      continue
    if "://" in link:
      continue
    paths.append(link)
  return paths


# Downloads a file as string from an URL. Decodes correctly.
def http_download_txt(url):
  # type: (str) -> str
  try:
    r = urllib2.urlopen(url)
    encoding = r.headers.getparam("charset")
    if not encoding:
      encoding = "utf-8"
    return r.read().decode(encoding)
  except:
    raise


# Extracts the list of paths (relative links, except to ../*) from the HTML code
# located at `url`.
def http_list_dir(url):
  # type: (str) -> List[str]
  try:
    html = http_download_txt(url)
  except:
    return []
  return http_paths_from_listing(html)


# Extracts the version and maintainer info for a package, from a Debian repository Packages file.
def deb_packages_extract_version(packages, name):
  # type: (str, str) -> str, str
  inside = False
  version = None
  maintainer = None
  for line in packages.split("\n"):
    if line == "Package: " + name:
      inside = True
    elif not line:
      if inside:
        break
    else:
      if inside:
        if line.startswith("Version:"):
          version = line.split(":", 1)[-1].strip()
        elif line.startswith("Maintainer:"):
          maintainer = line.split(":", 1)[-1].strip()
  return version, maintainer

# Extracts the version and maintainer info for a package, from an Arch PKGBUILD file.
def arch_pkgbuild_extract_version(pkgbuild):
  # type: (str) -> str, str
  version = None
  maintainer = None
  for line in pkgbuild.split("\n"):
    if line.startswith("# Maintainer:"):
      maintainer = line.split(":", 1)[-1].strip()
    elif line.startswith("pkgver="):
      version = line.split("=", 1)[-1].strip()
  return version, maintainer


# Debian

def get_debian_release_version(release):
  data = http_download_txt("http://metadata.ftp-master.debian.org/changelogs/main/t/tint2/{0}_changelog".format(release))
  version = data.split("\n", 1)[0].split("(", 1)[-1].split(")", 1)[0].strip()
  maintainer = [line.split("--", 1)[-1] for line in data.split("\n") if line.startswith(" --")][0].split("  ")[0].strip()
  return release, version, maintainer


def get_debian_versions():
  print >> sys.stderr, "Debian ..."
  return "Debian", "debian", [get_debian_release_version(release) for release in ["stable", "testing", "unstable", "experimental"]]


# Ubuntu

def get_ubuntu_versions():
  print >> sys.stderr, "Ubuntu ..."
  data = http_download_txt("https://api.launchpad.net/1.0/ubuntu/+archive/primary?ws.op=getPublishedSources&source_name=tint2&exact_match=true")
  data = json.loads(data)["entries"]
  data.reverse()
  versions = []
  for package in data:
    if package["status"] == "Published":
      version = package["source_package_version"]
      release = package["distro_series_link"].split("/")[-1]
      maintainer = package["package_maintainer_link"]
      versions.append((release, version, maintainer))
  return "Ubuntu", "ubuntu", versions


# BunsenLabs

def get_bunsenlabs_versions():
  print >> sys.stderr, "BunsenLabs ..."
  dirs = http_list_dir("https://eu.pkg.bunsenlabs.org/debian/dists/")
  versions = []
  for d in dirs:
    if d.endswith("/") and "/" not in d[:-1]:
      release = d.replace("/", "")
      packages = http_download_txt("https://eu.pkg.bunsenlabs.org/debian/dists/{0}/main/binary-amd64/Packages".format(release))
      version, maintainer = deb_packages_extract_version(packages, "tint2")
      if version:
        versions.append((release, version, maintainer))
  return "BunsenLabs", "bunsenlabs", versions


# Arch

def get_arch_versions():
  print >> sys.stderr, "Arch ..."
  pkgbuild = http_download_txt("https://git.archlinux.org/svntogit/community.git/plain/trunk/PKGBUILD?h=packages/tint2")
  version, maintainer = arch_pkgbuild_extract_version(pkgbuild)
  return "Arch Linux", "archlinux", [("Community", version, maintainer)]


# Fedora

def get_fedora_versions():
  print >> sys.stderr, "Fedora ..."
  dirs = http_list_dir("http://mirror.switch.ch/ftp/mirror/fedora/linux/development/")
  versions = []
  for d in dirs:
    if d.endswith("/") and "/" not in d[:-1]:
      release = d.replace("/", "")
      packages = http_list_dir("http://mirror.switch.ch/ftp/mirror/fedora/linux/development/{0}/Everything/source/tree/Packages/t/".format(release))
      for p in packages:
        if p.startswith("tint2-"):
          version = p.split("-", 1)[-1].split(".fc")[0]
          v = (release, version, "")
          if v not in versions:
            versions.append(v)
  return "Fedora", "fedora", versions


# Red Hat (EPEL)

def get_redhat_epel_versions():
  print >> sys.stderr, "RedHat ..."
  dirs = http_list_dir("http://mirror.switch.ch/ftp/mirror/epel/")
  versions = []
  for d in dirs:
    if d.endswith("/") and "/" not in d[:-1] and is_int(d[:-1]):
      release = d.replace("/", "")
      packages = http_list_dir("http://mirror.switch.ch/ftp/mirror/epel/{0}/SRPMS/t/".format(release))
      for p in packages:
        if p.startswith("tint2-"):
          version = p.split("-", 1)[-1].split(".el")[0]
          v = (release, version, "")
          if v not in versions:
            versions.append(v)
  return "RedHat (EPEL)", "rhel", versions


# SUSE

def get_suse_versions():
  print >> sys.stderr, "Suse ..."
  ftp = ftplib.FTP("mirror.switch.ch")
  ftp.login()
  releases, _ = ftp_list_dir(ftp, "/mirror/opensuse/opensuse/ports/update/leap/")
  versions = []
  for release in releases:
    root = "/mirror/opensuse/opensuse/ports/update/leap/{0}/oss/repodata/".format(release)
    _, files = ftp_list_dir(ftp, root)
    for fname in files:
      if fname.endswith("-primary.xml.gz"):
        data = ftp_download(ftp, "{0}/{1}".format(root, fname))
        xml = gzip.GzipFile(fileobj=StringIO(data)).read()
        root = ET.fromstring(xml)
        for package in root.findall("{http://linux.duke.edu/metadata/common}package"):
          name = package.find("{http://linux.duke.edu/metadata/common}name").text
          if name == "tint2":
            version = package.find("{http://linux.duke.edu/metadata/common}version").get("ver")
            versions.append((release, version, ""))
  ftp.quit()
  return "OpenSUSE", "opensuse", versions


# Gentoo

def get_gentoo_versions():
  print >> sys.stderr, "Gentoo ..."
  files = http_list_dir("https://gitweb.gentoo.org/repo/gentoo.git/tree/x11-misc/tint2")
  versions = []
  for f in files:
    if "tint2" in f and f.endswith(".ebuild"):
      version = f.split("tint2-")[-1].split(".ebuild")[0]
      v = ("", version, "")
      if v not in versions:
        versions.append(v)
  return "Gentoo", "gentoo", versions


# Void

def get_void_versions():
  print >> sys.stderr, "Void ..."
  template = http_download_txt("https://raw.githubusercontent.com/voidlinux/void-packages/master/srcpkgs/tint2/template")
  versions = []
  version = None
  maintainer = None
  for line in template.split("\n"):
    if line.startswith("version="):
      version = line.split("=", 1)[-1].replace('"', "").strip()
    elif line.startswith("maintainer="):
      maintainer = line.split("=", 1)[-1].replace('"', "").strip()
  if version:
    versions.append(("", version, maintainer))
  return "Void Linux", "void", versions


# Alpine

def get_alpine_versions():
  print >> sys.stderr, "Alpine ..."
  apkbuild = http_download_txt("https://git.alpinelinux.org/cgit/aports/plain/community/tint2/APKBUILD")
  versions = []
  version = None
  maintainer = None
  for line in apkbuild.split("\n"):
    if line.startswith("pkgver="):
      version = line.split("=", 1)[-1].replace('"', "").strip()
    elif line.startswith("# Maintainer:"):
      maintainer = line.split(":", 1)[-1].replace('"', "").strip()
  if version:
    versions.append(("", version, maintainer))
  return "Alpine Linux", "alpine", versions


# Slackware

def get_slack_versions():
  print >> sys.stderr, "Slackware ..."
  dirs = http_list_dir("https://slackbuilds.org/slackbuilds/")
  versions = []
  for d in dirs:
    if d.endswith("/") and "/" not in d[:-1]:
      release = d.replace("/", "")
      try:
        info = http_download_txt("https://slackbuilds.org/slackbuilds/{0}/desktop/tint2/tint2.info".format(release))
      except:
        continue
      version = None
      maintainer = None
      for line in info.split("\n"):
        if line.startswith("VERSION="):
          version = line.split("=", 1)[-1].replace('"', "").strip()
        elif line.startswith("MAINTAINER="):
          maintainer = line.split("=", 1)[-1].replace('"', "").strip()
      if version:
        versions.append((release, version, maintainer))
  return "Slackware", "slackware", versions

# FreeBSD

def get_freebsd_versions():
  print >> sys.stderr, "FreeBSD ..."
  makefile = http_download_txt("https://svnweb.freebsd.org/ports/head/x11/tint/Makefile?view=co")
  versions = []
  version = None
  maintainer = None
  for line in makefile.split("\n"):
    if line.startswith("PORTVERSION="):
      version = line.split("=", 1)[-1].strip()
    elif line.startswith("MAINTAINER="):
      maintainer = line.split("=", 1)[-1].strip()
  if version:
    versions.append(("", version, maintainer))
  return "FreeBSD", "freebsd", versions


# OpenBSD

def get_openbsd_versions():
  print >> sys.stderr, "OpenBSD ..."
  makefile = http_download_txt("http://cvsweb.openbsd.org/cgi-bin/cvsweb/~checkout~/ports/x11/tint2/Makefile?rev=1.5&content-type=text/plain")
  versions = []
  version = None
  for line in makefile.split("\n"):
    if line.startswith("V="):
      version = line.split("=", 1)[-1].strip()
  if version:
    versions.append(("", version, ""))
  return "OpenBSD", "openbsd", versions


# Upstream

def get_tint2_version():
  print >> sys.stderr, "Upstream ..."
  readme = http_download_txt("https://gitlab.com/o9000/tint2/raw/master/README.md")
  version = readme.split("\n", 1)[0].split(":", 1)[-1].strip()
  return version


def main():
  latest = get_tint2_version()
  distros = []
  distros.append(get_debian_versions())
  distros.append(get_bunsenlabs_versions())
  distros.append(get_ubuntu_versions())
  distros.append(get_fedora_versions())
  distros.append(get_redhat_epel_versions())
  distros.append(get_suse_versions())
  distros.append(get_alpine_versions())
  distros.append(get_slack_versions())
  distros.append(get_arch_versions())
  distros.append(get_void_versions())
  distros.append(get_gentoo_versions())
  distros.append(get_freebsd_versions())
  distros.append(get_openbsd_versions())
  print "| Distribution | Release | Version | Status |"
  print "| ------------ | ------- | ------- | ------ |"
  for dist, dcode, releases in distros:
    icon = "![](numix-icons/distributor-logo-{0}.svg.png)".format(dcode)
    for r in releases:
      if r[1].split("-", 1)[0] == latest:
        status = ":white_check_mark: Latest"
      else:
        status = ":warning: Out of date"
      print "| {0} {1} | {2} | {3} | {4} |".format(icon, dist, r[0], r[1], status)
  utc_datetime = datetime.datetime.utcnow()
  print ""
  print "Last updated:", utc_datetime.strftime("%Y-%m-%d %H:%M UTC")


if __name__ == "__main__":
  main()
