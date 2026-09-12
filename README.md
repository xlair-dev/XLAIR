# XLAIR

XLAIR is a rhythm game built with [Siv3D](https://siv3d.github.io/) v0.6.16.

<!-- TODO: 画像入れる -->
<!-- TODO: 無理に英語よりも日本語にしてしまってもよいかも -->

## Supported platforms

| Target       | Siv3D source                               | Build environment                      |
| ------------ | ------------------------------------------ | -------------------------------------- |
| Linux x86_64 | Preinstalled in the XLAIR container image  | Docker / devcontainer                  |
| Windows x64  | Official SDK and v0.6.16 runtime resources | Visual Studio Build Tools, MSVC, Ninja |
| macOS x86_64 | Official SDK                               | Xcode Command Line Tools, Ninja        |

CMake 3.22 or newer is required.

The Siv3D v0.6.16 macOS SDK targets x86_64. Builds run through Rosetta on Apple Silicon Macs.

## Development

### Linux

Open the repository in its devcontainer, then configure and build the project inside the container:

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

## Testing

SheetsAnalyzer uses the Catch2 version bundled with the Siv3D SDK.
Configure the project, build the test executable, and run it through CTest:

```bash
cmake --preset <platform>-debug
cmake --build --preset build-sheets-analyzer-tests-<platform>-debug
ctest --preset test-sheets-analyzer-<platform>-debug
```

Replace `<platform>` with `linux`, `macos` or `windows` to use the corresponding platform preset.
Tests are also included in the platform-wide `build-<platform>-<config>` presets while `BUILD_TESTING` is enabled.

## Project layout

```
.
├── apps/            # Applications
├── build/           # Build artifacts
├── cmake/           # CMake helpers
└── libs/            # Shared libraries
```
