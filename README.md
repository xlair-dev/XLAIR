# XLAIR

XLAIR is a rhythm game built with [Siv3D](https://siv3d.github.io/) v0.6.16.

## Supported platforms

| Target       | Siv3D source                               | Build environment                       |
| ------------ | ------------------------------------------ | --------------------------------------- |
| Linux x86_64 | Preinstalled in the XLAIR container image  | Docker / devcontainer                   |
| Windows x64  | Official SDK and v0.6.16 runtime resources | Visual Studio Build Tools, clang, Ninja |
| macOS x86_64 | Official SDK                               | Xcode Command Line Tools, Ninja         |

CMake 3.22 or newer is required.

The Siv3D v0.6.16 macOS SDK targets x86_64. Builds run through Rosetta on Apple Silicon Macs.

## Development

### Linux

Open the repository in its devcontainer, then configure and build XLAIR inside the container:

```bash
cmake --preset linux-debug
cmake --build --preset build-linux-debug
./build/linux-debug/App/XLAIR/XLAIR
```

The same environment can be started without a devcontainer:

```bash
docker compose up -d
docker compose exec xlair bash
```

The default image is `ghcr.io/xlair-dev/siv3d-docker-base:latest`. Set `SIV3D_IMAGE` to use a pinned tag or digest:

```bash
SIV3D_IMAGE=ghcr.io/xlair-dev/siv3d-docker-base:<tag-or-digest> docker compose up -d
```

### Windows

Run the following commands from a Visual Studio Developer PowerShell:

```powershell
cmake --preset windows-debug
cmake --build --preset build-windows-debug
./build/windows-debug/App/XLAIR/XLAIR.exe
```

### macOS

```bash
cmake --preset macos-debug
cmake --build --preset build-macos-debug
open build/macos-debug/App/XLAIR/XLAIR.app
```

The macOS bundle identifier is `dev.xlair.XLAIR`.

### Release builds

Replace `debug` with `release` in the configure and build preset names. For example:

```bash
cmake --preset macos-release
cmake --build --preset build-macos-release
```

Build artifacts are written to `build/<preset>/App/<target>/`.

## Project layout

Application targets live under `apps/`. Each application owns its `src/`,
`data/`, and `resources/` directories. Shared libraries should live under
`libs/`, and reusable CMake helpers live under `cmake/`.
