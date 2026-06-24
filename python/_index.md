> DIR: python/

# Overview

pybind11 + scikit-build-core Python wheel，distribution 名 `rcore-peripherals-py`，import 包名 `rcore_peripherals`，链接已安装的 `libsmrcore_peripherals`。构建入口 `../scripts/build_py.sh`，测试入口 `../scripts/run_test_py.sh`。Python >= 3.10。

# Contents

- pyproject.toml / CMakeLists.txt: scikit-build wheel 工程定义
- native/bindings.cpp: pybind11 绑定（`*Options` dict 构造、`FtSensor`/`SpaceMouse` 全生命周期 API）
- src/rcore_peripherals/: Python 包
  - `__init__.py`: 导出公开类型与外设类；`.libs/` 内嵌 native `.so` 并配置 `LD_LIBRARY_PATH`
  - `data.py`: DTO/`Options` Python dataclass 或等价封装
  - `ft_sensor.py` / `spacemouse.py`: 薄封装，对齐 C++ 门面
  - `_version.py`: 包版本
- app/: 读数示例脚本，见 `app/_index.md`
- tests/: 导入与 DTO 单测，见 `tests/_index.md`
- scripts/build_wheel.sh: wheel 打包（由 `../scripts/build_py.sh` 调用；要求已设置 `SMRCORE_PERIPHERALS_CPP_ROOT`）
- scripts/verify_wheel.sh: 安装 wheel 冒烟导入（由 `../scripts/run_test_py.sh` 调用）

> FLOW: C++ install → wheel 打包 native + Python → `import rcore_peripherals` → `FtSensor(options)` 同 C++ 语义
