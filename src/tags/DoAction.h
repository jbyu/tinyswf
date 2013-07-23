/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __DO_ACTION_H__
#define __DO_ACTION_H__

#include "tsTag.h"

namespace tinyswf {

//-----------------------------------------------------------------------------
class DoActionTag : public ITag
{
    struct ACTION {
        uint8_t code;
        uint8_t padding;
        uint16_t data;
		char	*buffer;
    };
    typedef std::vector<ACTION> ActionArray;
    ActionArray moActions;

public:
	static TagHeader scButtonHeader;

	DoActionTag( TagHeader& h ) 
		:ITag( h )
	{}

	DoActionTag( ) 
		:ITag( scButtonHeader )
	{}

	virtual ~DoActionTag();

	//restricted copy constructor
	DoActionTag( DoActionTag& );

	virtual bool read( Reader& reader, SWF& swf, MovieFrames& data);

	virtual void print()
    {
    	_header.print();
        ActionArray::iterator it = moActions.begin();
        while(moActions.end()!=it)
        {
            SWF_TRACE("0x%x,", (*it).code);
            ++it;
        }
        SWF_TRACE("\n");
    }

    virtual void setup(MovieClip&, bool skipAction);

	static ITag* create( TagHeader& header ) {
		return new DoActionTag( header );
	}				
};

}//namespace
#endif	// __DO_ACTION_H__