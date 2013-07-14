/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __BACKGROUND_H__
#define __BACKGROUND_H__

#include "tsTag.h"

namespace tinyswf
{
	class SetBackgroundColorTag : public ITag {
	public:
        uint8_t r,g,b;

		SetBackgroundColorTag( TagHeader& h ) 
		: ITag( h )
		{}
		
		virtual ~SetBackgroundColorTag()
        {}
		
		virtual bool read( Reader& reader, SWF& swf, MovieFrames& ) {
			r = reader.get<uint8_t>();
			g = reader.get<uint8_t>();
			b = reader.get<uint8_t>();

            COLOR4f &color = swf.getBackgroundColor();
            color.r = r * SWF_INV_COLOR;
            color.g = g * SWF_INV_COLOR;
            color.b = b * SWF_INV_COLOR;
            color.a = 1.f;
			return false; // delete tag
		}
		
		virtual void print() {
    		_header.print();
			SWF_TRACE("color:%d,%d,%d\n", r,g,b);
		}
		
		static ITag* create( TagHeader& header ) {
			return new SetBackgroundColorTag( header );
		}				
	};

}//namespace
#endif	// __BACKGROUND_H__