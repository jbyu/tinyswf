/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __PLACE_OBJECT_H__
#define __PLACE_OBJECT_H__

#include "tsTag.h"
#include "DoAction.h"

namespace tinyswf {

struct ButtonRecord;
class MovieClip;
class MovieObject;

//-----------------------------------------------------------------------------
	
struct ClipAction {
public:
	enum EVENT {
		EVENT_KEY_UP			= 1 << 7,
		EVENT_KEY_DOWN			= 1 << 6,
		EVENT_MOUSE_UP			= 1 << 5,
		EVENT_MOUSE_DOWN		= 1 << 4,
		EVENT_MOUSE_MOVE		= 1 << 3,
		EVENT_UNLOAD			= 1 << 2,
		EVENT_ENTER_FRAME		= 1 << 1,
		EVENT_LOAD				= 1 << 0,
		EVENT_DRAG_OVER			= 1 << 15,
		EVENT_ROLL_OUT			= 1 << 14,
		EVENT_ROLL_OVER			= 1 << 13,
		EVENT_RELEASE_OUTSIDE	= 1 << 12,
		EVENT_RELEASE			= 1 << 11,
		EVENT_PRESS				= 1 << 10,
		EVENT_INITIALIZE		= 1 << 9,
		EVENT_DATA				= 1 << 8,
		//RESERVED_5BITS,
		EVENT_CONSTRUCT			= 1 << 18,
		EVENT_KEY_PRESS			= 1 << 17,
		EVENT_DRAG_OUT			= 1 << 16,
		//RESERVED_8BITS,
	};
	uint32_t	_conditions;
	DoActionTag	_actions;

	bool read( Reader& reader, SWF& swf, MovieFrames& frames, uint32_t flag );
	void print() { SWF_TRACE("EventFlags[0x%x]\n",_conditions); _actions.print(); }
};

//-----------------------------------------------------------------------------

class PlaceObjectTag : public ITag
{
	enum Mode {
		INVALID,
		PLACE,
		MOVE,
		REPLACE
	};

	bool readPlaceObject3( Reader& reader, SWF&, MovieFrames& );

public:
	PlaceObjectTag( TagHeader& h );
	PlaceObjectTag( const ButtonRecord& h, const char*name = "btn" );

	virtual ~PlaceObjectTag();

	virtual bool read( Reader& reader, SWF&, MovieFrames& );

	virtual void print();

	virtual void setup(MovieClip&, bool skipAction);
	
	uint16_t depth()        const { return _depth; }
    uint16_t clipDepth()    const { return _clip_depth; }
	uint16_t characterID()  const { return _character_id; }

	MATRIX& transform() { return _transform; }

	const std::string& name() const { return _name; }

	void copyAttributes(MovieObject&);

	static ITag* create( TagHeader& header ) {
		return new PlaceObjectTag( header );
	}

	void trigger(MovieClip&target, int e) const;

	const Filter* getFilter() const { return _dropShadow; }

private:
	uint16_t	_character_id;
	uint16_t	_depth;
	uint16_t	_clip_depth;

	uint8_t		_has_matrix;
	uint8_t		_has_clip_depth;
	uint32_t	_has_color_transform;
	uint32_t	_all_event_flags;

	Mode			_placeMode;

    MATRIX          _transform;
    CXFORM          _cxform;
	std::string     _name;

	Filter *_dropShadow;

	typedef std::vector<ClipAction*> ClipActionArray;
	ClipActionArray _actions;
};

}//namespace
#endif // __PLACE_OBJECT_H__