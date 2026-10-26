# 本机订阅自动生成版本说明

本机 Codex 使用 ChatGPT 登录，定时读取 GitHub 默认分支的更新，生成双语草稿并推送至独立的 `automation/release-notes` 分支。
GitHub 不保存 ChatGPT 登录凭据，也不使用 API Key。任务消耗账号订阅内的 Codex 用量，仍受订阅用量限制。

## 链路

每 10 分钟检查远端 → 有新 SHA 才分析 → 比较上一已发布正式 Release → Codex 只读生成 → 校验提交依据 → 归档草稿。
草稿在 [automation/release-notes 分支](https://github.com/smore-robotics/smrcore_peripherals/tree/automation/release-notes)，其 README 链接最新结果。
不修改源码分支、不提 PR、不自动发布未经审核的说明；原有编译、网站和正式发布继续运行。
该归档分支不触发源码编译。GitHub API 版 Codex 工作流改为仅手动运行。

## 本机配置

先 `codex login` 并用 ChatGPT 登录；`codex login status` 必须显示 ChatGPT。
本机还需要 `gh auth login` 的仓库读写权限、Git、Python 3.10+ 和 systemd 用户会话。

```bash
python3 scripts/install_local_codex_notes.py
systemctl --user start peripherals-codex-notes.service
```

脚本使用独立目录 `~/.local/share/smrcore-peripherals-notes/`，不切换开发工作区分支、不搬运认证文件。
禁用 API 环境变量并忽略用户模型配置，避免意外切到 API 计费；任务开始先确认 ChatGPT 登录。
Codex 在只读沙箱内读取代码，不执行构建或机器人程序。工作流控制脚本负责校验和推送，模型不拥有推送步骤。

## 查看与停止

```bash
systemctl --user list-timers peripherals-codex-notes.timer
journalctl --user -u peripherals-codex-notes.service -n 30 --no-pager
cat ~/.local/share/smrcore-peripherals-notes/status.json
# 立即检查（失败后的等待期可用 --force 跳过）
python3 scripts/local_codex_notes.py --force
# 停止后续定时检查
systemctl --user disable --now peripherals-codex-notes.timer
```

失败后至少间隔一小时重试；成功的 SHA 不重复调用模型。每个 SHA 的草稿保留本地生成日志，推送失败可复用已生成内容。
本机必须开机联网；关机不运行，恢复后补查最新状态。若需退出登录后保持任务，可执行 `loginctl enable-linger "$USER"`（可能需要管理员授权）。
登录失效时在本机重新 `codex login`，不要把 auth.json 上传 GitHub。

生成文案仍是草稿；核实后再纳入正式版本说明。接口行为和实机验收不能仅由模型判断。
