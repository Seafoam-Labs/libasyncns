# Maintainer: Seafoam Labs
# Upstream 0.8 release hosted for Devario; not Arch's later Git snapshot.

pkgname=libasyncns
pkgver=0.8
pkgrel=1
epoch=1
pkgdesc='A C library for executing name service queries asynchronously'
arch=('x86_64')
url='https://github.com/Seafoam-Labs/libasyncns'
license=('LGPL-2.1-or-later')
depends=('glibc')

# Pin the unmodified release import, independently of packaging changes.
_commit=12c27ddd14b96086975112cdf5c287a4f7ff1293
source=("$pkgname-$pkgver-$_commit.tar.gz::$url/archive/$_commit.tar.gz")
sha256sums=('403284facfd7cdc6ca5a60fc5f9b96cd8b318cb61e76630ca21c18da3aa5ef8d')

build() {
  cd "$pkgname-$_commit"
  # The release includes configure and pre-generated documentation.
  ./configure \
    --prefix=/usr \
    --sysconfdir=/etc \
    --localstatedir=/var \
    --disable-static \
    --disable-lynx
  make
}

check() {
  cd "$pkgname-$_commit"
  # Upstream's check target builds its example test program; it does not
  # register automated runtime tests. The example itself uses public DNS.
  make check
}

package() {
  cd "$pkgname-$_commit"
  make DESTDIR="$pkgdir" install
  install -Dm644 LICENSE "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}
