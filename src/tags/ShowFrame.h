/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __SHOW_FRAME_H__
#define __SHOW_FRAME_H__

#include "tsTag.h"

namespace tinyswf {
	
	class ShowFrameTag : public ITag {
	public:
		ShowFrameTag( TagHeader& h ) 
		: ITag( h )
		{}
		
		virtual ~ShowFrameTag()
        {}
		
        virtual bool read( Reader& reader, SWF&, MovieFrames& ) { return false; } //delete tag

		virtual void print() { SWF_TRACE("SHOW_FRAME\n"); }

		static ITag* create( TagHeader& header ) {
			return new ShowFrameTag( header );
		}				
	};

}//namespace
#endif	// __SHOW_FRAME_H__