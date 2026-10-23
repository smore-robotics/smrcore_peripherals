> DIR: .github/workflows/

- ci.yml: push/PR/manual 构建 Ubuntu 22.04 C++ 读取/SDK bridge、3 个 Python ABI wheel 及双语说明书；标签且全部成功后发布 Release。PR 只读权限，release 单任务写权限。
- release-smoke.yml: 手动验证既有版本制品/SDK。

- docs.yml: 构建 MkDocs 文档，PR 仅构建，main 或指定 Fork 开发分支部署 Pages。

- ci.yml: numpy dependency and Zhonglin Python regression tests.

- ci.yml: includes Python Pico launcher regression tests.

- ci.yml: weekly Monday 01:17 UTC or manual weekly_release; default-branch tag planning and same-run artifact publication, no recursive push dependency.

- ci.yml: stage wheels and archives under dist before upload so merged release artifacts have a flat layout.

- ci.yml: manual job installs texlive-latex-extra for the vendored Eisvogel template.
