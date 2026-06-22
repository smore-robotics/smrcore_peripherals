# Dependencies

The public examples require released artifacts only:

- `smrcore_sdk`: robot SDK C++ package.
- `smrcore_peripherals`: peripheral reader/bridge C++ package.

The examples do not require rcore source code, FastDDSGen, IDL generation, or
the internal peripheral reader source tree.

`scripts/download.sh` extracts artifacts into:

- `third_party/smrcore_sdk`
- `third_party/smrcore_peripherals`

`scripts/build.sh` passes those prefixes through `CMAKE_PREFIX_PATH`.
