/*******************************************************************
 *
 *    DESCRIPTION: euler.h
 *
 *    AUTHOR: Converted from Ken Shoemake's Graphics Gems IV code by Dan Silva
 *
 *    HISTORY:  converted 11/21/96
 *
 *              RB: This file provides only a subset of those
 *                  found in the original Graphics Gems paper.
 *                  All orderings are 'static axis'.
 *
 *******************************************************************/

#ifndef __EULER__
#define __EULER__

#include "matrix3.h"
#include "quat.h"

#define EULERTYPE_XYZ	0
#define EULERTYPE_XZY	1
#define EULERTYPE_YZX	2
#define EULERTYPE_YXZ	3
#define EULERTYPE_ZXY	4
#define EULERTYPE_ZYX	5
#define EULERTYPE_XYX	6
#define EULERTYPE_YZY	7
#define EULERTYPE_ZXZ	8

void DllExport QuatToEuler(const Quat &q, float *ang,int type);
void DllExport EulerToQuat(float *ang, Quat &q,int type);
void DllExport MatrixToEuler(const Matrix3 &mat, float *ang,int type);
void DllExport EulerToMatrix(float *ang, Matrix3 &mat,int type);

#endif // __EULER__

