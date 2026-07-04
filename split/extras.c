/* Auto-generated from main.c — see compro.h for declarations */
#include "compro.h"

LRESULT CALLBACK snap_btn_proc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR idSubclass, DWORD_PTR refData) {
    SnapBtnCtx *ctx = (SnapBtnCtx*)refData;
    switch (m) {
    case WM_LBUTTONDOWN:
        SetCapture(h);
        ctx->longPressed = 0;
        SetTimer(h, SNAP_TIMER_ID, SNAP_LONG_PRESS, NULL);
        return 0;
    case WM_MOUSEMOVE:
        if (GetCapture() == h) {
            RECT rc; GetClientRect(h, &rc);
            POINT pt = { LOWORD(l), HIWORD(l) };
            if (pt.x < -20 || pt.x > rc.right + 20 || pt.y < -20 || pt.y > rc.bottom + 20) {
                KillTimer(h, SNAP_TIMER_ID);
                ReleaseCapture();
            }
        }
        return 0;
    case WM_LBUTTONUP:
        KillTimer(h, SNAP_TIMER_ID);
        if (GetCapture() == h) {
            ReleaseCapture();
            RECT rc; GetClientRect(h, &rc);
            POINT pt = { LOWORD(l), HIWORD(l) };
            if (pt.x >= 0 && pt.x < rc.right && pt.y >= 0 && pt.y < rc.bottom) {
                if (ctx->snap->active) {
                    /* clear name_set for registers in snap range so snap names apply */
                    for (int i = 0; i < g_nr; i++)
                        if (g_reg[i].slave == ctx->snap->slave &&
                            g_reg[i].addr >= ctx->snap->addr &&
                            g_reg[i].addr < ctx->snap->addr + ctx->snap->qty)
                            g_reg[i].name_set = 0;
                    g_bSyncing = 1;
                    SetWindowTextW(g_hSend, ctx->snap->send_cmd);
                    { WCHAR d[16]; wsprintfW(d, L"%d", ctx->snap->slave);
                      SetWindowTextW(g_hSlaveAddr, d); }
                    { WCHAR d[16]; wsprintfW(d, L"%d", ctx->snap->addr);
                      SetWindowTextW(g_hRegAddr, d); }
                    g_bSyncing = 0;
                    sync_val_from_send();
                    UpdateWindow(g_hSend);
                    if (SendMessageW(GetDlgItem(g_hWnd, ID_TIMER_CHK), BM_GETCHECK, 0, 0) != BST_CHECKED) {
                        do_send(FALSE);
                    }
                } else if (g_nr > 0) {
                    snap_capture(ctx->snap, ctx->btn);
                }
            }
        }
        return 0;
    case WM_TIMER:
        if (w == SNAP_TIMER_ID) {
            KillTimer(h, SNAP_TIMER_ID);
            ctx->longPressed = 1;
            ReleaseCapture();
            if (!ctx->snap->active) return 0;
            RECT rc; GetWindowRect(h, &rc);
            HWND parent = GetParent(h);
            MapWindowPoints(NULL, parent, (POINT*)&rc, 2);
            int ww = rc.right - rc.left;
            if (ww < 64) ww = 64;
            HWND edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", ctx->snap->sname,
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_BORDER,
                rc.left, rc.top, ww, rc.bottom - rc.top + 2,
                parent, NULL, (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE), NULL);
            SendMessageW(edit, WM_SETFONT, (WPARAM)g_hMono, TRUE);
            SendMessageW(edit, EM_SETSEL, 0, -1);
            g_hRenameEdit = edit;
            SetWindowSubclass(edit, rename_edit_proc, 0, (DWORD_PTR)ctx);
            SetFocus(edit);
        }
        return 0;
    case WM_RBUTTONUP: {
        HMENU menu = CreatePopupMenu();
        AppendMenuW(menu, MF_STRING, 1, L"暂存此页");
        AppendMenuW(menu, MF_STRING | (ctx->snap->active ? 0 : MF_GRAYED), 2, L"清除暂存");
        AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(menu, MF_STRING | (ctx->snap->active ? 0 : MF_GRAYED), 3, L"保存到注册表");
        POINT pt; GetCursorPos(&pt);
        int cmd = (int)TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, GetParent(h), NULL);
        DestroyMenu(menu);
        if (cmd == 1) {
            if (g_nr > 0) snap_capture(ctx->snap, ctx->btn);
        } else if (cmd == 2) {
            snap_clear(ctx->snap, ctx->btn);
        } else if (cmd == 3) {
            ctx->snap->saved_to_reg = 1;
            snap_save_reg(ctx->snap, ctx->idx);
        }
        return 0;
    }
    case WM_DESTROY:
        KillTimer(h, SNAP_TIMER_ID);
        RemoveWindowSubclass(h, snap_btn_proc, idSubclass);
        return 0;
    }
    return DefSubclassProc(h, m, w, l);
}

LRESULT CALLBACK rename_edit_proc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR idSubclass, DWORD_PTR refData) {
    SnapBtnCtx *ctx = (SnapBtnCtx*)refData;
    if (m == WM_KEYDOWN && w == VK_RETURN) {
        GetWindowTextW(h, ctx->snap->sname, 32);
        update_snap_text(ctx->snap, ctx->btn);
        RemoveWindowSubclass(h, rename_edit_proc, idSubclass);
        g_hRenameEdit = NULL; DestroyWindow(h);
        return 0;
    }
    if (m == WM_KEYDOWN && w == VK_ESCAPE) {
        RemoveWindowSubclass(h, rename_edit_proc, idSubclass);
        g_hRenameEdit = NULL; DestroyWindow(h);
        return 0;
    }
    if (m == WM_KILLFOCUS) {
        GetWindowTextW(h, ctx->snap->sname, 32);
        update_snap_text(ctx->snap, ctx->btn);
        RemoveWindowSubclass(h, rename_edit_proc, idSubclass);

#define REG_KEY L"Software\\ComPro"

void reg_set_str(const WCHAR *name, const WCHAR *val) {
    HKEY hk;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, NULL, 0,
        KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS) {
        DWORD cb = (DWORD)((wcslen(val) + 1) * sizeof(WCHAR));
        RegSetValueExW(hk, name, 0, REG_SZ, (BYTE*)val, cb);
        RegCloseKey(hk);
    }
}

int reg_get_str(const WCHAR *name, WCHAR *val, int n, const WCHAR *def) {
    DWORD cb = (DWORD)(n * sizeof(WCHAR));
    LONG r = RegGetValueW(HKEY_CURRENT_USER, REG_KEY, name,
        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, NULL, val, &cb);
    if (r == ERROR_SUCCESS) return 1;
    if (def) wcscpy(val, def);
    return 0;
}

void snap_clear_reg(int idx) {
    WCHAR key[64];
    static const WCHAR *keys[] = { L"Active", L"Slave", L"Fc", L"Addr", L"Qty",
        L"Sname", L"SendCmd", L"Fmts", L"Bos", L"Names", L"SavedToReg" };
    HKEY hk;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, KEY_SET_VALUE, &hk) == ERROR_SUCCESS) {
        for (int i = 0; i < 11; i++) {
            wsprintfW(key, L"Snap%d_%s", idx, keys[i]);
            RegDeleteValueW(hk, key);
        }
        RegCloseKey(hk);
    }
}

void snap_save_reg(SnapState *snap, int idx) {
    WCHAR key[64], val[32];
    wsprintfW(key, L"Snap%d_Active", idx);
    wsprintfW(val, L"%d", snap->active);
    reg_set_str(key, val);
    wsprintfW(key, L"Snap%d_SavedToReg", idx);
    wsprintfW(val, L"%d", snap->saved_to_reg);
    reg_set_str(key, val);
    if (!snap->active) return;
    wsprintfW(key, L"Snap%d_Slave", idx); wsprintfW(val, L"%d", snap->slave); reg_set_str(key, val);
    wsprintfW(key, L"Snap%d_Fc", idx); wsprintfW(val, L"%d", snap->fc); reg_set_str(key, val);
    wsprintfW(key, L"Snap%d_Addr", idx); wsprintfW(val, L"%d", snap->addr); reg_set_str(key, val);
    wsprintfW(key, L"Snap%d_Qty", idx); wsprintfW(val, L"%d", snap->qty); reg_set_str(key, val);
    wsprintfW(key, L"Snap%d_Sname", idx); reg_set_str(key, snap->sname);
    wsprintfW(key, L"Snap%d_SendCmd", idx); reg_set_str(key, snap->send_cmd);
    WCHAR fmt_str[1024] = {0}, bo_str[1024] = {0}, names[8700] = {0};
    for (int i = 0; i < snap->qty && i < 256; i++) {
        WCHAR d[8]; wsprintfW(d, L"%d ", snap->fmt[i]); wcscat(fmt_str, d);
        wsprintfW(d, L"%d ", snap->bo[i]); wcscat(bo_str, d);
        wcscat(names, snap->name[i]);
        if (i < snap->qty - 1) wcscat(names, L"|");
    }
    wsprintfW(key, L"Snap%d_Fmts", idx); reg_set_str(key, fmt_str);
    wsprintfW(key, L"Snap%d_Bos", idx); reg_set_str(key, bo_str);
    wsprintfW(key, L"Snap%d_Names", idx); reg_set_str(key, names);
}

void snap_load_reg(SnapState *snap, int idx, HWND btn) {
    WCHAR key[64], val[8700] = {0};
    wsprintfW(key, L"Snap%d_Active", idx);
    if (!reg_get_str(key, val, 32, L"0") || _wtoi(val) == 0) {
        snap->active = 0; snap->sname[0] = 0;
        update_snap_text(snap, btn);
        return;
    }
    snap->active = 1;
    wsprintfW(key, L"Snap%d_SavedToReg", idx);
    if (reg_get_str(key, val, 32, L"0")) snap->saved_to_reg = _wtoi(val);
    wsprintfW(key, L"Snap%d_Slave", idx); reg_get_str(key, val, 32, L"1"); snap->slave = _wtoi(val);
    wsprintfW(key, L"Snap%d_Fc", idx); reg_get_str(key, val, 32, L"3"); snap->fc = _wtoi(val);
    wsprintfW(key, L"Snap%d_Addr", idx); reg_get_str(key, val, 32, L"0"); snap->addr = _wtoi(val);
    wsprintfW(key, L"Snap%d_Qty", idx); reg_get_str(key, val, 32, L"0"); snap->qty = _wtoi(val);
    wsprintfW(key, L"Snap%d_Sname", idx); reg_get_str(key, snap->sname, 32, L"");
    wsprintfW(key, L"Snap%d_SendCmd", idx); reg_get_str(key, snap->send_cmd, 4096, L"");
    int nfmt = snap->qty, nbo = snap->qty, nname = snap->qty;
    wsprintfW(key, L"Snap%d_Fmts", idx);
    if (reg_get_str(key, val, 1024, L"")) {
        WCHAR *p = val; int i = 0;
        while (*p && i < 256) {
            while (*p == L' ') p++;
            if (!*p) break;
            snap->fmt[i] = _wtoi(p);
            while (*p && *p != L' ') p++;
            i++;
        }
        nfmt = i;
    }
    wsprintfW(key, L"Snap%d_Bos", idx);
    if (reg_get_str(key, val, 1024, L"")) {
        WCHAR *p = val; int i = 0;
        while (*p && i < 256) {
            while (*p == L' ') p++;
            if (!*p) break;
            snap->bo[i] = _wtoi(p);
            while (*p && *p != L' ') p++;
            i++;
        }
        nbo = i;
    }
    wsprintfW(key, L"Snap%d_Names", idx);
    if (reg_get_str(key, val, 8700, L"")) {
        WCHAR *p = val; int i = 0;
        while (*p && i < 256) {
            WCHAR *s = p;
            while (*p && *p != L'|') p++;
            int len = (int)(p - s);
            if (len > 31) len = 31;
            wcsncpy(snap->name[i], s, len);
            snap->name[i][len] = 0;
            i++;
            if (*p == L'|') p++;
        }
        nname = i;
    }
    { int m = snap->qty; if (nfmt < m) m = nfmt; if (nbo < m) m = nbo; if (nname < m) m = nname; snap->qty = m; }
    update_snap_text(snap, btn);
}

void save_settings(void) {
    WCHAR val[4096];
    #define WKEY(k,id) do { \
        if (id >= ID_BAUD_COMBO && id <= ID_STOP_COMBO) { \
            int ix = (int)SendMessageW(GetDlgItem(g_hWnd,id), CB_GETCURSEL, 0, 0); \
            wsprintfW(val, L"%d", ix); \
        } else if (id == ID_RTU_RADIO || id == ID_TCP_RADIO) { \
            wsprintfW(val, L"%d", (g_mode == 1) ? 1 : 0); \
        } else { \
            GetWindowTextW(GetDlgItem(g_hWnd, id), val, 4096); \
        } \
        reg_set_str(k, val); \
    } while(0)
    #define WCK(k,id) do { \
        int ck = (int)SendMessageW(GetDlgItem(g_hWnd,id), BM_GETCHECK, 0, 0); \
        wsprintfW(val, L"%d", ck == BST_CHECKED ? 1 : 0); \
        reg_set_str(k, val); \
    } while(0)
    WKEY(L"BaudRate", ID_BAUD_COMBO);
    WKEY(L"DataBits", ID_DATA_COMBO);
    WKEY(L"Parity", ID_PARITY_COMBO);
    WKEY(L"StopBits", ID_STOP_COMBO);
    WCK(L"Modbus", ID_MODBUS_CHK);
    WCK(L"AutoWrap", ID_TIME_CHK);
    WCK(L"TimeDisp", ID_TIME_DISP_CHK);
    WCK(L"DecTable", ID_DECTABLE_CHK);
    WKEY(L"TimerMs", ID_TIMER_MS);
    WKEY(L"SlaveAddr", ID_SLAVEADDR_DEC);
    WKEY(L"RegAddr", ID_REGADDR_DEC);
    WKEY(L"SendText", ID_SEND_INPUT);
    WKEY(L"TcpIp", ID_TCP_IP);
    WKEY(L"TcpPort", ID_TCP_PORT);
    WKEY(L"Mode", ID_RTU_RADIO);
    WKEY(L"ScanStart", ID_SCAN_START);
    WKEY(L"ScanEnd", ID_SCAN_END);
    WKEY(L"ScanIval", ID_SCAN_IVAL);
    WKEY(L"ScanTout", ID_SCAN_TOUT);
    WKEY(L"ScanThreads", ID_SCAN_THREADS);
    { wsprintfW(val, L"%d", g_scanMethod); reg_set_str(L"ScanMethod", val); }
    { wsprintfW(val, L"%d", g_bitEdAutoSend); reg_set_str(L"BitEdAutoSend", val); }
    #undef WKEY
    #undef WCK
    for (int i = 0; i < SNAP_COUNT; i++) {
        if (g_snap[i].saved_to_reg) snap_save_reg(&g_snap[i], i+1); else snap_clear_reg(i+1);
    }
}

void load_settings(void) {
    WCHAR val[4096];
    #define RKEY(k,id,def) do { \
        reg_get_str(k, val, 4096, def); \
        if (id >= ID_BAUD_COMBO && id <= ID_STOP_COMBO) { \
            int ix = _wtoi(val); \
            if (ix >= 0 && ix < (int)SendMessageW(GetDlgItem(g_hWnd,id), CB_GETCOUNT, 0, 0)) \
                SendMessageW(GetDlgItem(g_hWnd,id), CB_SETCURSEL, ix, 0); \
        } else { \
            SetWindowTextW(GetDlgItem(g_hWnd, id), val); \
        } \
    } while(0)
    #define RCK(k,id,def) do { \
        reg_get_str(k, val, 4096, def); \
        SendMessageW(GetDlgItem(g_hWnd,id), BM_SETCHECK, _wtoi(val) ? BST_CHECKED : BST_UNCHECKED, 0); \
    } while(0)
    RKEY(L"BaudRate", ID_BAUD_COMBO, L"4");
    RKEY(L"DataBits", ID_DATA_COMBO, L"0");
    RKEY(L"Parity", ID_PARITY_COMBO, L"0");
    RKEY(L"StopBits", ID_STOP_COMBO, L"0");
    RCK(L"Modbus", ID_MODBUS_CHK, L"1");
    RCK(L"AutoWrap", ID_TIME_CHK, L"1");
    RCK(L"TimeDisp", ID_TIME_DISP_CHK, L"1");
    RCK(L"DecTable", ID_DECTABLE_CHK, L"1");
    RKEY(L"TimerMs", ID_TIMER_MS, L"1000");
    RKEY(L"SlaveAddr", ID_SLAVEADDR_DEC, L"1");
    RKEY(L"RegAddr", ID_REGADDR_DEC, L"0");
    RKEY(L"SendText", ID_SEND_INPUT, L"01 03 00 00 00 01");
    RKEY(L"TcpIp", ID_TCP_IP, L"192.168.1.1");
    RKEY(L"TcpPort", ID_TCP_PORT, L"502");
    { reg_get_str(L"Mode", val, 4096, L"0");
      if (_wtoi(val) == 1) { g_mode = 1; CheckRadioButton(g_hWnd, ID_RTU_RADIO, ID_TCP_RADIO, ID_TCP_RADIO); } }
    RKEY(L"ScanStart", ID_SCAN_START, L"1");
    RKEY(L"ScanEnd", ID_SCAN_END, L"254");
    RKEY(L"ScanIval", ID_SCAN_IVAL, L"800");
    RKEY(L"ScanTout", ID_SCAN_TOUT, L"500");
    RKEY(L"ScanThreads", ID_SCAN_THREADS, L"16");
    { reg_get_str(L"ScanMethod", val, 4096, L"0"); int m = _wtoi(val); if (m < 0 || m > 1) m = 0; g_scanMethod = m; SendMessageW(g_hScanMethod, CB_SETCURSEL, m, 0); }
    { reg_get_str(L"BitEdAutoSend", val, 4096, L"1"); g_bitEdAutoSend = _wtoi(val) ? 1 : 0; }
    #undef RKEY
    #undef RCK
}

LRESULT CALLBACK about_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH br = CreateSolidBrush(RGB(252,252,248));
        FillRect(dc, &rc, br); DeleteObject(br);
        HFONT titleFont = CreateFontW(22, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
        HFONT bodyFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
        HFONT oldF = (HFONT)SelectObject(dc, titleFont);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(30,60,120));
        RECT tr = {20, 16, rc.right - 20, 56};
        DrawTextW(dc, L"ComPro " VERSION L" — Modbus RTU/TCP 调试工具", -1, &tr, DT_LEFT | DT_VCENTER);
        SelectObject(dc, bodyFont);
        SetTextColor(dc, RGB(50,50,50));
        const WCHAR *lines[] = {
            L"▸ Modbus RTU 串口通信（COM1–32，波特率 300–921600）",
            L"▸ Modbus TCP 网络通信（Socket，默认端口 502）",
            L"▸ TCP 自动重连 + RTU 串口热插拔检测 + 拔出自动关闭",
            L"▸ 串口即插即用：检测到新串口自动打开（空闲时）",
            L"▸ 十六进制发送 / 接收，实时日志与全链路诊断计时",
            L"▸ 十进表格：自动解析寄存器，支持 I16/U16/HEX/BIN 单寄存器",
            L"    及 I32/U32/F32 ×4字节序 双寄存器、I64/U64/F64 四寄存器",
            L"▸ 双击表格地址：自动识别 32-bit 格式（遍历 I32/U32/F32",
            L"    ×4字节序，过滤 NaN/Inf/亚正常数，选最接近 1.0 的值）",
            L"▸ 二进制位编辑器：Ctrl 组合选择、位字段解码与修改",
            L"▸ 6 组暂存按钮：短按恢复并发送，长按重命名，右键保存/加载",
            L"▸ 定时循环发送 + 发送框 HEX 格式校验（非法红底提示）",
            L"▸ 从站地址扫描 + 多线程 ICMP Ping + Modbus 命令扫描",
            L"▸ 扫描方式切换（Ping / Modbus）、写命令保护弹窗确认",
            L"▸ 双击扫描结果 IP 切换连接/断开，双击地址填入从站",
            L"▸ 参数通过注册表保存/恢复，6 组暂存可持久化",
            L"▸ Enter 智能判断：未连→连接，已连→发送",
            L"▸ Ctrl+C/V 复制/粘贴寄存器格式到其他行",
            NULL
        };
        int sy = 64;
        for (int i = 0; lines[i]; i++) {
            RECT lr = {24, sy, rc.right - 24, sy + 26};
            DrawTextW(dc, lines[i], -1, &lr, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            sy += 28;
        }
        sy += 8;
        SetTextColor(dc, RGB(130,130,130));
        RECT vr = {24, sy, rc.right - 24, sy + 22};
        DrawTextW(dc, L"版本 " VERSION L" | 纯 C Win32 + GCC/MinGW-w64 | 2026", -1, &vr, DT_LEFT | DT_VCENTER);
        SelectObject(dc, oldF);
        DeleteObject(titleFont); DeleteObject(bodyFont);
        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_KEYDOWN && w == VK_ESCAPE) { DestroyWindow(h); return 0; }
    if (m == WM_LBUTTONDOWN) { DestroyWindow(h); return 0; }
    if (m == WM_DESTROY) return 0;
    return DefWindowProcW(h, m, w, l);
}

void show_about(HWND parent) {
    WNDCLASSEXW wc = {sizeof(wc)};
    wc.lpfnWndProc = about_proc;
    wc.hInstance = (HINSTANCE)GetWindowLongPtrW(parent, GWLP_HINSTANCE);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ComProAbout";
    RegisterClassExW(&wc);
    int ww = 600, wh = 620;
    RECT pr; GetWindowRect(parent, &pr);
    int px = pr.left + ((pr.right - pr.left) - ww) / 2;
    int py = pr.top + ((pr.bottom - pr.top) - wh) / 2;
    HWND h = CreateWindowExW(WS_EX_TOPMOST, L"ComProAbout", L"关于 ComPro",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        px, py, ww, wh, parent, NULL, wc.hInstance, NULL);
    if (!h) return;
    ShowWindow(h, SW_SHOW);
    UpdateWindow(h);
    /* modal loop */
    MSG msg;
    while (IsWindow(h)) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        WaitMessage();
    }
}

