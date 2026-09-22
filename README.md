# libasyncns for Devario

Seafoam Labs hosts this source code to provide a stable HTTPS source location
for building and distributing **Devario**. Devario's package infrastructure
needs reproducible access to its dependencies even when the original upstream
Git hosting is unavailable or has certificate problems.

This repository contains the upstream **libasyncns 0.8 release source** by
Lennart Poettering and contributors. It is a distribution-maintained source
archive, not a claim of upstream ownership or a new upstream release.

## What libasyncns does

libasyncns is a small C library that performs hostname and DNS lookups
asynchronously using the system's name-resolution functions. It is a dependency
of the PulseAudio client library, `libpulse`, in Arch-derived package stacks.
Those client libraries can also be used with PipeWire's PulseAudio compatibility
service.

Hosting this dependency here supports Devario's package builds and source
availability. The upstream library's behavior has not been changed in this
initial import.

## Source provenance

- Upstream project: <https://0pointer.net/lennart/projects/libasyncns/>
- Original release URL: <http://0pointer.de/lennart/projects/libasyncns/libasyncns-0.8.tar.gz>
- Downloaded over HTTPS: <https://0pointer.de/lennart/projects/libasyncns/libasyncns-0.8.tar.gz>
- Release version: **0.8**, published **15 October 2009**.
- Original archive filename: `libasyncns-0.8.tar.gz`.
- Original archive SHA-256:

  ```text
  4f1a66e746cbe54ff3c2fbada5843df4fbbbe7481d80be003e8d11161935ab74
  ```

The first commit imports the extracted release files unchanged. The `v0.8` tag
identifies that import; it is a tag created by Seafoam Labs, not an upstream
signed Git tag. The original upstream Git history is not included. This README
was added in a separate documentation commit, and the upstream `README` remains
available unchanged.

The checksum above applies to the original upstream tarball, not to GitHub's
generated source archives. Package recipes should pin a commit or tag and use
the checksum appropriate to the archive they actually download.

## Version scope for Devario packaging

This snapshot is **0.8**, not Arch's `0.8+r3+g68cd5af` snapshot. It does not include
the three later upstream commits or the Git object
`68cd5aff1467638c086f1bedcc750e34917168e4`. A recipe pinned to that object cannot
use this repository simply by replacing its source URL. Recipes using this
release must use the corresponding version and release-tarball build steps.

The release includes a generated `configure` script, so it does not need the
upstream Git checkout's bootstrap step. Standard build commands are:

```sh
./configure --prefix=/usr --disable-static
make
make DESTDIR="$pkgdir" install
```

Here, `pkgdir` is the package builder's staging directory. See the original
`README` for upstream build documentation.

## License and attribution

libasyncns is copyright its upstream authors and is distributed under the
**GNU Lesser General Public License, version 2.1 or later**. The original
[LICENSE](LICENSE), source copyright notices, and upstream documentation are
preserved. Seafoam Labs' role here is to host the source for Devario.
