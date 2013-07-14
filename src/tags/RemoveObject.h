/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __REMOVE_OBJECT_H__
#define __REMOVE_OBJECT_H__

#include "tsTag.h"
#include "tsMovieClip.h"
//#include "PlaceObject.h"

namespace tinyswf {

class RemoveObjectTag : public ITag {
	uint16_t	_character_id;
	uint16_t	_depth;

public:
	RemoveObjectTag( TagHeader& h ) 
	:ITag( h )
    ,_character_id( 0 )
	{}
		
	virtual ~RemoveObjectTag()
    {}
		
	uint16_t characterId() const {
		return _character_id;
	}
	uint16_t depth() const {
		return _depth;
	}

    virtual void setup(MovieClip &movie, bool skipAction )
    {
        MovieClip::DisplayList& _display_list = movie.getDisplayList();
		const uint16_t depth = this->depth();
		MovieClip::DisplayList::iterator it = _display_list.find(depth);
		if (it == _display_list.end())
			return;

		//PlaceObjectTag* current_obj = it->second;;
        //if (current_obj) current_obj->gotoFrame(ICharacter::kFRAME_MAXIMUM, skipAction);
		_display_list.erase(it);
    }

	virtual bool read( Reader& reader, SWF&, MovieFrames& ) {
		if ( code() == TAG_REMOVE_OBJECT )
        {
			_character_id = reader.get<uint16_t>();
        }	
		_depth = reader.get<uint16_t>();
		
		return true; // keep tag
	}
		
	virtual void print() {
		if( code() == TAG_REMOVE_OBJECT )
			SWF_TRACE("REMOVEOBJECT id=%d, depth=%d\n", _character_id, _depth);
		else
			SWF_TRACE("REMOVEOBJECT2 depth=%d\n", _depth);
	}
		
	static ITag* create( TagHeader& header ) {
		return new RemoveObjectTag( header );
	}				
};
	
}//namespace
#endif	// __REMOVE_OBJECT_H__