> DIR: .github/workflows/

- ci.yml: push/PR/manual 构建 Ubuntu 22.04 C++ 读取/SDK bridge、3 个 Python ABI wheel 及双语说明书；标签且全部成功后发布 Release。PR 只读权限，release 单任务写权限。
- release-smoke.yml: 手动验证既有版本制品/SDK。
