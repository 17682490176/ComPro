# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

```bash
mingw32-make          # windres → compro_res.o → ComPro.exe
mingw32-make clean    # remove .exe and .o
mingw32-make run      # build + launch
```

Compiler: MinGW-w64 GCC 16.1.0 (WinGet POSIX UCRT variant). Resource file `compro.rc` embeds `compro.ico` and `compro.manifest` (Common Controls v6 for visual styles). Flags: `-municode -O2 -Wall -flto -s -mwindows`. Libs: `-lcomdlg32 -lcomctl32 -lgdi32 -lwinmm -lws2_32 -liphlpapi -ldwmapi`.

## Architecture

Single-file Win32 C application (`main.c`, ~160KB). No framework — pure Win32 API + Common Controls.

### Dual protocol mode

`g_mode`: 0 = RTU (serial COM port), 1 = TCP (Winsock). Toggled by radio buttons. TCP uses MBAP header (6 bytes, transaction ID in `g_mbap_tid`).

### COM port (RTU mode)

- Overlapped I/O throughout (`FILE_FLAG_OVERLAPPED`)
- Async open via background thread → `WM_OPEN_RESULT`
- Read thread (`rd_thread`) with `WaitForSingleObject` timeout
- `g_pauseRd` flag: pauses reads during scan (prevents data race)
- `CancelIoEx` in scan path after setting `g_pauseRd`
- Port enumeration: `CreateFileW` probe on COM1–32
- Path format: `L"\\\\.\\COM%d"`
- Coalesce timeout: 3.5 char times for RTU (calculated from baud+parity+stop), fixed 30ms for TCP

### TCP mode

- `g_sock` for persistent connection, `g_scanEvent` for scan response coordination
- `COALESCE_RX` timer (ID 3003) assembles TCP fragments into complete Modbus frames
- Ping scan: `IcmpSendEcho`-based IP sweep, configurable concurrency

### Scan engine

Dual-mode scan (RTU serial + TCP ping/probe). RTU scan sends user-defined hex template, replaces byte[0] with slave address, auto-appends CRC16. Result display with red/green color coding for offline/online. TCP scan method selector (`g_scanMethod`) with adjustable thread count.

### Register table (十进表格)

- ListView in report mode: 地址 | 名称 | 数值 columns
- `g_reg[]` array, max 256 entries (`MAX_REG`)
- `parse_modbus()` preserves custom names across refreshes (matched by slave+addr)
- Name column inline editing: double-click → edit control, Enter commits, Esc discards
- Custom draw alternating row colors
- Format/byte-order clipboard (`g_copyFmt`, `g_copyBo`) for copying cell values

### Snapshots (快照)

6 independent snapshot slots (`SNAP_COUNT=6`). Each stores slave address, function code, register range, per-register format/byte-order/name, plus a custom display name. Long-press (800ms) triggers rename. Separate snapshot panel with 6 buttons + rename UI.

### UI conventions

- Fixed window 920×695, no resize (WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX, no maximize button)
- Colors: BG warm white `RGB(248,248,244)`, RX blue `RGB(0,0,255)`, TX green `RGB(0,140,0)`, SYS gray `RGB(130,130,130)`, SCAN purple-blue `RGB(0,0,200)`
- Dual fonts: Segoe UI 15pt for UI controls, Consolas 16pt for hex data
- Dark title bar via DWM API
- `CHARFORMAT2 g_LogFont` enforced on every RichEdit insert to prevent auto font switching
- RichEdit display capped at ~60K chars (trims oldest lines)
- Timestamp format: `<HH:MM:SS.ms>`

### Key globals

`g_run` — reader thread run flag; `g_scan` — scan in progress; `g_pauseRd` — suspend reader during scan; `g_mode` — 0=RTU, 1=TCP; `g_lastSA/FC/AD/QN` — last Modbus request params; `g_snap[]` — snapshot state array.

### Thread safety

`InterlockedExchange` / `InterlockedIncrement` for flags shared with background threads. Reader thread paused (`g_pauseRd` + `CancelIoEx`) before scan thread touches COM port. All UI updates posted via `PostMessage`/`SendMessage` to main thread.
