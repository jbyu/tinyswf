/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/
#ifndef __DEFINE_FONT_H__
#define __DEFINE_FONT_H__

#include "tsTag.h"

namespace tinyswf {

class DefineFontTag : public ITag {
	uint16_t	_character_id;
	std::string _font_name;
	std::vector<uint16_t> _code_table;

public:
	DefineFontTag( TagHeader& h ) 
		:ITag( h )
	{}
		
	virtual ~DefineFontTag()
    {}
		
	virtual bool read( Reader& reader, SWF&, MovieFrames& data );
		
	virtual void print();

	const std::string& getFontName(void) { return _font_name; }

	static ITag* create( TagHeader& header ) {
		return new DefineFontTag( header );
	}				
};

//-----------------------------------------------------------------------------

class DefineFontNameTag : public ITag {
	uint16_t	_font_id;
	std::string _name;
	std::string _copyright;

public:
	DefineFontNameTag( TagHeader& h ) 
		:ITag( h )
	{}
		
	virtual ~DefineFontNameTag()
    {}
		
	virtual bool read( Reader& reader, SWF&, MovieFrames& data );
		
	virtual void print();
		
	static ITag* create( TagHeader& header ) {
		return new DefineFontNameTag( header );
	}				
};


}//namespace
#endif//__DEFINE_FONT_H__