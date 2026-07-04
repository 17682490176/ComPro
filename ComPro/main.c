#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <winsock2.h>
#include <windows.h>
#include <mstcpip.h>
#include <commctrl.h>
#include <richedit.h>
#include <dwmapi.h>
#include <ipexport.h>
#include <icmpapi.h>
#include <dbt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

/* ===== IDs ===== */
enum {
    ID_PORT_COMBO = 1001, ID_PORT_REFRESH, ID_BAUD_COMBO,
    ID_DATA_COMBO, ID_PARITY_COMBO, ID_STOP_COMBO,
    ID_MODBUS_CHK, ID_OPEN_BTN,
    ID_TIME_CHK, ID_TIME_DISP_CHK, ID_DECTABLE_CHK,
    ID_CLEAR_BTN, ID_SNAP_BTN, ID_SNAP2_BTN, ID_SNAP3_BTN,
    ID_SNAP4_BTN, ID_SNAP5_BTN, ID_SNAP6_BTN,
    ID_DISPLAY, ID_REG_LIST,
    ID_TIMER_CHK, ID_TIMER_MS, ID_SLAVEADDR_DEC, ID_REGADDR_DEC, ID_SEND_BTN, ID_SEND_INPUT,
    ID_SCAN_START, ID_SCAN_END, ID_SCAN_IVAL, ID_SCAN_TOUT,
    ID_SCAN_BTN, ID_SCAN_STATUS, ID_SCAN_RESULT, ID_SCAN_CLEAR,
    ID_STATUS,
    ID_RTU_RADIO, ID_TCP_RADIO, ID_TCP_IP, ID_TCP_PORT,
    ID_SCAN_TCP_START, ID_SCAN_TCP_END, ID_SCAN_THREADS, ID_VAL_DEC, ID_SCAN_METHOD,
    TIMER_SEND = 2001, TIMER_COM_SCAN = 3004, COALESCE_RX = 3003
};

enum { WM_RX_DATA = WM_USER+1, WM_LOG_MSG, WM_SCAN_PRG, WM_OPEN_RESULT, WM_PING_RESULT, WM_SCAN_DONE, WM_SCAN_RESULT, WM_UPD_UI };

#define VERSION L"4.1"
#define WW 920
#define WH 695
#define MG 12
#define GAP 7
#define RH 25

#define CLR_BG   RGB(248,248,244)
#define CLR_RX   RGB(0,0,255)
#define CLR_TX   RGB(0,140,0)
#define CLR_SYS  RGB(130,130,130)
#define CLR_SCAN RGB(0,0,200)
#define CLR_IP_A  RGB(0,72,186)
#define CLR_IP_B  RGB(186,72,0)

/* COM port device interface GUID {86E0D1E0-8089-11D0-9CE4-08003E301F73} */
static const GUID g_COM_GUID = {0x86E0D1E0,0x8089,0x11D0,{0x9C,0xE4,0x08,0x00,0x3E,0x30,0x1F,0x73}};

/* ===== globals ===== */
static HWND g_hWnd, g_hDisp, g_hRegList, g_hSend, g_hRegAddr, g_hSlaveAddr, g_hVal, g_hScanSt, g_hScanRes, g_hConn;
static HWND g_hSendBtn = NULL;  /* send button for enable/disable on validation */
static HWND g_hEditCell = NULL, g_hTcpIpLbl = NULL, g_hTcpPortLbl = NULL, g_hScanLbl = NULL, g_hTimeDispChk = NULL, g_hDectableChk = NULL;
static HWND g_hBitEd = NULL;
static HWND g_hScanStartLbl = NULL, g_hScanEndLbl = NULL, g_hScanIvalLbl = NULL;
#define SNAP_LONG_PRESS 800
#define SNAP_TIMER_ID 3002
static int g_bSyncing = 0;
static int g_dispH, g_editItem = -1, g_editCol = 0, g_editOldVal = 0;
static HBRUSH g_hBgBr, g_hRedBr;
static HFONT g_hMono, g_hFont;
static CHARFORMAT2 g_LogFont;  /* unified log font to prevent RichEdit font switching */
static int g_sendOk = 1;
static HANDLE g_hPort = INVALID_HANDLE_VALUE;
static SOCKET g_sock = INVALID_SOCKET;
static HANDLE g_hRdTh = NULL;
static volatile LONG g_run = 0, g_scan = 0, g_scanStop = 0, g_pauseRd = 0;
static int g_mode = 0;
static int g_scanMethod = 0;
static LONG g_mbap_tid = 1;
static char g_tcpIp[64]; static int g_tcpPort = 502;
static volatile int g_scanWaitAddr = 0;
static HANDLE g_scanEvent = NULL;
static BYTE g_scanRspBuf[2048];
static int g_scanRspLen = 0;
static volatile LONG g_pingNext;
static volatile LONG g_pingFc;
static int g_pingFound[254];
static WCHAR g_pingIpBase[64];
static WCHAR g_scanResult[5500];
static int g_srScrollY = 0;
static LONG g_concWrk = 16;
static HWND g_hThreadLbl = NULL, g_hScanMethod = NULL;
static HDEVNOTIFY g_hDevNotify = NULL;
static int g_reconnectDelay = 1000;  /* TCP reconnect backoff, 1s initial */


enum { MAX_REG = 256 };
static struct { int slave, addr, val, ok, mod, fc, fmt, bo, fmt_set, name_set; WCHAR name[32]; } g_reg[MAX_REG];
static int g_nr = 0;
static int g_lastSA, g_lastFC, g_lastAD, g_lastQN;
static int g_copyFmt = -1, g_copyBo = 0;  /* format clipboard */
typedef struct { int active, slave, fc, addr, qty, fmt[256], bo[256]; WCHAR name[256][32]; WCHAR sname[32]; WCHAR send_cmd[4096]; int saved_to_reg; } SnapState;
#define SNAP_COUNT 6
static SnapState g_snap[SNAP_COUNT];
typedef struct { SnapState *snap; HWND btn; int idx; int longPressed; } SnapBtnCtx;
static SnapBtnCtx g_snapCtx[SNAP_COUNT];
static HWND g_hSnapBtn[SNAP_COUNT];
static HWND g_hRenameEdit = NULL;
static int g_dispLen = 0;
static BYTE g_rxBuf[4096];
static int g_rxBufLen = 0;
static BYTE g_dispRxBuf[4096];
static int g_dispRxLen = 0;
static int g_dispRetry = 0;
static DWORD g_lastRegTicks = 0;
static LARGE_INTEGER g_freq, g_t0_send, g_t1_wr, g_t3_ui;
static const int g_baud_rates[] = {300,1200,2400,4800,9600,14400,19200,38400,57600,115200,230400,460800,921600};
#define BAUD_COUNT 13

/* ===== helpers ===== */

/* reliable TCP send — loops until all bytes are sent or error */
static int send_all(SOCKET sock, const char *buf, int len) {
    int total = 0;
    while (total < len) {
        int sent = send(sock, buf + total, len - total, 0);
        if (sent <= 0) return -1;
        total += sent;
    }
    return total;
}

static void ts(WCHAR *b, int n) {
    SYSTEMTIME t; GetLocalTime(&t);
    if (n < 16) return;
    wsprintfW(b,L"<%02d:%02d:%02d.%03d>",t.wHour,t.wMinute,t.wSecond,t.wMilliseconds);
}

static int baudrate(void) {
    int s=(int)SendMessageW(GetDlgItem(g_hWnd,ID_BAUD_COMBO),CB_GETCURSEL,0,0);
    if(s>=0&&s<BAUD_COUNT)return g_baud_rates[s];
    return 9600;
}

/* calculate coalesce timeout (ms) = 3.5 char times for Modbus RTU inter-frame gap */
static int coalesce_ms(void) {
    if (g_mode == 1) return 30; /* TCP: fixed 30ms */
    int bd = baudrate();
    if (bd <= 0) bd = 9600;
    int bits = 1; /* start bit */
    { int x = (int)SendMessageW(GetDlgItem(g_hWnd, ID_DATA_COMBO), CB_GETCURSEL, 0, 0);
      int ds[] = { 8,7,6,5 }; bits += (x >= 0 && x < 4) ? ds[x] : 8; }
    { int x = (int)SendMessageW(GetDlgItem(g_hWnd, ID_PARITY_COMBO), CB_GETCURSEL, 0, 0);
      bits += (x == 0) ? 0 : 1; } /* NOPARITY=0, EVEN/ODD have 1 parity bit */
    { int x = (int)SendMessageW(GetDlgItem(g_hWnd, ID_STOP_COMBO), CB_GETCURSEL, 0, 0);
      bits += (x == 1) ? 2 : 1; } /* ONESTOPBIT=0, TWOSTOPBITS=1 */
    int ms = (int)((3.5 * bits * 1000) / bd);
    if (ms < 15) ms = 15;   /* Windows timer granularity floor */
    if (ms > 250) ms = 250; /* reasonable ceiling */
    return ms;
}

static void fmt_hex(const BYTE *d, int n, WCHAR *o, int m) {
    int p=0;
    for(int i=0;i<n&&p<m-4;i++){wsprintfW(o+p,L"%02X ",d[i]);p+=3;}
    if(p>0)o[p-1]=0;else o[0]=0;
}

static void trim_and_scroll(void) {
    g_dispLen = GetWindowTextLengthW(g_hDisp);
    if (g_dispLen > 60000) {
        SendMessageW(g_hDisp, EM_SETSEL, 0, g_dispLen - 50000);
        SendMessageW(g_hDisp, EM_REPLACESEL, FALSE, (LPARAM)L"");
        g_dispLen = GetWindowTextLengthW(g_hDisp);
    }
    SCROLLINFO si = { sizeof(si), SIF_ALL };
    GetScrollInfo(g_hDisp, SB_VERT, &si);
    if (si.nPos + (int)si.nPage >= si.nMax - 30)
        SendMessageW(g_hDisp, WM_VSCROLL, SB_BOTTOM, 0);
}

/* core log writer: append styled text to RichEdit with optional gray timestamp prefix.
   When addTs is TRUE, a gray <HH:MM:SS.ms> prefix is conditionally prepended. */
static void log_write(const WCHAR *s, COLORREF c, BOOL addTs) {
    int curLen = GetWindowTextLengthW(g_hDisp);
    SendMessageW(g_hDisp, EM_SETSEL, curLen, curLen);
    if (addTs) {
        int showTime = g_hTimeDispChk &&
            SendMessageW(g_hTimeDispChk, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (showTime) {
            WCHAR t[32]; ts(t, 32);
            g_LogFont.dwMask |= CFM_COLOR; g_LogFont.crTextColor = CLR_SYS;
            SendMessageW(g_hDisp, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&g_LogFont);
            g_LogFont.dwMask &= ~CFM_COLOR;
            SendMessageW(g_hDisp, EM_REPLACESEL, FALSE, (LPARAM)t);
            SendMessageW(g_hDisp, EM_REPLACESEL, FALSE, (LPARAM)L" ");
        }
    }
    g_LogFont.dwMask |= CFM_COLOR; g_LogFont.crTextColor = c;
    SendMessageW(g_hDisp, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&g_LogFont);
    g_LogFont.dwMask &= ~CFM_COLOR;
    SendMessageW(g_hDisp, EM_REPLACESEL, FALSE, (LPARAM)s);
    SendMessageW(g_hDisp, EM_SETSEL, -1, -1);
    SendMessageW(g_hDisp, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
    trim_and_scroll();
}

static void log_line(const WCHAR *s, COLORREF c) {
    log_write(s, c, FALSE);  /* timestamp already baked into s by caller */
}

static void log_fmt(COLORREF c, const WCHAR *fmt, ...) {
    WCHAR b[8192];va_list a;va_start(a,fmt);wvsprintfW(b,fmt,a);va_end(a);
    WCHAR t[32];ts(t,32);WCHAR ln[8700];wsprintfW(ln,L"%s %s",t,b);
    log_line(ln,c);
}

/* log data line with gray timestamp prefix (when time display is enabled) */
static void log_data(const WCHAR *s, COLORREF c) {
    log_write(s, c, TRUE);
}

static unsigned short crc16(const BYTE *d, int n) {
    unsigned short c=0xFFFF;
    for(int i=0;i<n;i++){c^=d[i];for(int j=0;j<8;j++)c=(c&1)?(c>>1)^0xA001:(c>>1);}
    return c;
}

/* parse hex send text → bytes. returns byte count, or -1 on error */
static int parse_hex_send(const WCHAR *raw, BYTE *out, int maxLen) {
    WCHAR cl[4096]; int ci = 0;
    for (int i = 0; raw[i]; i++)
        if (raw[i] != L' ' && raw[i] != L'\t' && raw[i] != L'\r' && raw[i] != L'\n') cl[ci++] = raw[i];
    cl[ci] = 0;
    if (ci % 2 || ci / 2 > maxLen) return -1;
    int dl = 0;
    for (int i = 0; i < ci; i += 2) {
        WCHAR p[3] = { cl[i], cl[i + 1], 0 };
        unsigned v;
        if (swscanf(p, L"%2x", &v) != 1) return -1;
        out[dl++] = (BYTE)v;
    }
    return dl;
}

/* sync decimal address input with send command hex bytes 2-3 */
static void sync_addr_from_send(void) {
    if (g_bSyncing) return;
    WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
    WCHAR cl[4096]; int ci = 0;
    for (int i = 0; raw[i]; i++) {
        WCHAR c = raw[i];
        if (iswxdigit(c)) cl[ci++] = c;
    }
    /* slave address: hex offset 0-1 */
    if (ci >= 2) {
        unsigned v; WCHAR p[3] = { cl[0], cl[1], 0 };
        if (swscanf(p, L"%2x", &v) == 1) {
            g_bSyncing = 1;
            WCHAR dec[16]; wsprintfW(dec, L"%d", v);
            SetWindowTextW(g_hSlaveAddr, dec);
            g_bSyncing = 0;
        }
    }
    /* register address: hex offset 4-7 */
    int off = 4;
    if (ci < off + 4) return;
    unsigned v; WCHAR p[3] = { cl[off], cl[off+1], 0 };
    if (swscanf(p, L"%2x", &v) != 1) return;
    int addr = (v << 8);
    p[0] = cl[off+2]; p[1] = cl[off+3]; p[2] = 0;
    if (swscanf(p, L"%2x", &v) != 1) return;
    addr |= v;
    g_bSyncing = 1;
    WCHAR dec[16]; wsprintfW(dec, L"%d", addr);
    SetWindowTextW(g_hRegAddr, dec);
    g_bSyncing = 0;
}

static void sync_addr_to_send(void) {
    if (g_bSyncing) return;
    WCHAR dec[32]; GetWindowTextW(g_hRegAddr, dec, 32);
    int addr = 0;
    for (WCHAR *p = dec; *p >= L'0' && *p <= L'9'; p++) addr = addr * 10 + (*p - L'0');
    if (addr < 0 || addr > 65535) return;
    WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
    int hc = 0;
    for (int i = 0; raw[i]; i++)
        if (iswxdigit(raw[i])) hc++;
    /* PDU bytes 2-3 = hex offset 4-7 */
    int addrOff = 4;
    if (hc < addrOff + 4) return;
    WCHAR out[4096]; int oi = 0, bi = 0;
    for (int i = 0; raw[i]; ) {
        WCHAR c = raw[i];
        if (!((c >= L'0' && c <= L'9') || (c >= L'A' && c <= L'F') || (c >= L'a' && c <= L'f'))) { out[oi++] = raw[i++]; continue; }
        if (bi == addrOff) { wsprintfW(out + oi, L"%02X", (addr >> 8) & 0xFF); oi += 2; i += 2; bi += 2; }
        else if (bi == addrOff + 2) { wsprintfW(out + oi, L"%02X", addr & 0xFF); oi += 2; i += 2; bi += 2; }
        else { out[oi++] = raw[i++]; bi++; }
    }
    out[oi] = 0;
    g_bSyncing = 1;
    SetWindowTextW(g_hSend, out);
    g_bSyncing = 0;
}

static void sync_slave_to_send(void) {
    if (g_bSyncing) return;
    WCHAR dec[32]; GetWindowTextW(g_hSlaveAddr, dec, 32);
    int slv = 0;
    for (WCHAR *p = dec; *p >= L'0' && *p <= L'9'; p++) slv = slv * 10 + (*p - L'0');
    if (slv < 1 || slv > 254) return;
    WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
    int hc = 0;
    for (int i = 0; raw[i]; i++)
        if (iswxdigit(raw[i])) hc++;
    if (hc < 2) return;
    WCHAR out[4096]; int oi = 0, bi = 0;
    int off = 0;
    for (int i = 0; raw[i]; ) {
        WCHAR c = raw[i];
        if (!iswxdigit(c)) { out[oi++] = raw[i++]; continue; }
        if (bi == off) { wsprintfW(out + oi, L"%02X", slv & 0xFF); oi += 2; i += 2; bi += 2; }
        else { out[oi++] = raw[i++]; bi++; }
    }
    out[oi] = 0;
    g_bSyncing = 1;
    SetWindowTextW(g_hSend, out);
    g_bSyncing = 0;
}

static void sync_val_from_send(void) {
    if (g_bSyncing) return;
    WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
    WCHAR cl[4096]; int ci = 0;
    for (int i = 0; raw[i]; i++) {
        WCHAR c = raw[i];
        if (iswxdigit(c)) cl[ci++] = c;
    }
    int off = 8;
    if (ci < off + 4) return;
    unsigned v; WCHAR p[3] = { cl[off], cl[off+1], 0 };
    if (swscanf(p, L"%2x", &v) != 1) return;
    int val = (v << 8);
    p[0] = cl[off+2]; p[1] = cl[off+3]; p[2] = 0;
    if (swscanf(p, L"%2x", &v) != 1) return;
    val |= v;
    g_bSyncing = 1;
    WCHAR dec[16]; wsprintfW(dec, L"%d", val);
    SetWindowTextW(g_hVal, dec);
    g_bSyncing = 0;
}

static void sync_val_to_send(void) {
    if (g_bSyncing) return;
    WCHAR dec[32]; GetWindowTextW(g_hVal, dec, 32);
    int val = 0;
    for (WCHAR *p = dec; *p >= L'0' && *p <= L'9'; p++) val = val * 10 + (*p - L'0');
    if (val < 0 || val > 65535) return;
    WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
    int hc = 0;
    for (int i = 0; raw[i]; i++)
        if (iswxdigit(raw[i])) hc++;
    int valOff = 8;
    if (hc < valOff + 4) return;
    WCHAR out[4096]; int oi = 0, bi = 0;
    for (int i = 0; raw[i]; ) {
        WCHAR c = raw[i];
        if (!iswxdigit(c)) { out[oi++] = raw[i++]; continue; }
        if (bi == valOff) { wsprintfW(out + oi, L"%02X", (val >> 8) & 0xFF); oi += 2; i += 2; bi += 2; }
        else if (bi == valOff + 2) { wsprintfW(out + oi, L"%02X", val & 0xFF); oi += 2; i += 2; bi += 2; }
        else { out[oi++] = raw[i++]; bi++; }
    }
    out[oi] = 0;
    g_bSyncing = 1;
    SetWindowTextW(g_hSend, out);
    g_bSyncing = 0;
}

static void reg_table(void);
static LRESULT CALLBACK ec_proc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);

static int get_multi(int idx, int nreg, int bo, unsigned long long *out);

static void auto_detect_fmt(int idx) {
    /* auto-detect 32-bit format for one register pair:
       try I32/U32/F32 × 4 byte orders, filter garbage, pick value closest to 1.0 */
    if (idx + 1 >= g_nr || !g_reg[idx+1].ok ||
        g_reg[idx+1].addr != g_reg[idx].addr + 1 ||
        g_reg[idx+1].slave != g_reg[idx].slave) return;
    unsigned long long raw;
    double bestDist = 1e308, fallbackDist = 1e308;
    int bestFmt = -1, bestBo = 0, fallbackFmt = -1, fallbackBo = 0;
    for (int bo = 0; bo < 4; bo++) if (get_multi(idx, 2, bo, &raw)) {
        /* F32 */
        { float fv; memcpy(&fv, &raw, 4);
          double v = (double)fv, d = v - 1.0; if (d < 0) d = -d;
          if (d < fallbackDist) { fallbackDist = d; fallbackFmt = 8; fallbackBo = bo; }
          /* filter NaN/Inf/denormal/near-zero */
          if (v == v && v * 1e6 < 1e300 && (v > 0.001 || v < -0.001))
              { if (d < bestDist) { bestDist = d; bestFmt = 8; bestBo = bo; } }
        }
        /* I32 */
        { double v = (double)(int)(raw & 0xFFFFFFFFULL), d = v - 1.0; if (d < 0) d = -d;
          if (d < fallbackDist) { fallbackDist = d; fallbackFmt = 4; fallbackBo = bo; }
          if (d < bestDist) { bestDist = d; bestFmt = 4; bestBo = bo; }
        }
        /* U32 */
        { double v = (double)(unsigned)(raw & 0xFFFFFFFFULL), d = v - 1.0; if (d < 0) d = -d;
          if (d < fallbackDist) { fallbackDist = d; fallbackFmt = 5; fallbackBo = bo; }
          if (d < bestDist) { bestDist = d; bestFmt = 5; bestBo = bo; }
        }
    }
    if (bestFmt < 0) { bestFmt = fallbackFmt; bestBo = fallbackBo; }
    if (bestFmt >= 0) {
        g_reg[idx].fmt = bestFmt; g_reg[idx].bo = bestBo;
        g_reg[idx].fmt_set = 1;
    }
}

static void scan_result_set(const WCHAR *txt) {
    wcsncpy(g_scanResult, txt, 5499);
    g_scanResult[5499] = 0;
    g_srScrollY = 0;
    InvalidateRect(g_hScanRes, NULL, TRUE);
    int len = (int)wcslen(g_scanResult);
    ShowWindow(g_hScanRes, len > 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_CLEAR), len > 0 ? SW_SHOW : SW_HIDE);
}

static void update_scan_display(void) {
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

static void clear_disp(void) {
    if (g_hBitEd) { DestroyWindow(g_hBitEd); }
    SetWindowTextW(g_hDisp,L"");g_dispLen=0;
    g_nr=0;g_rxBufLen=0;g_lastRegTicks=0;reg_table();
}

static int get_multi(int idx, int nreg, int bo, unsigned long long *out) {
    if (idx + nreg > g_nr) return 0;
    for (int i = 1; i < nreg; i++) {
        if (g_reg[idx + i].addr != g_reg[idx].addr + i) return 0;
        if (!g_reg[idx + i].ok) return 0;
    }
    unsigned short r[4] = {0};
    for (int i = 0; i < nreg; i++) r[i] = (unsigned short)g_reg[idx + i].val;
    *out = 0;
    if (nreg == 2) {
        unsigned int bs[2];
        for (int i = 0; i < 2; i++) bs[i] = (bo & 2) ? ((r[i] >> 8) | (r[i] << 8)) & 0xFFFF : r[i];
        if (bo & 1) *out = ((unsigned long long)bs[1] << 16) | bs[0];
        else        *out = ((unsigned long long)bs[0] << 16) | bs[1];
        *out &= 0xFFFFFFFFULL;
    } else {
        unsigned long long w[4];
        for (int i = 0; i < 4; i++) w[i] = (bo & 2) ? ((r[i] >> 8) | (r[i] << 8)) & 0xFFFF : r[i];
        if (bo & 1) { for (int i = 0; i < 4; i++) w[i] = w[3-i]; }
        *out = (w[0] << 48) | (w[1] << 32) | (w[2] << 16) | w[3];
    }
    return 1;
}

static int g_consumeCache[MAX_REG];  /* memoization cache for is_consumed() */
static int g_consumeCacheGen = 0;     /* incremented each reg_table() rebuild */

static int is_consumed(int idx) {
    if (g_consumeCache[idx] == g_consumeCacheGen) return 0;  /* cached: not consumed */
    if (g_consumeCache[idx] == g_consumeCacheGen + 1) return 1;  /* cached: consumed */
    int result = 0;
    for (int j = idx - 1; j >= 0 && g_reg[j].addr <= g_reg[idx].addr; j--) {
        if (g_reg[j].fmt < 4) continue;
        int nr = (g_reg[j].fmt <= 5 || g_reg[j].fmt == 8) ? 2 : 4;
        if (g_reg[j].addr + nr > g_reg[idx].addr) {
            if (is_consumed(j)) continue;
            result = 1;
            break;
        }
    }
    g_consumeCache[idx] = g_consumeCacheGen + (result ? 1 : 0);
    return result;
}
/* invalidate is_consumed() cache — call at start of reg_table() rebuild */
static void consume_cache_reset(void) {
    g_consumeCacheGen += 2;
}

static void update_snap_text(SnapState *snap, HWND btn) {
    if (!snap->active) SetWindowTextW(btn, L"未暂存");
    else if (snap->sname[0]) SetWindowTextW(btn, snap->sname);
    else SetWindowTextW(btn, L"已暂存");
}

static void snap_clear(SnapState *snap, HWND btn) {
    snap->active = 0;
    snap->sname[0] = 0;
    snap->saved_to_reg = 0;
    update_snap_text(snap, btn);
}

static void snap_capture(SnapState *snap, HWND btn) {
    if (g_nr <= 0) return;  /* safety: no register data to capture */
    snap->slave = g_reg[0].slave;
    snap->fc = g_reg[0].fc;
    snap->addr = g_reg[0].addr;
    snap->qty = g_nr;
    for (int i = 0; i < g_nr && i < 256; i++) {
        snap->fmt[i] = g_reg[i].fmt;
        snap->bo[i] = g_reg[i].bo;
        wcscpy(snap->name[i], g_reg[i].name);
    }
    GetWindowTextW(g_hSend, snap->send_cmd, 4096);
    snap->active = 1;
    update_snap_text(snap, btn);
}

static LRESULT CALLBACK rename_edit_proc(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
static void snap_save_reg(SnapState *snap, int idx);
static void snap_load_reg(SnapState *snap, int idx, HWND btn);

static void do_send(BOOL silent);



static LRESULT CALLBACK snap_btn_proc(HWND h, UINT m, WPARAM w, LPARAM l,
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

static LRESULT CALLBACK rename_edit_proc(HWND h, UINT m, WPARAM w, LPARAM l,
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
        g_hRenameEdit = NULL; DestroyWindow(h);
        return 0;
    }
    return DefSubclassProc(h, m, w, l);
}

static void fmt_reg_val(int idx, WCHAR *v, int v_size) {
    if (is_consumed(idx)) { wcscpy(v, L""); return; }
    if (!g_reg[idx].ok) { wcscpy(v, L"?"); return; }
    if (g_reg[idx].fmt == 0) { wsprintfW(v, L"%d", (short)(g_reg[idx].val & 0xFFFF)); return; }
    if (g_reg[idx].fmt == 2) { wsprintfW(v, L"0x%04X", g_reg[idx].val & 0xFFFF); return; }
    if (g_reg[idx].fmt == 3) {
        int p = 0;
        for (int b = 15; b >= 0; b--) {
            v[p++] = (g_reg[idx].val >> b) & 1 ? L'1' : L'0';
            if (b % 4 == 0 && b > 0) v[p++] = L' ';
        }
        v[p] = 0;
        return;
    }
    if (g_reg[idx].fmt >= 4) {
        unsigned long long raw = 0;
        int nr = (g_reg[idx].fmt <= 5 || g_reg[idx].fmt == 8) ? 2 : 4;
        if (get_multi(idx, nr, g_reg[idx].bo, &raw)) {
            if (g_reg[idx].fmt == 4) wsprintfW(v, L"%d", (int)(raw & 0xFFFFFFFFULL));
            else if (g_reg[idx].fmt == 5) wsprintfW(v, L"%u", (unsigned)(raw & 0xFFFFFFFFULL));
            else if (g_reg[idx].fmt == 6) wsprintfW(v, L"%lld", (long long)raw);
            else if (g_reg[idx].fmt == 7) wsprintfW(v, L"%llu", raw);
            else if (g_reg[idx].fmt == 8) { float f; memcpy(&f, &raw, 4); swprintf(v, v_size, L"%.6g", f); }
            else { double d; memcpy(&d, &raw, 8); swprintf(v, v_size, L"%.12g", d); }
        } else { wcscpy(v, L"?"); }
        return;
    }
    wsprintfW(v, L"%u", g_reg[idx].val & 0xFFFF);
}

static void reg_table(void) {
    DWORD now=GetTickCount();
    if(now-g_lastRegTicks<30){SetTimer(g_hWnd,3001,30,NULL);return;}
    g_lastRegTicks=now;KillTimer(g_hWnd,3001);
    HWND lv=g_hRegList;
    BOOL sh=SendMessageW(GetDlgItem(g_hWnd,ID_DECTABLE_CHK),BM_GETCHECK,0,0)==BST_CHECKED;
    int tw = WW - MG*2 - 405;
    if(!sh){
        ShowWindow(lv, SW_HIDE);
        for(int i=0;i<SNAP_COUNT;i++)ShowWindow(g_hSnapBtn[i],SW_HIDE);
        SetWindowPos(g_hDisp,NULL,0,0,WW-MG*2,g_dispH,SWP_NOMOVE|SWP_NOZORDER);
        return;
    }
    for(int i=0;i<SNAP_COUNT;i++)ShowWindow(g_hSnapBtn[i],SW_SHOW);
    SetWindowPos(g_hDisp,NULL,0,0,tw,g_dispH,SWP_NOMOVE|SWP_NOZORDER);
    SendMessageW(lv,WM_SETREDRAW,FALSE,0);
    int prevCount = (int)SendMessageW(lv, LVM_GETITEMCOUNT, 0, 0);
    if(!g_nr){
        if (prevCount != 1) {
            SendMessageW(lv,LVM_DELETEALLITEMS,0,0);
            LVITEMW it={LVIF_TEXT,0,0,0,0,L"(等待数据...)"};
            SendMessageW(lv,LVM_INSERTITEMW,0,(LPARAM)&it);
        }
        SendMessageW(lv,WM_SETREDRAW,TRUE,0);
        ShowWindow(lv,SW_SHOW);
        InvalidateRect(GetDlgItem(g_hWnd, ID_SEND_BTN), NULL, TRUE);
        return;
    }
    int topIdx = (int)SendMessageW(lv, LVM_GETTOPINDEX, 0, 0);
    consume_cache_reset();  /* invalidate is_consumed() memoization */
    for(int i=0;i<g_nr;i++){
        int base = (g_reg[i].fc==1||g_reg[i].fc==2)?(g_reg[i].fc==2?10001:1):(g_reg[i].fc==4?30001:40001);
WCHAR b[64];wsprintfW(b,L"[%d] %04X  (%d)",g_reg[i].slave,g_reg[i].addr,base+g_reg[i].addr);
        if(i < prevCount){
            LVITEMW it={LVIF_TEXT,i,0,0,0,b};
            SendMessageW(lv,LVM_SETITEMW,0,(LPARAM)&it);
            LVITEMW s={LVIF_TEXT,i,1,0,0,g_reg[i].name};
            SendMessageW(lv,LVM_SETITEMW,0,(LPARAM)&s);
            WCHAR v[32]; fmt_reg_val(i, v, 32);
            s.iSubItem=2;s.pszText=v;SendMessageW(lv,LVM_SETITEMW,0,(LPARAM)&s);
        } else {
            LVITEMW it={LVIF_TEXT,i,0,0,0,b};int ix=(int)SendMessageW(lv,LVM_INSERTITEMW,0,(LPARAM)&it);
            LVITEMW s={LVIF_TEXT,ix,1,0,0,g_reg[i].name};SendMessageW(lv,LVM_SETITEMW,0,(LPARAM)&s);
            WCHAR v[32]; fmt_reg_val(i, v, 32);
            s.iSubItem=2;s.pszText=v;SendMessageW(lv,LVM_SETITEMW,0,(LPARAM)&s);
        }
    }
    for(int i=g_nr;i<prevCount;i++) SendMessageW(lv, LVM_DELETEITEM, g_nr, 0);
    if (topIdx >= 0 && topIdx < g_nr) SendMessageW(lv, LVM_ENSUREVISIBLE, topIdx, FALSE);
    SendMessageW(lv,WM_SETREDRAW,TRUE,0);
    ShowWindow(lv,SW_SHOW);
    InvalidateRect(GetDlgItem(g_hWnd, ID_SEND_BTN), NULL, TRUE);
}

/* forward declarations for TCP */
typedef struct { SOCKET sock; HWND hWnd; } TcpCtx;
static int build_mbap(BYTE *d, int dl, BYTE *out);
static DWORD WINAPI tcp_connect_thread(LPVOID a);
static DWORD WINAPI tcp_rd_thread(LPVOID a);
static void switch_mode(void);

static void auto_write(int sa, int addr, int val, int orig_fc) {
    if (!g_run) return;
    int tcp = (g_mode == 1);
    if (!tcp && g_hPort == INVALID_HANDLE_VALUE) return;
    if (tcp && g_sock == INVALID_SOCKET) return;
    BYTE cmd[8]; int cl;
    cmd[0] = (BYTE)sa;
    if (orig_fc == 1 || orig_fc == 2) {
        cmd[1] = 0x05;
        cmd[2] = (addr >> 8) & 0xFF;
        cmd[3] = addr & 0xFF;
        cmd[4] = val ? 0xFF : 0x00;
        cmd[5] = 0x00;
        cl = 6;
    } else {
        cmd[1] = 0x06;
        cmd[2] = (addr >> 8) & 0xFF;
        cmd[3] = addr & 0xFF;
        cmd[4] = (val >> 8) & 0xFF;
        cmd[5] = val & 0xFF;
        cl = 6;
    }
    BYTE sb[16]; int sl;
    if (tcp) {
        sl = build_mbap(cmd, cl, sb);
        send_all(g_sock, (char*)sb, sl);
    } else {
        unsigned short cr = crc16(cmd, cl);
        cmd[cl++] = cr & 0xFF; cmd[cl++] = (cr >> 8) & 0xFF;
        OVERLAPPED ow = {0}; ow.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL); DWORD wr = 0;
        if (!WriteFile(g_hPort, cmd, cl, &wr, &ow)) {
            if (GetLastError() == ERROR_IO_PENDING) GetOverlappedResult(g_hPort, &ow, &wr, TRUE);
        }
        CloseHandle(ow.hEvent);
        sl = cl; memcpy(sb, cmd, cl);
    }
    WCHAR hx[128]; fmt_hex(sb, sl, hx, 128);
    log_fmt(CLR_TX, L"[AUTO] %s", hx);
    QueryPerformanceCounter(&g_t0_send);
    g_t1_wr = g_t0_send; g_t3_ui = g_t0_send;
}

static void parse_modbus(const BYTE *d, int n) {
    if(n<5)return;
    int sa=d[0],fc=d[1],orig_fc=fc;
    if(sa<1||sa>254)return;
    /* Save existing names and user-modified values keyed by (slave,addr) */
    WCHAR sv[256][32]={0};int ssl[256],sad[256],smod[256],sval[256],sfmt[256],sbo[256],sfmts[256],snms[256],sn=0;
    for(int i=0;i<g_nr&&sn<256;i++)if(g_reg[i].name[0] || g_reg[i].mod || g_reg[i].fmt != 1 || g_reg[i].bo != 0 || g_reg[i].fmt_set || g_reg[i].name_set){
        ssl[sn]=g_reg[i].slave;sad[sn]=g_reg[i].addr;smod[sn]=g_reg[i].mod;sval[sn]=g_reg[i].val;sfmt[sn]=g_reg[i].fmt;sbo[sn]=g_reg[i].bo;sfmts[sn]=g_reg[i].fmt_set;snms[sn]=g_reg[i].name_set;wcscpy(sv[sn],g_reg[i].name);sn++;
    }
    int vs[128],vc=0,st=(g_lastSA==sa)?g_lastAD:0;
    if((fc==3||fc==4)&&n>=5){int bc=d[2];int end=3+bc;if(end>n)end=n;for(int i=3;i<end&&i+1<end;i+=2)if(vc<128)vs[vc++]=(d[i]<<8)|d[i+1];}
    else if((fc==1||fc==2)&&n>=5){int bc=d[2];for(int i=3;i<3+bc&&vc<128;i++)for(int b=0;b<8&&vc<128;b++)vs[vc++]=(d[i]>>b)&1;}
    else if((fc&0x80)&&n>=5){
        g_reg[0]=(typeof(g_reg[0])){sa,0,d[2],1,0,orig_fc,1,0};wsprintfW(g_reg[0].name,L"ERR %d",d[2]);g_nr=1;reg_table();return;
    }
    if(vc==0)return;
    g_nr=0;
    for(int i=0;i<vc;i++){
        g_reg[i]=(typeof(g_reg[0])){sa,st+i,vs[i],1,0,orig_fc,1,0};
        for(int j=0;j<sn;j++)if(ssl[j]==sa&&sad[j]==st+i){
            wcscpy(g_reg[i].name,sv[j]);
            g_reg[i].fmt=sfmt[j];g_reg[i].bo=sbo[j];g_reg[i].fmt_set=sfmts[j];g_reg[i].name_set=snms[j];
            if(smod[j] && sval[j]!=vs[i]){g_reg[i].val=sval[j];g_reg[i].mod=1;auto_write(sa,st+i,sval[j],orig_fc);}
            break;
        }
    }
    g_nr=vc;
    for (int s = 0; s < SNAP_COUNT; s++) {
        SnapState *snap = &g_snap[s];
        if (!snap->active) continue;
        for (int i = 0; i < g_nr; i++) {
            int off = g_reg[i].addr - snap->addr;
            if (off >= 0 && off < snap->qty) {
                if (!g_reg[i].fmt_set) {
                    g_reg[i].fmt = snap->fmt[off];
                    g_reg[i].bo = snap->bo[off];
                }
                if (!g_reg[i].name_set) {
                    wcscpy(g_reg[i].name, snap->name[off]);
                }
            }
        }
    }
    reg_table();
}

static void process_rx_frames(void) {
    int pos=0, tcp=(g_mode==1);
    while(pos<g_rxBufLen){
        if(!tcp){
            /* RTU: slave addr + PDU + CRC */
            if(g_rxBufLen-pos<4)break;
            int sa=g_rxBuf[pos];
            if(sa<1||sa>254){pos++;continue;}
            int fc=g_rxBuf[pos+1],fl=0;
            if(fc&0x80)fl=5;
            else if(fc==3||fc==4||fc==1||fc==2){if(g_rxBufLen-pos<5)break;int bc=g_rxBuf[pos+2];if(bc>250){pos++;continue;}fl=3+bc+2;}
            else if(fc==6||fc==0x10||fc==5||fc==0x0F)fl=8;
            else{pos++;continue;}
            if(fl<4||fl>256){pos++;continue;}
            if(g_rxBufLen-pos<fl)break;
            if(crc16(g_rxBuf+pos,fl)!=0){pos+=fl>0?fl:1;continue;}
            parse_modbus(g_rxBuf+pos,fl);
            pos+=fl;
        } else {
            /* TCP: MBAP(6) + UnitID(1) + PDU; length at bytes 4-5 */
            if(g_rxBufLen-pos<8)break;
            int mlen=(g_rxBuf[pos+4]<<8)|g_rxBuf[pos+5];
            int fl=6+mlen;
            if(fl<8||fl>2048){pos+=6;continue;}
            if(g_rxBufLen-pos<fl)break;
            parse_modbus(g_rxBuf+pos+6,mlen);
            pos+=fl;
        }
    }
    if(pos>0){g_rxBufLen-=pos;memmove(g_rxBuf,g_rxBuf+pos,g_rxBufLen);}
    if(g_rxBufLen>=3800){int d=g_rxBufLen-2048;if(d>0){memmove(g_rxBuf,g_rxBuf+d,g_rxBufLen-d);g_rxBufLen-=d;}}
}

/* ===== COM port ===== */
static void upd_ui(void);
static void refill_ports(void) {
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

static DWORD WINAPI open_thread(LPVOID a) {
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

static DWORD WINAPI rd_thread(LPVOID a) {
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



static void do_open(void) {
    if (g_run) return;
    if (g_mode == 1) {
        /* TCP mode */
        if (g_hRdTh) { WaitForSingleObject(g_hRdTh, 500); CloseHandle(g_hRdTh); g_hRdTh = NULL; }
        TcpCtx *ctx = malloc(sizeof(TcpCtx));
        if (!ctx) return;
        ctx->sock = INVALID_SOCKET; ctx->hWnd = g_hWnd;
        EnableWindow(GetDlgItem(g_hWnd, ID_OPEN_BTN), FALSE);
        EnableWindow(GetDlgItem(g_hWnd, ID_TCP_IP), FALSE);
        EnableWindow(GetDlgItem(g_hWnd, ID_TCP_PORT), FALSE);
        EnableWindow(GetDlgItem(g_hWnd, ID_RTU_RADIO), FALSE);
        EnableWindow(GetDlgItem(g_hWnd, ID_TCP_RADIO), FALSE);
        SetWindowTextW(GetDlgItem(g_hWnd, ID_OPEN_BTN), L"连接中...");
        SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 连接中...");
        { WCHAR wip[64]; GetWindowTextW(GetDlgItem(g_hWnd, ID_TCP_IP), wip, 64);
          WideCharToMultiByte(CP_ACP, 0, wip, -1, g_tcpIp, 64, NULL, NULL);
          g_tcpPort = (int)GetDlgItemInt(g_hWnd, ID_TCP_PORT, NULL, FALSE);
          if (g_tcpPort < 1 || g_tcpPort > 65535) g_tcpPort = 502; }
        HANDLE hTh = CreateThread(NULL, 0, tcp_connect_thread, ctx, 0, NULL);
        if (!hTh) {
            free(ctx); upd_ui();
            SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 就绪");
        } else {
            CloseHandle(hTh);
        }
        return;
    }
    /* RTU mode */
    HWND c = GetDlgItem(g_hWnd, ID_PORT_COMBO);
    int sel = (int)SendMessageW(c, CB_GETCURSEL, 0, 0);
    if (sel < 0) { MessageBoxW(g_hWnd, L"请先选择串口", L"提示", MB_OK | MB_ICONINFORMATION); return; }
    int com = (int)SendMessageW(c, CB_GETITEMDATA, sel, 0);
    if (com < 1 || com > 32) return;
    OpenCtx *ctx = malloc(sizeof(OpenCtx));
    if (!ctx) return;
    ctx->port = com; ctx->baud = baudrate(); ctx->hWnd = g_hWnd;
    { int x = (int)SendMessageW(GetDlgItem(g_hWnd, ID_DATA_COMBO), CB_GETCURSEL, 0, 0); int ds[] = { 8,7,6,5 }; ctx->data = (x >= 0 && x < 4) ? ds[x] : 8; }
    { int x = (int)SendMessageW(GetDlgItem(g_hWnd, ID_PARITY_COMBO), CB_GETCURSEL, 0, 0); int ps[] = { NOPARITY, EVENPARITY, ODDPARITY }; ctx->parity = (x >= 0 && x < 3) ? ps[x] : NOPARITY; }
    { int x = (int)SendMessageW(GetDlgItem(g_hWnd, ID_STOP_COMBO), CB_GETCURSEL, 0, 0); ctx->stop = (x == 1) ? TWOSTOPBITS : ONESTOPBIT; }
    EnableWindow(GetDlgItem(g_hWnd, ID_OPEN_BTN), FALSE);
    EnableWindow(c, FALSE);
    EnableWindow(GetDlgItem(g_hWnd, ID_PORT_REFRESH), FALSE);
    EnableWindow(GetDlgItem(g_hWnd, ID_RTU_RADIO), FALSE);
    EnableWindow(GetDlgItem(g_hWnd, ID_TCP_RADIO), FALSE);
    SetWindowTextW(GetDlgItem(g_hWnd, ID_OPEN_BTN), L"打开中...");
    SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 打开中...");
    HANDLE hTh2 = CreateThread(NULL, 0, open_thread, ctx, 0, NULL);
    if (!hTh2) {
        free(ctx); upd_ui();
        SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 就绪");
    } else {
        CloseHandle(hTh2);
    }
}

static void do_close(void) {
    if (g_scan) { g_scanStop = 1; int w = 0; while (g_scan && w < 300) { Sleep(10); w++; } }
    g_run = 0; g_pauseRd = 0;
    KillTimer(g_hWnd, TIMER_SEND);
    if (g_mode == 1) {
        if (g_sock != INVALID_SOCKET) { shutdown(g_sock, SD_BOTH); closesocket(g_sock); g_sock = INVALID_SOCKET; }
        SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 就绪");
        SetWindowTextW(g_hConn, L"");
        log_fmt(CLR_SYS, L"--- TCP 已断开 ---");
        upd_ui();
        return;
    }
    if (g_hPort != INVALID_HANDLE_VALUE) { CancelIoEx(g_hPort, NULL); g_hPort = INVALID_HANDLE_VALUE; }
    if (g_hRdTh) { WaitForSingleObject(g_hRdTh, 2000); CloseHandle(g_hRdTh); g_hRdTh = NULL; }
    SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 就绪");
    SetWindowTextW(g_hConn, L"");
    log_fmt(CLR_SYS, L"--- 串口已关闭 ---");
    upd_ui();
}

/* ===== send ===== */
static void do_send(BOOL silent) {
    if (!g_run) return;
    int tcp = (g_mode == 1);
    if (!tcp && g_hPort == INVALID_HANDLE_VALUE) return;
    if (tcp && g_sock == INVALID_SOCKET) return;
    QueryPerformanceCounter(&g_t0_send);
    WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
    if (!raw[0]) return;
    BOOL mb = SendMessageW(GetDlgItem(g_hWnd, ID_MODBUS_CHK), BM_GETCHECK, 0, 0) == BST_CHECKED;
    BYTE d[2048]; int dl = parse_hex_send(raw, d, 2048);
    if (dl < 0) { if (!silent) MessageBoxW(g_hWnd, L"发送区HEX格式无效", L"错误", MB_OK | MB_ICONERROR); return; }
    if (!dl) return;
    if (dl >= 6) { g_lastSA = d[0]; g_lastFC = d[1]; g_lastAD = (d[2] << 8) | d[3]; g_lastQN = (d[4] << 8) | d[5]; }

    BYTE sb[2060]; int sl;
    if (tcp && mb && dl >= 2) {
        sl = build_mbap(d, dl, sb);
    } else {
        sl = dl; memcpy(sb, d, dl);
        if (mb && dl >= 2) { unsigned short cr = crc16(d, dl); sb[sl++] = cr & 0xFF; sb[sl++] = (cr >> 8) & 0xFF; }
    }

    if (tcp) {
        if (send_all(g_sock, (char*)sb, sl) < 0) return;
    } else {
        OVERLAPPED ovw = { 0 }; ovw.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL); DWORD wr = 0;
        if (!WriteFile(g_hPort, sb, sl, &wr, &ovw)) {
            if (GetLastError() == ERROR_IO_PENDING) GetOverlappedResult(g_hPort, &ovw, &wr, TRUE);
        }
        CloseHandle(ovw.hEvent);
        }
    QueryPerformanceCounter(&g_t1_wr);

    WCHAR hx2[4096]; fmt_hex(sb, sl, hx2, 4096);
    log_data(hx2, CLR_TX);
    /* auto-format send box: normalize hex spacing */
    {   WCHAR fmt[4096]; fmt_hex(d, dl, fmt, 4096);
        if (wcscmp(raw, fmt) != 0) {
            g_bSyncing = 1;
            SetWindowTextW(g_hSend, fmt);
            g_bSyncing = 0;
        }
    }
}

/* ===== scan ===== */
typedef struct { HANDLE hPort; SOCKET sock; int start, end, ival, tout; BYTE tmpl[2048]; int tlen; int tcp; char ipBase[64]; int method; BYTE sendBuf[2048]; int sendLen; int tcpPort; } SI;

typedef struct { char ipBase[64]; int start, end; DWORD tout; } PingArg;
typedef struct { char ipBase[64]; int start, end; DWORD tout; BYTE sendBuf[2048]; int sendLen; int tcpPort; } ModbusScanArg;

static DWORD WINAPI ping_worker(LPVOID p) {
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

static int build_mbap(BYTE *d, int dl, BYTE *out) {
    int tid = (int)InterlockedIncrement(&g_mbap_tid);
    out[0] = (tid >> 8) & 0xFF; out[1] = tid & 0xFF;
    out[2] = 0; out[3] = 0;
    out[4] = (dl >> 8) & 0xFF; out[5] = dl & 0xFF;
    memcpy(out + 6, d, dl);
    return dl + 6;
}

static DWORD WINAPI modbus_scan_worker(LPVOID p) {
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

static void fmt_found_str(WCHAR *buf, int bufSize, int *found, int fc) {
    int pos = 0;
    for (int i = 0; i < fc && pos < bufSize - 1; i++) {
        int n = wsprintfW(buf + pos, i ? L" %d" : L"%d", found[i]);
        if (pos + n >= bufSize - 1) { pos = bufSize - 1; break; }
        pos += n;
    }
    buf[pos] = 0;
}

static DWORD WINAPI scan_thread(LPVOID a) {
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
}

static void start_scan(void) {
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
}

static unsigned short bswap16(unsigned short v) { return ((v >> 8) | (v << 8)) & 0xFFFF; }

static void commit_edit(HWND hEdit) {
    WCHAR txt[32] = {0}; GetWindowTextW(hEdit, txt, 32);
    if (g_editCol == 1) {
        wcscpy(g_reg[g_editItem].name, txt);
        g_reg[g_editItem].name_set = 1;
        LVITEMW s = {LVIF_TEXT, g_editItem, 1, 0, 0, g_reg[g_editItem].name};
        SendMessageW(g_hRegList, LVM_SETITEMW, 0, (LPARAM)&s);
    } else if (g_editCol == 2) {
        int f = g_reg[g_editItem].fmt, bo = g_reg[g_editItem].bo;
        int nr = 1;
        unsigned long long rawVal = 0;
        /* parse input text based on format */
        if (f == 0) { int v = 0; swscanf(txt, L"%d", &v); rawVal = (unsigned long long)(unsigned short)v; }
        else if (f == 2) { unsigned v = 0; swscanf(txt, L"%x", &v); rawVal = v & 0xFFFF; }
        else if (f == 3) { unsigned v = 0; for (WCHAR *p = txt; *p; p++) v = (v << 1) | (*p == L'1'); rawVal = v & 0xFFFF; }
        else if (f == 4) { int v = 0; swscanf(txt, L"%d", &v); memcpy(&rawVal, &v, 4); rawVal &= 0xFFFFFFFFULL; nr = 2; }
        else if (f == 5) { unsigned v = 0; swscanf(txt, L"%u", &v); rawVal = v; nr = 2; }
        else if (f == 6) { long long v = 0; swscanf(txt, L"%lld", &v); memcpy(&rawVal, &v, 8); nr = 4; }
        else if (f == 7) { swscanf(txt, L"%llu", &rawVal); nr = 4; }
        else if (f == 8) { float fv = 0; swscanf(txt, L"%f", &fv); memcpy(&rawVal, &fv, 4); rawVal &= 0xFFFFFFFFULL; nr = 2; }
        else if (f == 9) { double dv = 0; swscanf(txt, L"%lf", &dv); memcpy(&rawVal, &dv, 8); nr = 4; }
        else { unsigned v = 0; swscanf(txt, L"%u", &v); rawVal = v & 0xFFFF; nr = 1; }

        if (nr == 1) {
            unsigned short nv = (unsigned short)(rawVal & 0xFFFF);
            g_reg[g_editItem].val = (int)nv; g_reg[g_editItem].mod = 1;
            WCHAR v[32]; wsprintfW(v, L"%u", nv);
            LVITEMW s = {LVIF_TEXT, g_editItem, 2, 0, 0, v};
            SendMessageW(g_hRegList, LVM_SETITEMW, 0, (LPARAM)&s);
            if (nv != (unsigned short)g_editOldVal) auto_write(g_reg[g_editItem].slave, g_reg[g_editItem].addr, (int)nv, g_reg[g_editItem].fc);
        } else {
            unsigned long long oldRaw = 0;
            int changed = !get_multi(g_editItem, nr, bo, &oldRaw) || (oldRaw != rawVal);
            if (changed) {
                unsigned short regs[4] = {0};
                if (nr == 2) {
                    regs[0] = (unsigned short)((rawVal >> 16) & 0xFFFF);
                    regs[1] = (unsigned short)(rawVal & 0xFFFF);
                    if (bo & 1) { unsigned short t = regs[0]; regs[0] = regs[1]; regs[1] = t; }
                    if (bo & 2) { regs[0] = bswap16(regs[0]); regs[1] = bswap16(regs[1]); }
                } else {
                    regs[0] = (unsigned short)((rawVal >> 48) & 0xFFFF);
                    regs[1] = (unsigned short)((rawVal >> 32) & 0xFFFF);
                    regs[2] = (unsigned short)((rawVal >> 16) & 0xFFFF);
                    regs[3] = (unsigned short)(rawVal & 0xFFFF);
                    if (bo & 1) { for (int i = 0; i < 2; i++) { unsigned short t = regs[i]; regs[i] = regs[3-i]; regs[3-i] = t; } }
                    if (bo & 2) { for (int i = 0; i < 4; i++) regs[i] = bswap16(regs[i]); }
                }
                int sa = g_reg[g_editItem].slave, fc = g_reg[g_editItem].fc;
                for (int i = 0; i < nr; i++) {
                    int idx = g_editItem + i;
                    if (idx >= g_nr) break;
                    g_reg[idx].val = (int)regs[i]; g_reg[idx].mod = 1;
                    WCHAR v[32]; wsprintfW(v, L"%u", regs[i]);
                    LVITEMW s = {LVIF_TEXT, idx, 2, 0, 0, v};
                    SendMessageW(g_hRegList, LVM_SETITEMW, 0, (LPARAM)&s);
                    auto_write(sa, g_reg[idx].addr, (int)regs[i], fc);
                }
            }
        }
        reg_table();
    }
    g_editItem = -1; g_editCol = 0;
    RemoveWindowSubclass(hEdit, ec_proc, 0);
    DestroyWindow(hEdit); g_hEditCell = NULL;
}

/* ===== Bit editor popup ===== */
static int g_bitEdItem = -1;
static int g_bitEdAutoSend = 1;  /* 1=auto-send on bit click */
static int g_bitEdReadOnly = 0;  /* 1=read-only (FC 02/04) */
typedef struct { int lo, hi; HWND hBox; } BitGroup;
static BitGroup g_bitGroups[8];
static int g_bitGroupCnt = 0;
static int g_selStart = -1, g_selEnd = -1;  /* Ctrl select: -1=idle */
static int g_bitSync = 0;  /* prevent EN_CHANGE recursion */

static LRESULT CALLBACK bited_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_PAINT: {
        if (g_bitEdItem < 0 || g_bitEdItem >= g_nr) { DestroyWindow(h); return 0; }
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH br = CreateSolidBrush(RGB(250,250,245));
        FillRect(dc, &rc, br); DeleteObject(br);

        int sq = 20, gap = 3, margin = 5, v = g_reg[g_bitEdItem].val;
        /* cached fonts — create once, reuse across repaints */
        static HFONT s_bitBold = NULL, s_bitSmall = NULL;
        if (!s_bitBold) s_bitBold = CreateFontW(14, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
        if (!s_bitSmall) s_bitSmall = CreateFontW(11, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Consolas");
        HFONT bf = s_bitBold, sf = s_bitSmall;
        for (int b = 15; b >= 0; b--) {
            int x = margin + (15 - b) * (sq + gap), bitVal = (v >> b) & 1;
            RECT brc = {x, 4, x + sq, 4 + sq};
            HBRUSH fill = CreateSolidBrush(bitVal ? RGB(76, 175, 80) : RGB(210, 210, 210));
            FillRect(dc, &brc, fill); DeleteObject(fill);
            DrawEdge(dc, &brc, EDGE_RAISED, BF_RECT);
            HFONT oldF = SelectObject(dc, sf);
            SetBkMode(dc, TRANSPARENT); SetTextColor(dc, RGB(100, 100, 100));
            WCHAR bl[4]; wsprintfW(bl, L"%d", b);
            RECT lr = {x, 4 + sq, x + sq, 4 + sq + 16};
            DrawTextW(dc, bl, -1, &lr, DT_CENTER | DT_TOP | DT_SINGLELINE);
            SelectObject(dc, oldF);
            if (bitVal) {
                SelectObject(dc, bf); SetTextColor(dc, RGB(255, 255, 255));
                WCHAR ov[2] = {L'1', 0};
                RECT ovr = {x, 2, x + sq, 4 + sq};
                DrawTextW(dc, ov, -1, &ovr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                SelectObject(dc, oldF);
            }
        }
        /* group highlights */
        for (int i = 0; i < g_bitGroupCnt; i++) {
            HPEN hp = CreatePen(PS_SOLID, 2, RGB(33, 150, 243));
            HPEN oldP = SelectObject(dc, hp);
            HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH oldB = SelectObject(dc, nb);
            for (int b = g_bitGroups[i].lo; b <= g_bitGroups[i].hi; b++) {
                int x = margin + (15 - b) * (sq + gap);
                Rectangle(dc, x - 1, 3, x + sq + 1, 5 + sq);
            }
            SelectObject(dc, oldP); SelectObject(dc, oldB); DeleteObject(hp);
        }
        /* current Ctrl selection */
        if (g_selStart >= 0) {
            int lo = g_selStart, hi = g_selEnd >= 0 ? g_selEnd : g_selStart;
            if (lo > hi) { int t = lo; lo = hi; hi = t; }
            HPEN hp = CreatePen(PS_SOLID, 2, RGB(255, 100, 0));
            HPEN oldP = SelectObject(dc, hp);
            HBRUSH nb = (HBRUSH)GetStockObject(NULL_BRUSH);
            HBRUSH oldB = SelectObject(dc, nb);
            for (int b = lo; b <= hi; b++) {
                int x = margin + (15 - b) * (sq + gap);
                Rectangle(dc, x - 1, 3, x + sq + 1, 5 + sq);
            }
            SelectObject(dc, oldP); SelectObject(dc, oldB); DeleteObject(hp);
        }
        /* fonts are static — no DeleteObject needed */
        EndPaint(h, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        if (g_bitEdItem < 0 || g_bitEdItem >= g_nr) { DestroyWindow(h); return 0; }
        int sq = 20, gap = 3, margin = 5;
        int mx = (int)(short)LOWORD(l), my = (int)(short)HIWORD(l);
        int ctrlNow = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
        if (ctrlNow) {
            for (int b = 15; b >= 0; b--) {
                int x = margin + (15 - b) * (sq + gap);
                if (mx >= x && mx < x + sq && my >= 4 && my < 4 + sq) {
                    if (g_selStart < 0) { g_selStart = g_selEnd = b; }
                    else { g_selEnd = b; }
                    if (g_selStart >= 0 && g_selEnd >= 0 && g_selStart != g_selEnd && g_bitGroupCnt < 8) {
                        int lo = g_selStart < g_selEnd ? g_selStart : g_selEnd;
                        int hi = g_selStart > g_selEnd ? g_selStart : g_selEnd;
                        g_bitGroups[g_bitGroupCnt].lo = lo;
                        g_bitGroups[g_bitGroupCnt].hi = hi;
                        int ix = margin + (15 - hi) * (sq + gap);
                        int iw = (hi - lo + 1) * (sq + gap) - gap;
                        int mask = (1 << (hi - lo + 1)) - 1;
                        int val = (g_reg[g_bitEdItem].val >> lo) & mask;
                        WCHAR init[16]; wsprintfW(init, L"%d", val);
                        DWORD style = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER;
                        if (g_bitEdReadOnly) style |= ES_READONLY;
                        g_bitGroups[g_bitGroupCnt].hBox = CreateWindowW(L"EDIT", init,
                            style, ix, 42, iw, 20, h, (HMENU)(INT_PTR)(200 + g_bitGroupCnt), NULL, NULL);
                        SendMessageW(g_bitGroups[g_bitGroupCnt].hBox, WM_SETFONT,
                            (WPARAM)CreateFontW(13,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,
                            OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                            DEFAULT_PITCH|FF_DONTCARE,L"Consolas"), TRUE);
                        g_bitGroupCnt++;
                        g_selStart = g_selEnd = -1;
                    }
                    InvalidateRect(h, NULL, FALSE);
                    return 0;
                }
            }
            return 0;
        }
        /* normal click: toggle bit (skip if read-only) */
        if (g_bitEdReadOnly) return 0;
        for (int b = 15; b >= 0; b--) {
            int x = margin + (15 - b) * (sq + gap);
            if (mx >= x && mx < x + sq && my >= 4 && my < 4 + sq) {
                g_reg[g_bitEdItem].val ^= (1 << b);
                g_reg[g_bitEdItem].mod = 1;
                if (SendMessageW(GetDlgItem(h, 100), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    auto_write(g_reg[g_bitEdItem].slave, g_reg[g_bitEdItem].addr,
                        g_reg[g_bitEdItem].val, g_reg[g_bitEdItem].fc);
                    reg_table();
                }
                for (int i = 0; i < g_bitGroupCnt; i++) {
                    int mask = (1 << (g_bitGroups[i].hi - g_bitGroups[i].lo + 1)) - 1;
                    int gv = (g_reg[g_bitEdItem].val >> g_bitGroups[i].lo) & mask;
                    WCHAR buf[16]; wsprintfW(buf, L"%d", gv);
                    SetWindowTextW(g_bitGroups[i].hBox, buf);
                }
                InvalidateRect(h, NULL, FALSE);
                return 0;
            }
        }
        return 0;
    }
    case WM_COMMAND: {
        WORD id = LOWORD(w);
        if (id == 100 && HIWORD(w) == BN_CLICKED) {
            g_bitEdAutoSend = (SendMessageW(GetDlgItem(h, 100), BM_GETCHECK, 0, 0) == BST_CHECKED);
            return 0;
        }
        if (id == 101 && HIWORD(w) == BN_CLICKED) {
            if (g_bitEdItem >= 0 && g_bitEdItem < g_nr) {
                auto_write(g_reg[g_bitEdItem].slave, g_reg[g_bitEdItem].addr,
                    g_reg[g_bitEdItem].val, g_reg[g_bitEdItem].fc);
                reg_table();
            }
            return 0;
        }
        if (id >= 200 && id < 208 && HIWORD(w) == EN_CHANGE) {
            if (g_bitSync || g_bitEdReadOnly) return 0;
            int gi = id - 200;
            if (gi >= g_bitGroupCnt) return 0;
            g_bitSync = 1;
            int mask = (1 << (g_bitGroups[gi].hi - g_bitGroups[gi].lo + 1)) - 1;
            int v = (int)GetDlgItemInt(h, id, NULL, FALSE);
            if (v > mask) v = mask;
            g_reg[g_bitEdItem].val &= ~(mask << g_bitGroups[gi].lo);
            g_reg[g_bitEdItem].val |= (v & mask) << g_bitGroups[gi].lo;
            g_reg[g_bitEdItem].mod = 1;
            for (int j = 0; j < g_bitGroupCnt; j++) if (j != gi) {
                int m2 = (1 << (g_bitGroups[j].hi - g_bitGroups[j].lo + 1)) - 1;
                int gv = (g_reg[g_bitEdItem].val >> g_bitGroups[j].lo) & m2;
                WCHAR buf[16]; wsprintfW(buf, L"%d", gv);
                SetWindowTextW(g_bitGroups[j].hBox, buf);
            }
            if (SendMessageW(GetDlgItem(h, 100), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                auto_write(g_reg[g_bitEdItem].slave, g_reg[g_bitEdItem].addr,
                    g_reg[g_bitEdItem].val, g_reg[g_bitEdItem].fc);
                reg_table();
            }
            InvalidateRect(h, NULL, FALSE);
            g_bitSync = 0;
            return 0;
        }
        break;
    }
    case WM_KEYDOWN:
        if (w == VK_RETURN) { SetFocus(h); return 0; }
        if (w == VK_ESCAPE) {
            for (int i = 0; i < g_bitGroupCnt; i++)
                if (g_bitGroups[i].hBox) DestroyWindow(g_bitGroups[i].hBox);
            g_bitGroupCnt = 0;
            g_selStart = g_selEnd = -1;
            InvalidateRect(h, NULL, FALSE);
            return 0;
        }
        break;
    case WM_ACTIVATE:
        if (LOWORD(w) == WA_INACTIVE) DestroyWindow(h);
        return 0;
    case WM_DESTROY:
        for (int i = 0; i < g_bitGroupCnt; i++)
            if (g_bitGroups[i].hBox) DestroyWindow(g_bitGroups[i].hBox);
        g_bitGroupCnt = 0;
        g_selStart = g_selEnd = -1;
        g_hBitEd = NULL; g_bitEdItem = -1;
        g_editItem = -1; g_editCol = 0;
        /* cached fonts are cleaned when ComProBitEd class is unregistered (process exit) */
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}
static void show_bit_editor(HWND parent, HINSTANCE hi, int item) {
    static int clsReg = 0;
    if (!clsReg) {
        WNDCLASSW wc = {0};
        wc.lpfnWndProc = bited_proc;
        wc.hInstance = hi;
        wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.lpszClassName = L"ComProBitEd";
        RegisterClassW(&wc);
        clsReg = 1;
    }
    g_hBitEd = NULL; g_bitEdItem = item;
    g_editItem = item; g_editCol = 2;
    g_editOldVal = g_reg[item].val;
    /* position popup above the table, left-aligned */
    RECT tr; GetWindowRect(g_hRegList, &tr);
    int popW = 410, popH = 100;
    int popX = tr.left;
    if (popX + popW > GetSystemMetrics(SM_CXSCREEN)) popX = GetSystemMetrics(SM_CXSCREEN) - popW - 10;
    int popY = tr.top - popH - 2;
    if (popY < 0) popY = tr.bottom + 2;
    g_hBitEd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"ComProBitEd", L"",
        WS_POPUP | WS_BORDER,
        popX, popY, popW, popH, parent, NULL, hi, NULL);
    /* child controls (hidden in read-only mode) */
    if (!g_bitEdReadOnly) {
        HWND hChk = CreateWindowW(L"BUTTON", L"立即发送", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            8, 76, 90, 20, g_hBitEd, (HMENU)100, hi, NULL);
        SendMessageW(hChk, WM_SETFONT, (WPARAM)g_hMono, TRUE);
        SendMessageW(hChk, BM_SETCHECK, g_bitEdAutoSend ? BST_CHECKED : BST_UNCHECKED, 0);
        HWND hBtn = CreateWindowW(L"BUTTON", L"发送", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            popW - 62, 75, 52, 22, g_hBitEd, (HMENU)101, hi, NULL);
        SendMessageW(hBtn, WM_SETFONT, (WPARAM)g_hMono, TRUE);
    }
    ShowWindow(g_hBitEd, SW_SHOW);
    SetFocus(g_hBitEd);
}

/* ===== Edit cell subclass ===== */
static LRESULT CALLBACK ec_proc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR idSubclass, DWORD_PTR refData) {
    if (m == WM_KEYDOWN && w == VK_ESCAPE) {
        g_editItem = -1; g_editCol = 0;
        RemoveWindowSubclass(h, ec_proc, 0);
        DestroyWindow(h); g_hEditCell = NULL;
        return 0;
    }
    if (m == WM_KILLFOCUS) {
        if (g_editItem >= 0) commit_edit(h);
        return 0;
    }
    return DefSubclassProc(h, m, w, l);
}

/* ===== Scan result subclass: custom paint with alternating IP colors + double-click fill ===== */
static LRESULT CALLBACK sr_proc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR idSubclass, DWORD_PTR refData) {
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        int sw = GetSystemMetrics(SM_CXVSCROLL);
        int lineH = RH - 4;
        int maxX = rc.right - 5 - sw;
        if (maxX < 40) maxX = 40;

        /* layout pass — measure total height */
        int totalH = lineH;
        {
            WCHAR *p = g_scanResult; int x = 5;
            while (*p) {
                WCHAR *seg = p; int segLen = 0;
                if (*p == L'[') {
                    while (*p && *p != L']') { segLen++; p++; }
                    if (*p == L']') { segLen++; p++; }
                } else if (*p == L' ' || *p == L'|') {
                    while (*p == L' ' || *p == L'|') { segLen++; p++; }
                } else {
                    while (*p && *p != L' ' && *p != L'|' && *p != L'[') { segLen++; p++; }
                }
                if (segLen > 0) {
                    SIZE sz; GetTextExtentPoint32W(dc, seg, segLen, &sz);
                    if (x + sz.cx > maxX && x > 5) { x = 5; totalH += lineH; }
                    x += sz.cx;
                }
            }
        }

        /* clamp scroll */
        int maxScroll = totalH - rc.bottom;
        if (maxScroll < 0) maxScroll = 0;
        if (g_srScrollY > maxScroll) g_srScrollY = maxScroll;
        if (g_srScrollY < 0) g_srScrollY = 0;

        SCROLLINFO si = {sizeof(si), SIF_RANGE | SIF_PAGE | SIF_POS};
        si.nMin = 0; si.nMax = totalH; si.nPage = rc.bottom; si.nPos = g_srScrollY;
        SetScrollInfo(h, SB_VERT, &si, TRUE);

        /* double-buffer draw */
        HDC memDC = CreateCompatibleDC(dc);
        HBITMAP memBmp = CreateCompatibleBitmap(dc, rc.right, rc.bottom);
        HBITMAP oldBmp = (HBITMAP)SelectObject(memDC, memBmp);

        HBRUSH br = CreateSolidBrush(RGB(255,255,255));
        FillRect(memDC, &rc, br);
        DeleteObject(br);

        SetBkMode(memDC, TRANSPARENT);
        HFONT oldFont = (HFONT)SelectObject(memDC, g_hMono);

        WCHAR *txt = g_scanResult;
        int x = 5, y = 2 - g_srScrollY, ipIdx = 0;
        WCHAR *p = txt;
        while (*p) {
            WCHAR *seg = p; int segLen = 0;
            COLORREF clr = CLR_SYS;
            if (*p == L'[') {
                while (*p && *p != L']') { segLen++; p++; }
                if (*p == L']') { segLen++; p++; }
                clr = (ipIdx % 2) ? CLR_IP_B : CLR_IP_A; ipIdx++;
            } else if (*p == L' ' || *p == L'|') {
                while (*p == L' ' || *p == L'|') { segLen++; p++; }
            } else {
                while (*p && *p != L' ' && *p != L'|' && *p != L'[') { segLen++; p++; }
                clr = (ipIdx % 2) ? CLR_IP_B : CLR_IP_A; ipIdx++;
            }
            if (segLen > 0) {
                SIZE sz; GetTextExtentPoint32W(memDC, seg, segLen, &sz);
                if (x + sz.cx > maxX && x > 5) { x = 5; y += lineH; }
                SetTextColor(memDC, clr);
                if (y + lineH > 0 && y < rc.bottom)
                    TextOutW(memDC, x, y, seg, segLen);
                x += sz.cx;
            }
        }

        SelectObject(memDC, oldFont);
        BitBlt(dc, 0, 0, rc.right, rc.bottom, memDC, 0, 0, SRCCOPY);
        SelectObject(memDC, oldBmp);
        DeleteObject(memBmp);
        DeleteDC(memDC);
        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_VSCROLL) {
        RECT rc; GetClientRect(h, &rc);
        SCROLLINFO si = {sizeof(si), SIF_ALL};
        GetScrollInfo(h, SB_VERT, &si);
        int lineH = RH - 4;
        int newY = g_srScrollY;
        switch (LOWORD(w)) {
        case SB_LINEUP:    newY -= lineH; break;
        case SB_LINEDOWN:  newY += lineH; break;
        case SB_PAGEUP:    newY -= si.nPage; break;
        case SB_PAGEDOWN:  newY += si.nPage; break;
        case SB_THUMBTRACK: newY = HIWORD(w); break;
        }
        int maxScroll = si.nMax - (int)si.nPage;
        if (maxScroll < 0) maxScroll = 0;
        if (newY < 0) newY = 0;
        if (newY > maxScroll) newY = maxScroll;
        if (newY != g_srScrollY) {
            g_srScrollY = newY;
            InvalidateRect(h, NULL, TRUE);
        }
        return 0;
    }
    if (m == WM_MOUSEWHEEL) {
        int lineH = RH - 4;
        int delta = (short)HIWORD(w);
        g_srScrollY -= (delta / WHEEL_DELTA) * lineH * 3;
        RECT rc; GetClientRect(h, &rc);
        SCROLLINFO si = {sizeof(si), SIF_ALL};
        GetScrollInfo(h, SB_VERT, &si);
        int maxScroll = si.nMax - (int)si.nPage;
        if (maxScroll < 0) maxScroll = 0;
        if (g_srScrollY < 0) g_srScrollY = 0;
        if (g_srScrollY > maxScroll) g_srScrollY = maxScroll;
        InvalidateRect(h, NULL, TRUE);
        return 0;
    }
    if (m == WM_LBUTTONDBLCLK) {
        DWORD pos = GetMessagePos();
        POINT pt = { (int)(short)LOWORD(pos), (int)(short)HIWORD(pos) };
        ScreenToClient(h, &pt);
        pt.y += g_srScrollY;
        if (g_scanResult[0] == 0) return 0;
        RECT rc; GetClientRect(h, &rc);
        int sw = GetSystemMetrics(SM_CXVSCROLL);
        int maxX = rc.right - 5 - sw;
        if (maxX < 40) maxX = 40;
        int lineH = RH - 4;
        HDC dc = GetDC(h); SelectObject(dc, g_hMono);
        WCHAR *txt = g_scanResult; int x = 5, y = 2;
        WCHAR *p = txt;
        while (*p) {
            WCHAR *seg = p; int segLen = 0; BOOL isIp = FALSE;
            if (*p == L'[') {
                while (*p && *p != L']') { segLen++; p++; }
                if (*p == L']') { segLen++; p++; }
                isIp = TRUE;
            } else if (*p == L' ' || *p == L'|') {
                while (*p == L' ' || *p == L'|') { segLen++; p++; }
            } else {
                while (*p && *p != L' ' && *p != L'|' && *p != L'[') { segLen++; p++; }
            }
            if (segLen > 0) {
                SIZE sz; GetTextExtentPoint32W(dc, seg, segLen, &sz);
                if (x + sz.cx > maxX && x > 5) { x = 5; y += lineH; }
                if (pt.x >= x && pt.x < x + sz.cx && pt.y >= y && pt.y < y + sz.cy) {
                    if (isIp) {
                        WCHAR ip[64] = {0}; int ci = 0;
                        WCHAR *q = seg + 1;
                        while (*q && *q != L']' && ci < 63) ip[ci++] = *q++;
                        ip[ci] = 0;
                        ReleaseDC(h, dc);
                        /* toggle: same IP → disconnect; different IP → reconnect */
                        if (g_sock != INVALID_SOCKET) {
                            WCHAR curIp[64]; GetWindowTextW(GetDlgItem(g_hWnd, ID_TCP_IP), curIp, 64);
                            if (wcscmp(ip, curIp) == 0) { do_close(); return 0; }
                            do_close();
                        }
                        SetWindowTextW(GetDlgItem(g_hWnd, ID_TCP_IP), ip);
                        if (g_mode != 1) { g_mode = 1; CheckRadioButton(g_hWnd, ID_RTU_RADIO, ID_TCP_RADIO, ID_TCP_RADIO); switch_mode(); }
                        do_open();
                        return 0;
                    } else {
                        int addr = 0;
                        for (WCHAR *q = seg; *q >= L'0' && *q <= L'9'; q++) addr = addr * 10 + (*q - L'0');
                        if (addr >= 1 && addr <= 254) {
                            WCHAR dec[16]; wsprintfW(dec, L"%d", addr);
                            ReleaseDC(h, dc);
                            SetWindowTextW(GetDlgItem(g_hWnd, ID_SLAVEADDR_DEC), dec);
                            sync_slave_to_send();
                            return 0;
                        }
                    }
                }
                x += sz.cx;
            }
        }
        ReleaseDC(h, dc);
        return 0;
    }
    return DefSubclassProc(h, m, w, l);
}

/* ===== ListView subclass for alternating row colors ===== */
static LRESULT CALLBACK lv_proc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR idSubclass, DWORD_PTR refData) {
    if (m == WM_NOTIFY) {
        NMHDR *nm = (NMHDR*)l;
        if (nm->idFrom == ID_REG_LIST && nm->code == NM_CUSTOMDRAW) {
            NMLVCUSTOMDRAW *cd = (NMLVCUSTOMDRAW*)l;
            if (cd->nmcd.dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
            if (cd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT) {
                cd->clrTextBk = ((int)cd->nmcd.dwItemSpec % 2) ? RGB(242, 245, 250) : RGB(253, 253, 252);
                return CDRF_NEWFONT;
            }
        }
    }
    return DefSubclassProc(h, m, w, l);
}

/* ===== UI ===== */
static void upd_ui(void) {
    HWND b = GetDlgItem(g_hWnd, ID_OPEN_BTN);
    HWND tc = GetDlgItem(g_hWnd, ID_TIMER_CHK);
    int rtumode = (g_mode == 0);
    for (int id = ID_PORT_COMBO; id <= ID_STOP_COMBO; id++) EnableWindow(GetDlgItem(g_hWnd, id), !g_run && rtumode);
    EnableWindow(GetDlgItem(g_hWnd, ID_PORT_REFRESH), !g_run && rtumode);
    EnableWindow(GetDlgItem(g_hWnd, ID_TCP_IP), !g_run && !rtumode);
    EnableWindow(GetDlgItem(g_hWnd, ID_TCP_PORT), !g_run && !rtumode);
    EnableWindow(GetDlgItem(g_hWnd, ID_RTU_RADIO), !g_run);
    EnableWindow(GetDlgItem(g_hWnd, ID_TCP_RADIO), !g_run);
    BOOL timerActive = FALSE;
    if (g_run) {
        EnableWindow(b, TRUE);
        SetWindowTextW(b, rtumode ? L"关闭串口" : L"断开");
        KillTimer(g_hWnd, TIMER_COM_SCAN);
        if (SendMessageW(tc, BM_GETCHECK, 0, 0) == BST_CHECKED) {
            int ms = (int)GetDlgItemInt(g_hWnd, ID_TIMER_MS, NULL, FALSE);
            if (ms < 50) ms = 50;
            SetTimer(g_hWnd, TIMER_SEND, ms, NULL);
            timerActive = TRUE;
        } else {
            KillTimer(g_hWnd, TIMER_SEND);
        }
    } else {
        EnableWindow(b, TRUE);
        SetWindowTextW(b, rtumode ? L"打开串口" : L"连接");
        KillTimer(g_hWnd, TIMER_SEND);
        SetWindowTextW(GetDlgItem(g_hWnd, ID_SCAN_BTN), L"开始扫描");
        if (rtumode) SetTimer(g_hWnd, TIMER_COM_SCAN, 2000, NULL);
    }
    EnableWindow(GetDlgItem(g_hWnd, ID_TIMER_MS), !timerActive);
    EnableWindow(GetDlgItem(g_hWnd, ID_TIMER_CHK), g_run);
}

/* ===== TCP helpers ===== */
static void switch_mode(void) {
    scan_result_set(L"");
    g_pingFc = 0;
    int rtumode = (g_mode == 0);
    /* serial controls */
    int ser_ids[] = {ID_PORT_COMBO, ID_PORT_REFRESH, ID_BAUD_COMBO, ID_DATA_COMBO, ID_PARITY_COMBO, ID_STOP_COMBO};
    for (int i = 0; i < (int)(sizeof(ser_ids)/sizeof(ser_ids[0])); i++)
        ShowWindow(GetDlgItem(g_hWnd, ser_ids[i]), rtumode ? SW_SHOW : SW_HIDE);
    /* serial labels — find and toggle by iterating children */
    HWND child = GetWindow(g_hWnd, GW_CHILD);
    WCHAR buf[16];
    while (child) {
        GetClassNameW(child, buf, 16);
        if (wcscmp(buf, L"Static") == 0) {
            GetWindowTextW(child, buf, 16);
            if (wcscmp(buf, L"串口") == 0 || wcscmp(buf, L"波特") == 0 ||
                wcscmp(buf, L"数据") == 0 || wcscmp(buf, L"校验") == 0 ||
                wcscmp(buf, L"停止") == 0)
                ShowWindow(child, rtumode ? SW_SHOW : SW_HIDE);
        }
        child = GetWindow(child, GW_HWNDNEXT);
    }
    /* TCP controls */
    ShowWindow(GetDlgItem(g_hWnd, ID_TCP_IP), rtumode ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(g_hWnd, ID_TCP_PORT), rtumode ? SW_HIDE : SW_SHOW);
    if (g_hTcpIpLbl) ShowWindow(g_hTcpIpLbl, rtumode ? SW_HIDE : SW_SHOW);
    if (g_hTcpPortLbl) ShowWindow(g_hTcpPortLbl, rtumode ? SW_HIDE : SW_SHOW);
    /* scan label */
    if (g_hScanLbl) SetWindowTextW(g_hScanLbl, rtumode ? L"从站扫描" : L"IP扫描");
    /* scan controls: RTU show, TCP hide */
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_START), rtumode ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_END), rtumode ? SW_SHOW : SW_HIDE);
    if (g_hScanIvalLbl) ShowWindow(g_hScanIvalLbl, rtumode ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_IVAL), rtumode ? SW_SHOW : SW_HIDE);
    /* scan controls: TCP show, RTU hide */
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_TCP_START), rtumode ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_TCP_END), rtumode ? SW_HIDE : SW_SHOW);
    /* thread count (TCP only) */
    if (g_hThreadLbl) ShowWindow(g_hThreadLbl, rtumode ? SW_HIDE : SW_SHOW);
    ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_THREADS), rtumode ? SW_HIDE : SW_SHOW);
    /* scan method combo (TCP only) */
    if (g_hScanMethod) ShowWindow(g_hScanMethod, rtumode ? SW_HIDE : SW_SHOW);
    /* 起/止 labels: shift right in TCP mode to clear wider IP edits */
    /* "起"/"止" labels: reposition in TCP to clear the wider "IP扫描" label */
    if (g_hScanStartLbl) {
        RECT r; GetWindowRect(g_hScanStartLbl, &r);
        MapWindowPoints(NULL, g_hWnd, (POINT*)&r, 2);
        SetWindowPos(g_hScanStartLbl, NULL, MG + (rtumode ? 82 : 80), r.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    if (g_hScanEndLbl) {
        RECT r; GetWindowRect(g_hScanEndLbl, &r);
        MapWindowPoints(NULL, g_hWnd, (POINT*)&r, 2);
        SetWindowPos(g_hScanEndLbl, NULL, MG + (rtumode ? 162 : 220), r.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    }
    /* TCP IP edits: shift right to clear "起"/"止" labels */
    { HWND e = GetDlgItem(g_hWnd, ID_SCAN_TCP_START);
      if (e) { RECT r; GetWindowRect(e, &r); MapWindowPoints(NULL, g_hWnd, (POINT*)&r, 2);
        SetWindowPos(e, NULL, MG + 106, r.top, 108, r.bottom - r.top, SWP_NOZORDER); } }
    { HWND e = GetDlgItem(g_hWnd, ID_SCAN_TCP_END);
      if (e) { RECT r; GetWindowRect(e, &r); MapWindowPoints(NULL, g_hWnd, (POINT*)&r, 2);
        SetWindowPos(e, NULL, MG + 246, r.top, 108, r.bottom - r.top, SWP_NOZORDER); } }
    upd_ui();
}

static DWORD WINAPI tcp_connect_thread(LPVOID a) {
    TcpCtx *ctx = (TcpCtx*)a;
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

static DWORD WINAPI tcp_rd_thread(LPVOID a) {
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

static void reg_set_str(const WCHAR *name, const WCHAR *val) {
    HKEY hk;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, REG_KEY, 0, NULL, 0,
        KEY_WRITE, NULL, &hk, NULL) == ERROR_SUCCESS) {
        DWORD cb = (DWORD)((wcslen(val) + 1) * sizeof(WCHAR));
        RegSetValueExW(hk, name, 0, REG_SZ, (BYTE*)val, cb);
        RegCloseKey(hk);
    }
}

static int reg_get_str(const WCHAR *name, WCHAR *val, int n, const WCHAR *def) {
    DWORD cb = (DWORD)(n * sizeof(WCHAR));
    LONG r = RegGetValueW(HKEY_CURRENT_USER, REG_KEY, name,
        RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, NULL, val, &cb);
    if (r == ERROR_SUCCESS) return 1;
    if (def) wcscpy(val, def);
    return 0;
}

static void snap_clear_reg(int idx) {
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

static void snap_save_reg(SnapState *snap, int idx) {
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

static void snap_load_reg(SnapState *snap, int idx, HWND btn) {
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

static void save_settings(void) {
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

static void load_settings(void) {
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

static LRESULT CALLBACK about_proc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_PAINT) {
        PAINTSTRUCT ps; HDC dc = BeginPaint(h, &ps);
        RECT rc; GetClientRect(h, &rc);
        HBRUSH br = CreateSolidBrush(RGB(252,252,248));
        FillRect(dc, &rc, br); DeleteObject(br);
        static HFONT s_aboutTitle = NULL, s_aboutBody = NULL;
        if (!s_aboutTitle) s_aboutTitle = CreateFontW(22, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
        if (!s_aboutBody) s_aboutBody = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei");
        HFONT titleFont = s_aboutTitle, bodyFont = s_aboutBody;
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
        /* fonts are static — cached for dialog lifetime */
        EndPaint(h, &ps);
        return 0;
    }
    if (m == WM_ERASEBKGND) return 1;
    if (m == WM_KEYDOWN && w == VK_ESCAPE) { DestroyWindow(h); return 0; }
    if (m == WM_LBUTTONDOWN) { DestroyWindow(h); return 0; }
    if (m == WM_DESTROY) return 0;
    return DefWindowProcW(h, m, w, l);
}

static void show_about(HWND parent) {
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

static LRESULT CALLBACK send_edit_proc(HWND h, UINT m, WPARAM w, LPARAM l,
    UINT_PTR idSubclass, DWORD_PTR refData) {
    if (m == WM_CHAR) {
        if (iswxdigit((WCHAR)w)) {
            WCHAR buf[4096]; GetWindowTextW(h, buf, 4096);
            DWORD sel; SendMessageW(h, EM_GETSEL, 0, (LPARAM)&sel);
            int cp = LOWORD(sel);
            int hc = 0;
            for (int i = cp - 1; i >= 0 && iswxdigit(buf[i]); i--) hc++;
            LRESULT ret = DefSubclassProc(h, m, w, l);
            if (hc == 1 && (cp >= (int)wcslen(buf) || buf[cp] != L' ')) {
                SendMessageW(h, EM_REPLACESEL, TRUE, (LPARAM)L" ");
            }
            return ret;
        }
    }
    if (m == WM_KILLFOCUS) {
        WCHAR txt[4096]; GetWindowTextW(h, txt, 4096);
        int len = (int)wcslen(txt);
        while (len > 0 && (txt[len-1] == L' ' || txt[len-1] == L'\t')) len--;
        if (len < (int)wcslen(txt)) { txt[len] = 0; SetWindowTextW(h, txt); }
    }
    if (m == WM_DESTROY) {
        RemoveWindowSubclass(h, send_edit_proc, idSubclass);
    }
    return DefSubclassProc(h, m, w, l);
}

static void make_ui(HWND h) {
    HINSTANCE hi = (HINSTANCE)GetWindowLongPtrW(h, GWLP_HINSTANCE);
    /* fonts: Consolas for hex data, Segoe UI for all UI labels/controls */
    g_hMono = CreateFontW(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        FIXED_PITCH | FF_DONTCARE, L"Consolas");
    g_hFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    #define SETF(w) SendMessageW((w), WM_SETFONT, (WPARAM)g_hFont, TRUE)
    #define SETB(w) SendMessageW((w), WM_SETFONT, (WPARAM)g_hFont, TRUE)
    #define LABEL(t,x,w) do { HWND lb_=CreateWindowW(L"STATIC",(t),WS_CHILD|WS_VISIBLE,(x),y+4,(w),RH-2,h,NULL,hi,NULL); SETF(lb_); } while(0)

    int y = MG + 5, x;

    /* === Row 1: Mode + Connection === */
    HWND rb = CreateWindowW(L"BUTTON", L"RTU", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON|WS_GROUP,
        MG, y+2, 46, RH-2, h, (HMENU)ID_RTU_RADIO, hi, NULL);
    SETF(rb); SendMessageW(rb, BM_SETCHECK, BST_CHECKED, 0);
    rb = CreateWindowW(L"BUTTON", L"TCP", WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
        MG+50, y+2, 46, RH-2, h, (HMENU)ID_TCP_RADIO, hi, NULL);
    SETF(rb);

    /* RTU serial controls — wider labels for Segoe UI */
    x = MG + 100;
    LABEL(L"串口", x, 38); x += 42;
    HWND pc = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
        x, y, 82, 200, h, (HMENU)ID_PORT_COMBO, hi, NULL);
    SETF(pc); x += 88;
    HWND rf = CreateWindowW(L"BUTTON", L"↻", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
        x, y, 24, RH, h, (HMENU)ID_PORT_REFRESH, hi, NULL);
    SETF(rf); x += 30;
    LABEL(L"波特", x, 38); x += 42;
    HWND bc = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL,
        x, y, 88, 200, h, (HMENU)ID_BAUD_COMBO, hi, NULL);
    SETF(bc); x += 94;
    { for (int i=0; i<BAUD_COUNT; i++) { WCHAR s[16]; wsprintfW(s, L"%d", g_baud_rates[i]); SendMessageW(bc, CB_ADDSTRING, 0, (LPARAM)s); } SendMessageW(bc, CB_SETCURSEL, 4, 0); }
    LABEL(L"数据", x, 38); x += 42;
    HWND db = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,
        x, y, 46, 100, h, (HMENU)ID_DATA_COMBO, hi, NULL);
    SETF(db); x += 52;
    for (int i=8; i>=5; i--) { WCHAR s[2]; s[0]=L'0'+i; s[1]=0; SendMessageW(db, CB_ADDSTRING, 0, (LPARAM)s); } SendMessageW(db, CB_SETCURSEL, 0, 0);
    LABEL(L"校验", x, 38); x += 42;
    HWND pr = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,
        x, y, 66, 100, h, (HMENU)ID_PARITY_COMBO, hi, NULL);
    SETF(pr); x += 72;
    SendMessageW(pr, CB_ADDSTRING, 0, (LPARAM)L"N 无"); SendMessageW(pr, CB_ADDSTRING, 0, (LPARAM)L"E 偶"); SendMessageW(pr, CB_ADDSTRING, 0, (LPARAM)L"O 奇"); SendMessageW(pr, CB_SETCURSEL, 0, 0);
    LABEL(L"停止", x, 38); x += 42;
    HWND sb = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,
        x, y, 44, 100, h, (HMENU)ID_STOP_COMBO, hi, NULL);
    SETF(sb); x += 50;
    SendMessageW(sb, CB_ADDSTRING, 0, (LPARAM)L"1"); SendMessageW(sb, CB_ADDSTRING, 0, (LPARAM)L"2"); SendMessageW(sb, CB_SETCURSEL, 0, 0);

    /* TCP controls (hidden in RTU mode) */
    x = MG + 100;
    g_hTcpIpLbl = CreateWindowW(L"STATIC", L"IP", WS_CHILD, x, y+3, 24, RH, h, NULL, hi, NULL);
    SETF(g_hTcpIpLbl); x += 28;
    HWND ipEdit = CreateWindowW(L"EDIT", L"192.168.1.1", WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,
        x, y+1, 136, RH-1, h, (HMENU)ID_TCP_IP, hi, NULL);
    SETF(ipEdit); x += 144;
    g_hTcpPortLbl = CreateWindowW(L"STATIC", L"端口", WS_CHILD, x, y+3, 38, RH, h, NULL, hi, NULL);
    SETF(g_hTcpPortLbl); x += 42;
    HWND tpe = CreateWindowW(L"EDIT", L"502", WS_CHILD|WS_BORDER|ES_NUMBER,
        x, y+1, 56, RH-1, h, (HMENU)ID_TCP_PORT, hi, NULL);
    SETF(tpe);

    /* Open button — right-aligned */
    HWND ob = CreateWindowW(L"BUTTON", L"打开串口", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
        WW-MG-104, y, 104, RH+2, h, (HMENU)ID_OPEN_BTN, hi, NULL);
    SETF(ob);
    SetWindowPos(ob, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

    /* === Row 2: Options === */
    y += RH + GAP + 2;
    HWND cb;
    cb = CreateWindowW(L"BUTTON", L"Modbus", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
        MG, y, 80, RH, h, (HMENU)ID_MODBUS_CHK, hi, NULL);
    SETF(cb); SendMessageW(cb, BM_SETCHECK, BST_CHECKED, 0);
    cb = CreateWindowW(L"BUTTON", L"自动换行", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
        MG+98, y, 92, RH, h, (HMENU)ID_TIME_CHK, hi, NULL);
    SETF(cb); SendMessageW(cb, BM_SETCHECK, BST_CHECKED, 0);
    g_hTimeDispChk = CreateWindowW(L"BUTTON", L"时间显示", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
        MG+210, y, 92, RH, h, (HMENU)ID_TIME_DISP_CHK, hi, NULL);
    SETF(g_hTimeDispChk); SendMessageW(g_hTimeDispChk, BM_SETCHECK, BST_CHECKED, 0);
    g_hDectableChk = CreateWindowW(L"BUTTON", L"十进表格", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
        MG+310, y, 92, RH, h, (HMENU)ID_DECTABLE_CHK, hi, NULL);
    SETF(g_hDectableChk); SendMessageW(g_hDectableChk, BM_SETCHECK, BST_CHECKED, 0);

    /* === Display + Register Table === */
    int dy = y + RH + GAP + 2, dh = 248;
    g_dispH = dh;
    g_hDisp = CreateWindowExW(WS_EX_CLIENTEDGE, L"RICHEDIT50W", L"",
        WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_MULTILINE|ES_WANTRETURN|ES_NOHIDESEL|ES_READONLY|WS_CLIPSIBLINGS,
        MG, dy, WW-MG*2-380, dh, h, (HMENU)ID_DISPLAY, hi, NULL);
    SendMessageW(g_hDisp, EM_SETBKGNDCOLOR, 0, CLR_BG);
    SendMessageW(g_hDisp, WM_SETFONT, (WPARAM)g_hMono, TRUE);
    SendMessageW(g_hDisp, EM_SETMARGINS, EC_LEFTMARGIN, 6);
    SendMessageW(g_hDisp, EM_SETTARGETDEVICE, 0, 0);
    /* init unified log font: match g_hMono exactly, disable RichEdit auto-font */
    g_LogFont.cbSize = sizeof(CHARFORMAT2);
    g_LogFont.dwMask = CFM_FACE | CFM_CHARSET;
    g_LogFont.bCharSet = DEFAULT_CHARSET;
    g_LogFont.bPitchAndFamily = FIXED_PITCH | FF_DONTCARE;
    wcscpy_s(g_LogFont.szFaceName, LF_FACESIZE, L"Consolas");
    /* lock default font for entire control */
    SendMessageW(g_hDisp, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&g_LogFont);
    /* disable RichEdit auto-language-detection / auto-font switching */
    SendMessageW(g_hDisp, EM_SETLANGOPTIONS, 0, 0);

    /* snap buttons: 6 across */
    int sx = WW - MG - 406;
    UINT_PTR snap_ids[SNAP_COUNT] = {ID_SNAP_BTN, ID_SNAP2_BTN, ID_SNAP3_BTN, ID_SNAP4_BTN, ID_SNAP5_BTN, ID_SNAP6_BTN};
    int snap_y = dy - RH - 2;
    for (int i = 0; i < SNAP_COUNT; i++) {
        int bx = sx + 6 + i * 67;
        g_hSnapBtn[i] = CreateWindowW(L"BUTTON", L"未暂存", WS_CHILD|BS_PUSHBUTTON|BS_FLAT,
            bx, snap_y, 59, RH-2, h, (HMENU)snap_ids[i], hi, NULL);
        SETF(g_hSnapBtn[i]);
        g_snapCtx[i].snap = &g_snap[i]; g_snapCtx[i].btn = g_hSnapBtn[i]; g_snapCtx[i].idx = i+1;
        SetWindowSubclass(g_hSnapBtn[i], snap_btn_proc, 0, (DWORD_PTR)&g_snapCtx[i]);
        ShowWindow(g_hSnapBtn[i], SW_HIDE);
    }

    /* register table */
    g_hRegList = CreateWindowW(WC_LISTVIEWW, L"", WS_CHILD|LVS_REPORT|WS_VSCROLL|WS_CLIPSIBLINGS,
        WW-MG-405, dy, 405, dh, h, (HMENU)ID_REG_LIST, hi, NULL);
    SendMessageW(g_hRegList, LVM_SETEXTENDEDLISTVIEWSTYLE, 0,
        LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_DOUBLEBUFFER);
    SendMessageW(g_hRegList, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    { LVCOLUMNW cl={LVCF_TEXT|LVCF_WIDTH};
      cl.pszText=L"地址"; cl.cx=135; SendMessageW(g_hRegList, LVM_INSERTCOLUMNW, 0, (LPARAM)&cl);
      cl.pszText=L"名称"; cl.cx=90; SendMessageW(g_hRegList, LVM_INSERTCOLUMNW, 1, (LPARAM)&cl);
      cl.pszText=L"数值"; cl.cx=164; SendMessageW(g_hRegList, LVM_INSERTCOLUMNW, 2, (LPARAM)&cl); }
    ShowWindow(g_hRegList, SW_HIDE);
    SetWindowSubclass(g_hRegList, lv_proc, 0, 0);

    /* === Send Panel === */
    int sy = dy + dh + GAP + 2;
    y = sy;  /* update row baseline for LABEL macro */
    HWND cw;
    LABEL(L"从站(D)", MG, 64); x = MG+68;
    g_hSlaveAddr = CreateWindowW(L"EDIT", L"1", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        x, sy+1, 40, RH-1, h, (HMENU)ID_SLAVEADDR_DEC, hi, NULL);
    SETF(g_hSlaveAddr);
    LABEL(L"寄存器(D)", MG+116, 74); x = MG+196;
    g_hRegAddr = CreateWindowW(L"EDIT", L"0", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        x, sy+1, 48, RH-1, h, (HMENU)ID_REGADDR_DEC, hi, NULL);
    SETF(g_hRegAddr);
    LABEL(L"数值(D)", MG+252, 64); x = MG+320;
    g_hVal = CreateWindowW(L"EDIT", L"0", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        x, sy+1, 50, RH-1, h, (HMENU)ID_VAL_DEC, hi, NULL);
    SETF(g_hVal);

    /* timer + action buttons */
    cw = CreateWindowW(L"BUTTON", L"定时ms", WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX,
        MG+385, sy, 72, RH, h, (HMENU)ID_TIMER_CHK, hi, NULL);
    SETF(cw);
    cw = CreateWindowW(L"EDIT", L"1000", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        MG+461, sy+1, 50, RH-1, h, (HMENU)ID_TIMER_MS, hi, NULL);
    SETF(cw);
    /* clear + send btns — grouped together on right */
    cw = CreateWindowW(L"BUTTON", L"清除", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
        WW-MG-168, sy, 56, RH+2, h, (HMENU)ID_CLEAR_BTN, hi, NULL);
    SETF(cw);
    HWND hSendBtn = CreateWindowW(L"BUTTON", L"(S) 发送", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
        WW-MG-106, sy, 90, RH+2, h, (HMENU)ID_SEND_BTN, hi, NULL);
    SETF(hSendBtn); g_hSendBtn = hSendBtn;
    SetWindowPos(hSendBtn, HWND_TOP, 0,0,0,0, SWP_NOMOVE|SWP_NOSIZE);

    int sh = 70;
    g_hSend = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD|WS_VISIBLE|WS_BORDER|ES_MULTILINE|ES_AUTOVSCROLL|WS_VSCROLL|WS_CLIPSIBLINGS,
        MG, sy+RH+GAP, WW-MG*2, sh, h, (HMENU)ID_SEND_INPUT, hi, NULL);
    SendMessageW(g_hSend, WM_SETFONT, (WPARAM)g_hMono, TRUE);
    SetWindowSubclass(g_hSend, send_edit_proc, 0, 0);
    SetWindowTextW(g_hSend, L"01 03 00 00 00 01");
    sync_addr_from_send();
    sync_val_from_send();

    /* === Scan Panel === */
    int ay = sy + RH + sh + GAP + 4;
    y = ay;  /* update row baseline for LABEL macro */
    LABEL(L"从站扫描", MG, 72);
    g_hScanStartLbl = CreateWindowW(L"STATIC", L"起", WS_CHILD|WS_VISIBLE, MG+82, ay+4, 24, RH, h, NULL, hi, NULL);
    SETF(g_hScanStartLbl);
    cw = CreateWindowW(L"EDIT", L"1", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        MG+108, ay+1, 46, RH-1, h, (HMENU)ID_SCAN_START, hi, NULL);
    SETF(cw);
    g_hScanEndLbl = CreateWindowW(L"STATIC", L"止", WS_CHILD|WS_VISIBLE, MG+162, ay+4, 24, RH, h, NULL, hi, NULL);
    SETF(g_hScanEndLbl);
    cw = CreateWindowW(L"EDIT", L"254", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        MG+188, ay+1, 46, RH-1, h, (HMENU)ID_SCAN_END, hi, NULL);
    SETF(cw);
    g_hScanIvalLbl = CreateWindowW(L"STATIC", L"间隔ms", WS_CHILD|WS_VISIBLE, MG+242, ay+3, 56, RH, h, NULL, hi, NULL);
    SETF(g_hScanIvalLbl);
    cw = CreateWindowW(L"EDIT", L"800", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        MG+300, ay+1, 48, RH-1, h, (HMENU)ID_SCAN_IVAL, hi, NULL);
    SETF(cw);
    cw = CreateWindowW(L"STATIC", L"超时ms", WS_CHILD|WS_VISIBLE, MG+356, ay+3, 56, RH, h, NULL, hi, NULL);
    SETF(cw);
    cw = CreateWindowW(L"EDIT", L"500", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        MG+414, ay+1, 48, RH-1, h, (HMENU)ID_SCAN_TOUT, hi, NULL);
    SETF(cw);
    g_hThreadLbl = CreateWindowW(L"STATIC", L"线程", WS_CHILD|WS_VISIBLE, MG+472, ay+3, 38, RH, h, NULL, hi, NULL);
    SETF(g_hThreadLbl);
    cw = CreateWindowW(L"EDIT", L"16", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_NUMBER,
        MG+514, ay+1, 38, RH-1, h, (HMENU)ID_SCAN_THREADS, hi, NULL);
    SETF(cw);

    /* TCP scan IP fields (hidden in RTU mode) */
    cw = CreateWindowW(L"EDIT", L"192.168.1.1", WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,
        MG+78, ay+1, 120, RH-1, h, (HMENU)ID_SCAN_TCP_START, hi, NULL);
    SETF(cw);
    cw = CreateWindowW(L"EDIT", L"192.168.1.254", WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,
        MG+226, ay+1, 120, RH-1, h, (HMENU)ID_SCAN_TCP_END, hi, NULL);
    SETF(cw);

    g_hScanSt = CreateWindowW(L"STATIC", L"", WS_CHILD|WS_VISIBLE, MG+564, ay+3, 100, RH, h, (HMENU)ID_SCAN_STATUS, hi, NULL);
    SETF(g_hScanSt);
    g_hScanMethod = CreateWindowW(L"COMBOBOX", L"", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST,
        MG+670, ay+1, 110, 100, h, (HMENU)ID_SCAN_METHOD, hi, NULL);
    SETF(g_hScanMethod);
    SendMessageW(g_hScanMethod, CB_ADDSTRING, 0, (LPARAM)L"Ping");
    SendMessageW(g_hScanMethod, CB_ADDSTRING, 0, (LPARAM)L"Modbus命令");
    SendMessageW(g_hScanMethod, CB_SETCURSEL, 0, 0);
    cw = CreateWindowW(L"BUTTON", L"开始扫描", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
        WW-114, ay, 98, RH+2, h, (HMENU)ID_SCAN_BTN, hi, NULL);
    SETF(cw);

    /* === Scan Result Row === */
    int ary = ay + RH + GAP + 2;
    cw = CreateWindowW(L"STATIC", L"已发现:", WS_CHILD|WS_VISIBLE, MG, ary+3, 64, RH, h, NULL, hi, NULL);
    SETF(cw);
    g_hScanRes = CreateWindowExW(WS_EX_CLIENTEDGE, L"STATIC", L"",
        WS_CHILD|WS_VISIBLE|SS_NOTIFY|WS_VSCROLL,
        MG+54, ary+1, WW-MG*2-120, 148, h, (HMENU)ID_SCAN_RESULT, hi, NULL);
    SetWindowSubclass(g_hScanRes, sr_proc, 0, 0);
    cw = CreateWindowW(L"BUTTON", L"清除", WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON|BS_FLAT,
        WW-MG-56, ary, 44, RH, h, (HMENU)ID_SCAN_CLEAR, hi, NULL);
    SETF(cw);
    ShowWindow(GetDlgItem(h, ID_SCAN_CLEAR), SW_HIDE);

    /* === Status Bar === */
    HWND st = CreateWindowW(STATUSCLASSNAMEW, L"", WS_CHILD|WS_VISIBLE|SBARS_SIZEGRIP,
        0,0,0,0, h, (HMENU)ID_STATUS, hi, NULL);
    { int ps[]={60,210,-1}; SendMessageW(st, SB_SETPARTS, 3, (LPARAM)ps); }
    SendMessageW(st, SB_SETTEXTW, 0, (LPARAM)L" 就绪");
    SendMessageW(st, WM_SETFONT, (WPARAM)g_hFont, TRUE);
    g_hConn = CreateWindowW(L"STATIC", L"", WS_CHILD|WS_VISIBLE, 65, WH-26, WW-130, RH, h, NULL, hi, NULL);
    SETF(g_hConn);

    refill_ports();
    (void)pc; (void)bc; (void)db; (void)pr; (void)sb; (void)ipEdit; (void)tpe; (void)rf; (void)ob; (void)cb;
    load_settings();
    switch_mode();
    /* register for COM port device change notifications */
    { DEV_BROADCAST_DEVICEINTERFACE dbh = {0};
      dbh.dbcc_size = sizeof(dbh);
      dbh.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
      dbh.dbcc_classguid = g_COM_GUID;
      g_hDevNotify = RegisterDeviceNotificationW(h, &dbh, DEVICE_NOTIFY_WINDOW_HANDLE); }
    #undef SETF
    #undef SETB
    #undef LABEL
}

/* ===== wndproc ===== */
static LRESULT CALLBACK wp(HWND h, UINT m, WPARAM w, LPARAM l) {
    switch (m) {
    case WM_CREATE:
        g_hWnd = h;
        make_ui(h);
        /* enable dark title bar (Win10 1809+) */
        { BOOL useDark = TRUE;
          DwmSetWindowAttribute(h, DWMWA_USE_IMMERSIVE_DARK_MODE, &useDark, sizeof(useDark));
          DwmSetWindowAttribute(h, DWMWA_CAPTION_COLOR, &(COLORREF){RGB(32,32,32)}, sizeof(COLORREF)); }
        for (int i = 0; i < SNAP_COUNT; i++)
            snap_load_reg(&g_snap[i], i + 1, g_hSnapBtn[i]);
        upd_ui();
        reg_table();
        return 0;

    case WM_NOTIFY: {
        NMHDR *nm = (NMHDR*)l;
        if (nm->idFrom == ID_REG_LIST && nm->code == NM_DBLCLK) {
            NMITEMACTIVATE *nia = (NMITEMACTIVATE*)l;
            if (nia->iItem < 0 || nia->iItem >= g_nr) break;
            if (nia->iSubItem == 0) {
                /* auto-detect 32-bit format: apply to all selected rows, or just clicked row */
                int applied = 0;
                for (int si = 0; si < g_nr; si++)
                    if (SendMessageW(g_hRegList, LVM_GETITEMSTATE, si, LVIS_SELECTED) & LVIS_SELECTED)
                        { auto_detect_fmt(si); applied = 1; }
                if (!applied) auto_detect_fmt(nia->iItem);
                reg_table();
                break;
            }
            if (nia->iSubItem != 1 && nia->iSubItem != 2) break;
            RECT rc; rc.left = LVIR_BOUNDS; rc.top = nia->iSubItem;
            SendMessageW(g_hRegList, LVM_GETSUBITEMRECT, nia->iItem, (LPARAM)&rc);
            rc.top -= 1; rc.bottom += 2;
            if (nia->iSubItem == 2 && is_consumed(nia->iItem)) break;
            if (nia->iSubItem == 2 && g_reg[nia->iItem].fmt == 3) {
                g_bitEdReadOnly = (g_reg[nia->iItem].fc == 2 || g_reg[nia->iItem].fc == 4);
                show_bit_editor(h, (HINSTANCE)GetWindowLongPtrW(h, GWLP_HINSTANCE), nia->iItem);
                break;
            }
            if (nia->iSubItem == 2 && (g_reg[nia->iItem].fc == 2 || g_reg[nia->iItem].fc == 4)) break;
            g_editItem = nia->iItem; g_editCol = nia->iSubItem;
            WCHAR init[32];
            g_editOldVal = g_reg[g_editItem].val;
            if (nia->iSubItem == 1) { wcscpy(init, g_reg[g_editItem].name); }
            else {
                int f = g_reg[g_editItem].fmt, bo = g_reg[g_editItem].bo;
                if (f == 0) wsprintfW(init, L"%d", (short)(g_reg[g_editItem].val & 0xFFFF));
                else if (f == 2) wsprintfW(init, L"%04X", g_reg[g_editItem].val & 0xFFFF);
                else if (f >= 4) {
                    unsigned long long raw = 0;
                    int nr = (f <= 5 || f == 8) ? 2 : 4;
                    if (get_multi(g_editItem, nr, bo, &raw)) {
                        if (f == 4) wsprintfW(init, L"%d", (int)(raw & 0xFFFFFFFFULL));
                        else if (f == 5) wsprintfW(init, L"%u", (unsigned)(raw & 0xFFFFFFFFULL));
                        else if (f == 6) wsprintfW(init, L"%lld", (long long)raw);
                        else if (f == 7) wsprintfW(init, L"%llu", raw);
                        else if (f == 8) { float fv; memcpy(&fv, &raw, 4); swprintf(init, 32, L"%.6g", fv); }
                        else { double dv; memcpy(&dv, &raw, 8); swprintf(init, 32, L"%.12g", dv); }
                    } else { wsprintfW(init, L"%d", g_reg[g_editItem].val); }
                }
                else wsprintfW(init, L"%u", g_reg[g_editItem].val & 0xFFFF);
            }
            g_hEditCell = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", init,
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                g_hRegList, NULL, (HINSTANCE)GetWindowLongPtrW(h, GWLP_HINSTANCE), NULL);
            SendMessageW(g_hEditCell, WM_SETFONT, (WPARAM)g_hMono, TRUE);
            SendMessageW(g_hEditCell, EM_SETSEL, 0, -1);
            SetWindowSubclass(g_hEditCell, ec_proc, 0, 0);
            SetFocus(g_hEditCell);
        }
        if (nm->idFrom == ID_REG_LIST && nm->code == NM_RCLICK) {
            NMITEMACTIVATE *nia = (NMITEMACTIVATE*)l;
            if (nia->iItem < 0 || nia->iItem >= g_nr) {
                break;
            }
            /* if right-clicked item not selected, select only it */
            if (!(SendMessageW(g_hRegList, LVM_GETITEMSTATE, nia->iItem, LVIS_SELECTED) & LVIS_SELECTED)) {
                for (int si = 0; si < g_nr; si++)
                    SendMessageW(g_hRegList, LVM_SETITEMSTATE, si,
                        (LPARAM)&(LVITEMW){LVIF_STATE, si, 0, 0, LVIS_SELECTED});
                SendMessageW(g_hRegList, LVM_SETITEMSTATE, nia->iItem,
                    (LPARAM)&(LVITEMW){LVIF_STATE, nia->iItem, 0, LVIS_SELECTED, LVIS_SELECTED});
            }
            /* collect selected indices */
            int selIdx[256], selCnt = 0;
            for (int si = 0; si < g_nr && selCnt < 256; si++)
                if (SendMessageW(g_hRegList, LVM_GETITEMSTATE, si, LVIS_SELECTED) & LVIS_SELECTED)
                    selIdx[selCnt++] = si;
            /* show menu using first selected item's current format */
            int fi0 = selCnt > 0 ? selIdx[0] : nia->iItem;
            int cf = g_reg[fi0].fmt, cb = g_reg[fi0].bo;
            HMENU menu = CreatePopupMenu();
            AppendMenuW(menu, MF_STRING | (cf == 0 ? MF_CHECKED : 0), 1, L"16位有符号 (I16)");
            AppendMenuW(menu, MF_STRING | (cf == 1 ? MF_CHECKED : 0), 2, L"16位无符号 (U16)");
            AppendMenuW(menu, MF_STRING | (cf == 2 ? MF_CHECKED : 0), 3, L"十六进制 (HEX)");
            AppendMenuW(menu, MF_STRING | (cf == 3 ? MF_CHECKED : 0), 4, L"二进制 (BIN)");
            AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
            { static const WCHAR *fns[6] = { L"32位有符号 (I32)", L"32位无符号 (U32)",
                  L"64位有符号 (I64)", L"64位无符号 (U64)", L"32位浮点数 (F32)", L"64位浮点数 (F64)" };
              static const WCHAR *bos[4] = { L"AB CD (大端)", L"CD AB (字交换)",
                  L"BA DC (字节交换)", L"DC BA (小端)" };
              for (int fi = 0; fi < 6; fi++) {
                  int fid = fi + 4;
                  HMENU sub = CreatePopupMenu();
                  for (int bi = 0; bi < 4; bi++) {
                      AppendMenuW(sub, MF_STRING | ((cf == fid && cb == bi) ? MF_CHECKED : 0),
                          10 + fi * 4 + bi, bos[bi]);
                  }
                  AppendMenuW(menu, MF_POPUP | (cf == fid ? MF_CHECKED : 0),
                      (UINT_PTR)sub, fns[fi]);
              }
            }
            POINT pt; GetCursorPos(&pt);
            int cmd = (int)TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, h, NULL);
            DestroyMenu(menu);
            if (cmd >= 1 && cmd <= 4) {
                for (int si = 0; si < selCnt; si++) {
                    int idx = selIdx[si];
                    g_reg[idx].fmt = cmd - 1; g_reg[idx].bo = 0;
                    g_reg[idx].fmt_set = 1;
                }
                reg_table();
            } else if (cmd >= 10 && cmd <= 33) {
                int fi = (cmd - 10) / 4, bi = (cmd - 10) % 4;
                for (int si = 0; si < selCnt; si++) {
                    int idx = selIdx[si];
                    g_reg[idx].fmt = fi + 4; g_reg[idx].bo = bi;
                    g_reg[idx].fmt_set = 1;
                }
                reg_table();
            }
        }
        if (nm->idFrom == ID_REG_LIST && nm->code == LVN_COLUMNCLICK) {
            NMLISTVIEW *nlv = (NMLISTVIEW*)l;
            if (nlv->iSubItem == 0) {
                for (int si = 0; si < g_nr; si++)
                    SendMessageW(g_hRegList, LVM_SETITEMSTATE, si,
                        (LPARAM)&(LVITEMW){LVIF_STATE, si, 0, LVIS_SELECTED, LVIS_SELECTED});
            }
            return 0;
        }
        break;
    }

    case WM_CONTEXTMENU:
        if ((HWND)w == g_hRegList) {
            POINT pt = {LOWORD((DWORD)l), HIWORD((DWORD)l)};
            POINT cl = pt; ScreenToClient(g_hRegList, &cl);
            HWND hdr = ListView_GetHeader(g_hRegList);
            RECT hr; GetWindowRect(hdr, &hr);
            MapWindowPoints(HWND_DESKTOP, g_hRegList, (POINT*)&hr, 2);
            if (cl.y < hr.bottom) {
                int any = 0;
                for (int si = 0; si < g_nr; si++)
                    if (SendMessageW(g_hRegList, LVM_GETITEMSTATE, si, LVIS_SELECTED) & LVIS_SELECTED) any = 1;
                if (!any) for (int si = 0; si < g_nr; si++)
                    SendMessageW(g_hRegList, LVM_SETITEMSTATE, si,
                        (LPARAM)&(LVITEMW){LVIF_STATE, si, 0, LVIS_SELECTED, LVIS_SELECTED});
                int cf = g_nr > 0 ? g_reg[0].fmt : 1, cb = g_nr > 0 ? g_reg[0].bo : 0;
                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu, MF_STRING | (cf == 0 ? MF_CHECKED : 0), 1, L"16位有符号 (I16)");
                AppendMenuW(menu, MF_STRING | (cf == 1 ? MF_CHECKED : 0), 2, L"16位无符号 (U16)");
                AppendMenuW(menu, MF_STRING | (cf == 2 ? MF_CHECKED : 0), 3, L"十六进制 (HEX)");
                AppendMenuW(menu, MF_STRING | (cf == 3 ? MF_CHECKED : 0), 4, L"二进制 (BIN)");
                AppendMenuW(menu, MF_SEPARATOR, 0, NULL);
                { static const WCHAR *fns[6] = { L"32位有符号 (I32)", L"32位无符号 (U32)",
                      L"64位有符号 (I64)", L"64位无符号 (U64)", L"32位浮点数 (F32)", L"64位浮点数 (F64)" };
                  static const WCHAR *bos[4] = { L"AB CD (大端)", L"CD AB (字交换)",
                      L"BA DC (字节交换)", L"DC BA (小端)" };
                  for (int fi = 0; fi < 6; fi++) {
                      int fid = fi + 4;
                      HMENU sub = CreatePopupMenu();
                      for (int bi = 0; bi < 4; bi++)
                          AppendMenuW(sub, MF_STRING | ((cf == fid && cb == bi) ? MF_CHECKED : 0),
                              10 + fi * 4 + bi, bos[bi]);
                      AppendMenuW(menu, MF_POPUP | (cf == fid ? MF_CHECKED : 0), (UINT_PTR)sub, fns[fi]);
                  }
                }
                int cmd = (int)TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, h, NULL);
                DestroyMenu(menu);
                if (cmd >= 1 && cmd <= 4) {
                    for (int si = 0; si < g_nr; si++)
                        if (SendMessageW(g_hRegList, LVM_GETITEMSTATE, si, LVIS_SELECTED) & LVIS_SELECTED)
                            { g_reg[si].fmt = cmd - 1; g_reg[si].bo = 0; g_reg[si].fmt_set = 1; }
                    reg_table();
                } else if (cmd >= 10 && cmd <= 33) {
                    int fi = (cmd - 10) / 4, bi = (cmd - 10) % 4;
                    for (int si = 0; si < g_nr; si++)
                        if (SendMessageW(g_hRegList, LVM_GETITEMSTATE, si, LVIS_SELECTED) & LVIS_SELECTED)
                            { g_reg[si].fmt = fi + 4; g_reg[si].bo = bi; g_reg[si].fmt_set = 1; }
                    reg_table();
                }
                return 0;
            }
        }
        break;

    case WM_DEVICECHANGE: {
        if (w == DBT_DEVICEARRIVAL && ((DEV_BROADCAST_HDR*)l)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            /* only when idle: refill list and auto-open new port */
            if (!g_run && g_mode == 0) {
                HWND cb = GetDlgItem(h, ID_PORT_COMBO);
                int oldCnt = (int)SendMessageW(cb, CB_GETCOUNT, 0, 0);
                Sleep(300);  /* let Windows finish port init */
                refill_ports();
                int newCnt = (int)SendMessageW(cb, CB_GETCOUNT, 0, 0);
                if (newCnt > oldCnt) do_open();
            }
            /* when connected: ignore — read thread handles the open port */
        } else if (w == DBT_DEVICEREMOVECOMPLETE && ((DEV_BROADCAST_HDR*)l)->dbch_devicetype == DBT_DEVTYP_DEVICEINTERFACE) {
            /* only when idle: refresh the now-stale port list */
            if (!g_run && g_mode == 0) refill_ports();
            /* when connected: ignore — read thread detects disconnection */
        }
        break;
    }

    case WM_COMMAND: {
        WORD id = LOWORD(w), nf = HIWORD(w);
        switch (id) {
        case ID_OPEN_BTN:
            if (g_run) do_close(); else do_open();
            break;
        case ID_PORT_REFRESH: refill_ports(); break;
        case ID_SEND_BTN: do_send(FALSE); break;
        case ID_CLEAR_BTN: clear_disp(); break;
        case ID_SCAN_BTN:
            if (g_scan) { g_scanStop = 1; SetWindowTextW(g_hScanSt, L"正在停止..."); }
            else start_scan();
            break;
        case ID_SCAN_CLEAR:
            scan_result_set(L"");
            break;
        case ID_TIMER_CHK: case ID_TIMER_MS: upd_ui(); break;
        case ID_DECTABLE_CHK:
            if(nf==BN_CLICKED)reg_table();
            break;
        case ID_SEND_INPUT:
            if (nf == EN_CHANGE) {
                sync_addr_from_send(); sync_val_from_send();
                WCHAR raw[4096]; GetWindowTextW(g_hSend, raw, 4096);
                BYTE tmp[16]; g_sendOk = (raw[0] && parse_hex_send(raw, tmp, 16) >= 0);
                EnableWindow(g_hSendBtn, g_sendOk || !raw[0]);
                InvalidateRect(g_hSend, NULL, TRUE);
            }
            break;
        case ID_SLAVEADDR_DEC:
            if (nf == EN_CHANGE) sync_slave_to_send();
            break;
        case ID_REGADDR_DEC:
            if (nf == EN_CHANGE) sync_addr_to_send();
            break;
        case ID_VAL_DEC:
            if (nf == EN_CHANGE) sync_val_to_send();
            break;
        case ID_SCAN_METHOD:
            if (nf == CBN_SELCHANGE) g_scanMethod = (int)SendMessageW(g_hScanMethod, CB_GETCURSEL, 0, 0);
            break;
        case ID_SCAN_IVAL:
        case ID_SCAN_TOUT:
            /* free input, validated at scan start */
            break;
        case ID_RTU_RADIO:
            if (nf == BN_CLICKED && g_mode != 0) { g_mode = 0; switch_mode(); }
            break;
        case ID_TCP_RADIO:
            if (nf == BN_CLICKED && g_mode != 1) { g_mode = 1; switch_mode(); }
            break;
        case ID_TCP_IP:
            if (nf == EN_CHANGE) {
                WCHAR wip[64]; GetWindowTextW(GetDlgItem(g_hWnd, ID_TCP_IP), wip, 64);
                WideCharToMultiByte(CP_ACP, 0, wip, -1, g_tcpIp, 64, NULL, NULL);
            }
            break;
        case ID_TCP_PORT:
            if (nf == EN_CHANGE) g_tcpPort = (int)GetDlgItemInt(g_hWnd, ID_TCP_PORT, NULL, FALSE);
            break;
        }
        break;
    }

    case WM_RX_DATA: {
        BYTE *d = (BYTE*)w; int n = (int)l;
        QueryPerformanceCounter(&g_t3_ui);
        if (d && n > 0 && g_run) {
            if (!g_scanWaitAddr && g_t1_wr.QuadPart) {
                long long tUs = (g_t3_ui.QuadPart - g_t0_send.QuadPart) * 1000000LL / g_freq.QuadPart;
                WCHAR diag[256];
                swprintf(diag, 256, L"%dms", (int)(tUs/1000));
                log_line(diag, CLR_SYS);
                g_t1_wr.QuadPart = 0;  /* only show once per send */
            }
            if (g_scanWaitAddr && n > 0) {
                if (g_mode == 0) {
                    if (g_scanRspLen + n <= (int)sizeof(g_scanRspBuf)) {
                        memcpy(g_scanRspBuf + g_scanRspLen, d, n);
                        g_scanRspLen += n;
                    } else { g_scanRspLen = 0; }
                    if (g_scanRspLen >= 4 && g_scanRspBuf[0] == (BYTE)g_scanWaitAddr && crc16(g_scanRspBuf, g_scanRspLen) == 0)
                        SetEvent(g_scanEvent);
                } else if (g_mode == 1 && n > 6 && d[6] == (BYTE)g_scanWaitAddr) {
                    g_scanRspLen = n < (int)sizeof(g_scanRspBuf) ? n : (int)sizeof(g_scanRspBuf);
                    memcpy(g_scanRspBuf, d, g_scanRspLen);
                    SetEvent(g_scanEvent);
                }
            }
            /* coalesce rx hex display: buffer data, flush after idle gap (fixes RTU chunk line-break) */
            if (g_dispRxLen + n <= (int)sizeof(g_dispRxBuf)) {
                memcpy(g_dispRxBuf + g_dispRxLen, d, n);
                g_dispRxLen += n;
            } else {
                g_dispRxLen = 0;
                if (n <= (int)sizeof(g_dispRxBuf)) {
                    memcpy(g_dispRxBuf, d, n);
                    g_dispRxLen = n;
                }
            }
            g_dispRetry = 0;
            SetTimer(h, COALESCE_RX, coalesce_ms(), NULL);
            if(SendMessageW(GetDlgItem(h,ID_DECTABLE_CHK),BM_GETCHECK,0,0)==BST_CHECKED){
                if(g_rxBufLen+n<=(int)sizeof(g_rxBuf)){memcpy(g_rxBuf+g_rxBufLen,d,n);g_rxBufLen+=n;process_rx_frames();}
                else{log_fmt(CLR_SYS,L"RX缓冲溢出，已重置");g_rxBufLen=0;}
            }
        }
        free(d); return 0;
    }

    case WM_LOG_MSG: { WCHAR *tx = (WCHAR*)l; if (tx) {
        COLORREF clr = CLR_SYS;
        if (wcsncmp(tx, L"[扫描Tx→", 6) == 0) clr = CLR_TX;
        else if (wcsncmp(tx, L"[扫描Rx←", 6) == 0) clr = CLR_SCAN;
        log_line(tx, clr); free(tx);
    } return 0; }
    case WM_SCAN_PRG: { WCHAR b[64]; wsprintfW(b, L"扫描: %d/%d", (int)w, (int)l); SetWindowTextW(g_hScanSt, b); return 0; }
    case WM_PING_RESULT: {
        int octet = (int)w;
        int ok = (int)(l & 0xFFFF);
        DWORD avgRtt = (DWORD)((l >> 16) & 0xFFFF);
        WCHAR t2[32]; ts(t2, 32);
        WCHAR *m2 = malloc(256);
        if (ok) {
            wsprintfW(m2, L"%s [%s%d] √ ping OK RTT=%lums", t2, g_pingIpBase, octet, avgRtt);
            LONG idx = InterlockedIncrement(&g_pingFc) - 1;
            if (idx < 254) g_pingFound[idx] = octet;
            /* append incrementally to avoid O(n²) rebuild */
            { int len = (int)wcslen(g_scanResult);
              WCHAR seg[96]; int n = wsprintfW(seg, L"%s[%s%d]", len > 0 ? L" | " : L"", g_pingIpBase, octet);
              if (len + n < 5500) wcscat(g_scanResult, seg); }
            g_srScrollY = 0;
            InvalidateRect(g_hScanRes, NULL, TRUE);
            ShowWindow(g_hScanRes, SW_SHOW);
            ShowWindow(GetDlgItem(g_hWnd, ID_SCAN_CLEAR), SW_SHOW);
            log_line(m2, CLR_SCAN);
        } else {
            wsprintfW(m2, L"%s [%s%d] 无响应", t2, g_pingIpBase, octet);
            log_line(m2, CLR_SYS);
        }
        free(m2);
        return 0;
    }
    case WM_SCAN_DONE: {
        int total = (int)w;
        LONG fc = InterlockedExchangeAdd(&g_pingFc, 0);
        WCHAR t2[32]; ts(t2, 32);
        if (fc > 0) {
            WCHAR lst[8192] = {0}; int lstRem = 8191;
            for (LONG i = 0; i < fc && lstRem > 32; i++) {
                if (i > 0) { wcscat(lst, L"  "); lstRem -= 2; }
                WCHAR nm[32]; int n = wsprintfW(nm, L"[%s%d]", g_pingIpBase, g_pingFound[i]);
                wcscat(lst, nm); lstRem -= n;
            }
            log_fmt(CLR_SYS, L"=== 扫描完成: 发现 %d/%d 个设备: %s ===", (int)fc, total, lst);
        } else {
            log_fmt(CLR_SYS, L"=== 扫描完成: 未发现设备 ===");
        }
        update_scan_display();
        return 0;
    }

    case WM_SCAN_RESULT: {
        WCHAR *rt = (WCHAR*)l;
        if (rt) { scan_result_set(rt); free(rt); }
        return 0;
    }
    case WM_UPD_UI: upd_ui(); return 0;


    case WM_OPEN_RESULT: {
        /* TCP mode: wParam=TcpCtx*, lParam=0 on success; wParam=0, lParam=TcpCtx* on failure */
        if (g_mode == 1) {
            TcpCtx *ctx = (TcpCtx*)(w ? w : l);
            if (w && ctx && ctx->sock != INVALID_SOCKET) {
                g_sock = ctx->sock; free(ctx);
                g_run = 1; g_rxBufLen = 0;
                g_hRdTh = CreateThread(NULL, 0, tcp_rd_thread, (LPVOID)(INT_PTR)g_sock, 0, NULL);
                upd_ui();
                WCHAR ip[64]; GetWindowTextW(GetDlgItem(g_hWnd, ID_TCP_IP), ip, 64);
                int port = (int)GetDlgItemInt(g_hWnd, ID_TCP_PORT, NULL, FALSE);
                if (port < 1) port = 502;
                WCHAR info[128]; wsprintfW(info, L"%s:%d", ip, port);
                log_fmt(CLR_SYS, L"--- TCP 已连接: %s ---", info);
                SetWindowTextW(g_hConn, info);
                SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 已连接");
            } else {
                if (ctx) free(ctx);
                upd_ui();
                SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 就绪");
                MessageBoxW(g_hWnd, L"无法连接TCP，请检查IP和端口", L"错误", MB_OK | MB_ICONERROR);
            }
            return 0;
        }
        /* RTU mode */
        HANDLE hPort = (HANDLE)w;
        OpenCtx *ctx = (OpenCtx*)l;
        if (hPort && ctx) {
            int com = ctx->port, sv_baud = ctx->baud, sv_data = ctx->data, sv_parity = ctx->parity, sv_stop = ctx->stop;
            free(ctx);
            g_hPort = hPort;
            g_run = 1; g_rxBufLen = 0;
            g_hRdTh = CreateThread(NULL, 0, rd_thread, (LPVOID)hPort, 0, NULL);
            upd_ui();
            WCHAR info[128];
            {
                WCHAR pl = sv_parity == NOPARITY ? L'N' : sv_parity == EVENPARITY ? L'E' : L'O';
                wsprintfW(info, L"COM%d %dbps %d%c%s", com, sv_baud, sv_data, pl,
                    sv_stop == ONESTOPBIT ? L"1" : L"2");
            }
            log_fmt(CLR_SYS, L"--- 串口已打开: %s ---", info);
            SetWindowTextW(g_hConn, info);
            SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 已连接");
        } else {
            if (ctx) free(ctx);
            upd_ui();
            SendMessageW(GetDlgItem(g_hWnd, ID_STATUS), SB_SETTEXTW, 0, (LPARAM)L" 就绪");
            MessageBoxW(g_hWnd, L"无法打开串口，请检查是否被占用", L"错误", MB_OK | MB_ICONERROR);
        }
        return 0;
    }

    case WM_TIMER:
        if(w==TIMER_SEND)do_send(TRUE);
        else if(w==TIMER_COM_SCAN)refill_ports();
        else if(w==3001){KillTimer(h,3001);g_lastRegTicks=0;reg_table();}
        else if(w==COALESCE_RX){
            KillTimer(h,COALESCE_RX);
            if(g_dispRxLen>0){
                BOOL aw=SendMessageW(GetDlgItem(h,ID_TIME_CHK),BM_GETCHECK,0,0)==BST_CHECKED;
                BOOL mb=SendMessageW(GetDlgItem(h,ID_MODBUS_CHK),BM_GETCHECK,0,0)==BST_CHECKED;
                int found=0;
                if(aw && mb && g_mode==0){
                    /* RTU: find CRC-valid frames (request or response), display each as one line */
                    int pos=0;
                    while(pos<g_dispRxLen){
                        if(g_dispRxLen-pos<4)break;
                        int sa=g_dispRxBuf[pos];
                        if(sa<1||sa>254){pos++;continue;}
                        int fc=g_dispRxBuf[pos+1],fl=0;
                        if(fc&0x80){
                            fl=5; /* error response: sa+fc+ec+crc */
                        } else if(fc==3||fc==4||fc==1||fc==2){
                            /* try request (8 bytes fixed) and response (variable) */
                            int fl_req=8, ok_req=0;
                            if(g_dispRxLen-pos>=fl_req) ok_req=(crc16(g_dispRxBuf+pos,fl_req)==0);
                            int fl_rsp=0, ok_rsp=0;
                            if(g_dispRxLen-pos>=5){
                                int bc=g_dispRxBuf[pos+2];
                                if(bc<=250){fl_rsp=3+bc+2;
                                    if(g_dispRxLen-pos>=fl_rsp) ok_rsp=(crc16(g_dispRxBuf+pos,fl_rsp)==0);
                                }
                            }
                            if(ok_req) fl=fl_req;
                            else if(ok_rsp) fl=fl_rsp;
                            else {pos++;continue;}
                        } else if(fc==5||fc==6){
                            fl=8; /* fixed 8 bytes for single-register write */
                        } else if(fc==0x10||fc==0x0F){
                            /* try response (8 bytes) or request (variable) */
                            int fl_rsp=8, ok_rsp=0;
                            if(g_dispRxLen-pos>=fl_rsp) ok_rsp=(crc16(g_dispRxBuf+pos,fl_rsp)==0);
                            int fl_req=0, ok_req=0;
                            if(g_dispRxLen-pos>=7){
                                int bc=g_dispRxBuf[pos+6];
                                if(bc<=250){fl_req=7+bc+2;
                                    if(g_dispRxLen-pos>=fl_req) ok_req=(crc16(g_dispRxBuf+pos,fl_req)==0);
                                }
                            }
                            if(ok_rsp) fl=fl_rsp;
                            else if(ok_req) fl=fl_req;
                            else {pos++;continue;}
                        } else {pos++;continue;}
                        if(fl<4||fl>256){pos++;continue;}
                        if(g_dispRxLen-pos<fl)break;
                        if(crc16(g_dispRxBuf+pos,fl)!=0){pos++;continue;}
                        /* dump garbage before this frame as raw */
                        if(pos>0){
                            WCHAR hx2[4096];fmt_hex(g_dispRxBuf,pos,hx2,4096);
                            log_data(hx2,CLR_RX);
                        }
                        /* valid frame as its own line */
                        WCHAR hx3[4096];fmt_hex(g_dispRxBuf+pos,fl,hx3,4096);
                        log_data(hx3,CLR_RX);
                        pos+=fl;
                        found=1;
                    }
                    if(found){
                        g_dispRxLen-=pos;
                        if(g_dispRxLen>0)memmove(g_dispRxBuf,g_dispRxBuf+pos,g_dispRxLen);
                                }
                } else if(aw && mb && g_mode==1){
                    /* TCP: find frames by MBAP length */
                    int pos=0;
                    while(pos<g_dispRxLen){
                        if(g_dispRxLen-pos<8)break;
                        int mlen=(g_dispRxBuf[pos+4]<<8)|g_dispRxBuf[pos+5];
                        int fl=6+mlen;
                        if(fl<8||fl>2048){pos++;continue;}
                        if(g_dispRxLen-pos<fl)break;
                        if(pos>0){
                            WCHAR hx2[4096];fmt_hex(g_dispRxBuf,pos,hx2,4096);
                            log_data(hx2,CLR_RX);
                        }
                        WCHAR hx3[4096];fmt_hex(g_dispRxBuf+pos,fl,hx3,4096);
                        log_data(hx3,CLR_RX);
                        pos+=fl;
                        found=1;
                    }
                    if(found){
                        g_dispRxLen-=pos;
                        if(g_dispRxLen>0)memmove(g_dispRxBuf,g_dispRxBuf+pos,g_dispRxLen);
                    }
                }
                /* no valid frame found, or stale leftover — retry N times then force flush */
                if(!found){
                    g_dispRetry++;
                    if(g_dispRetry>=5){
                        WCHAR hx2[4096];fmt_hex(g_dispRxBuf,g_dispRxLen,hx2,4096);
                        log_data(hx2,CLR_RX);
                        g_dispRxLen=0;
                        g_dispRetry=0;
                    } else if(g_dispRxLen>0){
                        SetTimer(h,COALESCE_RX,coalesce_ms(),NULL);
                    }
                }
                /* frames found but trailing bytes remain — retry once more for late data */
                if(found && g_dispRxLen>0){
                    SetTimer(h,COALESCE_RX,coalesce_ms(),NULL);
                }
            }
        }
        return 0;

    case WM_DESTROY:
        if (g_hDevNotify) { UnregisterDeviceNotification(g_hDevNotify); g_hDevNotify = NULL; }
        save_settings();
        if (g_scan) { g_scanStop = 1; int w = 0; while (g_scan && w < 300) { Sleep(10); w++; } }
        g_run = 0; g_pauseRd = 0;
        KillTimer(g_hWnd, TIMER_SEND);
        KillTimer(g_hWnd, TIMER_COM_SCAN);
        KillTimer(g_hWnd, COALESCE_RX);
        if (g_sock != INVALID_SOCKET) { SOCKET s = g_sock; g_sock = INVALID_SOCKET; shutdown(s, SD_BOTH); closesocket(s); }
        if (g_hPort != INVALID_HANDLE_VALUE) { HANDLE hp = g_hPort; g_hPort = INVALID_HANDLE_VALUE; CancelIoEx(hp, NULL); }
        if (g_hRdTh) { HANDLE t = g_hRdTh; g_hRdTh = NULL; WaitForSingleObject(t, 3000); CloseHandle(t); }
        if (g_scanEvent) { CloseHandle(g_scanEvent); g_scanEvent = NULL; }
        RemoveWindowSubclass(g_hRegList, lv_proc, 0);
        DeleteObject(g_hBgBr); DeleteObject(g_hRedBr); DeleteObject(g_hMono); DeleteObject(g_hFont);
        WSACleanup();
        PostQuitMessage(0);
        return 0;

    case WM_NCLBUTTONDOWN:
        if (w == HTSYSMENU) { show_about(h); return 0; }
        break;

    case WM_CTLCOLOREDIT:
        if (!g_sendOk && (HWND)l == g_hSend) {
            SetBkColor((HDC)w, RGB(255, 220, 220));
            return (LRESULT)g_hRedBr;
        }
        break;
    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)w, RGB(240, 240, 245));
        SetBkMode((HDC)w, OPAQUE);
        return (LRESULT)g_hBgBr;
    }

    return DefWindowProcW(h, m, w, l);
}

/* ===== winmain ===== */


int WINAPI wWinMain(HINSTANCE hi, HINSTANCE hp, LPWSTR cl, int cs) {
    (void)hp; (void)cl;
    QueryPerformanceFrequency(&g_freq);
    timeBeginPeriod(1);
    { WSADATA wd; WSAStartup(MAKEWORD(2,2), &wd); }
    InitCommonControls();
    INITCOMMONCONTROLSEX ic = { sizeof(ic), ICC_STANDARD_CLASSES | ICC_LISTVIEW_CLASSES };
    InitCommonControlsEx(&ic);

    HMODULE re = LoadLibraryW(L"Msftedit.dll");
    if (!re) re = LoadLibraryW(L"Riched20.dll");
    if (!re) { MessageBoxW(NULL, L"无法加载 RichEdit", L"错误", MB_OK | MB_ICONERROR); return 1; }

    g_hBgBr = CreateSolidBrush(RGB(240, 240, 245));
    g_hRedBr = CreateSolidBrush(RGB(255, 220, 220));

    WNDCLASSW wc = { 0 }; wc.lpfnWndProc = wp; wc.hInstance = hi;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = g_hBgBr;
    wc.lpszClassName = L"ComProWnd";
    wc.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(1));
    RegisterClassW(&wc);

    HWND hw = CreateWindowExW(WS_EX_CONTROLPARENT, L"ComProWnd",
        L"ComPro " VERSION L" — Modbus RTU/TCP 调试工具",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        (GetSystemMetrics(SM_CXSCREEN) - WW) / 2, (GetSystemMetrics(SM_CYSCREEN) - WH) / 2,
        WW, WH, NULL, NULL, hi, NULL);
    if (!hw) { FreeLibrary(re); return 1; }

    ShowWindow(hw, cs); UpdateWindow(hw);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE && g_hBitEd) {
            HWND f = GetFocus();
            if (f && IsChild(g_hBitEd, f)) {
                SendMessageW(g_hBitEd, WM_KEYDOWN, VK_ESCAPE, 0);
                continue;
            }
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_RETURN) {
            HWND f = GetFocus();
            if (g_hRenameEdit && f == g_hRenameEdit) { TranslateMessage(&msg); DispatchMessageW(&msg); continue; }
            if (f == g_hSend && !(GetKeyState(VK_SHIFT) & 0x8000)) {
                if (!g_run) do_open(); else do_send(FALSE);
                continue;
            }
            if (f == g_hDisp) { TranslateMessage(&msg); DispatchMessageW(&msg); continue; }
            if (g_hEditCell && f == g_hEditCell && g_editItem >= 0) { commit_edit(g_hEditCell); continue; }
            /* any other Edit control: first Enter confirms (moves focus out), second Enter sends */
            if (f) {
                WCHAR cls[32];
                if (GetClassNameW(f, cls, 32) && wcscmp(cls, L"Edit") == 0) {
                    SetFocus(hw);
                    continue;
                }
            }
            /* non-input area: Enter = connect or send */
            if (f) {
                WCHAR cls[32];
                if (GetClassNameW(f, cls, 32) &&
                    wcscmp(cls, L"Edit") != 0 &&
                    wcscmp(cls, L"ComboBox") != 0) {
                    if (!g_run) do_open(); else do_send(FALSE);
                    continue;
                }
            }
        }
        if (msg.message == WM_KEYDOWN && msg.wParam == 'S' && (GetKeyState(VK_CONTROL) & 0x8000)) { do_send(FALSE); continue; }
        /* Ctrl+C: copy format from first selected register row */
        if (msg.message == WM_KEYDOWN && msg.wParam == 'C' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            HWND f = GetFocus();
            if (f == g_hRegList || IsChild(g_hRegList, f)) {
                for (int si = 0; si < g_nr; si++)
                    if (SendMessageW(g_hRegList, LVM_GETITEMSTATE, si, LVIS_SELECTED) & LVIS_SELECTED)
                        { g_copyFmt = g_reg[si].fmt; g_copyBo = g_reg[si].bo; break; }
            }
            continue;
        }
        /* Ctrl+V: paste format to all selected register rows */
        if (msg.message == WM_KEYDOWN && msg.wParam == 'V' && (GetKeyState(VK_CONTROL) & 0x8000)) {
            HWND f = GetFocus();
            if (g_copyFmt >= 0 && (f == g_hRegList || IsChild(g_hRegList, f))) {
                int applied = 0;
                for (int si = 0; si < g_nr; si++)
                    if (SendMessageW(g_hRegList, LVM_GETITEMSTATE, si, LVIS_SELECTED) & LVIS_SELECTED)
                        { g_reg[si].fmt = g_copyFmt; g_reg[si].bo = g_copyBo; g_reg[si].fmt_set = 1; applied = 1; }
                if (applied) reg_table();
            }
            continue;
        }
        if (!IsDialogMessageW(hw, &msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    }

    FreeLibrary(re);
    return (int)msg.wParam;
}
