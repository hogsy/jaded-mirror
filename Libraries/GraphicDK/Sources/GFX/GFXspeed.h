/*$T GFXsmoke.h GC! 1.081 09/21/00 11:59:44 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


#ifndef PSX2_TARGET
#pragma once
#endif
#ifndef __GFXSPEED_H__
#define __GFXSPEED_H__

#include "BASe/BAStypes.h"

#if defined (__cplusplus) && !defined(JADEFUSION)
extern "C"
{
#endif

/*$4
 ***********************************************************************************************************************
    Structures
 ***********************************************************************************************************************
 */

typedef struct	GFX_tdst_Speed_
{
	MATH_tdst_Vector	st_Pos;
    float               f_Time;
	float				f_Size;
    ULONG               ul_Color;
    MATH_tdst_Vector    st_OldPos;
} GFX_tdst_Speed;

/*$4
 ***********************************************************************************************************************
    Functions
 ***********************************************************************************************************************
 */

void	*GFX_Speed_Create(void);
int     GFX_i_Speed_Render(void *);

#if defined (__cplusplus) && !defined(JADEFUSION)
}
#endif
#endif /* __GFXSPEED_H__ */
