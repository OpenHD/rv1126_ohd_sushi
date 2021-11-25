/*
 * This is a every simple sample for MiniGUI.
 * It will create a main window and display a string of "Hello, world!" in it.
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h> 
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>
extern WINDOW_ELEMENT_RENDERER * __mg_def_renderer;
extern MG_EXPORT PLOGFONT g_SysLogFont [];

#define _ID_TIMER_MAIN                  100
#define TIMER_MAIN                      100

#define TTF_FONT_SIZE     16

#define LCD_W    720
#define LCD_H    1280

#define BG_PINT_X    0
#define BG_PINT_Y    0
#define BG_PINT_W    LCD_W
#define BG_PINT_H    LCD_H

RECT msg_rcBg = {BG_PINT_X, BG_PINT_Y, BG_PINT_X + BG_PINT_W, BG_PINT_Y + BG_PINT_H};

char timebuff[100];

BITMAP batt_bmap[6];
BITMAP background_bmap;

HWND mhWnd;

static char* mk_time(char* buff)
{
     time_t t;
     struct tm * tm;

     time (&t);
     tm = localtime (&t);
     sprintf (buff, "%02d:%02d:%02d", tm->tm_hour, tm->tm_min, tm->tm_sec);

     return buff;
}

void sysUsecTime(char* buff)
{
    struct timeval    tv;
    struct timezone tz;

    struct tm         *p;

    gettimeofday(&tv, &tz);

    p = localtime(&tv.tv_sec);
    sprintf(buff, "%04d-%02d-%02d %02d:%02d:%02d %03d", 1900+p->tm_year, 1+p->tm_mon, p->tm_mday, p->tm_hour, p->tm_min, p->tm_sec, tv.tv_usec / 1000);
}

int main_loadres(void)
{
    int i;
    char img[128];
    char *respath = "/usr/local/share/minigui/res/images/";

    snprintf(img, sizeof(img), "%sbackground.png", respath);
    if (LoadBitmap(HDC_SCREEN, &background_bmap, img))
        return -1;

    return 0;
}

void main_unloadres(void)
{
    int i;

    UnloadBitmap(&background_bmap);
}

static LRESULT MainWinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    HDC sndHdc;

    switch (message) {
        case MSG_CREATE:
        printf("MSG_CREATE\n");
            //main_loadres();
            SetTimer(hWnd, _ID_TIMER_MAIN, TIMER_MAIN);
            sndHdc = GetSecondaryDC((HWND)hWnd);
            SetMemDCAlpha(sndHdc, MEMDC_FLAG_SWSURFACE, 0);
            InvalidateRect(hWnd, &msg_rcBg, TRUE);
            RegisterMainWindow(hWnd);
            mhWnd = hWnd;
        break;
        case MSG_TIMER:
        printf("MSG_TIMER\n");
            if (wParam == _ID_TIMER_MAIN) {
                InvalidateRect(hWnd, &msg_rcBg, TRUE);
            }
        break;
        case MSG_PAINT:
            hdc = BeginPaint(hWnd);
/*
            FillBoxWithBitmap(hdc, BG_PINT_X,
                              BG_PINT_Y, BG_PINT_W,
                              BG_PINT_H, &background_bmap);
*/
            SetBrushColor(hdc, 0xffffffff);
            FillBox(hdc, 0, 0, LCD_W, LCD_H);
            SetBrushColor(hdc, 0x00000000);
            FillBox(hdc, 0, 0, 400, 400);
//            SetBrushColor(hdc, 0x80000000);
//            FillBox(hdc, 0, 100, 400, 200);

            EndPaint(hWnd, hdc);
            break;
        case MSG_KEYDOWN:
            printf("%s MSG_KEYDOWN message = 0x%x, 0x%x, 0x%x\n", __func__, message, wParam, lParam);
            break;
        case MSG_MAINWIN_KEYDOWN:
            break;
        case MSG_MAINWIN_KEYUP:
            break;
        case MSG_CLOSE:
            KillTimer(hWnd, _ID_TIMER_MAIN);
            UnregisterMainWindow(hWnd);
            DestroyMainWindow(hWnd);
            PostQuitMessage(hWnd);
            //main_unloadres();
        return 0;
    }

    return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

static void InitCreateInfo(PMAINWINCREATE pCreateInfo)
{
    pCreateInfo->dwStyle = WS_VISIBLE;
    pCreateInfo->dwExStyle = WS_EX_AUTOSECONDARYDC;
    pCreateInfo->spCaption = "Main" ;
    pCreateInfo->hMenu = 0;
    //pCreateInfo->hCursor = GetSystemCursor (0);
    pCreateInfo->hIcon = 0;
    pCreateInfo->MainWindowProc = MainWinProc;
    pCreateInfo->lx = 0;
    pCreateInfo->ty = 0;
    pCreateInfo->rx = LCD_W;
    pCreateInfo->by = LCD_H;
    pCreateInfo->iBkColor = PIXEL_lightwhite;
    pCreateInfo->dwAddData = 0;
    pCreateInfo->hHosting = HWND_DESKTOP;
}

void signal_func(int signal)
{
    printf("%s\n", __func__);
    switch (signal){
        case SIGUSR1:
            break;
        default:
            break;
    }
}

int MiniGUIMain(int args, const char* arg[])
{
    MSG Msg;
    MAINWINCREATE CreateInfo;
    struct sigaction sa;
    FILE *pid_file;
    HWND hMainWnd;

#ifdef _MGRM_PROCESSES
    JoinLayer (NAME_DEF_LAYER, arg[0], 0, 0);
#endif
    InitCreateInfo (&CreateInfo);

    pid_file = fopen("/tmp/pid", "w");
    if (!pid_file) {
        printf("open /tmp/pid fail...\n");
        return -1;
    }

    fprintf(pid_file, "%d", getpid());
    fclose(pid_file);

    sa.sa_sigaction = NULL;
    sa.sa_handler   = signal_func;
    sa.sa_flags     = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);

    hMainWnd = CreateMainWindow (&CreateInfo);

    if (hMainWnd == HWND_INVALID)
        return -1;

    while (GetMessage (&Msg, hMainWnd)) {
        DispatchMessage (&Msg);
    }
    MainWindowThreadCleanup (hMainWnd);

    return 0;
}