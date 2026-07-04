/* Auto-generated from main.c — see compro.h for declarations */
#include "compro.h"

int send_all(SOCKET sock, const char *buf, int len) {
    int total = 0;
    while (total < len) {
        int sent = send(sock, buf + total, len - total, 0);
        if (sent <= 0) return -1;
        total += sent;
    }
    return total;
}

int build_mbap(BYTE *d, int dl, BYTE *out) {
    int tid = (int)InterlockedIncrement(&g_mbap_tid);
    out[0] = (tid >> 8) & 0xFF; out[1] = tid & 0xFF;
    out[2] = 0; out[3] = 0;
    out[4] = (dl >> 8) & 0xFF; out[5] = dl & 0xFF;
    memcpy(out + 6, d, dl);
    return dl + 6;
}

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) {
        PostMessageW(ctx->hWnd, WM_OPEN_RESULT, 0, (LPARAM)ctx);
        return 0;
    }
    int timeout = 3000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    WCHAR ip[64]; GetWindowTextW(GetDlgItem(g_hWnd, ID_TCP_IP), ip, 64);
    char ipa[64]; WideCharToMultiByte(CP_ACP, 0, ip, -1, ipa, 64, NULL, NULL);
    addr.sin_addr.s_addr = inet_addr(ipa);
    int port = (int)GetDlgItemInt(g_hWnd, ID_TCP_PORT, NULL, FALSE);
    if (port < 1 || port > 65535) port = 502;
    addr.sin_port = htons((u_short)port);
    if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        PostMessageW(ctx->hWnd, WM_OPEN_RESULT, 0, (LPARAM)ctx);
        return 0;
    }
    /* TCP keep-alive to prevent idle disconnect */
    { int ka = 1; setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&ka, sizeof(ka)); }
    { struct tcp_keepalive ka = { 1, 10000, 5000 }; DWORD br;
      WSAIoctl(s, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0, &br, NULL, NULL); }
    ctx->sock = s;
    PostMessageW(ctx->hWnd, WM_OPEN_RESULT, (WPARAM)ctx, 0);
    return 0;
}

DWORD WINAPI tcp_rd_thread(LPVOID a) {
    SOCKET s = (SOCKET)(INT_PTR)a;
    BYTE b[4096];
    while (g_run) {
        if (g_pauseRd) { Sleep(5); continue; }
        if (s == INVALID_SOCKET) {
            if (!g_run) break;
            s = socket(AF_INET, SOCK_STREAM, 0);
            if (s == INVALID_SOCKET) {                 for (int _w = 0; _w < (g_reconnectDelay/100) && g_run; _w++) Sleep(100);
            continue; }
            /* non-blocking connect — cancellable per g_run */
            { u_long nb = 1; ioctlsocket(s, FIONBIO, &nb); }
            { struct sockaddr_in addr = {0};
              addr.sin_family = AF_INET;
              addr.sin_addr.s_addr = inet_addr(g_tcpIp);
              addr.sin_port = htons((u_short)g_tcpPort);
              connect(s, (struct sockaddr*)&addr, sizeof(addr)); }
            { fd_set wfds; struct timeval tv = {0, 100000}; int ok = 0;
              while (g_run && !ok) {
                  FD_ZERO(&wfds); FD_SET(s, &wfds);
                  int r = select(0, NULL, &wfds, NULL, &tv);
                  if (r > 0) { int err = 0; int len = sizeof(err);
                      getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
                      if (err == 0) ok = 1; else break; }
                  else if (r == SOCKET_ERROR) break;
              }
              if (!ok || !g_run) { closesocket(s); s = INVALID_SOCKET;
                                  for (int _w = 0; _w < (g_reconnectDelay/100) && g_run; _w++) Sleep(100);
                  continue; }
            }
            { u_long nb = 0; ioctlsocket(s, FIONBIO, &nb); }
            { int timeout = 3000;
              setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
              setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout)); }
            { int ka = 1; setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&ka, sizeof(ka)); }
            { struct tcp_keepalive ka = { 1, 10000, 5000 }; DWORD br;
              WSAIoctl(s, SIO_KEEPALIVE_VALS, &ka, sizeof(ka), NULL, 0, &br, NULL, NULL); }
            g_sock = s;
            g_reconnectDelay = 1000;  /* reset backoff on successful reconnect */
            { WCHAR wip[64]; MultiByteToWideChar(CP_ACP, 0, g_tcpIp, -1, wip, 64);
              WCHAR info[128]; wsprintfW(info, L"%s:%d", wip, g_tcpPort);
              log_fmt(CLR_SYS, L"--- TCP 已连接: %s ---", info);
              SetWindowTextW(g_hConn, info); }
            SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 已连接");
        }
        fd_set fds; FD_ZERO(&fds); FD_SET(s, &fds);
        struct timeval tv = {0, 50000};
        int sel = select(0, &fds, NULL, NULL, &tv);
        if (sel == SOCKET_ERROR) goto tcp_lost;
        if (sel == 0) continue;
        int n = recv(s, (char*)b, sizeof(b), 0);
        if (n == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) continue;
            goto tcp_lost;
        }
        if (n == 0) goto tcp_lost;
        if (n > 0 && g_run && !g_pauseRd) {
            BYTE *cp = malloc(n);
            if (cp) { memcpy(cp, b, n); PostMessageW(g_hWnd, WM_RX_DATA, (WPARAM)cp, n); }
        }
        continue;
    tcp_lost:
        closesocket(s); s = INVALID_SOCKET; g_sock = INVALID_SOCKET;
        SetWindowTextW(g_hConn, L"");
        if (g_reconnectDelay < 30000) g_reconnectDelay *= 2;  /* exponential backoff */
        SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 等待重连");
                        for (int _w = 0; _w < (g_reconnectDelay/100) && g_run; _w++) Sleep(100);
        continue;
    }
    if (s != INVALID_SOCKET && g_run) { shutdown(s, SD_BOTH); closesocket(s); }
    else if (s != INVALID_SOCKET) { closesocket(s); }
    return 0;
}

#define REG_KEY L"Software\\ComPro"

void reg_set_str(const WCHAR *name, const WCHAR *val) {
    HKEY hk;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, NULL, 0,
        KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS) {
        DWORD cb = (DWORD)((wcslen(val) + 1) * sizeof(WCHAR));
        RegSetValueExW(hk, name, 0, REG_SZ, (BYTE*)val, cb);
        RegCloseKey(hk);

