/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/
#ifndef __DEFINE_SCALING_GRID_H__
#define __DEFINE_SCALING_GRID_H__

#include "tsTag.h"
#include "tsReader.h"

namespace tinyswf
{
	class DefineScalingGridTag : public ITag {
		uint16_t _character_id;
		RECT	_center;

	public:
		DefineScalingGridTag( TagHeader& h ) 
			:ITag( h )
		{}
		
		virtual ~DefineScalingGridTag()
        {}
		
		virtual bool read( Reader& reader, SWF&, MovieFrames&  )
        {
			_character_id = reader.get<uint16_t>();
			reader.getRectangle(_center);
			reader.align();
			return false;//delete tag
		}
		
		virtual void print() {
    		_header.print();
			SWF_TRACE("id=%d, ", _character_id);
			SWF_TRACE("RECT:{%.2f,%.2f,%.2f,%.2f}\n",
				_center.xmin, _center.ymin,
				_center.xmax, _center.ymax);
		}
		
		static ITag* create( TagHeader& header ) {
			return new DefineScalingGridTag( header );
		}				
	};
}	
#endif//__DEFINE_SCALING_GRID_H__