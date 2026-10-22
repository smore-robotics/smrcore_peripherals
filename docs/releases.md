# 制品与配套 SDK

## 下载和运行

正式制品在 [GitHub Releases](https://github.com/smore-robotics/smrcore_peripherals/releases)；普通提交和 PR 制品在对应 Actions 运行的 Artifacts 中保留 14 天。开发包名带提交短哈希，wheel 的开发版本为 0.0.0，不能与正式版本混淆。

当前流水线目标为 Ubuntu 22.04 / Linux x86_64，生成 C++ 读取包、原 SpaceMouse/F/T SDK bridge 包、CPython 3.10/3.11/3.12 wheel，以及中英文 PDF/HTML 说明书。无 Windows、ARM64 或模拟器发布任务。基础 wheel 不包含机器人 SDK。

下载正式制品后执行 `sha256sum -c SHA256SUMS` 校验完整下载集。C++ 包内 `build-info.json` 记录源码提交、版本及配套 SDK；使用 `./run.sh <程序名> [参数]` 加载包内动态库。主机仍需提供 glibc/libstdc++ 等系统库和设备访问权限。

## SDK 兼容性

- `.sdk-version` 固定原 SpaceMouse/F/T bridge 的 SDK 版本，当前 0.2.0。
- 发布流水线只下载精确的官方 `v<version>` 资源，不回退 prerelease。
- Pico 双臂示例内置 B3 支架变换，只适用于对应 B3 双臂安装构型。
- 原官方 v0.2.0 库中已有双臂符号，但缺少公开头文件。SDK 修复提交 `0852bae` 补上安装清单；该修复正式发布并验证前，Pico 双臂不进入默认预编译制品。
- SDK 当前的双臂 Initialize 接口没有 domain/interface 参数；连接柜子的兼容性需单独验证。
- 更新 SDK：修改 `.sdk-version` → 验证原 bridge 与标定工具 → 验证官方包双臂头文件及链接 → 更新兼容性说明。不要把尚未发布的版本写入默认下载配置。
- 启用 Pico 发布任务时，还需验证实机停止、断流恢复、夹爪及安装变换，不能只凭头文件存在就宣称支持。

## 自动构建与发布

每次 push/PR 和手动触发都会运行读数包、wheel、SDK bridge 和说明书任务。PR 无 Release 写权限，只有仓库内 `vX.Y.Z` 标签 push 且全部任务通过，才发布 Release、自动提交摘要及 SHA256SUMS。

正式发布前确保该提交已审阅，并设置合适的 tag 保护规则。标签版本同时传入 C++ 和 wheel；SDK 版本独立记录。Release 失败时先检查 Actions 日志；同名 Release 已存在时不会覆盖已有资产。

## 本地生成

```bash
./scripts/build.sh --with-sdk OFF --tests ON
python3 scripts/package_release.py --install build_Release/install --version 0.1.0 --output dist
./scripts/build_py.sh --no-install

# 需要 pandoc、XeLaTeX 和 Noto Sans CJK SC 字体
./scripts/build_manual.sh dist 0.1.0
```

SDK bridge 包需先用 `--with-sdk ON --with-pico OFF` 构建，然后为打包脚本增加 `--sdk-root third_party/smrcore_sdk`。使用独立干净构建目录/安装树，避免旧的可选程序混入。

打包脚本不执行机器人程序。SDK runtime 包保留 SDK 的 share/归属文件；发布 wheel 和二进制不代表实机验收完成。
