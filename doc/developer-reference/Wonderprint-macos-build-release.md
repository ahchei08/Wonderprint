# Wonderprint macOS Build and Release

This document describes the Wonderprint macOS build flow on the stable `wonder2_3_1` release line.

The repository already supports `arm64`, `x86_64`, and `universal` builds through [`build_release_macos.sh`](../../build_release_macos.sh). The workflow in [`.github/workflows/wonderprint-macos.yml`](../../.github/workflows/wonderprint-macos.yml) packages that existing build output into a DMG, uploads it as a workflow artifact, and can optionally sign, notarize, and publish it to a GitHub Release.

## Workflow behavior

The macOS workflow is manual by design:

- `publish_release=false`, `sign_and_notarize=false`
  - Builds the universal app.
  - Produces `Wonderprint-Orca_Mac_universal_V<version>_unsigned.dmg`.
  - Uploads the DMG as a workflow artifact.
- `publish_release=true`, `sign_and_notarize=false`
  - Does the same unsigned build.
  - Also creates or updates a GitHub Release in the current repository and uploads the unsigned DMG.
- `publish_release=true`, `sign_and_notarize=true`
  - Builds the universal app.
  - Signs and notarizes the app and DMG.
  - Produces `Wonderprint-Orca_Mac_universal_V<version>.dmg`.
  - Uploads the DMG as both an artifact and a release asset.

`release_tag` defaults to `v<version from version.inc>` when left blank. `release_name` defaults to `Wonderprint-Orca <version>`.

## Required secrets for signing and notarization

Unsigned artifact builds do not require Apple credentials.

Signed and notarized builds require:

- `MACOS_CERTIFICATE_P12_BASE64`
- `MACOS_CERTIFICATE_PASSWORD`
- `APPLE_ID`
- `APPLE_APP_SPECIFIC_PASSWORD`
- `APPLE_TEAM_ID`

The workflow also accepts the existing legacy secret names for certificate migration:

- `WONDERMAKERDIST`
- `MAC_P12_CERT_PASSWORD`

## Manual local flow

Use the same stable branch and build flags as the workflow:

```shell
./build_release_macos.sh -dx -a universal -t 10.15 -1
./build_release_macos.sh -s -x -a universal -t 10.15 -1
./scripts/package_macos_dmg.sh \
  ./build/universal/Wonderprint-Orca/Wonderprint-Orca.app \
  ./build/universal/Wonderprint-Orca_Mac_universal_V2.3.1_unsigned.dmg \
  Wonderprint-Orca
```

For a signed local build, export the signing environment variables listed above and run:

```shell
./scripts/sign_notarize_macos.sh \
  ./build/universal/Wonderprint-Orca/Wonderprint-Orca.app \
  ./build/universal/Wonderprint-Orca_Mac_universal_V2.3.1.dmg \
  Wonderprint-Orca
```

## Notes

- The current release line for this flow is `wonder2_3_1`.
- The public macOS asset is the main Wonderprint app DMG. The profile validator helper is not published as a separate macOS release artifact.
- GitHub only exposes `workflow_dispatch` workflows from the repository default branch in the Actions UI. If you want to trigger this workflow directly from the UI on this release line, set the repository default branch to `wonder2_3_1`.
