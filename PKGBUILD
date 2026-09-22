# Maintainer: Seafoam Labs
# Latest verified upstream Git snapshot, preserved for Devario.

pkgname=libasyncns
pkgver=0.8+r3+g68cd5af
pkgrel=6
epoch=1
pkgdesc='A C library for executing name service queries asynchronously'
arch=('x86_64')
url='https://github.com/Seafoam-Labs/libasyncns'
license=('LGPL-2.1-or-later')
depends=('glibc')
makedepends=('git' 'lynx')

# Fetch only from Seafoam Labs. Use a distinct cache name so an old
# libasyncns source cache from 0pointer is not reused.
_commit=68cd5aff1467638c086f1bedcc750e34917168e4
source=("seafoam-libasyncns::git+https://github.com/Seafoam-Labs/libasyncns.git#commit=$_commit")
sha256sums=('SKIP')

prepare() {
  cd "$srcdir/seafoam-libasyncns"
  # Git snapshots do not include the release's generated configure script.
  autoreconf -fi
}

build() {
  cd "$srcdir/seafoam-libasyncns"
  ./configure \
    --prefix=/usr \
    --sysconfdir=/etc \
    --localstatedir=/var \
    --disable-static
  make
}

check() {
  cd "$srcdir/seafoam-libasyncns"
  # Upstream's check target builds its example test program; it does not
  # register automated runtime tests. The example itself uses public DNS.
  make check
}

package() {
  cd "$srcdir/seafoam-libasyncns"
  make DESTDIR="$pkgdir" install
  # Lynx emits absolute build-directory links; keep installed docs relocatable.
  sed -i 's,file://[^[:space:]]*/doc/README.html,README.html,g' \
    "$pkgdir/usr/share/doc/$pkgname/README"
  install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
