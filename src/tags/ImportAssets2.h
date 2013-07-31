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
		ICharacter *_character;

	public:
		ImportAssets2Tag( TagHeader& h ) 
			:ITag( h )
			,_character(NULL)
		{}
		
		virtual ~ImportAssets2Tag()
        {}

		ICharacter *getCharacter() { return _character; }

		virtual bool read( Reader& reader, SWF& swf, MovieFrames&) {
            const char *url = reader.getString();
			reader.get<uint8_t>();//reserved
			reader.get<uint8_t>();//reserved
            uint16_t count = reader.get<uint16_t>();
            for (uint16_t i = 0; i < count; ++i) {
			    uint16_t id = reader.get<uint16_t>();
                const char *name = reader.getString();
                _character = swf.addAsset(id, name, url);
                SWF_TRACE("id=%d, symbol=%s from %s\n", id, name, url);
				if (_character) {
					swf.addCharacter( this, id );
					return true; // keep tag
				}
            }
			return false; // delete tag
		}

		virtual void print() {
    		_header.print();
		}

		static ITag* create( TagHeader& header ) {
			return new ImportAssets2Tag( header );
		}				
    };

}//namespace
#endif//__IMPORT_ASSETS2_H__