# 手册模板

PDF 使用 [Eisvogel v2.4.0](https://github.com/Wandmalfarbe/pandoc-latex-template/tree/v2.4.0)，固定版本适配现有 Ubuntu 22.04 / Pandoc 2.x 构建。`eisvogel.latex` 来自该版本的 `eisvogel.tex`（仅清理行尾空白）；许可保存在 `EISVOGEL-LICENSE`，并嵌入导出的手册末尾。

`style.yaml` 和 `layout.tex` 是本仓品牌配置：深蓝封面、青色横线、Noto 中文字体、独立目录、章节换页和代码自动折行。`style.css` 是本仓独立 HTML 样式，无在线字体或 CSS 依赖。

运行 `./scripts/build_manual.sh /tmp/peripherals-manual-preview preview` 生成中英文 PDF/HTML。需要 Pandoc、XeLaTeX、texlive-latex-extra、Noto CJK 和 DejaVu 字体。自动发布流水线使用同一脚本。

章节由 `scripts/assemble_manual.py` 汇总现有文档；保留原文，将相对文档链接转换为仓库链接。
