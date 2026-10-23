# 下载与版本

## 去哪里下载

- [正式 Releases](https://github.com/smore-robotics/smrcore_peripherals/releases)：推送版本标签并通过检查后发布。
- [开发构建 Actions](https://github.com/smore-robotics/smrcore_peripherals/actions)：普通提交的构建包在对应运行页面底部 Artifacts，需登录 GitHub，保留 14 天。

如果使用个人 Fork，请打开该 Fork 的 Actions / Releases；个人分支构建不会出现在组织仓库的发布页。旧版本仅有 Source code 附件时，表示该版本没有上传编译包。

| 产物 | 内容 |
| --- | --- |
| `cpp-linux-x86_64` 压缩包 | C++ 外设库、头文件、读取器 |
| `sdk-bridge-linux-x86_64` 压缩包 | 原 SpaceMouse/F/T bridge、标定工具、SDK 运行库；不含 Pico 双臂遥操 |
| `rcore_peripherals_py-*.whl` | Python 外设读取库，选择对应 cp310 / cp311 / cp312 |
| `manual-zh` / `manual-en` PDF、HTML | 构建时生成的中英文说明书 |
| `SHA256SUMS` | 正式发布的文件校验和 |

## 使用预编译包

解压 C++ 包，进入解压目录，通过 wrapper 加载包内动态库：

```bash
./run.sh app_peripherals_read_pico --port 8765
```

在下载 wheel 所在目录安装与当前 Python 匹配的文件：

```bash
python -m pip install ./rcore_peripherals_py-*.whl
```

该目录只放一个匹配的 wheel。若需要 Python 示例脚本，还需获取源码仓库的 `python/app/`。

## SDK 版本与站点版本

升级 bridge 时同步检查 `.sdk-version`、头文件与运行库，不能只替换单个头文件。

本网站跟随部署分支更新，描述开发状态；使用旧版制品时优先参考该版本随附说明书。当前发布平台为 Linux x86_64，不承诺 Windows 或 ARM64 二进制。
