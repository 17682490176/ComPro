# ComPro 更新日志

## v4.1 — 2026-06-21

### UI 全面优化
- 启用 Visual Styles (Common Controls v6 manifest)，控件从 Win95 风格变为现代 Windows 主题
- 双字体体系：UI 控件用 Segoe UI 15pt，十六进制数据区用 Consolas 16pt ClearType
- 深色标题栏 (DWM immersive dark mode, Win10 1809+)
- 所有标签宽度加宽，适配 Segoe UI 中文不再截断换行
- 控件间距优化，标签去冒号，视觉更简洁
- RTU/TCP 单选按钮与串口标签对齐同一基线
- 寄存器表格行色柔化：交替 RGB(242,245,250) / RGB(253,253,252)，不再刺眼
- WM_CTLCOLORSTATIC 改用 OPAQUE 模式修复文字闪/截断问题

### 功能调整
- 去除串口参数自动侦测功能（auto_detect_worker 及相关弹窗）
- 扫描间隔/超时输入框解耦：自由输入，互不联动
- 扫描开始时不自动调整超时时间
- 点击"开始扫描"自动停掉定时发送

### TCP 模式扫描区布局修复
- "起"/"止"标签在 TCP 模式正确右移，不再与"IP扫描"标签重叠
- TCP IP 编辑框位置调整，与"超时ms"保持间距

### 文件
- 新增 `compro.manifest` (Visual Styles)
- 更新 `compro.rc` (嵌入 manifest)
- 更新 `Makefile` (链接 `-ldwmapi`)
- 新增 `win32-ui-style/SKILL.md` (Win32 UI 风格 skill)
