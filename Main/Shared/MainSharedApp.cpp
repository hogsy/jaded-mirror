// Created by Mark "hogsy" Sowden, 2023-2024 <hogsy@snortysoft.net>
// https://oldtimes-software.com/jaded/

#include "Precomp.h"

#include "MainSharedSystem.h"

#include "BASe/MEMory/MEMpro.h"
#include "ENGine/Sources/ENGinit.h"
#include "ENGine/Sources/ENGvars.h"
#include "ENGine/Sources/ENGloop.h"
#include "ENGine/Sources/WORld/WORinit.h"
#include "SDK/Sources/BIGfiles/BIGopen.h"
#include "GDInterface/GDIrasters.h"
#include "GDInterface/GDInterface.h"

#include "ImGuiInterface.h"

#include "Profiler.h"

jaded::sys::Profiler jaded::sys::profiler;

static SDL_Window   *sdlWindow;
static SDL_GLContext sdlGLContext;

//#define USE_SDL_GL_CONTEXT

/******************************************************************/
/******************************************************************/

void jaded::sys::Profiler::StartProfiling( const std::string &set )
{
	if ( !isActive )
	{
		return;
	}

	auto i = profSets.find( set );
	if ( i == profSets.end() )
	{
		profSets.emplace( set, Profile() );
		return;
	}

	i->second.Start();
}

void jaded::sys::Profiler::EndProfiling( const std::string &set )
{
	if ( !isActive )
	{
		return;
	}

	auto i = profSets.find( set );
	assert( i != profSets.end() );
	i->second.End();
}

// The below should work with the old BeginRaster / EndRaster macros

extern "C" void Jaded_Profiler_StartProfiling( unsigned int set )
{
	jaded::sys::profiler.StartProfiling( std::to_string( set ) );
}

extern "C" void Jaded_Profiler_EndProfiling( unsigned int set )
{
	jaded::sys::profiler.EndProfiling( std::to_string( set ) );
}

/******************************************************************/
/******************************************************************/

#if defined( _WIN32 )

extern int EDI_EditorWin32Execution( HINSTANCE );

static HWND nativeWindowHandle;

#	include <DbgHelp.h>

static LONG WINAPI Win32CrashHandler( EXCEPTION_POINTERS *exception )
{
	MessageBox( nullptr, "Encountered an exception, attempting to generate dump!", "Error", MB_OK | MB_ICONERROR );

	HMODULE dbgHelpLib = LoadLibrary( "DBGHELP.DLL" );
	if ( dbgHelpLib == nullptr )
		return EXCEPTION_CONTINUE_SEARCH;

	typedef BOOL( WINAPI * MINIDUMP_WRITE_DUMP )(
	        IN HANDLE                                                hProcess,
	        IN DWORD                                                 ProcessId,
	        IN HANDLE                                                hFile,
	        IN MINIDUMP_TYPE                                         DumpType,
	        IN CONST PMINIDUMP_EXCEPTION_INFORMATION                 ExceptionParam,
	        OPTIONAL IN PMINIDUMP_USER_STREAM_INFORMATION            UserStreamParam,
	        OPTIONAL IN PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL );

	auto MiniDumpWriteDump_ = ( MINIDUMP_WRITE_DUMP ) GetProcAddress( dbgHelpLib, "MiniDumpWriteDump" );
	if ( MiniDumpWriteDump_ == nullptr )
	{
		FreeLibrary( dbgHelpLib );
		return EXCEPTION_CONTINUE_SEARCH;
	}

	MINIDUMP_EXCEPTION_INFORMATION M;
	CHAR                           Dump_Path[ MAX_PATH ];

	M.ThreadId          = GetCurrentThreadId();
	M.ExceptionPointers = exception;
	M.ClientPointers    = 0;

	GetModuleFileName( nullptr, Dump_Path, sizeof( Dump_Path ) );
	lstrcpy( Dump_Path + lstrlen( Dump_Path ) - 3, "dmp" );

	HANDLE fileDump = CreateFile( Dump_Path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( fileDump )
	{
		MiniDumpWriteDump_( GetCurrentProcess(), GetCurrentProcessId(), fileDump, MiniDumpNormal, ( exception ) ? &M : nullptr, nullptr, nullptr );
		CloseHandle( fileDump );
	}

	FreeLibrary( dbgHelpLib );
	return EXCEPTION_CONTINUE_SEARCH;
}

#endif

static void ParseStartupParameters()
{
	assert( jaded::sys::launchArguments != nullptr );

	for ( int i = 0; i < jaded::sys::numLaunchArguments; ++i )
	{
		if ( *jaded::sys::launchArguments[ i ] != '/' )
			continue;

		if ( SDL_strcasecmp( jaded::sys::launchArguments[ i ], "/editor" ) == 0 )
		{
			jaded::sys::launchOperations.editorMode = true;
			continue;
		}
		else if ( SDL_strcasecmp( jaded::sys::launchArguments[ i ], "/popupError" ) == 0 )// Showin added Param for PopUp Script Errors (if off it uses console)
		{
			jaded::sys::launchOperations.popupError = true;
			continue;
		}
		else if ( SDL_strcasecmp( jaded::sys::launchArguments[ i ], "/console" ) == 0 )
		{
			jaded::sys::launchOperations.debugConsole = true;
			continue;
		}
		else if ( SDL_strcasecmp( jaded::sys::launchArguments[ i ], "/window" ) == 0 )
		{
			jaded::sys::launchOperations.forceWindowed = true;
			continue;
		}
		else if ( SDL_strncasecmp( jaded::sys::launchArguments[ i ], "/width", 6 ) == 0 )
		{
			jaded::sys::launchOperations.forcedWidth = strtol( jaded::sys::launchArguments[ i ] + 7, nullptr, 10 );
			continue;
		}
		else if ( SDL_strncasecmp( jaded::sys::launchArguments[ i ], "/height", 7 ) == 0 )
		{
			jaded::sys::launchOperations.forcedHeight = strtol( jaded::sys::launchArguments[ i ] + 8, nullptr, 10 );
			continue;
		}
		else if ( SDL_strncasecmp( jaded::sys::launchArguments[ i ], "/profile", 8 ) == 0 )
		{
			jaded::sys::profiler.SetActive( true );
			ENG_gb_LimitFPS = false;
			continue;
		}
		else if ( SDL_strncasecmp( jaded::sys::launchArguments[ i ], "/bf", 3 ) == 0 )
		{
			if ( ++i >= jaded::sys::numLaunchArguments )
			{
				break;
			}

#if 0// init occurs after we parse launch arguments, and resets this...
			snprintf( MAI_gst_InitStruct.asz_ProjectName, sizeof( MAI_gst_InitStruct.asz_ProjectName ), "%s", jaded::sys::launchArguments[ i ] );
#else
			jaded::sys::launchOperations.projectFile = jaded::sys::launchArguments[ i ];
#endif
			continue;
		}
	}
}

static SDL_Window *CreateSDLWindow()
{
#if defined( USE_SDL_GL_CONTEXT )

	int flags = SDL_WINDOW_OPENGL;
	if ( !jaded::sys::launchOperations.forceWindowed )
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

	SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 5 );
	SDL_GL_SetAttribute( SDL_GL_STENCIL_SIZE, 8 );

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 3 );

#else

	int flags = 0;

#endif

	int                    w, h;
	const SDL_DisplayMode *displayMode;
	if ( ( displayMode = SDL_GetDesktopDisplayMode( 0 ) ) != nullptr )
	{
		w = displayMode->w;
		h = displayMode->h;
	}
	else
	{
		printf( "Failed to get desktop display mode: %s\n", SDL_GetError() );
		w = 1024;
		h = 768;
	}

	if ( jaded::sys::launchOperations.forcedWidth > 0 ) w = jaded::sys::launchOperations.forcedWidth;
	if ( jaded::sys::launchOperations.forcedHeight > 0 ) h = jaded::sys::launchOperations.forcedHeight;

	sdlWindow = SDL_CreateWindow( "Jaded", w, h, flags );
	if ( sdlWindow == nullptr )
	{
		return nullptr;
	}

	if ( !jaded::sys::launchOperations.forceWindowed )
	{
		SDL_SetWindowFullscreen( sdlWindow, true );
	}

#if defined( USE_SDL_GL_CONTEXT )

	sdlGLContext = SDL_GL_CreateContext( sdlWindow );
	if ( sdlGLContext == nullptr )
		return nullptr;

	SDL_GL_MakeCurrent( sdlWindow, sdlGLContext );

#endif

#if defined( _WIN32 )

	nativeWindowHandle = ( HWND ) SDL_GetPointerProperty( SDL_GetWindowProperties( sdlWindow ), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr );

#endif

	return sdlWindow;
}

static void SetupAspectRatio()
{
	int w, h;
	SDL_GetWindowSizeInPixels( sdlWindow, &w, &h );

	float r = ( ( float ) w / h );
	if ( fabsf( r - ( 4.0f / 3.0f ) ) < fabsf( r - ( 16.0f / 9.0f ) ) )
	{
		MAI_gst_MainHandles.pst_DisplayData->st_ScreenFormat.l_ScreenRatioConst = GDI_Cul_SRC_4over3;
	}
	else
	{
		MAI_gst_MainHandles.pst_DisplayData->st_ScreenFormat.l_ScreenRatioConst = GDI_Cul_SRC_16over9;
	}
}

static void InitializeDisplay()
{
	MAI_gst_MainHandles.h_DisplayWindow = nativeWindowHandle;
	MAI_gst_MainHandles.pst_DisplayData = GDI_fnpst_CreateDisplayData();
	GDI_gpst_CurDD                      = MAI_gst_MainHandles.pst_DisplayData;

	GDI_fnl_InitInterface( &MAI_gst_MainHandles.pst_DisplayData->st_GDI, 1 );

	MAI_gst_MainHandles.pst_DisplayData->pv_SpecificData = MAI_gst_MainHandles.pst_DisplayData->st_GDI.pfnpv_InitDisplay();
	GDI_AttachDisplay( MAI_gst_MainHandles.pst_DisplayData, MAI_gst_MainHandles.h_DisplayWindow );

#ifdef RASTERS_ON

	GDI_Rasters_Init( MAI_gst_MainHandles.pst_DisplayData->pst_Raster, "Display Data" );

#endif

	MAI_gst_MainHandles.pst_DisplayData->uc_EngineCamera = TRUE;
	MAI_gst_MainHandles.pst_DisplayData->ul_DrawMask |= GDI_Cul_DM_NoAutoClone;

	MAI_gst_MainHandles.pst_DisplayData->st_ScreenFormat.ul_Flags = GDI_Cul_SFF_OccupyAll;

	// Determine aspect ratio - TODO: should get triggered again whenever window-size changes...
	SetupAspectRatio();
}

static void ShutdownDisplay()
{
	if ( MAI_gst_MainHandles.pst_DisplayData == nullptr )
		return;

	GDI_DetachDisplay( MAI_gst_MainHandles.pst_DisplayData );
	MAI_gst_MainHandles.pst_DisplayData->st_GDI.pfnv_DesinitDisplay( MAI_gst_MainHandles.pst_DisplayData->pv_SpecificData );
	GDI_fnv_DestroyDisplayData( MAI_gst_MainHandles.pst_DisplayData );
}

#if defined( JADED_USE_WINMAIN_SDL )// hogsy: keep this for now, so if we need to, we can revert to the old crap

#	if defined( _WIN32 )
int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow )
#	else
int main( int argc, char **argv )
#	endif
{
#	if defined( _WIN32 )

	jaded::sys::numLaunchArguments = __argc;
	jaded::sys::launchArguments    = __argv;

	SetUnhandledExceptionFilter( Win32CrashHandler );

#	else

	jaded::sys::numLaunchArguments = argc;
	jaded::sys::launchArguments    = argv;

#	endif

	ParseStartupParameters();

	if ( !SDL_Init( SDL_INIT_GAMEPAD | SDL_INIT_VIDEO ) )
	{
		jaded::sys::AlertBox( "SDL Init fail: " + std::string( SDL_GetError() ),
		                      "Jaded Error",
		                      jaded::sys::ALERT_BOX_ERROR );
		return EXIT_FAILURE;
	}

#	if defined( _WIN32 )

	if ( jaded::sys::launchOperations.debugConsole )
	{
		AllocConsole();
		FILE *tmp;
		freopen_s( &tmp, "CONIN$", "r", stdin );
		freopen_s( &tmp, "CONOUT$", "w", stderr );
		freopen_s( &tmp, "CONOUT$", "w", stdout );
	}

	// hogsy: for now we'll only support editor functionality under win32
	if ( jaded::sys::launchOperations.editorMode )
		return EDI_EditorWin32Execution( hInstance );

#	endif

	if ( CreateSDLWindow() == nullptr )
	{
		jaded::sys::AlertBox( "SDL Window fail: " + std::string( SDL_GetError() ),
		                      "Jaded Error",
		                      jaded::sys::ALERT_BOX_ERROR );
	}

	ImGuiInterface_Initialize( sdlWindow );

	MEMpro_Init();
	MEMpro_StartMemRaster();

	ENG_InitApplication();

	// Default big file name
	const char *projectFile = "Rayman4.bf";
	if ( !jaded::sys::launchOperations.projectFile.empty() )
	{
		projectFile = jaded::sys::launchOperations.projectFile.c_str();
	}
	snprintf( MAI_gst_InitStruct.asz_ProjectName, sizeof( MAI_gst_InitStruct.asz_ProjectName ), "%s", projectFile );
	if ( !BIG_Open( MAI_gst_InitStruct.asz_ProjectName ) )
		return EXIT_FAILURE;

	InitializeDisplay();

	ENG_InitEngine();

	ENG_Loop();

	WOR_Universe_Close( 0 );

	ENG_CloseEngine();
	ENG_CloseApplication();

	ImGuiInterface_Shutdown();

	SDL_DestroyWindow( sdlWindow );

#	if defined( _WIN32 ) && !defined( NDEBUG )

	if ( jaded::sys::launchOperations.debugConsole )
		FreeConsole();

#	endif

	return EXIT_SUCCESS;
}

#endif
