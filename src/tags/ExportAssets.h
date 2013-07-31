/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __EXPORT_ASSETS_H__
#define __EXPORT_ASSETS_H__

#include "tsTag.h"

namespace tinyswf
{
	class ExportAssetsTag : public ITag
    {
        uint16_t count;

	public:
		ExportAssetsTag( TagHeader& h ) 
		: ITag( h )
		{}
		
		virtual ~ExportAssetsTag()
        {}

		virtual bool read( Reader& reader, SWF& swf, MovieFrames&) {
			count = reader.get<uint16_t>();
            for (uint16_t i = 0; i < count; ++i)
            {
			    uint16_t tag = reader.get<uint16_t>();
                const char *name = reader.getString();
                swf.addAsset(tag, name, NULL );
                SWF_TRACE("id=%d, symbol=%s\n", tag, name);
            }
			return false; // delete tag
		}

		virtual void print() {
    		_header.print();
			//SWF_TRACE("EXPORT_ASSETS:%d\n", count);
		}

		static ITag* create( TagHeader& header ) {
			return new ExportAssetsTag( header );
		}				
    };

}//namespace
#endif	// __EXPORT_ASSETS_H__