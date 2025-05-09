/**********************************************************************
 *<
	FILE: maxtess.h

	DESCRIPTION: Tessellation Approximation class

	CREATED BY: Charles Thaeler

	HISTORY: created 12 Dec 1996

 *>	Copyright (c) 1996, All Rights Reserved.
 **********************************************************************/

#ifndef MAXTESS_H
#define MAXTESS_H

typedef enum {
	TESS_SET,		// This is the old MAX form for Bezier Patches
	TESS_ISO,		// This is obsolete and should not be used.
	TESS_PARAM,
	TESS_SPATIAL,
	TESS_CURVE,
	TESS_LDA,
	TESS_REGULAR
} TessType;

typedef enum {
	ISO_ONLY,
	ISO_AND_MESH,
	MESH_ONLY
} ViewConfig;

typedef enum {
	SUBDIV_TREE,
	SUBDIV_GRID
} TessSubdivStyle;

class TessApprox {
public:
	TessType type;
	ViewConfig vpt_cfg;
	TessSubdivStyle subdiv;
	BOOL view;
	float merge;
	int u, v;
	int u_iso, v_iso;
	float ang, dist, edge;
	int minSub, maxSub;

	UtilExport TessApprox();
	UtilExport TessApprox(const TessApprox &tess);
	UtilExport TessApprox & operator=(const TessApprox& tess);
	UtilExport int operator==(const TessApprox &tess) const;


	UtilExport IOResult Load(ILoad* iload);
	UtilExport IOResult Save(ISave* isave);
};

#endif