/**********************************************************************
 *<
	FILE: matrix3.h

	DESCRIPTION: Class definitions for Matrix3

	CREATED BY: Dan Silva

	HISTORY:

 *>	Copyright (c) 1994, All Rights Reserved.
 **********************************************************************/
#ifndef _MATRIX3_H 

#define _MATRIX3_H 

#include "ioapi.h"
#include "point3.h"
#include "point4.h"

//Flags
#define POS_IDENT  1
#define ROT_IDENT  2
#define SCL_IDENT  4
#define MAT_IDENT (POS_IDENT|ROT_IDENT|SCL_IDENT)

typedef float MRow[3];

class Quat;

class Matrix3 {
	friend Matrix3 DllExport RotateXMatrix(float angle);   
	friend Matrix3 DllExport RotateYMatrix(float angle); 
	friend Matrix3 DllExport RotateZMatrix(float angle); 
	friend Matrix3 DllExport TransMatrix(const Point3& p);
	friend Matrix3 DllExport ScaleMatrix(const Point3& s);
	friend Matrix3 DllExport RotateYPRMatrix(float Yaw, float Pitch, float Roll);
	friend Matrix3 DllExport RotAngleAxisMatrix(Point3& axis, float angle);
	friend Matrix3 DllExport Inverse(const Matrix3& M);
	friend Point3 DllExport operator*(const Matrix3& A, const Point3& V);
	friend Point3 DllExport operator*(const Point3& V, const Matrix3& A);
	friend Point3 DllExport VectorTransform(const Matrix3& M, const Point3& V); 
	friend Matrix3 DllExport XFormMat(const Matrix3& xm, const Matrix3& m);

	friend class Quat;
	float m[4][3];
	// Access i-th row as Point3 for read or assignment:
	Point3& operator[](int i) { return((Point3&)(*m[i]));  }
	Point3& operator[](int i) const { return((Point3&)(*m[i])); }
	DWORD flags;

public:
	// if change any components directly via GetAddr, must call this
	void SetNotIdent() { flags &= ~MAT_IDENT; }
	void SetIdentFlags(DWORD f) { flags &= ~MAT_IDENT; flags |= f; }
	DWORD GetIdentFlags() const { return flags; }
	void ClearIdentFlag(DWORD f) { flags &= ~f; }
	BOOL IsIdentity() { return ((flags&MAT_IDENT)==MAT_IDENT); }
	DllExport void ValidateFlags(); // recomputes the IDENT flags

	// CAUTION: if you change the matrix via this pointer, you MUST clear the
	// proper IDENT flags !!!
	MRow* GetAddr() const { return (MRow *)(m); }

	// Constructors
	Matrix3(){ flags = 0; }	 // NO INITIALIZATION done in this constructor!! 				 
	Matrix3(BOOL init) {flags=0; IdentityMatrix();} // RB: An option to initialize
	DllExport Matrix3(float (*fp)[3]); 

	// Assignment operators
	DllExport Matrix3& operator-=( const Matrix3& M);
	DllExport Matrix3& operator+=( const Matrix3& M); 
	DllExport Matrix3& operator*=( const Matrix3& M);  	// Matrix multiplication

	// Operations on matrix
	DllExport void IdentityMatrix(); 		// Make into the Identity Matrix
	DllExport void Zero();		// set all elements to 0
	
	Point3 GetRow(int i) const { return (*this)[i]; }	
	DllExport void SetRow(int i, Point3 p);

	DllExport Point4 GetColumn(int i);
	DllExport void SetColumn(int i,  Point4 col);
	DllExport Point3 GetColumn3(int i);

	// zero the translation part;
	DllExport void NoTrans();
	// null out the rotation part;
	DllExport void NoRot();
	// null out the scale part;
	DllExport void NoScale();
	
	// This is an "unbiased" orthogonalization
	// It seems to take a maximum of 4 iterations to converge.
	DllExport void Orthogonalize();

	// Access the translation row
	void SetTrans(const Point3 p) { (*this)[3] = p;  flags &= ~POS_IDENT; }
	void SetTrans(int i, float v) { (*this)[3][i] = v; flags &= ~POS_IDENT; }
	Point3 GetTrans() { return (*this)[3]; }
   
	// Apply Incremental transformations to this matrix
	// Equivalent to multiplying on the RIGHT by transform
	DllExport void Translate(const Point3& p);
	DllExport void RotateX(float angle);  
	DllExport void RotateY(float angle);
	DllExport void RotateZ(float angle);
	// if trans = FALSE the translation component is unaffected:
	DllExport void Scale(const Point3& s, BOOL trans = FALSE);

	// Apply Incremental transformations to this matrix
	// Equivalent to multiplying on the LEFT by transform 
	DllExport void PreTranslate(const Point3& p);
	DllExport void PreRotateX(float angle);  
	DllExport void PreRotateY(float angle);
	DllExport void PreRotateZ(float angle);
	// if trans = FALSE the translation component is unaffected:
	DllExport void PreScale(const Point3& s, BOOL trans = FALSE);
		
	DllExport Matrix3 operator*(const Matrix3&) const;
	DllExport Matrix3 operator+(const Matrix3&) const;
	DllExport Matrix3 operator-(const Matrix3&) const;

	DllExport IOResult Save(ISave* isave);
	DllExport IOResult Load(ILoad* iload);

	// Returns FALSE if right handed (normal case) and TRUE if right handed.
	DllExport BOOL Parity() const;
	};


// Build new matrices for transformations
Matrix3 DllExport RotateXMatrix(float angle);   
Matrix3 DllExport RotateYMatrix(float angle); 
Matrix3 DllExport RotateZMatrix(float angle); 
Matrix3 DllExport TransMatrix(const Point3& p);
Matrix3 DllExport ScaleMatrix(const Point3& s);
Matrix3 DllExport RotateYPRMatrix(float Yaw, float Pitch, float Roll);
Matrix3 DllExport RotAngleAxisMatrix(Point3& axis, float angle);
 
Matrix3 DllExport Inverse(const Matrix3& M);  // return Inverse of matrix

// These two versions of transforming a point with a matrix do the same thing,
// regardless of the order of operands (linear algebra rules notwithstanding).
Point3 DllExport operator*(const Matrix3& A, const Point3& V); // Transform Point with matrix
Point3 DllExport operator*(const Point3& V, const Matrix3& A); // Transform Point with matrix


Point3 DllExport VectorTransform(const Matrix3& M, const Point3& V); 

// transform a plane: this only works if M is orthogonal
Point4 DllExport TransformPlane(const Matrix3& M, const Point4& plin); 

// transformats matrix m so it is applied in the space of matrix xm:
//  returns xm*m*Inverse(xm)
Matrix3 DllExport XFormMat(const Matrix3& xm, const Matrix3& m);

#endif _MATRIX3_H 
