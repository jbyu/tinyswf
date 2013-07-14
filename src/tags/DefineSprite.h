/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __DEFINE_SPRITE_H__
#define __DEFINE_SPRITE_H__

#include "tsTag.h"

namespace tinyswf {

class DefineSpriteTag : public ITag {
public:
	DefineSpriteTag( TagHeader& h )
	:	ITag( h )
	{
	}
	
	virtual ~DefineSpriteTag();

	virtual bool read( Reader& reader, SWF&, MovieFrames& );
	virtual void print();
	
	const MovieFrames& getMovieFrames(void) const { return _data; }

	static ITag* create( TagHeader& header ) {
		return new DefineSpriteTag( header );
	}

private:
	uint16_t	_sprite_id;
	uint16_t	_frame_count;
	MovieFrames	_data;
};

}//namespace
#endif // __DEFINE_SPRITE_H__
