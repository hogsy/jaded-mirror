/*$T OGLinit.c GC! 1.081 09/04/00 14:23:37 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


#include "Precomp.h"

#include "BASe/BAStypes.h"
#include "BASe/MEMory/MEM.h"
#include "TIMer/PROfiler/PROPS2.h"
#include "GDInterface/GDInterface.h"
#include "GDInterface/GDIrequest.h"
#include "GDInterface/GDIrasters.h"
#include "OGLinit.h"
#include "OGLtex.h"
#include "GEOmetric/GEO_STRIP.h"
#include "ENGine/Sources/MoDiFier/MDFmodifier_SPG2.h"

#ifdef ACTIVE_EDITORS
#	include "ENGine/Sources/COLlision/COLvars.h"
#	include "ENGine/Sources/WORld/WORinit.h"
#	include "ENGine/Sources/WORld/WORaccess.h"
#endif

#pragma comment( lib, "opengl32.lib" )
#pragma comment( lib, "glew32s.lib" )

extern "C" u32 Stats_ulNumberOfTRiangles = 0;
extern "C" u32 Stats_ulCallToDrawNb      = 0;

/*$4
 ***********************************************************************************************************************
    constant
 ***********************************************************************************************************************
 */
BOOL OGL_gb_Init                  = 0;
BOOL OGL_gb_DispStrip             = 0;
extern "C" BOOL OGL_gb_DispLOD    = 0;
extern "C" ULONG OGL_ulLODAmbient = 0;

/*$4
 ***********************************************************************************************************************
    Private and external function prototype
 ***********************************************************************************************************************
 */

static bool OGL_SetDCPixelFormat( HDC _hDC, int maxAASamples );
void OGL_SetupRC( OGL_tdst_SpecificData * );

extern void LOA_BeginSpeedMode( BIG_KEY _ul_Key );
extern void LOA_EndSpeedMode( void );
extern "C" BOOL GDI_gb_WaveSprite;
#ifdef ACTIVE_EDITORS
extern "C" COL_tdst_GlobalVars COL_gst_GlobalVars;
#endif

/*$4
 ***********************************************************************************************************************
    Public Function
 ***********************************************************************************************************************
 */

/*
 =======================================================================================================================
    Aim:    Create a device
 =======================================================================================================================
 */
OGL_tdst_SpecificData *OGL_pst_CreateDevice( void )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_tdst_SpecificData *pst_SD;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	pst_SD = ( OGL_tdst_SpecificData * ) MEM_p_Alloc( sizeof( OGL_tdst_SpecificData ) );
	L_memset( pst_SD, 0, sizeof( OGL_tdst_SpecificData ) );

	pst_SD->dst_BumpTex  = ( OGL_tdst_BumpTexture  *) MEM_p_Alloc( sizeof( OGL_tdst_BumpTexture ) * OGL_C_BumpTexGran );
	pst_SD->l_MaxBumpTex = 32;

	return pst_SD;
}

/*
 =======================================================================================================================
    Aim:    Destroy a device
 =======================================================================================================================
 */
void OGL_DestroyDevice( void *_pst_SD )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_tdst_SpecificData *pst_SD;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	pst_SD = ( OGL_tdst_SpecificData * ) _pst_SD;
	if ( pst_SD->dst_BumpTex )
		MEM_Free( pst_SD->dst_BumpTex );
	if ( pst_SD ) MEM_Free( pst_SD );
}

/*
 =======================================================================================================================
    Aim:    Close OpenGL display
 =======================================================================================================================
 */
HRESULT OGL_l_Close( GDI_tdst_DisplayData *_pst_DD )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_tdst_SpecificData *pst_SD;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	OGL_gb_Init = 0;

	pst_SD = ( OGL_tdst_SpecificData * ) _pst_DD->pv_SpecificData;

	if ( pst_SD->h_DC )
	{
		wglMakeCurrent( pst_SD->h_DC, NULL );
		pst_SD->h_DC = NULL;
	}

	if ( pst_SD->h_RC )
	{
		wglDeleteContext( pst_SD->h_RC );
		pst_SD->h_RC = NULL;
	}

	return S_OK;
}

/* Aim: Init OpenGL object */
/*
 =======================================================================================================================
 =======================================================================================================================
 */
static bool CreateFakeContext( int *maxAASamples );
void OGL_InitAllShadows( void );
LONG OGL_l_Init( HWND _hWnd, GDI_tdst_DisplayData *_pst_DD )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	RECT st_Rect;
	OGL_tdst_SpecificData *pst_SD;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


	/* Cleanup any objects that might've been created before */
	if ( OGL_l_Close( _pst_DD ) != S_OK ) return E_FAIL;

	OGL_gb_Init = 1;

	/* Check window size */
	GetClientRect( _hWnd, &st_Rect );
	if ( ( st_Rect.top >= st_Rect.bottom ) || ( st_Rect.left >= st_Rect.right ) ) return S_OK;

	pst_SD                 = ( OGL_tdst_SpecificData                 *) _pst_DD->pv_SpecificData;
	pst_SD->h_Wnd          = _hWnd;
	pst_SD->h_DC           = GetDC( _hWnd );
	pst_SD->pst_ProjMatrix = &_pst_DD->st_Camera.st_ProjectionMatrix;

	GetClientRect( _hWnd, &pst_SD->rcViewportRect );

	int maxAASamples;
	if ( !CreateFakeContext( &maxAASamples ) )
		return S_FALSE;

	/* Select the pixel format */
	if ( !OGL_SetDCPixelFormat( pst_SD->h_DC, maxAASamples ) )
	{
		MessageBox( NULL, "Failed to set OpenGL pixel format!", "OpenGL warning", MB_OK | MB_ICONWARNING | MB_TASKMODAL );
		return S_FALSE;
	}

	pst_SD->h_RC = wglCreateContext( pst_SD->h_DC );
	wglMakeCurrent( pst_SD->h_DC, pst_SD->h_RC );

	if ( GLEW_EXT_compiled_vertex_array )
	{
		OGL_CALL( glEnableClientState( GL_VERTEX_ARRAY ) );
	}

	OGL_SetupRC( pst_SD );
	OGL_RS_Init( &pst_SD->st_RS );

	_pst_DD->GlobalMul2X          = TRUE;
	_pst_DD->GlobalMul2XFactor    = 1.f;
	_pst_DD->ShowAEInEngine       = TRUE;
	_pst_DD->ShowAEEditor         = TRUE;
	_pst_DD->DrawGraphicDebugInfo = FALSE;
	_pst_DD->TRI_ALarm            = 500000;
	_pst_DD->SPG_ALarm            = 20000;
	_pst_DD->DRAW_ALarm           = 100;
	_pst_DD->OBJ_ALarm            = 300;
	_pst_DD->SMALL_ALarm          = 0;// Inactive by default.
	_pst_DD->ColorCostIAThresh    = 4;
	//_pst_DD->b_AntiAliasingBlur   = true;

	OGL_InitAllShadows();

	return S_OK;
}

/*
 =======================================================================================================================
    Aim:    Adapt OpenGL display to window
 =======================================================================================================================
 */
extern void OGL_AE_Radapt();
HRESULT OGL_l_ReadaptDisplay( HWND _hWnd, GDI_tdst_DisplayData *_pst_DD )
{
	HRESULT h;
	OGL_AE_Radapt();


	if ( OGL_gb_Init )
	{
		/*~~~~~*/
		int w, h;
		/*~~~~~*/

		w = _pst_DD->st_Device.l_Width;
		h = _pst_DD->st_Device.l_Height;
		OGL_CALL( glViewport( 0, ( h - w ) / 2, w, w ) );
		return 1;
	}

	h = OGL_l_Init( _hWnd ? _hWnd : OGL_M_SD( _pst_DD )->h_Wnd, _pst_DD );
	return h;
}

/*
 =======================================================================================================================
    Aim:    Flip buffer
 =======================================================================================================================
 */
#ifdef ACTIVE_EDITORS
extern "C"
{
	ULONG SPG2_PrimitivCounter = 0;
	ULONG SPG2_PrimitivLimit   = 100000;
};
#endif


void OGL_Flip()
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_tdst_SpecificData *pst_SD;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	extern void OGL_AE_BeforeFlip();
	OGL_AE_BeforeFlip();

	pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;

	OGL_CALL( glFinish() );
	PRO_StartTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Flip );

	SwapBuffers( pst_SD->h_DC );

	PRO_StopTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Flip );
}

/*
=======================================================================================================================
Aim:    Clear buffer
=======================================================================================================================
*/
void OGL_Clear( LONG _l_Buffer, ULONG _ul_Color )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	LONG l_Buffer;
	OGL_tdst_SpecificData *pst_SD;
	unsigned char *color;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	extern void OGL_AE_BeforeClear();
	OGL_AE_BeforeClear();

#ifdef ACTIVE_EDITORS
	SPG2_PrimitivCounter = 0;
#endif

	pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;
	wglMakeCurrent( pst_SD->h_DC, pst_SD->h_RC );

#ifdef ACTIVE_EDITORS
	if ( ( GDI_gpst_CurDD->ul_WiredMode & 3 ) == 2 ) _ul_Color = 0;
	if ( GDI_gpst_CurDD->ShowHistogramm ) _ul_Color = 0;
#endif


	if ( GDI_gpst_CurDD->GlobalMul2X )
	{
		_ul_Color &= 0xFEFEFEFE;
		_ul_Color >>= 1;
	}
	color = ( unsigned char * ) &_ul_Color;

	OGL_CALL( glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ) );
	OGL_CALL( glClearColor( ( ( float ) color[ 0 ] ) / 255.0f, ( ( float ) color[ 1 ] ) / 255.0f, ( ( float ) color[ 2 ] ) / 255.0f, 0.0f ) );

	OGL_RS_DepthMask( &pst_SD->st_RS, 1 );
	l_Buffer = ( ( _l_Buffer & GDI_Cl_ColorBuffer ) ? GL_COLOR_BUFFER_BIT : 0 ) | ( ( _l_Buffer & GDI_Cl_ZBuffer ) ? GL_DEPTH_BUFFER_BIT : 0 );

	PRO_StartTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Clear );
	//glEnable(GL_SCISSOR_TEST);
	OGL_CALL( glClear( l_Buffer ) );
	//glDisable(GL_SCISSOR_TEST);

	pst_SD->st_RS.l_LastTexture = -2;

	PRO_StopTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Clear );

	// gross place to shove this, but hey ho! ~hogsy
	if ( GDI_gpst_CurDD->b_AntiAliasingBlur )
		glEnable( GL_MULTISAMPLE );
	else
		glDisable( GL_MULTISAMPLE );
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
void OGL_SetViewMatrix( MATH_tdst_Matrix *_pst_Matrix )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	MATH_tdst_Matrix st_OGLMatrix;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_CALL( glMatrixMode( GL_MODELVIEW ) );

	/*
	 * YLT,if there is scale in the matrix, we need to put the scale (last column)
	 * into the transformation matrix before sending to open gl
	 */
	if ( MATH_b_TestScaleType( _pst_Matrix ) )
	{
		MATH_MakeOGLMatrix( &st_OGLMatrix, _pst_Matrix );
		OGL_CALL( glLoadMatrixf( ( float * ) &st_OGLMatrix ) );
	}
	else
	{
		OGL_CALL( glLoadMatrixf( ( float * ) _pst_Matrix ) );
	}
}

extern "C" ULONG g_ul_BIG_SNAPSHOT_COUNTER;

/*
 =======================================================================================================================
    Aim:    Send View matrix for render
 =======================================================================================================================
 */
void OGL_SetProjectionMatrix( CAM_tdst_Camera *_pst_Cam )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_tdst_SpecificData *pst_SD;
	float f, f_ScreenRatio;
	LONG w, h, W, H;
	LONG x, y;
	ULONG Flags;
	float l, r, t, b;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;

	/* Compute screen ratio */
	if ( !( _pst_Cam->ul_Flags & CAM_Cul_Flags_Perspective ) )
		f_ScreenRatio = GDI_gaf_ScreenRation[ GDI_Cul_SRC_Square ];
	else
	{
		x = GDI_gpst_CurDD->st_ScreenFormat.l_ScreenRatioConst;
		if ( ( x <= 0 ) || ( x >= GDI_Cul_SRC_Number ) )
			f_ScreenRatio = GDI_gpst_CurDD->st_ScreenFormat.f_ScreenYoverX;
		else
			f_ScreenRatio = GDI_gaf_ScreenRation[ x ];
	}

	f = _pst_Cam->f_YoverX * GDI_gpst_CurDD->st_ScreenFormat.f_PixelYoverX * f_ScreenRatio;
#ifdef ACTIVE_EDITORS
	if ( WOR_gpst_CurrentWorld && WOR_gpst_CurrentWorld->b_IsSplitScreen ) f /= 2;
#endif
	Flags = GDI_gpst_CurDD->st_ScreenFormat.ul_Flags;

	/* Compute height and width of screen */
	w = GDI_gpst_CurDD->st_Device.l_Width;
	h = GDI_gpst_CurDD->st_Device.l_Height;

	if ( Flags & GDI_Cul_SFF_ReferenceIsY )
	{
		H = h;
		W = ( LONG ) ( h / f );

		if ( ( ( Flags & GDI_Cul_SFF_CropToWindow ) && ( W > w ) ) || ( ( Flags & GDI_Cul_SFF_OccupyAll ) && ( W < w ) ) )
		{
			H = ( LONG ) ( w * f );
			W = w;
		}
	}
	else
	{
		H = ( LONG ) ( w * f );
		W = w;

		if ( ( ( Flags & GDI_Cul_SFF_CropToWindow ) && ( H > h ) ) || ( ( Flags & GDI_Cul_SFF_OccupyAll ) && ( H < h ) ) )
		{
			W = ( LONG ) ( h / f );
			H = h;
		}
	}

	if ( _pst_Cam->f_ViewportWidth == 0 )
	{
		_pst_Cam->f_ViewportWidth  = 1.0f;
		_pst_Cam->f_ViewportHeight = 1.0f;
		_pst_Cam->f_ViewportLeft   = 0.0f;
		_pst_Cam->f_ViewportTop    = 0.0f;
	}

	x = ( int ) ( _pst_Cam->f_ViewportLeft * W + ( ( w - W ) / 2 ) );
	y = ( int ) ( _pst_Cam->f_ViewportTop * H + ( ( h - H ) / 2 ) );
	h = ( int ) ( _pst_Cam->f_ViewportHeight * H );
	w = ( int ) ( _pst_Cam->f_ViewportWidth * W );

	_pst_Cam->l_ViewportRealLeft = x;
	_pst_Cam->l_ViewportRealTop  = y;

#ifdef ACTIVE_EDITORS
	wglMakeCurrent( pst_SD->h_DC, pst_SD->h_RC );
#endif
	OGL_CALL( glMatrixMode( GL_PROJECTION ) );

	if ( _pst_Cam->ul_Flags & CAM_Cul_Flags_Ortho )
	{
		glLoadIdentity();
		if ( _pst_Cam->ul_Flags & CAM_Cul_Flags_OrthoYInvert )
			glOrtho( 0, 1, 1, 0, -1.0f, 1.0f );
		else
			glOrtho( 0, w, 0, h, -1.0f, 1.0f );
		return;
	}

	if ( _pst_Cam->ul_Flags & CAM_Cul_Flags_Perspective )
	{
		f = 1.0f / fNormalTan( _pst_Cam->f_FieldOfVision / 2 );
		MATH_SetIdentityMatrix( pst_SD->pst_ProjMatrix );

		if ( GDI_gpst_CurDD->st_ScreenFormat.ul_Flags & GDI_Cul_SFF_ReferenceIsY )
		{
			pst_SD->pst_ProjMatrix->Ix = f * f_ScreenRatio;
			pst_SD->pst_ProjMatrix->Jy = -f;
		}
		else
		{
			pst_SD->pst_ProjMatrix->Ix = f;
			pst_SD->pst_ProjMatrix->Jy = -f / f_ScreenRatio;
		}

		pst_SD->pst_ProjMatrix->Kz  = ( _pst_Cam->f_FarPlane + _pst_Cam->f_NearPlane ) / ( _pst_Cam->f_FarPlane - _pst_Cam->f_NearPlane );
		pst_SD->pst_ProjMatrix->Sz  = 1.0f;
		pst_SD->pst_ProjMatrix->T.z = -0.1f * _pst_Cam->f_NearPlane;
		pst_SD->pst_ProjMatrix->w   = 0.0f;// MATRIX W!

		glLoadMatrixf( ( float * ) pst_SD->pst_ProjMatrix );
	}
	else
	{
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
		float f_IsoFactorZoom, f_Scale;
		/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

		MATH_SetIdentityMatrix( pst_SD->pst_ProjMatrix );
		glLoadMatrixf( ( float * ) pst_SD->pst_ProjMatrix );

		f_IsoFactorZoom = _pst_Cam->f_IsoFactor * _pst_Cam->f_IsoZoom;
		f_Scale         = f_IsoFactorZoom;

		if ( GDI_gpst_CurDD->st_ScreenFormat.ul_Flags & GDI_Cul_SFF_ReferenceIsY )
		{
			f = 1 / GDI_gpst_CurDD->st_ScreenFormat.f_ScreenYoverX;

			if ( _pst_Cam->f_IsoFactor == 0 )
			{
				OGL_CALL( glOrtho( ( 1 - f ) / 2, ( 1 + f ) / 2, 0, 1, -_pst_Cam->f_NearPlane, -_pst_Cam->f_FarPlane ) );
			}
			else
			{
				l = f_Scale * ( -f );
				r = f_Scale * ( f );
				b = f_Scale * -1;
				t = f_Scale * 1;

				//glOrtho(l, r, b, t, (_pst_Cam->f_NearPlane + _pst_Cam->f_IsoFactor), -_pst_Cam->f_FarPlane);
				OGL_CALL( glOrtho( l, r, b, t, _pst_Cam->f_FarPlane, -_pst_Cam->f_FarPlane ) );
			}
		}
		else
		{
			f = GDI_gpst_CurDD->st_ScreenFormat.f_ScreenYoverX;
			if ( _pst_Cam->f_IsoFactor == 0 )
				glOrtho( 0, 1, ( 1 - f ) / 2, ( 1 + f ) / 2, -_pst_Cam->f_NearPlane, -_pst_Cam->f_FarPlane );
			else
			{
				t = f_Scale * ( -f );
				b = f_Scale * ( f );
				l = f_Scale * -1;
				r = f_Scale * 1;

				//glOrtho(l, r, b, t, (_pst_Cam->f_NearPlane + _pst_Cam->f_IsoFactor), -_pst_Cam->f_FarPlane);
				glOrtho( l, r, b, t, _pst_Cam->f_FarPlane, -_pst_Cam->f_FarPlane );
			}
		}
	}
#ifdef ACTIVE_EDITORS
	{
		w &= ~3;

		GDI_gpst_CurDD->st_Device.Vx = x;
		GDI_gpst_CurDD->st_Device.Vy = y;
		GDI_gpst_CurDD->st_Device.Vw = w;
		GDI_gpst_CurDD->st_Device.Vh = h;
		OGL_CALL( glScissor( x, y ? y - 1 : 0, w + 2, h + 2 ) );

		if ( g_ul_BIG_SNAPSHOT_COUNTER )
		{
			w <<= 2;
			h <<= 2;
			x -= ( ( g_ul_BIG_SNAPSHOT_COUNTER >> 2 ) & 3 ) * ( w >> 2 );
			y -= ( g_ul_BIG_SNAPSHOT_COUNTER & 3 ) * ( h >> 2 );
		}
		OGL_CALL( glViewport( x, y, w, h ) );
	}
#else
	OGL_CALL( glScissor( x, y, w, h ) );
	OGL_CALL( glViewport( x, y, w, h ) );
#endif
}


#define SHADOW_TEX_MAX        16
#define SHADOW_TEX_RESOLUTION 512

static GLuint SHADOW_TEX_HANDLE[ SHADOW_TEX_MAX + 1 ];
static GLclampf SHADOW_TEX_Priority[ SHADOW_TEX_MAX + 1 ];
static u32 Sdw_Tex_IsInit = 0;
void OGL_ShadowBindTexture( u32 Value );
void OGL_ShadowConstructor()
{
	for ( u32 Counter = 0; Counter < SHADOW_TEX_MAX + 1; Counter++ )
	{
		// Check we actually have valid texture handles before trying to clear them,
		// probably safe just to check first slot ~hogsy
		if ( SHADOW_TEX_HANDLE[ Counter ] != 0 )
		{
			OGL_CALL( glDeleteTextures( SHADOW_TEX_MAX + 1, SHADOW_TEX_HANDLE ) );
		}
		SHADOW_TEX_HANDLE[ Counter ] = 0xc0de2004;
	}
	Sdw_Tex_IsInit = 1;
}

static u32 CurrentShadowTexture = 0;
void OGL_ShadowBindTexture( u32 Value )
{
	Value &= 0xffff;
	if ( SHADOW_TEX_MAX != Value )
	{
		Value = ( Value & ( SHADOW_TEX_MAX - 1 ) );
	}

	OGL_CALL( glEnable( GL_TEXTURE_2D ) );

	u32 Dummy;
	if ( ( SHADOW_TEX_HANDLE[ Value ] == 0xc0de2004 ) || !glIsTexture( SHADOW_TEX_HANDLE[ Value ] ) || !glAreTexturesResident( 1, &SHADOW_TEX_HANDLE[ Value ], ( GLboolean * ) &Dummy ) )
	{
		if ( glIsTexture( SHADOW_TEX_HANDLE[ Value ] ) )
		{
			OGL_CALL( glDeleteTextures( 1, &SHADOW_TEX_HANDLE[ Value ] ) );
		}
		OGL_CALL( glGenTextures( 1, &SHADOW_TEX_HANDLE[ Value ] ) );
		SHADOW_TEX_Priority[ Value ] = 1.0f;
		OGL_CALL( glBindTexture( GL_TEXTURE_2D, SHADOW_TEX_HANDLE[ Value ] ) );
		OGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR ) );
		OGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR ) );
		OGL_CALL( glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, SHADOW_TEX_RESOLUTION, SHADOW_TEX_RESOLUTION, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL ) );
		OGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE ) );
		OGL_CALL( glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ) );
	}
	OGL_CALL( glBindTexture( GL_TEXTURE_2D, SHADOW_TEX_HANDLE[ Value ] ) );
}

void OGL_ShadowImgSave( u32 TexNum )
{
	OGL_ShadowBindTexture( TexNum );
	//	glCopyTexSubImage2D(GL_TEXTURE_2D, 0 ,  0 , 0 , 0 , 0 , SHADOW_TEX_RESOLUTION, SHADOW_TEX_RESOLUTION);//*/
	OGL_CALL( glCopyTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, SHADOW_TEX_RESOLUTION, SHADOW_TEX_RESOLUTION, 0 ) );
}

void OGL_ShadowSelect( u32 TexNum )
{
	TexNum &= 0xffff;
	if ( SHADOW_TEX_MAX != TexNum )
	{
		TexNum = ( TexNum & ( SHADOW_TEX_MAX - 1 ) );
	}

	if ( ( SHADOW_TEX_HANDLE[ TexNum ] != 0xc0de2004 ) && glIsTexture( SHADOW_TEX_HANDLE[ TexNum ] ) )
	{
		OGL_ShadowBindTexture( TexNum );
	}
}

void OGL_ShadowImgLoad( u32 TexNum )
{
	u32 Color;
	MATH_tdst_Matrix st_SavedMatrix;
	Color = 0xffffff;
	OGL_ShadowBindTexture( TexNum );
	glMatrixMode( GL_MODELVIEW );
	glGetFloatv( GL_MODELVIEW_MATRIX, ( GLfloat * ) &st_SavedMatrix );
	glLoadIdentity();
	if ( GDI_gpst_CurDD != NULL )
	{
		GDI_gpst_CurDD->profilingInformation.numBatches++;
	}
	glBegin( GL_POLYGON );
	glColor4ubv( ( GLubyte * ) &Color );
	glTexCoord2f( 0, 0 );
	glVertex3f( -1, 1, 0.0 );
	glTexCoord2f( 1, 0 );
	glVertex3f( 1, 1, 0.0 );
	glTexCoord2f( 1, 1 );
	glVertex3f( 1, -1, 0.0 );
	glTexCoord2f( 0, 1 );
	glVertex3f( -1, -1, 0.0 );
	glEnd();
	glMatrixMode( GL_MODELVIEW );
	glLoadMatrixf( ( float * ) &st_SavedMatrix );
	OGL_CALL( glDisable( GL_TEXTURE_2D ) );
}

extern void OGL_ES_PushState();
extern void OGL_ES_PopState();
void OGL_ShadowBegin( u32 TexNum )
{
	OGL_ES_PushState();
	CurrentShadowTexture = TexNum;
	/* 1 Save frame buffer */
	OGL_ShadowImgSave( SHADOW_TEX_MAX );
	/* 2 retore tex num */
	OGL_ShadowImgLoad( TexNum );
}

void OGL_ShadowEnd()
{
	/* 1 Save tex num */
	OGL_ShadowImgSave( CurrentShadowTexture );
	/* 2 restore frame buffer */
	OGL_ShadowImgLoad( SHADOW_TEX_MAX );
	OGL_ES_PopState();
}

void OGL_InitAllShadows()
{
	OGL_ShadowConstructor();

	u32 Counter = SHADOW_TEX_MAX + 1;
	while ( Counter-- )
	{
		OGL_ShadowBegin( Counter );
		OGL_ShadowEnd();
	}//*/
}

static u32 ProjectionisNotRestored;
static MATH_tdst_Matrix st_SavedProjection;
static GLint st_SavedViewport[ 4 ];
static GLint st_SavedScissor[ 4 ];
void OGL_SetTextureTarget( ULONG Num, ULONG Clear )
{
	if ( Num != 0xffffffff )
	{
		OGL_ShadowBegin( Num );
		if ( Clear )
		{
			u32 ulColor;
			ulColor = 0x0;
			GDI_gpst_CurDD->profilingInformation.numBatches++;
			OGL_CALL( glBegin( GL_POLYGON ) );
			glColor4ubv( ( GLubyte * ) &ulColor );
			glVertex3f( 10000, 10000, 0 );
			glVertex3f( -10000, 10000, 0 );
			glVertex3f( -10000, -10000, 0 );
			glVertex3f( 10000, -10000, 0 );
			glEnd();
		}
	}
	else
	{
		/* restore original frame buffer */
		OGL_ShadowEnd();
		//		if (ProjectionisNotRestored)
		{
			glMatrixMode( GL_PROJECTION );
			glLoadMatrixf( ( GLfloat * ) &st_SavedProjection );
			ProjectionisNotRestored = 0;
			glViewport( st_SavedViewport[ 0 ], st_SavedViewport[ 1 ], st_SavedViewport[ 2 ], st_SavedViewport[ 3 ] );
			glScissor( st_SavedScissor[ 0 ], st_SavedScissor[ 1 ], st_SavedScissor[ 2 ], st_SavedScissor[ 3 ] );
		}//*/
	}
}
void OGL_SetViewMatrix_SDW( MATH_tdst_Matrix *_pst_Matrix, float *Limits )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	MATH_tdst_Matrix st_OGLMatrix;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	MATH_SetIdentityMatrix( &st_OGLMatrix );

	ProjectionisNotRestored = 1;
	OGL_CALL( glLoadMatrixf( ( GLfloat * ) &st_OGLMatrix ) );

	glMatrixMode( GL_PROJECTION );
	glGetFloatv( GL_PROJECTION_MATRIX, ( GLfloat * ) &st_SavedProjection );
	OGL_CALL( glLoadIdentity() );
	OGL_CALL( glOrtho( -1.0, 1.0, 1.0, -1.0f, -1.0, 1.0 ) );

	OGL_CALL( glMatrixMode( GL_MODELVIEW ) );
	{
		MATH_MakeOGLMatrix( &st_OGLMatrix, _pst_Matrix );
		st_OGLMatrix.Ix *= 1.f;
		st_OGLMatrix.Jy *= 1.f;
		st_OGLMatrix.Kz  = 0.0f;
		st_OGLMatrix.T.z = 0.99999f;
		OGL_CALL( glLoadMatrixf( ( float * ) &st_OGLMatrix ) );
	}

	OGL_CALL( glGetIntegerv( GL_VIEWPORT, st_SavedViewport ) );
	OGL_CALL( glGetIntegerv( GL_SCISSOR_BOX, st_SavedScissor ) );
	OGL_CALL( glViewport( 0, 0, SHADOW_TEX_RESOLUTION, SHADOW_TEX_RESOLUTION ) );
	OGL_CALL( glScissor( 0, 0, SHADOW_TEX_RESOLUTION, SHADOW_TEX_RESOLUTION ) );
}

/*
 =======================================================================================================================
    Aim:    Draw indexed triangles
 =======================================================================================================================
 */
#define OGL_SetColorRGBABasic( a )                     \
	if ( pst_Color )                                   \
	{                                                  \
		ulOGLSetCol = pst_Color[ a ] | ulOGLSetCol_Or; \
		ulOGLSetCol ^= ulOGLSetCol_XOr;                \
		if ( pst_Alpha )                               \
		{                                              \
			ulOGLSetCol &= 0x00ffffff;                 \
			ulOGLSetCol |= pst_Alpha[ a ];             \
		}                                              \
		glColor4ubv( ( GLubyte * ) &ulOGLSetCol );     \
	}                                                  \
	else if ( pst_Alpha )                              \
	{                                                  \
		ulOGLSetCol &= 0x00ffffff;                     \
		ulOGLSetCol |= pst_Alpha[ a ];                 \
		glColor4ubv( ( GLubyte * ) &ulOGLSetCol );     \
	}

#ifdef ACTIVE_EDITORS
#	define OGL_SetColorRGBA( a )                                                                                                                                \
		if ( IRSVDoit == 2 )                                                                                                                                     \
		{                                                                                                                                                        \
			ULONG ulColor;                                                                                                                                       \
			if ( pst_Color )                                                                                                                                     \
			{                                                                                                                                                    \
				ulColor = pst_Color[ a ] | ulOGLSetCol_Or;                                                                                                       \
				ulColor ^= ulOGLSetCol_XOr;                                                                                                                      \
			}                                                                                                                                                    \
			else                                                                                                                                                 \
			{                                                                                                                                                    \
				ulColor = ulOGLSetCol;                                                                                                                           \
			}                                                                                                                                                    \
			ulColor = ulSpecialColor + ( ulColor & 0xFF000000 ) + ( ( ulColor & 0xFE ) >> 1 ) + ( ( ulColor & 0xFE00 ) >> 1 ) + ( ( ulColor & 0xFE0000 ) >> 1 ); \
			glColor4ubv( ( GLubyte * ) &ulColor );                                                                                                               \
		}                                                                                                                                                        \
		else if ( IRSVDoit == 4 )                                                                                                                                \
		{                                                                                                                                                        \
			ULONG ulColor;                                                                                                                                       \
			if ( pst_Color )                                                                                                                                     \
			{                                                                                                                                                    \
				ulColor = pst_Color[ a ] | ulOGLSetCol_Or;                                                                                                       \
				ulColor ^= ulOGLSetCol_XOr;                                                                                                                      \
			}                                                                                                                                                    \
			else                                                                                                                                                 \
			{                                                                                                                                                    \
				ulColor = ulOGLSetCol;                                                                                                                           \
			}                                                                                                                                                    \
			ulColor = ( ( ( ULONG ) ( ( 1.f - fRedCoef ) * ( float ) ( ( ulColor & 0xFF0000 ) >> 16 ) ) ) << 16 ) +                                              \
			          ( ( ( ULONG ) ( ( 1.f - fRedCoef ) * ( float ) ( ( ulColor & 0xFF00 ) >> 8 ) ) ) << 8 ) +                                                  \
			          ( ( ( ULONG ) ( 255.f * fRedCoef + ( 1.f - fRedCoef ) * ( float ) ( ( ulColor & 0xFF ) ) ) ) ) +                                           \
			          ( ulColor & 0xFF000000 );                                                                                                                  \
			glColor4ubv( ( GLubyte * ) &ulColor );                                                                                                               \
		}                                                                                                                                                        \
		else                                                                                                                                                     \
			OGL_SetColorRGBABasic( a )
#else//ACTIVE_EDITORS
#	define OGL_SetColorRGBA( a ) OGL_SetColorRGBABasic( a )
#endif//ACTIVE_EDITORS

#ifdef ACTIVE_EDITORS
#	define OGL_TestHideTriangle( t ) if ( !( ( t )->ul_MaxFlags & 0x80000000 ) )
#else
#	define OGL_TestHideTriangle( t )
#endif

extern "C" ULONG GEO_ulCurrentTriangleNb;
extern "C" ULONG GAODisplayFlag;

#ifdef ACTIVE_EDITORS

#	define OGL_RenderSlopeVars                             \
		MATH_tdst_Vector IRSV0, IRSV1, IRSVN, st_CameraDir; \
		float IRSVcos, IRSVh;                               \
		float fRedCoef = 0.0f;                              \
		int IRSVDoit;                                       \
		ULONG ulSpecialColor;

#	ifdef JADEFUSION//POPOWARNING A TERMINER
#		define OGL_InitRenderSlope                                              \
			if ( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_ShowSlope )         \
			{                                                                    \
				IRSVcos = COL_gst_GlobalVars.f_WallCosAngle;                     \
				IRSVcos *= IRSVcos;                                              \
				IRSVDoit  = 1;                                                   \
				pst_Color = NULL;                                                \
				pst_Alpha = NULL;                                                \
			}                                                                    \
			else if ( GDI_gpst_CurDD_SPR.SMALL_ALarm > GEO_ulCurrentTriangleNb ) \
			{                                                                    \
				IRSVDoit = 4;                                                    \
			}                                                                    \
			else                                                                 \
				IRSVDoit = 0;
#	else//JADEFUSION
#		define OGL_InitRenderSlope                                                                        \
			if ( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_ShowSlope )                                   \
			{                                                                                              \
				IRSVcos = COL_gst_GlobalVars.f_WallCosAngle;                                               \
				IRSVcos *= IRSVcos;                                                                        \
				IRSVDoit  = 1;                                                                             \
				pst_Color = NULL;                                                                          \
				pst_Alpha = NULL;                                                                          \
			}                                                                                              \
			else if ( !( GDI_gpst_CurDD->ul_DrawMask & GDI_Cul_DM_NoFacetMode ) )                          \
			{                                                                                              \
				MATH_tdst_Matrix stCurrentInverted;                                                        \
				MATH_tdst_Vector stLocal;                                                                  \
				IRSVDoit = 2;                                                                              \
				MATH_InvertMatrix( &stCurrentInverted, GDI_gpst_CurDD->st_MatrixStack.pst_CurrentMatrix ); \
				MATH_InitVector( &stLocal, 0.0f, 0.0f, 1.0f );                                             \
				MATH_TransformVector( &st_CameraDir, &stCurrentInverted, &stLocal );                       \
			}                                                                                              \
			else if ( !( GDI_gpst_CurDD->ul_DrawMask & GDI_Cul_DM_NoShowRLIPlaceMode ) )                   \
			{                                                                                              \
				MATH_tdst_Matrix stCurrentInverted;                                                        \
				MATH_tdst_Vector stLocal;                                                                  \
				IRSVDoit  = 3;                                                                             \
				pst_Color = NULL;                                                                          \
				MATH_InvertMatrix( &stCurrentInverted, GDI_gpst_CurDD->st_MatrixStack.pst_CurrentMatrix ); \
				MATH_InitVector( &stLocal, 0.0f, 0.0f, 1.0f );                                             \
				MATH_TransformVector( &st_CameraDir, &stCurrentInverted, &stLocal );                       \
			}                                                                                              \
			else if ( GDI_gpst_CurDD_SPR.SMALL_ALarm > GEO_ulCurrentTriangleNb )                           \
			{                                                                                              \
				IRSVDoit = 4;                                                                              \
			}                                                                                              \
			else                                                                                           \
				IRSVDoit = 0;
#	endif JADEFUSION

// IRSVDoit -> activate special color mode.
// IRSVDoit==1 : Shows walls and ground
// IRSVDoit==2 : Sends a light from the camera to see the triangles (mix 50% normal color and 50% lighted color)
// IRSVDoit==3 : Shows if RLI in GAO, GRO or LOD
// IRSVDoit==4 : Shows in red objects with small number of triangles.

#	define OGL_RenderSlope( t )                                                                                            \
		if ( IRSVDoit )                                                                                                     \
		{                                                                                                                   \
			MATH_SubVector( &IRSV0, &_pst_Point[ t->auw_Index[ 1 ] ], &_pst_Point[ t->auw_Index[ 0 ] ] );                   \
			MATH_SubVector( &IRSV1, &_pst_Point[ t->auw_Index[ 2 ] ], &_pst_Point[ t->auw_Index[ 0 ] ] );                   \
			MATH_CrossProduct( &IRSVN, &IRSV0, &IRSV1 );                                                                    \
			if ( IRSVDoit == 1 )                                                                                            \
			{                                                                                                               \
				IRSVh = ( IRSVN.z * IRSVN.z );                                                                              \
				IRSVh = IRSVh / ( ( IRSVN.x * IRSVN.x ) + ( IRSVN.y * IRSVN.y ) + IRSVh );                                  \
				if ( ( ( IRSVN.z < 0 ) && ( IRSVh < 0.64 ) ) || ( ( IRSVN.z >= 0 ) && ( IRSVh < IRSVcos ) ) )               \
					ulSpecialColor = 0x00808000;                                                                            \
				else                                                                                                        \
					ulSpecialColor = 0x00003380;                                                                            \
				glColor4ubv( ( GLubyte * ) &ulSpecialColor );                                                               \
			}                                                                                                               \
			else if ( IRSVDoit == 2 )                                                                                       \
			{                                                                                                               \
				float fColor;                                                                                               \
				ULONG ulColor;                                                                                              \
				MATH_NormalizeAnyVector( &IRSVN, &IRSVN );                                                                  \
				fColor         = 127.f * MATH_f_FloatLimit( fAbs( MATH_f_DotProduct( &IRSVN, &st_CameraDir ) ), 0.f, 1.f ); \
				ulColor        = ( ULONG ) fColor;                                                                          \
				ulSpecialColor = ( ulColor << 16 ) + ( ulColor << 8 ) + ( ulColor );                                        \
			}                                                                                                               \
			else if ( IRSVDoit == 4 )                                                                                       \
			{                                                                                                               \
				fRedCoef = ( float ) GEO_ulCurrentTriangleNb / ( float ) GDI_gpst_CurDD_SPR.SMALL_ALarm;                    \
			}                                                                                                               \
			else                                                                                                            \
			{                                                                                                               \
				ulSpecialColor &= 0xff000000;                                                                               \
				ulSpecialColor |= GAODisplayFlag;                                                                           \
				glColor4ubv( ( GLubyte * ) &ulSpecialColor );                                                               \
			}                                                                                                               \
		}


/*
Version du mode facet avec lumi�re omni provenant de la cam�ra (pas terrible)
#define OGL_RenderSlopeVars\
	MATH_tdst_Vector	IRSV0, IRSV1, IRSVN;\
    MATH_tdst_Vector    st_CameraPos;\
	float				IRSVcos, IRSVh;\
	int					IRSVDoit;\

#define OGL_InitRenderSlope\
	if ( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_ShowSlope )\
	{\
        IRSVcos = COL_gst_GlobalVars.f_WallCosAngle;\
		IRSVcos *= IRSVcos;\
		IRSVDoit = 1;\
		pst_Color = NULL;\
		pst_Alpha = NULL;\
	}\
	else if (!(GDI_gpst_CurDD->ul_DrawMask & GDI_Cul_DM_NoFacetMode))\
    {\
    	MATH_tdst_Matrix stCurrentInverted;\
        IRSVDoit = 2;\
		pst_Color = NULL;\
    	MATH_InvertMatrix(&stCurrentInverted, GDI_gpst_CurDD->st_MatrixStack.pst_CurrentMatrix);\
	    MATH_InitVector(&st_CameraPos , stCurrentInverted.T.x,stCurrentInverted.T.y,stCurrentInverted.T.z);\
    }\
    else\
		IRSVDoit = 0;
	
#define OGL_RenderSlope(t)\
	if (IRSVDoit)\
	{\
    	MATH_SubVector( &IRSV0, &_pst_Point[t->auw_Index[1]], &_pst_Point[t->auw_Index[0]] );\
	    MATH_SubVector( &IRSV1, &_pst_Point[t->auw_Index[2]], &_pst_Point[t->auw_Index[0]] );\
	    MATH_CrossProduct( &IRSVN, &IRSV0, &IRSV1 );\
        if (IRSVDoit == 1)\
        {\
    		IRSVh = (IRSVN.z * IRSVN.z);\
	       	IRSVh = IRSVh / ((IRSVN.x * IRSVN.x) + (IRSVN.y * IRSVN.y ) + IRSVh );\
		    if ( ( (IRSVN.z < 0) && (IRSVh < 0.64) ) || ( (IRSVN.z >= 0) && (IRSVh < IRSVcos ) ) )\
			    ulOGLSetCol = 0x00808000;\
    		else\
	    		ulOGLSetCol = 0x00003380;\
        }\
        else\
        {\
            float fColor;\
            ULONG ulColor;\
            MATH_NormalizeAnyVector(&IRSVN, &IRSVN);\
            IRSV0.x*=.3333333f;\
            IRSV0.y*=.3333333f;\
            IRSV0.z*=.3333333f;\
            IRSV1.x*=.3333333f;\
            IRSV1.y*=.3333333f;\
            IRSV1.z*=.3333333f;\
            MATH_AddVector(&IRSV0,&IRSV0,&IRSV1);\
            MATH_AddVector(&IRSV0,&IRSV0,&_pst_Point[t->auw_Index[0]]);\
            MATH_SubVector(&IRSV0,&IRSV0,&st_CameraPos);\
            fColor = MATH_f_FloatLimit(fAbs(MATH_f_DotProduct(&IRSVN,&IRSV0)),0.f,1.f);\
            ulColor = (ULONG)(255.f * fColor * fColor * fColor * fColor);\
            ulOGLSetCol &= 0xff000000;\
            ulOGLSetCol |= (ulColor<<16)+(ulColor<<8)+ulColor;\
        }\
		glColor4ubv((GLubyte *) &ulOGLSetCol);\
	}*/

#else

#	define OGL_RenderSlopeVars
#	define OGL_InitRenderSlope
#	define OGL_RenderSlope( t )

#endif//ACTIVE_EDITORS

#ifdef ACTIVE_EDITORS
extern COL_tdst_GlobalVars COL_gst_GlobalVars;
/*
 =======================================================================================================================
 =======================================================================================================================
 */
extern "C" LONG OGL_l_DrawSlopeTriangle(
        GEO_tdst_ElementIndexedTriangles *_pst_Element,
        GEO_Vertex *_pst_Point,
        int i_NumTriangle,
        MATH_tdst_Matrix *_pst_Matrix,
        GDI_tdst_DisplayData *pst_CurDD )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_tdst_SpecificData *pst_SD;
	GEO_tdst_IndexedTriangle *t;
	MATH_tdst_Vector *pst_A, *pst_B, *pst_C;
	MATH_tdst_Vector st_V1, st_V2, st_Norm;
	MATH_tdst_Vector st_Transform_Norm;
	float f;
	ULONG *pst_Color;
	ULONG *pst_Alpha;
	ULONG ulDrwMskDelta;
	ULONG ulOGLSetCol_Or;
	ULONG ulOGLSetCol_XOr;
	ULONG ulOGLSetCol;
	BOOL b_Ground, b_Wired;
	float f_CosAngle;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


	pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;
	t      = _pst_Element->dst_Triangle + i_NumTriangle;

	pst_A = &_pst_Point[ t->auw_Index[ 0 ] ];
	pst_B = &_pst_Point[ t->auw_Index[ 1 ] ];
	pst_C = &_pst_Point[ t->auw_Index[ 2 ] ];

	MATH_SubVector( &st_V1, pst_B, pst_A );
	MATH_SubVector( &st_V2, pst_C, pst_A );

	MATH_CrossProduct( &st_Norm, &st_V1, &st_V2 );

	if ( _pst_Matrix )
		MATH_TransformVectorNoScale( &st_Transform_Norm, _pst_Matrix, &st_Norm );
	else
		MATH_CopyVector( &st_Transform_Norm, &st_Norm );

	MATH_NormalizeEqualVector( &st_Transform_Norm );

	f = st_Transform_Norm.z;

	if ( pst_CurDD->uc_ColMapShowSlope == 1 )
		f_CosAngle = COL_gst_GlobalVars.f_WallCosAngle;
	else
		f_CosAngle = Cf_Cos45;

	b_Ground = ( ( ( f >= 0.0f ) && ( f < f_CosAngle ) ) || ( ( f < 0.0f ) && ( f > -0.80f ) ) );


	ulOGLSetCol_XOr = GDI_gpst_CurDD->pst_ComputingBuffers->ulColorXor;

	ulDrwMskDelta  = GDI_gpst_CurDD->LastDrawMask ^ GDI_gpst_CurDD->ul_CurrentDrawMask;
	pst_Color      = GDI_gpst_CurDD->pst_ComputingBuffers->CurrentColorField;
	pst_Alpha      = GDI_gpst_CurDD->pst_ComputingBuffers->CurrentAlphaField;
	ulOGLSetCol_Or = pst_SD->ulColorOr;

	OGL_RS_DrawWired( &pst_SD->st_RS, !( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_NotWired ) );
	b_Wired = !( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_NotWired );
	OGL_RS_CullFace( &pst_SD->st_RS, ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_TestBackFace ) );
	OGL_RS_CullFaceInverted( &pst_SD->st_RS, !( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_NotInvertBF ) );
	OGL_RS_DepthTest( &pst_SD->st_RS, GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_ZTest );
	OGL_RS_Fogged( &pst_SD->st_RS, ( ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_Fogged ) && ( pst_SD->ulFogState ) ) );

	GDI_gpst_CurDD->LastDrawMask = GDI_gpst_CurDD->ul_CurrentDrawMask;

	GDI_gpst_CurDD->profilingInformation.numBatches++;
	glBegin( GL_TRIANGLES );

	OGL_TestHideTriangle( t )
	{
		if ( !b_Wired )
		{
			if ( b_Ground )
				ulOGLSetCol = 0x00FFFF00;
			else
				ulOGLSetCol = 0x000066FF;

			glColor4ubv( ( GLubyte * ) &ulOGLSetCol );
		}

		if ( b_Wired )
			OGL_SetColorRGBABasic( t->auw_Index[ 0 ] );
		glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 0 ] ] );
		if ( b_Wired )
			OGL_SetColorRGBABasic( t->auw_Index[ 1 ] );
		glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 1 ] ] );
		if ( b_Wired )
			OGL_SetColorRGBABasic( t->auw_Index[ 2 ] );
		glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 2 ] ] );
	}

	glEnd();

	return 1;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
LONG OGL_l_DrawTriangle(
        GEO_tdst_ElementIndexedTriangles *_pst_Element,
        GEO_Vertex *_pst_Point,
        int i_NumTriangle )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_tdst_SpecificData *pst_SD;
	GEO_tdst_IndexedTriangle *t;
	ULONG *pst_Color;
	ULONG *pst_Alpha;
	ULONG ulDrwMskDelta;
	ULONG ulOGLSetCol_Or;
	ULONG ulOGLSetCol_XOr;
	ULONG ulOGLSetCol;
	BOOL bStrip;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


	pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;
	t      = _pst_Element->dst_Triangle + i_NumTriangle;

	ulOGLSetCol_XOr = GDI_gpst_CurDD->pst_ComputingBuffers->ulColorXor;

	ulDrwMskDelta  = GDI_gpst_CurDD->LastDrawMask ^ GDI_gpst_CurDD->ul_CurrentDrawMask;
	pst_Color      = GDI_gpst_CurDD->pst_ComputingBuffers->CurrentColorField;
	pst_Alpha      = GDI_gpst_CurDD->pst_ComputingBuffers->CurrentAlphaField;
	ulOGLSetCol_Or = pst_SD->ulColorOr;

	OGL_RS_DrawWired( &pst_SD->st_RS, !( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_NotWired ) );
	OGL_RS_CullFace( &pst_SD->st_RS, ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_TestBackFace ) );
	OGL_RS_CullFaceInverted( &pst_SD->st_RS, !( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_NotInvertBF ) );
	OGL_RS_DepthTest( &pst_SD->st_RS, GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_ZTest );
	OGL_RS_Fogged( &pst_SD->st_RS, ( ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_Fogged ) && ( pst_SD->ulFogState ) ) );

	GDI_gpst_CurDD->LastDrawMask = GDI_gpst_CurDD->ul_CurrentDrawMask;

	bStrip = FALSE;

	/* if there is no strip */
	if ( bStrip == FALSE )
	{
		GDI_gpst_CurDD->profilingInformation.numBatches++;
		glBegin( GL_TRIANGLES );

		if ( pst_Color == NULL )
		{
			ulOGLSetCol = GDI_gpst_CurDD->pst_ComputingBuffers->ul_Ambient | ulOGLSetCol_Or;
			ulOGLSetCol ^= ulOGLSetCol_XOr;
			glColor4ubv( ( GLubyte * ) &ulOGLSetCol );
		}

		if ( 1 )
		{
			OGL_TestHideTriangle( t )
			{
				OGL_SetColorRGBABasic( t->auw_Index[ 0 ] );
				glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 0 ] ] );
				OGL_SetColorRGBABasic( t->auw_Index[ 1 ] );
				glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 1 ] ] );
				OGL_SetColorRGBABasic( t->auw_Index[ 2 ] );
				glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 2 ] ] );
			}
		}

		glEnd();
	}

	return 1;
}

#endif


/*
 =======================================================================================================================
 =======================================================================================================================
 */
#ifdef ACTIVE_EDITORS
extern "C" BOOL OGL_bCountTriangles = TRUE;
#endif//ACTIVE_EDITORS

LONG OGL_l_DrawElementIndexedTriangles(
        GEO_tdst_ElementIndexedTriangles *_pst_Element,
        GEO_Vertex *_pst_Point,
        GEO_tdst_UV *_pst_UV,
        ULONG ulnumberOfPoints )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	OGL_tdst_SpecificData *pst_SD;
	GEO_tdst_IndexedTriangle *t, *tend;
	GEO_tdst_IndexedTriangle *st, *stend;
	ULONG *pst_Color;
	ULONG *pst_Alpha;
	ULONG ulDrwMskDelta;
	ULONG ulOGLSetCol_Or;
	ULONG ulOGLSetCol_XOr;
	ULONG ulOGLSetCol;
	BOOL bStrip;
	USHORT auw_Index, auw_UV;
	GEO_tdst_OneStrip *pStrip, *pStripEnd;

#ifdef ACTIVE_EDITORS
	ULONG ulSaveAmbientColor;
#endif//ACTIVE_EDITORS

	//MATH_tdst_Matrix mat_save;
	OGL_RenderSlopeVars
	        /*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	        pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;
	t              = _pst_Element->dst_Triangle;
	tend           = t + _pst_Element->l_NbTriangles;
	st             = t;
	stend          = tend;
#ifdef ACTIVE_EDITORS
	if ( OGL_bCountTriangles )
	{
		Stats_ulNumberOfTRiangles += _pst_Element->l_NbTriangles;
	}
#endif

	ulOGLSetCol_XOr = GDI_gpst_CurDD->pst_ComputingBuffers->ulColorXor;

	ulDrwMskDelta = GDI_gpst_CurDD->LastDrawMask ^ GDI_gpst_CurDD->ul_CurrentDrawMask;
#ifdef ACTIVE_EDITORS
	if ( OGL_gb_DispLOD && OGL_ulLODAmbient )
	{
		pst_Color                                        = NULL;
		ulSaveAmbientColor                               = GDI_gpst_CurDD->pst_ComputingBuffers->ul_Ambient;
		GDI_gpst_CurDD->pst_ComputingBuffers->ul_Ambient = OGL_ulLODAmbient;
	}
	else
#endif// ACTIVE_EDITORS
		pst_Color = GDI_gpst_CurDD->pst_ComputingBuffers->CurrentColorField;
	pst_Alpha      = GDI_gpst_CurDD->pst_ComputingBuffers->CurrentAlphaField;
	ulOGLSetCol_Or = pst_SD->ulColorOr;

	OGL_RS_DrawWired( &pst_SD->st_RS, !( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_NotWired ) );
	OGL_RS_CullFace( &pst_SD->st_RS, ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_TestBackFace ) );
	OGL_RS_CullFaceInverted( &pst_SD->st_RS, !( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_NotInvertBF ) );
	OGL_RS_DepthTest( &pst_SD->st_RS, GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_ZTest );
	OGL_RS_Fogged( &pst_SD->st_RS, ( ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_Fogged ) && ( pst_SD->ulFogState ) ) );


	GDI_gpst_CurDD->LastDrawMask = GDI_gpst_CurDD->ul_CurrentDrawMask;

	if ( _pst_Element->pst_StripData != NULL )
	{
		if ( _pst_Element->pst_StripData->ulFlag & GEO_C_Strip_DataValid )
			bStrip = TRUE;
		else
			bStrip = FALSE;
	}
	else
		bStrip = FALSE;

	// DRL: Prevent using strip data for facemaps
	if ( bStrip && _pst_UV && !( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_UseOneUVPerPoint ) && ( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_FaceMap ) )
		bStrip = FALSE;

	if ( bStrip == FALSE )
	{
		{
			OGL_InitRenderSlope

			        PRO_StartTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Begin );
			GDI_gpst_CurDD->profilingInformation.numBatches++;
			OGL_CALL( glBegin( GL_TRIANGLES ) );
			PRO_StopTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Begin );

			if ( pst_Color == NULL )
			{
				ulOGLSetCol = GDI_gpst_CurDD->pst_ComputingBuffers->ul_Ambient | ulOGLSetCol_Or;
				ulOGLSetCol ^= ulOGLSetCol_XOr;
				glColor4ubv( ( GLubyte * ) &ulOGLSetCol );
			}

			if ( !_pst_UV )
			{
				while ( t < tend )
				{
					OGL_TestHideTriangle( t )
					{
						OGL_RenderSlope( t )
						        OGL_SetColorRGBA( t->auw_Index[ 0 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 0 ] ] ) );
						OGL_SetColorRGBA( t->auw_Index[ 1 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 1 ] ] ) );
						OGL_SetColorRGBA( t->auw_Index[ 2 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 2 ] ] ) );
					}
					t++;
				}
			}
			else if ( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_UseOneUVPerPoint )
			{
				while ( t < tend )
				{
					OGL_TestHideTriangle( t )
					{
						OGL_RenderSlope( t )
						        OGL_CALL( glTexCoord2fv( &_pst_UV[ t->auw_Index[ 0 ] ].fU ) );
						OGL_SetColorRGBA( t->auw_Index[ 0 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 0 ] ] ) );
						OGL_CALL( glTexCoord2fv( &_pst_UV[ t->auw_Index[ 1 ] ].fU ) );
						OGL_SetColorRGBA( t->auw_Index[ 1 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 1 ] ] ) );
						OGL_CALL( glTexCoord2fv( &_pst_UV[ t->auw_Index[ 2 ] ].fU ) );
						OGL_SetColorRGBA( t->auw_Index[ 2 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 2 ] ] ) );
					}
					t++;
				}
			}
			else if ( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_FaceMap )
			{
				while ( t < tend )
				{
					OGL_TestHideTriangle( t )
					{
						OGL_RenderSlope( t )
						        OGL_CALL( glTexCoord2fv( ( float * ) &_pst_UV[ ( ( t->ul_MaxFlags >> 7 ) & 3 ) ] ) );
						OGL_SetColorRGBA( t->auw_Index[ 0 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 0 ] ] ) );
						OGL_CALL( glTexCoord2fv( ( float * ) &_pst_UV[ ( ( t->ul_MaxFlags >> 9 ) & 3 ) ] ) );
						OGL_SetColorRGBA( t->auw_Index[ 1 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 1 ] ] ) );
						OGL_CALL( glTexCoord2fv( ( float * ) &_pst_UV[ ( ( t->ul_MaxFlags >> 11 ) & 3 ) ] ) );
						OGL_SetColorRGBA( t->auw_Index[ 2 ] );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 2 ] ] ) );
					}
					t++;
				}
			}
			else
			{
				while ( t < tend )
				{
					OGL_TestHideTriangle( t )
					{
						OGL_RenderSlope( t )
						        glTexCoord2fv( &_pst_UV[ t->auw_UV[ 0 ] ].fU );
						OGL_SetColorRGBA( t->auw_Index[ 0 ] );
						glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 0 ] ] );
						glTexCoord2fv( &_pst_UV[ t->auw_UV[ 1 ] ].fU );
						OGL_SetColorRGBA( t->auw_Index[ 1 ] );
						glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 1 ] ] );
						glTexCoord2fv( &_pst_UV[ t->auw_UV[ 2 ] ].fU );
						OGL_SetColorRGBA( t->auw_Index[ 2 ] );
						glVertex3fv( ( float * ) &_pst_Point[ t->auw_Index[ 2 ] ] );
					}
					t++;
				}
			}

			PRO_StartTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_End );
			glEnd();

			PRO_StopTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_End );
		}
	}
	else
	{
		/*~~~~~~~~~~~~~~~~*/

		/* *** draw strip *** */
		ULONG i, j, k;
		ULONG ulEdgeColor;
		/*~~~~~~~~~~~~~~~~*/

		OGL_InitRenderSlope

		        if ( ( OGL_gb_DispStrip || ( _pst_Element->pst_StripData->ulFlag & GEO_C_Strip_DisplayStrip ) ) && ( ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_NotWired ) ) )
		{
			pStrip    = _pst_Element->pst_StripData->pStripList;
			pStripEnd = pStrip + _pst_Element->pst_StripData->ulStripNumber;
			OGL_CALL( glDisable( GL_TEXTURE_2D ) );

			for ( j = 0; pStrip < pStripEnd; pStrip++, j++ )
			{
				k           = j;
				ulEdgeColor = 0;
				while ( k )
				{
					if ( k & 1 ) ulEdgeColor += 0x7f;
					if ( k & 2 ) ulEdgeColor += 0x7f00;
					if ( k & 4 ) ulEdgeColor += 0x7f0000;
					k >>= 3;
				}

				PRO_StartTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Begin );
				GDI_gpst_CurDD->profilingInformation.numBatches++;
				OGL_CALL( glBegin( GL_TRIANGLE_STRIP /*GL_TRIANGLE_FAN/*GL_TRIANGLE_STRIP*/ ) );
				PRO_StopTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Begin );

				if ( pst_Color == NULL )
				{
					ulOGLSetCol = GDI_gpst_CurDD->pst_ComputingBuffers->ul_Ambient | ulOGLSetCol_Or;
					ulOGLSetCol ^= ulOGLSetCol_XOr;
					glColor4ubv( ( GLubyte * ) &ulOGLSetCol );
				}

				if ( !_pst_UV )
				{
					for ( i = 0; i < pStrip->ulVertexNumber; i++ )
					{
						auw_Index = pStrip->pMinVertexDataList[ i ].auw_Index;

						OGL_SetColorRGBA( auw_Index );
						glColor4ubv( ( GLubyte * ) &ulEdgeColor );
						glVertex3fv( ( float * ) &_pst_Point[ auw_Index ] );
					}
				}
				else
				{
					for ( i = 0; i < pStrip->ulVertexNumber; i++ )
					{
						auw_Index = pStrip->pMinVertexDataList[ i ].auw_Index;
						auw_UV    = pStrip->pMinVertexDataList[ i ].auw_UV;

						glTexCoord2fv( &_pst_UV[ auw_UV ].fU );
						OGL_SetColorRGBA( auw_Index );
						glColor4ubv( ( GLubyte * ) &ulEdgeColor );
						glVertex3fv( ( float * ) &_pst_Point[ auw_Index ] );
					}
				}

				PRO_StartTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_End );
				glEnd();
				PRO_StopTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_End );
			}
		}
		else
		{
			pStrip    = _pst_Element->pst_StripData->pStripList;
			pStripEnd = pStrip + _pst_Element->pst_StripData->ulStripNumber;

			for ( ; pStrip < pStripEnd; pStrip++ )
			{
				PRO_StartTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Begin );
				GDI_gpst_CurDD->profilingInformation.numBatches++;
				OGL_CALL( glBegin( GL_TRIANGLE_STRIP /*GL_TRIANGLE_FAN/*GL_TRIANGLE_STRIP*/ ) );
				PRO_StopTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_Begin );

				if ( pst_Color == NULL )
				{
					ulOGLSetCol = GDI_gpst_CurDD->pst_ComputingBuffers->ul_Ambient | ulOGLSetCol_Or;
					ulOGLSetCol ^= ulOGLSetCol_XOr;
					glColor4ubv( ( GLubyte * ) &ulOGLSetCol );
				}

				if ( !_pst_UV )
				{
					for ( i = 0; i < pStrip->ulVertexNumber; i++ )
					{
						auw_Index = pStrip->pMinVertexDataList[ i ].auw_Index;

						OGL_SetColorRGBA( auw_Index );
						glVertex3fv( ( float * ) &_pst_Point[ auw_Index ] );
					}
				}
				else if ( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_UseOneUVPerPoint )
				{
					for ( i = 0; i < pStrip->ulVertexNumber; i++ )
					{
						auw_Index = pStrip->pMinVertexDataList[ i ].auw_Index;

						glTexCoord2fv( &_pst_UV[ auw_Index ].fU );
						OGL_SetColorRGBA( auw_Index );
						glVertex3fv( ( float * ) &_pst_Point[ auw_Index ] );
					}
				}
				else if ( GDI_gpst_CurDD->ul_DisplayInfo & GDI_Cul_DI_FaceMap )
				{
					ERR_X_ForceError( "pas de chance, on n'a pas facemap + strip en magasin", NULL );
				}
				else
				{
					for ( i = 0; i < pStrip->ulVertexNumber; i++ )
					{
						auw_Index = pStrip->pMinVertexDataList[ i ].auw_Index;
						auw_UV    = pStrip->pMinVertexDataList[ i ].auw_UV;

						OGL_CALL( glTexCoord2fv( &_pst_UV[ auw_UV ].fU ) );
						OGL_SetColorRGBA( auw_Index );
						OGL_CALL( glVertex3fv( ( float * ) &_pst_Point[ auw_Index ] ) );
					}
				}

				PRO_StartTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_End );
				glEnd();
				PRO_StopTrameRaster( &GDI_gpst_CurDD->pst_Raster->st_GL_End );
			}
		}
	}

#ifdef ACTIVE_EDITORS
	if ( OGL_gb_DispLOD && OGL_ulLODAmbient )
		GDI_gpst_CurDD->pst_ComputingBuffers->ul_Ambient = ulSaveAmbientColor;
#endif//ACTIVE_EDITORS

	/* fin */
	return _pst_Element->l_NbTriangles;
}

/*
 =======================================================================================================================
    Aim:    Setup blending mode
 =======================================================================================================================
 */
#ifdef ACTIVE_EDITORS
static int restore_polygon_offset;
#	define M4Edit_RestorePolygonOffset restore_polygon_offset = 1;
#else
#	define M4Edit_RestorePolygonOffset
#endif

extern "C" ULONG OpenglCorrectBugMul2X;
void OGL_SetTextureBlending( ULONG _l_Texture, ULONG BM )
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	ULONG Flag;
	ULONG Delta;
	OGL_tdst_SpecificData *pst_SD;

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	Delta = 0;

	Flag   = MAT_GET_FLAG( BM );
	pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;

	_l_Texture = OGL_RS_UseTexture( pst_SD, _l_Texture );

	if ( _l_Texture != ( ULONG ) -1 )
	{
		if ( pst_SD->dul_TextureDeltaBlend )
		{
			Delta = pst_SD->dul_TextureDeltaBlend[ _l_Texture ] ^ BM;
		}

		/* BEGIN TEXTURE SPECIFIC. BE CAREFULL TO "DELTA" CHANGES */
		if ( Delta & MAT_Cul_Flag_TileU )
		{
			if ( Flag & MAT_Cul_Flag_TileU )
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			else
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		}

		if ( Delta & MAT_Cul_Flag_TileV )
		{
			if ( Flag & MAT_Cul_Flag_TileV )
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
			else
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
		}

		if ( Delta & MAT_Cul_Flag_Bilinear )
		{
			if ( Flag & MAT_Cul_Flag_Bilinear )
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
			else
				glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		}

		if ( Delta & ( MAT_Cul_Flag_Trilinear | MAT_Cul_Flag_Bilinear ) )
		{
			if ( TEX_gst_GlobalList.dst_Texture[ _l_Texture ].uw_Flags & TEX_uw_Mipmap )
			{
				if ( Flag & MAT_Cul_Flag_Bilinear )
				{
					if ( Flag & MAT_Cul_Flag_Trilinear )
						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
					else
						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
				}
				else
				{
					if ( Flag & MAT_Cul_Flag_Trilinear )
						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
					else
						glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST );
				}
			}
		}

		if ( pst_SD->dul_TextureDeltaBlend )
		{
			pst_SD->dul_TextureDeltaBlend[ _l_Texture ] = BM;
		}

		/* END TEXTURE SPECIFIC. BE CAREFULL TO "DELTA" CHANGES */
	}

	Delta = GDI_gpst_CurDD->LastBlendingMode ^ BM;

	/* Delta = 0xffffffff; */
	if ( Delta & ( MAT_Cul_Flag_HideAlpha | MAT_Cul_Flag_HideColor ) )
	{
		switch ( Flag & ( MAT_Cul_Flag_HideAlpha | MAT_Cul_Flag_HideColor ) )
		{
			case 0:
				OGL_CALL( glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE ) );
				break;
			case MAT_Cul_Flag_HideAlpha:
				OGL_CALL( glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE ) );
				break;
			case MAT_Cul_Flag_HideColor:
				OGL_CALL( glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE ) );
				break;
			case MAT_Cul_Flag_HideColor | MAT_Cul_Flag_HideAlpha:
				// ShadowMode
				OGL_CALL( glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE ) );
				break;
		}
	}

	if ( Delta & ( MAT_Cul_Flag_AlphaTest | MAT_Cc_AlphaTresh_MASK | MAT_Cul_Flag_InvertAlpha ) )
	{
		if ( Flag & MAT_Cul_Flag_AlphaTest )
		{
			if ( Flag & MAT_Cul_Flag_InvertAlpha )
				glAlphaFunc( GL_LESS, 1.0f / 256.0f * ( float ) MAT_GET_AlphaTresh( BM ) );
			else
				glAlphaFunc( GL_GREATER, 1.0f / 256.0f * ( float ) MAT_GET_AlphaTresh( BM ) );

			glEnable( GL_ALPHA_TEST );
			//glEnable( GL_SAMPLE_ALPHA_TO_COVERAGE );
		}
		else
		{
			glDisable( GL_ALPHA_TEST );
			//glDisable( GL_SAMPLE_ALPHA_TO_COVERAGE );
		}
	}

	if ( Flag & MAT_Cul_Flag_UseLocalAlpha )
		pst_SD->ulColorOr = 0xff000000;
	else
		pst_SD->ulColorOr = 0;

	OGL_RS_DepthFunc( &pst_SD->st_RS, ( Flag & MAT_Cul_Flag_ZEqual ) ? GL_EQUAL : GL_LEQUAL );
	OGL_RS_DepthMask( &pst_SD->st_RS, ( Flag & MAT_Cul_Flag_NoZWrite ) ? 0 : 1 );

#ifdef ACTIVE_EDITORS
	if ( restore_polygon_offset )
	{
		restore_polygon_offset = 0;
		GDI_gpst_CurDD->st_GDI.pfnl_Request( GDI_Cul_Request_PolygonOffsetRestore, 0 );
	}
#else
	glDisable( GL_POLYGON_OFFSET_FILL );
#endif

	if ( ( MAT_GET_Blending( Delta ) ) || ( Delta & MAT_Cul_Flag_InvertAlpha ) )
	{
		switch ( MAT_GET_Blending( BM ) )
		{
			case MAT_Cc_Op_Copy:
			case MAT_Cc_Op_Glow:
				OGL_CALL( glDisable( GL_BLEND ) );
				OGL_CALL( glBlendFunc( GL_ONE, GL_ZERO ) );
				break;
			case MAT_Cc_Op_Alpha:
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Copy ) glEnable( GL_BLEND );
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Glow ) glEnable( GL_BLEND );
				if ( Flag & MAT_Cul_Flag_InvertAlpha )
					glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA );
				else
					glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
				break;
			case MAT_Cc_Op_AlphaPremult:
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Copy ) glEnable( GL_BLEND );
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Glow ) glEnable( GL_BLEND );

				if ( Flag & MAT_Cul_Flag_InvertAlpha )
					glBlendFunc( GL_ONE, GL_SRC_ALPHA );
				else
					glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
				break;
			case MAT_Cc_Op_AlphaDest:
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Copy ) glEnable( GL_BLEND );
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Glow ) glEnable( GL_BLEND );
				if ( Flag & MAT_Cul_Flag_InvertAlpha )
					glBlendFunc( GL_ONE_MINUS_DST_ALPHA, GL_DST_ALPHA );
				else
					glBlendFunc( GL_DST_ALPHA, GL_ONE_MINUS_DST_ALPHA );
				break;
			case MAT_Cc_Op_AlphaDestPremult:
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Copy ) glEnable( GL_BLEND );
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Glow ) glEnable( GL_BLEND );
				if ( Flag & MAT_Cul_Flag_InvertAlpha )
					glBlendFunc( GL_ONE_MINUS_DST_ALPHA, GL_ONE );
				else
					glBlendFunc( GL_DST_ALPHA, GL_ONE );
				break;
			case MAT_Cc_Op_Add:
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Copy ) glEnable( GL_BLEND );
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Glow ) glEnable( GL_BLEND );
				if ( OpenglCorrectBugMul2X )
				{
					if ( Flag & MAT_Cul_Flag_InvertAlpha )
						glBlendFunc( GL_ONE_MINUS_SRC_ALPHA, GL_ONE );
					else
						glBlendFunc( GL_SRC_ALPHA, GL_ONE );
				}
				else
					glBlendFunc( GL_ONE, GL_ONE );
				break;
			case MAT_Cc_Op_PSX2ShadowSpecific:
				M4Edit_RestorePolygonOffset
				        OGL_CALL( glPolygonOffset( -0.91f, -1000.0f ) );
				glEnable( GL_POLYGON_OFFSET_FILL );
				OGL_RS_DepthMask( &pst_SD->st_RS, 0 );

			case MAT_Cc_Op_Sub:
				OGL_CALL( glFogfv( GL_FOG_COLOR, pst_SD->fFogColor ) );
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Copy ) glEnable( GL_BLEND );
				if ( MAT_GET_Blending( GDI_gpst_CurDD->LastBlendingMode ) == MAT_Cc_Op_Glow ) glEnable( GL_BLEND );
				OGL_CALL( glBlendFunc( GL_ZERO, GL_ONE_MINUS_SRC_COLOR ) );
				// test bump
				//glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
				break;
		}
	}

	if ( pst_SD->ulFogState & 1 )
	{
		switch ( MAT_GET_Blending( BM ) )
		{
			case MAT_Cc_Op_Copy:
			case MAT_Cc_Op_Glow:
			case MAT_Cc_Op_Alpha:
			case MAT_Cc_Op_AlphaDest:
				if ( pst_SD->ulFogState != 1 )
				{
					pst_SD->ulFogState = 1;
					glFogfv( GL_FOG_COLOR, pst_SD->fFogColor );
				}
				break;
			case MAT_Cc_Op_AlphaPremult:
			case MAT_Cc_Op_AlphaDestPremult:
				if ( pst_SD->ulFogState != 5 )
				{
					pst_SD->ulFogState        = 5;
					pst_SD->fFogColorOn2[ 3 ] = 0.0f;
					glFogfv( GL_FOG_COLOR, pst_SD->fFogColor );
				}
				break;
			case MAT_Cc_Op_Add:
			case MAT_Cc_Op_Sub:
				if ( pst_SD->ulFogState != 3 )
				{
					pst_SD->ulFogState = 3;
					glFogfv( GL_FOG_COLOR, pst_SD->fFogBlack );
				}
				break;
		}
	}

	GDI_gpst_CurDD->LastTextureUsed  = _l_Texture;
	GDI_gpst_CurDD->LastBlendingMode = BM;
}

extern "C" float TIM_gf_MainClockReal;
LONG OGL_l_DrawElementIndexedSprites(
        GEO_tdst_ElementIndexedSprite *_pst_Element,
        GEO_Vertex *_pst_Point,
        ULONG ulnumberOfPoints )
{
	OGL_tdst_SpecificData *pst_SD;
	MATH_tdst_Vector XCam, YCam, *p_point;
	MATH_tdst_Vector Sprite[ 5 ];
	float Size;
	GDI_tdst_DisplayData *pst_DD;
	GEO_tdst_IndexedSprite *p_Frst, *p_Last;
	ULONG ulOGLSetCol_Or;
	ULONG ulOGLSetCol_XOr, ulOGLSetCol;
	ULONG *pst_Color;
	ULONG *pst_Alpha;
	float count;

	pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;

	ulOGLSetCol_XOr = GDI_gpst_CurDD->pst_ComputingBuffers->ulColorXor;


	pst_Color      = GDI_gpst_CurDD->pst_ComputingBuffers->CurrentColorField;
	pst_Alpha      = GDI_gpst_CurDD->pst_ComputingBuffers->CurrentAlphaField;
	ulOGLSetCol_Or = ( ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData )->ulColorOr;
	ulOGLSetCol    = GDI_gpst_CurDD->pst_ComputingBuffers->ul_Ambient | ulOGLSetCol_Or;
	ulOGLSetCol ^= ulOGLSetCol_XOr;


	OGL_RS_Fogged( &pst_SD->st_RS, ( ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_Fogged ) && ( pst_SD->ulFogState ) ) );
	pst_DD = GDI_gpst_CurDD;
	XCam.x = -pst_DD->st_MatrixStack.pst_CurrentMatrix->Ix * _pst_Element->fGlobalSize * _pst_Element->fGlobalRatio;
	XCam.y = -pst_DD->st_MatrixStack.pst_CurrentMatrix->Jx * _pst_Element->fGlobalSize * _pst_Element->fGlobalRatio;
	XCam.z = -pst_DD->st_MatrixStack.pst_CurrentMatrix->Kx * _pst_Element->fGlobalSize * _pst_Element->fGlobalRatio;
	YCam.x = pst_DD->st_MatrixStack.pst_CurrentMatrix->Iy * _pst_Element->fGlobalSize;
	YCam.y = pst_DD->st_MatrixStack.pst_CurrentMatrix->Jy * _pst_Element->fGlobalSize;
	YCam.z = pst_DD->st_MatrixStack.pst_CurrentMatrix->Ky * _pst_Element->fGlobalSize;
	pst_DD->st_GDI.pfnl_Request( GDI_Cul_Request_BeforeDrawSprite, 0 );

	p_Frst = _pst_Element->dst_Sprite;
	p_Last = p_Frst + _pst_Element->l_NbSprites;
	if ( p_Frst->auw_Index == 0xC0DE )// GFX Signal of mega-flux (no indexes) , points ares SOFT_tdst_AVertexes with w = size
	{
		SOFT_tdst_AVertex *pS, *pSE;
		pS = ( SOFT_tdst_AVertex * ) _pst_Point;
		pS += p_Frst[ 1 ].auw_Index;
		pSE = pS + _pst_Element->l_NbSprites;
		if ( pst_Color ) pst_Color += p_Frst[ 1 ].auw_Index;
		if ( pst_Alpha ) pst_Alpha += p_Frst[ 1 ].auw_Index;
		while ( pS < pSE )
		{
			if ( pst_Color )
			{
				ulOGLSetCol = *pst_Color | ulOGLSetCol_Or;
				ulOGLSetCol ^= ulOGLSetCol_XOr;
				if ( pst_Alpha )
				{
					ulOGLSetCol &= 0x00ffffff;
					ulOGLSetCol |= pst_Alpha[ p_Frst->auw_Index ];
				}
				pst_Color++;
			}
			else if ( pst_Alpha )
			{
				ulOGLSetCol &= 0x00ffffff;
				ulOGLSetCol |= pst_Alpha[ p_Frst->auw_Index ];
			}

			*( ULONG * ) &Sprite[ 4 ].x = ulOGLSetCol;
			Size                        = pS->w;

			Sprite[ 0 ].x = pS->x + ( -XCam.x - YCam.x ) * Size;
			Sprite[ 0 ].y = pS->y + ( -XCam.y - YCam.y ) * Size;
			Sprite[ 0 ].z = pS->z + ( -XCam.z - YCam.z ) * Size;
			Sprite[ 1 ].x = pS->x + ( +XCam.x - YCam.x ) * Size;
			Sprite[ 1 ].y = pS->y + ( +XCam.y - YCam.y ) * Size;
			Sprite[ 1 ].z = pS->z + ( +XCam.z - YCam.z ) * Size;
			Sprite[ 2 ].x = pS->x + ( +XCam.x + YCam.x ) * Size;
			Sprite[ 2 ].y = pS->y + ( +XCam.y + YCam.y ) * Size;
			Sprite[ 2 ].z = pS->z + ( +XCam.z + YCam.z ) * Size;
			Sprite[ 3 ].x = pS->x + ( -XCam.x + YCam.x ) * Size;
			Sprite[ 3 ].y = pS->y + ( -XCam.y + YCam.y ) * Size;
			Sprite[ 3 ].z = pS->z + ( -XCam.z + YCam.z ) * Size;

			pst_DD->st_GDI.pfnl_Request( GDI_Cul_Request_DrawSpriteUV, ( ULONG ) Sprite );
			pS++;
		}
	}
	else
	{
		count = 0;
		while ( p_Frst < p_Last )
		{
			count += 1.0f;
			if ( pst_Color )
			{
				ulOGLSetCol = pst_Color[ p_Frst->auw_Index ] | ulOGLSetCol_Or;
				ulOGLSetCol ^= ulOGLSetCol_XOr;
				if ( pst_Alpha )
				{
					ulOGLSetCol &= 0x00ffffff;
					ulOGLSetCol |= pst_Alpha[ p_Frst->auw_Index ];
				}
			}
			else if ( pst_Alpha )
			{
				ulOGLSetCol &= 0x00ffffff;
				ulOGLSetCol |= pst_Alpha[ p_Frst->auw_Index ];
			}

			*( ULONG * ) &Sprite[ 4 ].x = ulOGLSetCol;
			p_point                     = _pst_Point + p_Frst->auw_Index;
			Size                        = *( float                        *) p_Frst;

			Sprite[ 0 ].x = p_point->x + ( -XCam.x - YCam.x ) * Size;
			Sprite[ 0 ].y = p_point->y + ( -XCam.y - YCam.y ) * Size;
			Sprite[ 0 ].z = p_point->z + ( -XCam.z - YCam.z ) * Size;
			Sprite[ 1 ].x = p_point->x + ( +XCam.x - YCam.x ) * Size;
			Sprite[ 1 ].y = p_point->y + ( +XCam.y - YCam.y ) * Size;
			Sprite[ 1 ].z = p_point->z + ( +XCam.z - YCam.z ) * Size;
			Sprite[ 2 ].x = p_point->x + ( +XCam.x + YCam.x ) * Size;
			Sprite[ 2 ].y = p_point->y + ( +XCam.y + YCam.y ) * Size;
			Sprite[ 2 ].z = p_point->z + ( +XCam.z + YCam.z ) * Size;
			Sprite[ 3 ].x = p_point->x + ( -XCam.x + YCam.x ) * Size;
			Sprite[ 3 ].y = p_point->y + ( -XCam.y + YCam.y ) * Size;
			Sprite[ 3 ].z = p_point->z + ( -XCam.z + YCam.z ) * Size;

			if ( GDI_gb_WaveSprite )
			{
				float ff, ff1;
				ff  = fOptSin( count );
				ff1 = fOptSin( TIM_gf_MainClockReal * ff * 2.5f ) * ff * 0.2f;
				Sprite[ 0 ].x += ff1;
				Sprite[ 1 ].x += ff1;
			}

			pst_DD->st_GDI.pfnl_Request( GDI_Cul_Request_DrawSpriteUV, ( ULONG ) Sprite );
			p_Frst++;
		}
	}
	pst_DD->st_GDI.pfnl_Request( GDI_Cul_Request_AfterDrawSprite, 0 );
	return 0;
}
/**********************************************************************************************************************/
/* SPG2 specific functions BEGIN **************************************************************************************/
/**********************************************************************************************************************/
#define CosAlpha -0.34202014332566873304409961468226f
#define SinAlpha 0.9396926207859083840541092773247f
ULONG OGL_ScaleColor( float Factor, ULONG C0 )
{
	ULONG X;
	ULONG CX;
	ULONG CXp;
	X   = ( ULONG ) ( Factor );
	CX  = C0 & 0xff00ff;
	CXp = ( C0 >> 8 ) & 0xff00ff;
	CX *= X;
	CXp *= X;

	CX &= 0xff00ff00;
	CXp &= 0x0000ff00;
	CXp |= CX >> 8;
	CXp |= C0 & 0xff000000;

	return CXp;
}

void OGL_l_DrawSPG2_2X(
        SOFT_tdst_AVertex *Coordinates,
        ULONG *pColors,
        ULONG ulnumberOfPoints,
        ULONG ulNumberOfSegments,
        float fTrapeze,
        float fEOHP,
        float fRatio,
        ULONG TileNumber,
        ULONG ulMode,
        SOFT_tdst_AVertex *pWind,
        SPG2_InstanceInforamtion *p_stII )
{
	SOFT_tdst_UV tf_UV;
	ULONG BM;

	float DeltaU;
	float fTrapezeDelta, fTrapezeInc, fOoNumOfSeg;


	ULONG Counter;

	u32 TextureTilerUV_Base, U_SHIFT, V_SHIFT;
	float TexUMax, TexVMax;
	float TexUBase, TexVBase;

	TexUBase = TexVBase = 0.0f;

	TextureTilerUV_Base = ( p_stII->BaseAnimUv >> 16 ) & 0xff;
	U_SHIFT             = ( p_stII->BaseAnimUv >> 24 ) & 0xf;
	V_SHIFT             = ( p_stII->BaseAnimUv >> 28 ) & 0xf;

	/* Fast One Over 2^N */
	*( u32 * ) &TexUMax = ( 127 - U_SHIFT ) << 23;
	*( u32 * ) &TexVMax = ( 127 - V_SHIFT ) << 23;

	BM          = 0;
	fOoNumOfSeg = 1.0f / ( float ) ulNumberOfSegments;


	DeltaU        = TexVMax * ( float ) TileNumber * fOoNumOfSeg;
	fTrapezeDelta = -fTrapeze * fOoNumOfSeg;
	while ( ulnumberOfPoints-- )
	{
		SOFT_tdst_AVertex u_4Vert[ 4 ];
		SOFT_tdst_AVertex u_4Vert_Trpz[ 4 ];
		SOFT_tdst_AVertex stTrpeze;
		SOFT_tdst_AVertex stNormale;
		SOFT_tdst_AVertex stGravity;
		SOFT_tdst_AVertex stDerivative;
		float Interpolator, InterpolatorIntensity;
		float fColorBlenderAcc, fColorBlenderAdd;
		ULONG ulColor;
		if ( p_stII->BaseAnimUv )
		{
			TexUBase = ( float ) ( TextureTilerUV_Base & ( ( 1 << U_SHIFT ) - 1 ) ) * TexUMax;
			TexVBase = ( float ) ( ( TextureTilerUV_Base >> U_SHIFT ) ^ 0xf ) * TexVMax;
			TextureTilerUV_Base += 3;
		}
		tf_UV.u      = TexUBase;
		tf_UV.v      = TexVBase;
		u_4Vert[ 0 ] = *( Coordinates++ );// Pos
		MATH_AddVector( ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], &p_stII->GlobalPos );
		u_4Vert[ 1 ] = *( Coordinates++ );// Xa
		u_4Vert[ 2 ] = *( Coordinates++ );// Ya

		stGravity             = pWind[ Coordinates->c ^ p_stII->BaseWind ];
		InterpolatorIntensity = stGravity.w * fOoNumOfSeg;
		stNormale             = *( Coordinates++ );
		MATH_AddVector( ( MATH_tdst_Vector * ) &stNormale, ( MATH_tdst_Vector * ) &stNormale, &p_stII->GlobalZADD );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &stNormale, p_stII->GlobalSCale );

		Counter      = ulNumberOfSegments + 1;
		Interpolator = 0.0f;
		fTrapezeInc  = fTrapeze;

		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &stNormale, u_4Vert[ 2 ].w * fOoNumOfSeg );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &stGravity, u_4Vert[ 2 ].w * fOoNumOfSeg );
		if ( ulMode == 0 )// DrawX
		{
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], -0.5f * u_4Vert[ 2 ].w * fRatio );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], u_4Vert[ 2 ].w * fRatio );
		}
		else if ( ulMode == 1 )// DrawY
		{
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 2 ], -0.5f * u_4Vert[ 2 ].w * fRatio );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 2 ], u_4Vert[ 2 ].w * fRatio );
		}
		else if ( ulMode == 2 )// DrawH
		{
			/*	
			F'(X) = 2AX + B
			F(X) = AX� + BX + C

			X E [0,1]
			C = Position(x) de dep
			B = x de la normale
			A = ( x de G - x de N ) / 2
	*/
			MATH_SubVector( ( MATH_tdst_Vector * ) &stTrpeze, ( MATH_tdst_Vector * ) &stGravity, ( MATH_tdst_Vector * ) &stNormale );
			MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &stTrpeze, ( float ) ulNumberOfSegments * InterpolatorIntensity * fEOHP * fEOHP / 2.0f );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &stTrpeze, ( MATH_tdst_Vector * ) &stTrpeze, ( MATH_tdst_Vector * ) &stNormale, fEOHP );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &stTrpeze, ( float ) ulNumberOfSegments );
			DeltaU   = TexVMax;
			Counter  = 2;
			fTrapeze = 0.0f;
			MATH_ScaleVector( ( MATH_tdst_Vector * ) &stNormale, ( MATH_tdst_Vector * ) &u_4Vert[ 2 ], u_4Vert[ 2 ].w * fRatio );
			stGravity = stNormale;
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], -0.5f * u_4Vert[ 2 ].w * fRatio );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 2 ], -0.5f * u_4Vert[ 2 ].w * fRatio );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], u_4Vert[ 2 ].w * fRatio );
		}

		fColorBlenderAcc = 255.0f;
		fColorBlenderAdd = -fOoNumOfSeg * 255.0f;
		GDI_gpst_CurDD->profilingInformation.numBatches++;
		OGL_CALL( glBegin( GL_TRIANGLE_STRIP ) );
		ulColor = 0;
		if ( p_stII->GlobalColor )
		{
			ulColor = p_stII->GlobalColor;
		}
		else
		{
			ulColor = *pColors++;
		}
		OGL_CALL( glColor4ubv( ( GLubyte * ) &ulColor ) );

		while ( Counter-- )
		{
			//ULONG LocalColor ;

			MATH_SubVector( ( MATH_tdst_Vector * ) &stTrpeze, ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 1 ] );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert_Trpz[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &stTrpeze, fTrapezeInc );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &u_4Vert_Trpz[ 1 ], ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], ( MATH_tdst_Vector * ) &stTrpeze, -fTrapezeInc );
			fTrapezeInc += fTrapezeDelta;
			tf_UV.u = TexUBase;
			glTexCoord2fv( ( float * ) &tf_UV );
			glVertex3fv( ( float * ) &u_4Vert_Trpz[ 0 ] );
			tf_UV.u = TexUBase + TexUMax;
			glTexCoord2fv( ( float * ) &tf_UV );
			glVertex3fv( ( float * ) &u_4Vert_Trpz[ 1 ] );
			tf_UV.v += DeltaU;
			MATH_BlendVector( ( MATH_tdst_Vector * ) &stDerivative, ( MATH_tdst_Vector * ) &stNormale, ( MATH_tdst_Vector * ) &stGravity, Interpolator );
			MATH_AddVector( ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &u_4Vert[ 0 ], ( MATH_tdst_Vector * ) &stDerivative );
			MATH_AddVector( ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], ( MATH_tdst_Vector * ) &u_4Vert[ 1 ], ( MATH_tdst_Vector * ) &stDerivative );
			Interpolator += InterpolatorIntensity;
			Interpolator = fMin( Interpolator, 1.0f );
			/*
			fColorBlenderAcc += fColorBlenderAdd;
			LocalColor = OGL_ScaleColor(fColorBlenderAcc , ulColor );
			glColor4ubv((GLubyte *) &LocalColor);//*/
		}
		glEnd();
	}
}


void OGL_l_DrawSPG2_SPRITES_2X(
        SOFT_tdst_AVertex *Coordinates,
        GEO_Vertex *XCamera,
        GEO_Vertex *YCamera,
        ULONG *pColors,
        ULONG ulnumberOfPoints,
        ULONG ulNumberOfSprites,
        float CosAlpha2,
        float SinAlpha2,
        float SpriteGenRadius,
        float fEOHP,
        float fRatio,
        SOFT_tdst_AVertex *pWind,
        SPG2_InstanceInforamtion *p_stII )
{

	ULONG BM;
	ULONG Counter;
	float OoNSPR;
	float ComplexRadius_RE;
	float ComplexRadius_IM;
	float ComplexRadius_LOCAL;
	SOFT_tdst_AVertex XCam, YCam;
	BM = 0;


	*( MATH_tdst_Vector * ) &XCam = *XCamera;
	*( MATH_tdst_Vector * ) &YCam = *YCamera;

	ComplexRadius_IM = 1.0f;
	ComplexRadius_RE = 0.0f;

	ulNumberOfSprites &= 63;
	if ( !ulNumberOfSprites ) return;
	OoNSPR = 1.0f / ( float ) ulNumberOfSprites;


	while ( ulnumberOfPoints-- )
	{
		SOFT_tdst_AVertex VC, Xa, Ya, Za, LocalX, LocalY, stGravity;
		float Interpolator, InterpolatorIntensity, fdEPT;
		ULONG DiffuseColor;
		VC = *( Coordinates++ );
		MATH_AddVector( ( MATH_tdst_Vector * ) &VC, ( MATH_tdst_Vector * ) &VC, &p_stII->GlobalPos );
		Xa                    = *( Coordinates++ );
		Ya                    = *( Coordinates++ );
		stGravity             = pWind[ Coordinates->c ^ p_stII->BaseWind ];
		InterpolatorIntensity = stGravity.w * OoNSPR;

		Za = *( Coordinates++ );
		MATH_AddVector( ( MATH_tdst_Vector * ) &Za, ( MATH_tdst_Vector * ) &Za, &p_stII->GlobalZADD );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &Za, p_stII->GlobalSCale );

		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &Za, Ya.w );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &stGravity, Ya.w );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &Xa, Ya.w * SpriteGenRadius * fRatio );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &Ya, Ya.w * SpriteGenRadius * fRatio );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &Za, OoNSPR );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &stGravity, OoNSPR );


		MATH_SubVector( ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &stGravity, ( MATH_tdst_Vector * ) &Za );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &LocalX, ( float ) ulNumberOfSprites * InterpolatorIntensity * fEOHP * fEOHP / 2.0f );
		MATH_AddScaleVector( ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &Za, fEOHP );
		MATH_AddScaleVector( ( MATH_tdst_Vector * ) &VC, ( MATH_tdst_Vector * ) &VC, ( MATH_tdst_Vector * ) &LocalX, ( float ) ulNumberOfSprites );

		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &Za, 1.0f - fEOHP );
		MATH_ScaleEqualVector( ( MATH_tdst_Vector * ) &stGravity, 1.0f - fEOHP );


		if ( p_stII->GlobalColor )
			DiffuseColor = p_stII->GlobalColor;
		else
			DiffuseColor = *pColors++;

		Counter = ulNumberOfSprites;

		GDI_gpst_CurDD->profilingInformation.numBatches++;
		glBegin( GL_QUADS );

		Interpolator = fEOHP * ( float ) ulNumberOfSprites * InterpolatorIntensity;
		fdEPT        = 0.0f;


		while ( Counter-- )
		{
			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			static float tf_UV[ 5 ] = { 1.0f, 0.0f, 0.0f, 1.0f, 1.0f };
			MATH_tdst_Vector Cx, C0, C1, C2, C3, stDerivative;
			ULONG LocalColor;
			//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

			LocalColor = OGL_ScaleColor( 127.f + fdEPT * 127.f, DiffuseColor );
			fdEPT += OoNSPR;
			glColor4ubv( ( GLubyte * ) &DiffuseColor );


			MATH_AddScaleVector( &Cx, ( MATH_tdst_Vector * ) &VC, ( MATH_tdst_Vector * ) &Xa, ComplexRadius_IM );
			MATH_SubScaleVector( &C0, &Cx, ( MATH_tdst_Vector * ) &XCam, 0.5f );
			MATH_SubScaleVector( &C0, &C0, ( MATH_tdst_Vector * ) &YCam, 0.5f );
			MATH_AddVector( &C1, &C0, ( MATH_tdst_Vector * ) &XCam );
			MATH_AddVector( &C2, &C1, ( MATH_tdst_Vector * ) &YCam );
			MATH_SubVector( &C3, &C2, ( MATH_tdst_Vector * ) &XCam );

			glTexCoord2fv( &tf_UV[ 3 ] );// 00
			glVertex3fv( ( float * ) &C0 );
			glTexCoord2fv( &tf_UV[ 2 ] );// 10
			glVertex3fv( ( float * ) &C1 );
			glTexCoord2fv( &tf_UV[ 1 ] );// 11
			glVertex3fv( ( float * ) &C2 );
			glTexCoord2fv( &tf_UV[ 0 ] );// 01
			glVertex3fv( ( float * ) &C3 );


			MATH_BlendVector( ( MATH_tdst_Vector * ) &stDerivative, ( MATH_tdst_Vector * ) &Za, ( MATH_tdst_Vector * ) &stGravity, Interpolator );
			MATH_AddVector( ( MATH_tdst_Vector * ) &VC, ( MATH_tdst_Vector * ) &VC, ( MATH_tdst_Vector * ) &stDerivative );
			Interpolator += InterpolatorIntensity * ( 1.0f - fEOHP );
			Interpolator = fMin( Interpolator, 1.0f );

			// Helico�dal effect
			MATH_ScaleVector( ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &Xa, CosAlpha );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &Ya, SinAlpha );
			MATH_ScaleVector( ( MATH_tdst_Vector * ) &LocalY, ( MATH_tdst_Vector * ) &Ya, CosAlpha );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &LocalY, ( MATH_tdst_Vector * ) &LocalY, ( MATH_tdst_Vector * ) &Xa, -SinAlpha );
			Xa = LocalX;
			Ya = LocalY;

			// Helico�dal effect
			MATH_ScaleVector( ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &XCam, CosAlpha2 );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &LocalX, ( MATH_tdst_Vector * ) &YCam, SinAlpha2 );
			MATH_ScaleVector( ( MATH_tdst_Vector * ) &LocalY, ( MATH_tdst_Vector * ) &YCam, CosAlpha2 );
			MATH_AddScaleVector( ( MATH_tdst_Vector * ) &LocalY, ( MATH_tdst_Vector * ) &LocalY, ( MATH_tdst_Vector * ) &XCam, -SinAlpha2 );
			XCam = LocalX;
			YCam = LocalY;


			ComplexRadius_LOCAL = ComplexRadius_IM * CosAlpha + ComplexRadius_RE * SinAlpha;
			ComplexRadius_RE    = ComplexRadius_IM * SinAlpha - ComplexRadius_RE * CosAlpha;
			ComplexRadius_IM    = ComplexRadius_LOCAL;
		}

		glEnd();
	}
}

#define CosAlpha -0.34202014332566873304409961468226f
#define SinAlpha 0.9396926207859083840541092773247f

#define U32 ULONG

extern "C" void OGL_l_DrawSPG2( SPG2_CachedPrimitivs *pCachedLine,
                                ULONG *ulTextureID,
                                GEO_Vertex *XCam,
                                GEO_Vertex *YCam,
                                SPG2_tdst_Modifier *_pst_SPG2,
                                SOFT_tdst_AVertex *pWind,
                                SPG2_InstanceInforamtion *p_stII )
{
	OGL_tdst_SpecificData *pst_SD;
	float fExtractionOfHorizontalPlane;
	ULONG BM, Transparency, Transparency2;
	ULONG ulnumberOfPoints;
	ULONG NumberOfSegments;

	// Droolie start
	BM                           = 0;
	Transparency                 = 0;
	Transparency2                = 0;
	pst_SD                       = NULL;
	fExtractionOfHorizontalPlane = 0.0;
	ulnumberOfPoints             = 0;
	NumberOfSegments             = 0;
	// Droolie end

#if defined( _XENON_RENDER_PC )
	// SC: No SPG2 rendering when not using the OpenGL renderer
	if ( !OGL_gb_Init )
		return;
#endif

	ulnumberOfPoints = pCachedLine->a_PtrLA2 >> 2;
	NumberOfSegments = _pst_SPG2->NumberOfSegments;
	if ( NumberOfSegments >> 1 )
	{
		NumberOfSegments = ( ULONG ) ( ( float ) NumberOfSegments * p_stII->Culling );
		if ( NumberOfSegments <= 2 ) NumberOfSegments = 2;
	}

	/* Xaxis lookat ***********************/
	if ( ( _pst_SPG2->ulFlags & SPG2_XAxisIsInlookat ) && ( pCachedLine->ulFlags & 2 ) )
	{
		ULONG Counter;
		SOFT_tdst_AVertex *Coordinates;
		MATH_tdst_Vector stCameraDir;
		_pst_SPG2->ulFlags &= ~( SPG2_DrawY | SPG2_RotationNoise );
		_pst_SPG2->ulFlags |= SPG2_DrawX;
		pCachedLine->ulFlags &= ~2;
		Coordinates = pCachedLine->a_PointLA2;
		Counter     = ulnumberOfPoints;
		MATH_CrossProduct( &stCameraDir, ( MATH_tdst_Vector * ) YCam, ( MATH_tdst_Vector * ) XCam );
		while ( Counter-- )
		{
			MATH_tdst_Vector LocalX;
			MATH_CrossProduct( &LocalX, &stCameraDir, ( MATH_tdst_Vector * ) ( Coordinates + 3 ) );
			MATH_NormalizeEqualVector( &LocalX );
			*( MATH_tdst_Vector * ) ( Coordinates + 2 ) = LocalX;
			Coordinates += 4;
		}
	}
	else
		/* "Random" rotation ***********************/
		if ( ( _pst_SPG2->ulFlags & SPG2_RotationNoise ) && ( pCachedLine->ulFlags & 1 ) )
		{
			ULONG Counter;
			float CosV, SinV, Swap;
			SOFT_tdst_AVertex *Coordinates;
			CosV        = 1.0f;
			SinV        = 0.0f;
			Coordinates = pCachedLine->a_PointLA2;
			Counter     = ulnumberOfPoints;
			while ( Counter-- )
			{
				MATH_tdst_Vector LocalX, LocalY;
				Swap = CosAlpha * CosV + SinAlpha * SinV;
				SinV = SinAlpha * CosV - CosAlpha * SinV;
				CosV = Swap;
				MATH_ScaleVector( &LocalX, ( MATH_tdst_Vector * ) ( Coordinates + 1 ), CosV );
				MATH_AddScaleVector( &LocalX, &LocalX, ( MATH_tdst_Vector * ) ( Coordinates + 2 ), SinV );
				MATH_ScaleVector( &LocalY, ( MATH_tdst_Vector * ) ( Coordinates + 2 ), CosV );
				MATH_AddScaleVector( &LocalY, &LocalY, ( MATH_tdst_Vector * ) ( Coordinates + 1 ), -SinV );
				*( MATH_tdst_Vector * ) ( Coordinates + 1 ) = LocalX;
				*( MATH_tdst_Vector * ) ( Coordinates + 2 ) = LocalY;
				Coordinates += 4;
			}
		}

	pst_SD = ( OGL_tdst_SpecificData * ) GDI_gpst_CurDD->pv_SpecificData;
	OGL_RS_CullFace( &pst_SD->st_RS, 0 );
	OGL_RS_DepthTest( &pst_SD->st_RS, GDI_Cul_DM_ZTest );

	if ( _pst_SPG2->ulFlags & SPG2_ModeAdd )
		MAT_SET_FLAG( BM, MAT_Cul_Flag_Bilinear | MAT_Cul_Flag_HideAlpha | MAT_Cul_Flag_TileU | MAT_Cul_Flag_TileV );
	else
		MAT_SET_FLAG( BM, MAT_Cul_Flag_Bilinear | MAT_Cul_Flag_AlphaTest | MAT_Cul_Flag_TileU | MAT_Cul_Flag_TileV );

	// DRL: Add this from final game code
	if ( _pst_SPG2->ulFlags1 & SPG2_HideAlpha )
		MAT_SET_FLAG( BM, MAT_GET_FLAG( BM ) | MAT_Cul_Flag_HideAlpha );

	MAT_SET_Blending( BM, MAT_Cc_Op_Copy );
	MAT_SET_AlphaTresh( BM, _pst_SPG2->AlphaThreshold );

	fExtractionOfHorizontalPlane = _pst_SPG2->fExtractionOfHorizontalPlane + 0.5f;

#ifdef ACTIVE_EDITORS
	if ( SPG2_PrimitivCounter + ulnumberOfPoints > SPG2_PrimitivLimit )
		ulnumberOfPoints = SPG2_PrimitivLimit - SPG2_PrimitivCounter;
	if ( !ulnumberOfPoints ) return;
	SPG2_PrimitivCounter += ulnumberOfPoints;

#endif
	Transparency  = MAT_Cc_Op_Copy;
	Transparency2 = MAT_Cc_Op_Add;
	if ( _pst_SPG2->ulFlags & SPG2_DrawinAlpha )
	{
		Transparency  = MAT_Cc_Op_Alpha;
		Transparency2 = MAT_Cc_Op_Add;
		MAT_SET_FLAG( BM, MAT_GET_FLAG( BM ) | MAT_Cul_Flag_NoZWrite );
	}

	if ( _pst_SPG2->ulFlags & SPG2_ModeAdd )
	{
		Transparency  = MAT_Cc_Op_AlphaPremult;
		Transparency2 = MAT_Cc_Op_AlphaPremult;
		MAT_SET_FLAG( BM, MAT_GET_FLAG( BM ) | MAT_Cul_Flag_NoZWrite );
		OGL_RS_Fogged( &pst_SD->st_RS, 0 );
	}
	else
		OGL_RS_Fogged( &pst_SD->st_RS, ( ( GDI_gpst_CurDD->ul_CurrentDrawMask & GDI_Cul_DM_Fogged ) && ( pst_SD->ulFogState ) ) );


	if ( _pst_SPG2->ulFlags & SPG2_DrawHat )
	{
		MAT_SET_Blending( BM, Transparency );
		OGL_SetTextureBlending( ulTextureID[ 0 ], BM );
		OGL_l_DrawSPG2_2X( pCachedLine->a_PointLA2, pCachedLine->a_ColorLA2, ulnumberOfPoints, NumberOfSegments, _pst_SPG2->fTrapeze, fExtractionOfHorizontalPlane, _pst_SPG2->f_GlobalRatio, _pst_SPG2->TileNumber, 2, pWind, p_stII );
		if ( !GDI_gpst_CurDD->GlobalMul2X )
		{
			MAT_SET_Blending( BM, Transparency2 );
			OGL_SetTextureBlending( ulTextureID[ 0 ], BM );
			OGL_l_DrawSPG2_2X( pCachedLine->a_PointLA2, pCachedLine->a_ColorLA2, ulnumberOfPoints, NumberOfSegments, _pst_SPG2->fTrapeze, fExtractionOfHorizontalPlane, _pst_SPG2->f_GlobalRatio, _pst_SPG2->TileNumber, 2, pWind, p_stII );
		}
	}

	if ( _pst_SPG2->ulFlags & SPG2_DrawY )
	{
		MAT_SET_Blending( BM, Transparency );
		OGL_SetTextureBlending( ulTextureID[ 1 ], BM );
		OGL_l_DrawSPG2_2X( pCachedLine->a_PointLA2, pCachedLine->a_ColorLA2, ulnumberOfPoints, NumberOfSegments, _pst_SPG2->fTrapeze, fExtractionOfHorizontalPlane, _pst_SPG2->f_GlobalRatio, _pst_SPG2->TileNumber, 0, pWind, p_stII );
		if ( !GDI_gpst_CurDD->GlobalMul2X )
		{
			MAT_SET_Blending( BM, Transparency2 );
			OGL_SetTextureBlending( ulTextureID[ 1 ], BM );
			OGL_l_DrawSPG2_2X( pCachedLine->a_PointLA2, pCachedLine->a_ColorLA2, ulnumberOfPoints, NumberOfSegments, _pst_SPG2->fTrapeze, fExtractionOfHorizontalPlane, _pst_SPG2->f_GlobalRatio, _pst_SPG2->TileNumber, 0, pWind, p_stII );
		}
	}

	if ( _pst_SPG2->ulFlags & SPG2_DrawX )
	{
		MAT_SET_Blending( BM, Transparency );
		OGL_SetTextureBlending( ulTextureID[ 2 ], BM );
		OGL_l_DrawSPG2_2X( pCachedLine->a_PointLA2, pCachedLine->a_ColorLA2, ulnumberOfPoints, NumberOfSegments, _pst_SPG2->fTrapeze, fExtractionOfHorizontalPlane, _pst_SPG2->f_GlobalRatio, _pst_SPG2->TileNumber, 1, pWind, p_stII );
		if ( !GDI_gpst_CurDD->GlobalMul2X )
		{
			MAT_SET_Blending( BM, Transparency2 );
			OGL_SetTextureBlending( ulTextureID[ 2 ], BM );
			OGL_l_DrawSPG2_2X( pCachedLine->a_PointLA2, pCachedLine->a_ColorLA2, ulnumberOfPoints, NumberOfSegments, _pst_SPG2->fTrapeze, fExtractionOfHorizontalPlane, _pst_SPG2->f_GlobalRatio, _pst_SPG2->TileNumber, 1, pWind, p_stII );
		}
	}

	if ( _pst_SPG2->ulFlags & SPG2_DrawSprites )
	{
		float CA, SA;
		if ( _pst_SPG2->ulFlags & SPG2_SpriteRotation )
		{
			CA = CosAlpha;
			SA = SinAlpha;
		}
		else
		{
#ifdef JADEFUSION
			CA = -1.0f;
#else
			CA = 1.0f;
#endif
			SA = 0.0f;
		}
		MAT_SET_Blending( BM, Transparency );
		OGL_SetTextureBlending( ulTextureID[ 3 ], BM );
		OGL_l_DrawSPG2_SPRITES_2X( pCachedLine->a_PointLA2, XCam, YCam, pCachedLine->a_ColorLA2, ulnumberOfPoints, _pst_SPG2->NumberOfSprites, CA, SA, _pst_SPG2->f_SpriteGeneratorRadius, fExtractionOfHorizontalPlane, _pst_SPG2->f_GlobalRatio, pWind, p_stII );
		if ( !GDI_gpst_CurDD->GlobalMul2X )
		{
			MAT_SET_Blending( BM, Transparency2 );
			OGL_SetTextureBlending( ulTextureID[ 3 ], BM );
			OGL_l_DrawSPG2_SPRITES_2X( pCachedLine->a_PointLA2, XCam, YCam, pCachedLine->a_ColorLA2, ulnumberOfPoints, _pst_SPG2->NumberOfSprites, CA, SA, _pst_SPG2->f_SpriteGeneratorRadius, fExtractionOfHorizontalPlane, _pst_SPG2->f_GlobalRatio, pWind, p_stII );
		}
	}
}

void OGL_l_DrawSPG2_Alpha(
        SOFT_tdst_AVertex *Coordinates,
        ULONG *pColors,
        ULONG ulTextureID,
        ULONG ulnumberOfPoints,
        ULONG ulNumberOfSegments,
        ULONG AlphaT,
        float fTrapeze,
        ULONG TileNumber,
        ULONG ulMode )
{
}

void OGL_l_DrawSPG2_SPRITES(
        SOFT_tdst_AVertex *Coordinates,
        GEO_Vertex *XCam,
        GEO_Vertex *YCam,
        ULONG *pColors,
        ULONG ulTextureID,
        ULONG ulnumberOfPoints,
        ULONG ulNumberOfSprites,
        ULONG AlphaT )
{
	/*	OGL_l_DrawSPG2_SPRITES_2X(Coordinates,XCam,YCam,pColors,ulTextureID,ulnumberOfPoints,ulNumberOfSprites,AlphaT,MAT_Cc_Op_Copy);
	OGL_l_DrawSPG2_SPRITES_2X(Coordinates,XCam,YCam,pColors,ulTextureID,ulnumberOfPoints,ulNumberOfSprites,AlphaT,MAT_Cc_Op_Add);*/
}

/**********************************************************************************************************************/
/* SPG2 specific functions END ****************************************************************************************/
/**********************************************************************************************************************/


/*$4
 ***********************************************************************************************************************
    Private function
 ***********************************************************************************************************************
 */

/* Aim: Set Device Context pixel format */

static bool CreateFakeContext( int *maxAASamples )
{
	WNDCLASSA windowClass     = {};
	windowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc   = DefWindowProcA;
	windowClass.hInstance     = GetModuleHandle( 0 );
	windowClass.lpszClassName = "DummyWGLClass";

	if ( !RegisterClassA( &windowClass ) )
	{
		MessageBox( NULL, "Failed to register dummy OpenGL class!", "OpenGL warning", MB_OK | MB_ICONWARNING | MB_TASKMODAL );
		return false;
	}

	HWND dummyWindow = CreateWindowExA(
	        0,
	        windowClass.lpszClassName,
	        "Dummy WGL Window",
	        0,
	        CW_USEDEFAULT, CW_USEDEFAULT,
	        CW_USEDEFAULT, CW_USEDEFAULT,
	        0,
	        0,
	        windowClass.hInstance,
	        0 );
	if ( dummyWindow == NULL )
	{
		MessageBox( NULL, "Failed to create dummy OpenGL window!", "OpenGL warning", MB_OK | MB_ICONWARNING | MB_TASKMODAL );
		return false;
	}

	static const PIXELFORMATDESCRIPTOR pfd =
	        {
	                sizeof( PIXELFORMATDESCRIPTOR ),
	                1,
	                PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,// Flags
	                PFD_TYPE_RGBA,                                             // The kind of framebuffer. RGBA or palette.
	                24,                                                        // Colordepth of the framebuffer.
	                0, 0, 0, 0, 0, 0,
	                0,
	                0,
	                0,
	                0, 0, 0, 0,
	                32,// Number of bits for the depthbuffer
	                8, // Number of bits for the stencilbuffer
	                0, // Number of Aux buffers in the framebuffer.
	                PFD_MAIN_PLANE,
	                0,
	                0, 0, 0 };

	HDC dc = GetDC( dummyWindow );

	int l_PixelFormat = ChoosePixelFormat( dc, &pfd );
	SetPixelFormat( dc, l_PixelFormat, &pfd );

	HGLRC fakeContext = wglCreateContext( dc );
	wglMakeCurrent( dc, fakeContext );

	glGetIntegerv( GL_MAX_SAMPLES, ( GLint * ) maxAASamples );

	GLenum err = glewInit();

	wglMakeCurrent( dc, 0 );
	wglDeleteContext( fakeContext );

	ReleaseDC( dummyWindow, dc );

	DestroyWindow( dummyWindow );
	UnregisterClass( "DummyWGLClass", windowClass.hInstance );

	if ( err != GLEW_OK )
	{
		char tmp[ 256 ];
		snprintf( tmp, sizeof( tmp ), "Failed to initialize GLEW: %s", glewGetErrorString( err ) );
		MessageBox( NULL, tmp, "OpenGL warning", MB_OK | MB_ICONWARNING | MB_TASKMODAL );
		return false;
	}

	return true;
}

/*
 =======================================================================================================================
 =======================================================================================================================
 */
static bool OGL_SetDCPixelFormat( HDC _hDC, int maxAASamples )
{
	// rewritten ~hogsy

	int pixelFormat;
	if ( WGLEW_ARB_pixel_format )
	{
		const int attribList[] =
		        {
		                WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		                WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		                WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		                WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		                WGL_COLOR_BITS_ARB, 24,
		                WGL_DEPTH_BITS_ARB, 32,
		                WGL_STENCIL_BITS_ARB, 8,
		                WGL_SAMPLE_BUFFERS_ARB, maxAASamples > 1 ? 1 : 0,
		                WGL_SAMPLES_ARB, maxAASamples > 4 ? 4 : maxAASamples,
		                0,// End
		        };

		unsigned int numFormats;
		wglChoosePixelFormatARB( _hDC, attribList, 0, 1, &pixelFormat, &numFormats );
		if ( numFormats > 0 )
		{
			PIXELFORMATDESCRIPTOR pfd;
			DescribePixelFormat( _hDC, pixelFormat, sizeof( pfd ), &pfd );
			if ( SetPixelFormat( _hDC, pixelFormat, &pfd ) )
				return true;
		}
	}

	// if this fails, attempt fallback ~hogsy

	PIXELFORMATDESCRIPTOR pfd =
	        {
	                sizeof( PIXELFORMATDESCRIPTOR ), /* Size of this structure */
	                1,                               /* Version of this structure */
	                PFD_DRAW_TO_WINDOW |             /* Draw to Window (not to bitmap) */
	                        PFD_SUPPORT_OPENGL |     /* Support OpenGL calls in window */
	                        PFD_DOUBLEBUFFER |       /* Double buffered */
	                        PFD_SWAP_EXCHANGE | PFD_GENERIC_ACCELERATED,
	                PFD_TYPE_RGBA,    /* RGBA Color mode */
	                24,               /* Want 24bit color */
	                0, 0, 0, 0, 0, 0, /* Not used to select mode */
	                1, 0,             /* Not used to select mode */
	                0, 0, 0, 0, 0,    /* Accumulation buffer */
	                32,               /* Size of depth buffer */
	                0,                /* Not used to select mode */
	                0,                /* Not used to select mode */
	                PFD_MAIN_PLANE,   /* Draw in main plane */
	                0,                /* Not used to select mode */
	                0, 0, 0           /* Not used to select mode */
	        };

	pixelFormat = ChoosePixelFormat( _hDC, &pfd );
	SetPixelFormat( _hDC, pixelFormat, &pfd );

#ifdef ACTIVE_EDITORS
	static bool first = true;
	DescribePixelFormat( _hDC, pixelFormat, sizeof( pfd ), &pfd );
	if ( ( pfd.cColorBits < 24 ) && ( first ) )
	{
		MessageBox(
		        NULL,
		        "Your desktop must be configured in at least 24bit mode (True colors) for making OPENGL working properly.. \n\n"
		        "Some graphics features will not be enabled \n\n"
		        "Jade must be restarted for taking effect of your eventual modification.",
		        "OpenGL warning",
		        MB_OK | MB_ICONWARNING | MB_TASKMODAL );
		first = false;
	}
#endif

	return true;
}

/*
 =======================================================================================================================
    Aim:    Setup rendering context
 =======================================================================================================================
 */
void OGL_SetupRC( OGL_tdst_SpecificData *_pst_SD )
{
	/*~~~~~~~~~*/
	LONG w, h;
	/*~~~~~~~~~*/

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	OGL_CALL( glClearColor( 1.0f, .5f, .25f, 1.0f ) );

	w = _pst_SD->rcViewportRect.right - _pst_SD->rcViewportRect.left;
	h = _pst_SD->rcViewportRect.bottom - _pst_SD->rcViewportRect.top;
	OGL_CALL( glViewport( 0, ( h - w ) / 2, w, w ) );

	/* Reset coordinate system */
	OGL_CALL( glMatrixMode( GL_PROJECTION ) );

	/* Establish clipping volume (left, right, bottom, top, near, far) */
	_pst_SD->pst_ProjMatrix->Jy  = -1.0f;
	_pst_SD->pst_ProjMatrix->Sz  = 1.0f;
	_pst_SD->pst_ProjMatrix->T.z = -0.1f;
	_pst_SD->pst_ProjMatrix->w   = 0.0f;// MATRIX W!

	OGL_CALL( glLoadMatrixf( ( float * ) ( _pst_SD->pst_ProjMatrix ) ) );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

	glEnable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE ); /* Do not calculate inside of solid object */
	glCullFace( GL_BACK );
	glDepthFunc( GL_LEQUAL );
	glDepthMask( GL_TRUE );
	glFrontFace( GL_CCW );

	glDisable( GL_MULTISAMPLE );

	//glSampleCoverage( 0.5f, GL_FALSE );

	OGL_InitAllShadows();

	/*$F
    glPixelTransferf(GL_RED_SCALE, 4.0f );
    glPixelTransferf(GL_GREEN_SCALE, 4.0f );
    glPixelTransferf(GL_BLUE_SCALE, 4.0f );
    glPixelTransferf(GL_ALPHA_SCALE, 4.0f );
    */
}
