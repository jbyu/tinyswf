/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __TS_READER_H__
#define __TS_READER_H__

#include "tsTypes.h"
#include <string.h>

namespace tinyswf {
	
class Reader {
public:
	Reader( char* data, int32_t sz )
		:	_data( data )
		,	_sz( sz )
		,	_cur( 0 )
		,	_bitpos( 0 )
	{}
		
	template< class T> T get() {
		if( _bitpos > 0 && _bitpos < 8)
			_cur++;
		_bitpos = 0;
		T *ret = (T*)(_data + _cur);
		_cur += sizeof( T );
		return *ret;
	}
		
	inline void skip( int32_t bytes ) {
		_cur += bytes;
		_bitpos = 0;
	}
		
	inline void align() {
		if( _bitpos > 0 && _bitpos < 8)
			_cur++;
		_bitpos = 0;
	}
		
	inline uint32_t getbit() {
		uint32_t v = (_data[_cur] >> (7 - _bitpos++)) & 1;
		if( _bitpos == 8 )
			get<uint8_t>();
		return v;
	}
		
	inline uint32_t getbits( uint32_t nbits ) {
		uint32_t val = 0;
		for( uint32_t t = 0; t < nbits; t++ )
		{
			val <<= 1;
			val |= getbit();
		}
		return val;
	}
		
	inline int32_t getsignedbits( uint32_t nbits ) {
		uint32_t res = getbits( nbits );
		if ( res & ( 1 << ( nbits - 1 ) ) ) 
			res |= (0xffffffff << nbits);
				  
		return (int32_t)res;			
	}
		
	inline void getRectangle( RECT& rect ) {
		const uint32_t nbits =  getbits( 5 );
		rect.xmin = getsignedbits( nbits ) * SWF_INV_TWIPS;
		rect.xmax = getsignedbits( nbits ) * SWF_INV_TWIPS;
		rect.ymin = getsignedbits( nbits ) * SWF_INV_TWIPS;
		rect.ymax = getsignedbits( nbits ) * SWF_INV_TWIPS;
	}
		
	inline void getMatrix( MATRIX& m ) {
		int32_t nbits;
		if ( getbits( 1 ) )	{	// has scale
			nbits = getbits( 5 );
			m.sx = (float)getsignedbits( nbits )*SWF_INV_MATRIX;
			m.sy = (float)getsignedbits( nbits )*SWF_INV_MATRIX;
		} else {
			m.sx = m.sy = 1.0f;
		}

		if ( getbits( 1 ) ) { // has rotation
			nbits = getbits( 5 );
			m.r0 = (float)getsignedbits( nbits )*SWF_INV_MATRIX;
			m.r1 = (float)getsignedbits( nbits )*SWF_INV_MATRIX;
		} else {
			m.r0 = m.r1 = 0.0f;	
		}

		nbits = getbits( 5 );
		m.tx = getsignedbits( nbits ) * SWF_INV_TWIPS;
		m.ty = getsignedbits( nbits ) * SWF_INV_TWIPS;
	}

    inline void getColor( COLOR4f& color, uint32_t nbits ) {
		color.r = getsignedbits( nbits ) * SWF_INV_CXFORM;
		color.g = getsignedbits( nbits ) * SWF_INV_CXFORM;
		color.b = getsignedbits( nbits ) * SWF_INV_CXFORM;
		color.a = getsignedbits( nbits ) * SWF_INV_CXFORM;
    }

    inline void getRGBA( RGBA& color ) {
		color.r = get<uint8_t>();
		color.g = get<uint8_t>();
		color.b = get<uint8_t>();
		color.a = get<uint8_t>();
    }

	inline void getCXForm( CXFORM& cx ) {
		const uint32_t has_add_terms = getbits( 1 );
		const uint32_t has_mult_terms = getbits( 1 );
		const uint32_t nbits = getbits( 4 );
		if( has_mult_terms ) {
            getColor( cx.mult, nbits);
		} else {
			COLOR4f color = {1.f,1.f,1.f,1.f};
			cx.mult = color;
		}
		if( has_add_terms ) {
            getColor( cx.add, nbits);
		} else {
			COLOR4f color = {0.f,0.f,0.f,0.f};
			cx.add = color;
		}
    }

	inline int32_t getCurrentPos() const {
		return _cur;
	}
		
	inline uint8_t getBitPos() const {
		return _bitpos;
	}
		
	int32_t getBytes( int32_t s, uint8_t* b ) {
		if ( _cur + s > _sz ) {
			s = _sz - _cur;
			if ( s == 0 ) {
				return 0;
			}
		}
		memcpy( b, &_data[_cur], s );
		_cur += s;
		return s;
	}
		
	void setNewData( char* data, int32_t sz ) {
		_data = data;
		_sz = sz;
		_cur = 0;
		_bitpos = 0;
	}

    inline const char* getString() {
		const char *src = (const char *)&_data[_cur];
		size_t len = strlen(src) + 1;
		_cur += len;
        return src;
        /*
		char *dst = new char[len];
		strcpy(dst, src);
		return( dst );
        */
	}

	void getFilterList();

	float getFIXED() {
		int32_t	val = get<int32_t>();
		return (float) val / 65536.0f;
	}

	float getFIXED8() {
		int16_t	val = get<int16_t>();
		return (float) val / 256.0f;
	}

	const char *getData() const { return _data + _cur; }

private:
	char*		_data;
	int32_t		_sz;		// total size of the buffer
	int32_t		_cur;		// current ptr/idx into buffer
	uint8_t		_bitpos;	// for reading bits
};
	
//	template<> char Reader::get<char>();
}//namespace

#endif // __TS_READER_H__