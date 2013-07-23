/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "DefineButton.h"
#include "PlaceObject.h"
#include "tsSWF.h"

using namespace tinyswf;

//-----------------------------------------------------------------------------
	
// Return true if we read a record; false if this is a null record.
bool ButtonRecord::read( Reader& reader, SWF& swf, int tag_type ) {
	uint8_t	flags = reader.get<uint8_t>();
	if (0 == flags)
		return false;
	/*
	_has_blend_mode	= flags & 0x20 ? true : false;
	_has_filter_list= flags & 0x10 ? true : false;
	_hit_test		= flags & 0x08 ? true : false;
	_down			= flags & 0x04 ? true : false;
	_over			= flags & 0x02 ? true : false;
	_up				= flags & 0x01 ? true : false;
	*/
	_state = flags;
	_character_id = reader.get<uint16_t>();
	_depth = reader.get<uint16_t>();

	reader.getMatrix(_matrix);
	reader.align();

	if (TAG_DEFINE_BUTTON2 == tag_type) {
		reader.getCXForm( _cxform );
		reader.align();
		if ( flags & 0x10 ) {
			reader.getFilterList();
		}
		if ( flags & 0x20 ) {
			_blend_mode = reader.get<uint8_t>();
		}
	}

	return true;
}

void ButtonRecord::print() {
#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
	(byte & 0x80 ? 1 : 0), \
	(byte & 0x40 ? 1 : 0), \
	(byte & 0x20 ? 1 : 0), \
	(byte & 0x10 ? 1 : 0), \
	(byte & 0x08 ? 1 : 0), \
	(byte & 0x04 ? 1 : 0), \
	(byte & 0x02 ? 1 : 0), \
	(byte & 0x01 ? 1 : 0) 
	SWF_TRACE("state:0x" BYTETOBINARYPATTERN " character=%d, depth=%d", BYTETOBINARY(_state), _character_id, _depth);
	SWF_TRACE("\tmtx:%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f\n",
		_matrix.m_[0][0], _matrix.m_[0][1], _matrix.m_[0][2],
		_matrix.m_[1][0], _matrix.m_[1][1], _matrix.m_[1][2]);
}

//-----------------------------------------------------------------------------

bool ButtonAction::read( Reader& reader, SWF& swf, MovieFrames& frames, int tag_type )
{
	// Read condition flags.
	if (TAG_DEFINE_BUTTON == tag_type) {
		_conditions = OVER_DOWN_TO_OVER_UP;
	} else {
		assert(tag_type == TAG_DEFINE_BUTTON2);
		_conditions = reader.get<uint16_t>();
	}

	// Read actions.
	return _actions.read( reader, swf, frames);
}

//-----------------------------------------------------------------------------

DefineButton2Tag::~DefineButton2Tag()
{
	for (size_t i = 0; i < _buttonActions.size(); ++i) {
		delete _buttonActions[i];
	}
}

bool DefineButton2Tag::read( Reader& reader, SWF& swf, MovieFrames& frames) 
{
	_buttonId = reader.get<uint16_t>();
	_asMenu = reader.get<uint8_t>() != 0;

	int pos = reader.getCurrentPos();
	uint16_t _actionOffset = reader.get<uint16_t>();

	RECT empty = {FLT_MAX,FLT_MAX,-FLT_MAX,-FLT_MAX};
	_bound = empty;

	// Read button records.
	while (1) {
		ButtonRecord rec;
		if (rec.read(reader, swf, TAG_DEFINE_BUTTON2) == false) {
			// Null record; marks the end of button records.
			break;
		}
		RECT rect = swf.calculateRectangle(rec._character_id, &rec._matrix);
		MergeRectangle(_bound, rect);

		_buttonRecords.push_back(rec);
	}

    swf.addCharacter( this, _buttonId );
	if (0 == _actionOffset)
		return true;

	int skip = _actionOffset + pos - reader.getCurrentPos();
	reader.skip(skip);
	
	while (1) {
		int curr = reader.getCurrentPos();
		int	actionSize = reader.get<uint16_t>();

		ButtonAction *action = new ButtonAction;
		action->read( reader, swf, frames, TAG_DEFINE_BUTTON2);
		_buttonActions.push_back(action);

		if (actionSize == 0)
			break;
	}
	return true; // keep tag
}

void DefineButton2Tag::print() {
    _header.print();
	SWF_TRACE("id=%d\n", _buttonId);
	for (size_t i = 0; i < _buttonRecords.size(); ++i) {
		_buttonRecords[i].print();
	}
	for (size_t i = 0; i < _buttonActions.size(); ++i) {
		_buttonActions[i]->print();
	}
}

//-----------------------------------------------------------------------------

Button::Button( MovieClip& parent,  DefineButton2Tag& data )
	:MovieClip( parent.getSWF(), _frames )
	,_parent(parent)
	,_definition(data)
	,_mouseState(MOUSE_UP)
{
	SWF_TRACE("create Button\n");
	// allocate PlaceObjects for button states
	ButtonRecordArray::const_iterator it = getDefinition()._buttonRecords.begin();
	while( getDefinition()._buttonRecords.end() != it) {
		PlaceObjectTag *object = new PlaceObjectTag( *it );
		ButtonState state = { it->_state, object };
		_buttonStates.push_back( state );

		// create instances for hit test
		if (it->_state & kButtonStateHitTest) {
			const uint16_t depth = it->_depth;
			MovieObject& mo = _buttonHitTests[ depth ];
			mo._character = this->getInstance(object);
			object->copyAttributes(mo);
		}
		++it;
	}
	clearDisplayList();
	setupFrame();
}

Button::~Button()
{
	SWF_TRACE("delete Button[%x]\n", this);
	StateArray::const_iterator it = _buttonStates.begin();
	while( _buttonStates.end() != it) {
		delete it->_object;
		++it;
	}
}

void Button::update(void) {
    // update the display list
	DisplayList::iterator iter = _display_list.begin();
	while ( _display_list.end() != iter ) {
		iter->second.update();
		++iter;
	}
}

ICharacter* Button::getTopMost(float x, float y, bool polygonTest) {
	DisplayList::reverse_iterator rit = _buttonHitTests.rbegin();
	while( rit != _buttonHitTests.rend() ) {
		MovieObject &object = rit->second;
		ICharacter* pCharacter = object._character;
		if (pCharacter) {
			MATRIX m;
			POINT local, world = {x,y};
			m.setInverse( object._transform );
			m.transform( local, world );
			if (pCharacter->getTopMost(local.x, local.y, true))
				return this;
		}
        ++rit;
	}
	return NULL;
}

void Button::setupFrame(void) {
	clearDisplayList();
	int flag = 1 << _mouseState;
	StateArray::const_iterator it = _buttonStates.begin();
	while( _buttonStates.end() != it) {
		if (it->_state & flag) {
			it->_object->setup(*this, false);
		}
		++it;
	}
}

void Button::onEvent(Event::Code code) {
	int flag = 0;
	switch(code) {
	case Event::ROLL_OUT:
		_mouseState = MOUSE_UP;
		flag |= (ButtonAction::OVER_UP_TO_IDLE);
		break;
	case Event::ROLL_OVER:
		_mouseState = MOUSE_OVER;
		flag |= (ButtonAction::IDLE_TO_OVER_UP);
		break;
	case Event::PRESS:
		_mouseState = MOUSE_DOWN;
		flag |= (ButtonAction::OVER_UP_TO_OVER_DOWN);
		break;
	case Event::RELEASE:
		_mouseState = MOUSE_UP;
		flag |= (ButtonAction::OVER_DOWN_TO_OVER_UP);
		break;
	case Event::RELEASE_OUTSIDE:
		_mouseState = MOUSE_UP;
		flag |= (ButtonAction::IDLE_TO_OVER_UP);
		break;
	default:
		//else if (id.m_id == event_id::DRAG_OUT) c |= (button_action::OVER_DOWN_TO_OUT_DOWN);
		//else if (id.m_id == event_id::DRAG_OVER) c |= (button_action::OUT_DOWN_TO_OVER_DOWN);
		//else if (id.m_id == event_id::RELEASE_OUTSIDE) c |= (button_action::OUT_DOWN_TO_IDLE);
		break;
	}
	// setup display list
	setupFrame();

#ifdef SWF_DEBUG
	const char*strEvent[] = {"N/A","PRESS","RELEASE","RELEASE_OUTSIDE","ROLL_OVER","ROLL_OUT","DRAG_OVER","DRAG_OUT","KEY_PRESS"};
	SWF_TRACE("event:%s on %x\n",strEvent[code],this);
#endif

	// actions can delete THIS & PARENT through execute_frame_tags()
	ButtonActionArray::iterator it = getDefinition()._buttonActions.begin();
	while( getDefinition()._buttonActions.end() != it ) {
		if ((*it)->_conditions & flag) {
			(*it)->_actions.setup(_parent, false);
		}
		++it;
	}
}

	