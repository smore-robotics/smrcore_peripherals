# Getting Started

Choose C++ or Python to read peripheral inputs. Robot control is a separate integration step.

<div class="grid cards" markdown>

- :material-language-cpp:{ .lg .middle } **C++ SDK**

    ---

    Build the library and diagnostic tools.

    [C++ SDK →](#c-sdk)

- :material-language-python:{ .lg .middle } **Python SDK**

    ---

    Install the reader wheel and run examples.

    [Python SDK →](#python-sdk)

</div>

## Requirements

Prebuilt packages target Linux x86_64, with CPython 3.10–3.12 wheels. Ubuntu 22.04 is recommended.

```bash
sudo apt-get update
sudo apt-get install -y git build-essential cmake ninja-build python3-venv python3-dev
python3 -m venv .venv-peripherals
source .venv-peripherals/bin/activate
python -m pip install 'conan>=2,<3'
git clone https://github.com/smore-robotics/smrcore_peripherals.git
cd smrcore_peripherals
```

## C++ SDK {#c-sdk}

```bash
./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release
./build_Release/bin/app_peripherals_probe
```

## Python SDK {#python-sdk}

After the C++ build, use the same environment:

```bash
./scripts/build_py.sh --no-install
python -m pip install --force-reinstall python/dist/*.whl
python -c "import rcore_peripherals; print('import OK')"
```

The wheel contains peripheral readers, not the robot SDK or a teleoperation core. See [downloads](downloads.md) for prebuilt packages and [readers](readers.md) for examples.
