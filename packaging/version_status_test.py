#!/usr/bin/env python2
# -*- coding: utf-8 -*-

import unittest
from version_status import *


class TestStringFunctions(unittest.TestCase):
  def test_collapse_multiple_spaces(self):
    self.assertEqual(collapse_multiple_spaces("asdf"), "asdf")
    self.assertEqual(collapse_multiple_spaces("as df"), "as df")
    self.assertEqual(collapse_multiple_spaces("as  df"), "as df")
    self.assertEqual(collapse_multiple_spaces("a  s    d f"), "a s d f")


class TestFtpFunctions(unittest.TestCase):
  def test_ftp_file_name_from_listing(self):
    self.assertEqual(ftp_file_name_from_listing("-rw-rw-r-- 1 1176 1176 1063 Jun 15 10:18 README"), "README")
    self.assertEqual(ftp_file_name_from_listing("-rw-rw-r-- 1 1176 1176 1063 Jun 15 10:18:12 README"), "README")
    self.assertEqual(ftp_file_name_from_listing("-rw-rw-r-- 1 1176 1176 1063 Jun 15 10:18 READ ME"), "READ ME")
    self.assertEqual(ftp_file_name_from_listing("-rw-rw-r-- 1 1176 1176 1063 Jun 15 10:18:12 READ ME"), "READ ME")
    self.assertEqual(ftp_file_name_from_listing("-rw-rw-r-- 1 1176 1176 1063 Jun 15 10:18 README"), "README")
    self.assertEqual(ftp_file_name_from_listing("-rw-rw-r-- 1 1176 1176 1063 Jun 15 10:18:12 README.txt"), "README.txt")
    self.assertEqual(ftp_file_name_from_listing("-rw-rw-r-- 1 1176 1176 1063 Jun 15 10:18:12 READ ME.txt"), "READ ME.txt")

  def test_ftp_list_dir_process_listing(self):
    lines = [ "-rw-rw-r--    1 1176     1176         1063 Jun 15 10:18 README",
              "-rw-rw-r--    1 1176     1176         1063 Jun 15 10:18:11 READ ME.txt",
              "drwxr-sr-x    5 1176     1176         4096 Dec 19  2000 pool",
              "drwxr-sr-x    4 1176     1176         4096 Nov 17  2008 project",
              "drwxr-xr-x    3 1176     1176         4096 Oct 10  2012 tools"]
    dirs_check = ["pool", "project", "tools"]
    files_check = ["README", "READ ME.txt"]
    dirs, files = ftp_list_dir_process_listing(lines)
    dirs.sort()
    dirs_check.sort()
    files.sort()
    files_check.sort()
    self.assertEqual(dirs, dirs_check)
    self.assertEqual(files, files_check)


class TestHttpFunctions(unittest.TestCase):
  def test_http_links_from_listing(self):
    html = """<html>
              <head><title>Index of /debian/dists/</title></head>
              <body bgcolor="white">
              <a href="http://google.com">google/</a>
              <h1>Index of /debian/dists/</h1><hr><pre><a href="../">../</a>
              <a href="bunsen-hydrogen/">bunsen-hydrogen/</a>                                   08-May-2017 20:31       -
              <a href="jessie-backports/">jessie-backports/</a>                                  01-Jul-2017 15:58       -
              <a href="unstable/">unstable/</a>                                          12-Aug-2017 19:32       -
              </pre><hr></body>
              </html>"""
    links_check = ["../", "bunsen-hydrogen/", "jessie-backports/", "unstable/", "http://google.com"]
    links = http_links_from_listing(html)
    links.sort()
    links_check.sort()
    self.assertEqual(links, links_check)

  def test_http_paths_from_listing(self):
    html = """<html>
              <head><title>Index of /debian/dists/</title></head>
              <body bgcolor="white">
              <h1>Index of /debian/dists/</h1><hr><pre><a href="../">../</a>
              <a href="bunsen-hydrogen/">bunsen-hydrogen/</a>                                   08-May-2017 20:31       -
              <a href="jessie-backports/">jessie-backports/</a>                                  01-Jul-2017 15:58       -
              <a href="unstable/">unstable/</a>                                          12-Aug-2017 19:32       -
              </pre><hr></body>
              </html>"""
    paths_check = ["bunsen-hydrogen/", "jessie-backports/", "unstable/"]
    paths = http_paths_from_listing(html)
    paths.sort()
    paths_check.sort()
    self.assertEqual(paths, paths_check)


class TestPackageFunctions(unittest.TestCase):
  def test_deb_packages_extract_version(self):
    packages = """Package: sendmailanalyzer
Version: 9.2-1.1
Architecture: all
Maintainer: Dominique Fournier <dominique@fournier38.fr>
Installed-Size: 749
Pre-Depends: perl
Depends: apache2
Homepage: http://sareport.darold.net/
Priority: optional
Section: mail
Filename: pool/main/s/sendmailanalyzer/sendmailanalyzer_9.2-1.1_all.deb
Size: 143576
SHA256: 0edcbde19a23333c8c894e27af32447582b38e3ccd84122ac07720fdaab8fa0c
SHA1: a7f4dcf42e850acf2c201bc4594cb6b765dced20
MD5sum: adb39196fc33a826b24e9d0e440cba25
Description: Perl Sendmail/Postfix log analyser
 SendmailAnalyzer continuously read your mail log file to generate
 periodical HTML and graph reports. All reports are shown through
 a CGI web interface.
 It reports all you ever wanted to know about email trafic on your network.
 You can also use it in ISP environment with per domain report.

Package: tint2
Version: 0.14.6-1
Architecture: amd64
Maintainer: Jens John <dev@2ion.de>
Installed-Size: 1230
Depends: libatk1.0-0 (>= 1.12.4), libc6 (>= 2.15), libcairo2 (>= 1.2.4), libfontconfig1 (>= 2.11), libfreetype6 (>= 2.2.1), libgdk-pixbuf2.0-0 (>= 2.22.0), libglib2.0-0 (>= 2.35.9), libgtk2.0-0 (>= 2.14.0), libimlib2 (>= 1.4.5), libpango-1.0-0 (>= 1.20.0), libpangocairo-1.0-0 (>= 1.14.0), libpangoft2-1.0-0 (>= 1.14.0), librsvg2-2 (>= 2.14.4), libstartup-notification0 (>= 0.4), libx11-6, libxcomposite1 (>= 1:0.3-1), libxdamage1 (>= 1:1.1), libxfixes3, libxinerama1, libxrandr2 (>= 2:1.2.99.3), libxrender1
Homepage: https://gitlab.com/o9000/tint2/
Priority: optional
Section: x11
Filename: pool/main/t/tint2/tint2_0.14.6-1_amd64.deb
Size: 279638
SHA256: c96e745425a97828952e9e0277176fe68e2512056915560ac968a66c88a0a8b7
SHA1: 82edd60429a494bb127e6d8a10434fca0ee60f61
MD5sum: 65455638fb41503361560b25a70b33b7
Description: lightweight taskbar
 Tint is a simple panel/taskbar intentionally made for openbox3, but should
 also work with other window managers. The taskbar includes transparency and
 color settings for the font, icons, border, and background. It also supports
 multihead setups, customized mouse actions, and a built-in clock. Tint was
 originally based on ttm code. Since then, support has also been added
 for a battery monitor and system tray.
 .
 The goal is to keep a clean and unintrusive look with lightweight code and
 compliance with freedesktop specification.

Package: xfce4-power-manager
Version: 1.4.4-4~bpo8+1
Architecture: amd64
Maintainer: Debian Xfce Maintainers <pkg-xfce-devel@lists.alioth.debian.org>
Installed-Size: 541
Depends: libc6 (>= 2.4), libcairo2 (>= 1.2.4), libdbus-1-3 (>= 1.0.2), libdbus-glib-1-2 (>= 0.88), libgdk-pixbuf2.0-0 (>= 2.22.0), libglib2.0-0 (>= 2.41.1), libgtk2.0-0 (>= 2.24.0), libnotify4 (>= 0.7.0), libpango-1.0-0 (>= 1.14.0), libpangocairo-1.0-0 (>= 1.14.0), libupower-glib3 (>= 0.99.0), libx11-6, libxext6, libxfce4ui-1-0 (>= 4.9.0), libxfce4util6 (>= 4.9.0), libxfconf-0-2 (>= 4.6.0), libxrandr2 (>= 2:1.2.99.2), upower (>= 0.99), xfce4-power-manager-data (= 1.4.4-4~bpo8+1)
Recommends: libpam-systemd, xfce4-power-manager-plugins
Homepage: http://goodies.xfce.org/projects/applications/xfce4-power-manager
Priority: optional
Section: xfce
Filename: pool/main/x/xfce4-power-manager/xfce4-power-manager_1.4.4-4~bpo8+1_amd64.deb
Size: 214122
SHA256: 992b606afe5e9934bce19a1df2b8d7067c98b9d64e23a9b63dbd0c4cf28b4ac9
SHA1: 6bfcd77071f31577a37abab063bf21a34f4d616c
MD5sum: fb777aecbbfe39742649b768eb22c697
Description: power manager for Xfce desktop
 This power manager for the Xfce desktop enables laptop users to set up
 a power profile for two different modes "on battery power" and "on ac
 power" while still allowing desktop users to at least change the DPMS
 settings and CPU frequency using the settings dialogue..
 .
 It features:
   * battery monitoring
   * cpu frequency settings
   * monitor DPMS settings
   * suspend/Hibernate
   * LCD brightness control
   * Lid, sleep and power switches control"""
    version, maintainer = deb_packages_extract_version(packages, "tint2")
    self.assertEqual(version, "0.14.6-1")
    self.assertEqual(maintainer, "Jens John <dev@2ion.de>")
    version, maintainer = deb_packages_extract_version(packages, "asdf")
    self.assertEqual(version, None)
    self.assertEqual(maintainer, None)

  def test_arch_packages_extract_version(self):
    pkgbuild = """# $Id$
# Maintainer: Alexander F Rødseth <xyproto@archlinux.org>
# Contributor: Blue Peppers <bluepeppers@archlinux.us>
# Contributor: Stefan Husmann <stefan-husmann@t-online.de>
# Contributor: Yannick LM <LMyannicklm1337@gmail.com>

pkgname=tint2
pkgver=0.14.6
pkgrel=2
pkgdesc='Basic, good-looking task manager for WMs'
arch=('x86_64' 'i686')
url='https://gitlab.com/o9000/tint2'
license=('GPL2')
depends=('gtk2' 'imlib2' 'startup-notification')
makedepends=('cmake' 'startup-notification' 'git' 'ninja' 'setconf')
source=("$pkgname-$pkgver.tar.bz2::https://gitlab.com/o9000/tint2/repository/archive.tar.bz2?ref=$pkgver")
sha256sums=('b40079fb187aa248cd3b6957076f138d040c723b309e1b254ac0c8ec9826a451')

prepare() {
  mv "$pkgname-$pkgver-"* "$pkgname"
  setconf "$pkgname/get_version.sh" VERSION "$pkgver"
}

build() {
  mkdir -p build
  cd build
  cmake "../$pkgname" \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DENABLE_TINT2CONF=1 \
    -GNinja
  ninja
}

package() {
  DESTDIR="$pkgdir" ninja -C build install
}

# getver: gitlab.com/o9000/tint2/blob/master/README.md
# vim: ts=2 sw=2 et:"""
    version, maintainer = arch_pkgbuild_extract_version(pkgbuild)
    self.assertEqual(version, "0.14.6")
    self.assertEqual(maintainer, "Alexander F Rødseth <xyproto@archlinux.org>")


if __name__ == '__main__':
  unittest.main()
