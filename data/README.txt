【中文字体安装说明】
本程序已启用全量中文字库支持，但由于字体文件过大（约2-5MB），无法直接集成在代码中。
请按照以下步骤自行安装字体：

1. 获取字体文件 (.vlw 格式)
   - 方法 A（推荐）：在网上搜索下载 "simkai.vlw" 或 "msyh.vlw" (微软雅黑) 等 Processing 字体文件。
   - 方法 B（自制）：下载 Processing 软件 -> Tools -> Create Font -> 选择中文字体 -> 勾选 "All Characters" -> 创建。

2. 重命名与放置
   - 将下载或生成的字体文件重命名为 "font.vlw"。
   - 将该文件放入本项目根目录下的 "data" 文件夹中。

3. 上传文件系统
   - 在 VSCode/PlatformIO 中，运行 "Upload Filesystem Image" 任务。
   - 这将把 data 文件夹中的内容写入开发板的存储区。

注意：如果没有上传字体文件，屏幕将自动回退到英文显示模式。
