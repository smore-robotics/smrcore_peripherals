# 维护者发布流程

## 版本与范围

面向用户的发布说明以 `docs/releases.md` / `docs/releases.en.md` 为真源，网站下载页直接引用，Release 正文由 `scripts/release_notes.py` 生成。更新功能、制品范围或 SDK 时同步修改两种语言；不要只给新链路追加例外段落。

制品文件名保持兼容。四条外设链路统一列出，但明确区分预编译程序、Python 示例、只读库和依赖项。不得把源码示例写成已提供预编译制品。

## 自动化

- 每次 push / PR：构建和验证，上传 Actions 制品，保留 14 天。
- `vX.Y.Z` tag：构建全部制品与说明书；通过后生成校验和和 Release 正文，发布经审核的用户版本说明。
- 每周一北京时间 09:17：检查默认分支；有新提交递增补丁版本，无更新且已发布则跳过。同提交未发布的 tag 复用重试，不移动 tag。
- 手动检查：Actions → Build and Release → Run workflow，在默认分支勾选 `weekly_release`。不勾选只构建开发制品。

当前发布目标是个人 fork `zhanhui-li-smore/smrcore_peripherals`，默认分支 `feat/pico-readers-and-release`；不自动发布到官方仓库，不创建 PR。上游接收后由其维护者决定发布分支。

定时任务可能延迟；GitHub 可能暂停长期无活动的公共仓库定时任务。自动打 tag 后由同一次工作流继续发布，不依赖 GITHUB_TOKEN 的 push 递归触发。

## SDK 更新

修改 `.sdk-version` → 验证下载包头文件与运行库 → 编译对应应用 → 更新两种语言的支持表 → 验证实机适用范围。Pico 进入预编译制品前应验证双臂链接、构型映射、停止、断流和夹爪行为。SDK 版本与外设版本独立，不把未发布的 SDK 写入默认下载配置。

## 本地检查与故障恢复

```bash
./scripts/build.sh --with-sdk OFF --tests ON
./scripts/run_tests.sh -t Release
./scripts/build_py.sh --no-install
./scripts/build_manual.sh dist preview
python3 scripts/release_notes.py --tag vX.Y.Z --repository owner/repo --output /tmp/release-notes.md
```

最后一条的 vX.Y.Z 应替换为实际版本。使用干净安装树打包；wheel 与压缩包统一暂存于 dist 后上传。缺失制品时修复原因，不能跳过文件校验后发布不完整版本。

失败先查看各 job。只有发布步骤失败、构建已通过时，可核对原 tag、提交、版本和校验和后用原始制品补发；不要重打或移动已有 tag。已发布资产不覆盖，正文纠错不得声称旧版包含后续代码修改。

## 自动版本记录

`version_history.py` 从目标提交可达的正式 `vX.Y.Z` tag 计算累计范围，未打 tag 的提交显示为“未发布”。按变更路径归类并保留提交链接；原始标题作为缺少发布文案时的回退，不推断代码行为。合并提交不重复列出，其包含的普通提交仍收录。版本说明按 Git tag 标记，不代表该 tag 的制品已发布成功；下载状态以 Releases 页为准。

要得到面向用户的双语条目，可以在同一提交消息正文增加：

```text
Release-note-zh: Pico 支持读取左右手柄姿态。
Release-note-en: Pico can read left and right controller poses.
```

网站构建、PDF/HTML 手册、正式 Release 均使用同一个 Git 汇总器。网站在 push 和默认分支构建成功后更新，因此每周自动创建 tag 后也会刷新版本说明；不由 CI 写回源代码。网站生成文件是构建产物。本地运行 MkDocs 时，构建 hook 自动生成版本页；也可单独执行 `python3 scripts/version_history.py`。需要完整 Git 历史与 tag，浅克隆应执行 `git fetch --unshallow --tags`。

## Codex 草稿

本机订阅定时任务自动检查默认分支，生成基于源码差异的双语草稿，独立于正式发布流水线。配置与归档位置见[本机 Codex 自动流](local-codex-notes.md)；API 方式仅保留手动触发。

## 用户发布基线

`docs/release-baseline.json` 汇总 v0.1.0 交付能力，网站、说明书和 Release 共用。此前开发日志不再展开；旧 tag 和制品保留在 GitHub。
基线后的条目仅收录已审核摘要或 `Release-note-zh` / `Release-note-en`，内部维护提交不自动显示。提交功能变更时必须提供两种语言的条目，或在发版前审核归档草稿并写入 `docs/reviewed-release-notes.json`。
C++ 包只打包用户文档与示例；本目录、AI 索引和模板源码不进入安装包。
