# 维护者发布流程

## 版本与范围

面向用户的发布说明以 `docs/releases.md` / `docs/releases.en.md` 为真源，网站下载页直接引用，Release 正文由 `scripts/release_notes.py` 生成。更新功能、制品范围或 SDK 时同步修改两种语言；不要只给新链路追加例外段落。

制品文件名保持兼容。四条外设链路统一列出，但明确区分预编译程序、Python 示例、只读库和依赖项。不得把源码示例写成已提供预编译制品。

## 自动化

- 每次 push / PR：构建和验证，上传 Actions 制品，保留 14 天。
- `vX.Y.Z` tag：构建全部制品与说明书；通过后生成校验和和 Release 正文，附加 GitHub 提交摘要并发布。
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
