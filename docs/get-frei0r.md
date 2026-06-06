# Get frei0r

frei0r is distributed as source code, release archives and packages maintained
by operating-system communities. After installing it, open a compatible
[application or framework](/software) to use the plugins.

## Releases

The project publishes source archives and platform artifacts on
[GitHub Releases](https://github.com/dyne/frei0r/releases).

- **Windows:** release entries include Win64 plugin archives when the release
  build succeeds.
- **macOS:** release entries include macOS plugin archives; Homebrew is usually
  the simpler installation route.
- **GNU/Linux:** distributions package frei0r, and releases include build
  artifacts intended primarily for packaging and testing.

Release assets are produced automatically. Check the selected release entry for
the exact files it contains rather than relying on a fixed filename.

## Package managers

Package versions can lag behind upstream releases. The
[Repology overview](https://repology.org/project/frei0r/versions) compares
packages across many distributions.

### GNU/Linux

Search your distribution for `frei0r` or `frei0r-plugins`. Install both the
runtime plugins and development headers when compiling a host or a new plugin.

### macOS

Install the [Homebrew formula](https://formulae.brew.sh/formula/frei0r):

```sh
brew install frei0r
```

### BSD

- [FreeBSD port](https://www.freshports.org/graphics/frei0r/)
- [NetBSD pkgsrc](https://pkgsrc.se/multimedia/frei0r)
- OpenBSD users can search the ports/packages collection for `frei0r-plugins`.

## Build from source

You need a C compiler, a C++11 compiler, CMake, and Ninja or Make. Clone the
repository and use an out-of-tree build:

```sh
git clone https://github.com/dyne/frei0r.git
cd frei0r
cmake -S . -B build -G Ninja
cmake --build build
```

Install using CMake's install step, with administrator privileges only when the
chosen prefix requires them:

```sh
cmake --install build
```

The authoritative maintained instructions are in
[`BUILD.md`](https://github.com/dyne/frei0r/blob/master/BUILD.md).

## Optional plugin dependencies

The build detects optional libraries that enable additional effects:

| Library | Used by |
| --- | --- |
| OpenCV | Face-detection and related filters |
| Cairo | Cairo-based filters and mixers |
| Gavl | Filters such as `scale0tilt` and `vectorscope` |

Disable unavailable groups explicitly when making a minimal build:

```sh
cmake -S . -B build -G Ninja \
  -DWITHOUT_OPENCV=ON \
  -DWITHOUT_CAIRO=ON \
  -DWITHOUT_GAVL=ON
cmake --build build
```

Applications using MLT may need face-recognition plugins disabled to avoid
protobuf conflicts:

```sh
cmake -S . -B build -DWITHOUT_FACERECOGNITION=ON
cmake --build build
```

## Test a source build

The repository includes runtime tools that load and exercise built plugins:

```sh
cd test
make frei0r-asan
make check
make frei0r-meta
make scan-meta
```

The test Makefile expects plugins under the repository's `build/src` tree.

## What to do next

- [See software that can host the plugins](/software)
- [Use frei0r with FFmpeg and other hosts](/using-frei0r)
- [Write and contribute a plugin](/develop-plugin)
- [Ask installation questions on Telegram](https://t.me/frei0r)
