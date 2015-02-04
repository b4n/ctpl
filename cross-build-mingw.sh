#!/bin/sh
# A script to automate setup and build for Windows cross-compilation
#
# What it does:
# 1) prepare a build directory
# 2) download and unpacked the dependencies
#    (see http://www.gtk.org/download/win32.php)
# 2.1) fixup the unpacked pkg-config file paths
# 3) setup the few required paths
# 4) configure with sensible options for this
# 5) build
# 6) install in a local directory
# 7) pack the installation in a ZIP file, ready to be used
#    (but does not pack the dependencies)

# You may change those
HOST=i686-w64-mingw32
BUILDDIR=_build-cross-mingw
GLIB_ZIP="http://win32builder.gnome.org/packages/3.6/glib-dev_2.34.3-1_win32.zip"
ICONV_ZIP="http://win32builder.gnome.org/packages/3.6/libiconv-dev_1.13.1-1_win32.zip"
GETTEXT_ZIP="http://win32builder.gnome.org/packages/3.6/gettext-dev_0.18.2.1-1_win32.zip"

# USAGE: fetch_and_unzip URL DEST_PREFIX
fetch_and_unzip()
{
  local basename=${1##*/}
  curl -# "$1" > "$basename"
  unzip -q "$basename" -d "$2"
  rm -f "$basename"
}

if test -d "$BUILDDIR"; then
  cat >&2 <<EOF
** Directory "$BUILDDIR/" already exists.
If it was created by this tool and just want to build, simply run make:
  $ make -C "$BUILDDIR/"

If however you want to recreate it, please remove it first:
  $ rm -rf "$BUILDDIR/"
EOF
  exit 1
fi

set -e
set -x


test -f configure
# check if the host tools are available, because configure falls back
# on default non-prefixed tools if they are missing, and it can spit
# quite a lot of subtle errors.  also avoids going further if something
# is obviously missing.
type "$HOST-gcc"

mkdir "$BUILDDIR"
cd "$BUILDDIR"

mkdir _deps
fetch_and_unzip "$GLIB_ZIP" _deps
fetch_and_unzip "$ICONV_ZIP" _deps
fetch_and_unzip "$GETTEXT_ZIP" _deps
# fixup the prefix= in the pkg-config files
sed -i "s%^\(prefix=\).*$%\1$PWD/_deps%" _deps/lib/pkgconfig/*.pc

export PKG_CONFIG_PATH="$PWD/_deps/lib/pkgconfig/"
export CPPFLAGS="-I$PWD/_deps/include"
export LDFLAGS="-L$PWD/_deps/lib"

INSTALL_SUBDIR="ctpl-$(../configure --version | sed 's/.* configure \(.*\)$/\1/;q')_win32"

../configure \
  --host=$HOST \
  --enable-nls \
  --disable-silent-rules \
  --prefix="$PWD/$INSTALL_SUBDIR"
make -j3
make install

zip -r "$INSTALL_SUBDIR".zip "$INSTALL_SUBDIR"

cat << EOF
Build finished successfully!

The resulting installation can be found at "$BUILDDIR/$INSTALL_SUBDIR",
and a ZIP archive was created for it at "$BUILDDIR/$INSTALL_SUBDIR.zip"

When done, you can remove the build directory altogether if you don't
want to re-use it without rebuilding it:
  $ rm -rf "$BUILDDIR/"
EOF
