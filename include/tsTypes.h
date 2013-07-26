/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __TS_TYPES_H__
#define __TS_TYPES_H__

#include <float.h>
#include <stdint.h>
#include <stdio.h>
#include <cassert>
//#include <cmath>
//#include <cstddef>

namespace tinyswf {

//typedef     signed long     fixed_t;
//typedef     signed long     coord_t;
	
#define coord_t_MAX		0x7fffffff
#define coord_t_MIN		-0x80000000
	
const int   SWF_TWIPS		= 20;
const float SWF_INV_TWIPS   = 1.f / SWF_TWIPS;
const float SWF_INV_CXFORM  = 1.f / (1<<8);
const float SWF_INV_MATRIX  = 1.f / (1<<16);
const float SWF_INV_COLOR   = 1.f / 255.f;

// basic structures
struct POINT { 
	float        x;
	float        y;

	bool operator == (const POINT& p) const { return x == p.x && y == p.y; }

	void operator += (const POINT& v) {
		x += v.x; y += v.y;
	}
	void operator -= (const POINT& v) {
		x -= v.x; y -= v.y;
	}
	void operator *= (float a) {
		x *= a; y *= a;
	}

	POINT operator + (const POINT& v) const {
		POINT ret = *this;
		ret += v;
		return ret;
	}

	POINT operator - (const POINT& v) const {
		POINT ret = *this;
		ret -= v;
		return ret;
	}

	POINT operator * (float s) const {
		POINT ret = *this;
		ret *= s;
		return ret;
	}

	float magnitude() const {
		return x * x + y * y;
	}

	float dot(const POINT& pt) const {
		return x*pt.x + y*pt.y;
	}
};

struct RGBA { 
	uint8_t    a;
	uint8_t    r;
	uint8_t    g;
	uint8_t    b;
};

struct COLOR4f {
    float r,g,b,a;

	void operator += (const COLOR4f& o) {
		r += o.r;
		g += o.g;
		b += o.b;
		a += o.a;
	}
	void operator *= (const COLOR4f& o) {
		r *= o.r;
		g *= o.g;
		b *= o.b;
		a *= o.a;
	}
	COLOR4f operator * (const COLOR4f& o) const {
		COLOR4f ret = *this;
		ret *= o;
		return ret;
	}
};
  
const COLOR4f kBLACK = {0,0,0,1};

struct YUV {
	uint8_t	y,u,v;
};
	
struct RECT { 
	float        xmin;
	float        ymin;
	float        xmax;
	float        ymax;

	void addPoint(const POINT& p) {
		if (p.x < xmin) xmin = p.x;
		else if (p.x > xmax) xmax = p.x;
		if (p.y < ymin) ymin = p.y;
		else if (p.y > ymax) ymax = p.y;
	}

	bool isInside(const RECT& other) {
		if ((other.xmax > xmax) &&
			(other.xmin < xmin) &&
			(other.ymax > ymax) &&
			(other.ymin < ymin) )
			return true;
		return false;
	}
};

const RECT kRectangleInvalidate = { FLT_MAX,FLT_MAX,-FLT_MAX,-FLT_MAX };

inline void MergeRectangle(RECT &rect, const RECT other) {
	if (other.xmin < rect.xmin)
		rect.xmin = other.xmin;
	if (other.xmax > rect.xmax)
		rect.xmax = other.xmax;
	if (other.ymin < rect.ymin)
		rect.ymin = other.ymin;
	if (other.ymax > rect.ymax)
		rect.ymax = other.ymax;
}

inline bool IsWithinRectangle(const RECT& rect, float x, float y) {
	if (x < rect.xmin)
		return false;
	if (x > rect.xmax)
		return false;
	if (y < rect.ymin)
		return false;
	if (y > rect.ymax)
		return false;
	return true;
}
		
union MATRIX { 
	struct {
		float  sx, r1, tx;
		float  r0, sy, ty;
	};
	float m_[2][3];

	void setIdentity() {
		sx = 1; r1 = tx = 0;
		sy = 1; r0 = ty = 0;
	}
	void setInverse(const MATRIX& m);
	void transform(POINT& result, const POINT& p) const;
	void transform(RECT& result, const RECT& p) const;
	void scale(float x, float y);
	static MATRIX concatenate(const MATRIX& a, const MATRIX& b);
};

const MATRIX kMatrixIdentity = {
    1,0,0,
    0,1,0
};

inline void MATRIX::scale(float x, float y) {
	m_[0][0] *= x; m_[0][1] *= x; m_[0][2] *= x;
	m_[1][0] *= y; m_[1][1] *= y; m_[1][2] *= y;
}

// concatenate matricies
inline MATRIX MATRIX::concatenate(const MATRIX& a, const MATRIX& b)  {
	MATRIX m;
	m.sx = a.sx * b.sx + a.r0 * b.r1;
	m.r0 = a.sx * b.r0 + a.r0 * b.sy;
	m.r1 = a.r1 * b.sx + a.sy * b.r1;
	m.sy = a.r1 * b.r0 + a.sy * b.sy;
	m.tx = a.tx * b.sx + a.ty * b.r1 + b.tx;
	m.ty = a.tx * b.r0 + a.ty * b.sy + b.ty;
	return m;
}

// Transform point 'p' by our matrix.  Put the result in result.
inline void MATRIX::transform(POINT& result, const POINT& p) const {
	result.x = m_[0][0] * p.x + m_[0][1] * p.y + m_[0][2];
	result.y = m_[1][0] * p.x + m_[1][1] * p.y + m_[1][2];
}

// Set this matrix to the inverse of the given matrix.
inline void MATRIX::setInverse(const MATRIX& m)
{
	// Invert the rotation part.
	float det = m.m_[0][0] * m.m_[1][1] - m.m_[0][1] * m.m_[1][0];
	if (det == 0.0f) {
		// Arbitrary fallback.
		setIdentity();
		m_[0][2] = -m.m_[0][2];
		m_[1][2] = -m.m_[1][2];
	} else {
		float	inv_det = 1.0f / det;
		m_[0][0] = m.m_[1][1] * inv_det;
		m_[1][1] = m.m_[0][0] * inv_det;
		m_[0][1] = -m.m_[0][1] * inv_det;
		m_[1][0] = -m.m_[1][0] * inv_det;

		m_[0][2] = -(m_[0][0] * m.m_[0][2] + m_[0][1] * m.m_[1][2]);
		m_[1][2] = -(m_[1][0] * m.m_[0][2] + m_[1][1] * m.m_[1][2]);
	}
}

struct MATRIX3f {
    float* operator [] ( const int Row ) {
		return &f[Row*3];
	}
	float f[9];	/*!< Array of float */
};

inline void MATRIX3fSet(MATRIX3f &mOut, const MATRIX& mIn)
{
    mOut.f[0] = mIn.sx;
    mOut.f[4] = mIn.sy;
    mOut.f[1] = mIn.r0;
    mOut.f[3] = mIn.r1;
    mOut.f[6] = mIn.tx;
    mOut.f[7] = mIn.ty;
    mOut.f[2] = 0;
    mOut.f[5] = 0;
    mOut.f[8] = 1;
}

inline void MATRIX3fMultiply(
	MATRIX3f		&mOut,
	const MATRIX3f	&mA,
	const MATRIX3f	&mB)
{
	MATRIX3f mRet;
	mRet.f[0] = mA.f[0]*mB.f[0] + mA.f[1]*mB.f[3] + mA.f[2]*mB.f[6];
	mRet.f[1] = mA.f[0]*mB.f[1] + mA.f[1]*mB.f[4] + mA.f[2]*mB.f[7];
	mRet.f[2] = mA.f[0]*mB.f[2] + mA.f[1]*mB.f[5] + mA.f[2]*mB.f[8];

	mRet.f[3] = mA.f[3]*mB.f[0] + mA.f[4]*mB.f[3] + mA.f[5]*mB.f[6];
	mRet.f[4] = mA.f[3]*mB.f[1] + mA.f[4]*mB.f[4] + mA.f[5]*mB.f[7];
	mRet.f[5] = mA.f[3]*mB.f[2] + mA.f[4]*mB.f[5] + mA.f[5]*mB.f[8];

	mRet.f[6] = mA.f[6]*mB.f[0] + mA.f[7]*mB.f[3] + mA.f[8]*mB.f[6];
	mRet.f[7] = mA.f[6]*mB.f[1] + mA.f[7]*mB.f[4] + mA.f[8]*mB.f[7];
	mRet.f[8] = mA.f[6]*mB.f[2] + mA.f[7]*mB.f[5] + mA.f[8]*mB.f[8];
	mOut = mRet;
}

typedef struct _CXFORM {
    COLOR4f mult;
    COLOR4f add;
} CXFORM;


//static inline float degrees (float radians) {return radians * (180.0f/3.14159f);}	
inline void COLOR4fSet(
	COLOR4f		&mOut,
	const COLOR4f	&mIn)
{
    mOut.r = mIn.r;
    mOut.g = mIn.g;
    mOut.b = mIn.b;
    mOut.a = mIn.a;
}
inline void COLOR4fMultiply(
	COLOR4f		&mOut,
	const COLOR4f	&mA,
	const COLOR4f	&mB)
{
    mOut.r = mA.r * mB.r;
    mOut.g = mA.g * mB.g;
    mOut.b = mA.b * mB.b;
    mOut.a = mA.a * mB.a;
}
inline void COLOR4fAdd(
	COLOR4f		&mOut,
	const COLOR4f	&mA,
	const COLOR4f	&mB)
{
    mOut.r = mA.r + mB.r;
    mOut.g = mA.g + mB.g;
    mOut.b = mA.b + mB.b;
    mOut.a = mA.a + mB.a;
}

inline void CXFORMMultiply(
	CXFORM		&mOut,
	const CXFORM	&child,
	const CXFORM	&parent)
{
    COLOR4f add;
    COLOR4fMultiply(add, parent.mult, child.add);
    COLOR4fAdd(mOut.add, parent.add, add);
    COLOR4fMultiply(mOut.mult, child.mult, parent.mult);
}

struct GRADIENT {
	int num;
	uint8_t* ratios;
	RGBA* rgba;
};

const CXFORM kCXFormIdentity = {
    {1,1,1,1},
    {0,0,0,0}
};
const MATRIX3f kMatrix3fIdentity = {
	{1,0,0,
	0,1,0,
	0,0,1}
};

#define SWF_UNUSED_PARAM(unusedparam) (void)unusedparam

void Log(const char* szmsg, ...);

#ifdef SWF_DEBUG
#define SWF_ASSERT	assert
#define SWF_TRACE	Log
#else
	#ifdef WIN32
	#define SWF_ASSERT	(void)
	#define SWF_TRACE	(void)
	#else
	#define SWF_ASSERT(...)
	#define SWF_TRACE(...)
	#endif    
#endif//SWF_DEBUG

}//namespace
#endif	// __TS_TYPES_H__
