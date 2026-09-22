# libasyncns for Devario

Seafoam Labs hosts libasyncns to provide a stable HTTPS source location for
building and distributing **Devario**. Devario's package infrastructure needs
reproducible access to dependencies even when the original upstream Git hosting
is unavailable or has certificate problems.

This is a distribution-maintained source mirror of the work of Lennart
Poettering and contributors. Seafoam Labs does not claim upstream ownership or
represent this snapshot as a new upstream release.

## Current source snapshot

The source on `main` matches the latest upstream commit we could verify:

```text
68cd5aff1467638c086f1bedcc750e34917168e4
```

- Upstream commit date: **30 October 2009**.
- Upstream commit subject: **use O_CLOEXEC if available**.
- Package version: **0.8+r3+g68cd5af**, three commits after upstream v0.8.
- The [upstream branch](https://github.com/Seafoam-Labs/libasyncns/tree/upstream)
  preserves that original commit and its Git history.
- The `v0.8-r3` tag identifies the unmodified snapshot. This tag was created by
  Seafoam Labs, not signed by upstream.
- `main` adds this README, the Devario PKGBUILD, and its `.SRCINFO` metadata.

The history was recovered from the
[Cor0n4V1rus mirror](https://github.com/Cor0n4V1rus/libasyncns), whose preserved
upstream master matches the full commit pinned by
[Arch's libasyncns package](https://gitlab.archlinux.org/archlinux/packaging/packages/libasyncns).
The mirror's later README-only commit is not included. No newer upstream code
commit has been verified.

The original project remains at
<https://0pointer.net/lennart/projects/libasyncns/>. Its Git hosting is currently
unusable over HTTPS because of a hostname certificate mismatch; this repository
lets Devario retrieve the verified source with TLS verification enabled.

## What libasyncns does

libasyncns is a small C library that performs hostname and DNS lookups
asynchronously using the system's name-resolution functions. In Arch-derived
package stacks, the PulseAudio client library, `libpulse`, depends on it. Those
client libraries can also communicate with PipeWire's PulseAudio compatibility
service.

## Building a Devario package

The root [PKGBUILD](PKGBUILD) builds **1:0.8+r3+g68cd5af-4** for x86_64. It
retrieves an archive of the original upstream commit from this repository and
verifies SHA-256:

```text
69597d5a2791f857f1660888d60cf5bf59284c972a4f318bba064752b0641436
```

The commit is pinned independently of changes to the packaging branch. This
snapshot matches Arch's source revision; package release `4` sorts above Arch's
`1:0.8+r3+g68cd5af-3` and our previous `1:0.8-1` release package.

Use an Arch-compatible `base-devel` environment with `glibc` and `lynx` installed:

```sh
makepkg
```

Shelly can review and build the same recipe:

```sh
shelly build --review-only --json ./PKGBUILD
shelly build --sync-deps ./PKGBUILD
```

Unlike the release tarball, this Git snapshot has no generated `configure`
script. The recipe runs `autoreconf -fi` during preparation and uses Lynx to
regenerate the upstream plain-text documentation. The `check()` step runs
upstream's `make check`, which compiles its example test program but does not
execute a runtime test suite. The example makes public DNS queries and is not
run automatically.

## Original release import

The repository began with the unmodified **0.8 release tarball**, published on
15 October 2009. The existing `v0.8` tag still identifies that initial import;
it is a Seafoam Labs import tag, not the original upstream Git tag. It remains
available for reproducibility but is no longer the PKGBUILD's source.

- Original URL: <http://0pointer.de/lennart/projects/libasyncns/libasyncns-0.8.tar.gz>
- Downloaded over HTTPS: <https://0pointer.de/lennart/projects/libasyncns/libasyncns-0.8.tar.gz>
- Original archive SHA-256:

  ```text
  4f1a66e746cbe54ff3c2fbada5843df4fbbbe7481d80be003e8d11161935ab74
  ```

This checksum applies only to the original release archive, not to the Git
snapshot archive used by the current PKGBUILD.

## License and attribution

libasyncns is copyright its upstream authors and is distributed under the
**GNU Lesser General Public License, version 2.1 or later**. The original
[LICENSE](LICENSE), source copyright notices, and upstream documentation are
preserved. Seafoam Labs' role here is to host the source for Devario.
