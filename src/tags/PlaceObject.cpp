/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "PlaceObject.h"
#include "DefineButton.h"
#include "tsSWF.h"

using namespace std;
using namespace tinyswf;

#define vgLoadMatrix (void)
#define vgTranslate (void)
#define vgScale (void)
#define vgMultMatrix (void)
#define vgGetMatrix (void)

PlaceObjectTag::PlaceObjectTag( const ButtonRecord& rec )
	:ITag( DoActionTag::scButtonHeader )
	,_character_id( rec._character_id )
	,_depth( rec._depth )
    ,_clip_depth( 0 )
	,_has_matrix( 1 )
	,_has_clip_depth( 0 )
	,_has_color_transform( 1 )
	,_placeMode( PLACE )
	,_transform( rec._matrix )
	,_cxform( rec._cxform )
	,_name( "btn" )
{
}

bool PlaceObjectTag::read( Reader& reader, SWF& swf, MovieFrames& data )
{
	if ( reader.getBitPos() )
		SWF_TRACE("UNALIGNED!!\n");

	uint8_t has_clip_actions = reader.getbits( 1 );
	_has_clip_depth = reader.getbits( 1 );
	uint8_t has_name = reader.getbits( 1 );
	uint8_t has_ratio = reader.getbits( 1 );
	_has_color_transform = reader.getbits( 1 );
	_has_matrix = reader.getbits( 1 );
	uint8_t _has_character = reader.getbits( 1 );
	uint8_t _has_move = reader.getbits( 1 );
		
	_depth = reader.get<uint16_t>();

	if ( _has_character )
	{
		_character_id = reader.get<uint16_t>();
	}
			
	if ( _has_matrix ) {
		reader.getMatrix( _transform );
        reader.align();
	} else {
        _transform = kMatrixIdentity;
    }
		
	if ( _has_color_transform ) {
		reader.getCXForm( _cxform );
        reader.align();
    }
		
	if ( has_ratio ) {
		// dummy read
		// TODO
		reader.get<uint16_t>();
	}
	if ( has_name ) {
        _name.assign( reader.getString() );
	}
	if ( _has_clip_depth ) {
		_clip_depth = reader.get<uint16_t>();
	}
	if ( has_clip_actions ) {
		// TOOD: support clip actions 
		assert( 0 );
	}

	if ( _has_move == 0 && _has_character == 1 ) {
		_placeMode = PLACE;
	} else if ( _has_move == 1 && _has_character == 0 ) {
		_placeMode = MOVE;
	} else if ( _has_move == 1 && _has_character == 1 ) {
		_placeMode = REPLACE;
	}

	if (! MovieClip::sbCalculateRectangle) 
		return true;

	if ( _has_character ) {
		RECT rect = swf.calculateRectangle(_character_id, 0<_has_matrix? &_transform:NULL );
		MergeRectangle(data._rectangle, rect);
	}

	return true; // keep tag
}

void PlaceObjectTag::print() {
	_header.print();
#ifdef SWF_DEBUG
	const char *mode[] = {"N/A","PLACE","MOVE","REPLACE"};
	SWF_TRACE("character=%d, depth=%d, clip=%d, mode=%s, name=%s\n", _character_id, _depth, _clip_depth, mode[_placeMode], _name.c_str() );
	//SWF_TRACE("mtx:%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f\n", _transform.m_[0][0], _transform.m_[0][1], _transform.m_[0][2],	_transform.m_[1][0], _transform.m_[1][1], _transform.m_[1][2]);
    //SWF_TRACE("\tCXFORM-Mult:%1.2f,%1.2f,%1.2f,%1.2f\n", _cxform.mult.r, _cxform.mult.g, _cxform.mult.b, _cxform.mult.a );
    //SWF_TRACE("\tCXFORM-Add: %1.2f,%1.2f,%1.2f,%1.2f\n", _cxform.add.r, _cxform.add.g, _cxform.add.b, _cxform.add.a );
#endif
}

void PlaceObjectTag::copyAttributes(MovieObject& object) {
	if ( this->_has_matrix ) {
		object._transform = _transform;
	//} else { current_obj->gotoFrame(ICharacter::kFRAME_MAXIMUM, skipAction);
	}
	if ( this->_has_color_transform ) {
		object._cxform = _cxform;
	}
	if ( this->_has_clip_depth ) {
		object._clip_depth = _clip_depth;
	}
}

void PlaceObjectTag::setup(MovieClip& movie, bool skipAction) 
{
    MovieClip::DisplayList& _display_list = movie.getDisplayList();
	const uint16_t depth = this->depth();
	MovieObject& object = _display_list[ depth ];

	switch (_placeMode) {
	default:
		SWF_ASSERT(false);
		break;

	case PLACE:
		// A new character (with ID of CharacterId) is placed on the display list at the specified
		// depth. Other fields set the attributes of this new character.
		// copy over the previous matrix if the new character doesn't have one
		this->copyAttributes(object);
		object._character = movie.getInstance(this);
		break;

	case MOVE:
		// The character at the specified depth is modified. Other fields modify the attributes of this
		// character. Because any given depth can have only one character, no CharacterId is required.
		this->copyAttributes(object);
		break;

	case REPLACE:
		// The character at the specified Depth is removed, and a new character (with ID of CharacterId) 
		// is placed at that depth. Other fields set the attributes of this new character.
		this->copyAttributes(object);
		object._character = movie.getInstance(this);
		break;
	}
}

