// For licensing and usage information, read docs/release/winui_license.txt
//****************************************************************************

 /***************************************************************************

  winui.cpp

  Win32 GUI code.

  Created 8/12/97 by Christopher Kirmse (ckirmse@ricochet.net)
  Additional code November 1997 by Jeff Miller (miller@aa.net)
  More July 1998 by Mike Haaland (mhaaland@hypertech.com)
  Added Spitters/Property Sheets/Removed Tabs/Added Tree Control in
  Nov/Dec 1998 - Mike Haaland

***************************************************************************/
// dirwatch doesn't seem to do anything useful, so it's been disconnected 2025-03-22
//#define DIRWATCH

// standard windows headers
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <commctrl.h>
#include <uxtheme.h>

// standard C headers
#include <dlgs.h>
#include <sys/stat.h>
#include <tchar.h>

// MAME/MAMEUI headers
#include "emu.h"
#include "mame.h"
#include "language.h"
#include "unzip.h"
#include "winutf8.h"
#include "strconv.h"
#include "window.h"
#include "zippath.h"
#include "corestr.h"

#include "resource.h"
#include "resource.hm"
#include "mui_util.h"
#include "mui_audit.h"
#include "directories.h"
#include "mui_opts.h"
#include "emu_opts.h"
#include "properties.h"
#include "columnedit.h"
#include "picker.h"
#include "tabview.h"
#include "bitmask.h"
#include "treeview.h"
#include "splitters.h"
#ifdef DIRWATCH
#include "dirwatch.h"
#endif
#include "help.h"
#include "history.h"
#include "dialogs.h"
#include "directinput.h"
#include "dijoystick.h"     /* For DIJoystick availability. */
#include "softwarelist.h"
#include "messui.h"
#include "drivenum.h"
#include "mameopts.h"
#include "modules/diagnostics/diagnostics_module.h"
#include <fstream>

#define MM_PLAY_GAME (WM_APP + 102)

#define JOYGUI_MS 100

#define JOYGUI_TIMER 1
#define SCREENSHOT_TIMER 2
#define GAMEWND_TIMER 3

/* Max size of a sub-menu */
#define DBU_MIN_WIDTH  292
#define DBU_MIN_HEIGHT 190

static int MIN_WIDTH  = DBU_MIN_WIDTH;
static int MIN_HEIGHT = DBU_MIN_HEIGHT;

#define NO_FOLDER -1
#define STATESAVE_VERSION 1
//I could not find a predefined value for this event and docs just say it has 1 for the parameter
#define TOOLBAR_EDIT_ACCELERATOR_PRESSED 1


/***************************************************************************
 externally defined global variables
 ***************************************************************************/
extern const ICONDATA g_iconData[];
extern const TCHAR g_szPlayGameString[];
extern const char g_szGameCountString[];

UINT8 playopts_apply = 0;
static BOOL m_resized = false;

typedef struct _play_options play_options;
struct _play_options
{
	const char *record;      // OPTION_RECORD
	const char *playback;    // OPTION_PLAYBACK
	const char *state;       // OPTION_STATE
	const char *wavwrite;    // OPTION_WAVWRITE
	const char *mngwrite;    // OPTION_MNGWRITE
	const char *aviwrite;    // OPTION_AVIWRITE
};

/***************************************************************************
    function prototypes
 ***************************************************************************/

static BOOL             Win32UI_init(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow);
static void             Win32UI_exit();

static BOOL             PumpMessage();
static BOOL             OnIdle(HWND hWnd);
static void             OnSize(HWND hwnd, UINT state, int width, int height);
static LRESULT CALLBACK MameWindowProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);

static void             SetView(int menu_id);
static void             ResetListView();
static void             UpdateGameList(BOOL bUpdateRomAudit, BOOL bUpdateSampleAudit);
static void             DestroyIcons();
static void             ReloadIcons();
static void             PollGUIJoystick();
//static void             PressKey(HWND hwnd,UINT vk);
static BOOL             MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify);
static void             KeyboardKeyDown(int syskey, int vk_code, int special);
static void             KeyboardKeyUp(int syskey, int vk_code, int special);
static void             KeyboardStateClear();

static void             UpdateStatusBar();
//static BOOL             PickerHitTest(HWND hWnd);
static BOOL             TreeViewNotify(NMHDR *nm);

//static void             ResetBackground(char *szFile);
static void             LoadBackgroundBitmap();
static void             PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y);

static int              GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem);

static void             DisableSelection();
static void             EnableSelection(int nGame);

static HICON            GetSelectedPickItemIcon();
static void             SetRandomPickItem();
static void             PickColor(COLORREF *cDefault);

static LPTREEFOLDER     GetSelectedFolder();
static HICON            GetSelectedFolderIcon();

static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static void             MamePlayRecordGame();
static void             MamePlayBackGame();
static void             MamePlayRecordWave();
static void             MamePlayRecordMNG();
static void             MamePlayRecordAVI();
static void             MameLoadState();
static BOOL             GameCheck();
static BOOL             FolderCheck();

static void             ToggleScreenShot();
static void             AdjustMetrics();

/* Icon routines */
static DWORD            GetShellLargeIconSize();
static DWORD            GetShellSmallIconSize();
static void             CreateIcons();
static int              GetIconForDriver(int nItem);
static void             AddDriverIcon(int nItem,int default_icon_index);

// Context Menu handlers
static void             UpdateMenu(HMENU hMenu);
static void             InitTreeContextMenu(HMENU hTreeMenu);
static void             InitBodyContextMenu(HMENU hBodyContextMenu);
static void             ToggleShowFolder(int folder);
static BOOL             HandleTreeContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static BOOL             HandleScreenShotContextMenu( HWND hWnd, WPARAM wParam, LPARAM lParam);
static void             GamePicker_OnHeaderContextMenu(POINT pt, int nColumn);
static void             GamePicker_OnBodyContextMenu(POINT pt);

static void             InitListView();
/* Re/initialize the ListView header columns */
static void             ResetColumnDisplay(BOOL first_time);

static void             CopyToolTipText (LPTOOLTIPTEXT lpttt);

static void             ProgressBarShow();
static void             ProgressBarHide();
static void             ResizeProgressBar();
static void             ProgressBarStep();
static void             ProgressBarStepParam(int iGameIndex, int nGameCount);

static HWND             InitProgressBar(HWND hParent);
static HWND             InitToolbar(HWND hParent);
static HWND             InitStatusBar(HWND hParent);

static LRESULT          Statusbar_MenuSelect (HWND hwnd, WPARAM wParam, LPARAM lParam);

static void             UpdateHistory(string software);


static void RemoveCurrentGameCustomFolder();
static void RemoveGameCustomFolder(int driver_index);

static void BeginListViewDrag(NM_LISTVIEW *pnmv);
static void MouseMoveListViewDrag(POINTS pt);
static void ButtonUpListViewDrag(POINTS p);

static void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict_height);

BOOL MouseHasBeenMoved();
static void SwitchFullScreenMode();

static HBRUSH hBrush = NULL;
//static HBRUSH hBrushDlg = NULL;
static HDC hDC = NULL;
static HWND	hSplash = NULL;
static HWND	hProgress = NULL;
static intptr_t CALLBACK StartupProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
static bool m_lock = false;    // prevent MAME from being launched twice by accident, and crashing the entire app.
static bool bEnableIndent = false;

static bool CommonListDialog(common_file_dialog_proc cfd);
static void SaveGameListToFile(char *szFile);

/***************************************************************************
    External variables
 ***************************************************************************/

static void load_translation(emu_options &m_options)
{
	util::unload_translation();

	std::string name = m_options.language();
	if (name.empty())
		return;

	strreplace(name, " ", "_");
	strreplace(name, "(", "");
	strreplace(name, ")", "");

	// MESSUI: See if language file exists. If not, try English, see if that exists. If not, use inbuilt default.
	emu_file file(m_options.language_path(), OPEN_FLAG_READ);
	if (file.open(name + PATH_SEPARATOR "strings.mo"))
	{
		osd_printf_verbose("Error opening translation file %s\n", name);
		name = "English";
		if (file.open(name + PATH_SEPARATOR "strings.mo"))
		{
			osd_printf_verbose("Error opening translation file %s\n", name);
			return;
		}
	}

	osd_printf_verbose("Loading translation file %s\n", file.fullpath());
	util::load_translation(file);
}
/***************************************************************************
    Internal structures
 ***************************************************************************/

/*
 * These next two structs represent how the icon information
 * is stored in an ICO file.
 */
typedef struct
{
	BYTE    bWidth;               /* Width of the image */
	BYTE    bHeight;              /* Height of the image (times 2) */
	BYTE    bColorCount;          /* Number of colors in image (0 if >=8bpp) */
	BYTE    bReserved;            /* Reserved */
	WORD    wPlanes;              /* Color Planes */
	WORD    wBitCount;            /* Bits per pixel */
	DWORD   dwBytesInRes;         /* how many bytes in this resource? */
	DWORD   dwImageOffset;        /* where in the file is this image */
} ICONDIRENTRY, *LPICONDIRENTRY;

typedef struct
{
	UINT            Width, Height, Colors; /* Width, Height and bpp */
	LPBYTE          lpBits;                /* ptr to DIB bits */
	DWORD           dwNumBytes;            /* how many bytes? */
	LPBITMAPINFO    lpbi;                  /* ptr to header */
	LPBYTE          lpXOR;                 /* ptr to XOR image bits */
	LPBYTE          lpAND;                 /* ptr to AND image bits */
} ICONIMAGE, *LPICONIMAGE;

/* Which edges of a control are anchored to the corresponding side of the parent window */
#define RA_LEFT     0x01
#define RA_RIGHT    0x02
#define RA_TOP      0x04
#define RA_BOTTOM   0x08
#define RA_ALL      0x0F

#define RA_END  0
#define RA_ID   1
#define RA_HWND 2

typedef struct
{
	int         type;       /* Either RA_ID or RA_HWND, to indicate which member of u is used; or RA_END
                               to signify last entry */
	union                   /* Can identify a child window by control id or by handle */
	{
		int     id;         /* Window control id */
		HWND    hwnd;       /* Window handle */
	} u;
	BOOL        setfont;    /* Do we set this item's font? */
	int         action;     /* What to do when control is resized */
	void        *subwindow; /* Points to a Resize structure for this subwindow; NULL if none */
} ResizeItem;

typedef struct
{
	RECT        rect;       /* Client rect of window; must be initialized before first resize */
	const ResizeItem* items;      /* Array of subitems to be resized */
} Resize;

static void ResizeWindow(HWND hParent, Resize *r);
static void SetAllWindowsFont(HWND hParent, const Resize *r, HFONT hFont, BOOL bRedraw);

/* List view Icon defines */
#define LG_ICONMAP_WIDTH    GetSystemMetrics(SM_CXICON)
#define LG_ICONMAP_HEIGHT   GetSystemMetrics(SM_CYICON)
#define ICONMAP_WIDTH       GetSystemMetrics(SM_CXSMICON)
#define ICONMAP_HEIGHT      GetSystemMetrics(SM_CYSMICON)

typedef struct tagPOPUPSTRING
{
	HMENU hMenu;
	UINT uiString;
} POPUPSTRING;

#define MAX_MENUS 3

// Struct needed for Game Window Communication

typedef struct
{
	LPPROCESS_INFORMATION ProcessInfo;
	HWND hwndFound;
} FINDWINDOWHANDLE;

/***************************************************************************
    Internal variables
 ***************************************************************************/

static HWND   hMain  = NULL;
static HACCEL hAccel = NULL;

static HWND hwndList  = NULL;
static HWND hTreeView = NULL;
static HWND hProgWnd  = NULL;
static HWND hTabCtrl  = NULL;

static HINSTANCE hInst = NULL;

static HFONT hFont = NULL;     /* Font for list view */

static int optionfolder_count = 0;

/* global data--know where to send messages */
static BOOL in_emulation = 0;

/* idle work at startup */
static BOOL idle_work = 0;

static int  game_index = 0;
static int  progBarStep = 0;

static BOOL bDoGameCheck = false;

/* Tree control variables */
static BOOL bShowToolBar   = 1;
static BOOL bShowStatusBar = 1;
static BOOL bShowTabCtrl   = 1;
static BOOL bProgressShown = false;
static BOOL bListReady     = false;

#define	WM_MAME32_FILECHANGED (WM_APP + 100)
#define	WM_MAME32_AUDITGAME   (WM_APP + 101)

#ifdef DIRWATCH
static PDIRWATCHER s_pWatcher;
#endif

/* use a joystick subsystem in the gui? */
static const struct OSDJoystick* g_pJoyGUI = NULL;

/* store current keyboard state (in bools) here */
static bool keyboard_state[4096]; /* __code_max #defines the number of internal key_codes */

/* search */
static char g_SearchText[2048];
/* table copied from windows/inputs.c */
// table entry indices
#define MAME_KEY      0
#define DI_KEY        1
#define VIRTUAL_KEY   2
#define ASCII_KEY     3

// master keyboard translation table
static const int win_key_trans_table[][4] =
{
	// MAME key             dinput key          virtual key     ascii
	{ ITEM_ID_ESC,			DIK_ESCAPE,			VK_ESCAPE,		27 },
	{ ITEM_ID_1,			DIK_1,				'1',			'1' },
	{ ITEM_ID_2,			DIK_2,				'2',			'2' },
	{ ITEM_ID_3,			DIK_3,				'3',			'3' },
	{ ITEM_ID_4,			DIK_4,				'4',			'4' },
	{ ITEM_ID_5,			DIK_5,				'5',			'5' },
	{ ITEM_ID_6,			DIK_6,				'6',			'6' },
	{ ITEM_ID_7,			DIK_7,				'7',			'7' },
	{ ITEM_ID_8,			DIK_8,				'8',			'8' },
	{ ITEM_ID_9,			DIK_9,				'9',			'9' },
	{ ITEM_ID_0,			DIK_0,				'0',			'0' },
	{ ITEM_ID_BACKSPACE,	DIK_BACK,			VK_BACK,		8 },
	{ ITEM_ID_TAB,			DIK_TAB,			VK_TAB, 		9 },
	{ ITEM_ID_Q,			DIK_Q,				'Q',			'Q' },
	{ ITEM_ID_W,			DIK_W,				'W',			'W' },
	{ ITEM_ID_E,			DIK_E,				'E',			'E' },
	{ ITEM_ID_R,			DIK_R,				'R',			'R' },
	{ ITEM_ID_T,			DIK_T,				'T',			'T' },
	{ ITEM_ID_Y,			DIK_Y,				'Y',			'Y' },
	{ ITEM_ID_U,			DIK_U,				'U',			'U' },
	{ ITEM_ID_I,			DIK_I,				'I',			'I' },
	{ ITEM_ID_O,			DIK_O,				'O',			'O' },
	{ ITEM_ID_P,			DIK_P,				'P',			'P' },
	{ ITEM_ID_OPENBRACE,	DIK_LBRACKET,		VK_OEM_4,		'[' },
	{ ITEM_ID_CLOSEBRACE,	DIK_RBRACKET,		VK_OEM_6,		']' },
	{ ITEM_ID_ENTER,		DIK_RETURN, 		VK_RETURN,		13 },
	{ ITEM_ID_LCONTROL, 	DIK_LCONTROL,		VK_LCONTROL,	0 },
	{ ITEM_ID_A,			DIK_A,				'A',			'A' },
	{ ITEM_ID_S,			DIK_S,				'S',			'S' },
	{ ITEM_ID_D,			DIK_D,				'D',			'D' },
	{ ITEM_ID_F,			DIK_F,				'F',			'F' },
	{ ITEM_ID_G,			DIK_G,				'G',			'G' },
	{ ITEM_ID_H,			DIK_H,				'H',			'H' },
	{ ITEM_ID_J,			DIK_J,				'J',			'J' },
	{ ITEM_ID_K,			DIK_K,				'K',			'K' },
	{ ITEM_ID_L,			DIK_L,				'L',			'L' },
	{ ITEM_ID_COLON,		DIK_SEMICOLON,		VK_OEM_1,		';' },
	{ ITEM_ID_QUOTE,		DIK_APOSTROPHE,		VK_OEM_7,		'\'' },
	{ ITEM_ID_TILDE,		DIK_GRAVE,			VK_OEM_3,		'`' },
	{ ITEM_ID_LSHIFT,		DIK_LSHIFT, 		VK_LSHIFT,		0 },
	{ ITEM_ID_BACKSLASH,	DIK_BACKSLASH,		VK_OEM_5,		'\\' },
	{ ITEM_ID_Z,			DIK_Z,				'Z',			'Z' },
	{ ITEM_ID_X,			DIK_X,				'X',			'X' },
	{ ITEM_ID_C,			DIK_C,				'C',			'C' },
	{ ITEM_ID_V,			DIK_V,				'V',			'V' },
	{ ITEM_ID_B,			DIK_B,				'B',			'B' },
	{ ITEM_ID_N,			DIK_N,				'N',			'N' },
	{ ITEM_ID_M,			DIK_M,				'M',			'M' },
	{ ITEM_ID_SLASH,		DIK_SLASH,			VK_OEM_2,		'/' },
	{ ITEM_ID_RSHIFT,		DIK_RSHIFT, 		VK_RSHIFT,		0 },
	{ ITEM_ID_ASTERISK, 	DIK_MULTIPLY,		VK_MULTIPLY,	'*' },
	{ ITEM_ID_LALT, 		DIK_LMENU,			VK_LMENU,		0 },
	{ ITEM_ID_SPACE,		DIK_SPACE,			VK_SPACE,		' ' },
	{ ITEM_ID_CAPSLOCK, 	DIK_CAPITAL,		VK_CAPITAL, 	0 },
	{ ITEM_ID_F1,			DIK_F1,				VK_F1,			0 },
	{ ITEM_ID_F2,			DIK_F2,				VK_F2,			0 },
	{ ITEM_ID_F3,			DIK_F3,				VK_F3,			0 },
	{ ITEM_ID_F4,			DIK_F4,				VK_F4,			0 },
	{ ITEM_ID_F5,			DIK_F5,				VK_F5,			0 },
	{ ITEM_ID_F6,			DIK_F6,				VK_F6,			0 },
	{ ITEM_ID_F7,			DIK_F7,				VK_F7,			0 },
	{ ITEM_ID_F8,			DIK_F8,				VK_F8,			0 },
	{ ITEM_ID_F9,			DIK_F9,				VK_F9,			0 },
	{ ITEM_ID_F10,			DIK_F10,			VK_F10, 		0 },
	{ ITEM_ID_NUMLOCK,		DIK_NUMLOCK,		VK_NUMLOCK, 	0 },
	{ ITEM_ID_SCRLOCK,		DIK_SCROLL,			VK_SCROLL,		0 },
	{ ITEM_ID_7_PAD,		DIK_NUMPAD7,		VK_NUMPAD7, 	0 },
	{ ITEM_ID_8_PAD,		DIK_NUMPAD8,		VK_NUMPAD8, 	0 },
	{ ITEM_ID_9_PAD,		DIK_NUMPAD9,		VK_NUMPAD9, 	0 },
	{ ITEM_ID_MINUS_PAD,	DIK_SUBTRACT,		VK_SUBTRACT,	0 },
	{ ITEM_ID_4_PAD,		DIK_NUMPAD4,		VK_NUMPAD4, 	0 },
	{ ITEM_ID_5_PAD,		DIK_NUMPAD5,		VK_NUMPAD5, 	0 },
	{ ITEM_ID_6_PAD,		DIK_NUMPAD6,		VK_NUMPAD6, 	0 },
	{ ITEM_ID_PLUS_PAD, 	DIK_ADD,			VK_ADD, 		0 },
	{ ITEM_ID_1_PAD,		DIK_NUMPAD1,		VK_NUMPAD1, 	0 },
	{ ITEM_ID_2_PAD,		DIK_NUMPAD2,		VK_NUMPAD2, 	0 },
	{ ITEM_ID_3_PAD,		DIK_NUMPAD3,		VK_NUMPAD3, 	0 },
	{ ITEM_ID_0_PAD,		DIK_NUMPAD0,		VK_NUMPAD0, 	0 },
	{ ITEM_ID_DEL_PAD,		DIK_DECIMAL,		VK_DECIMAL, 	0 },
	{ ITEM_ID_F11,			DIK_F11,			VK_F11, 		0 },
	{ ITEM_ID_F12,			DIK_F12,			VK_F12, 		0 },
	{ ITEM_ID_F13,			DIK_F13,			VK_F13, 		0 },
	{ ITEM_ID_F14,			DIK_F14,			VK_F14, 		0 },
	{ ITEM_ID_F15,			DIK_F15,			VK_F15, 		0 },
	{ ITEM_ID_ENTER_PAD,	DIK_NUMPADENTER,	VK_RETURN,		0 },
	{ ITEM_ID_RCONTROL, 	DIK_RCONTROL,		VK_RCONTROL,	0 },
	{ ITEM_ID_SLASH_PAD,	DIK_DIVIDE,			VK_DIVIDE,		0 },
	{ ITEM_ID_PRTSCR,		DIK_SYSRQ,			0,				0 },
	{ ITEM_ID_RALT, 		DIK_RMENU,			VK_RMENU,		0 },
	{ ITEM_ID_HOME, 		DIK_HOME,			VK_HOME,		0 },
	{ ITEM_ID_UP,			DIK_UP,				VK_UP,			0 },
	{ ITEM_ID_PGUP, 		DIK_PRIOR,			VK_PRIOR,		0 },
	{ ITEM_ID_LEFT, 		DIK_LEFT,			VK_LEFT,		0 },
	{ ITEM_ID_RIGHT,		DIK_RIGHT,			VK_RIGHT,		0 },
	{ ITEM_ID_END,			DIK_END,			VK_END, 		0 },
	{ ITEM_ID_DOWN, 		DIK_DOWN,			VK_DOWN,		0 },
	{ ITEM_ID_PGDN, 		DIK_NEXT,			VK_NEXT,		0 },
	{ ITEM_ID_INSERT,		DIK_INSERT,			VK_INSERT,		0 },
	{ ITEM_ID_DEL,			DIK_DELETE,			VK_DELETE,		0 },
	{ ITEM_ID_LWIN, 		DIK_LWIN,			VK_LWIN,		0 },
	{ ITEM_ID_RWIN, 		DIK_RWIN,			VK_RWIN,		0 },
	{ ITEM_ID_MENU, 		DIK_APPS,			VK_APPS,		0 },
	{ ITEM_ID_PAUSE,		DIK_PAUSE,			VK_PAUSE,		0 },
	{ ITEM_ID_CANCEL,		0,					VK_CANCEL,		0 },
};



typedef struct
{
	char        name[40];    // functionality name (optional)
	input_seq   is;      // the input sequence (the keys pressed)
	UINT        func_id;        // the identifier
	input_seq* (*const getiniptr)();  // pointer to function to get the value from .ini file
} GUISequence;

static const GUISequence GUISequenceControl[]=
{
	{"gui_key_up",                   input_seq(),  ID_UI_UP,                  Get_ui_key_up },
	{"gui_key_down",                 input_seq(),  ID_UI_DOWN,                Get_ui_key_down },
	{"gui_key_left",                 input_seq(),  ID_UI_LEFT,                Get_ui_key_left },
	{"gui_key_right",                input_seq(),  ID_UI_RIGHT,               Get_ui_key_right },
	{"gui_key_start",                input_seq(),  ID_UI_START,               Get_ui_key_start },
	{"gui_key_pgup",                 input_seq(),  ID_UI_PGUP,                Get_ui_key_pgup },
	{"gui_key_pgdwn",                input_seq(),  ID_UI_PGDOWN,              Get_ui_key_pgdwn },
	{"gui_key_home",                 input_seq(),  ID_UI_HOME,                Get_ui_key_home },
	{"gui_key_end",                  input_seq(),  ID_UI_END,                 Get_ui_key_end },
	{"gui_key_ss_change",            input_seq(),  IDC_SSFRAME,               Get_ui_key_ss_change },
	{"gui_key_history_up",           input_seq(),  ID_UI_HISTORY_UP,          Get_ui_key_history_up },
	{"gui_key_history_down",         input_seq(),  ID_UI_HISTORY_DOWN,        Get_ui_key_history_down },

	{"gui_key_context_filters",      input_seq(),  ID_CONTEXT_FILTERS,        Get_ui_key_context_filters },
	{"gui_key_select_random",        input_seq(),  ID_CONTEXT_SELECT_RANDOM,  Get_ui_key_select_random },
	{"gui_key_game_audit",           input_seq(),  ID_GAME_AUDIT,             Get_ui_key_game_audit },
	{"gui_key_game_properties",      input_seq(),  ID_GAME_PROPERTIES,        Get_ui_key_game_properties },
	{"gui_key_help_contents",        input_seq(),  ID_HELP_CONTENTS,          Get_ui_key_help_contents },
	{"gui_key_update_gamelist",      input_seq(),  ID_UPDATE_GAMELIST,        Get_ui_key_update_gamelist },
	{"gui_key_view_folders",         input_seq(),  ID_VIEW_FOLDERS,           Get_ui_key_view_folders },
	{"gui_key_view_fullscreen",      input_seq(),  ID_VIEW_FULLSCREEN,        Get_ui_key_view_fullscreen },
	{"gui_key_view_pagetab",         input_seq(),  ID_VIEW_PAGETAB,           Get_ui_key_view_pagetab },
	{"gui_key_view_picture_area",    input_seq(),  ID_VIEW_PICTURE_AREA,      Get_ui_key_view_picture_area },
	{"gui_key_view_software_area",   input_seq(),  ID_VIEW_SOFTWARE_AREA,     Get_ui_key_view_software_area },
	{"gui_key_view_status",          input_seq(),  ID_VIEW_STATUS,            Get_ui_key_view_status },
	{"gui_key_view_toolbars",        input_seq(),  ID_VIEW_TOOLBARS,          Get_ui_key_view_toolbars },

	{"gui_key_view_tab_cabinet",     input_seq(),  ID_VIEW_TAB_CABINET,       Get_ui_key_view_tab_cabinet },
	{"gui_key_view_tab_cpanel",      input_seq(),  ID_VIEW_TAB_CONTROL_PANEL, Get_ui_key_view_tab_cpanel },
	{"gui_key_view_tab_flyer",       input_seq(),  ID_VIEW_TAB_FLYER,         Get_ui_key_view_tab_flyer },
	{"gui_key_view_tab_history",     input_seq(),  ID_VIEW_TAB_HISTORY,       Get_ui_key_view_tab_history },
	{"gui_key_view_tab_marquee",     input_seq(),  ID_VIEW_TAB_MARQUEE,       Get_ui_key_view_tab_marquee },
	{"gui_key_view_tab_screenshot",  input_seq(),  ID_VIEW_TAB_SCREENSHOT,    Get_ui_key_view_tab_screenshot },
	{"gui_key_view_tab_title",       input_seq(),  ID_VIEW_TAB_TITLE,         Get_ui_key_view_tab_title },
	{"gui_key_view_tab_pcb",         input_seq(),  ID_VIEW_TAB_PCB,           Get_ui_key_view_tab_pcb },
	{"gui_key_quit",                 input_seq(),  ID_FILE_EXIT,              Get_ui_key_quit },
};


#define NUM_GUI_SEQUENCES (sizeof(GUISequenceControl) / sizeof(GUISequenceControl[0]))


static UINT    lastColumnClick   = 0;
static WNDPROC g_lpHistoryWndProc = NULL;
static WNDPROC g_lpPictureFrameWndProc = NULL;
static WNDPROC g_lpPictureWndProc = NULL;

static POPUPSTRING popstr[MAX_MENUS + 1];

/* Tool and Status bar variables */
static HWND hStatusBar = 0;
static HWND s_hToolBar   = 0;

/* Used to recalculate the main window layout */
static int  bottomMargin = 0;
static int  topMargin = 0;
static int  have_history = false;

static BOOL have_selection = false;

static HBITMAP hMissing_bitmap = NULL;

/* Icon variables */
static HIMAGELIST   hLarge = NULL;
static HIMAGELIST   hSmall = NULL;
static HIMAGELIST   hHeaderImages = NULL;
static std::unique_ptr<int[]> icon_index; // for custom per-game icons

static const TBBUTTON tbb[] =
{
	{0, ID_VIEW_FOLDERS,       TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 0},
	{1, ID_VIEW_SOFTWARE_AREA, TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 1},
	{1, ID_VIEW_PICTURE_AREA,  TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 2},
	{0, 0,                     TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{2, ID_VIEW_LARGE_ICON,    TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 3},
	{3, ID_VIEW_SMALL_ICON,    TBSTATE_ENABLED, TBSTYLE_CHECKGROUP, {0, 0}, 0, 4},
	{0, 0,                     TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{6, ID_VIEW_INDENT,        TBSTATE_ENABLED, TBSTYLE_CHECK,      {0, 0}, 0, 5},
	{0, 0,                     TBSTATE_ENABLED, TBSTYLE_SEP,        {0, 0}, 0, 0},
	{7, ID_HELP_ABOUT,         TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 6},
//	{8, ID_HELP_CONTENTS,      TBSTATE_ENABLED, TBSTYLE_BUTTON,     {0, 0}, 0, 7}
};

#define NUM_TOOLBUTTONS (sizeof(tbb) / sizeof(tbb[0]))

#define NUM_TOOLTIPS 8

static const TCHAR szTbStrings[NUM_TOOLTIPS][30] =
{
	TEXT("Toggle Folder List"),
	TEXT("Toggle Software List"),
	TEXT("Toggle Screen Shot"),
	TEXT("Large Icons"),
	TEXT("Small Icons"),
	TEXT("Indent Clones"),
	TEXT("About"),
	TEXT("Help")
};

static const int CommandToString[] =
{
	ID_VIEW_FOLDERS,
	ID_VIEW_SOFTWARE_AREA,
	ID_VIEW_PICTURE_AREA,
	ID_VIEW_LARGE_ICON,
	ID_VIEW_SMALL_ICON,
	ID_VIEW_INDENT,
	ID_HELP_ABOUT,
	ID_HELP_CONTENTS,
	-1
};

static const int s_nPickers[] =
{
	IDC_LIST,
	IDC_SWLIST,
	IDC_SOFTLIST
};


/* How to resize toolbar sub window */
static ResizeItem toolbar_resize_items[] =
{
	{ RA_ID,   { ID_TOOLBAR_EDIT }, true,  RA_RIGHT | RA_TOP, NULL },
	{ RA_END,  { 0 },               false, 0,                 NULL }
};

static Resize toolbar_resize = { {0, 0, 0, 0}, toolbar_resize_items };

/* How to resize main window */
static ResizeItem main_resize_items[] =
{
	{ RA_HWND, { 0 },            false, RA_LEFT  | RA_RIGHT  | RA_TOP,     &toolbar_resize },
	{ RA_HWND, { 0 },            false, RA_LEFT  | RA_RIGHT  | RA_BOTTOM,  NULL },
	{ RA_ID,   { IDC_DIVIDER },  false, RA_LEFT  | RA_RIGHT  | RA_TOP,     NULL },
	{ RA_ID,   { IDC_TREE },     true,  RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_LIST },     true,  RA_ALL,                            NULL },
	{ RA_ID,   { IDC_SPLITTER }, false, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER2 },false, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSFRAME },  false, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSPICTURE },false, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_HISTORY },  true,  RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SSTAB },    false, RA_LEFT  | RA_TOP,                 NULL },
	{ RA_ID,   { IDC_SWLIST },    true, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SOFTLIST },  true, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_ID,   { IDC_SPLITTER3 },false, RA_LEFT  | RA_BOTTOM | RA_TOP,     NULL },
	{ RA_END,  { 0 },            false, 0,                                 NULL }
};

static Resize main_resize = { {0, 0, 0, 0}, main_resize_items };

static wchar_t list_directory[MAX_PATH] = TEXT(".");
static BOOL g_listview_dragging = false;
static HIMAGELIST himl_drag;
static int game_dragged; /* which game started the drag */
static HTREEITEM prev_drag_drop_target; /* which tree view item we're currently highlighting */

static BOOL g_in_treeview_edit = false;

/***************************************************************************
    Global variables
 ***************************************************************************/

/* Background Image handles also accessed from TreeView.cpp */
static HPALETTE         hPALbg   = 0;
static HBITMAP          hBackground  = 0;
static MYBITMAPINFO     bmDesc;

/* List view Column text */
extern const LPCTSTR column_names[COLUMN_MAX] =
{
	TEXT("Machine"),
	TEXT("Source"),
	TEXT("Directory"),
	TEXT("Type"),
	TEXT("Screen"),
	TEXT("Manufacturer"),
	TEXT("Year"),
	TEXT("Played"),
	TEXT("Play Time"),
	TEXT("Clone Of"),
	TEXT("Trackball"),
	TEXT("Samples"),
	TEXT("ROMs"),
};

/***************************************************************************
    Message Macros
 ***************************************************************************/

#ifndef StatusBar_GetItemRect
#define StatusBar_GetItemRect(hWnd, iPart, lpRect) SendMessage(hWnd, SB_GETRECT, (WPARAM) iPart, (LPARAM) (LPRECT) lpRect)
#endif

#ifndef ToolBar_CheckButton
#define ToolBar_CheckButton(hWnd, idButton, fCheck) SendMessage(hWnd, TB_CHECKBUTTON, (WPARAM)idButton, (LPARAM)MAKELONG(fCheck, 0))
#endif

//============================================================
//  winui_output_error
//============================================================


class mameui_output_error : public osd_output
{
public:
	virtual void output_callback(osd_output_channel channel, const util::format_argument_pack<char> &args) override
	{
		std::ostringstream sbuffer;
		util::stream_format(sbuffer, args);
		string s = sbuffer.str();
		const char* buffer = s.c_str();
		if (channel == OSD_OUTPUT_CHANNEL_VERBOSE)
		{
			//printf("%s",buffer);
			return;
		}

		int s_action = 0;

		if (channel == OSD_OUTPUT_CHANNEL_ERROR)
		{
			s_action = 0x80;
		}
		else
		if (channel == OSD_OUTPUT_CHANNEL_WARNING)
		{
			if (strstr(buffer, "WRONG"))
			{
				s_action = 0x81;
			}
		}

		if (s_action)
		{
			// if we are in fullscreen mode, go to windowed mode
			if ((video_config.windowed == 0) && !osd_common_t::window_list().empty())
				winwindow_toggle_full_screen();

			win_message_box_utf8(!osd_common_t::window_list().empty() ?
				dynamic_cast<win_window_info &>(*osd_common_t::window_list().front()).platform_window() :
					hMain, buffer, MAMEUINAME, (BIT(s_action, 0) ? MB_ICONINFORMATION : MB_ICONERROR) | MB_OK);
		}

//		else
//			chain_output(channel, msg, args);   // goes down the black hole
		// LOG all messages
		FILE *pFile;
		pFile = fopen("winui.log", "a");
		fputs(buffer, pFile);
		fflush(pFile);
		fclose (pFile);
/*  List of output types:
		case OSD_OUTPUT_CHANNEL_ERROR:
		case OSD_OUTPUT_CHANNEL_WARNING:
			vfprintf(stderr, msg, args);     // send errors and warnings to standard error (=console)
			break;
		case OSD_OUTPUT_CHANNEL_INFO:
		case OSD_OUTPUT_CHANNEL_LOG:
			vfprintf(stdout, msg, args);     // send info and logging to standard output (=console)
			break;
		case OSD_OUTPUT_CHANNEL_VERBOSE:
			if (verbose()) vfprintf(stdout, msg, args);      // send verbose (2nd half) to console if enabled (first half lost)
			break;
		case OSD_OUTPUT_CHANNEL_DEBUG:   // only for debug builds
			vfprintf(stdout, msg, args);
*/
	}
};


static std::wstring s2ws(const string& s)
{
	int slength = (int)s.length() + 1;
	int len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0); 
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}


/***************************************************************************
    External functions
 ***************************************************************************/
static DWORD RunMAME(int nGameIndex, const play_options *playopts)
{
	int i = 0;
	m_lock = true;
	windows_options global_opts;

	// Tell mame where to get the INIs
	SetDirectories(global_opts);

	SetSystemName(global_opts, OPTIONS_GLOBAL, nGameIndex);
	const char* name = driver_list::driver(nGameIndex).name;

	// set some startup options
	global_opts.set_value(OPTION_PLUGINDATAPATH, GetEmuPath(), OPTION_PRIORITY_HIGH);
	global_opts.set_value(OPTION_LANGUAGE, GetLanguageUI(), OPTION_PRIORITY_HIGH);
	global_opts.set_value(OPTION_PLUGINS, GetEnablePlugins(), OPTION_PRIORITY_HIGH);
	global_opts.set_value(OPTION_PLUGIN, GetPlugins(), OPTION_PRIORITY_HIGH);
	global_opts.set_value(OPTION_SYSTEMNAME, name, OPTION_PRIORITY_HIGH);

	// set any specified play options
	if (playopts_apply == 0x57)
	{
		if (playopts->record)
			global_opts.set_value(OPTION_RECORD, playopts->record, OPTION_PRIORITY_HIGH);
		if (playopts->playback)
			global_opts.set_value(OPTION_PLAYBACK, playopts->playback, OPTION_PRIORITY_HIGH);
		if (playopts->state)
			global_opts.set_value(OPTION_STATE, playopts->state, OPTION_PRIORITY_HIGH);
		if (playopts->wavwrite)
			global_opts.set_value(OPTION_WAVWRITE, playopts->wavwrite, OPTION_PRIORITY_HIGH);
		if (playopts->mngwrite)
			global_opts.set_value(OPTION_MNGWRITE, playopts->mngwrite, OPTION_PRIORITY_HIGH);
		if (playopts->aviwrite)
			global_opts.set_value(OPTION_AVIWRITE, playopts->aviwrite, OPTION_PRIORITY_HIGH);
	}

	// redirect messages to our handler
	mameui_output_error winerror;
	printf("********** STARTING %s **********\n", name);fflush(stdout);
//	osd_output::push(&winerror);
//	osd_printf_verbose("********** STARTING %s **********\n", name);
//	osd_printf_info("********** STARTING %s **********\n", name);
//	osd_output::pop(&winerror);
	// Mame will parse all the needed .ini files.

	// hide mameui
	ShowWindow(hMain, SW_HIDE);

	for (i = 0; i < std::size(s_nPickers); i++)
		Picker_ClearIdle(GetDlgItem(hMain, s_nPickers[i]));

	// run the emulation
	// pass down any command-line arguments
	windows_osd_interface osd(global_opts);
	osd_output::push(&winerror);
	osd.register_options();
	mame_machine_manager *manager = mame_machine_manager::instance(global_opts, osd);
	std::ostringstream option_errors;
	mame_options::parse_standard_inis(global_opts, option_errors);
	load_translation(global_opts);
	// start processes
	manager->start_http_server();
	manager->start_luaengine();
	// run the game
	time_t start = 0, end = 0;
	time(&start);
	manager->execute();
//	osd_printf_info("********** FINISHED %s **********\n", name);
	// turn off message redirect
	osd_output::pop(&winerror);
	delete manager;
	// save game time played
	time(&end);
	double elapsedtime = end - start;
	IncrementPlayTime(nGameIndex, elapsedtime);
	printf("********** FINISHED %s **********\n", name);

	// clear any specified play options
	// do it this way to preserve slots and software entries
	if (playopts_apply == 0x57)
	{
		windows_options o;
		load_options(o, OPTIONS_GAME, nGameIndex, 0);
		if (playopts->record)
			o.set_value(OPTION_RECORD, "", OPTION_PRIORITY_HIGH);
		if (playopts->playback)
			o.set_value(OPTION_PLAYBACK, "", OPTION_PRIORITY_HIGH);
		if (playopts->state)
			o.set_value(OPTION_STATE, "", OPTION_PRIORITY_HIGH);
		if (playopts->wavwrite)
			o.set_value(OPTION_WAVWRITE, "", OPTION_PRIORITY_HIGH);
		if (playopts->mngwrite)
			o.set_value(OPTION_MNGWRITE, "", OPTION_PRIORITY_HIGH);
		if (playopts->aviwrite)
			o.set_value(OPTION_AVIWRITE, "", OPTION_PRIORITY_HIGH);
		// apply the above to the ini file
		save_options(o, OPTIONS_GAME, nGameIndex);
	}
	playopts_apply = 0;

	// the emulation is complete; continue
	for (i = 0; i < std::size(s_nPickers); i++)
		Picker_ResetIdle(GetDlgItem(hMain, s_nPickers[i]));
	ShowWindow(hMain, SW_SHOW);
	SetForegroundWindow(hMain);

	// update display in case software was changed by the machine or by newui
	MessReadMountedSoftware(nGameIndex); // messui.cpp
	//MessUpdateSoftwareList();
	m_lock = false;

	return (DWORD)0;
}


int MameUIMain(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	// delete old log file, ignore any error
	unlink("winui.log");

	if (__argc != 1)
	{
		/* Rename main because gcc will use it instead of WinMain even with -mwindows */
		extern int main_(int argc, char *argv[]);
		exit(main_(__argc, __argv));
	}
	// printf's not allowed before here, else they get into mame queries

	printf("MAMEUI starting\n");fflush(stdout);

	diagnostics_module::get_instance()->init_crash_diagnostics();
	hSplash = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_STARTUP), hMain, StartupProc);
	SetActiveWindow(hSplash);
	SetForegroundWindow(hSplash);

	bool res = Win32UI_init(hInstance, lpCmdLine, nCmdShow);
	DestroyWindow(hSplash);
	if (!res)
		return 1;

	// pump message, but quit on WM_QUIT
	while(PumpMessage())
		;

	Win32UI_exit();
	return 0;
}


HWND GetMainWindow()
{
	return hMain;
}


HWND GetTreeView()
{
	return hTreeView;
}

// used by messui.cpp
HWND GetToolbar()
{
	return s_hToolBar;
}

// used by messui.cpp
HBITMAP GetBackground()
{
	return hBackground;
}


// used by messui.cpp
HIMAGELIST GetLargeImageList()
{
	return hLarge;
}


// used by messui.cpp
HIMAGELIST GetSmallImageList()
{
	return hSmall;
}


void GetRealColumnOrder(int order[])
{
	int tmpOrder[COLUMN_MAX] = {0};
	int nColumnMax = Picker_GetNumColumns(hwndList);

	/* Get the Column Order and save it */
	ListView_GetColumnOrderArray(hwndList, nColumnMax, tmpOrder);

	for (int i = 0; i < nColumnMax; i++)
		order[i] = Picker_GetRealColumnFromViewColumn(hwndList, tmpOrder[i]);
}


/*
 * PURPOSE: Format raw data read from an ICO file to an HICON
 * PARAMS:  PBYTE ptrBuffer  - Raw data from an ICO file
 *          UINT nBufferSize - Size of buffer ptrBuffer
 * RETURNS: HICON - handle to the icon, NULL for failure
 * History: July '95 - Created
 *          March '00- Seriously butchered from MSDN for mine own
 *          purposes, sayeth H0ek.
 */
static HICON FormatICOInMemoryToHICON(PBYTE ptrBuffer, UINT nBufferSize)
{
	/* Is there a WORD? */
	if (nBufferSize < sizeof(WORD))
		return NULL;

	/* Was it 'reserved' ?   (ie 0) */
	UINT nBufferIndex = 0;
	if ((WORD)(ptrBuffer[nBufferIndex]) != 0)
		return NULL;

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
		return NULL;

	/* Was it type 1? */
	if ((WORD)(ptrBuffer[nBufferIndex]) != 1)
		return NULL;

	nBufferIndex += sizeof(WORD);

	/* Is there a WORD? */
	if (nBufferSize - nBufferIndex < sizeof(WORD))
		return NULL;

	/* Then that's the number of images in the ICO file */
	UINT nNumImages = (WORD)(ptrBuffer[nBufferIndex]);

	/* Is there at least one icon in the file? */
	if ( nNumImages < 1 )
		return NULL;

	nBufferIndex += sizeof(WORD);

	/* Is there enough space for the icon directory entries? */
	if ((nBufferIndex + nNumImages * sizeof(ICONDIRENTRY)) > nBufferSize)
		return NULL;

	/* Assign icon directory entries from buffer */
	LPICONDIRENTRY lpIDE = (LPICONDIRENTRY)(&ptrBuffer[nBufferIndex]);
	nBufferIndex += nNumImages * sizeof (ICONDIRENTRY);

	ICONIMAGE IconImage;
	IconImage.dwNumBytes = lpIDE->dwBytesInRes;

	/* Seek to beginning of this image */
	if ( lpIDE->dwImageOffset > nBufferSize )
		return NULL;

	nBufferIndex = lpIDE->dwImageOffset;

	/* Read it in */
	if ((nBufferIndex + lpIDE->dwBytesInRes) > nBufferSize)
		return NULL;

	IconImage.lpBits = &ptrBuffer[nBufferIndex];
	nBufferIndex += lpIDE->dwBytesInRes;
	HICON hIcon = NULL;

	/* We would break on NT if we try with a 16bpp image */
	if (((LPBITMAPINFO)IconImage.lpBits)->bmiHeader.biBitCount != 16)
		hIcon = CreateIconFromResourceEx(IconImage.lpBits, IconImage.dwNumBytes, true, 0x00030000,0,0,LR_DEFAULTSIZE);

	return hIcon;
}


HICON LoadIconFromFile(const char *iconname)
{
	HICON hIcon = 0;
	struct stat file_stat;
	char tmpStr[MAX_PATH];
	char tmpIcoName[MAX_PATH];
	PBYTE bufferPtr = 0;
	util::archive_file::ptr zip;

	const string t = dir_get_value(40);
	char s[t.length()+1];
	strcpy(s, t.c_str());
	char* s1 = strtok(s, ";");
	while (s1 && !hIcon)
	{
		sprintf(tmpStr, "%s/%s.ico", s1, iconname);
		if (stat(tmpStr, &file_stat) != 0 || (hIcon = win_extract_icon_utf8(hInst, tmpStr, 0)) == 0)
		{
			sprintf(tmpStr, "%s/icons.zip", s1);
			sprintf(tmpIcoName, "%s.ico", iconname);

			if (!util::archive_file::open_zip(tmpStr, zip))
			{
				if (zip->search(tmpIcoName, false) >= 0)
				{
					bufferPtr = (PBYTE)malloc(zip->current_uncompressed_length());
					if (bufferPtr)
					{
						if (!zip->decompress(bufferPtr, zip->current_uncompressed_length()))
							hIcon = FormatICOInMemoryToHICON(bufferPtr, zip->current_uncompressed_length());

						free(bufferPtr);
					}
				}
				zip.reset();
			}
			else
			{
				sprintf(tmpStr, "%s/icons.7z", s1);
				sprintf(tmpIcoName, "%s.ico", iconname);

				if (!util::archive_file::open_7z(tmpStr, zip))
				{
					if (zip->search(tmpIcoName, false) >= 0)
					{
						bufferPtr = (PBYTE)malloc(zip->current_uncompressed_length());
						if (bufferPtr)
						{
							if (!zip->decompress(bufferPtr, zip->current_uncompressed_length()))
								hIcon = FormatICOInMemoryToHICON(bufferPtr, zip->current_uncompressed_length());

							free(bufferPtr);
						}
					}
					zip.reset();
				}
			}
		}
		s1 = strtok(NULL, ";");
	}
	return hIcon;
}


/* Return the number of folders with options */
void SetNumOptionFolders(int count)
{
	optionfolder_count = count;
}


/* search */
const char* GetSearchText()
{
	return g_SearchText;
}


/* Sets the treeview and listviews sizes in accordance with their visibility and the splitters */
static void ResizeTreeAndListViews(BOOL bResizeHidden)
{
	AREA area;
	GetWindowArea(&area);
	bool bShowPicture = BIT(GetWindowPanes(), 3);
	bool bShowSoftware = BIT(GetWindowPanes(), 2);
	int nLastWidth = 0;
	int nLeftWindowWidth = 0;

	/* Size the List Control in the Picker */
	RECT rect;
	GetClientRect(hMain, &rect);

	// first time, we use the saved values rather than current ones
	if (!m_resized)
	{
	}
	else
	{
	}
	int fullwidth = area.width;

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	/* Tree control */
	ShowWindow(GetDlgItem(hMain, IDC_TREE), BIT(GetWindowPanes(), 0) ? SW_SHOW : SW_HIDE);

	for (int i = 0; g_splitterInfo[i].nSplitterWindow; i++)
	{
		bool bVisible = GetWindowLong(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), GWL_STYLE) & WS_VISIBLE ? true : false;
		if (bResizeHidden || bVisible)
		{
			nLeftWindowWidth = nSplitterOffset[i] - SPLITTER_WIDTH/2 - nLastWidth;

			/* special case for the rightmost pane when the screenshot is gone */
			if (!bShowPicture && !bShowSoftware && !g_splitterInfo[i+1].nSplitterWindow)
				//nLeftWindowWidth = rect.right - nLastWidth;
				nLeftWindowWidth = fullwidth - nLastWidth;
			//printf("Sizes: nLastWidth %d, fullwidth %d, nLastWidth + nLeftWindowWidth %d\n",nLastWidth,fullwidth,nLastWidth + nLeftWindowWidth);
			if (nLastWidth > fullwidth)
				nLastWidth = fullwidth - MIN_VIEW_WIDTH;
			if ((nLastWidth + nLeftWindowWidth) > fullwidth)
				nLeftWindowWidth = MIN_VIEW_WIDTH;
			//printf("ResizeTreeAndListViews: Window %d, Left %d, Right %d\n",i,nLastWidth, nLeftWindowWidth + nLastWidth);
			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow), nLastWidth, rect.top + 2, nLeftWindowWidth, rect.bottom - rect.top - 4, true); // window

			MoveWindow(GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow), nSplitterOffset[i], rect.top + 2, SPLITTER_WIDTH, rect.bottom - rect.top - 4, true); // splitter
		}

		if (bVisible)
		{
			nLastWidth += nLeftWindowWidth + SPLITTER_WIDTH;
		}
	}
}


/* Adjust the list view and screenshot button based on GetShowScreenShot() */
void UpdateScreenShot()
{
	/* first time through can't do this stuff */
	//printf("Update Screenshot: A\n");fflush(stdout);
	if (hwndList == NULL)
		return;

	/* Size the List Control in the Picker */
	RECT rect;
	GetClientRect(hMain, &rect);

	//printf("Update Screenshot: B\n");fflush(stdout);
	if (bShowStatusBar)
		rect.bottom -= bottomMargin;
	if (bShowToolBar)
		rect.top += topMargin;

	//printf("Update Screenshot: C\n");fflush(stdout);
	BOOL bShowImage = BIT(GetWindowPanes(), 3); // ss
	CheckMenuItem(GetMenu(hMain), ID_VIEW_PICTURE_AREA, bShowImage ? MF_CHECKED : MF_UNCHECKED);
	ToolBar_CheckButton(s_hToolBar, ID_VIEW_PICTURE_AREA, bShowImage ? MF_CHECKED : MF_UNCHECKED);

	//printf("Update Screenshot: F\n");fflush(stdout);
	ResizeTreeAndListViews(false);

	//printf("Update Screenshot: G\n");fflush(stdout);
	FreeScreenShot();

	//printf("Update Screenshot: H\n");fflush(stdout);
	if (have_selection)
		LoadScreenShot(Picker_GetSelectedItem(hwndList), g_szSelectedItem, TabView_GetCurrentTab(hTabCtrl));

	// figure out if we have a history or not, to place our other windows properly
	//printf("Update Screenshot: I\n");fflush(stdout);
	UpdateHistory(g_szSelectedItem);

	// setup the picture area

	//printf("Update Screenshot: J\n");fflush(stdout);
	if (bShowImage)
	{
		DWORD dwStyle;
		DWORD dwStyleEx;
		BOOL showing_history;

		POINT p = {0, 0};
		ClientToScreen(hMain, &p);
		RECT fRect;
		GetWindowRect(GetDlgItem(hMain, IDC_SSFRAME), &fRect);
		OffsetRect(&fRect, -p.x, -p.y);

		// show history on this tab IF
		// - we have history for the game
		// - we're on the first tab
		// - we DON'T have a separate history tab
		showing_history = (have_history && (TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab()
			|| GetHistoryTab() == TAB_ALL ) && GetShowTab(TAB_HISTORY) == false);
		CalculateBestScreenShotRect(GetDlgItem(hMain, IDC_SSFRAME), &rect,showing_history);

		dwStyle   = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_STYLE);
		dwStyleEx = GetWindowLong(GetDlgItem(hMain, IDC_SSPICTURE), GWL_EXSTYLE);

		AdjustWindowRectEx(&rect, dwStyle, false, dwStyleEx);
		MoveWindow(GetDlgItem(hMain, IDC_SSPICTURE), fRect.left + rect.left, fRect.top + rect.top, rect.right - rect.left, rect.bottom - rect.top, true);

		ShowWindow(GetDlgItem(hMain,IDC_SSPICTURE), (TabView_GetCurrentTab(hTabCtrl) != TAB_HISTORY) ? SW_SHOW : SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSFRAME),SW_SHOW);
		ShowWindow(GetDlgItem(hMain,IDC_SSTAB),bShowTabCtrl ? SW_SHOW : SW_HIDE);

		InvalidateRect(GetDlgItem(hMain,IDC_SSPICTURE),NULL,false);
	}
	else
	{
		ShowWindow(GetDlgItem(hMain,IDC_SSPICTURE),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSFRAME),SW_HIDE);
		ShowWindow(GetDlgItem(hMain,IDC_SSTAB),SW_HIDE);
	}
	printf("Update Screenshot: Finished\n");fflush(stdout);
}


void ResizePickerControls(HWND hWnd)
{
	RECT rect, sRect;
	static BOOL afirstTime = true;
	BOOL doSSControls = true;
	int nSplitterCount = GetSplitterCount();

	/* Size the List Control in the Picker */
	GetClientRect(hWnd, &rect);

	/* Calc the display sizes based on g_splitterInfo */
	if (afirstTime)
	{
		for (int i = 0; i < nSplitterCount; i++)
//			nSplitterOffset[i] = rect.right * g_splitterInfo[i].dPosition;
			nSplitterOffset[i] = GetSplitterPos(i);

		RECT rWindow;
		GetWindowRect(hStatusBar, &rWindow);
		bottomMargin = rWindow.bottom - rWindow.top;
		GetWindowRect(s_hToolBar, &rWindow);
		topMargin = rWindow.bottom - rWindow.top;
		/*buttonMargin = (sRect.bottom + 4); */

		afirstTime = false;
	}
	else
	{
		doSSControls = BIT(GetWindowPanes(), 3);
	}

	if (bShowStatusBar)
		rect.bottom -= bottomMargin;

	if (bShowToolBar)
		rect.top += topMargin;

	MoveWindow(GetDlgItem(hWnd, IDC_DIVIDER), rect.left, rect.top - 4, rect.right, 2, true);

	ResizeTreeAndListViews(true);
	int nListWidth = 0;
	if (BIT(GetWindowPanes(), 2)) // sw
		nListWidth = nSplitterOffset[2];
	else
		nListWidth = nSplitterOffset[1];

	int nScreenShotWidth = rect.right - nListWidth - SPLITTER_WIDTH;

	/* Screen shot Page tab control */
	if (bShowTabCtrl)
	{
		MoveWindow(GetDlgItem(hWnd, IDC_SSTAB), nListWidth + 4, rect.top + 2, nScreenShotWidth - 2, rect.top + 20, doSSControls);
		rect.top += 20;
	}

	/* resize the Screen shot frame */
	MoveWindow(GetDlgItem(hWnd, IDC_SSFRAME), nListWidth + 4, rect.top + 2, nScreenShotWidth - 2, rect.bottom - rect.top - 4, doSSControls);

	/* The screen shot controls */
	RECT frameRect;
	GetClientRect(GetDlgItem(hWnd, IDC_SSFRAME), &frameRect);

	/* Text control - game history */
	sRect.left = nListWidth + 14;
	sRect.right = sRect.left + nScreenShotWidth - 22;

	if (GetShowTab(TAB_HISTORY))
	{
		// We're using the new mode, with the history filling the entire tab (almost)
		sRect.top = rect.top + 14;
		sRect.bottom = rect.bottom - rect.top - 30;
	}
	else
	{
		// We're using the original mode, with the history beneath the SS picture
		sRect.top = rect.top + 264;
		sRect.bottom = rect.bottom - rect.top - 278;
	}

	MoveWindow(GetDlgItem(hWnd, IDC_HISTORY), sRect.left, sRect.top, sRect.right - sRect.left, sRect.bottom, doSSControls);

	/* the other screen shot controls will be properly placed in UpdateScreenshot() */
}


char *ModifyThe(const char *str)
{
	static int  bufno = 0;
	static char buffer[4][2048];

	if (strncmp(str, "The ", 4) == 0)
	{
		char *s, *p;
		char temp[2048];

		strcpy(temp, &str[4]);

		bufno = (bufno + 1) % 4;

		s = buffer[bufno];

		/* Check for version notes in parens */
		p = strchr(temp, '(');
		if (p)
			p[-1] = '\0';

		strcpy(s, temp);
		strcat(s, ", The");

		if (p)
		{
			strcat(s, " ");
			strcat(s, p);
		}

		return s;
	}
	return (char *)str;
}


HBITMAP GetBackgroundBitmap()
{
	return hBackground;
}


HPALETTE GetBackgroundPalette()
{
	return hPALbg;
}


MYBITMAPINFO * GetBackgroundInfo()
{
	return &bmDesc;
}


int GetMinimumScreenShotWindowWidth()
{
	BITMAP bmp;
	GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);

	return bmp.bmWidth + 6; // 6 is for a little breathing room
}


int GetParentIndex(const game_driver *driver)
{
	return GetGameNameIndex(driver->parent);
}


int GetCompatIndex(const game_driver *driver)
{
	const char *t = driver->compatible_with;
	if (t)
	{
		return GetGameNameIndex(t);
	}
	else
	{
		return -1;
	}
}


int GetParentRomSetIndex(const game_driver *driver)
{
	int nParentIndex = GetGameNameIndex(driver->parent);

	if( nParentIndex >= 0)
	{
		if ((driver_list::driver(nParentIndex).flags & MACHINE_IS_BIOS_ROOT) == 0)
			return nParentIndex;
	}

	return -1;
}


int GetGameNameIndex(const char *name)
{
	return driver_list::find(name);
}


/***************************************************************************
    Internal functions
 ***************************************************************************/

static void SetMainTitle()
{
	char buffer[100];
	snprintf(buffer, std::size(buffer), "%s %s", MAMEUINAME, GetVersionString());
	win_set_window_text_utf8(hMain,buffer);
}


//static void memory_error(const char *message)
//{
//	win_message_box_utf8(hMain, message, emulator_info::get_appname(), MB_OK);
//	exit(-1);
//}


static intptr_t CALLBACK StartupProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// Need a correctly-sized bitmap
			HBITMAP hBmp = (HBITMAP)LoadImage(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH), IMAGE_BITMAP, 0, 0, LR_SHARED);
			SendMessage(GetDlgItem(hDlg, IDC_SPLASH), STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBmp);
			hBrush = GetSysColorBrush(COLOR_3DFACE);
			hProgress = CreateWindowEx(0, PROGRESS_CLASS, NULL, WS_CHILD | WS_VISIBLE, 0, 136, 526, 18, hDlg, NULL, hInst, NULL);
			SetWindowTheme(hProgress, L" ", L" ");
			SendMessage(hProgress, PBM_SETBKCOLOR, 0, GetSysColor(COLOR_3DFACE));
			//SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendMessage(hProgress, PBM_SETPOS, 0, 0);
			return true;
		}

		case WM_CTLCOLORDLG:
			return (LRESULT) hBrush;

		case WM_CTLCOLORSTATIC:
			hDC = (HDC)wParam;
			SetBkMode(hDC, TRANSPARENT);
			SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHT));
			return (LRESULT) hBrush;
	}

	return false;
}


static BOOL Win32UI_init(HINSTANCE hInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	win_set_window_text_utf8(GetDlgItem(hSplash, IDC_PROGBAR), "Please wait...");
	SendMessage(hProgress, PBM_SETPOS, 10, 0);

	extern const FOLDERDATA g_folderData[];
	extern const FILTER_ITEM g_filterList[];
	m_resized = false;

	printf("Win32UI_init: About to init options\n");fflush(stdout);
	OptionsInit();
	SendMessage(hProgress, PBM_SETPOS, 25, 0);
	emu_opts_init(0);
	printf("Win32UI_init: Options loaded\n");fflush(stdout);
	SendMessage(hProgress, PBM_SETPOS, 40, 0);
	//win_message_box_utf8(hMain, "test", emulator_info::get_appname(), MB_OK);

	// custom per-game icons
	icon_index = make_unique_clear<int[]>(driver_list::total());

	// set up window class
	WNDCLASS wndclass;
	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = MameWindowProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = DLGWINDOWEXTRA;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAMEUI));
	wndclass.hCursor       = NULL;
	wndclass.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
	wndclass.lpszMenuName  = MAKEINTRESOURCE(IDR_UI_MENU);
	wndclass.lpszClassName = TEXT("MainClass");

	RegisterClass(&wndclass);

	MView_RegisterClass(); // messui.cpp

	InitCommonControls();
	SendMessage(hProgress, PBM_SETPOS, 55, 0);

	// Are we using an Old comctl32.dll?
	LONG common_control_version = GetCommonControlVersion();
	printf("COMCTL32.DLL version = %ld %ld\n",common_control_version >> 16, common_control_version & 0xffff);

	if (common_control_version < PACKVERSION(6,0))
	{
		char buf[] = MAMEUINAME " needs COMCTL32.DLL version 6.0\n\nUnable to proceed.\n\n";

		win_message_box_utf8(0, buf, MAMEUINAME " Outdated COMCTL32.DLL Error", MB_OK | MB_ICONWARNING);
		return false;
	}

	HelpInit();

	hMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, NULL);
	if (hMain == NULL)
	{
		printf("Win32UI_init: Error creating main dialog, aborting\n");fflush(stdout);
		return false;
	}

#ifdef DIRWATCH
	s_pWatcher = DirWatcher_Init(hMain, WM_MAME32_FILECHANGED);
	if (s_pWatcher)
	{
		DirWatcher_Watch(s_pWatcher, 0, dir_get_value(2), false);  // roms
		DirWatcher_Watch(s_pWatcher, 1, dir_get_value(4), false);  // samples
		DirWatcher_Watch(s_pWatcher, 2, dir_get_value(13), true);  // loose sw
	}
#endif

	SetMainTitle();
	hTabCtrl = GetDlgItem(hMain, IDC_SSTAB);
	SendMessage(hProgress, PBM_SETPOS, 70, 0);

	{
		struct TabViewOptions opts;

		static const struct TabViewCallbacks s_tabviewCallbacks =
		{
			GetShowTabCtrl,			// pfnGetShowTabCtrl
			SetCurrentTab,			// pfnSetCurrentTab
			GetCurrentTab,			// pfnGetCurrentTab
			SetShowTab,				// pfnSetShowTab
			GetShowTab,				// pfnGetShowTab

			GetImageTabShortName,	// pfnGetTabShortName
			GetImageTabLongName,	// pfnGetTabLongName
			UpdateScreenShot		// pfnOnSelectionChanged
		};

		memset(&opts, 0, sizeof(opts));
		opts.pCallbacks = &s_tabviewCallbacks;
		opts.nTabCount = MAX_TAB_TYPES;

		if (!SetupTabView(hTabCtrl, &opts))
			return false;
	}

	/* subclass history window */
	LONG_PTR l = GetWindowLongPtr(GetDlgItem(hMain, IDC_HISTORY), GWLP_WNDPROC);
	g_lpHistoryWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetDlgItem(hMain, IDC_HISTORY), GWLP_WNDPROC, (LONG_PTR)HistoryWndProc);

	/* subclass picture frame area */
	l = GetWindowLongPtr(GetDlgItem(hMain, IDC_SSFRAME), GWLP_WNDPROC);
	g_lpPictureFrameWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetDlgItem(hMain, IDC_SSFRAME), GWLP_WNDPROC, (LONG_PTR)PictureFrameWndProc);

	/* subclass picture area */
	l = GetWindowLongPtr(GetDlgItem(hMain, IDC_SSPICTURE), GWLP_WNDPROC);
	g_lpPictureWndProc = (WNDPROC)l;
	SetWindowLongPtr(GetDlgItem(hMain, IDC_SSPICTURE), GWLP_WNDPROC, (LONG_PTR)PictureWndProc);

	/* Load the pic for the default screenshot. */
	hMissing_bitmap = LoadBitmap(GetModuleHandle(NULL),MAKEINTRESOURCE(IDB_ABOUT));

	/* Stash hInstance for later use */
	hInst = hInstance;

	s_hToolBar   = InitToolbar(hMain);
	hStatusBar = InitStatusBar(hMain);
	hProgWnd   = InitProgressBar(hStatusBar);

	main_resize_items[0].u.hwnd = s_hToolBar;
	main_resize_items[1].u.hwnd = hStatusBar;

	/* In order to handle 'Large Fonts' as the Windows
     * default setting, we need to make the dialogs small
     * enough to fit in our smallest window size with
     * large fonts, then resize the picker, tab and button
     * controls to fill the window, no matter which font
     * is currently set.  This will still look like bad
     * if the user uses a bigger default font than 125%
     * (Large Fonts) on the Windows display setting tab.
     *
     * NOTE: This has to do with Windows default font size
     * settings, NOT our picker font size.
     */

	RECT rect;
	GetClientRect(hMain, &rect);

	hTreeView = GetDlgItem(hMain, IDC_TREE);
	hwndList  = GetDlgItem(hMain, IDC_LIST);

	if (!InitSplitters())
		return false;

	int nSplitterCount = GetSplitterCount();
	for (int i = 0; i < nSplitterCount; i++)
	{
		HWND hWnd = GetDlgItem(hMain, g_splitterInfo[i].nSplitterWindow);
		HWND hWndLeft = GetDlgItem(hMain, g_splitterInfo[i].nLeftWindow);
		HWND hWndRight = GetDlgItem(hMain, g_splitterInfo[i].nRightWindow);

		AddSplitter(hWnd, hWndLeft, hWndRight, g_splitterInfo[i].pfnAdjust);
	}

	/* Initial adjustment of controls on the Picker window */
	ResizePickerControls(hMain);

	TabView_UpdateSelection(hTabCtrl);

	bDoGameCheck = GetGameCheck();
	idle_work    = true;
	game_index   = 0;

	bShowToolBar   = GetShowToolBar();
	bShowStatusBar = GetShowStatusBar();
	bShowTabCtrl   = GetShowTabCtrl();
	bEnableIndent = GetEnableIndent();

	CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, BIT(GetWindowPanes(), 0) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(s_hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
	ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, (bShowTabCtrl) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_SOFTWARE_AREA, BIT(GetWindowPanes(), 2) ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(GetMenu(hMain), ID_VIEW_PICTURE_AREA, BIT(GetWindowPanes(), 3) ? MF_CHECKED : MF_UNCHECKED);
	if (bShowToolBar)
	{
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_FOLDERS, BIT(GetWindowPanes(), 0) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_SOFTWARE_AREA, BIT(GetWindowPanes(), 2) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_PICTURE_AREA, BIT(GetWindowPanes(), 3) ? MF_CHECKED : MF_UNCHECKED);
	}

	LoadBackgroundBitmap();

	SendMessage(hProgress, PBM_SETPOS, 85, 0);
	printf("Win32UI_init: About to init tree\n");fflush(stdout);
	InitTree(g_folderData, g_filterList);
	printf("Win32UI_init: Did init tree\n");fflush(stdout);
	SendMessage(hProgress, PBM_SETPOS, 100, 0);

	/* Initialize listview columns */
	InitMessPicker(); // messui.cpp
	printf("Win32UI_init: About to InitListView\n");fflush(stdout);
	InitListView();
	SetFocus(hwndList);

	/* Reset the font */
	printf("Win32UI_init: Reset the font\n");fflush(stdout);
	{
		LOGFONT logfont;

		GetListFont(&logfont);
		if (hFont)
		{
			//Cleanup old Font, otherwise we have a GDI handle leak
			DeleteFont(hFont);
		}
		hFont = CreateFontIndirect(&logfont);
		if (hFont)
			SetAllWindowsFont(hMain, &main_resize, hFont, false);
	}

	/* Init DirectInput */
	printf("Win32UI_init: Init directinput\n");fflush(stdout);
	if (!DirectInputInitialize())
	{
		DialogBox(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_DIRECTX), NULL, DirectXDialogProc);
		return false;
	}

	printf("Win32UI_init: Adjusting..\n");fflush(stdout);
	AdjustMetrics();
	//ShowHideSoftwareArea();   // messui.cpp
	UpdateScreenShot();

	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDA_TAB_KEYS));

	/* clear keyboard state */
	printf("Win32UI_init: Keyboard\n");fflush(stdout);
	KeyboardStateClear();

	if (GetJoyGUI() == true)
	{
		g_pJoyGUI = &DIJoystick;
		if (g_pJoyGUI->init() != 0)
			g_pJoyGUI = NULL;
		else
			SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);
	}
	else
		g_pJoyGUI = NULL;

	printf("Win32UI_init: Mouse\n");fflush(stdout);
	if (GetHideMouseOnStartup())
	{
		/*  For some reason the mouse is centered when a game is exited, which of
            course causes a WM_MOUSEMOVE event that shows the mouse. So we center
            it now, before the startup coords are initialised, and that way the mouse
            will still be hidden when exiting from a game (i hope) :) */
		SetCursorPos(GetSystemMetrics(SM_CXSCREEN)/2,GetSystemMetrics(SM_CYSCREEN)/2);

		// Then hide it
		ShowCursor(false);
	}

	printf("Win32UI_init: About to show window\n");fflush(stdout);

	nCmdShow = GetWindowState();
	if (nCmdShow == SW_HIDE || nCmdShow == SW_MINIMIZE || nCmdShow == SW_SHOWMINIMIZED)
		nCmdShow = SW_RESTORE;

	if (GetRunFullScreen())
	{
		LONG lMainStyle;

		// Remove menu
		SetMenu(hMain,NULL);

		// Frameless dialog (fake fullscreen)
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);

		nCmdShow = SW_MAXIMIZE;
	}

	ShowWindow(hMain, nCmdShow);


	switch (GetViewMode())
	{
	case VIEW_LARGE_ICONS :
		SetView(ID_VIEW_LARGE_ICON);
		break;
	default :
		SetView(ID_VIEW_SMALL_ICON);
		break;
	}

	if (GetCycleScreenshot() > 0)
	{
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to Seconds
	}

	return true;
}


static void Win32UI_exit()
{
	printf("Exiting...\n");fflush(stdout);

	MySoftwareListClose(); // messui.cpp

#ifdef DIRWATCH
	if (s_pWatcher)
		DirWatcher_Free(s_pWatcher);
#endif

	if (g_pJoyGUI)
		g_pJoyGUI->exit();

	/* Free GDI resources */
	if (hMain) {
		DeleteObject(hMain);
		hMain = NULL;
	}

	if (hMissing_bitmap)
	{
		DeleteBitmap(hMissing_bitmap);
		hMissing_bitmap = NULL;
	}

	if (hBackground)
	{
		DeleteBitmap(hBackground);
		hBackground = NULL;
	}

	if (hPALbg)
	{
		DeletePalette(hPALbg);
		hPALbg = NULL;
	}

	if (hFont)
	{
		DeleteFont(hFont);
		hFont = NULL;
	}

	DestroyIcons();

	DestroyAcceleratorTable(hAccel);

	DirectInputClose();

	SetSavedFolderID(GetCurrentFolderID());
	SaveGameListOptions();
	mui_save_ini();
	ui_save_ini();

	FreeFolders();

    /* DestroyTree(hTreeView); */

	FreeScreenShot();

	HelpExit();

	printf("Successful end of program\n");fflush(stdout);
}


static LRESULT CALLBACK MameWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	MINMAXINFO *mminfo;
	int i = 0;
	TCHAR szClass[128];
	BOOL res = 0;

	switch (message)
	{
	case WM_CTLCOLORSTATIC:
		if (hBackground && (HWND)lParam == GetDlgItem(hMain, IDC_HISTORY))
		{
			static HBRUSH hBrush=0;
			HDC hDC=(HDC)wParam;

			if (hBrush)
				DeleteBrush(hBrush);
			LOGBRUSH lb;
			lb.lbStyle = BS_HOLLOW;
			hBrush = CreateBrushIndirect(&lb);
			SetBkMode(hDC, TRANSPARENT);
			SetTextColor(hDC, GetListFontColor());
			return (LRESULT) hBrush;
		}
		break;

	case WM_INITDIALOG:
		/* Initialize info for resizing subitems */
		GetClientRect(hWnd, &main_resize.rect);
		return true;

	case WM_SETFOCUS:
		SetFocus(hwndList);
		break;

	case WM_SETTINGCHANGE:
		AdjustMetrics();
		return 0;

	case WM_SIZE:
		OnSize(hWnd, wParam, LOWORD(lParam), HIWORD(lParam));
		return true;

	case WM_MENUSELECT:
		return Statusbar_MenuSelect(hWnd, wParam, lParam);

	case MM_PLAY_GAME:
		MamePlayGame();
		return true;

	case WM_INITMENUPOPUP:
		UpdateMenu(GetMenu(hWnd));
		break;

	case WM_CONTEXTMENU:
		if (HandleTreeContextMenu(hWnd, wParam, lParam) || HandleScreenShotContextMenu(hWnd, wParam, lParam))
			return false;
		break;

	case WM_COMMAND:
		return MameCommand(hWnd,(int)(LOWORD(wParam)),(HWND)(lParam),(UINT)HIWORD(wParam));

	case WM_GETMINMAXINFO:
		/* Don't let the window get too small; it can break resizing */
		mminfo = (MINMAXINFO *) lParam;
		mminfo->ptMinTrackSize.x = MIN_WIDTH;
		mminfo->ptMinTrackSize.y = MIN_HEIGHT;
		return 0;

	case WM_TIMER:
		switch (wParam)
		{
		case JOYGUI_TIMER:
			PollGUIJoystick();
			break;
		case SCREENSHOT_TIMER:
			TabView_CalculateNextTab(hTabCtrl);
			UpdateScreenShot();
			TabView_UpdateSelection(hTabCtrl);
			break;
		default:
			break;
		}
		return true;

	case WM_CLOSE:
		{
			/* save current item */
			WINDOWPLACEMENT wndpl;

			wndpl.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(hMain, &wndpl);
			UINT state = wndpl.showCmd;

			// Restore the window before we attempt to save parameters,
			// This fixed the lost window on startup problem, among other problems
			if (state == SW_MINIMIZE || state == SW_SHOWMINIMIZED || state == SW_MAXIMIZE)
			{
				if( wndpl.flags & WPF_RESTORETOMAXIMIZED || state == SW_MAXIMIZE)
					state = SW_MAXIMIZE;
				else
				{
					state = SW_RESTORE;
					ShowWindow(hWnd, SW_RESTORE);
				}
			}
			for (i = 0; i < GetSplitterCount(); i++)
				SetSplitterPos(i, nSplitterOffset[i]);
			SetWindowState(state);

			for (i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
				Picker_SaveColumnWidths(GetDlgItem(hMain, s_nPickers[i]));

			// Save the current gui screen dimensions
			RECT rect;
			AREA area;
			GetWindowRect(hWnd, &rect);
			area.x = rect.left;
			area.y = rect.top;
			area.width = rect.right - rect.left;
			area.height = rect.bottom - rect.top;
			SetWindowArea(&area);

			/* Save the users current game options and default game */
			int nItem = Picker_GetSelectedItem(hwndList);
			SetDefaultGame(nItem);

			/* hide window to prevent orphan empty rectangles on the taskbar */
			/* ShowWindow(hWnd,SW_HIDE); */
			DestroyWindow( hWnd );

			/* PostQuitMessage(0); */
			break;
		}

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_LBUTTONDOWN:
		OnLButtonDown(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

		/*
          Check to see if the mouse has been moved by the user since
          startup. I'd like this checking to be done only in the
          main WinProc (here), but somehow the WM_MOUSEDOWN messages
          are eaten up before they reach MameWindowProc. That's why
          there is one check for each of the subclassed windows too.

          POSSIBLE BUGS:
          I've included this check in the subclassed windows, but a
          mouse move in either the title bar, the menu, or the
          toolbar will not generate a WM_MOUSEOVER message. At least
          not one that I know how to pick up. A solution could maybe
          be to subclass those too, but that's too much work :) */

	case WM_MOUSEMOVE:
	{
		if (MouseHasBeenMoved())
			ShowCursor(true);

		if (g_listview_dragging)
			MouseMoveListViewDrag(MAKEPOINTS(lParam));
		else
			/* for splitters */
			OnMouseMove(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;
	}

	case WM_LBUTTONUP:
		if (g_listview_dragging)
			ButtonUpListViewDrag(MAKEPOINTS(lParam));
		else
			/* for splitters */
			OnLButtonUp(hWnd, (UINT)wParam, MAKEPOINTS(lParam));
		break;

	case WM_NOTIFY:
		/* Where is this message intended to go */
		{
			LPNMHDR lpNmHdr = (LPNMHDR)lParam;

			/* Fetch tooltip text */
			if (lpNmHdr->code == TTN_NEEDTEXT)
			{
				LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT)lParam;
				CopyToolTipText(lpttt);
			}

			if (lpNmHdr->hwndFrom == hTreeView)
				return TreeViewNotify(lpNmHdr);

			GetClassName(lpNmHdr->hwndFrom, szClass, sizeof(szClass) / sizeof(szClass[0]));
			if (!_tcscmp(szClass, TEXT("SysListView32")))
				return Picker_HandleNotify(lpNmHdr);
			if (!_tcscmp(szClass, TEXT("SysTabControl32")))
				return TabView_HandleNotify(lpNmHdr);
		}
		break;

	case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpDis = (LPDRAWITEMSTRUCT)lParam;

			GetClassName(lpDis->hwndItem, szClass, sizeof(szClass) / sizeof(szClass[0]));
			if (!_tcscmp(szClass, TEXT("SysListView32")))
				Picker_HandleDrawItem(GetDlgItem(hMain, lpDis->CtlID), lpDis);
		}
		break;

	case WM_MEASUREITEM :
	{
		LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT) lParam;

		// tell the list view that each row (item) should be just taller than our font

		//DefWindowProc(hWnd, message, wParam, lParam);
		//printf("default row height calculation gives %u\n",lpmis->itemHeight);fflush(stdout);

		TEXTMETRIC tm;
		HDC hDC = GetDC(NULL);
		HFONT hFontOld = (HFONT)SelectObject(hDC,hFont);

		GetTextMetrics(hDC,&tm);

		lpmis->itemHeight = tm.tmHeight + tm.tmExternalLeading + 1;
		if (lpmis->itemHeight < 17)
			lpmis->itemHeight = 17;
		//printf("we would do %u\n",tm.tmHeight + tm.tmExternalLeading + 1);fflush(stdout);
		SelectObject(hDC,hFontOld);
		ReleaseDC(NULL,hDC);

		return true;
	}

	case WM_MAME32_FILECHANGED:
		{
			int (*pfnGetAuditResults)(int driver_index) = NULL;
			void (*pfnSetAuditResults)(int driver_index, int audit_results) = NULL;

			switch(HIWORD(wParam))
			{
				case 0:
					pfnGetAuditResults = GetRomAuditResults;
					pfnSetAuditResults = SetRomAuditResults;
					break;
				case 1:
					pfnGetAuditResults = GetSampleAuditResults;
					pfnSetAuditResults = SetSampleAuditResults;
					break;
			}

			if (pfnGetAuditResults && pfnSetAuditResults)
			{
				int nGameIndex, nParentIndex = -1;
				char szFileName[256];
				snprintf(szFileName, sizeof(szFileName), "%s", (LPCSTR) lParam);
				char* s = strchr(szFileName, '.');
				if (s)
					*s = '\0';
				s = strchr(szFileName, '\\');
				if (s)
					*s = '\0';

				for (nGameIndex = 0; nGameIndex  < driver_list::total(); nGameIndex++)
				{
					for (nParentIndex = nGameIndex; nGameIndex == -1; nParentIndex = GetParentIndex(&driver_list::driver(nParentIndex)))
					{
						if (core_stricmp(driver_list::driver(nParentIndex).name, szFileName)==0)
						{
							if (pfnGetAuditResults(nGameIndex) != UNKNOWN)
							{
								pfnSetAuditResults(nGameIndex, UNKNOWN);
								PostMessage(hMain, WM_MAME32_AUDITGAME, wParam, nGameIndex);
							}
							break;
						}
					}
				}
			}
		}
		break;

	case WM_MAME32_AUDITGAME:
		{
			LV_FINDINFO lvfi;

			int nGameIndex = lParam;

			switch(HIWORD(wParam))
			{
				case 0:
					MameUIVerifyRomSet(nGameIndex, 0);
					break;
				case 1:
					MameUIVerifySampleSet(nGameIndex);
					break;
			}

			memset(&lvfi, 0, sizeof(lvfi));
			lvfi.flags = LVFI_PARAM;
			lvfi.lParam = nGameIndex;

			i = ListView_FindItem(hwndList, -1, &lvfi);
			if (i != -1)
			{
				res = ListView_RedrawItems(hwndList, i, i);
			}
		}
		break;

	default:
		break;
	}
	res++;
	return DefWindowProc(hWnd, message, wParam, lParam);
}


static int HandleKeyboardGUIMessage(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	switch (message)
	{
		case WM_CHAR: /* List-View controls use this message for searching the items "as user types" */
			//MessageBox(NULL,"wm_char message arrived","TitleBox",MB_OK);
			return true;

		case WM_KEYDOWN:
			KeyboardKeyDown(0, wParam, lParam);
			return true;

		case WM_KEYUP:
			KeyboardKeyUp(0, wParam, lParam);
			return true;

		case WM_SYSKEYDOWN:
			KeyboardKeyDown(1, wParam, lParam);
			return true;

		case WM_SYSKEYUP:
			KeyboardKeyUp(1, wParam, lParam);
			return true;
	}

	return false; /* message not processed */
}


static BOOL PumpMessage()
{
	MSG msg;

	if (!GetMessage(&msg, NULL, 0, 0))
		return false;

	if (IsWindow(hMain))
	{
		BOOL absorbed_key = false;
		if (GetKeyGUI())
			absorbed_key = HandleKeyboardGUIMessage(msg.hwnd, msg.message, msg.wParam, msg.lParam);
		else
			absorbed_key = TranslateAccelerator(hMain, hAccel, &msg);

		if (!absorbed_key)
		{
			if (!IsDialogMessage(hMain, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	return true;
}


static BOOL FolderCheck()
{
	int nGameIndex = 0;
	int i=0;
	LV_FINDINFO lvfi;
	int nCount = ListView_GetItemCount(hwndList);
	BOOL res = 0;
	MSG msg;

	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;
		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;
		res = ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		SetRomAuditResults(nGameIndex, UNKNOWN);
	}
	if( nCount > 0)
		ProgressBarShow();
	else
		return false;

	int iStep = 0;
	if( nCount < 100 )
		iStep = 100 / nCount;
	else
		iStep = nCount/100;

	UpdateListView();
	UpdateWindow(hMain);

	BOOL changed = false;
	for(i=0; i<nCount;i++)
	{
		LV_ITEM lvi;

		lvi.iItem = i;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;
		res = ListView_GetItem(hwndList, &lvi);
		nGameIndex  = lvi.lParam;
		if (GetRomAuditResults(nGameIndex) == UNKNOWN)
		{
			MameUIVerifyRomSet(nGameIndex, 0);
			changed = true;
		}

		if (GetSampleAuditResults(nGameIndex) == UNKNOWN)
		{
			MameUIVerifySampleSet(nGameIndex);
			changed = true;
		}

		lvfi.flags = LVFI_PARAM;
		lvfi.lParam = nGameIndex;

		i = ListView_FindItem(hwndList, -1, &lvfi);
		if (changed && i != -1)
		{
			res = ListView_RedrawItems(hwndList, i, i);
			while( PeekMessage( &msg, hwndList, 0, 0, PM_REMOVE ) != 0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		changed = false;
		if ((i % iStep) == 0)
			ProgressBarStepParam(i, nCount);
	}
	ProgressBarHide();
	const char* pDescription;
	if (Picker_GetSelectedItem(hwndList) >= 0)
		pDescription = ModifyThe(driver_list::driver(Picker_GetSelectedItem(hwndList)).type.fullname());
	else pDescription = "No Selection";
	SetStatusBarText(0, pDescription);
	UpdateStatusBar();
	res++;
	return true;
}


static BOOL GameCheck()
{
	if (game_index == 0)
		ProgressBarShow();

	if (game_index >= driver_list::total())
	{
		bDoGameCheck = false;
		ProgressBarHide();
		ResetWhichGamesInFolders();
		ResetListView(); // reset the list after F5
		return false;
	}

	BOOL changed = false;
	if (GetRomAuditResults(game_index) == UNKNOWN)
	{
		MameUIVerifyRomSet(game_index, 0);
		changed = true;
	}

	if (GetSampleAuditResults(game_index) == UNKNOWN)
	{
		MameUIVerifySampleSet(game_index);
		changed = true;
	}

	LV_FINDINFO lvfi;
	lvfi.flags = LVFI_PARAM;
	lvfi.lParam = game_index;

	int i = ListView_FindItem(hwndList, -1, &lvfi);

	BOOL res = 0;
	if (changed && i != -1)
		res = ListView_RedrawItems(hwndList, i, i);

	if ((game_index % progBarStep) == 0)
		ProgressBarStep();

	game_index++;
	res++;
	return changed;
}


static BOOL OnIdle(HWND hWnd)
{
	static int bFirstTime = true;
	static int bResetList = true;

	if (bFirstTime)
	{
		bResetList = false;
		bFirstTime = false;
	}
	if (bDoGameCheck)
	{
		bResetList |= GameCheck();
		return idle_work;
	}
	// NPW 17-Jun-2003 - Commenting this out because it is redundant
	// and it causes the game to reset back to the original game after an F5
	// refresh
	//drvindex = GetGameNameIndex(GetDefaultGame());
	//SetSelectedPickItem(drvindex);

	// in case it's not found, get it back
	const char *pDescription;
	int drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex >= 0)
		pDescription = ModifyThe(driver_list::driver(drvindex).type.fullname());
	else
		pDescription = "No Selection";
	SetStatusBarText(0, pDescription);
	idle_work = false;
	UpdateStatusBar();
	bFirstTime = true;

// don't need this any more 2014-01-26
//	if (!idle_work)
//		PostMessage(GetMainWindow(),WM_COMMAND, MAKEWPARAM(ID_VIEW_LINEUPICONS, true),(LPARAM)NULL);
	return idle_work;
}


static void OnSize(HWND hWnd, UINT nState, int nWidth, int nHeight)
{
	static BOOL firstTime = true;

	if (nState != SIZE_MAXIMIZED && nState != SIZE_RESTORED)
		return;

	ResizeWindow(hWnd, &main_resize);
	ResizeProgressBar();
	if (firstTime == false)
	{
		OnSizeSplitter(hMain);
		m_resized = true;
	}
	//firstTime = false;
	/* Update the splitters structures as appropriate */
	RecalcSplitters();
	if (firstTime == false)
		ResizePickerControls(hMain);
	firstTime = false;
	UpdateScreenShot();
	printf("OnSize: Finished\n");fflush(stdout);
}


static HWND GetResizeItemWindow(HWND hParent, const ResizeItem *ri)
{
	HWND hControl;
	if (ri->type == RA_ID)
		hControl = GetDlgItem(hParent, ri->u.id);
	else
		hControl = ri->u.hwnd;
	return hControl;
}


static void SetAllWindowsFont(HWND hParent, const Resize *r, HFONT hTheFont, BOOL bRedraw)
{
	for (int i = 0; r->items[i].type != RA_END; i++)
	{
		HWND hControl = GetResizeItemWindow(hParent, &r->items[i]);
		if (r->items[i].setfont)
			SetWindowFont(hControl, hTheFont, bRedraw);

		/* Take care of subcontrols, if appropriate */
		if (r->items[i].subwindow != NULL)
			SetAllWindowsFont(hControl, (const Resize*)r->items[i].subwindow, hTheFont, bRedraw);
	}
}


static void ResizeWindow(HWND hParent, Resize *r)
{
	if (hParent == NULL)
		return;

	RECT parent_rect, rect;

	/* Calculate change in width and height of parent window */
	GetClientRect(hParent, &parent_rect);
	int dy = parent_rect.bottom - r->rect.bottom;
	int dx = parent_rect.right - r->rect.right;
	POINT p = {0, 0};
	ClientToScreen(hParent, &p);

	HWND hControl = 0;
	const ResizeItem *ri;
	int cmkindex = 0;
	while (r->items[cmkindex].type != RA_END)
	{
		ri = &r->items[cmkindex];
		if (ri->type == RA_ID)
			hControl = GetDlgItem(hParent, ri->u.id);
		else
			hControl = ri->u.hwnd;

		if (hControl == NULL)
		{
			cmkindex++;
			continue;
		}

		/* Get control's rectangle relative to parent */
		GetWindowRect(hControl, &rect);
		OffsetRect(&rect, -p.x, -p.y);
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;

		if (!(ri->action & RA_LEFT))
			rect.left += dx;

		if (!(ri->action & RA_TOP))
			rect.top += dy;

		if (ri->action & RA_RIGHT)
			rect.right += dx;

		if (ri->action & RA_BOTTOM)
			rect.bottom += dy;

		//Sanity Check the child rect
		if (parent_rect.top > rect.top)
			rect.top = parent_rect.top;

		if (parent_rect.left > rect.left)
			rect.left = parent_rect.left;

		if (parent_rect.bottom < rect.bottom)
		{
			rect.bottom = parent_rect.bottom;
			//ensure we have at least a minimal height
			rect.top = rect.bottom - height;
			if (rect.top < parent_rect.top)
				rect.top = parent_rect.top;
		}

		if (parent_rect.right < rect.right)
		{
			rect.right = parent_rect.right;
			//ensure we have at least a minimal width
			rect.left = rect.right - width;
			if (rect.left < parent_rect.left)
				rect.left = parent_rect.left;
		}

		MoveWindow(hControl, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);

		/* Take care of subcontrols, if appropriate */
		if (ri->subwindow)
			ResizeWindow(hControl, (Resize*)ri->subwindow);

		cmkindex++;
	}

	/* Record parent window's new location */
	memcpy(&r->rect, &parent_rect, sizeof(RECT));
}


static void ProgressBarShow()
{
	int widths[2] = {150, -1};

	if (driver_list::total() < 100)
		progBarStep = 100 / driver_list::total();
	else
		progBarStep = (driver_list::total() / 100);

	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
	SendMessage(hProgWnd, PBM_SETRANGE, 0, (LPARAM)MAKELONG(0, driver_list::total()));
	SendMessage(hProgWnd, PBM_SETSTEP, (WPARAM)progBarStep, 0);
	SendMessage(hProgWnd, PBM_SETPOS, 0, 0);

	RECT rect;
	StatusBar_GetItemRect(hStatusBar, 1, &rect);

	MoveWindow(hProgWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);

	bProgressShown = true;
}


static void ProgressBarHide()
{
	if (hProgWnd == NULL)
		return;

	int  widths[4];
	SIZE size;
	HDC hDC = GetDC(hProgWnd);

	ShowWindow(hProgWnd, SW_HIDE);

	GetTextExtentPoint32(hDC, TEXT("MMX"), 3, &size);
	widths[3] = size.cx;
	GetTextExtentPoint32(hDC, TEXT("MMMM games"), 10, &size);
	widths[2] = size.cx;
	//Just specify 24 instead of 30, gives us sufficient space to display the message, and saves some space
	GetTextExtentPoint32(hDC, TEXT("Screen flip support is missing"), 24, &size);
	widths[1] = size.cx;

	ReleaseDC(hProgWnd, hDC);

	widths[0] = -1;
	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)1, (LPARAM)(LPINT)widths);
	RECT rect;
	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	widths[0] = (rect.right - rect.left) - (widths[1] + widths[2] + widths[3]);
	widths[1] += widths[0];
	widths[2] += widths[1];
	widths[3] = -1;

	int numParts = 4;
	SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)numParts, (LPARAM)(LPINT)widths);
	UpdateStatusBar();

	bProgressShown = false;
}


static void ResizeProgressBar()
{
	if (bProgressShown)
	{
		RECT rect;
		int  widths[2] = {150, -1};

		SendMessage(hStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)(LPINT)widths);
		StatusBar_GetItemRect(hStatusBar, 1, &rect);
		MoveWindow(hProgWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, true);
	}
	else
	{
		ProgressBarHide();
	}
}


static void ProgressBarStepParam(int iGameIndex, int nGameCount)
{
	SetStatusBarTextF(0, "Game search %d%% complete",((iGameIndex + 1) * 100) / nGameCount);
	if (iGameIndex == 0)
		ShowWindow(hProgWnd, SW_SHOW);
	SendMessage(hProgWnd, PBM_STEPIT, 0, 0);
}


static void ProgressBarStep()
{
	ProgressBarStepParam(game_index, driver_list::total());
}


static HWND InitProgressBar(HWND hParent)
{
	RECT rect;

	StatusBar_GetItemRect(hStatusBar, 0, &rect);

	rect.left += 150;

	return CreateWindowEx(WS_EX_STATICEDGE,
			PROGRESS_CLASS,
			TEXT("Progress Bar"),
			WS_CHILD | PBS_SMOOTH,
			rect.left,
			rect.top,
			rect.right - rect.left,
			rect.bottom - rect.top,
			hParent,
			NULL,
			hInst,
			NULL);
}


static void CopyToolTipText(LPTOOLTIPTEXT lpttt)
{
	int iButton = lpttt->hdr.idFrom;
	BOOL bConverted = false;

	/* Map command ID to string index */
	for (int i = 0; CommandToString[i] != -1; i++)
	{
		if (CommandToString[i] == iButton)
		{
			iButton = i;
			bConverted = true;
			break;
		}
	}

	static TCHAR String[1024];
	if( bConverted )
	{
		/* Check for valid parameter */
		if(iButton > NUM_TOOLTIPS)
		{
			_tcscpy(String,TEXT("Invalid Button Index"));
		}
		else
		{
			_tcscpy(String,szTbStrings[iButton]);
		}
	}
	else
	if ( iButton <= 2 )
	{
		//Statusbar
		SendMessage(lpttt->hdr.hwndFrom, TTM_SETMAXTIPWIDTH, 0, 200);
		if( iButton != 1)
			SendMessage(hStatusBar, SB_GETTEXT, (WPARAM)iButton, (LPARAM)&String );
		else {
			//for first pane we get the Status directly, to get the line breaks
			TCHAR* t_gameinfostatus = ui_wstring_from_utf8( GameInfoStatus(Picker_GetSelectedItem(hwndList), false));
			if( !t_gameinfostatus )
				return;
			_tcscpy(String, t_gameinfostatus);
			free(t_gameinfostatus);
		}
	}
	else
		_tcscpy(String,TEXT("Invalid Button Index"));

	lpttt->lpszText = String;
}


static HWND InitToolbar(HWND hParent)
{
	HWND hToolBar = CreateToolbarEx(hParent,
						WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS |
						CCS_TOP | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
						1,
						8,
						hInst,
						IDB_TOOLBAR,
						tbb,
						NUM_TOOLBUTTONS,
						16,
						16,
						0,
						0,
						sizeof(TBBUTTON));
	RECT rect;

	// get Edit Control position
	int idx = SendMessage(hToolBar, TB_BUTTONCOUNT, (WPARAM)0, (LPARAM)0) - 1;
	SendMessage(hToolBar, TB_GETITEMRECT, (WPARAM)idx, (LPARAM)&rect);
	int iPosX = rect.right + 10;
	int iPosY = rect.top + 1;
	int iHeight = rect.bottom - rect.top - 2;

	// create Edit Control
	win_create_window_ex_utf8( 0L, "Edit", SEARCH_PROMPT, WS_CHILD | WS_BORDER | WS_VISIBLE | ES_LEFT,
					iPosX, iPosY, 200, iHeight, hToolBar, (HMENU)ID_TOOLBAR_EDIT, hInst, NULL );

	return hToolBar;
}


static HWND InitStatusBar(HWND hParent)
{
	HMENU hMenu = GetMenu(hParent);

	popstr[0].hMenu    = 0;
	popstr[0].uiString = 0;
	popstr[1].hMenu    = hMenu;
	popstr[1].uiString = IDS_UI_FILE;
	popstr[2].hMenu    = GetSubMenu(hMenu, 1);
	popstr[2].uiString = IDS_VIEW_TOOLBAR;
	popstr[3].hMenu    = 0;
	popstr[3].uiString = 0;

	return CreateStatusWindow(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | CCS_BOTTOM | SBARS_SIZEGRIP | SBT_TOOLTIPS, TEXT("Ready"), hParent, 2);
}


static LRESULT Statusbar_MenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	UINT  fuFlags = (UINT)HIWORD(wParam);
	HMENU hMainMenu = NULL;
	int iMenu = 1;

	/* Handle non-system popup menu descriptions. */
	if (  (fuFlags & MF_POPUP) && (!(fuFlags & MF_SYSMENU)))
	{
		for (iMenu = 1; iMenu < MAX_MENUS; iMenu++)
		{
			if ((HMENU)lParam == popstr[iMenu].hMenu)
			{
				hMainMenu = (HMENU)lParam;
				break ;
			}
		}
	}

	if (hMainMenu)
	{
		/* Display helpful text in status bar */
		MenuHelp(WM_MENUSELECT, wParam, lParam, hMainMenu, hInst, hStatusBar, (UINT *)&popstr[iMenu]);
	}
	else
	{
		UINT nZero = 0;
		MenuHelp(WM_MENUSELECT, wParam, lParam, NULL, hInst, hStatusBar, &nZero);
	}

	return 0;
}


static void UpdateStatusBar()
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	if (!lpFolder)
		return;

	int games_shown = 0;
	int i = -1;

	while (1)
	{
		i = FindGame(lpFolder,i+1);
		if (i == -1)
			break;

		if (!GameFiltered(i, lpFolder->m_dwFlags))
			games_shown++;
	}

	/* Show number of games in the current 'View' in the status bar */
	SetStatusBarTextF(2, g_szGameCountString, games_shown);

	i = Picker_GetSelectedItem(hwndList);

	if (games_shown == 0)
		DisableSelection();
	else
	{
		const char* pStatus = GameInfoStatus(i, false);
		SetStatusBarText(1, pStatus);
	}
}


static void UpdateHistory(string software)
{
	//DWORD dwStyle = GetWindowLong(GetDlgItem(hMain, IDC_HISTORY), GWL_STYLE);
	have_history = false;

	if (GetSelectedPick() >= 0)
	{
		char *histText = GetGameHistory(Picker_GetSelectedItem(hwndList), software);

		have_history = (histText && histText[0]) ? true : false;
		win_set_window_text_utf8(GetDlgItem(hMain, IDC_HISTORY), histText);
	}

	if (have_history && BIT(GetWindowPanes(), 3)
		&& ((TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
			(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ||
			(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ))
	{
		RECT rect;
		Edit_GetRect(GetDlgItem(hMain, IDC_HISTORY),&rect);
		int nLines = Edit_GetLineCount(GetDlgItem(hMain, IDC_HISTORY) );
		HDC hDC = GetDC(GetDlgItem(hMain, IDC_HISTORY));
		TEXTMETRIC tm;
		GetTextMetrics (hDC, &tm);
		int nLineHeight = tm.tmHeight - tm.tmInternalLeading;
		if( ( (rect.bottom - rect.top) / nLineHeight) < (nLines) )
		{
			//more than one Page, so show Scrollbar
			SetScrollRange(GetDlgItem(hMain, IDC_HISTORY), SB_VERT, 0, nLines, true);
		}
		else
		{
			//hide Scrollbar
			SetScrollRange(GetDlgItem(hMain, IDC_HISTORY),SB_VERT, 0, 0, true);

		}
		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_SHOW);
	}
	else
		ShowWindow(GetDlgItem(hMain, IDC_HISTORY), SW_HIDE);
}


static void DisableSelection()
{
	MENUITEMINFO mmi;
	HMENU hMenu = GetMenu(hMain);
	BOOL prev_have_selection = have_selection;

	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = (TCHAR *) TEXT("&Play");
	mmi.cch = _tcslen(mmi.dwTypeData);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, false, &mmi);

	EnableMenuItem(hMenu, ID_FILE_PLAY, MF_GRAYED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
	EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MESS_OPEN_SOFTWARE, MF_GRAYED);

	SetStatusBarText(0, "No Selection");
	SetStatusBarText(1, "");
	SetStatusBarText(3, "");

	have_selection = false;

	if (prev_have_selection != have_selection)
		UpdateScreenShot();
}


static void EnableSelection(int nGame)
{
	printf("EnableSelection: A = %d = %s\n",nGame,driver_list::driver(nGame).name);fflush(stdout);
	BOOL has_software = MyFillSoftwareList(nGame, false); // messui.cpp
	//printf("EnableSelection: B\n");fflush(stdout);

	TCHAR* t_description = ui_wstring_from_utf8(ConvertAmpersandString(ModifyThe(driver_list::driver(nGame).type.fullname())));
	if( !t_description )
		return;

	//printf("EnableSelection: C\n");fflush(stdout);
	TCHAR buf[200];
	_sntprintf(buf, sizeof(buf) / sizeof(buf[0]), g_szPlayGameString, t_description);
	MENUITEMINFO mmi;
	mmi.cbSize = sizeof(mmi);
	mmi.fMask = MIIM_TYPE;
	mmi.fType = MFT_STRING;
	mmi.dwTypeData = buf;
	mmi.cch = _tcslen(mmi.dwTypeData);
	HMENU hMenu = GetMenu(hMain);
	SetMenuItemInfo(hMenu, ID_FILE_PLAY, false, &mmi);

	//printf("EnableSelection: D\n");fflush(stdout);
	const char * pText;
	pText = ModifyThe(driver_list::driver(nGame).type.fullname());
	SetStatusBarText(0, pText);
	/* Add this game's status to the status bar */
	pText = GameInfoStatus(nGame, false);
	SetStatusBarText(1, pText);

	// Show number of software_list items in box at bottom right.
	//printf("EnableSelection: E\n");fflush(stdout);
	int items = SoftwareList_GetNumberOfItems();
	if (items)
	{
		sprintf((char *)pText, "%d", items);
		SetStatusBarText(3, pText);
	}
	else
		SetStatusBarText(3, "");

	/* If doing updating game status */

	//printf("EnableSelection: F\n");fflush(stdout);
	EnableMenuItem(hMenu, ID_FILE_PLAY, MF_ENABLED);
	EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_ENABLED);
	if (has_software)
		EnableMenuItem(hMenu, ID_MESS_OPEN_SOFTWARE, MF_ENABLED);
	else
		EnableMenuItem(hMenu, ID_MESS_OPEN_SOFTWARE, MF_GRAYED);

	EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_ENABLED);

	printf("EnableSelection: G = %d = %d\n",bListReady ? 1:0,nGame);
	if (bListReady)
		SetDefaultGame(nGame);

	have_selection = true;

	//printf("EnableSelection: H\n");fflush(stdout);
	ShowHideSoftwareArea();   // messui.cpp
	UpdateScreenShot();

	printf("EnableSelection: Finished\n");fflush(stdout);
	free(t_description);
}


static void PaintBackgroundImage(HWND hWnd, HRGN hRgn, int x, int y)
{
	RECT rcClient;
	HDC hDC = GetDC(hWnd);

	/* x and y are offsets within the background image that should be at 0,0 in hWnd */

	/* So we don't paint over the control's border */
	GetClientRect(hWnd, &rcClient);

	HDC htempDC = CreateCompatibleDC(hDC);
	HBITMAP oldBitmap = (HBITMAP)SelectObject(htempDC, hBackground);

	if (hRgn == NULL)
	{
		/* create a region of our client area */
		HRGN rgnBitmap = CreateRectRgnIndirect(&rcClient);
		SelectClipRgn(hDC, rgnBitmap);
		DeleteBitmap(rgnBitmap);
	}
	else
	{
		/* use the passed in region */
		SelectClipRgn(hDC, hRgn);
	}

	HPALETTE hPAL = GetBackgroundPalette();
	if (hPAL == NULL)
		hPAL = CreateHalftonePalette(hDC);

	if (GetDeviceCaps(htempDC, RASTERCAPS) & RC_PALETTE && hPAL != NULL)
	{
		SelectPalette(htempDC, hPAL, false);
		RealizePalette(htempDC);
	}

	for (int i = rcClient.left-x; i < rcClient.right; i += bmDesc.bmWidth)
		for (int j = rcClient.top-y; j < rcClient.bottom; j += bmDesc.bmHeight)
			BitBlt(hDC, i, j, bmDesc.bmWidth, bmDesc.bmHeight, htempDC, 0, 0, SRCCOPY);

	SelectObject(htempDC, oldBitmap);
	DeleteDC(htempDC);

	if (GetBackgroundPalette() == NULL)
	{
		DeletePalette(hPAL);
		hPAL = NULL;
	}

	ReleaseDC(hWnd, hDC);
}


static LPCSTR GetCloneParentName(int nItem)
{
	int nParentIndex = -1;

	if (DriverIsClone(nItem) == true)
	{
		nParentIndex = GetParentIndex(&driver_list::driver(nItem));
		if( nParentIndex >= 0)
			return ModifyThe(driver_list::driver(nParentIndex).type.fullname());
	}
	return "";
}


static BOOL TreeViewNotify(LPNMHDR nm)
{
	switch (nm->code)
	{
		case TVN_SELCHANGED :
		{
			HTREEITEM hti = TreeView_GetSelection(hTreeView);
			TVITEM tvi;

			tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
			tvi.hItem = hti;

			if (TreeView_GetItem(hTreeView, &tvi))
			{
				SetCurrentFolder((LPTREEFOLDER)tvi.lParam);
				if (bListReady)
				{
					ResetListView();
					MessUpdateSoftwareList(); // messui.cpp
					UpdateScreenShot();
				}
			}
			return true;
		}
		case TVN_BEGINLABELEDIT :
		{
			TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
			LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;

			if (folder->m_dwFlags & F_CUSTOM)
			{
				// user can edit custom folder names
				g_in_treeview_edit = true;
				return false;
			}
			// user can't edit built in folder names
			return true;
		}
		case TVN_ENDLABELEDIT :
		{
			TV_DISPINFO *ptvdi = (TV_DISPINFO *)nm;
			LPTREEFOLDER folder = (LPTREEFOLDER)ptvdi->item.lParam;
			char* utf8_szText;
			BOOL result = 0;
			g_in_treeview_edit = false;

			if (ptvdi->item.pszText == NULL || _tcslen(ptvdi->item.pszText) == 0)
				return false;

			utf8_szText = ui_utf8_from_wstring(ptvdi->item.pszText);
			if( !utf8_szText )
				return false;

			result = TryRenameCustomFolder(folder, utf8_szText);

			free(utf8_szText);

			return result;
		}
	}
	return false;
}


static void GamePicker_OnHeaderContextMenu(POINT pt, int nColumn)
{
	// Right button was clicked on header

	HMENU hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_HEADER));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);
	lastColumnClick = nColumn;
	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,NULL);

	DestroyMenu(hMenuLoad);
}


char* ConvertAmpersandString(const char *s)
{
	/* takes a string and changes any ampersands to double ampersands,
       for setting text of window controls that don't allow us to disable
       the ampersand underlining.
     */
	/* returns a static buffer--use before calling again */

	static char buf[256];
	char *ptr;

	ptr = buf;
	while (*s)
	{
		if (*s == '&')
			*ptr++ = *s;
		*ptr++ = *s++;
	}
	*ptr = 0;

	return buf;
}


static int GUI_seq_pressed(const input_seq *seq)
{
	int codenum = 0;
	int res = 1;
	int invert = 0;
	int count = 0;

	for (codenum = 0; (*seq)[codenum] != input_seq::end_code; codenum++)
	{
		input_code code = (*seq)[codenum];

		if (code == input_seq::not_code)
			invert = !invert;

		else if (code == input_seq::or_code)
		{
			if (res && count)
				return 1;
			res = 1;
			count = 0;
		}
		else
		{
			if (res)
			{
				if ((keyboard_state[(int)(code.item_id())] != 0) == invert)
					res = 0;
			}
			invert = 0;
			++count;
		}
	}
	return res && count;
}


static void check_for_GUI_action()
{
	for (int i = 0; i < NUM_GUI_SEQUENCES; i++)
	{
		const input_seq *is = &(GUISequenceControl[i].is);

		if (GUI_seq_pressed(is))
		{
			printf("seq =%s pressed\n", GUISequenceControl[i].name);fflush(stdout);
			switch (GUISequenceControl[i].func_id)
			{
			case ID_GAME_AUDIT:
			case ID_GAME_PROPERTIES:
			case ID_CONTEXT_FILTERS:
			case ID_UI_START:
				KeyboardStateClear(); /* because we won't receive KeyUp mesage when we lose focus */
				break;
			default:
				break;
			}
			SendMessage(hMain, WM_COMMAND, GUISequenceControl[i].func_id, 0);
		}
	}
}


static void KeyboardStateClear()
{
	memset(keyboard_state, 0, sizeof(keyboard_state));
	printf("keyboard gui state cleared.\n");fflush(stdout);
}


static void KeyboardKeyDown(int syskey, int vk_code, int special)
{
	int i = 0, found = 0;
	int icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = 1;

		/* a hack for right shift - it's better to use Direct X for keyboard input it seems......*/
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) /* on my keyboard left shift scancode is 0x2a, right shift is 0x36 */
				special_code = 1;

		if (special_code) /* right hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_RALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_RCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_RSHIFT.item_id());
				break;
			}
		}
		else /* left hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_LALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_LCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_LSHIFT.item_id());
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < std::size(win_key_trans_table); i++)
		{
			if ( vk_code == win_key_trans_table[i][VIRTUAL_KEY])
			{
				icode = win_key_trans_table[i][MAME_KEY];
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		printf("VK_code pressed not found =  %i\n",vk_code);fflush(stdout);
		//MessageBox(NULL,"keydown message arrived not processed","TitleBox",MB_OK);
		return;
	}
	printf("VK_code pressed found =  %i, sys=%i, mame_keycode=%i special=%08x\n", vk_code, syskey, icode, special);fflush(stdout);
	keyboard_state[icode] = true;
	check_for_GUI_action();
}


static void KeyboardKeyUp(int syskey, int vk_code, int special)
{
	int i, found = 0;
	int icode = 0;
	int special_code = (special >> 24) & 1;
	int scancode = (special>>16) & 0xff;

	if ((vk_code==VK_MENU) || (vk_code==VK_CONTROL) || (vk_code==VK_SHIFT))
	{
		found = 1;

		/* a hack for right shift - it's better to use Direct X for keyboard input it seems......*/
		if (vk_code==VK_SHIFT)
			if (scancode>0x30) /* on my keyboard left shift scancode is 0x2a, right shift is 0x36 */
				special_code = 1;

		if (special_code) /* right hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_RALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_RCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_RSHIFT.item_id());
				break;
			}
		}
		else /* left hand keys */
		{
			switch(vk_code)
			{
			case VK_MENU:
				icode = (int)(KEYCODE_LALT.item_id());
				break;
			case VK_CONTROL:
				icode = (int)(KEYCODE_LCONTROL.item_id());
				break;
			case VK_SHIFT:
				icode = (int)(KEYCODE_LSHIFT.item_id());
				break;
			}
		}
	}
	else
	{
		for (i = 0; i < std::size(win_key_trans_table); i++)
		{
			if (vk_code == win_key_trans_table[i][VIRTUAL_KEY])
			{
				icode = win_key_trans_table[i][MAME_KEY];
				found = 1;
				break;
			}
		}
	}
	if (!found)
	{
		printf("VK_code released not found =  %i\n",vk_code);fflush(stdout);
		//MessageBox(NULL,"keyup message arrived not processed","TitleBox",MB_OK);
		return;
	}
	keyboard_state[icode] = false;
	printf("VK_code released found=  %i, sys=%i, mame_keycode=%i special=%08x\n", vk_code, syskey, icode, special );fflush(stdout);
	check_for_GUI_action();
}


static void PollGUIJoystick()
{
	if (in_emulation)
		return;

	if (g_pJoyGUI == NULL)
		return;

	g_pJoyGUI->poll_joysticks();

	// User pressed UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyUp(0), GetUIJoyUp(1), GetUIJoyUp(2), GetUIJoyUp(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_UP, 0);

	// User pressed DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyDown(0), GetUIJoyDown(1), GetUIJoyDown(2), GetUIJoyDown(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_DOWN, 0);

	// User pressed LEFT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyLeft(0), GetUIJoyLeft(1), GetUIJoyLeft(2), GetUIJoyLeft(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_LEFT, 0);

	// User pressed RIGHT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyRight(0), GetUIJoyRight(1), GetUIJoyRight(2), GetUIJoyRight(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_RIGHT, 0);

	// User pressed START GAME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyStart(0), GetUIJoyStart(1), GetUIJoyStart(2), GetUIJoyStart(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_START, 0);

	// User pressed PAGE UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageUp(0), GetUIJoyPageUp(1), GetUIJoyPageUp(2), GetUIJoyPageUp(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_PGUP, 0);

	// User pressed PAGE DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyPageDown(0), GetUIJoyPageDown(1), GetUIJoyPageDown(2), GetUIJoyPageDown(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_PGDOWN, 0);

	// User pressed HOME
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHome(0), GetUIJoyHome(1), GetUIJoyHome(2), GetUIJoyHome(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_HOME, 0);

	// User pressed END
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyEnd(0), GetUIJoyEnd(1), GetUIJoyEnd(2), GetUIJoyEnd(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_END, 0);

	// User pressed CHANGE SCREENSHOT
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoySSChange(0), GetUIJoySSChange(1), GetUIJoySSChange(2), GetUIJoySSChange(3))))
		SendMessage(hMain, WM_COMMAND, IDC_SSFRAME, 0);

	// User pressed SCROLL HISTORY UP
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryUp(0), GetUIJoyHistoryUp(1), GetUIJoyHistoryUp(2), GetUIJoyHistoryUp(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_UP, 0);

	// User pressed SCROLL HISTORY DOWN
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyHistoryDown(0), GetUIJoyHistoryDown(1), GetUIJoyHistoryDown(2), GetUIJoyHistoryDown(3))))
		SendMessage(hMain, WM_COMMAND, ID_UI_HISTORY_DOWN, 0);

	// For the exec timer, will keep track of how long the button has been pressed
	static int exec_counter = 0;

	// User pressed EXECUTE COMMANDLINE
	// Note: this option is not documented, nor supported in the GUI.
	if (g_pJoyGUI->is_joy_pressed(JOYCODE(GetUIJoyExec(0), GetUIJoyExec(1), GetUIJoyExec(2), GetUIJoyExec(3))))
	{
		// validate
		int execwait = GetExecWait();
		if (execwait < 1)
			return;
		if (++exec_counter >= execwait) // Button has been pressed > exec timeout
		{
			// validate
			string exec_command = GetExecCommand();
			if (exec_command.empty())
				return;
			TCHAR *t_exec_command = ui_wstring_from_utf8(exec_command.c_str());
			if( !t_exec_command )
				return;

			// Reset counter
			exec_counter = 0;

			STARTUPINFO si;
			ZeroMemory( &si, sizeof(si) );
			si.dwFlags = STARTF_FORCEONFEEDBACK;
			si.cb = sizeof(si);

			PROCESS_INFORMATION pi;
			ZeroMemory( &pi, sizeof(pi) );

			CreateProcess(NULL, t_exec_command, NULL, NULL, false, 0, NULL, NULL, &si, &pi);

			free(t_exec_command);

			// We will not wait for the process to finish cause it might be a background task
			// The process won't get closed when MAME32 closes either.

			// But close the handles cause we won't need them anymore. Will not close process.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}
	else
	{
		// Button has been released within the timeout period, reset the counter
		exec_counter = 0;
	}
}


static void SetView(int menu_id)
{
	// first uncheck previous menu item, check new one
	CheckMenuRadioItem(GetMenu(hMain), ID_VIEW_LARGE_ICON, ID_VIEW_SMALL_ICON, menu_id, MF_CHECKED);
	ToolBar_CheckButton(s_hToolBar, menu_id, MF_CHECKED);

	// Associate the image lists with the list view control.
	if (menu_id == ID_VIEW_LARGE_ICON)
		(void)ListView_SetImageList(hwndList, hLarge, LVSIL_SMALL);
	else
		(void)ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);

	for (int i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SetViewID(GetDlgItem(hMain, s_nPickers[i]), menu_id - ID_VIEW_LARGE_ICON);

	for (int i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_Sort(GetDlgItem(hMain, s_nPickers[i]));
}


static void ResetListView()
{
	LPTREEFOLDER lpFolder = GetCurrentFolder();
	if (!lpFolder)
		return;

	int i = -1;
	LV_ITEM lvi;
	BOOL no_selection = false;

	/* If the last folder was empty, no_selection is true */
	if (have_selection == false)
		no_selection = true;

	int drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0)
		drvindex = 0;

	SetWindowRedraw(hwndList,false);

	BOOL res = ListView_DeleteAllItems(hwndList);
	res++;

	// hint to have it allocate it all at once
	ListView_SetItemCount(hwndList,driver_list::total());

	lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_INDENT;
	lvi.stateMask = 0;

	do
	{
		/* Add the games that are in this folder */
		if ((i = FindGame(lpFolder, i + 1)) != -1)
		{
			if (GameFiltered(i, lpFolder->m_dwFlags))
				continue;

			lvi.iItem    = i;
			lvi.iSubItem = 0;
			lvi.lParam   = i;
			lvi.pszText  = LPSTR_TEXTCALLBACK;
			lvi.iImage   = I_IMAGECALLBACK;
			lvi.iIndent = 0;

			if (GetEnableIndent())
				if ((GetParentFound(i) || !GetOffsetClones()) && DriverIsClone(i))
					lvi.iIndent = 1;

			res = ListView_InsertItem(hwndList, &lvi);
		}
	} while (i != -1);

	Picker_Sort(hwndList);

	if (bListReady)
	{
		/* If last folder was empty, select the first item in this folder */
		if (no_selection)
			Picker_SetSelectedPick(hwndList, 0);
		else
			Picker_SetSelectedItem(hwndList, drvindex);
	}

	/*RS Instead of the Arrange Call that was here previously on all Views
         We now need to set the ViewMode for SmallIcon again,
         for an explanation why, see SetView*/
	if (GetViewMode() == VIEW_SMALL_ICONS)
		SetView(ID_VIEW_SMALL_ICON);

	SetWindowRedraw(hwndList, true);

	UpdateStatusBar();
}


static void UpdateGameList(BOOL bUpdateRomAudit, BOOL bUpdateSampleAudit)
{
	for (int i = 0; i < driver_list::total(); i++)
	{
		if (bUpdateRomAudit && DriverUsesRoms(i))
			SetRomAuditResults(i, UNKNOWN);
		if (bUpdateSampleAudit && DriverUsesSamples(i))
			SetSampleAuditResults(i, UNKNOWN);
	}

	idle_work = true;
	bDoGameCheck = true;
	game_index = 0;

	ReloadIcons();

	// Let REFRESH also load new background if found
	LoadBackgroundBitmap();
	InvalidateRect(hMain,NULL,true);
	Picker_ResetIdle(hwndList);
}

static void UpdateCache()
{
	int current_id = GetCurrentFolderID(); // remember selected folder
	SetWindowRedraw(hwndList, false);   // stop screen updating
	ForceRebuild();          // tell system that cache needs redoing
	OptionsInit();      // reload options and fix game cache
	emu_opts_init(1);
	//extern const FOLDERDATA g_folderData[];
	//extern const FILTER_ITEM g_filterList[];
	//InitTree(g_folderData, g_filterList);         // redo folders... This crashes, leave out for now
	ResetTreeViewFolders();                      // something with folders
	SelectTreeViewFolder(current_id);            // select previous folder
	SetWindowRedraw(hwndList, true);             // refresh screen
}

UINT_PTR CALLBACK CFHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
// handle to dialog box, message identifier, message parameter, message parameter
{
	int iIndex = 0, i = 0;
	COLORREF cCombo=0, cList=0;
	switch (uiMsg)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hdlg, cmb4, CB_ADDSTRING, 0, (LPARAM)TEXT("Custom"));
			iIndex = SendDlgItemMessage(hdlg, cmb4, CB_GETCOUNT, 0, 0);
			cList = GetListFontColor();
			SendDlgItemMessage(hdlg, cmb4, CB_SETITEMDATA,(WPARAM)iIndex-1,(LPARAM)cList );
			for( i = 0; i< iIndex; i++)
			{
				cCombo = SendDlgItemMessage(hdlg, cmb4, CB_GETITEMDATA,(WPARAM)i,0 );
				if( cList == cCombo)
				{
					SendDlgItemMessage(hdlg, cmb4, CB_SETCURSEL,(WPARAM)i,0 );
					break;
				}
			}
			break;
		case WM_COMMAND:
			if( LOWORD(wParam) == cmb4)
			{
				switch (HIWORD(wParam))
				{
					case CBN_SELCHANGE:  // The color ComboBox changed selection
						iIndex = (int)SendDlgItemMessage(hdlg, cmb4, CB_GETCURSEL, 0, 0L);
						if( iIndex == SendDlgItemMessage(hdlg, cmb4, CB_GETCOUNT, 0, 0)-1)
						{
							//Custom color selected
							cList = GetListFontColor();
							PickColor(&cList);
							SendDlgItemMessage(hdlg, cmb4, CB_DELETESTRING, iIndex, 0);
							SendDlgItemMessage(hdlg, cmb4, CB_ADDSTRING, 0, (LPARAM)TEXT("Custom"));
							SendDlgItemMessage(hdlg, cmb4, CB_SETITEMDATA,(WPARAM)iIndex,(LPARAM)cList);
							SendDlgItemMessage(hdlg, cmb4, CB_SETCURSEL,(WPARAM)iIndex,0 );
							return true;
						}
				}
			}
			break;
	}
	return false;
}


static void PickFont()
{
	LOGFONT font;
	CHOOSEFONT cf;
	TCHAR szClass[128];
	HWND hWnd;
	HRESULT res = 0;
	BOOL b_res = 0;

	GetListFont(&font);
	font.lfQuality = DEFAULT_QUALITY;

	cf.lStructSize = sizeof(CHOOSEFONT);
	cf.hwndOwner   = hMain;
	cf.lpLogFont   = &font;
	cf.lpfnHook = &CFHookProc;
	cf.rgbColors   = GetListFontColor();
	cf.Flags  = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT | CF_EFFECTS | CF_ENABLEHOOK;
	if (!ChooseFont(&cf))
		return;

	SetListFont(&font);
	if (hFont != NULL)
		DeleteFont(hFont);
	hFont = CreateFontIndirect(&font);
	if (hFont != NULL)
	{
		COLORREF textColor = cf.rgbColors;

		if (textColor == RGB(255,255,255))
			textColor = RGB(240, 240, 240);

		SetAllWindowsFont(hMain, &main_resize, hFont, true);

		hWnd = GetWindow(hMain, GW_CHILD);
		while(hWnd)
		{
			if (GetClassName(hWnd, szClass, sizeof(szClass) / sizeof(szClass[0])))
			{
				if (!_tcscmp(szClass, TEXT("SysListView32")))
					b_res = ListView_SetTextColor(hWnd, textColor);
				else
				if (!_tcscmp(szClass, TEXT("SysTreeView32")))
					res = TreeView_SetTextColor(hTreeView, textColor);
			}
			hWnd = GetWindow(hWnd, GW_HWNDNEXT);
		}
		SetListFontColor(cf.rgbColors);
		ResetListView();
	}
	res++;
	b_res++;
}


static void PickColor(COLORREF *cDefault)
{
	CHOOSECOLOR cc;
	COLORREF choice_colors[16];
	int i = 0;

	for (i=0;i<16;i++)
		choice_colors[i] = GetCustomColor(i);

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner   = hMain;
	cc.rgbResult   = *cDefault;
	cc.lpCustColors = choice_colors;
	cc.Flags       = CC_ANYCOLOR | CC_RGBINIT | CC_SOLIDCOLOR;
	if (!ChooseColor(&cc))
		return;

	for (i=0;i<16;i++)
		SetCustomColor(i,choice_colors[i]);

	*cDefault = cc.rgbResult;
}


static void PickCloneColor()
{
	COLORREF cClonecolor;
	cClonecolor = GetListCloneColor();
	PickColor( &cClonecolor);
	SetListCloneColor(cClonecolor);
	InvalidateRect(hwndList,NULL,false);
}


static BOOL MameCommand(HWND hwnd,int id, HWND hwndCtl, UINT codeNotify)
{
	int i = 0;
	LPTREEFOLDER folder;
	char* utf8_szFile;
	BOOL res = 0;
	int drvindex = Picker_GetSelectedItem(hwndList);

	switch (id)
	{
	case ID_FILE_PLAY:
		MamePlayGame();
		return true;

	case ID_FILE_PLAY_RECORD:
		MamePlayRecordGame();
		return true;

	case ID_FILE_PLAY_BACK:
		MamePlayBackGame();
		return true;

	case ID_FILE_PLAY_RECORD_WAVE:
		MamePlayRecordWave();
		return true;

	case ID_FILE_PLAY_RECORD_MNG:
		MamePlayRecordMNG();
		return true;

	case ID_FILE_PLAY_RECORD_AVI:
		MamePlayRecordAVI();
		return true;

	case ID_FILE_LOADSTATE :
		MameLoadState();
		return true;

	case ID_FILE_AUDIT:
		AuditDialog(hMain, 1);
		ResetWhichGamesInFolders();
		ResetListView();
		SetFocus(hwndList);
		return true;

	case ID_FILE_AUDIT_X:
		AuditDialog(hMain, 2);
		ResetWhichGamesInFolders();
		ResetListView();
		SetFocus(hwndList);
		return true;

	case ID_FILE_FILTER:
		if (CommonListDialog(GetOpenFileName))
			return true;
		else
			break;

	case ID_FILE_EXIT:
		PostMessage(hMain, WM_CLOSE, 0, 0);
		return true;

	case ID_VIEW_LARGE_ICON:
		SetView(ID_VIEW_LARGE_ICON);
			UpdateListView();
		return true;

	case ID_VIEW_SMALL_ICON:
		SetView(ID_VIEW_SMALL_ICON);
			UpdateListView();
		return true;

	case ID_VIEW_INDENT:
		bEnableIndent = !bEnableIndent;
		SetEnableIndent(bEnableIndent);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);
		UpdateListView();
		break;

	/* Arrange Icons submenu */
	case ID_VIEW_BYGAME:
		SetSortReverse(false);
		SetSortColumn(COLUMN_GAMES);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYDIRECTORY:
		SetSortReverse(false);
		SetSortColumn(COLUMN_DIRECTORY);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYMANUFACTURER:
		SetSortReverse(false);
		SetSortColumn(COLUMN_MANUFACTURER);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYTIMESPLAYED:
		SetSortReverse(false);
		SetSortColumn(COLUMN_PLAYED);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYTYPE:
		SetSortReverse(false);
		SetSortColumn(COLUMN_TYPE);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_BYYEAR:
		SetSortReverse(false);
		SetSortColumn(COLUMN_YEAR);
		Picker_Sort(hwndList);
		break;

	case ID_VIEW_FOLDERS:
	{
		int val = GetWindowPanes() ^ 1;
		BOOL bShowTree = BIT(val, 0);
		SetWindowPanes(val);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_FOLDERS, (bShowTree) ? MF_CHECKED : MF_UNCHECKED);
		UpdateScreenShot();
		break;
	}

	case ID_VIEW_TOOLBARS:
		bShowToolBar = !bShowToolBar;
		SetShowToolBar(bShowToolBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ToolBar_CheckButton(s_hToolBar, ID_VIEW_TOOLBARS, (bShowToolBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(s_hToolBar, (bShowToolBar) ? SW_SHOW : SW_HIDE);
		if (bShowToolBar)
		{
			ToolBar_CheckButton(s_hToolBar, ID_VIEW_FOLDERS, BIT(GetWindowPanes(), 0) ? MF_CHECKED : MF_UNCHECKED);
			ToolBar_CheckButton(s_hToolBar, ID_VIEW_INDENT, (bEnableIndent) ? MF_CHECKED : MF_UNCHECKED);
			ToolBar_CheckButton(s_hToolBar, ID_VIEW_SOFTWARE_AREA, BIT(GetWindowPanes(), 2) ? MF_CHECKED : MF_UNCHECKED);
			ToolBar_CheckButton(s_hToolBar, ID_VIEW_PICTURE_AREA, BIT(GetWindowPanes(), 3) ? MF_CHECKED : MF_UNCHECKED);
		}
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_STATUS:
		bShowStatusBar = !bShowStatusBar;
		SetShowStatusBar(bShowStatusBar);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		//ToolBar_CheckButton(s_hToolBar, ID_VIEW_STATUS, (bShowStatusBar) ? MF_CHECKED : MF_UNCHECKED);
		ShowWindow(hStatusBar, (bShowStatusBar) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		break;

	case ID_VIEW_PAGETAB:
		bShowTabCtrl = !bShowTabCtrl;
		SetShowTabCtrl(bShowTabCtrl);
		ShowWindow(hTabCtrl, (bShowTabCtrl) ? SW_SHOW : SW_HIDE);
		ResizePickerControls(hMain);
		UpdateScreenShot();
		InvalidateRect(hMain,NULL,true);
		break;

        /*
          Switches to fullscreen mode. No check mark handling
          for this item cause in fullscreen mode the menu won't
          be visible anyways.
        */
	case ID_VIEW_FULLSCREEN:
		SwitchFullScreenMode();
		break;

	case ID_TOOLBAR_EDIT:
		{
			string buf;
			HWND hToolbarEdit;

			buf = win_get_window_text_utf8(hwndCtl);
			switch (codeNotify)
			{
			case TOOLBAR_EDIT_ACCELERATOR_PRESSED:
				hToolbarEdit = GetDlgItem( s_hToolBar, ID_TOOLBAR_EDIT);
				SetFocus(hToolbarEdit);
				break;
			case EN_CHANGE:
				//put search routine here first, add a 200ms timer later.
				if ((!_stricmp(buf.c_str(), SEARCH_PROMPT) && !_stricmp(g_SearchText, "")) ||
					(!_stricmp(g_SearchText, SEARCH_PROMPT) && !_stricmp(buf.c_str(), "")))
				{
					strcpy(g_SearchText, buf.c_str());
				}
				else
				{
					strcpy(g_SearchText, buf.c_str());
					ResetListView();
				}
				break;
			case EN_SETFOCUS:
				if (!_stricmp(buf.c_str(), SEARCH_PROMPT))
					win_set_window_text_utf8(hwndCtl, "");
				break;
			case EN_KILLFOCUS:
				if (*buf.c_str() == 0)
					win_set_window_text_utf8(hwndCtl, SEARCH_PROMPT);
				break;
			}
		}
		break;

	case ID_GAME_AUDIT:
		InitGameAudit(drvindex);
		if (drvindex >= 0)
		{
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItemIcon(), OPTIONS_GAME, -1, drvindex, AUDIT_PAGE);
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	/* ListView Context Menu */
	case ID_CONTEXT_ADD_CUSTOM:
	{
		if (drvindex >= 0)
			DialogBoxParam(GetModuleHandle(NULL),MAKEINTRESOURCE(IDD_CUSTOM_FILE), hMain, AddCustomFileDialogProc, drvindex);
		SetFocus(hwndList);
		break;
	}

	case ID_CONTEXT_REMOVE_CUSTOM:
	{
		RemoveCurrentGameCustomFolder();
		break;
	}

	/* Tree Context Menu */
	case ID_CONTEXT_FILTERS:
		if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FILTERS), hMain, FilterDialogProc) == true)
			ResetListView();
		SetFocus(hwndList);
		return true;

		// ScreenShot Context Menu
		// select current tab
	case ID_VIEW_TAB_SCREENSHOT:
	case ID_VIEW_TAB_TITLE:
	case ID_VIEW_TAB_SCORES:
	case ID_VIEW_TAB_HOWTO:
	case ID_VIEW_TAB_SELECT:
	case ID_VIEW_TAB_VERSUS:
	case ID_VIEW_TAB_BOSSES:
	case ID_VIEW_TAB_COVER:
	case ID_VIEW_TAB_ENDS:
	case ID_VIEW_TAB_GAMEOVER:
	case ID_VIEW_TAB_LOGO:
	case ID_VIEW_TAB_ARTWORK:
	case ID_VIEW_TAB_FLYER:
	case ID_VIEW_TAB_CABINET:
	case ID_VIEW_TAB_MARQUEE:
	case ID_VIEW_TAB_CONTROL_PANEL:
	case ID_VIEW_TAB_PCB:
	case ID_VIEW_TAB_HISTORY:
		if (id == ID_VIEW_TAB_HISTORY && GetShowTab(TAB_HISTORY) == false)
			break;

		TabView_SetCurrentTab(hTabCtrl, id - ID_VIEW_TAB_ARTWORK);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;

		// toggle tab's existence
	case ID_TOGGLE_TAB_SCREENSHOT:
	case ID_TOGGLE_TAB_TITLE:
	case ID_TOGGLE_TAB_SCORES:
	case ID_TOGGLE_TAB_HOWTO:
	case ID_TOGGLE_TAB_SELECT:
	case ID_TOGGLE_TAB_VERSUS:
	case ID_TOGGLE_TAB_BOSSES:
	case ID_TOGGLE_TAB_COVER:
	case ID_TOGGLE_TAB_ENDS:
	case ID_TOGGLE_TAB_GAMEOVER:
	case ID_TOGGLE_TAB_LOGO:
	case ID_TOGGLE_TAB_ARTWORK:
	case ID_TOGGLE_TAB_FLYER:
	case ID_TOGGLE_TAB_CABINET:
	case ID_TOGGLE_TAB_MARQUEE:
	case ID_TOGGLE_TAB_CONTROL_PANEL:
	case ID_TOGGLE_TAB_PCB:
	case ID_TOGGLE_TAB_HISTORY:
	{
		int toggle_flag = id - ID_TOGGLE_TAB_ARTWORK;

		if (AllowedToSetShowTab(toggle_flag,!GetShowTab(toggle_flag)) == false)
		{
			// attempt to hide the last tab
			// should show error dialog? hide picture area? or ignore?
			break;
		}

		SetShowTab(toggle_flag,!GetShowTab(toggle_flag));

		TabView_Reset(hTabCtrl);

		if (TabView_GetCurrentTab(hTabCtrl) == toggle_flag && GetShowTab(toggle_flag) == false)
		{
			// we're deleting the tab we're on, so go to the next one
			TabView_CalculateNextTab(hTabCtrl);
		}

		// Resize the controls in case we toggled to another history
		// mode (and the history control needs resizing).

		ResizePickerControls(hMain);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;
	}

	/* Header Context Menu */
	case ID_SORT_ASCENDING:
		SetSortReverse(false);
		SetSortColumn(Picker_GetRealColumnFromViewColumn(hwndList, lastColumnClick));
		Picker_Sort(hwndList);
		break;

	case ID_SORT_DESCENDING:
		SetSortReverse(true);
		SetSortColumn(Picker_GetRealColumnFromViewColumn(hwndList, lastColumnClick));
		Picker_Sort(hwndList);
		break;

	case ID_CUSTOMIZE_FIELDS:
		if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_COLUMNS), hMain, ColumnDialogProc) == true)
			ResetColumnDisplay(false);
		SetFocus(hwndList);
		return true;

	case ID_VIEW_LINEUPICONS:
		if( codeNotify == false)
			ResetListView();
		else
		{
			/*it was sent after a refresh (F5) was done, we only reset the View if "available" is the selected folder
			 as it doesn't affect the others. */
			folder = GetSelectedFolder();
			if( folder )
			{
				if (folder->m_nFolderId == FOLDER_AVAILABLE )
					ResetListView();
			}
		}
		break;

	case ID_GAME_PROPERTIES:
		if (drvindex >= 0)
			InitPropertyPageToPage(hInst, hwnd, GetSelectedPickItemIcon(), OPTIONS_GAME, -1, drvindex, PROPERTIES_PAGE);
		UpdateStatusBar();
		break;

	case ID_FOLDER_PROPERTIES:
		{
			folder = GetSelectedFolder();
			if (folder)
				if (folder->m_dwFlags & F_INIEDIT)
				{
					LPCFOLDERDATA data = FindFilter(folder->m_nFolderId);
					if (data)
						if (data->m_opttype < OPTIONS_MAX)
							InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), data->m_opttype, folder->m_nFolderId, -1);
				}
		}
		UpdateStatusBar();
		break;

	case ID_FOLDER_SOURCEPROPERTIES:
	{
		if (drvindex < 0)
			return true;
		InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), OPTIONS_SOURCE, -1, drvindex);
		UpdateStatusBar();
		SetFocus(hwndList);
		return true;
		//folder = GetFolderByName(FOLDER_SOURCE, GetDriverFilename(Picker_GetSelectedItem(hwndList)) );
		//InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), (folder->m_nFolderId == FOLDER_VECTOR) ? OPTIONS_VECTOR : OPTIONS_SOURCE , folder->m_nFolderId, Picker_GetSelectedItem(hwndList));
		//UpdateStatusBar();
		//break;
	}

	case ID_FOLDER_VECTORPROPERTIES:
		if (drvindex >= 0)
		{
			folder = GetFolderByID( FOLDER_VECTOR );
			InitPropertyPage(hInst, hwnd, GetSelectedFolderIcon(), OPTIONS_VECTOR, folder->m_nFolderId, drvindex);
		}
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_FOLDER_AUDIT:
		FolderCheck();
		/* Just in case the toggle MMX on/off */
		UpdateStatusBar();
		break;

	case ID_VIEW_PICTURE_AREA :
		ToggleScreenShot();
		break;

	case ID_VIEW_SOFTWARE_AREA :
		SetWindowPanes(GetWindowPanes() ^ 4);
		ShowHideSoftwareArea();   // messui.cpp
		break;

	case ID_UPDATE_GAMELIST:
		UpdateGameList(true, true);
		break;

	case ID_UPDATE_CACHE:
		UpdateCache();
		break;

	case ID_OPTIONS_FONT:
		PickFont();
		return true;

	case ID_OPTIONS_CLONE_COLOR:
		PickCloneColor();
		return true;

	case ID_OPTIONS_DEFAULTS:
		/* Check the return value to see if changes were applied */
		InitDefaultPropertyPage(hInst, hwnd);
		SetFocus(hwndList);
		return true;

	case ID_OPTIONS_DIR:
		{
			int nResult = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIRECTORIES), hMain, DirectoriesDialogProc);

			global_save_ini();
			mui_save_ini();
			ui_save_ini();

			BOOL bUpdateRoms    = ((nResult & DIRDLG_ROMS) == DIRDLG_ROMS) ? true : false;
			BOOL bUpdateSamples = ((nResult & DIRDLG_SAMPLES) == DIRDLG_SAMPLES) ? true : false;

#ifdef DIRWATCH
			if (s_pWatcher)
			{
				if (bUpdateRoms)
					DirWatcher_Watch(s_pWatcher, 0, dir_get_value(2), false);
				if (bUpdateSamples)
					DirWatcher_Watch(s_pWatcher, 1, dir_get_value(4), false);
				if (bUpdateSoftware)
					DirWatcher_Watch(s_pWatcher, 2, dir_get_value(13), true);
			}
#endif

			/* update game list */
			if (bUpdateRoms == true || bUpdateSamples == true)
				UpdateGameList(bUpdateRoms, bUpdateSamples);

			SetFocus(hwndList);
		}
		return true;

	case ID_OPTIONS_RESET_DEFAULTS:
		if (DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_RESET), hMain, ResetDialogProc) == true)
		{
			// these may have been changed
			global_save_ini();
			mui_save_ini();
			ui_save_ini();
			DestroyWindow(hwnd);
			PostQuitMessage(0);
		}
		else
		{
			ResetListView();
			SetFocus(hwndList);
		}
		return true;

	case ID_OPTIONS_INTERFACE:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_INTERFACE_OPTIONS), hMain, InterfaceDialogProc);
		global_save_ini();
		mui_save_ini();
		ui_save_ini();

		KillTimer(hMain, SCREENSHOT_TIMER);
		if( GetCycleScreenshot() > 0)
			SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL ); // Scale to seconds

		return true;

	case ID_VIDEO_SNAP:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				string path = GetVideoDir() + string("\\") + driver_list::driver(nGame).name + string(".mp4");
				ShellExecuteCommon(hMain, path.c_str());
			}
			SetFocus(hwndList);
		}
		break;

	case ID_MANUAL:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				string path = GetManualsDir() + string("\\") + driver_list::driver(nGame).name + string(".pdf");
				ShellExecuteCommon(hMain, path.c_str());
			}
			SetFocus(hwndList);
		}
		break;

	case ID_RC_CLEAN:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				// INI
				string t = dir_get_value(7);
				char buf7[t.size()+1];
				strcpy(buf7, t.c_str());
				char * t_dir = strtok(buf7, ";");
				while (t_dir)
				{
					string t_path = string(t_dir) + PATH_SEPARATOR + driver_list::driver(nGame).name + ".ini";
					printf("Deleting %s\n",t_path.c_str());
					remove(t_path.c_str());
					t_dir = strtok(NULL, ";");
				}
				// CFG
				t = dir_get_value(14);
				char buf14[t.size()+1];
				strcpy(buf14, t.c_str());
				t_dir = strtok(buf14, ";");
				while (t_dir)
				{
					string t_path = string(t_dir) + PATH_SEPARATOR + driver_list::driver(nGame).name + ".cfg";
					printf("Deleting %s\n",t_path.c_str());
					remove(t_path.c_str());
					t_dir = strtok(NULL, ";");
				}
				// NVRAM
				t = dir_get_value(15);
				char buf15[t.size()+1];
				strcpy(buf15, t.c_str());
				t_dir = strtok(buf15, ";");
				while (t_dir)
				{
					string t_path = string("rd /s /q ") + string(t_dir) + PATH_SEPARATOR + driver_list::driver(nGame).name;
					printf("Deleting %s\n",t_path.c_str());
					system(t_path.c_str());
					t_dir = strtok(NULL, ";");
				}
				// Save states
				t = dir_get_value(17);
				char buf17[t.size()+1];
				strcpy(buf17, t.c_str());
				t_dir = strtok(buf17, ";");
				while (t_dir)
				{
					string t_path = string("rd /s /q ") + string(t_dir) + PATH_SEPARATOR + driver_list::driver(nGame).name;
					printf("Deleting %s\n",t_path.c_str());
					system(t_path.c_str());
					t_dir = strtok(NULL, ";");
				}
			}
			SetFocus(hwndList);
		}
		break;

	case ID_NOTEPAD:
		{
			int nGame = Picker_GetSelectedItem(hwndList);
			if (nGame >= 0)
			{
				const char* filename = "history.wtx";
				string t2 = GetGameHistory(nGame);
				std::ofstream outfile (filename, std::ios::out | std::ios::trunc);
				size_t size = t2.size();
				char t1[size+1];
				strcpy(t1, t2.c_str());
				outfile.write(t1, size);
				outfile.close();
				string path = string(".\\") + filename;
				ShellExecuteCommon(hMain, path.c_str());
			}
			SetFocus(hwndList);
		}
		break;

	case ID_OPTIONS_BG:
		{
			// Get the path from the existing filename; if no filename go to root
			TCHAR* t_bgdir = TEXT(".");
			bool free_bgdir = false;
			string s = GetBgDir();
			string as = util::zippath_parent(s);
			size_t t1 = as.length()-1;
			if (as[t1] == '\\')
				as = as.substr(0, t1-1);
			t1 = as.find(':');
			if (t1 != string::npos)
			{
				t_bgdir = ui_wstring_from_utf8(as.c_str());
				free_bgdir = true;
				if( !t_bgdir )
					return false;
			}

			OPENFILENAME OFN;
			TCHAR szFile[MAX_PATH] = TEXT("\0");

			OFN.lStructSize       = sizeof(OPENFILENAME);
			OFN.hwndOwner         = hMain;
			OFN.hInstance         = 0;
			OFN.lpstrFilter       = TEXT("Image Files (*.png)\0*.PNG\0");
			OFN.lpstrCustomFilter = NULL;
			OFN.nMaxCustFilter    = 0;
			OFN.nFilterIndex      = 1;
			OFN.lpstrFile         = szFile;
			OFN.nMaxFile          = sizeof(szFile);
			OFN.lpstrFileTitle    = NULL;
			OFN.nMaxFileTitle     = 0;
			OFN.lpstrInitialDir   = t_bgdir;
			OFN.lpstrTitle        = TEXT("Select a Background Image");
			OFN.nFileOffset       = 0;
			OFN.nFileExtension    = 0;
			OFN.lpstrDefExt       = NULL;
			OFN.lCustData         = 0;
			OFN.lpfnHook          = NULL;
			OFN.lpTemplateName    = NULL;
			OFN.Flags             = OFN_NOCHANGEDIR | OFN_SHOWHELP | OFN_EXPLORER;

			BOOL res = GetOpenFileName(&OFN);
			if (res)
			{
				if (free_bgdir)
					free(t_bgdir);
				utf8_szFile = ui_utf8_from_wstring(szFile);
				if( !utf8_szFile )
					return false;

				// Make this file as the new default
				SetBgDir(utf8_szFile);

				// Display new background
				LoadBackgroundBitmap();
				InvalidateRect(hMain, NULL, true);
				free(utf8_szFile);
				return true;
			}
			if (free_bgdir)
				free(t_bgdir);
		}
		break;

	case ID_HELP_ABOUT:
		DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_ABOUT), hMain, AboutDialogProc);
		SetFocus(hwndList);
		return true;

	case IDOK :
		if (codeNotify != EN_CHANGE && codeNotify != EN_UPDATE)
		{
			/* enter key */
			if (g_in_treeview_edit)
			{
				res = TreeView_EndEditLabelNow(hTreeView, false);
				return true;
			}
			else
				if (have_selection)
					MamePlayGame();
		}
		break;

	case IDCANCEL : /* esc key */
		if (g_in_treeview_edit)
			res = TreeView_EndEditLabelNow(hTreeView, true);
		break;

	case IDC_PLAY_GAME :
		if (have_selection)
			MamePlayGame();
		break;

	case ID_UI_START:
		SetFocus(hwndList);
		MamePlayGame();
		break;

	case ID_UI_UP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
		break;

	case ID_UI_DOWN:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
		break;

	case ID_UI_PGUP:
		Picker_SetSelectedPick(hwndList, GetSelectedPick() - ListView_GetCountPerPage(hwndList));
		break;

	case ID_UI_PGDOWN:
		if ( (GetSelectedPick() + ListView_GetCountPerPage(hwndList)) < ListView_GetItemCount(hwndList) )
			Picker_SetSelectedPick(hwndList,  GetSelectedPick() + ListView_GetCountPerPage(hwndList) );
		else
			Picker_SetSelectedPick(hwndList,  ListView_GetItemCount(hwndList)-1 );
		break;

	case ID_UI_HOME:
		Picker_SetSelectedPick(hwndList, 0);
		break;

	case ID_UI_END:
		Picker_SetSelectedPick(hwndList,  ListView_GetItemCount(hwndList)-1 );
		break;

	case ID_UI_LEFT:
		SendMessage(hwndList,WM_HSCROLL, SB_LINELEFT, 0);
		break;

	case ID_UI_RIGHT:
		SendMessage(hwndList,WM_HSCROLL, SB_LINERIGHT, 0);
		break;

	case ID_UI_HISTORY_UP:
		{
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEUP, 0);
		}
		break;

	case ID_UI_HISTORY_DOWN:
		{
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);
			SendMessage(hHistory, EM_SCROLL, SB_PAGEDOWN, 0);
		}
		break;

	case IDC_SSFRAME:
		TabView_CalculateNextTab(hTabCtrl);
		UpdateScreenShot();
		TabView_UpdateSelection(hTabCtrl);
		break;

	case ID_CONTEXT_SELECT_RANDOM:
		SetRandomPickItem();
		break;

	case ID_CONTEXT_RESET_PLAYSTATS:
		if (drvindex < 0)
			break;
		ResetPlayTime(drvindex);
		ResetPlayCount(drvindex);
		res = ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		break;

	case ID_CONTEXT_RENAME_CUSTOM :
		TreeView_EditLabel(hTreeView,TreeView_GetSelection(hTreeView));
		break;

	default:
		if (id >= ID_CONTEXT_SHOW_FOLDER_START && id < ID_CONTEXT_SHOW_FOLDER_END)
		{
			ToggleShowFolder(id - ID_CONTEXT_SHOW_FOLDER_START);
			break;
		}
		for (i = 0; g_helpInfo[i].nMenuItem > 0; i++)
		{
			if (g_helpInfo[i].nMenuItem == id)
			{
				//printf("%X: %ls\n",g_helpInfo[i].bIsHtmlHelp, g_helpInfo[i].lpFile);fflush(stdout);
				if (i == 1) // get current whatsnew.txt from mamedev.org
				{
					string version = string(GetVersionString()); // turn version string into std
					version.erase(1,1); // take out the decimal point
					version.erase(4, string::npos); // take out the date
					string url = "https://mamedev.org/releases/whatsnew_" + version + ".txt"; // construct url
					std::wstring stemp = s2ws(url); // convert to wide string (yeah, typical c++ mess)
					LPCWSTR result = stemp.c_str(); // then convert to const wchar_t*
					ShellExecute(hMain, TEXT("open"), result, TEXT(""), NULL, SW_SHOWNORMAL); // show web page
				}
				else
				if (g_helpInfo[i].bIsHtmlHelp)
//					HelpFunction(hMain, g_helpInfo[i].lpFile, HH_DISPLAY_TOPIC, 0);
					ShellExecute(hMain, TEXT("open"), g_helpInfo[i].lpFile, TEXT(""), NULL, SW_SHOWNORMAL);
//				else
//					DisplayTextFile(hMain, g_helpInfo[i].lpFile);
				return false;
			}
		}
		return MessCommand(hwnd, id, hwndCtl, codeNotify); // messui.cpp: Open Other Software menu choice
	}
	res++;
	return false;
}


static void LoadBackgroundBitmap()
{
	HGLOBAL hDIBbg;

	if (hBackground)
	{
		DeleteBitmap(hBackground);
		hBackground = 0;
	}

	if (hPALbg)
	{
		DeletePalette(hPALbg);
		hPALbg = 0;
	}

	if (LoadDIBBG(&hDIBbg, &hPALbg)) // screenshot.cpp
	{
		HDC hDC = GetDC(hwndList);
		hBackground = DIBToDDB(hDC, hDIBbg, &bmDesc);
		GlobalFree(hDIBbg);
		ReleaseDC(hwndList, hDC);
	}
}


static void ResetColumnDisplay(BOOL first_time)
{
	if (!first_time)
		Picker_ResetColumnDisplay(hwndList);

	ResetListView();

	Picker_SetSelectedItem(hwndList, GetDefaultGame());
}


static int GamePicker_GetItemImage(HWND hwndPicker, int nItem)
{
	return GetIconForDriver(nItem);
}


static const TCHAR *GamePicker_GetItemString(HWND hwndPicker, int nItem, int nColumn, TCHAR *pszBuffer, UINT nBufferLength)
{
	const TCHAR *s = NULL;
	const char* utf8_s = NULL;
	char playtime_buf[256];

	switch(nColumn)
	{
		case COLUMN_GAMES:
			/* Driver description */
			utf8_s = ModifyThe(driver_list::driver(nItem).type.fullname());
			break;

		case COLUMN_ORIENTATION:
			utf8_s = DriverIsVertical(nItem) ? "Vertical" : "Horizontal";
			break;

		case COLUMN_ROMS:
			utf8_s = GetAuditString(GetRomAuditResults(nItem));
			break;

		case COLUMN_SAMPLES:
			/* Samples */
			if (DriverUsesSamples(nItem))
				utf8_s = GetAuditString(GetSampleAuditResults(nItem));
			else
				s = TEXT("-");
			break;

		case COLUMN_DIRECTORY:
			/* Driver name (directory) */
			utf8_s = driver_list::driver(nItem).name;
			break;

		case COLUMN_SRCDRIVERS:
			/* Source drivers */
			utf8_s = GetDriverFilename(nItem);
			break;

		case COLUMN_PLAYTIME:
			/* Source drivers */
			GetTextPlayTime(nItem, playtime_buf);
			utf8_s = playtime_buf;
			break;

		case COLUMN_TYPE:
			{
				machine_config config(driver_list::driver(nItem),MameUIGlobal());
				/* Vector/Raster */
				if (isDriverVector(&config))
					s = TEXT("Vector");
				else
					s = TEXT("Raster");
			}
			break;

		case COLUMN_TRACKBALL:
			/* Trackball */
			if (DriverUsesTrackball(nItem))
				s = TEXT("Yes");
			else
				s = TEXT("No");
			break;

		case COLUMN_PLAYED:
			/* times played */
			_sntprintf(pszBuffer, nBufferLength, TEXT("%i"), GetPlayCount(nItem));
			s = pszBuffer;
			break;

		case COLUMN_MANUFACTURER:
			/* Manufacturer */
			utf8_s = driver_list::driver(nItem).manufacturer;
			break;

		case COLUMN_YEAR:
			/* Year */
			utf8_s = driver_list::driver(nItem).year;
			break;

		case COLUMN_CLONE:
			utf8_s = GetCloneParentName(nItem);
			break;
	}

	if( utf8_s )
	{
		TCHAR* t_s = ui_wstring_from_utf8(utf8_s);
		if( !t_s )
			return s;

		_sntprintf(pszBuffer, nBufferLength, TEXT("%s"), t_s);
		free(t_s);

		s = pszBuffer;
	}

	return s;
}


static void GamePicker_LeavingItem(HWND hwndPicker, int nItem)
{
	// leaving item
	g_szSelectedItem.clear();
}


static void GamePicker_EnteringItem(HWND hwndPicker, int nItem)
{
	// printf("entering %s\n",driver_list::driver(nItem).name);fflush(stdout);
	EnableSelection(nItem);

	MessReadMountedSoftware(nItem); // messui.cpp

	// decide if it is valid to load a savestate
	EnableMenuItem(GetMenu(hMain), ID_FILE_LOADSTATE, BIT(GetDriverCacheLower(nItem), 7) ? MFS_ENABLED : MFS_GRAYED);
}


static int GamePicker_FindItemParent(HWND hwndPicker, int nItem)
{
	return GetParentRomSetIndex(&driver_list::driver(nItem));
}


/* Initialize the Picker and List controls */
static void InitListView()
{
	LVBKIMAGE bki;
	//TCHAR path[MAX_PATH];
	TCHAR* t_bgdir;
	BOOL res = 0;

	static const struct PickerCallbacks s_gameListCallbacks =
	{
		SetSortColumn,				/* pfnSetSortColumn */
		GetSortColumn,				/* pfnGetSortColumn */
		SetSortReverse,				/* pfnSetSortReverse */
		GetSortReverse,				/* pfnGetSortReverse */
		SetViewMode,				/* pfnSetViewMode */
		GetViewMode,				/* pfnGetViewMode */
		SetColumnWidths,			/* pfnSetColumnWidths */
		GetColumnWidths,			/* pfnGetColumnWidths */
		SetColumnOrder,				/* pfnSetColumnOrder */
		GetColumnOrder,				/* pfnGetColumnOrder */
		SetColumnShown,				/* pfnSetColumnShown */
		GetColumnShown,				/* pfnGetColumnShown */
		GetOffsetClones,			/* pfnGetOffsetChildren */

		GamePicker_Compare,			/* pfnCompare */
		MamePlayGame,				/* pfnDoubleClick */
		GamePicker_GetItemString,	/* pfnGetItemString */
		GamePicker_GetItemImage,	/* pfnGetItemImage */
		GamePicker_LeavingItem,		/* pfnLeavingItem */
		GamePicker_EnteringItem,	/* pfnEnteringItem */
		BeginListViewDrag,			/* pfnBeginListViewDrag */
		GamePicker_FindItemParent,	/* pfnFindItemParent */
		OnIdle,							/* pfnIdle */
		GamePicker_OnHeaderContextMenu,	/* pfnOnHeaderContextMenu */
		GamePicker_OnBodyContextMenu	/* pfnOnBodyContextMenu */
	};

	struct PickerOptions opts;

	// subclass the list view
	memset(&opts, 0, sizeof(opts));
	opts.pCallbacks = &s_gameListCallbacks;
	opts.nColumnCount = COLUMN_MAX;
	opts.ppszColumnNames = column_names;
	SetupPicker(hwndList, &opts);

	res = ListView_SetTextBkColor(hwndList, CLR_NONE);
	res = ListView_SetBkColor(hwndList, CLR_NONE);
	t_bgdir = ui_wstring_from_utf8(GetBgDir().c_str());
	if( !t_bgdir )
		return;

	bki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
	bki.pszImage = t_bgdir;
	if( hBackground )
		res = ListView_SetBkImage(hwndList, &bki);

	CreateIcons();

	ResetColumnDisplay(true);

	// Allow selection to change the default saved game
	bListReady = true;
	res++;
	free(t_bgdir);
}


static void AddDriverIcon(int nItem,int default_icon_index)
{
	HICON hIcon = 0;
	int nParentIndex = -1;

	/* if already set to rom or clone icon, we've been here before */
	if (icon_index[nItem] == 1 || icon_index[nItem] == 3)
		return;

	hIcon = LoadIconFromFile((char *)driver_list::driver(nItem).name);
	if (hIcon == NULL)
	{
		nParentIndex = GetParentIndex(&driver_list::driver(nItem));
		if( nParentIndex >= 0)
		{
			hIcon = LoadIconFromFile((char *)driver_list::driver(nParentIndex).name);
			nParentIndex = GetParentIndex(&driver_list::driver(nParentIndex));
			if (hIcon == NULL && nParentIndex >= 0)
				hIcon = LoadIconFromFile((char *)driver_list::driver(nParentIndex).name);
		}
	}

	if (hIcon)
	{
		int nIconPos = ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
		if (nIconPos != -1)
			icon_index[nItem] = nIconPos;
		DestroyIcon(hIcon);
	}
	if (icon_index[nItem] == 0)
		icon_index[nItem] = default_icon_index;
}


static void DestroyIcons()
{
	if (hSmall)
	{
		ImageList_Destroy(hSmall);
		hSmall = NULL;
	}

	if (icon_index)
	{
		int i;
		for (i=0;i<driver_list::total();i++)
			icon_index[i] = 0; // these are indices into hSmall
	}

	if (hLarge)
	{
		ImageList_Destroy(hLarge);
		hLarge = NULL;
	}

	if (hHeaderImages)
	{
		ImageList_Destroy(hHeaderImages);
		hHeaderImages = NULL;
	}

}


static void ReloadIcons()
{
	HICON hIcon;
	INT i = 0;

	// clear out all the images
	ImageList_RemoveAll(hSmall);
	ImageList_RemoveAll(hLarge);

	if (icon_index)
		for (i=0;i<driver_list::total();i++)
			icon_index[i] = 0; // these are indices into hSmall

	for (i = 0; g_iconData[i].icon_name; i++)
	{
		hIcon = LoadIconFromFile((char *) g_iconData[i].icon_name);
		if (hIcon == NULL)
			hIcon = LoadIcon(hInst, MAKEINTRESOURCE(g_iconData[i].resource));

		ImageList_AddIcon(hSmall, hIcon);
		ImageList_AddIcon(hLarge, hIcon);
		DestroyIcon(hIcon);
	}
}


static DWORD GetShellLargeIconSize()
{
	DWORD  dwSize = 32, dwLength = 512, dwType = REG_SZ;
	HKEY   hKey;
	LPTSTR tErrorMessage = NULL;

	/* Get the Key */
	LONG lRes = RegOpenKey(HKEY_CURRENT_USER, TEXT("Control Panel\\Desktop\\WindowMetrics"), &hKey);
	if( lRes != ERROR_SUCCESS )
	{
		GetSystemErrorMessage(lRes, &tErrorMessage);
		MessageBox(GetMainWindow(), tErrorMessage, TEXT("Large shell icon size registry access"), MB_OK | MB_ICONERROR);
		LocalFree(tErrorMessage);
		return dwSize;
	}

	/* Save the last size */
	TCHAR  szBuffer[512];
	lRes = RegQueryValueEx(hKey, TEXT("Shell Icon Size"), NULL, &dwType, (LPBYTE)szBuffer, &dwLength);
	if( lRes != ERROR_SUCCESS )
	{
		GetSystemErrorMessage(lRes, &tErrorMessage);
		MessageBox(GetMainWindow(), tErrorMessage, TEXT("Large shell icon size registry query"), MB_OK | MB_ICONERROR);
		LocalFree(tErrorMessage);
		RegCloseKey(hKey);
		return dwSize;
	}

	dwSize = _ttol(szBuffer);
	if (dwSize < 32)
		dwSize = 32;

	if (dwSize > 48)
		dwSize = 48;

	/* Clean up */
	RegCloseKey(hKey);
	return dwSize;
}


static DWORD GetShellSmallIconSize()
{
	DWORD dwSize = ICONMAP_WIDTH;

	if (dwSize < 48)
	{
		if (dwSize < 32)
			dwSize = 16;
		else
			dwSize = 32;
	}
	else
	{
		dwSize = 48;
	}
	return dwSize;
}


// create iconlist for Listview control
static void CreateIcons()
{
	DWORD dwSmallIconSize = GetShellSmallIconSize();
	DWORD dwLargeIconSize = GetShellLargeIconSize();
	HICON hIcon;
	int icon_count = 0;
	int grow = 5000;

	while(g_iconData[icon_count].icon_name)
		icon_count++;

	// the current window style affects the sizing of the rows when changing
	// between list views, so put it in small icon mode temporarily while we associate
	// our image list

	// using large icon mode instead kills the horizontal scrollbar when doing
	// full refresh, which seems odd (it should recreate the scrollbar when
	// set back to report mode, for example, but it doesn't).

	DWORD dwStyle = GetWindowLong(hwndList,GWL_STYLE);
	SetWindowLong(hwndList,GWL_STYLE,(dwStyle & ~LVS_TYPEMASK) | LVS_ICON);

	hSmall = ImageList_Create(dwSmallIconSize, dwSmallIconSize, ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (NULL == hSmall)
	{
		win_message_box_utf8(hwndList, "Cannot allocate small icon image list", "Allocation error - Exiting", IDOK);
		PostQuitMessage(0);
	}

	hLarge = ImageList_Create(dwLargeIconSize, dwLargeIconSize, ILC_COLORDDB | ILC_MASK, icon_count, icon_count + grow);

	if (NULL == hLarge)
	{
		win_message_box_utf8(hwndList, "Cannot allocate large icon image list", "Allocation error - Exiting", IDOK);
		PostQuitMessage(0);
	}

	ReloadIcons();

	// Associate the image lists with the list view control.
	(void)ListView_SetImageList(hwndList, hSmall, LVSIL_SMALL);
	(void)ListView_SetImageList(hwndList, hLarge, LVSIL_NORMAL);

	// restore our view
	SetWindowLong(hwndList,GWL_STYLE,dwStyle);

	CreateMessIcons(); // messui.cpp

	// Now set up header specific stuff
	hHeaderImages = ImageList_Create(8,8,ILC_COLORDDB | ILC_MASK,2,2);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_UP));
	ImageList_AddIcon(hHeaderImages,hIcon);
	hIcon = LoadIcon(hInst,MAKEINTRESOURCE(IDI_HEADER_DOWN));
	ImageList_AddIcon(hHeaderImages,hIcon);

	for (int i = 0; i < sizeof(s_nPickers) / sizeof(s_nPickers[0]); i++)
		Picker_SetHeaderImageList(GetDlgItem(hMain, s_nPickers[i]), hHeaderImages);
}


static int GamePicker_Compare(HWND hwndPicker, int index1, int index2, int sort_subitem)
{
	int value = 0;  /* Default to 0, for unknown case */
	const char *name1 = NULL;
	const char *name2 = NULL;
	char file1[MAX_PATH];
	char file2[MAX_PATH];
	int nTemp1=0, nTemp2=0;

	switch (sort_subitem)
	{
	case COLUMN_GAMES:
		return core_stricmp(ModifyThe(driver_list::driver(index1).type.fullname()),
			ModifyThe(driver_list::driver(index2).type.fullname()));

	case COLUMN_ORIENTATION:
		nTemp1 = DriverIsVertical(index1) ? 1 : 0;
		nTemp2 = DriverIsVertical(index2) ? 1 : 0;
		value = nTemp1 - nTemp2;
		break;

	case COLUMN_DIRECTORY:
		value = core_stricmp(driver_list::driver(index1).name, driver_list::driver(index2).name);
		break;

	case COLUMN_SRCDRIVERS:
		strcpy(file1, GetDriverFilename(index1));
		strcpy(file2, GetDriverFilename(index2));
		value = core_stricmp(file1, file2);
		break;

	case COLUMN_PLAYTIME:
		value = GetPlayTime(index1) - GetPlayTime(index2);
		break;

	case COLUMN_ROMS:
		value = GetRomAuditResults(index1) - GetRomAuditResults(index2);
		break;

	case COLUMN_SAMPLES:
		value = GetSampleAuditResults(index1) - GetSampleAuditResults(index2);
		break;

	case COLUMN_TYPE:
		{
			machine_config config1(driver_list::driver(index1),MameUIGlobal());
			machine_config config2(driver_list::driver(index2),MameUIGlobal());
			value = isDriverVector(&config1) - isDriverVector(&config2);
		}
		break;

	case COLUMN_TRACKBALL:
		value = DriverUsesTrackball(index1) - DriverUsesTrackball(index2);
		break;

	case COLUMN_PLAYED:
		value = GetPlayCount(index1) - GetPlayCount(index2);
		break;

	case COLUMN_MANUFACTURER:
		value = core_stricmp(driver_list::driver(index1).manufacturer, driver_list::driver(index2).manufacturer);
		break;

	case COLUMN_YEAR:
		value = core_stricmp(driver_list::driver(index1).year, driver_list::driver(index2).year);
		break;

	case COLUMN_CLONE:
		name1 = GetCloneParentName(index1);
		name2 = GetCloneParentName(index2);

		if (*name1 == '\0')
			name1 = NULL;
		if (*name2 == '\0')
			name2 = NULL;

		if (NULL == name1 && NULL == name2)
			value = 0;
		else if (name2 == NULL)
			value = -1;
		else if (name1 == NULL)
			value = 1;
		else
			value = core_stricmp(name1, name2);
		break;
	}

	// Handle same comparisons here
	if (0 == value && COLUMN_GAMES != sort_subitem)
		value = GamePicker_Compare(hwndPicker, index1, index2, COLUMN_GAMES);

	return value;
}


int GetSelectedPick()
{
	/* returns index of listview selected item */
	/* This will return -1 if not found */
	return ListView_GetNextItem(hwndList, -1, LVIS_SELECTED | LVIS_FOCUSED);
}


static HICON GetSelectedPickItemIcon()
{
	LV_ITEM lvi;
	lvi.iItem = GetSelectedPick();
	lvi.iSubItem = 0;
	lvi.mask = LVIF_IMAGE;
	ListView_GetItem(hwndList, &lvi);
	return ImageList_GetIcon(hLarge, lvi.iImage, ILD_TRANSPARENT);
}


static void SetRandomPickItem()
{
	int nListCount = ListView_GetItemCount(hwndList);

	if (nListCount > 0)
		Picker_SetSelectedPick(hwndList, rand() % nListCount);
}


BOOL CommonFileDialog(common_file_dialog_proc cfd, char *filename, int filetype)
{
	OPENFILENAME ofn;
	string dirname;
	TCHAR* t_filename;
	TCHAR t_filename_buffer[MAX_PATH]  = {0, };

	// convert the filename to UTF-8 and copy into buffer
	t_filename = ui_wstring_from_utf8(filename);
	if (t_filename)
	{
		_sntprintf(t_filename_buffer, std::size(t_filename_buffer), TEXT("%s"), t_filename);
		free(t_filename);
	}

	ofn.lStructSize       = sizeof(ofn);
	ofn.hwndOwner         = hMain;
	ofn.hInstance         = NULL;
	switch (filetype)
	{
	case FILETYPE_INPUT_FILES :
		ofn.lpstrFilter   = TEXT("input files (*.inp,*.zip,*.7z)\0*.inp;*.zip;*.7z\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("inp");
		dirname = dir_get_value(16);
		break;
	case FILETYPE_SAVESTATE_FILES :
		ofn.lpstrFilter   = TEXT("savestate files (*.sta)\0*.sta;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("sta");
		dirname = dir_get_value(17);
		break;
	case FILETYPE_WAVE_FILES :
		ofn.lpstrFilter   = TEXT("sounds (*.wav)\0*.wav;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("wav");
		dirname = dir_get_value(18);
		break;
	case FILETYPE_MNG_FILES :
		ofn.lpstrFilter   = TEXT("videos (*.mng)\0*.mng;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("mng");
		dirname = dir_get_value(18);
		break;
	case FILETYPE_AVI_FILES :
		ofn.lpstrFilter   = TEXT("videos (*.avi)\0*.avi;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("avi");
		dirname = dir_get_value(18);
		break;
	case FILETYPE_EFFECT_FILES :
		ofn.lpstrFilter   = TEXT("effects (*.png)\0*.png;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("png");
		dirname = dir_get_value(5);
		break;
	case FILETYPE_SHADER_FILES :
		ofn.lpstrFilter   = TEXT("shaders (*.vsh)\0*.vsh;\0");
		ofn.lpstrDefExt   = TEXT("vsh");
		dirname = dir_get_value(22); // + PATH_SEPARATOR + "glsl";
//		ofn.lpstrTitle  = TEXT("Select a GLSL shader file");
		break;
	case FILETYPE_BGFX_FILES :
		ofn.lpstrFilter   = TEXT("bgfx (*.json)\0*.json;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("json");
		dirname = dir_get_value(21) + PATH_SEPARATOR + "chains";
		break;
	case FILETYPE_LUASCRIPT_FILES :
		ofn.lpstrFilter   = TEXT("scripts (*.lua)\0*.lua;\0All files (*.*)\0*.*\0");
		ofn.lpstrDefExt   = TEXT("lua");
		dirname = ".";
		break;
	default:
		return false;
	}
	ofn.lpstrCustomFilter = NULL;
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 1;
	ofn.lpstrFile         = t_filename_buffer;
	ofn.nMaxFile          = std::size(t_filename_buffer);
	ofn.lpstrFileTitle    = NULL;
	ofn.nMaxFileTitle     = 0;

	// Only want first directory
	size_t i = dirname.find(";");
	if (i != string::npos)
		dirname.resize(i);
	if (dirname.empty())
		dirname = ".";
	ofn.lpstrInitialDir   = ui_wstring_from_utf8(dirname.c_str());

	ofn.lpstrTitle        = NULL;
	ofn.Flags             = OFN_EXPLORER | OFN_NOCHANGEDIR | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lCustData         = 0;
	ofn.lpfnHook          = NULL;
	ofn.lpTemplateName    = NULL;

	BOOL success = cfd(&ofn);
	if (success)
	{
		//printf("got filename %s nFileExtension %u\n",filename,ofn.nFileExtension);fflush(stdout);
		/*GetDirectory(filename,last_directory,sizeof(last_directory));*/
	}

	char *utf8_filename = ui_utf8_from_wstring(t_filename_buffer);
	if (utf8_filename)
	{
		snprintf(filename, MAX_PATH, "%s", utf8_filename);
		free(utf8_filename);
	}

	return success;
}


void SetStatusBarText(int part_index, const char *message)
{
	TCHAR* t_message = ui_wstring_from_utf8(message);
	if( !t_message )
		return;
	SendMessage(hStatusBar, SB_SETTEXT, (WPARAM) part_index, (LPARAM)(LPCTSTR) win_tstring_strdup(t_message));
	free(t_message);
}


void SetStatusBarTextF(int part_index, const char *fmt, ...)
{
	char buf[256];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	va_end(va);

	SetStatusBarText(part_index, buf);
}


static void CLIB_DECL ATTR_PRINTF(1,2) MameMessageBox(const char *fmt, ...)
{
	char buf[2048];
	va_list va;

	va_start(va, fmt);
	vsprintf(buf, fmt, va);
	win_message_box_utf8(GetMainWindow(), buf, MAMEUINAME, MB_OK | MB_ICONERROR);
	va_end(va);
}


static void MamePlayGameWithOptions(int nGame, const play_options *playopts)
{
	m_lock = true;
	if (g_pJoyGUI)
		KillTimer(hMain, JOYGUI_TIMER);

	if (GetCycleScreenshot() > 0)
		KillTimer(hMain, SCREENSHOT_TIMER);

	in_emulation = true;

	DWORD dwExitCode = RunMAME(nGame, playopts);
	if (dwExitCode == 0)
	{
		IncrementPlayCount(nGame);
		BOOL res = ListView_RedrawItems(hwndList, GetSelectedPick(), GetSelectedPick());
		res++;
	}
	else
	{
		ShowWindow(hMain, SW_SHOW);
	}

	in_emulation = false;

	// re-sort if sorting on # of times played
	if (GetSortColumn() == COLUMN_PLAYED)
		Picker_Sort(hwndList);

	UpdateStatusBar();

	ShowWindow(hMain, SW_SHOW);
	SetFocus(hwndList);

	if (g_pJoyGUI)
		SetTimer(hMain, JOYGUI_TIMER, JOYGUI_MS, NULL);

	if (GetCycleScreenshot() > 0)
		SetTimer(hMain, SCREENSHOT_TIMER, GetCycleScreenshot()*1000, NULL); //scale to seconds
}


static void MamePlayBackGame()
{
	char filename[MAX_PATH];
	*filename = 0;

	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
		strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_INPUT_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char bare_fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];
		char fname[MAX_PATH];
		play_options playopts;

		_splitpath(filename, drive, dir, bare_fname, ext);

		sprintf(path,"%s%s",drive,dir);
		sprintf(fname,"%s%s",bare_fname,ext);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		emu_file pPlayBack(MameUIGlobal().input_directory(), OPEN_FLAG_READ);
		std::error_condition fileerr = pPlayBack.open(fname);
		if (fileerr)
		{
			MameMessageBox("Could not open '%s' as a valid input file.", filename);
			return;
		}

		// check for game name embedded in .inp header
		inp_header header;

		/* read the header and verify that it is a modern version; if not, print an error */
		if (!header.read(pPlayBack))
		{
			MameMessageBox("Input file is corrupt or invalid (missing header)");
			return;
		}
		if ((!header.check_magic()) || (header.get_majversion() != inp_header::MAJVERSION))
		{
			MameMessageBox("Input file invalid or in an older, unsupported format");
			return;
		}

		string const sysname = header.get_sysname();
		nGame = -1;
		for (int i = 0; i < driver_list::total(); i++) // find game and play it
		{
			if (driver_list::driver(i).name == sysname)
			{
				nGame = i;
				break;
			}
		}
		if (nGame == -1)
		{
			MameMessageBox("Game \"%s\" cannot be found", sysname.c_str());
			return;
		}

		memset(&playopts, 0, sizeof(playopts));
		playopts.playback = fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}


static void MameLoadState()
{
	char filename[MAX_PATH];
	char selected_filename[MAX_PATH];

	*filename = 0;

	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
	{
		strcpy(filename, driver_list::driver(nGame).name);
		strcpy(selected_filename, driver_list::driver(nGame).name);
	}
	if (CommonFileDialog(GetOpenFileName, filename, FILETYPE_SAVESTATE_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char ext[_MAX_EXT];
		char path[MAX_PATH];
		char fname[MAX_PATH];
		char bare_fname[_MAX_FNAME];

		_splitpath(filename, drive, dir, bare_fname, ext);

		// parse path
		sprintf(path, "%s%s", drive, dir);
		sprintf(fname, "%s%s", bare_fname, ext);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		char* state_fname = filename;

		emu_file pSaveState(MameUIGlobal().state_directory(), OPEN_FLAG_READ);
		if (pSaveState.open(state_fname))
		{
			MameMessageBox("Could not open '%s' as a valid savestate file.", filename);
			return;
		}

		// call the MAME core function to check the save state file
		//int rc = state_manager::check_file(NULL, pSaveState, selected_filename, MameMessageBox);
		//if (rc)

		play_options playopts;
		memset(&playopts, 0, sizeof(playopts));
		playopts.state = state_fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}


static void MamePlayRecordGame()
{
	char filename[MAX_PATH];
	*filename = 0;
	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
		strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_INPUT_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];

		_splitpath(filename, drive, dir, fname, ext);

		sprintf(path,"%s%s",drive,dir);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		play_options playopts;
		memset(&playopts, 0, sizeof(playopts));
		strcat(fname, ".inp");
		playopts.record = fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}


void MamePlayGame()
{
	int nGame = Picker_GetSelectedItem(hwndList);

	if (m_lock)
		return;

	if (nGame != -1)
	{
		play_options playopts;
		memset(&playopts, 0, sizeof(playopts));
		MamePlayGameWithOptions(nGame, &playopts);
	}
}


static void MamePlayRecordWave()
{
	char filename[MAX_PATH];
	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
		strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_WAVE_FILES))
	{
		play_options playopts;
		memset(&playopts, 0, sizeof(playopts));
		playopts.wavwrite = filename;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}


static void MamePlayRecordMNG()
{
	char filename[MAX_PATH] = { 0, };
	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
		strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_MNG_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];

		_splitpath(filename, drive, dir, fname, ext);

		sprintf(path,"%s%s",drive,dir);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		play_options playopts;
		memset(&playopts, 0, sizeof(playopts));
		strcat(fname, ".mng");
		playopts.mngwrite = fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}


static void MamePlayRecordAVI()
{
	char filename[MAX_PATH] = { 0, };
	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame != -1)
		strcpy(filename, driver_list::driver(nGame).name);

	if (CommonFileDialog(GetSaveFileName, filename, FILETYPE_AVI_FILES))
	{
		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		char fname[_MAX_FNAME];
		char ext[_MAX_EXT];
		char path[MAX_PATH];

		_splitpath(filename, drive, dir, fname, ext);

		sprintf(path,"%s%s",drive,dir);
		if (path[strlen(path)-1] == '\\')
			path[strlen(path)-1] = 0; // take off trailing back slash

		play_options playopts;
		memset(&playopts, 0, sizeof(playopts));
		strcat(fname, ".avi");
		playopts.aviwrite = fname;
		playopts_apply = 0x57;
		MamePlayGameWithOptions(nGame, &playopts);
	}
}


/* Toggle ScreenShot ON/OFF */
static void ToggleScreenShot()
{
	UINT val = GetWindowPanes() ^ 8;
	BOOL show = BIT(val, 3);
	SetWindowPanes(val);
	UpdateScreenShot();

	/* Redraw list view */
	if (hBackground && show)
		InvalidateRect(hwndList, NULL, false);
}


static void AdjustMetrics()
{
	printf("Adjust Metrics\n");fflush(stdout);
	/* WM_SETTINGCHANGE also */
	int xtraX  = GetSystemMetrics(SM_CXFIXEDFRAME); /* Dialog frame width */
	int xtraY  = GetSystemMetrics(SM_CYFIXEDFRAME); /* Dialog frame height */
	xtraY += GetSystemMetrics(SM_CYMENUSIZE); /* Menu height */
	xtraY += GetSystemMetrics(SM_CYCAPTION); /* Caption Height */
	int maxX   = GetSystemMetrics(SM_CXSCREEN); /* Screen Width */
	int maxY   = GetSystemMetrics(SM_CYSCREEN); /* Screen Height */

	TEXTMETRIC tm;
	HDC hDC = GetDC(hMain);
	GetTextMetrics (hDC, &tm);

	/* Convert MIN Width/Height from Dialog Box Units to pixels. */
	MIN_WIDTH  = (int)((tm.tmAveCharWidth / 4.0) * DBU_MIN_WIDTH)  + xtraX;
	MIN_HEIGHT = (int)((tm.tmHeight / 8.0) * DBU_MIN_HEIGHT) + xtraY;
	ReleaseDC(hMain, hDC);

	COLORREF textColor;
	if ((textColor = GetListFontColor()) == RGB(255, 255, 255))
		textColor = RGB(240, 240, 240);

	TCHAR szClass[128];
	HWND hWnd = GetWindow(hMain, GW_CHILD);
	while(hWnd)
	{
		if (GetClassName(hWnd, szClass, sizeof(szClass) / sizeof(szClass[0])))
		{
			if (!_tcscmp(szClass, TEXT("SysListView32")))
			{
				BOOL res = ListView_SetBkColor(hWnd, GetSysColor(COLOR_WINDOW));
				res = ListView_SetTextColor(hWnd, textColor);
				res++;
			}
			else
			if (!_tcscmp(szClass, TEXT("SysTreeView32")))
			{
				HRESULT hres = TreeView_SetBkColor(hTreeView, GetSysColor(COLOR_WINDOW));
				hres = TreeView_SetTextColor(hTreeView, textColor);
				hres++;
			}
		}
		hWnd = GetWindow(hWnd, GW_HWNDNEXT);
	}

	AREA area;
	GetWindowArea(&area); // read window size from ini

	// Reposition the window so that the top or left side is in view.
	// The width and height never change, even if they stretch off the screen.
	if (area.x < 0)
		area.x = 0;
	if (area.y < 0)
		area.y = 0;

	// If the width or height is too small, or bigger than the screen, default them to the max screen size.
	if ((area.width < 200) || (area.width > maxX))
		area.width = maxX;
	if ((area.height < 100) || (area.height > maxY))
		area.height = maxY;

	SetWindowArea(&area);
	SetWindowPos(hMain, 0, area.x, area.y, area.width, area.height, SWP_NOZORDER | SWP_SHOWWINDOW | SWP_NOACTIVATE);
	printf("Adjust Metrics: Finished\n");fflush(stdout);
}


int FindIconIndex(int nIconResource)
{
	for(int i = 0; g_iconData[i].icon_name; i++)
	{
		if (g_iconData[i].resource == nIconResource)
			return i;
	}
	return -1;
}

// not used
int FindIconIndexByName(const char *icon_name)
{
	for (int i = 0; g_iconData[i].icon_name; i++)
	{
		if (!strcmp(g_iconData[i].icon_name, icon_name))
			return i;
	}
	return -1;
}


static int GetIconForDriver(int nItem)
{
	int iconRoms = 1;

	if (DriverUsesRoms(nItem))
	{
		int audit_result = GetRomAuditResults(nItem);
		if (audit_result == -1)
			return 2;
		else
		if (IsAuditResultYes(audit_result))
			iconRoms = 1;
		else
			iconRoms = 0;
	}

	// iconRoms is now either 0 (no roms), 1 (roms), or 2 (unknown)

	/* these are indices into icon_names, which maps into our image list
     * also must match IDI_WIN_NOROMS + iconRoms
     */

	if (iconRoms == 1)
	{
		// Show Red-X if the ROMs are present and flagged as NOT WORKING
		if (DriverIsBroken(nItem))
			iconRoms = FindIconIndex(IDI_WIN_REDX);  // iconRoms now = 4
		else
		// Show imperfect if the ROMs are present and flagged as imperfect
		if (DriverIsImperfect(nItem))
			iconRoms = FindIconIndex(IDI_WIN_IMPERFECT); // iconRoms now = 5
		else
		// show clone icon if we have roms and game is working
		if (DriverIsClone(nItem))
			iconRoms = FindIconIndex(IDI_WIN_CLONE); // iconRoms now = 3
	}

	// if we have the roms, then look for a custom per-game icon to override
	// not 2, because this indicates F5 must be done; not 0, because this indicates roms are missing; only use 4 if user chooses it
	BOOL redx = GetOverrideRedX() & (iconRoms == 4);
	if (iconRoms == 1 || iconRoms == 3 || iconRoms == 5 || redx)
	{
		if (icon_index[nItem] == 0)
			AddDriverIcon(nItem,iconRoms);
		iconRoms = icon_index[nItem];
	}

	return iconRoms;
}


static BOOL HandleTreeContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	TVHITTESTINFO hti;
	POINT pt;
	BOOL res = 0;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_TREE))
		return false;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	/* select the item that was right clicked or shift-F10'ed */
	hti.pt = pt;
	ScreenToClient(hTreeView,&hti.pt);
	(void)TreeView_HitTest(hTreeView,&hti);
	if ((hti.flags & TVHT_ONITEM) != 0)
		res = TreeView_SelectItem(hTreeView,hti.hItem);

	HMENU hTreeMenu = LoadMenu(hInst,MAKEINTRESOURCE(IDR_CONTEXT_TREE));

	InitTreeContextMenu(hTreeMenu);

	HMENU hMenu = GetSubMenu(hTreeMenu, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hTreeMenu);
	res++;
	return true;
}


static void GamePicker_OnBodyContextMenu(POINT pt)
{
	HMENU hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_MENU));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);
	InitBodyContextMenu(hMenu);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hMain,NULL);

	DestroyMenu(hMenuLoad);
}


static BOOL HandleScreenShotContextMenu(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	POINT pt;

	if ((HWND)wParam != GetDlgItem(hWnd, IDC_SSPICTURE) && (HWND)wParam != GetDlgItem(hWnd, IDC_SSFRAME))
		return false;

	pt.x = GET_X_LPARAM(lParam);
	pt.y = GET_Y_LPARAM(lParam);
	if (pt.x < 0 && pt.y < 0)
		GetCursorPos(&pt);

	HMENU hMenuLoad = LoadMenu(hInst, MAKEINTRESOURCE(IDR_CONTEXT_SCREENSHOT));
	HMENU hMenu = GetSubMenu(hMenuLoad, 0);

	UpdateMenu(hMenu);

	TrackPopupMenu(hMenu,TPM_LEFTALIGN | TPM_RIGHTBUTTON,pt.x,pt.y,0,hWnd,NULL);

	DestroyMenu(hMenuLoad);

	return true;
}


static void UpdateMenu(HMENU hMenu)
{
	TCHAR buf[200];
	MENUITEMINFO mItem;
	int nGame = Picker_GetSelectedItem(hwndList);
	if (nGame < 0)
		have_selection = 0;

	LPTREEFOLDER lpFolder = GetCurrentFolder();
	int i = 0;

	if (have_selection)
	{
		TCHAR* t_description = ui_wstring_from_utf8(ConvertAmpersandString(ModifyThe(driver_list::driver(nGame).type.fullname())));
		if( !t_description )
			return;

		_sntprintf(buf, std::size(buf), g_szPlayGameString, t_description);

		mItem.cbSize = sizeof(mItem);
		mItem.fMask = MIIM_TYPE;
		mItem.fType = MFT_STRING;
		mItem.dwTypeData = buf;
		mItem.cch = _tcslen(mItem.dwTypeData);

		SetMenuItemInfo(hMenu, ID_FILE_PLAY, false, &mItem);

		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_ENABLED);

		free(t_description);
	}
	else
	{
		EnableMenuItem(hMenu, ID_FILE_PLAY, MF_GRAYED);
		EnableMenuItem(hMenu, ID_FILE_PLAY_RECORD, MF_GRAYED);
		EnableMenuItem(hMenu, ID_GAME_PROPERTIES, MF_GRAYED);
		EnableMenuItem(hMenu, ID_CONTEXT_SELECT_RANDOM, MF_GRAYED);
	}

	if (lpFolder->m_dwFlags & F_CUSTOM)
	{
		EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_ENABLED);
		EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_ENABLED);
	}
	else
	{
		EnableMenuItem(hMenu,ID_CONTEXT_REMOVE_CUSTOM,MF_GRAYED);
		EnableMenuItem(hMenu,ID_CONTEXT_RENAME_CUSTOM,MF_GRAYED);
	}
	//const char* pParent = GetFolderNameByID(lpFolder->m_nParent+1);

	if (lpFolder->m_dwFlags & F_INIEDIT)
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_ENABLED);
	else
		EnableMenuItem(hMenu,ID_FOLDER_PROPERTIES,MF_GRAYED);

	CheckMenuRadioItem(hMenu, ID_VIEW_TAB_ARTWORK, ID_VIEW_TAB_HISTORY,
		ID_VIEW_TAB_ARTWORK + TabView_GetCurrentTab(hTabCtrl), MF_BYCOMMAND);

	// set whether we're showing the tab control or not
	if (bShowTabCtrl)
		CheckMenuItem(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu,ID_VIEW_PAGETAB,MF_BYCOMMAND | MF_UNCHECKED);

	for (i=0;i<MAX_TAB_TYPES;i++)
	{
		// disable menu items for tabs we're not currently showing
		if (GetShowTab(i))
			EnableMenuItem(hMenu,ID_VIEW_TAB_ARTWORK + i,MF_BYCOMMAND | MF_ENABLED);
		else
			EnableMenuItem(hMenu,ID_VIEW_TAB_ARTWORK + i,MF_BYCOMMAND | MF_GRAYED);

		// check toggle menu items
		if (GetShowTab(i))
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_ARTWORK + i,MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu, ID_TOGGLE_TAB_ARTWORK + i,MF_BYCOMMAND | MF_UNCHECKED);
	}

	for (i=0;i<MAX_FOLDERS;i++)
	{
		if (GetShowFolder(i))
			CheckMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_CHECKED);
		else
			CheckMenuItem(hMenu,ID_CONTEXT_SHOW_FOLDER_START + i,MF_BYCOMMAND | MF_UNCHECKED);
	}
}


void InitTreeContextMenu(HMENU hTreeMenu)
{
	extern const FOLDERDATA g_folderData[];

	MENUITEMINFO mii;
	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.wID = -1;
	mii.fMask = MIIM_SUBMENU | MIIM_ID;

	HMENU hMenu = GetSubMenu(hTreeMenu, 0);

	if (GetMenuItemInfo(hMenu,3,true,&mii) == false)
	{
		printf("can't find show folders context menu\n");fflush(stdout);
		return;
	}

	if (mii.hSubMenu == NULL)
	{
		printf("can't find submenu for show folders context menu\n");fflush(stdout);
		return;
	}

	hMenu = mii.hSubMenu;

	for (int i=0; g_folderData[i].m_lpTitle; i++)
	{
		if (!g_folderData[i].m_process)
		{
			TCHAR* t_title = ui_wstring_from_utf8(g_folderData[i].m_lpTitle);
			if( !t_title )
				return;

			mii.fMask = MIIM_TYPE | MIIM_ID;
			mii.fType = MFT_STRING;
			mii.dwTypeData = t_title;
			mii.cch = _tcslen(mii.dwTypeData);
			mii.wID = ID_CONTEXT_SHOW_FOLDER_START + g_folderData[i].m_nFolderId;

			// menu in resources has one empty item (needed for the submenu to setup properly)
			// so overwrite this one, append after
			if (i == 0)
				SetMenuItemInfo(hMenu,ID_CONTEXT_SHOW_FOLDER_START,false,&mii);
			else
				InsertMenuItem(hMenu,i,false,&mii);

			free(t_title);
		}
	}
}


void InitBodyContextMenu(HMENU hBodyContextMenu)
{
	int drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex < 0)
		return;

	TCHAR tmp[256];
	MENUITEMINFO mii;
	ZeroMemory(&mii,sizeof(mii));
	mii.cbSize = sizeof(mii);

	if (GetMenuItemInfo(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,false,&mii) == false)
	{
		printf("can't find show folders context menu\n");fflush(stdout);
		return;
	}

	_sntprintf(tmp,std::size(tmp),TEXT("Properties for %s"), ui_wstring_from_utf8(GetDriverFilename(drvindex) ));
	mii.fMask = MIIM_TYPE | MIIM_ID;
	mii.fType = MFT_STRING;
	mii.dwTypeData = tmp;
	mii.cch = _tcslen(mii.dwTypeData);
	mii.wID = ID_FOLDER_SOURCEPROPERTIES;

	// menu in resources has one default item
	// so overwrite this one
	SetMenuItemInfo(hBodyContextMenu,ID_FOLDER_SOURCEPROPERTIES,false,&mii);
	EnableMenuItem(hBodyContextMenu, ID_FOLDER_VECTORPROPERTIES, DriverIsVector(drvindex) ? MF_ENABLED : MF_GRAYED);
}


void ToggleShowFolder(int folder)
{
	int current_id = GetCurrentFolderID();
	SetWindowRedraw(hwndList, false);
	SetShowFolder(folder,!GetShowFolder(folder));
	ResetTreeViewFolders();
	SelectTreeViewFolder(current_id);
	SetWindowRedraw(hwndList, true);
}


static LRESULT CALLBACK HistoryWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (hBackground)
	{
		switch (uMsg)
		{
		case WM_MOUSEMOVE:
			{
				if (MouseHasBeenMoved())
					ShowCursor(true);
				break;
			}

			case WM_ERASEBKGND:
				return true;
			case WM_PAINT:
			{
				POINT p = { 0, 0 };

				/* get base point of background bitmap */
				MapWindowPoints(hWnd,hTreeView,&p,1);
				PaintBackgroundImage(hWnd, NULL, p.x, p.y);
				/* to ensure our parent procedure repaints the whole client area */
				InvalidateRect(hWnd, NULL, false);
				break;
			}
		}
	}
	return CallWindowProc(g_lpHistoryWndProc, hWnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK PictureFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		{
			if (MouseHasBeenMoved())
				ShowCursor(true);
			break;
		}

	case WM_NCHITTEST :
		{
			POINT pt;
			RECT  rect;
			HWND hHistory = GetDlgItem(hMain, IDC_HISTORY);

			pt.x = LOWORD(lParam);
			pt.y = HIWORD(lParam);
			GetWindowRect(hHistory, &rect);
			// check if they clicked on the picture area (leave 6 pixel no man's land
			// by the history window to reduce mistaken clicks)
			// no more no man's land, the Cursor changes when Edit control is left, should be enough feedback
			if (have_history &&
				( ( (TabView_GetCurrentTab(hTabCtrl) == TAB_HISTORY) ||
					(TabView_GetCurrentTab(hTabCtrl) == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ||
					(TAB_ALL == GetHistoryTab() && GetShowTab(TAB_HISTORY) == false) ) &&
//					(rect.top - 6) < pt.y && pt.y < (rect.bottom + 6) ) )
					PtInRect( &rect, pt ) ) )

			{
				return HTTRANSPARENT;
			}
			else
			{
				return HTCLIENT;
			}
		}
		break;
	case WM_CONTEXTMENU:
		if ( HandleScreenShotContextMenu(hWnd, wParam, lParam))
			return false;
		break;
	}

	if (hBackground)
	{
		switch (uMsg)
		{
		case WM_ERASEBKGND :
			return true;
		case WM_PAINT :
			{
				RECT rect,nodraw_rect;
				HRGN region,nodraw_region;
				POINT p = { 0, 0 };

				/* get base point of background bitmap */
				MapWindowPoints(hWnd,hTreeView,&p,1);

				/* get big region */
				GetClientRect(hWnd,&rect);
				region = CreateRectRgnIndirect(&rect);

				if (IsWindowVisible(GetDlgItem(hMain,IDC_HISTORY)))
				{
					/* don't draw over this window */
					GetWindowRect(GetDlgItem(hMain,IDC_HISTORY),&nodraw_rect);
					MapWindowPoints(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
					nodraw_region = CreateRectRgnIndirect(&nodraw_rect);
					CombineRgn(region,region,nodraw_region,RGN_DIFF);
					DeleteObject(nodraw_region);
				}

				if (IsWindowVisible(GetDlgItem(hMain,IDC_SSPICTURE)))
				{
					/* don't draw over this window */
					GetWindowRect(GetDlgItem(hMain,IDC_SSPICTURE),&nodraw_rect);
					MapWindowPoints(HWND_DESKTOP,hWnd,(LPPOINT)&nodraw_rect,2);
					nodraw_region = CreateRectRgnIndirect(&nodraw_rect);
					CombineRgn(region,region,nodraw_region,RGN_DIFF);
					DeleteObject(nodraw_region);
				}

				PaintBackgroundImage(hWnd,region,p.x,p.y);

				DeleteObject(region);

				/* to ensure our parent procedure repaints the whole client area */
				InvalidateRect(hWnd, NULL, false);

				break;
			}
		}
	}
	return CallWindowProc(g_lpPictureFrameWndProc, hWnd, uMsg, wParam, lParam);
}


static LRESULT CALLBACK PictureWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND :
		return true;
	case WM_PAINT :
		{
			int nBordersize = GetScreenshotBorderSize();
			HBRUSH hBrush = CreateSolidBrush(GetScreenshotBorderColor());
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd,&ps);
			HDC hdc_temp = CreateCompatibleDC(hdc);

			HBITMAP old_bitmap;
			int width,height;
			if (ScreenShotLoaded())
			{
				width = GetScreenShotWidth();
				height = GetScreenShotHeight();
				old_bitmap = (HBITMAP)SelectObject(hdc_temp,GetScreenShotHandle());
			}
			else
			{
				BITMAP bmp;
				GetObject(hMissing_bitmap,sizeof(BITMAP),&bmp);
				width = bmp.bmWidth;
				height = bmp.bmHeight;
				old_bitmap = (HBITMAP)SelectObject(hdc_temp,hMissing_bitmap);
			}

			RECT rect;
			GetClientRect(hWnd,&rect);
			RECT rect2 = rect;
			//Configurable Borders around images
			rect.bottom -= nBordersize;
			if( rect.bottom < 0)
				rect.bottom = rect2.bottom;
			rect.right -= nBordersize;
			if( rect.right < 0)
				rect.right = rect2.right;
			rect.top += nBordersize;
			if( rect.top > rect.bottom )
				rect.top = rect2.top;
			rect.left += nBordersize;
			if( rect.left > rect.right )
				rect.left = rect2.left;
			HRGN region1 = CreateRectRgnIndirect(&rect);
			HRGN region2 = CreateRectRgnIndirect(&rect2);
			CombineRgn(region2,region2,region1,RGN_DIFF);
			HBRUSH holdBrush = (HBRUSH)SelectObject(hdc, hBrush);
			FillRgn(hdc,region2, hBrush );
			SelectObject(hdc, holdBrush);
			DeleteBrush(hBrush);
			SetStretchBltMode(hdc,STRETCH_HALFTONE);
			StretchBlt(hdc,nBordersize,nBordersize,rect.right-rect.left,rect.bottom-rect.top,hdc_temp,0,0,width,height,SRCCOPY);
			SelectObject(hdc_temp,old_bitmap);
			DeleteDC(hdc_temp);
			DeleteObject(region1);
			DeleteObject(region2);
			EndPaint(hWnd,&ps);
			return true;
		}
	}

	return CallWindowProc(g_lpPictureWndProc, hWnd, uMsg, wParam, lParam);
}


static void RemoveCurrentGameCustomFolder()
{
	int drvindex = Picker_GetSelectedItem(hwndList);
	if (drvindex >= 0)
		RemoveGameCustomFolder(drvindex);
}


static void RemoveGameCustomFolder(int driver_index)
{
	TREEFOLDER **folders;
	int num_folders = 0;

	GetFolders(&folders,&num_folders);

	for (int i=0;i<num_folders;i++)
	{
		if (folders[i]->m_dwFlags & F_CUSTOM && folders[i]->m_nFolderId == GetCurrentFolderID())
		{
			int current_pick_index;

			RemoveFromCustomFolder(folders[i],driver_index);

			if (driver_index == Picker_GetSelectedItem(hwndList))
			{
			/* if we just removed the current game,
                  move the current selection so that when we rebuild the listview it
                  leaves the cursor on next or previous one */

				current_pick_index = GetSelectedPick();
				Picker_SetSelectedPick(hwndList, GetSelectedPick() + 1);
				if (current_pick_index == GetSelectedPick()) /* we must have deleted the last item */
					Picker_SetSelectedPick(hwndList, GetSelectedPick() - 1);
			}

			ResetListView();
			return;
		}
	}
	MessageBox(GetMainWindow(), TEXT("Error searching for custom folder"), TEXT(MAMEUINAME), MB_OK | MB_ICONERROR);
}


static void BeginListViewDrag(NM_LISTVIEW *pnmv)
{
	LV_ITEM lvi;
	POINT pt;
	BOOL res = 0;

	lvi.iItem = pnmv->iItem;
	lvi.mask = LVIF_PARAM;
	res = ListView_GetItem(hwndList, &lvi);

	game_dragged = lvi.lParam;

	pt.x = 0;
	pt.y = 0;

	/* Tell the list view control to create an image to use for dragging. */
	himl_drag = ListView_CreateDragImage(hwndList,pnmv->iItem,&pt);

	/* Start the drag operation. */
	ImageList_BeginDrag(himl_drag, 0, 0, 0);

	pt = pnmv->ptAction;
	ClientToScreen(hwndList,&pt);
	ImageList_DragEnter(GetDesktopWindow(),pt.x,pt.y);

	/* Hide the mouse cursor, and direct mouse input to the parent window. */
	SetCapture(hMain);

	prev_drag_drop_target = NULL;

	g_listview_dragging = true;
	res++;
}


static void MouseMoveListViewDrag(POINTS p)
{
	HTREEITEM htiTarget;
	TV_HITTESTINFO tvht;
	BOOL res;

	POINT pt;
	pt.x = p.x;
	pt.y = p.y;

	ClientToScreen(hMain,&pt);

	ImageList_DragMove(pt.x,pt.y);

	MapWindowPoints(GetDesktopWindow(),hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = TreeView_HitTest(hTreeView,&tvht);

	if (htiTarget != prev_drag_drop_target)
	{
		ImageList_DragShowNolock(false);
		if (htiTarget != NULL)
			res = TreeView_SelectDropTarget(hTreeView,htiTarget);
		else
			res = TreeView_SelectDropTarget(hTreeView,NULL);
		ImageList_DragShowNolock(true);

		prev_drag_drop_target = htiTarget;
	}
	res++;
}


static void ButtonUpListViewDrag(POINTS p)
{
	POINT pt;
	HTREEITEM htiTarget;
	TV_HITTESTINFO tvht;
	TVITEM tvi;
	BOOL res = 0;

	ReleaseCapture();

	ImageList_DragLeave(hwndList);
	ImageList_EndDrag();
	ImageList_Destroy(himl_drag);

	res = TreeView_SelectDropTarget(hTreeView,NULL);

	g_listview_dragging = false;

	/* see where the game was dragged */

	pt.x = p.x;
	pt.y = p.y;

	MapWindowPoints(hMain,hTreeView,&pt,1);

	tvht.pt = pt;
	htiTarget = TreeView_HitTest(hTreeView,&tvht);
	if (htiTarget == NULL)
	{
		LVHITTESTINFO lvhtti;
		LPTREEFOLDER folder;
		RECT rcList;

		/* the user dragged a game onto something other than the treeview */
		/* try to remove if we're in a custom folder */

		/* see if it was dragged within the list view; if so, ignore */

		MapWindowPoints(hTreeView,hwndList,&pt,1);
		lvhtti.pt = pt;
		GetWindowRect(hwndList, &rcList);
		ClientToScreen(hwndList, &pt);
		if( PtInRect(&rcList, pt) != 0 )
			return;

		folder = GetCurrentFolder();
		if (folder->m_dwFlags & F_CUSTOM)
		{
			/* dragged out of a custom folder, so let's remove it */
			RemoveCurrentGameCustomFolder();
		}
		return;
	}

	tvi.lParam = 0;
	tvi.mask  = TVIF_PARAM | TVIF_HANDLE;
	tvi.hItem = htiTarget;

	if (TreeView_GetItem(hTreeView, &tvi))
	{
		LPTREEFOLDER folder = (LPTREEFOLDER)tvi.lParam;
		AddToCustomFolder(folder,game_dragged);
	}
	res++;
}


static LPTREEFOLDER GetSelectedFolder()
{
	HTREEITEM htree = TreeView_GetSelection(hTreeView);
	if(htree)
	{
		TVITEM tvi;
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		TreeView_GetItem(hTreeView,&tvi);
		return (LPTREEFOLDER)tvi.lParam;
	}
	return NULL;
}


#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
static HICON GetSelectedFolderIcon()
{
	LPTREEFOLDER folder;
	HTREEITEM htree = TreeView_GetSelection(hTreeView);
	if (htree)
	{
		TVITEM tvi;
		tvi.hItem = htree;
		tvi.mask = TVIF_PARAM;
		(void)TreeView_GetItem(hTreeView,&tvi);
		folder = (LPTREEFOLDER)tvi.lParam;
		HIMAGELIST hSmall_icon;
		//hSmall_icon = TreeView_GetImageList(hTreeView,(int)tvi.iImage);
		hSmall_icon = NULL;
		return ImageList_GetIcon(hSmall_icon, tvi.iImage, ILD_TRANSPARENT);
	}
	return NULL;
}
#pragma GCC diagnostic error "-Wunused-but-set-variable"


/* Updates all currently displayed Items in the List with the latest Data*/
void UpdateListView()
{
	ResetWhichGamesInFolders();
	ResetListView();
	(void)ListView_RedrawItems(hwndList, ListView_GetTopIndex(hwndList), ListView_GetTopIndex(hwndList) + ListView_GetCountPerPage(hwndList));
	SetFocus(hwndList);
}


static void CalculateBestScreenShotRect(HWND hWnd, RECT *pRect, BOOL restrict_height)
{
	int destX=0, destY=0;
	int destW=0, destH=0;
	RECT rect;
	/* for scaling */
	int x=0, y=0;
	double scale=0;
	BOOL bReduce = false;

	GetClientRect(hWnd, &rect);

	// Scale the bitmap to the frame specified by the passed in hwnd
	if (ScreenShotLoaded())
	{
		x = GetScreenShotWidth();
		y = GetScreenShotHeight();
	}
	else
	{
		BITMAP bmp;
		GetObject(hMissing_bitmap, sizeof(BITMAP), &bmp);
		x = bmp.bmWidth;
		y = bmp.bmHeight;
	}
	int rWidth  = (rect.right  - rect.left);
	int rHeight = (rect.bottom - rect.top);

	/* Limit the screen shot to max height of 264 */
	if (restrict_height == true && rHeight > 264)
	{
		rect.bottom = rect.top + 264;
		rHeight = 264;
	}

	/* If the bitmap does NOT fit in the screenshot area */
	if ((x > rWidth - 10 || y > rHeight - 10) || GetStretchScreenShotLarger())
	{
		rect.right  -= 10;
		rect.bottom -= 10;
		rWidth  -= 10;
		rHeight -= 10;
		bReduce = true;
		/* Try to scale it properly */
		/*  assumes square pixels, doesn't consider aspect ratio */
		if (x > y)
			scale = (double)rWidth / x;
		else
			scale = (double)rHeight / y;

		destW = (int)(x * scale);
		destH = (int)(y * scale);

		/* If it's still too big, scale again */
		if (destW > rWidth || destH > rHeight)
		{
			if (destW > rWidth)
				scale = (double)rWidth / destW;
			else
				scale = (double)rHeight / destH;

			destW = (int)(destW * scale);
			destH = (int)(destH * scale);
		}
	}
	else
	{
		// Use the bitmaps size if it fits
		destW = x;
		destH = y;
	}

	destX = ((rWidth  - destW) / 2);
	destY = ((rHeight - destH) / 2);

	if (bReduce)
	{
		destX += 5;
		destY += 5;
	}

	int nBorder = GetScreenshotBorderSize();

	if( destX > nBorder+1)
		pRect->left = destX - nBorder;
	else
		pRect->left = 2;

	if( destY > nBorder+1)
		pRect->top = destY - nBorder;
	else
		pRect->top = 2;

	if( rWidth >= destX + destW + nBorder)
		pRect->right = destX + destW + nBorder;
	else
		pRect->right = rWidth - pRect->left;

	if( rHeight >= destY + destH + nBorder)
		pRect->bottom = destY + destH + nBorder;
	else
		pRect->bottom = rHeight - pRect->top;
}


/*
  Switches to either fullscreen or normal mode, based on the
  current mode.

  POSSIBLE BUGS:
  Removing the menu might cause problems later if some
  function tries to poll info stored in the menu. Don't
  know if you've done that, but this was the only way I
  knew to remove the menu dynamically.
*/
static void SwitchFullScreenMode()
{
	LONG lMainStyle=0;

	if (GetRunFullScreen())
	{
		// Return to normal

		// Restore the menu
		SetMenu(hMain, LoadMenu(hInst,MAKEINTRESOURCE(IDR_UI_MENU)));

		// Refresh the checkmarks
		CheckMenuItem(GetMenu(hMain), ID_VIEW_FOLDERS, BIT(GetWindowPanes(), 0) ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_TOOLBARS, GetShowToolBar() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_STATUS, GetShowStatusBar() ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(GetMenu(hMain), ID_VIEW_PAGETAB, GetShowTabCtrl() ? MF_CHECKED : MF_UNCHECKED);

		// Add frame to dialog again
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle | WS_BORDER;
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);

		// Show the window maximized
		if( GetWindowState() == SW_MAXIMIZE )
		{
			ShowWindow(hMain, SW_NORMAL);
			ShowWindow(hMain, SW_MAXIMIZE);
		}
		else
			ShowWindow(hMain, SW_RESTORE);

		SetRunFullScreen(false);
	}
	else
	{
		// Set to fullscreen

		// Remove menu
		SetMenu(hMain,NULL);

		// Frameless dialog (fake fullscreen)
		lMainStyle = GetWindowLong(hMain, GWL_STYLE);
		lMainStyle = lMainStyle & (WS_BORDER ^ 0xffffffff);
		SetWindowLong(hMain, GWL_STYLE, lMainStyle);
		if( IsMaximized(hMain) )
		{
			ShowWindow(hMain, SW_NORMAL);
			SetWindowState( SW_MAXIMIZE );
		}
		ShowWindow(hMain, SW_MAXIMIZE);

		SetRunFullScreen(true);
	}
}


/*
  Checks to see if the mouse has been moved since this func
  was first called (which is at startup). The reason for
  storing the startup coordinates of the mouse is that when
  a window is created it generates WM_MOUSEOVER events, even
  though the user didn't actually move the mouse. So we need
  to know when the WM_MOUSEOVER event is user-triggered.

  POSSIBLE BUGS:
  Gets polled at every WM_MOUSEMOVE so it might cause lag,
  but there's probably another way to code this that's
  way better?

*/
BOOL MouseHasBeenMoved()
{
	static int mouse_x = -1;
	static int mouse_y = -1;
	POINT p;

	GetCursorPos(&p);

	if (mouse_x == -1) // First time
	{
		mouse_x = p.x;
		mouse_y = p.y;
	}

	return (p.x != mouse_x || p.y != mouse_y);
}

static bool CommonListDialog(common_file_dialog_proc cfd)
{
	bool success = false;
	OPENFILENAME of;
	wchar_t szFile[MAX_PATH];
	wchar_t szCurDir[MAX_PATH];

	szFile[0] = 0;

	// Save current directory (avoids mame file creation further failure)
	if (GetCurrentDirectory(MAX_PATH, szCurDir) > MAX_PATH)
	{
		// Path too large
		szCurDir[0] = 0;
	}

	of.lStructSize = sizeof(OPENFILENAME);
	of.hwndOwner = hMain;
	of.hInstance = NULL;

	of.lpstrTitle  = TEXT("Enter a filter name");

	of.lpstrFilter = TEXT("Filter file (*.ini)\0*.ini\0");
	of.lpstrCustomFilter = NULL;
	of.nMaxCustFilter = 0;
	of.nFilterIndex = 1;
	of.lpstrFile = szFile;
	of.nMaxFile = sizeof(szFile);
	of.lpstrFileTitle = NULL;
	of.nMaxFileTitle = 0;
	of.lpstrInitialDir = ui_wstring_from_utf8(dir_get_value(24).c_str());
	of.nFileOffset = 0;
	of.nFileExtension = 0;
	of.lpstrDefExt = TEXT("ini");
	of.lCustData = 0;
	of.lpfnHook = NULL; //&OFNHookProc;
	of.lpTemplateName = NULL;
	of.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK;

	while(!success)
	{
		if (GetSaveFileName(&of))
		{
			if (GetFileAttributes(szFile) != -1)
			{
				if (win_message_box_utf8(hMain, "File already exists, overwrite ?", MAMEUINAME, MB_ICONQUESTION | MB_YESNO) != IDYES )
					continue;
				else
					success = true;

				SetFileAttributes(szFile, FILE_ATTRIBUTE_NORMAL);
			}

			SaveGameListToFile(ui_utf8_from_wstring(szFile));
			// Save current directory (avoids mame file creation further failure)
			GetCurrentDirectory(MAX_PATH, list_directory);
			// Restore current file path
			if (szCurDir[0] != 0)
				SetCurrentDirectory(szCurDir);

			success = true;
		}
		else
			break;
	}

	return success;
}

static void SaveGameListToFile(char *szFile)
{
	int nListCount = ListView_GetItemCount(hwndList);
	LVITEM lvi;

	FILE *f = fopen(szFile, "w");

	if (f == NULL)
	{
		ErrorMessageBox("Error : unable to open file");
		return;
	}

	// Header
	fprintf(f, "[ROOT_FOLDER]\n");

	// Games
	for (int nIndex = 0; nIndex < nListCount; nIndex++)
	{
		lvi.iItem = nIndex;
		lvi.iSubItem = 0;
		lvi.mask = LVIF_PARAM;

		if (ListView_GetItem(hwndList, &lvi))
		{
			int nGameIndex  = lvi.lParam;
			fprintf(f, "%s%s", driver_list::driver(nGameIndex).name,"\n");
		}
	}

	fclose(f);
	win_message_box_utf8(hMain, "File saved successfully.", MAMEUINAME, MB_ICONINFORMATION | MB_OK);
}


string longdots(string incoming, uint16_t howmany)
{
	// change all newlines to spaces
	for (uint16_t i = 0; i < incoming.size(); i++)
		if (incoming[i] == '\n')
			incoming[i] = ' ';
	// Now assume all is ok
	string outgoing = incoming;
	// But if it's too long, replace the excess with dots
	if ((howmany > 5) && (incoming.length() > howmany))
		outgoing = incoming.substr(0, howmany) + "...";
	return outgoing;
}

 //  wstring_from_utf8
 //============================================================
 
WCHAR *ui_wstring_from_utf8(const char *utf8string)
{
	int char_count;
	WCHAR *result;

	// convert MAME string (UTF-8) to UTF-16
	char_count = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, nullptr, 0);
	result = (WCHAR *)malloc(char_count * sizeof(*result));
	if (result != nullptr)
		MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, result, char_count);
 
	return result;
}

char *ui_utf8_from_wstring(const WCHAR *wstring)
{
	int char_count;
	char *result;

	// convert UTF-16 to MAME string (UTF-8)
	char_count = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, nullptr, 0, nullptr, nullptr);
	result = (char *)malloc(char_count * sizeof(*result));
	if (result != nullptr)
		WideCharToMultiByte(CP_UTF8, 0, wstring, -1, result, char_count, nullptr, nullptr);
	return result;
}

