# Codex 自动版本说明

当前默认采用[本机 ChatGPT 订阅自动流](local-codex-notes.md)。本文的 API 方式保留为 Actions 手动运行 **Codex release notes**，不再在 push 时自动调用 API。
Codex 读取上一已发布正式 Release 到本次提交的 diff 和相关源码，生成中英文说明草稿。
比较范围固定到提交 SHA；未发布成功的 tag 不作为基线。

## 一次性配置

在当前 fork 的 Settings → Secrets and variables → Actions 中新增 Secret：

- `OPENAI_API_KEY`：OpenAI API Key，只填在 GitHub Secret 中。
- 可选 Repository Variable `RELEASE_CODEX_MODEL`：指定账号可用的 Codex 模型；不填使用官方 Action 默认模型。

[打开 Secrets 设置](https://github.com/zhanhui-li-smore/smrcore_peripherals/settings/secrets/actions)。
这里使用 API Key 调用模型，不复制本机 Codex 登录凭据。API 使用由该 Key 所属项目计费；可在对应项目设置用量限制。
配置完成后 Actions → Codex release notes → Run workflow，选择默认分支即可验证。

## 结果在哪里

Actions 运行完成后下载 `codex-release-notes-<完整提交 SHA>`：

- `release-notes-zh.md` / `release-notes-en.md`：按外设链路和功能分类的草稿。
- `draft.json`：成功生成时保留的结构化内容。
- `context.json`：基线版本、目标 SHA、SDK 版本、提交范围和生成状态。

没有 Key 时状态为 `missing-key`；没有新提交为 `no-changes`。
模型失败或证据校验不通过时，该工作流报告失败，同时保留带状态的 Git 摘要，不能当作 Codex 已成功生成。
草稿保留 90 天；该工作流不自动提交、不提 PR、不覆盖旧 Release，也不修改机器人代码。
网站、PDF 和 Release 共用版本说明入口，读取已审核的摘要；未审核草稿保留在独立归档分支。

## 说明质量与发布

Codex 合并相关变更，说明接口、使用方式和兼容影响，每条带该范围内的提交依据；不把编译配置当成实机验收。
审核后将摘要、基线 SHA、目标 SHA 和覆盖提交列表写入 `docs/reviewed-release-notes.json`。构建从目标版本的 Git 树读取记录，验证覆盖范围并替代对应提交标题；后续提交继续使用 `Release-note-zh` / `Release-note-en` 或标题。正式发版按 tag 固定内容，旧版本不受影响。

工作流仅接受默认分支的手动触发；不对外部 PR 暴露 Key。Codex 无仓库写权限、采用只读沙箱和 `drop-sudo`；验证与打包在另一个干净 runner 进行。
模型只能输出结构化草稿，输出不会作为 shell 脚本执行。该校验确认格式及提交范围，不代替语义审核。

接入依据：[官方 Codex GitHub Action 文档](https://developers.openai.com/codex/github-action)。
