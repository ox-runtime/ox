# Release process

Releases are built by [`.github/workflows/release.yml`](https://github.com/ox-runtime/ox/blob/main/.github/workflows/release.yml). The workflow makes the platform packages, runs the unit tests, and attaches the files to a GitHub release.

## When it runs

The workflow runs in two cases:

- **A published GitHub release**: the version comes from the tag with the leading `v` removed. Tag `v0.7.0` becomes version `0.7.0`.
- **A manual `workflow_dispatch`**: the version is `0.0.0-<run-number>`. This is for test builds. It does not upload assets, because the upload step only runs for a published release.

## Build targets

Four jobs run in parallel:

| Job | Runner | Architecture | Build folder |
|---|---|---|---|
| Windows x64 | `windows-latest` | x86_64 | `build/win-x64` |
| Linux x64 | `ubuntu-24.04` | x86_64 | `build/linux-x64` |
| macOS arm64 | `macos-latest` | arm64 | `build/macos-arm64` |
| macOS x64 | `macos-latest` | x86_64 | `build/macos-x64` |

The macOS jobs set `CMAKE_OSX_DEPLOYMENT_TARGET` to `11.0` for arm64 and `10.15` for x64. The lower value keeps the Intel build usable on older machines.

## Tests before packaging

Each job builds the host and both test targets:

```bash
cmake --build build/... --config Release --target ox runtime_tests ipc_proxy_tests
```

It then runs:

- `runtime_tests`, the OpenXR runtime suite with a mock driver.
- `ipc_proxy_tests`, the IPC client and server suite.

A failure stops the job before packaging. The matrix sets `fail-fast: false`, so the other platforms still finish and report. No package is made for the failing platform. A failing test on one platform blocks that platform's release files.

## Tools installed by the workflow

- **Windows**: the Vulkan SDK (headers and loader) through `humbletim/setup-vulkan-sdk`, plus NSIS for the installer.
- **Linux**: Mesa GL headers, PNG, Vulkan, X11 and GLFW headers, `pkg-config`, Flatpak and `flatpak-builder`, snapd and snapcraft, `squashfs-tools`, and `appimagetool`.
- **All platforms**: Python 3.12 and [`easyinstaller`](https://github.com/cmdr2/easyinstaller) v0.4.0.

The workflow copies `build/bin` into `dist/stage/<asset-basename>` and deletes the `tests` folder before packaging. Test binaries stay out of the user-facing archives.

## Package formats

`easyinstaller` makes the per-platform files:

| Platform | Formats |
|---|---|
| Windows | Zip and an NSIS installer (`-setup`). |
| Linux | Zip, AppImage, Flatpak, and Snap. |
| macOS | `.app` inside a DMG, signed and notarized. |

The asset names are `ox-windows-x64`, `ox-linux-x64`, `ox-macos-arm64`, and `ox-macos-x64`. The Windows installer is `<base>-setup`. The app name is `ox`. The executable is `ox.exe` on Windows and `ox` elsewhere.

## macOS signing and notarization

The macOS jobs need six repository secrets:

- `MAC_CERTIFICATE_P12_BASE64`
- `MAC_CERTIFICATE_PASSWORD`
- `MAC_NOTARY_APPLE_ID`
- `MAC_NOTARY_TEAM_ID`
- `MAC_NOTARY_TEAM_NAME`
- `MAC_NOTARY_PASSWORD`

A check step fails early if any are missing. The certificate step decodes the P12, re-exports it with older PBE algorithms, makes a temporary keychain, imports the certificate, and sets the keychain partition list so `codesign` can use it without a prompt.

The `easyinstaller` call then uses `--type app-in-dmg`, `--mac-notarize`, and the notary credentials to sign and notarize the app before putting it in the DMG.

## Uploading the files

On a published release, the last step runs:

```bash
gh release upload "${{ github.event.release.tag_name }}" "${files[@]}" --clobber
```

`GH_TOKEN` is set to the workflow token, and the workflow has `contents: write` permission. It uploads every file in `dist/packages/<asset-basename>`. `--clobber` replaces an existing file with the same name, so re-running a release build is safe.

## macOS Metal check

[`macos-metal-test.yml`](https://github.com/ox-runtime/ox/blob/main/.github/workflows/macos-metal-test.yml) is a separate manual workflow for the Metal backend. It builds ox in Release, runs `runtime_tests`, downloads the example driver, and runs the pyopenxr smoke examples.

It is not part of the release matrix. Use it as an extra check when you change Metal or macOS code. Its smoke-test steps still refer to the old `ox-service` executable name, so update those steps if you rely on it.

## Related pages

- [Building](building.md)
- [Development workflow](development-workflow.md)
- [Release workflow](https://github.com/ox-runtime/ox/blob/main/.github/workflows/release.yml)
