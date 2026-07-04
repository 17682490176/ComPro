/* Auto-generated from main.c — see compro.h for declarations */
#include "compro.h"

/* ===== COM port ===== */
void upd_ui(void);
void refill_ports(void) {
    HWND c=GetDlgItem(g_hWnd,ID_PORT_COMBO);
    /* save old port list to detect new arrivals */
    int oldPorts[32], oldCnt = (int)SendMessageW(c, CB_GETCOUNT, 0, 0);
    int oldSel = (int)SendMessageW(c, CB_GETCURSEL, 0, 0);
    int oldSelPort = (oldSel >= 0) ? (int)SendMessageW(c, CB_GETITEMDATA, oldSel, 0) : 0;
    for (int i = 0; i < oldCnt && i < 32; i++)
        oldPorts[i] = (int)SendMessageW(c, CB_GETITEMDATA, i, 0);

    SendMessageW(c,CB_RESETCONTENT,0,0);
    int newPorts[32], newCnt = 0;
    /* Method 1: registry enumeration */
    HKEY hk;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"HARDWARE\\DEVICEMAP\\SERIALCOMM", 0, KEY_READ, &hk) == ERROR_SUCCESS) {
        WCHAR name[256], data[256];
        for (int i = 0; ; i++) {
            DWORD ns = 256, ds = sizeof(data);
            if (RegEnumValueW(hk, i, name, &ns, NULL, NULL, (BYTE*)data, &ds) != ERROR_SUCCESS) break;
            int com = 0;
            for (WCHAR *p = data; *p >= L'0' && *p <= L'9'; p++) com = com * 10 + (*p - L'0');
            if (com >= 1 && com <= 32) {
                int ix = (int)SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)data);
                SendMessageW(c, CB_SETITEMDATA, ix, com);
                if (newCnt < 32) newPorts[newCnt++] = com;
            }
        }
        RegCloseKey(hk);
    }
    /* Method 2: probe COM1-32 for ports not in registry (virtual/legacy) */
    if (SendMessageW(c, CB_GETCOUNT, 0, 0) == 0) {
        for (int com = 1; com <= 32; com++) {
            WCHAR path[16]; wsprintfW(path, L"\\\\.\\COM%d", com);
            HANDLE h = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, 0, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                CloseHandle(h);
                WCHAR name[8]; wsprintfW(name, L"COM%d", com);
                int ix = (int)SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)name);
                SendMessageW(c, CB_SETITEMDATA, ix, com);
                if (newCnt < 32) newPorts[newCnt++] = com;
            }
        }
    }
    if (newCnt == 0) return;
    /* find newly appeared ports */
    int freshPorts[32], freshCnt = 0;
    for (int i = 0; i < newCnt; i++) {
        int found = 0;
        for (int j = 0; j < oldCnt; j++) if (newPorts[i] == oldPorts[j]) { found = 1; break; }
        if (!found && freshCnt < 32) freshPorts[freshCnt++] = newPorts[i];
    }
    if (freshCnt == 1) {
        /* exactly one new port → auto-select it */
        for (int i = 0; i < newCnt; i++)
            if ((int)SendMessageW(c, CB_GETITEMDATA, i, 0) == freshPorts[0])
                { SendMessageW(c, CB_SETCURSEL, i, 0); return; }
    }
    /* no new port → restore previous selection if still present */
    for (int i = 0; i < newCnt; i++)
        if (newPorts[i] == oldSelPort) { SendMessageW(c, CB_SETCURSEL, i, 0); return; }
    /* fallback */
    if (oldCnt == 0) SendMessageW(c, CB_SETCURSEL, 0, 0);
}

typedef struct { int port, baud, data, parity, stop; HWND hWnd; } OpenCtx;

DWORD WINAPI open_thread(LPVOID a) {
    OpenCtx *ctx = (OpenCtx*)a;
    WCHAR nm[16]; wsprintfW(nm, L"\\\\.\\COM%d", ctx->port);
    HANDLE h = CreateFileW(nm, GENERIC_READ|GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        SetupComm(h, 4096, 4096);
        DCB d = { sizeof(DCB) }; GetCommState(h, &d);
        d.BaudRate = ctx->baud; d.ByteSize = ctx->data;
        d.Parity = ctx->parity; d.StopBits = ctx->stop;
        d.fBinary = TRUE; d.fDtrControl = DTR_CONTROL_ENABLE; d.fRtsControl = RTS_CONTROL_ENABLE;
        d.fOutxCtsFlow = FALSE; d.fOutxDsrFlow = FALSE; d.fDsrSensitivity = FALSE;
        d.fTXContinueOnXoff = TRUE; d.fOutX = FALSE; d.fInX = FALSE;
        d.fAbortOnError = FALSE;
        SetCommState(h, &d);
        COMMTIMEOUTS to = { MAXDWORD, MAXDWORD, 20, 0, 0 }; SetCommTimeouts(h, &to);
        PurgeComm(h, PURGE_RXCLEAR | PURGE_TXCLEAR);
        PostMessageW(ctx->hWnd, WM_OPEN_RESULT, (WPARAM)h, (LPARAM)ctx);
    } else {
        PostMessageW(ctx->hWnd, WM_OPEN_RESULT, 0, (LPARAM)ctx);
    }
    return 0;
}

DWORD WINAPI rd_thread(LPVOID a) {
    HANDLE hp = (HANDLE)a;
    BYTE b[4096]; OVERLAPPED ov = { 0 };
    ov.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    while (g_run) {
        if (g_pauseRd) { Sleep(5); continue; }
        DWORD n = 0; ResetEvent(ov.hEvent);
        BOOL sync = ReadFile(hp, b, sizeof(b), &n, &ov);
        if (!sync) {
            if (GetLastError() == ERROR_IO_PENDING) {
                if (!GetOverlappedResult(hp, &ov, &n, TRUE)) {
                    if (GetLastError() == ERROR_OPERATION_ABORTED) continue;
                    break;
                }
            } else break;
        }
        if (n > 0 && g_run && !g_pauseRd) {
            BYTE *cp = malloc(n);
            if (cp) { memcpy(cp, b, n); PostMessageW(g_hWnd, WM_RX_DATA, (WPARAM)cp, n); }
        }
    }
    CloseHandle(ov.hEvent);
    CloseHandle(hp);
    if (g_run) { g_run = 0; g_hPort = INVALID_HANDLE_VALUE;
        PostMessageW(g_hWnd, WM_LOG_MSG, 0, (LPARAM)_wcsdup(L"串口已断开"));
        PostMessageW(g_hWnd, WM_UPD_UI, 0, 0); }
    return 0;
}


