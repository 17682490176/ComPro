/* compro.h — shared declarations for ComPro Modbus RTU/TCP debug tool */
#ifndef COMPRO_H
#define COMPRO_H

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

/* ===== version ===== */
#define VERSION L"4.1"

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

/* ===== layout constants ===== */
#define WW 920
#define WH 695
#define MG 12
#define GAP 7
#define RH 25

/* ===== colors ===== */
#define CLR_BG   RGB(248,248,244)
#define CLR_RX   RGB(0,0,255)
#define CLR_TX   RGB(0,140,0)
#define CLR_SYS  RGB(130,130,130)
#define CLR_SCAN RGB(0,0,200)
#define CLR_IP_A  RGB(0,72,186)
#define CLR_IP_B  RGB(186,72,0)

/* ===== COM port device interface GUID ===== */
static const GUID g_COM_GUID = {0x86E0D1E0,0x8089,0x11D0,{0x9C,0xE4,0x08,0x00,0x3E,0x30,0x1F,0x73}};

/* ===== limits ===== */
#define SNAP_LONG_PRESS 800
#define SNAP_TIMER_ID 3002
#define SNAP_COUNT 6
#define MAX_REG 256
#define BAUD_COUNT 13
#define REG_KEY L"Software\\ComPro"

/* ===== structs ===== */
/* register table entry */
typedef struct { int slave, addr, val, ok, mod, fc, fmt, bo, fmt_set, name_set; WCHAR name[32]; } RegEntry;

/* snapshot state */
typedef struct { int active, slave, fc, addr, qty, fmt[256], bo[256]; WCHAR name[256][32]; WCHAR sname[32]; WCHAR send_cmd[4096]; int saved_to_reg; } SnapState;
typedef struct { SnapState *snap; HWND btn; int idx; int longPressed; } SnapBtnCtx;

/* async open context */
typedef struct { int port, baud, data, parity, stop; HWND hWnd; } OpenCtx;

/* TCP context */
typedef struct { SOCKET sock; HWND hWnd; } TcpCtx;

/* scan info */
typedef struct { HANDLE hPort; SOCKET sock; int start, end, ival, tout; BYTE tmpl[2048]; int tlen; int tcp; char ipBase[64]; int method; BYTE sendBuf[2048]; int sendLen; int tcpPort; } SI;

/* scan workers */
typedef struct { char ipBase[64]; int start, end; DWORD tout; } PingArg;
typedef struct { char ipBase[64]; int start, end; DWORD tout; BYTE sendBuf[2048]; int sendLen; int tcpPort; } ModbusScanArg;

/* bit editor group */
typedef struct { int lo, hi; HWND hBox; } BitGroup;

/* ===== global externs ===== */
extern HWND g_hWnd, g_hDisp, g_hRegList, g_hSend, g_hRegAddr, g_hSlaveAddr, g_hVal, g_hScanSt, g_hScanRes, g_hConn;
extern HWND g_hSendBtn;
extern HWND g_hEditCell, g_hTcpIpLbl, g_hTcpPortLbl, g_hScanLbl, g_hTimeDispChk, g_hDectableChk;
extern HWND g_hBitEd;
extern HWND g_hScanStartLbl, g_hScanEndLbl, g_hScanIvalLbl;
extern HWND g_hRenameEdit;
extern HWND g_hSnapBtn[SNAP_COUNT];
extern HWND g_hThreadLbl, g_hScanMethod;
extern HDEVNOTIFY g_hDevNotify;

extern int g_bSyncing;
extern int g_dispH, g_editItem, g_editCol, g_editOldVal;
extern HBRUSH g_hBgBr, g_hRedBr;
extern HFONT g_hMono, g_hFont;
extern CHARFORMAT2 g_LogFont;
extern int g_sendOk;
extern HANDLE g_hPort;
extern SOCKET g_sock;
extern HANDLE g_hRdTh;
extern volatile LONG g_run, g_scan, g_scanStop, g_pauseRd;
extern int g_mode;
extern int g_scanMethod;
extern LONG g_mbap_tid;
extern char g_tcpIp[64];
extern int g_tcpPort;
extern volatile int g_scanWaitAddr;
extern HANDLE g_scanEvent;
extern BYTE g_scanRspBuf[2048];
extern int g_scanRspLen;
extern volatile LONG g_pingNext;
extern volatile LONG g_pingFc;
extern int g_pingFound[254];
extern WCHAR g_pingIpBase[64];
extern WCHAR g_scanResult[5500];
extern int g_srScrollY;
extern LONG g_concWrk;

extern RegEntry g_reg[MAX_REG];
extern int g_nr;
extern int g_lastSA, g_lastFC, g_lastAD, g_lastQN;
extern int g_copyFmt, g_copyBo;
extern SnapState g_snap[SNAP_COUNT];
extern SnapBtnCtx g_snapCtx[SNAP_COUNT];
extern int g_dispLen;
extern BYTE g_rxBuf[4096];
extern int g_rxBufLen;
extern BYTE g_dispRxBuf[4096];
extern int g_dispRxLen;
extern int g_dispRetry;
extern DWORD g_lastRegTicks;
extern LARGE_INTEGER g_freq, g_t0_send, g_t1_wr, g_t3_ui;
extern const int g_baud_rates[BAUD_COUNT];

/* bit editor state */
extern int g_bitEdItem;
extern int g_bitEdAutoSend;
extern int g_bitEdReadOnly;
extern BitGroup g_bitGroups[8];
extern int g_bitGroupCnt;
extern int g_selStart, g_selEnd;
extern int g_bitSync;

extern int g_reconnectDelay;
/* is_consumed cache */
extern int g_consumeCache[MAX_REG];
extern int g_consumeCacheGen;

/* TCP reconnect backoff */
extern int g_reconnectDelay;

/* ===== function prototypes ===== */

/* helpers */
void ts(WCHAR *b, int n);
int baudrate(void);
int coalesce_ms(void);
void fmt_hex(const BYTE *d, int n, WCHAR *o, int m);
unsigned short crc16(const BYTE *d, int n);
int parse_hex_send(const WCHAR *raw, BYTE *out, int maxLen);
unsigned short bswap16(unsigned short v);

/* send helpers */
int send_all(SOCKET sock, const char *buf, int len);
int build_mbap(BYTE *d, int dl, BYTE *out);

/* display / logging */
void trim_and_scroll(void);
void log_write(const WCHAR *s, COLORREF c, BOOL addTs);
void log_line(const WCHAR *s, COLORREF c);
void log_fmt(COLORREF c, const WCHAR *fmt, ...);
void log_data(const WCHAR *s, COLORREF c);
void clear_disp(void);

/* sync */
void sync_addr_from_send(void);
void sync_addr_to_send(void);
void sync_slave_to_send(void);
void sync_val_from_send(void);
void sync_val_to_send(void);

/* modbus */
void parse_modbus(const BYTE *d, int n);
void process_rx_frames(void);
int get_multi(int idx, int nreg, int bo, unsigned long long *out);
int is_consumed(int idx);
void consume_cache_reset(void);
void auto_detect_fmt(int idx);
void fmt_reg_val(int idx, WCHAR *v, int v_size);
void auto_write(int sa, int addr, int val, int orig_fc);

/* register table */
void reg_table(void);
void show_bit_editor(HWND parent, HINSTANCE hi, int item);
void commit_edit(HWND hEdit);

/* subclasses */
LRESULT CALLBACK ec_proc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR idSubclass, DWORD_PTR refData);
LRESULT CALLBACK sr_proc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR idSubclass, DWORD_PTR refData);
LRESULT CALLBACK lv_proc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR idSubclass, DWORD_PTR refData);
LRESULT CALLBACK bited_proc(HWND h, UINT m, WPARAM w, LPARAM l);
LRESULT CALLBACK snap_btn_proc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR idSubclass, DWORD_PTR refData);
LRESULT CALLBACK rename_edit_proc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR idSubclass, DWORD_PTR refData);
LRESULT CALLBACK send_edit_proc(HWND h, UINT m, WPARAM w, LPARAM l, UINT_PTR idSubclass, DWORD_PTR refData);
LRESULT CALLBACK about_proc(HWND h, UINT m, WPARAM w, LPARAM l);

/* snapshots */
void update_snap_text(SnapState *snap, HWND btn);
void snap_clear(SnapState *snap, HWND btn);
void snap_capture(SnapState *snap, HWND btn);
void snap_save_reg(SnapState *snap, int idx);
void snap_load_reg(SnapState *snap, int idx, HWND btn);
void snap_clear_reg(int idx);

/* serial */
void refill_ports(void);
DWORD WINAPI open_thread(LPVOID a);
DWORD WINAPI rd_thread(LPVOID a);

/* tcp */
DWORD WINAPI tcp_connect_thread(LPVOID a);
DWORD WINAPI tcp_rd_thread(LPVOID a);

/* scan */
void start_scan(void);
DWORD WINAPI scan_thread(LPVOID a);
DWORD WINAPI ping_worker(LPVOID p);
DWORD WINAPI modbus_scan_worker(LPVOID p);
void fmt_found_str(WCHAR *buf, int bufSize, int *found, int fc);
void scan_result_set(const WCHAR *txt);
void update_scan_display(void);

/* connection */
void do_open(void);
void do_close(void);
void do_send(BOOL silent);

/* ui */
void make_ui(HWND h);
void upd_ui(void);
void switch_mode(void);
void show_about(HWND parent);

/* settings */
void reg_set_str(const WCHAR *name, const WCHAR *val);
int reg_get_str(const WCHAR *name, WCHAR *val, int n, const WCHAR *def);
void save_settings(void);
void load_settings(void);

#endif /* COMPRO_H */
