#!/bin/sh -e

test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.
(
  cd "$srcdir" &&
  autoreconf --warnings=all --force --install
) || exit
test -n "$NOCONFIGURE" || "$srcdir/configure" "$@"
