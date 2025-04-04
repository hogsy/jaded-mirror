/*$T BROframe_vss.cpp GC!1.71 02/08/00 13:59:29 */

/*
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 */

#include "Precomp.h"
#ifdef ACTIVE_EDITORS
#include "LINKs/LINKmsg.h"
#include "LINKs/LINKtoed.h"
#include "BASe/CLIbrary/CLIstr.h"
#include "BASe/ERRors/ERRasser.h"
#include "BIGfiles/BIGmdfy_file.h"
#include "BIGfiles/BIGmdfy_dir.h"
#include "BIGfiles/BIGfat.h"
#include "BIGfiles/BIGopen.h"
#include "AIFrame.h"

#include "EDItors/Sources/BROwser/BROframe.h"

#include <assert.h>

//////////////////////////////////////////////////////////////////////////
//

//////////////////////////////////////////////////////////////////////////
//
//------------------------------------------------------------
//   void EAI_cl_Frame::OnPerforceEditModel()
/// \author    YCharbonneau
/// \date      2005-01-17
/// \par       Description: 
///            No description available ...
/// \param     No param description available... 
/// \see 
//------------------------------------------------------------
void EAI_cl_Frame::RunCommandOnFiles( int nCommand )
{
	if ( mul_CurrentEditModel == BIG_C_InvalidIndex ) 
		return;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
	char	asz_Path[BIG_C_MaxLenPath];
	char	asz_Path1[BIG_C_MaxLenPath];
	char	asz_Name[BIG_C_MaxLenPath];
	char	asz_Name1[BIG_C_MaxLenPath];
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

	AfxGetApp()->DoWaitCursor(1);
	BIG_ComputeFullName(BIG_ParentFile(mul_CurrentEditModel), asz_Path);
	L_strcpy(asz_Path1, asz_Path);
	L_strcpy(asz_Name, BIG_NameFile(mul_CurrentEditModel));
	L_strcpy(asz_Name1, BIG_NameFile(mul_CurrentEditModel));
	*L_strrchr(asz_Name, '.') = 0;
	L_strcat(asz_Path, "/");
	L_strcat(asz_Path, asz_Name);
	BIG_INDEX ulDir = BIG_ul_SearchDir(asz_Path);
	
	if ( ulDir == BIG_C_InvalidIndex ) 
	{	
		AfxGetApp()->DoWaitCursor(-1);
		return;
	}

	RefreshDialogBar();
	DisplayPaneNames();
	AfxGetApp()->DoWaitCursor(-1);
}

#endif /* ACTIVE_EDITORS */
