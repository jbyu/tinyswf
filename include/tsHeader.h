/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __TS_HEADER_H__
#define __TS_HEADER_H__

#include "tsTypes.h"

namespace tinyswf {

class Reader;
class Header {
public:
	Header() : _outputBuffer(NULL)
	{}

	virtual ~Header() {
		if ( _outputBuffer ) {
			delete [] _outputBuffer;
		}
	}
		
	bool read( Reader& reader );
	void print();
		
    //The FrameSize RECTalways has Xmin and Ymin value of 0;
    /*
	float getFrameWidth() const {return (_frame_size.xmax - _frame_size.xmin);}
	float getFrameHeight() const {return (_frame_size.ymax - _frame_size.ymin);	}
    */
	float   getFrameWidth() const { return _frame_size.xmax; }
	float   getFrameHeight()const { return _frame_size.ymax; }
    float   getFrameRate()  const { return _frame_rate; }
	int     getFrameCount() const { return _frame_count; }
	
private:
	uint8_t		_signature[3];
	uint8_t		_version;
	uint32_t	_file_length;
	RECT		_frame_size;
	float		_frame_rate;
	uint16_t	_frame_count;
	char*		_outputBuffer;		// used for compressed swf files
};

}//namespace

#endif // __TS_HEADER_H__
