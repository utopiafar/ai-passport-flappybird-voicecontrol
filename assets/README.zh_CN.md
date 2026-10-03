<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

### 声翼字体

`fonts/NotoSansSC-Flappy-subset.ttf` 是 [Noto Sans SC](https://github.com/google/fonts/tree/main/ofl/notosanssc)
的 199 字符应用子集，从上游可变 TTF 实例化为字重 500；源文件 SHA-256 为
`a3041811a78c361b1de50f953c805e0244951c21c5bd412f7232ef0d899af0da`。
按 SIL Open Font License 分发，许可保存在 `fonts/NotoSansSC-OFL.txt`，子集字体家族名为
Noto Sans SC Flappy。`fonts/flappy_font_16.c` 是 16 px、4-bpp、未压缩的 LVGL 字形，
直接编译到 `main`。`fonts/flappy_chars.txt` 保存可打印 ASCII 和全部固定中文字符，
`fonts/flappy_glyphs.c` 用于启动覆盖检查。所有标签均显式绑定该字体。

使用 Python/fonttools 4.60.1 和官方 `lv_font_conv` 1.5.3 再生成（脚本锁定版本，需要 Node/npm）：

```bash
python3 tools/generate_flappy_font.py assets/fonts/NotoSansSC-Flappy-subset.ttf
python3 tests/test_flappy_font.py
```

新增子集中没有的字时，改为传入原始授权 TTF；转换前会断言源字形覆盖。
小鸟、景物及 16 kHz / 16-bit 单声道合成音效均为本项目 MIT 许可下原创代码，
未嵌入商业游戏素材或音频录音。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。
