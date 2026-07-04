/* Auto-generated from main.c — see compro.h for declarations */
#include "compro.h"

void scan_result_set(const WCHAR *txt) {
    wcsncpy(g_scanResult, txt, 5499);
    g_scanResult[5499] = 0;
    g_srScrollY = 0;
    InvalidateRect(g_hScanRes, NULL, TRUE);
    int len = (int)wcslen(g_scanResult);
    ShowWindow(g_hScanRes, len > 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_CLEAR), len > 0 ? SW_SHOW : SW_HIDE);
}

void update_scan_display(void) {
    LONG fc = InterlockedExchangeAdd(&g_pingFc, 0);
    if (fc <= 0) { scan_result_set(L""); return; }
    WCHAR rt[5500];
    int pos = 0;
    for (LONG i = 0; i < fc && pos < 5490; i++) {
        if (i > 0) { rt[pos++] = L' '; rt[pos++] = L'|'; rt[pos++] = L' '; }
        int n = wsprintfW(rt + pos, L"[%s%d]", g_pingIpBase, g_pingFound[i]);
        pos += n;
    }
    rt[pos] = 0;
    scan_result_set(rt);
}

/* ===== scan ===== */
typedef struct { HANDLE hPort; SOCKET sock; int start, end, ival, tout; BYTE tmpl[2048]; int tlen; int tcp; char ipBase[64]; int method; BYTE sendBuf[2048]; int sendLen; int tcpPort; } SI;

typedef struct { char ipBase[64]; int start, end; DWORD tout; } PingArg;
typedef struct { char ipBase[64]; int start, end; DWORD tout; BYTE sendBuf[2048]; int sendLen; int tcpPort; } ModbusScanArg;

DWORD WINAPI ping_worker(LPVOID p) {
    PingArg *a = (PingArg*)p;

    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) return 0;

    DWORD perEcho = a->tout / 2;
    if (perEcho < 100) perEcho = 100;

    BYTE replyBuf[sizeof(ICMP_ECHO_REPLY) + 8];

    for (;;) {
        LONG idx = InterlockedIncrement(&g_pingNext) - 1;
        int octet = a->start + (int)idx;
        if (octet > a->end || !g_scan || g_scanStop) break;

        char fullIp[72]; snprintf(fullIp, sizeof(fullIp), "%s%d", a->ipBase, octet);
        IPAddr dst = inet_addr(fullIp);
        if (dst == INADDR_NONE) continue;

        int ok = 1;
        DWORD totalRtt = 0;
        for (int i = 0; i < 2; i++) {
            DWORD ret = IcmpSendEcho(hIcmp, dst, NULL, 0, NULL,
                                     replyBuf, sizeof(replyBuf), perEcho);
            if (ret == 0 || ((PICMP_ECHO_REPLY)replyBuf)->Status != IP_SUCCESS) { ok = 0; break; }
            totalRtt += ((PICMP_ECHO_REPLY)replyBuf)->RoundTripTime;
        }
        DWORD avgRtt = ok ? totalRtt / 2 : 0;

        PostMessageW(g_hWnd, WM_PING_RESULT, (WPARAM)octet, (LPARAM)((ok ? 1 : 0) | (avgRtt << 16)));
    }

    IcmpCloseHandle(hIcmp);
    return 0;
}

int build_mbap(BYTE *d, int dl, BYTE *out) {
    int tid = (int)InterlockedIncrement(&g_mbap_tid);
    out[0] = (tid >> 8) & 0xFF; out[1] = tid & 0xFF;
    out[2] = 0; out[3] = 0;
    out[4] = (dl >> 8) & 0xFF; out[5] = dl & 0xFF;
    memcpy(out + 6, d, dl);
    return dl + 6;
}

DWORD WINAPI modbus_scan_worker(LPVOID p) {
    ModbusScanArg *a = (ModbusScanArg*)p;
    DWORD to = a->tout;
    if (to < 200) to = 200;

    for (;;) {
        LONG idx = InterlockedIncrement(&g_pingNext) - 1;
        int octet = a->start + (int)idx;
        if (octet > a->end || !g_scan || g_scanStop) break;

        char fullIp[72]; snprintf(fullIp, sizeof(fullIp), "%s%d", a->ipBase, octet);

        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            PostMessageW(g_hWnd, WM_PING_RESULT, (WPARAM)octet, 0);
            continue;
        }

        struct sockaddr_in addr = {0};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = inet_addr(fullIp);
        addr.sin_port = htons((u_short)a->tcpPort);

        { u_long nb = 1; ioctlsocket(sock, FIONBIO, &nb); }
        connect(sock, (struct sockaddr*)&addr, sizeof(addr));

        int connected = 0;
        { fd_set wfds; struct timeval tv;
          tv.tv_sec = to / 1000; tv.tv_usec = (to % 1000) * 1000;
          FD_ZERO(&wfds); FD_SET(sock, &wfds);
          int r = select(0, NULL, &wfds, NULL, &tv);
          if (r > 0) {
              int err = 0; int len = sizeof(err);
              getsockopt(sock, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
              if (err == 0) connected = 1;
          }
        }

        int ok = 0;
        if (connected) {
            { u_long nb = 0; ioctlsocket(sock, FIONBIO, &nb); }
            setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (char*)&to, sizeof(to));
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (char*)&to, sizeof(to));
            if (send_all(sock, (char*)a->sendBuf, a->sendLen) == a->sendLen) {
                BYTE rsp[256];
                int n = recv(sock, (char*)rsp, sizeof(rsp), 0);
                if (n > 0) ok = 1;
            }
        }
        closesocket(sock);

        PostMessageW(g_hWnd, WM_PING_RESULT, (WPARAM)octet, (LPARAM)(ok ? 1 : 0));
    }
    return 0;
}

void fmt_found_str(WCHAR *buf, int bufSize, int *found, int fc) {
    int pos = 0;
    for (int i = 0; i < fc && pos < bufSize - 1; i++) {
        int n = wsprintfW(buf + pos, i ? L" %d" : L"%d", found[i]);
        if (pos + n >= bufSize - 1) { pos = bufSize - 1; break; }
        pos += n;
    }
    buf[pos] = 0;
}

DWORD WINAPI scan_thread(LPVOID a) {
    SI *s = (SI*)a; int found[254], fc = 0, tcp = s->tcp;
    WCHAR t[32]; ts(t, 32);
    {
        WCHAR *m = malloc(8192);
        if (tcp) {
            WCHAR wb[64]; MultiByteToWideChar(CP_ACP, 0, s->ipBase, -1, wb, 64);
            if (s->method == 1) {
                WCHAR hx[256]; fmt_hex(s->sendBuf, s->sendLen, hx, 256);
                wsprintfW(m, L"%s === Modbus扫描 (%s%d-%s%d) 命令:%s 超时:%dms ===", t, wb, s->start, wb, s->end, hx, s->tout);
            } else {
                wsprintfW(m, L"%s === IP扫描 (%s%d-%s%d) 超时:%dms ===", t, wb, s->start, wb, s->end, s->tout);
            }
        } else {
            WCHAR hx[4096]; fmt_hex(s->tmpl, s->tlen, hx, 4096);
            wsprintfW(m, L"%s === 从站扫描 (%d-%d) 模板:%s 间隔:%dms ===", t, s->start, s->end, hx, s->ival);
        }
        PostMessageW(g_hWnd, WM_LOG_MSG, 0, (LPARAM)m);
    }
    if (tcp) {
        MultiByteToWideChar(CP_ACP, 0, s->ipBase, -1, g_pingIpBase, 64);
        g_pingNext = 0;
        g_pingFc = 0;
        int total = s->end - s->start + 1;

        int nWorkers = (int)g_concWrk;
        if (nWorkers > total) nWorkers = total;
        if (nWorkers < 1) nWorkers = 1;
        if (nWorkers > 64) nWorkers = 64;

        HANDLE *threads = (HANDLE*)calloc(nWorkers, sizeof(HANDLE));
        int nCreated = 0;
        void *scanArg = NULL;

        if (s->method == 1) {
            ModbusScanArg *marg = (ModbusScanArg*)malloc(sizeof(ModbusScanArg));
            if (!marg) { g_scan = 0; g_scanStop = 0; free(threads); free(s); return 0; }
            scanArg = marg;
            memcpy(marg->ipBase, s->ipBase, 64);
            marg->start = s->start;
            marg->end = s->end;
            marg->tout = (DWORD)s->tout;
            marg->tcpPort = s->tcpPort;
            memcpy(marg->sendBuf, s->sendBuf, s->sendLen);
            marg->sendLen = s->sendLen;
            for (int i = 0; i < nWorkers; i++) {
                threads[i] = CreateThread(NULL, 0, modbus_scan_worker, marg, 0, NULL);
                if (threads[i]) nCreated++;
            }
        } else {
            PingArg *arg = (PingArg*)malloc(sizeof(PingArg));
            if (!arg) { g_scan = 0; g_scanStop = 0; free(threads); free(s); return 0; }
            scanArg = arg;
            memcpy(arg->ipBase, s->ipBase, 64);
            arg->start = s->start;
            arg->end = s->end;
            arg->tout = (DWORD)s->tout;
            for (int i = 0; i < nWorkers; i++) {
                threads[i] = CreateThread(NULL, 0, ping_worker, arg, 0, NULL);
                if (threads[i]) nCreated++;
            }
        }

        /* poll progress while workers run */
        while (g_scan && !g_scanStop) {
            LONG done = g_pingNext;
            if (done > total) done = total;
            PostMessageW(g_hWnd, WM_SCAN_PRG, (WPARAM)(s->start + done), s->end);
            if (done >= total) break;
            Sleep(60);
        }

        if (g_scanStop) {
            for (int i = 0; i < 30 && g_pingNext < total; i++) Sleep(50);
        }

        if (nCreated > 0) WaitForMultipleObjects(nCreated, threads, TRUE, 15000);
        for (int i = 0; i < nWorkers; i++) if (threads[i]) CloseHandle(threads[i]);
        free(threads); free(scanArg);

        PostMessageW(g_hWnd, WM_SCAN_DONE, (WPARAM)total, 0);
        g_scan = 0; g_scanStop = 0;
        SetWindowTextW(g_hScanSt, L"");
        SetWindowTextW(GetDlgItem(g_hWnd, ID_SCAN_BTN), L"开始扫描");
        free(s); return 0;
    }

    /* ===== RTU mode below ===== */
    if (!g_scanEvent) g_scanEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    OVERLAPPED ow = {0};
    ow.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);

    /* 暂停读线程，防止串口竞争 */
    InterlockedExchange(&g_pauseRd, 1);
    CancelIoEx(s->hPort, NULL);  /* 取消正在进行的读操作 */
    Sleep(10);  /* 等待读线程响应 */

    for (int a = s->start; a <= s->end && g_scan && !g_scanStop; a++) {
        DWORD t0 = GetTickCount();
        PostMessageW(g_hWnd, WM_SCAN_PRG, a, s->end);
        {
            /* RTU: Modbus protocol */
            BYTE raw_req[2050]; memcpy(raw_req, s->tmpl, s->tlen); raw_req[0] = (BYTE)a;
            BYTE req[2100]; memcpy(req, raw_req, s->tlen); int rl = s->tlen;
            unsigned short cr = crc16(raw_req, s->tlen);
            req[rl++] = cr & 0xFF; req[rl++] = (cr >> 8) & 0xFF;
            { WCHAR t2[32], hx2[4096]; ts(t2, 32); fmt_hex(req, rl, hx2, 4096);
              WCHAR *m = malloc(8192); wsprintfW(m, L"%s[扫描Tx→%d] %s", t2, a, hx2);
              PostMessageW(g_hWnd, WM_LOG_MSG, 0, (LPARAM)m); }
            g_scanRspLen = 0;
            g_scanWaitAddr = a;
            PurgeComm(s->hPort, PURGE_RXCLEAR);
            DWORD w = 0; ResetEvent(ow.hEvent);
            BOOL wrOk = WriteFile(s->hPort, req, rl, &w, &ow);
            if (!wrOk) {
                if (GetLastError() == ERROR_IO_PENDING) wrOk = GetOverlappedResult(s->hPort, &ow, &w, TRUE);
            }
            if (wrOk && w == (DWORD)rl) {
                FlushFileBuffers(s->hPort);
                /* dynamic wait: 3.5 char times + 10ms slave processing margin */
                { int waitMs = coalesce_ms() + 10; if (waitMs < 20) waitMs = 20; Sleep(waitMs); }
            } else {
                WCHAR *em = malloc(256); wsprintfW(em, L"[扫描] 发送失败(地址%d)", a);
                PostMessageW(g_hWnd, WM_LOG_MSG, 0, (LPARAM)em);
                g_scanWaitAddr = 0; continue;
            }
            DWORD wr = WaitForSingleObject(g_scanEvent, s->tout);
            g_scanWaitAddr = 0;
            if (!g_scan || g_scanStop) break;
            if (wr == WAIT_OBJECT_0 && g_scanRspLen > 0) {
                BYTE *rp = g_scanRspBuf; int nr = g_scanRspLen;
                { WCHAR t2[32], hx2[4096]; ts(t2, 32); fmt_hex(rp, nr, hx2, 4096);
                  WCHAR *m = malloc(8192); wsprintfW(m, L"%s[扫描Rx←%d] %s", t2, a, hx2);
                  PostMessageW(g_hWnd, WM_LOG_MSG, 0, (LPARAM)m); }
                if (rp[0] == a && nr >= 4 && crc16(rp, nr) == 0) {
                    found[fc++] = a;
                    WCHAR *m2 = malloc(512); WCHAR t3[32]; ts(t3, 32);
                    wsprintfW(m2, L"%s [%d] √ %s!", t3, a,
                        (rp[1] & 0x80) ? L"从站异常响应" : L"从站响应");
                    PostMessageW(g_hWnd, WM_LOG_MSG, 0, (LPARAM)m2);
                    { WCHAR *rt = malloc(2048); fmt_found_str(rt, 2048, found, fc);
                      PostMessageW(g_hWnd, WM_SCAN_RESULT, 0, (LPARAM)rt); }
                }
            }
            if (!g_scan || g_scanStop) break;
            DWORD elapsed = GetTickCount() - t0;
            int rem = (int)s->ival - (int)elapsed;
            if (rem > 0 && a < s->end) {
                while (rem > 0 && g_scan && !g_scanStop) {
                    int chunk = rem > 200 ? 200 : rem;
                    Sleep(chunk);
                    rem -= chunk;
                }
            }
        }
    }
    CloseHandle(ow.hEvent);
    /* 恢复读线程 */
    InterlockedExchange(&g_pauseRd, 0);
    {
        WCHAR *sm = malloc(4096); WCHAR tt[32]; ts(tt, 32);
        if (fc > 0) {
            WCHAR lst[2048]; fmt_found_str(lst, 2048, found, fc);
            wsprintfW(sm, L"%s === 扫描完成: 发现 %d/%d 个设备: %s ===", tt, fc, s->end - s->start + 1, lst);
        } else {
            wsprintfW(sm, L"%s === 扫描完成: 未发现设备 ===", tt);
        }
        PostMessageW(g_hWnd, WM_LOG_MSG, 0, (LPARAM)sm);
    }
    {
        WCHAR *rt = malloc(2048); fmt_found_str(rt, 2048, found, fc);
        PostMessageW(g_hWnd, WM_SCAN_RESULT, 0, (LPARAM)rt);
    }
    g_scan = 0; g_scanStop = 0;
    SetWindowTextW(g_hScanSt, L"");
    SetWindowTextW(GetDlgItem(g_hWnd, ID_SCAN_BTN), L"开始扫描");
    free(s); return 0;


void start_scan(void) {
    int tcp = (g_mode == 1);
    if (!tcp && !g_run) { MessageBoxW(g_hWnd, L"请先打开串口", L"提示", MB_OK | MB_ICONINFORMATION); return; }
    if (g_scan) return;
    /* stop timer send before scanning */
    KillTimer(g_hWnd, TIMER_SEND);
    SendMessageW(GetDlgItem(g_hWnd, ID_TIMER_CHK), BM_SETCHECK, BST_UNCHECKED, 0);
    if (!tcp && g_hPort == INVALID_HANDLE_VALUE) return;
    SI *s = malloc(sizeof(SI)); memset(s, 0, sizeof(SI));
    s->hPort = tcp ? NULL : g_hPort; s->tcp = tcp;
    s->tout = (int)GetDlgItemInt(g_hWnd, ID_SCAN_TOUT, NULL, FALSE);
    if (tcp) {
        WCHAR wip[64]; char ipa[64], ipe[64];
        GetWindowTextW(GetDlgItem(g_hWnd, ID_SCAN_TCP_START), wip, 64);
        WideCharToMultiByte(CP_ACP, 0, wip, -1, ipa, 64, NULL, NULL);
        char *dot = strrchr(ipa, '.');
        if (dot) { s->start = atoi(dot + 1); int bl = (int)(dot - ipa) + 1; memcpy(s->ipBase, ipa, bl); s->ipBase[bl] = 0; }
        else { s->start = 1; strcpy(s->ipBase, "192.168.1."); }
        GetWindowTextW(GetDlgItem(g_hWnd, ID_SCAN_TCP_END), wip, 64);
        WideCharToMultiByte(CP_ACP, 0, wip, -1, ipe, 64, NULL, NULL);
        dot = strrchr(ipe, '.');
        s->end = dot ? atoi(dot + 1) : 254;
        if (s->tout < 400) s->tout = 400;
        s->ival = s->tout + 100;
        s->method = g_scanMethod;
        s->tcpPort = (int)GetDlgItemInt(g_hWnd, ID_TCP_PORT, NULL, FALSE);
        if (s->tcpPort < 1 || s->tcpPort > 65535) s->tcpPort = 502;
        g_concWrk = (LONG)GetDlgItemInt(g_hWnd, ID_SCAN_THREADS, NULL, FALSE);
        if (g_concWrk < 1) g_concWrk = 16;
        if (s->method == 1) {
            WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
            if (!raw[0]) { MessageBoxW(g_hWnd, L"请在发送区输入Modbus命令(HEX)", L"提示", MB_OK | MB_ICONINFORMATION); free(s); return; }
            BYTE pdu[2048]; int pl = parse_hex_send(raw, pdu, 2048);
            if (pl < 4) { MessageBoxW(g_hWnd, L"发送区HEX命令无效或太短(需≥4字节)", L"提示", MB_OK | MB_ICONINFORMATION); free(s); return; }
            BOOL mb = SendMessageW(GetDlgItem(g_hWnd, ID_MODBUS_CHK), BM_GETCHECK, 0, 0) == BST_CHECKED;
            if (mb) { s->sendLen = build_mbap(pdu, pl, s->sendBuf); }
            else { s->sendLen = pl; memcpy(s->sendBuf, pdu, pl); }
        }
    } else {
        WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
        if (!raw[0]) { MessageBoxW(g_hWnd, L"请在发送区输入请求报文(HEX)", L"提示", MB_OK | MB_ICONINFORMATION); free(s); return; }
        s->start = (int)GetDlgItemInt(g_hWnd, ID_SCAN_START, NULL, FALSE);
        s->end = (int)GetDlgItemInt(g_hWnd, ID_SCAN_END, NULL, FALSE);
        s->ival = (int)GetDlgItemInt(g_hWnd, ID_SCAN_IVAL, NULL, FALSE);
        s->tlen = parse_hex_send(raw, s->tmpl, 2048);
        if (s->tlen < 1) { free(s); return; }
        if (s->tlen >= 6) { g_lastSA = s->tmpl[0]; g_lastFC = s->tmpl[1]; g_lastAD = (s->tmpl[2] << 8) | s->tmpl[3]; g_lastQN = (s->tmpl[4] << 8) | s->tmpl[5]; }
    }
    if (s->start < 1 || s->end > 254 || s->start > s->end || s->tout < 10) { free(s); return; }
    if (!tcp) {
        if (s->ival < 1) s->ival = 1;
        if (s->tout < 1) s->tout = 1;
    }
    /* warn if scanning with write command */
    { BYTE fc = 0;
      if (tcp && s->method == 1 && s->sendLen > 7) fc = s->sendBuf[7];
      else if (!tcp && s->tlen >= 2) fc = s->tmpl[1];
      if (fc == 0x05 || fc == 0x06 || fc == 0x0F || fc == 0x10) {
          if (MessageBoxW(g_hWnd, L"当前命令为写命令，继续扫描会修改设备数据，是否继续？",
              L"警告", MB_YESNO | MB_ICONWARNING) != IDYES) { free(s); return; }
      }
    }
    g_scan = 1; g_scanStop = 0;
    SetWindowTextW(g_hScanSt, L"扫描中..."); scan_result_set(L"");
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_CLEAR), SW_HIDE);
    SetWindowTextW(GetDlgItem(g_hWnd, ID_SCAN_BTN), L"停止扫描");
    HANDLE hTh3 = CreateThread(NULL, 0, scan_thread, s, 0, NULL);
    if (hTh3) CloseHandle(hTh3);

