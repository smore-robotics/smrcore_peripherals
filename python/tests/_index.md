> DIR: python/tests/

# Overview

Python 包导入与 DTO 语义单测（pytest）。

# Contents

- test_import.py: 包导入、native 扩展加载、C++ SDK 链接/版本信息
- test_data.py: `PeripheralOptions`、`FtSensorOptions`、`SpaceMouseOptions` 与各 Sample DTO 字段/构造

运行:

```bash
./scripts/build_py.sh
./scripts/run_test_py.sh
./scripts/run_test_py.sh tests/test_import.py
```
