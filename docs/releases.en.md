# Assets and Robot SDK Compatibility

## Download and run

Tagged builds are published on [GitHub Releases](https://github.com/smore-robotics/smrcore_peripherals/releases). Push/PR artifacts are retained in Actions for 14 days. Development archive names include a commit hash; development wheels use version 0.0.0.

The pipeline targets Ubuntu 22.04 / Linux x86_64: C++ readers, original SpaceMouse/F/T SDK bridges, CPython 3.10/3.11/3.12 wheels, and Chinese/English PDF and HTML manuals. There are no Windows, ARM64 or simulator release jobs. Reader wheels do not contain the robot SDK.

Verify a complete release download with `sha256sum -c SHA256SUMS`. C++ archives contain `build-info.json` with source commit, version and SDK version. Run applications using `./run.sh <binary> [arguments]` to resolve bundled libraries. System glibc/libstdc++ and device permissions remain host requirements.

## SDK compatibility

`.sdk-version` pins 0.2.0 for the original bridges. Release CI downloads that exact official tag, without prerelease fallback.

Pico dual-arm teleoperation embeds B3 mounting-bracket transforms for the corresponding B3 configuration. The original official v0.2.0 binary exports dual-arm symbols but omits its public header. SDK fix `0852bae` repairs the install list. Until a corrected official package is published and validated, default binary releases exclude Pico dual-arm teleoperation. Its current Initialize API has no domain/interface arguments.

To update the SDK, change `.sdk-version`, validate bridges and calibration tools, check dual-arm header/link compatibility, and update documentation. Do not pin an unpublished release. Hardware stopping, stream recovery, grippers and mounting transforms require separate acceptance before enabling Pico release binaries.

## Build and publish

Every push, PR and manual run builds artifacts. PRs cannot publish releases. Only a repository `vX.Y.Z` tag push with all jobs passing creates a Release with generated commit notes and SHA256SUMS. Protect release tags and review the tagged commit first.

The tag supplies the C++/wheel version; the SDK version is independent. Existing releases/assets are not overwritten. Inspect Actions logs if publishing fails.

## Local packaging

```bash
./scripts/build.sh --with-sdk OFF --tests ON
python3 scripts/package_release.py --install build_Release/install --version 0.1.0 --output dist
./scripts/build_py.sh --no-install

# Requires pandoc, XeLaTeX, and Noto Sans CJK SC
./scripts/build_manual.sh dist 0.1.0
```

For SDK bridge archives, first build with `--with-sdk ON --with-pico OFF`, then add `--sdk-root third_party/smrcore_sdk` to the packager. Use clean build/install trees to avoid stale optional programs. Packaging does not run robot applications and retains SDK attribution files.
