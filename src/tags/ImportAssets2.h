/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __IMPORT_ASSETS2_H__
#define __IMPORT_ASSETS2_H__

#include "tsTag.h"

namespace tinyswf
{
	class ImportAssets2Tag : public ITag
    {
        uint16_t count;

	public:
		ImportAssets2Tag( TagHeader& h ) 
		: ITag( h )
		{}
		
		virtual ~ImportAssets2Tag()
        {}

		virtual bool read( Reader& reader, SWF& swf, MovieFrames&) {
            const char *url = reader.getString();
			reader.get<uint8_t>();//reserved
			reader.get<uint8_t>();//reserved
            count = reader.get<uint16_t>();
            SWF_TRACE("from %s\n", url);
            for (uint16_t i = 0; i < count; ++i)
            {
			    uint16_t tag = reader.get<uint16_t>();
                const char *name = reader.getString();
                swf.addAsset(tag, name, true );
                SWF_TRACE("import[%d] %s\n", tag, name);
            }
			return false; // delete tag
		}

		virtual void print() {
    		_header.print();
			//SWF_TRACE("EXPORT_ASSETS:%d\n", count);
		}

		static ITag* create( TagHeader& header ) {
			return new ImportAssets2Tag( header );
		}				
    };

}//namespace
#endif//__IMPORT_ASSETS2_H__