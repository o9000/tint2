### New unstable release: 0.12-rc2
Changes: https://gitlab.com/o9000/tint2/blob/master/ChangeLog

Documentation: https://gitlab.com/o9000/tint2/wikis/home

Try it out with (for dependencies see https://gitlab.com/o9000/tint2/wikis/Install#dependencies):
```
mkdir tint2-0.12-rc2
cd tint2-0.12-rc2
wget 'https://gitlab.com/o9000/tint2/repository/archive.tar.gz?ref=v0.12-rc2' --output-document tint2-0.12-rc2.tar.gz
tar -xzf tint2-0.12-rc2.tar.gz
cd tint2.git
mkdir build
cd build
cmake ..
make -j4
./tint2 &
./src/tint2conf/tint2conf &
```
Please report any problems to https://gitlab.com/o9000/tint2/issues. Your feedback is much appreciated.

P.S. GitLab is now the official location of the tint2 project, migrated from Google Code, which is shutting down. In case you are wondering why not GitHub, BitBucket etc., we chose GitLab because it is open source, it is mature and works well, looks cool and has a very nice team.

### What is tint2?

tint2 is a simple panel/taskbar made for modern X window managers. It was specifically made for Openbox but it should also work with other window managers (GNOME, KDE, XFCE etc.). It is based on ttm http://code.google.com/p/ttm/.

### Features

  * Panel with taskbar, system tray, clock and launcher icons;
  * Easy to customize: color/transparency on fonts, icons, borders and backgrounds;
  * Pager like capability: move tasks between workspaces (virtual desktops), switch between workspaces;
  * Multi-monitor capability: create one panel per monitor, showing only the tasks from the current monitor;
  * Customizable mouse events.

### Goals

  * Be unintrusive and light (in terms of memory, CPU and aesthetic);
  * Follow the freedesktop.org specifications;
  * Make certain workflows, such as multi-desktop and multi-monitor, easy to use.

### I want it!

  * [Install tint2](https://gitlab.com/o9000/tint2/wikis/Install)

### How do I ...

  * [Install](https://gitlab.com/o9000/tint2/wikis/Install)
  * [Configure](https://gitlab.com/o9000/tint2/wikis/Configure)
  * [Add applet not supported by tint2](https://gitlab.com/o9000/tint2/wikis/ThirdPartyApplets)
  * [Other frequently asked questions](https://gitlab.com/o9000/tint2/wikis/FAQ)
  * [Debug](https://gitlab.com/o9000/tint2/wikis/Debug)  

### How can I help out?

  * Report bugs and ask questions on the [issue tracker](https://gitlab.com/o9000/tint2/issues);
  * Contribute to the development by helping us fix bugs and suggesting new features.

### Links
  * Home page: https://gitlab.com/o9000/tint2
  * Git repository: https://gitlab.com/o9000/tint2.git
  * Documentation: https://gitlab.com/o9000/tint2/wikis/home
  * Downloads: https://gitlab.com/o9000/tint2-archive/tree/master or https://code.google.com/p/tint2/downloads/list
  * Old project location (inactive): https://code.google.com/p/tint2

### Releases
  * Latest stable release: tint2 0.11 (June 2010)
  * Next release: planned for mid 2015

### Screenshots
![screenshot](https://gitlab.com/o9000/tint2/wikis/screenshot.png)