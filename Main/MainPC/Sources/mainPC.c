
#include "stdio.h"
#include "BASe/CLIbrary/CLIwin.h"
#include "BASe/CLIbrary/CLIstr.h"
#include "ENGine/Sources/ENGinit.h"
#include "ENGine/Sources/ENGvars.h"
#include "ENGine/Sources/ENGmsg.h"
#include "ENGine/Sources/ENGloop.h"
#include "ENGine/Sources/WORld/WORinit.h"
#include "BIGfiles/BIGopen.h"
#include "BIGfiles/BIGfat.h"
#include "SouND/sources/SNDstruct.h"
#include "SouND/sources/SNDconst.h"
#include "INOut/INOjoystick.h"

extern BOOL LOA_gb_SpeedMode;
extern int  TEX_gi_ForceText;
extern int  GDI_gi_GDIType;


/*
 =======================================================================================================================
 =======================================================================================================================
 */
static LRESULT CALLBACK sfnl_WindowProcMainOwner(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	/*~~~~~~~~~~~~~*/
	LRESULT l_Result;
	/*~~~~~~~~~~~~~*/

	if(MAI_b_TreatMainWndMessages(hwnd, message, wParam, lParam, &l_Result)) return l_Result;
	if(MAI_b_TreatOwnerWndMessages(hwnd, &MAI_gst_MainHandles, message, wParam, lParam, &l_Result)) return l_Result;
	if(MAI_b_TreatDisplayWndMessages(hwnd, &MAI_gst_MainHandles, message, wParam, lParam, &l_Result)) return l_Result;

	return DefWindowProc(hwnd, message, wParam, lParam);
}

/*
 =======================================================================================================================
    Purpose: Create application window.
 =======================================================================================================================
 */
static void s_CreateWindow(HINSTANCE hInstance)
{
	/*~~~~~~~~~~~~~~~~*/
	WNDCLASS	x_Class;
	RECT		rect;
	/*~~~~~~~~~~~~~~~~*/

	/* Create main window */
	x_Class.style = 0;
	x_Class.lpfnWndProc = sfnl_WindowProcMainOwner;
	x_Class.cbClsExtra = 0;
	x_Class.cbWndExtra = 0;
	x_Class.hInstance = hInstance;
	x_Class.hIcon = NULL;
	x_Class.hCursor = NULL;
	x_Class.hbrBackground = NULL;
	x_Class.lpszMenuName = NULL;
	x_Class.lpszClassName = "BGEMain";
	RegisterClass(&x_Class);

	rect.left = 0;
	rect.top = 0;
	rect.right = 640;
	rect.bottom = 480;
	AdjustWindowRect( &rect, WS_OVERLAPPEDWINDOW, FALSE );
    
	MAI_gh_MainWindow = MAI_gst_MainHandles.h_OwnerWindow = CreateWindow
		(
			"BGEMain",
			"Beyond Good & Evil (c) Ubi Soft Entertainment",
			WS_OVERLAPPEDWINDOW,
			20,
			20,
			rect.right - rect.left,
			rect.bottom - rect.top,
			NULL,
			NULL,
			hInstance,
			NULL
		);

	MAI_gst_MainHandles.h_DisplayWindow = MAI_gh_MainWindow;

	ShowWindow(MAI_gh_MainWindow, SW_SHOW);
}

/*
 =======================================================================================================================
    Purpose: Analyse command line.
 =======================================================================================================================
 */
static void s_AnalyseCommandLine(char *_psz_Line)
{
	while(1)
	{
		while(*_psz_Line == ' ') *_psz_Line++;
		if(*_psz_Line != '/') break;

		_psz_Line++;
		if(!L_strnicmp(_psz_Line, "nosound", 7))
		{
			SND_gc_NoSound = 1;
			_psz_Line += 7;
		}
		else if(!L_strnicmp(_psz_Line, "joy", 3))
		{
			/*~~~~~~~~~~~~~*/
			FILE	*hp_File;
			/*~~~~~~~~~~~~~*/

			hp_File = fopen("joy.ini", "rt");
			if(hp_File)
			{
				fscanf(hp_File, "%d\n", &win32INO_l_Joystick_YDownStart);
				fscanf(hp_File, "%d\n", &win32INO_l_Joystick_XRightStart);
				fscanf(hp_File, "%d\n", &win32INO_l_Joystick_YUpStart);
				fscanf(hp_File, "%d\n", &win32INO_l_Joystick_XLeftStart);
				fscanf(hp_File, "%d\n", &win32INO_l_Joystick_YDown);
				fscanf(hp_File, "%d\n", &win32INO_l_Joystick_XRight);
				fscanf(hp_File, "%d\n", &win32INO_l_Joystick_YUp);
				fscanf(hp_File, "%d\n", &win32INO_l_Joystick_XLeft);
			}

			_psz_Line += 3;
		}
		else if(!L_strnicmp(_psz_Line, "ps2joy", 6))
		{
			win32INO_l_Joystick_Mode = INO_Joy_Ps2Mode;
			_psz_Line += 6;
		}
		else if(!L_strnicmp(_psz_Line, "pcjoy", 5))
		{
			win32INO_l_Joystick_Mode = INO_Joy_PCMode;
			_psz_Line += 5;
		}
		else if(!L_strnicmp(_psz_Line, "B", 1))
		{
			LOA_gb_SpeedMode = TRUE;
			_psz_Line += 1;
		}
        else if(!L_strnicmp(_psz_Line, "T4", 2))
        {
            TEX_gi_ForceText |= TEX_Manager_Accept4bpp;
            _psz_Line += 2;
        }
        else if(!L_strnicmp(_psz_Line, "T8", 2))
        {
            TEX_gi_ForceText |= TEX_Manager_Accept8bpp;
            _psz_Line += 2;
        }
        else if(!L_strnicmp(_psz_Line, "TA", 2))
        {
            TEX_gi_ForceText |= TEX_Manager_AcceptAlphaPalette;
            _psz_Line += 2;
        }
        else if(!L_strnicmp(_psz_Line, "TM", 2))
        {
            TEX_gi_ForceText |= TEX_Manager_OneTexForRawPal;
            _psz_Line += 2;
        }
        else if(!L_strnicmp(_psz_Line, "TX", 2))
        {
            TEX_gi_ForceText |= TEX_Manager_AcceptAllPalette | TEX_Manager_OneTexForRawPal;
            _psz_Line += 2;
        }
        else if(!L_strnicmp(_psz_Line, "TF", 2))
        {
            TEX_gi_ForceText |= TEX_Manager_FixVRam;
            _psz_Line += 2;
        }
        else if(!L_strnicmp(_psz_Line, "T32", 3))
        {	// accept ONLY 32 bpp testures 
            TEX_gi_ForceText |= TEX_Manager_Accept32bpp;
            TEX_gi_ForceText &= ~TEX_Manager_Accept4bpp;
            TEX_gi_ForceText &= ~TEX_Manager_Accept8bpp;
            TEX_gi_ForceText &= ~TEX_Manager_Accept16bpp_4444;
            TEX_gi_ForceText &= ~TEX_Manager_Accept16bpp_1555;
            TEX_gi_ForceText &= ~TEX_Manager_Accept24bpp;
			_psz_Line += 3;
        }
        else if(!L_strnicmp(_psz_Line, "DX8", 3))
        {
            GDI_gi_GDIType = 1;
            _psz_Line += 3;
        }
		else
		{
			while(*_psz_Line && (*_psz_Line != ' ')) _psz_Line++;
		}
	}

	/* Get size of blocks */
	sscanf(_psz_Line, "%s", MAI_gst_InitStruct.asz_ProjectName);
}

/*
 =======================================================================================================================
    Purpose: Windows main entry function.
 =======================================================================================================================
 */
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	/* Common SDK inits */
	ENG_InitApplication();

	/* Analyse the command line to get bigfile name and eventually other things... */
	s_AnalyseCommandLine((char *) lpCmdLine);

	/* Test argument */
	if(!(*MAI_gst_InitStruct.asz_ProjectName))
	{
		strcpy(MAI_gst_InitStruct.asz_ProjectName, "sally.bf"); // HACK: hardcoded default filename
	}

	/* Open bigfile */
	BIG_Open(MAI_gst_InitStruct.asz_ProjectName);

	/* Create main window */
	MAI_gh_MainInstance = hInstance;
	s_CreateWindow(hInstance);

	/* Call the engine main loop */
	ENG_InitEngine();
	ENG_Loop();
	WOR_Universe_Close(0);
	ENG_CloseEngine();

	/* Destroy main window */
	DestroyWindow(MAI_gh_MainWindow);

	/* Close bigfile */
	BIG_Close();

	/* Close SDK */
	ENG_CloseApplication();

	return 0;
}

