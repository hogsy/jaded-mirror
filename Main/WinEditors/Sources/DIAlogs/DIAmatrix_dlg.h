/*$T DIAmatrix_dlg.h GC! 1.098 12/05/00 14:04:11 */


/*$6
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */


#pragma once
#ifdef ACTIVE_EDITORS
#include "MATHs/MATH.h"
#include "DIAlogs/DIAbase.h"

/*$4
 ***********************************************************************************************************************
 ***********************************************************************************************************************
 */

class EDIA_cl_MatrixDialog : public EDIA_cl_BaseDialog
{

/*$2
 -----------------------------------------------------------------------------------------------------------------------
    CONSTRUCT.
 -----------------------------------------------------------------------------------------------------------------------
 */

public:
	EDIA_cl_MatrixDialog(void);

/*$2
 -----------------------------------------------------------------------------------------------------------------------
    ATTRIBUTES.
 -----------------------------------------------------------------------------------------------------------------------
 */

public:
	MATH_tdst_Matrix	*mpst_Matrix;
	MATH_tdst_Matrix	mst_SavedMatrix;
    MATH_tdst_Vector    mst_Rotation;
    BOOL                mb_First;

/*$2
 -----------------------------------------------------------------------------------------------------------------------
    OVERWRITE.
 -----------------------------------------------------------------------------------------------------------------------
 */

public:
	void	MakeRotation(void);
    void	MakeRotationWithXYZ(void);
	void	DoDataExchange(CDataExchange *);
	BOOL	OnInitDialog(void);

/*$2
 -----------------------------------------------------------------------------------------------------------------------
    MESSAGE MAP.
 -----------------------------------------------------------------------------------------------------------------------
 */

protected:
	void	OnAngleH(void);
	void	OnAngleV(void);
    void	OnAngleX(void);
    void	OnAngleY(void);
    void	OnAngleZ(void);

	afx_msg void	OnDestroy(void);

	DECLARE_MESSAGE_MAP()
};
#endif /* ACTIVE_EDITORS */
