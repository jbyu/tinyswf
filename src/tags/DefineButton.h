/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __DEFINE_BUTTON_H__
#define __DEFINE_BUTTON_H__

#include "tsTag.h"
#include "tsReader.h"
#include "tsMovieClip.h"
#include "DoAction.h"

namespace tinyswf {
class MovieClip;
class Button;
class PlaceObjectTag;

//-----------------------------------------------------------------------------

const int kButtonStateHitTest = 0x08;

struct ButtonRecord {
	/*
	ButtonReserved			UB[2] Reserved bits; always 0
	ButtonHasBlendMode		UB[1] 0 = No blend mode, 1 = Has blend mode (SWF 8 and later only)
	ButtonHasFilterList 	UB[1] 0 = No filter list, 1 = Has filter list (SWF 8 and later only)
	ButtonStateHitTest		UB[1] Present in hit test state
	ButtonStateDown			UB[1] Present in down state
	ButtonStateOver			UB[1] Present in over state
	ButtonStateUp			UB[1] Present in up state
	*/
	int		_state;
	uint16_t	_character_id;
	uint16_t	_depth;
	MATRIX	_matrix;
	CXFORM	_cxform;
	int		_blend_mode;

	bool read( Reader& read, SWF& swf, int tag_type );
	void print();
};

//-----------------------------------------------------------------------------
	
struct ButtonAction {
public:
	enum Condition {
		IDLE_TO_OVER_UP			= 1 << 0,
		OVER_UP_TO_IDLE			= 1 << 1,
		OVER_UP_TO_OVER_DOWN	= 1 << 2,
		OVER_DOWN_TO_OVER_UP	= 1 << 3,
		OVER_DOWN_TO_OUT_DOWN	= 1 << 4,
		OUT_DOWN_TO_OVER_DOWN	= 1 << 5,
		OUT_DOWN_TO_IDLE		= 1 << 6,
		IDLE_TO_OVER_DOWN		= 1 << 7,
		OVER_DOWN_TO_IDLE		= 1 << 8,
	};
	int	_conditions;
	DoActionTag	_actions;

	bool read( Reader& reader, SWF& swf, MovieFrames& frames, int tag_type );
	void print() { SWF_TRACE("Condition[0x%x]\n",_conditions); _actions.print(); }
};

//-----------------------------------------------------------------------------

//TAG_DEFINE_BUTTON
class DefineButtonTag : public ITag
{
    uint16_t _buttonId;

public:
	DefineButtonTag( TagHeader& h ) 
	: ITag( h )
	{}
		
	virtual ~DefineButtonTag()
    {}

	virtual bool read( Reader& reader, SWF& , MovieFrames& ) {
		_buttonId = reader.get<uint16_t>();
        reader.skip( length()-2 );
		return false;//delete tag
	}

	virtual void print() {
    	_header.print();
		SWF_TRACE("id=%d\n", _buttonId);
	}

	static ITag* create( TagHeader& header ) {
		return new DefineButtonTag( header );
	}				
};

//-----------------------------------------------------------------------------

typedef std::vector<ButtonRecord> ButtonRecordArray;
typedef std::vector<ButtonAction*> ButtonActionArray;

class DefineButton2Tag : public ITag
{
	friend class Button;
private:
    uint16_t	_buttonId;
	bool		_asMenu;
	RECT		_bound;
	ButtonRecordArray _buttonRecords;
	ButtonActionArray _buttonActions;

public:
	DefineButton2Tag( TagHeader& h ) 
	: ITag( h )
	,_buttonId(0)
	,_asMenu(0)
	{}
		
	virtual ~DefineButton2Tag();

    virtual bool read( Reader& reader, SWF& swf, MovieFrames& frames);
	virtual void print();

	const RECT& getRectangle(void) const { return _bound; }

	static ITag* create( TagHeader& header ) {
		return new DefineButton2Tag( header );
	}				
};

//-----------------------------------------------------------------------------

}// namespace	

#endif	// __DEFINE_BUTTON_H__
