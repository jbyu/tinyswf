/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "tsHeader.h"
#include "tsReader.h"

#ifdef WIN32
#include <windows.h>
//#define USE_ZLIB
#endif

#ifdef USE_ZLIB
#include "..\zlib\include\zlib.h"
#pragma comment(lib, "../../zlib/lib/zdll.lib")
#endif

using namespace std;

short int endianSwap16(short int val)
{
	long int i = 1;
	const char *p = (const char *) &i;
	if (p[0] == 1)  // Lowest address contains the least significant byte
	{
		return (((val & 0xff00) >> 8) | ((val & 0x00ff) << 8));
	}
	return val;
}

namespace tinyswf {
	
bool Header::read( Reader& reader ) {
	// little endian
	long int i = 0x12345678;
	const char *p = (const char*) &i;
	assert(0x78 == *p);
	
	_signature[0] = reader.get<uint8_t>();
	_signature[1] = reader.get<uint8_t>();
	_signature[2] = reader.get<uint8_t>();
		
	_version = reader.get<uint8_t>();
	assert(5 < _version);
		
	_file_length = reader.get<uint32_t>();
		
	if ( _signature[0] == 'C' ) {
#ifdef USE_ZLIB
		SWF_TRACE("** inflate: %d bytes ***\n", _file_length);
		// uncompressed file
		_outputBuffer = new char[_file_length];
			
		z_stream stream;
		const int MAX_BUFFER = 128 * 1024;
		unsigned char * inputBuffer = new unsigned char[ MAX_BUFFER ];
		int status;
			
		stream.avail_in = 0;
		stream.next_in = inputBuffer;
		stream.next_out = (Bytef*) _outputBuffer;
		stream.zalloc = (alloc_func) NULL;
		stream.zfree = (free_func) NULL;
		stream.opaque = (voidpf) 0;
		stream.avail_out = _file_length;
			
		status = inflateInit( &stream );

		if( status != Z_OK ) {
			fprintf( stderr, "Error decompressing SWF: %s\n", stream.msg );
			delete [] inputBuffer;
			return false;
		}

		do {
			if ( stream.avail_in == 0 ) {
				stream.next_in = inputBuffer;
				stream.avail_in = reader.getBytes( MAX_BUFFER, inputBuffer );
			}
			if ( stream.avail_in == 0 )
				break;
			status = inflate( &stream, Z_SYNC_FLUSH );
		} while( status == Z_OK );
			
        (void)inflateEnd(&stream);
		delete [] inputBuffer;

		if( status != Z_STREAM_END && status != Z_OK ) {
			fprintf( stderr, "Error decompressing SWF: %s\n", stream.msg );
			return false;
		}
		reader.setNewData( _outputBuffer, _file_length );
#else
        return false;
#endif
	}
		
		
	// get the bound rectangle
    reader.getRectangle(_frame_size);
	reader.align();
	uint16_t fr = reader.get<uint16_t>();
	_frame_rate = 1.f/(fr>>8);
	_frame_count = reader.get<uint16_t>(); 
				
	return true;
}
	
void Header::print() {
	SWF_TRACE("signature:\t%c%c%c\n",_signature[0], _signature[1], _signature[2]);
	SWF_TRACE("version:\t\t%d\n",	 _version); 
	SWF_TRACE("file length:\t%d\n",  _file_length);
	SWF_TRACE("frame width:\t%f\n",  getFrameWidth());
	SWF_TRACE("frame height:\t%f\n", getFrameHeight());
	SWF_TRACE("frame rate:\t%f\n",	 _frame_rate);
	SWF_TRACE("frame count:\t%d\n",  _frame_count);
}

#ifdef WIN32
void Log(const char* szmsg, ...) {
    const int kMAXIMUM = 1024;
	static char buffer[kMAXIMUM];
	va_list arglist;
	va_start(arglist, szmsg);
	{
		vsprintf_s(buffer,kMAXIMUM,szmsg,arglist);
		OutputDebugString(buffer);
	}
	va_end(arglist);
}
#endif

}//namespace