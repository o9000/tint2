#!/bin/bash

PS4='\e[99;33m${BASH_SOURCE}:$LINENO\e[0m '
set -e
set -x

sudo apt-get install ninja-build python2.7 wget
sudo apt-get build-dep libx11-6 libxrender1 libcairo2 libglib2.0-0 libpango-1.0-0 libimlib2 librsvg2-2

export PKG_CONFIG_PATH=""

DEPS=$HOME/tint2-deps
X11=$(pkg-config --modversion x11)
XRENDER=$(pkg-config --modversion xrender)
XCOMPOSITE=$(pkg-config --modversion xcomposite)
XDAMAGE=$(pkg-config --modversion xdamage)
XEXT=$(pkg-config --modversion xext)
XRANDR=$(pkg-config --modversion xrandr)
XINERAMA=$(pkg-config --modversion xinerama)
IMLIB=$(pkg-config --modversion imlib2)
GLIB=$(pkg-config --modversion glib-2.0)
CAIRO=$(pkg-config --modversion cairo)
PANGO=$(pkg-config --modversion pango)
PIXBUF=$(pkg-config --modversion gdk-pixbuf-2.0)
RSVG=$(pkg-config --modversion librsvg-2.0)

mkdir -p $DEPS/src

function two_digits() {
    echo "$@" | cut -d. -f 1-2
}

function download_and_build() {
    URL="$1"
    shift
    CFGARGS="$@"
    ARCHIVE="$(basename "$URL")"
    NAME="$(echo "$ARCHIVE" | sed s/\.tar.*$//g)"

    cd "$DEPS/src"
    rm -rf "$ARCHIVE"
    rm -rf "$NAME"
    wget "$URL" -O "$ARCHIVE"
    tar xf "$ARCHIVE"
    cd "$NAME"
    export PKG_CONFIG_PATH="$DEPS/lib/pkgconfig"
    export PATH="$DEPS/bin:$PATH"
    export CFLAGS="-O0 -fno-common -fno-omit-frame-pointer -rdynamic -fsanitize=address -g"
    export LDFLAGS="-Wl,--no-as-needed -Wl,-z,defs -O0 -fno-common -fno-omit-frame-pointer -rdynamic -fsanitize=address -fuse-ld=gold -g -ldl -lasan"
    if [[ -x ./configure ]]
    then
        ./configure "--prefix=$DEPS" "$@"
        make -j
        make install
    elif [[ -f meson.build ]]
    then
        mkdir build
        cd build
        meson "--prefix=$DEPS" "$@" ..
        ninja install
    else
        echo "unknown build method"
        exit 1
    fi
}

download_and_build "https://www.x.org/archive/individual/lib/libX11-$X11.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXrender-$XRENDER.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXcomposite-$XCOMPOSITE.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXdamage-$XDAMAGE.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXext-$XEXT.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXrandr-$XRANDR.tar.gz" --enable-static=no
download_and_build "https://www.x.org/archive//individual/lib/libXinerama-$XINERAMA.tar.gz" --enable-static=no
download_and_build "https://downloads.sourceforge.net/enlightenment/imlib2-$IMLIB.tar.bz2" --enable-static=no
download_and_build "https://ftp.gnome.org/pub/gnome/sources/glib/$(two_digits "$GLIB")/glib-$GLIB.tar.xz" --enable-debug=yes
download_and_build "https://ftp.gnome.org/pub/gnome/sources/gdk-pixbuf/$(two_digits "$PIXBUF")/gdk-pixbuf-$PIXBUF.tar.xz"
download_and_build "https://cairographics.org/snapshots/cairo-$CAIRO.tar.xz"
download_and_build "https://ftp.gnome.org/pub/gnome/sources/pango/$(two_digits "$PANGO")/pango-$PANGO.tar.xz"
download_and_build "https://ftp.gnome.org/pub/gnome/sources/librsvg/$(two_digits "$RSVG")/librsvg-$RSVG.tar.xz" --enable-pixbuf-loader
