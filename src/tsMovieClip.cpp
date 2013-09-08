/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "tsMovieClip.h"
#include "tsSWF.h"
#include "tags/PlaceObject.h"
#include "tags/DefineSprite.h"
#include "tags/DefineShape.h"
#include "tags/DefineButton.h"
#include "tags/DefineText.h"
#include "tags/ImportAssets2.h"

using namespace tinyswf;

bool MovieClip::sbCalculateRectangle = false;

void MovieClip::destroyFrames(MovieFrames &data)
{
	// foreach frames in sprite
	FrameList::iterator it = data._frames.begin();
	while (data._frames.end() != it)
    {
		// foreach tags in frame
		TagList *_tag_list = *it;
        TagList::iterator tag_it = _tag_list->begin();
        while(_tag_list->end() != tag_it)
        {
            delete *tag_it;
            ++tag_it;
        }
		// delete the tags container
        delete *it;
        ++it;
    }
}

bool MovieClip::createFrames( Reader& reader, SWF& swf, MovieFrames &data )
{
	RECT empty = {FLT_MAX,FLT_MAX,-FLT_MAX,-FLT_MAX};
	data._rectangle = empty;
	sbCalculateRectangle = true;

	// get all tags and build frame lists
    TagHeader header;
	ITag* tag = NULL;
	TagList* frame_tags = new TagList;
	TagList actionTags;
	do {
		header.read( reader );
        const uint32_t code = header.code();
		SWF::TagFactoryFunc factory = SWF::getTagFactory( code );
		if (! factory ) {
			// no registered factory so skip this tag
			SWF_TRACE("*** SKIP *** ");
			header.print();
			reader.skip( header.length() );
			continue;
		}
		tag = factory( header );
		SWF_ASSERT(tag);

		int32_t end_pos = reader.getCurrentPos() + tag->length();
		bool keepTag = tag->read( reader, swf, data );
		tag->print();
		reader.align();

		int32_t dif = end_pos - reader.getCurrentPos();
		if( dif != 0 ) {
			SWF_TRACE("WARNING: tag not read correctly. trying to skip.\n");
			reader.skip( dif );
		}

        if (keepTag) {
			// collect action tags
			if (tag->code() == TAG_DO_ACTION) {
				actionTags.push_back(tag);
			} else {
				frame_tags->push_back( tag );
			}
        } else {
            delete tag;
        }

        // create a new frame
        if ( TAG_SHOW_FRAME == code ) {
			SWF_TRACE("[%d] has %d tags\n", data._frames.size(), frame_tags->size());

			// put action tags at the end of list in order to avoid callback error
			TagList::iterator it = actionTags.begin();
			while (it != actionTags.end()) {
				frame_tags->push_back( *it );
				++it;
			}
			actionTags.clear();

			data._frames.push_back( frame_tags );
			frame_tags = new TagList;
			sbCalculateRectangle = false;
        }
	} while( header.code() != TAG_END );
	delete frame_tags;
    return true;
}

//-----------------------------------------------------------------------------

MovieClip::MovieClip( SWF* swf, MovieClip* parent,  const MovieFrames& data, const PlaceObjectTag *def )
	:_data(data)
	,_owner(swf)
	,_parent(parent)
	,_definition(def)
	,_play(true)
	,_frame(0xffffffff)
{
	//SWF_TRACE("create MovieClip\n");
	//if (0 < data._frames.size()) gotoFrame(0, false);
}

MovieClip::~MovieClip()
{
	//SWF_TRACE("delete MovieClip[%x]\n",this);

	// clean up
	CharacterArray::iterator it = _characters.begin();
	while ( _characters.end() != it ) {
		delete (*it);
		++it;
	}
	_cache.clear();
}

class nullCharacter : public ICharacter {
	RECT _bound;
public:
	virtual const RECT& getRectangle(void) const { return _bound; }
	virtual void draw(void) {}
	virtual void update(void) {}
	virtual ICharacter* getTopMost(float localX, float localY, bool polygonTest) {
		SWF_UNUSED_PARAM(localX);
		SWF_UNUSED_PARAM(localY);
		SWF_UNUSED_PARAM(polygonTest);
		return NULL;
	}
	virtual void onEvent(Event::Code) {}
	virtual TYPE type(void) const { return TYPE_NONE; }
};

static nullCharacter soDefaultNullCharacter;

ICharacter *MovieClip::createCharacter(const ITag* tag, const PlaceObjectTag* place) {
	ICharacter *character = NULL;
	switch ( tag->code() ) {
	case TAG_DEFINE_SPRITE:
		character = new MovieClip(_owner, this, ((DefineSpriteTag*)tag)->getMovieFrames(), place);
		character->update();
		_characters.push_back(character);
		break;
	case TAG_DEFINE_BUTTON2:
		character = new Button(*this, *(DefineButton2Tag*)tag);
		_characters.push_back(character);
		break;
	case TAG_DEFINE_EDIT_TEXT:
		character = new Text(*(DefineEditTextTag*)tag, place);
		_characters.push_back(character);
		break;
	case TAG_IMPORT_ASSETS2:
		character = ((ImportAssets2Tag*)tag)->getCharacter();
		break;
	default:
		character = (DefineShapeTag*)tag;
		break;
	}
	return character;
}

ICharacter *MovieClip::getInstance(const PlaceObjectTag* placeTag) {
	// find cache from this instance
	CharacterCache::iterator it = _cache.find(placeTag);
	if (it != _cache.end())
		return it->second;

	// find character tag from swf library
	ICharacter *character = &soDefaultNullCharacter;
	uint16_t character_id = placeTag->characterID();
	ITag *tag = _owner->getCharacter( character_id );
	if (! tag)
		return character;

	character = createCharacter(tag, placeTag);
	SWF_ASSERT(character);
	_cache[placeTag] = character;

	return character;
}

ICharacter *MovieClip::getCharacter(const char* name) {
	FrameList::const_iterator fit = _data._frames.begin();
	while (fit != _data._frames.end()) {
		const TagList *tags = (*fit);
		TagList::const_iterator it = tags->begin();
		while (it != tags->end()) {
			const ITag* tag = (*it);
			if (tag->code() == TAG_PLACE_OBJECT2 || tag->code() == TAG_PLACE_OBJECT3) {
				PlaceObjectTag* placeTag = (PlaceObjectTag*)tag;
				if (placeTag->name() == name) {
					ICharacter *ch = getInstance(placeTag);
					ch->setTransform( placeTag->transform() );
					return ch;
				}
			}
			++it;
		}
		++fit;
	}
	return NULL;
}

void MovieClip::clearDisplayList(void)
{
#if 0
	DisplayList::iterator iter = _display_list.begin();
	while ( _display_list.end() != iter )
	{
    	//PlaceObjectTag *place_obj = iter->second;
       	//if (place_obj) place_obj->play(false);
		iter->second = NULL;
		++iter;
	}
#endif
	_display_list.clear();
}

void MovieClip::gotoFrame( uint32_t frame, bool skipAction )
{
	if (getFrameCount() <= frame)
	{
		frame = 0;
        clearDisplayList();
	}
    // build up the display list
	_frame = frame;
	TagList* frame_tags = _data._frames[ frame ];
	setupFrame( *frame_tags, skipAction );
}

void MovieClip::gotoLabel( const char* label)
{
    LabelList::const_iterator it = _data._labels.find(label);
    if (_data._labels.end() == it)
        return;
	gotoAndPlay( it->second );
}

void MovieClip::gotoAndPlay( uint32_t frame )
{
	if (frame < _frame || frame > _data._frames.size()) {
		gotoFrame(ICharacter::kFRAME_MAXIMUM, true);
	}
	if (frame == _frame)
		return;
    while (frame > _frame + 1) {
        step(1, true); // skip actions
    }
	step(1, false); // execute actions in target frame
}

void MovieClip::update(void)
{
    if (_play)
        gotoFrame(_frame + 1, false);

    // update the display list
	DisplayList::iterator iter = _display_list.begin();
	while ( _display_list.end() != iter ) {
		iter->second.update();
		++iter;
	}
}

void MovieObject::draw() {
	SWF_ASSERT (_character);
	if (! _character->visible())
		return;

    //concatenate matrix
	MATRIX3f& currMTX = SWF::getCurrentMatrix();
    MATRIX3f origMTX = currMTX, mtx;
    MATRIX3fSet(mtx, _transform); // convert matrix format
    MATRIX3fMultiply(currMTX, mtx, currMTX);

	CXFORM& currCXF = SWF::getCurrentCXForm();
    CXFORM origCXF = currCXF;
    CXFORMMultiply(currCXF, _cxform, currCXF);

    Renderer::getInstance()->applyTransform(currMTX);

    // C' = C * Mult + Add
    // in opengl, use blend mode and multi-pass to achieve that
    // 1st pass TexEnv(GL_BLEND) with glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)
    //      Cp * (1-Ct) + Cc *Ct 
    // 2nd pass TexEnv(GL_MODULATE) with glBlendFunc(GL_SRC_ALPHA, GL_ONE)
    //      dest + Cp * Ct
    // let Mult as Cc and Add as Cp, then we get the result
	_character->draw();

	// restore old matrix
    currMTX = origMTX;
    currCXF = origCXF;
}
	
void MovieClip::draw(void)
{
    MovieObject *mask = NULL;
	uint16_t highest_masked_layer = 0;
	// draw the display list
	DisplayList::iterator iter = _display_list.begin();
	while ( _display_list.end() != iter )
	{
		MovieObject &object = iter->second;
		const uint16_t clip = object._clip_depth;
		const uint16_t depth = iter->first;
		if (mask && depth > highest_masked_layer) {
            // restore stencil
            Renderer::getInstance()->maskOffBegin();
            mask->draw(); 
            Renderer::getInstance()->maskOffEnd();
    		mask = NULL;
		}
		if (0 < clip) {
            // draw mask
    		Renderer::getInstance()->maskBegin();
            object.draw(); 
    		Renderer::getInstance()->maskEnd();
	    	highest_masked_layer = clip;
			mask = &object;
		} else {
            object.draw(); 
        }
		++iter;
	}
	if (mask) {
		// If a mask masks the scene all the way up to the highest layer, 
        // it will not be disabled at the end of drawing the display list, so disable it manually.
        Renderer::getInstance()->maskOffBegin();
        mask->draw(); 
        Renderer::getInstance()->maskOffEnd();
    	mask = NULL;
	}
}

void MovieClip::setupFrame(const TagList& tags, bool skipAction) {
    TagList::const_iterator it = tags.begin();
	while( it != tags.end() ) {
        ITag* tag = *it;
		tag->setup(*this, skipAction);
        ++it;
	}
}

ICharacter* MovieClip::getTopMost(float x, float y, bool polygonTest) {
	ICharacter* pRet = NULL;
	DisplayList::reverse_iterator rit = _display_list.rbegin();
	while( rit != _display_list.rend() ) {
		MovieObject &object = rit->second;
		ICharacter* pCharacter = object._character;
		if (pCharacter && pCharacter->visible()) {
			MATRIX m;
			POINT local, world = {x,y};
			m.setInverse( object._transform );
			m.transform(local, world);
			pRet = pCharacter->getTopMost(local.x, local.y, polygonTest);
			if (pRet) {
				switch(pRet->type()) {
				case ICharacter::TYPE_BUTTON:
				case ICharacter::TYPE_MOVIE:
					return pRet;
				default:
					return this;
				}
			}
		}
        ++rit;
	}
	return NULL;
}

void MovieClip::onEvent(Event::Code code) {
	int flag = 0;
	switch(code) {
	case Event::ROLL_OUT:
		flag |= ClipAction::EVENT_ROLL_OUT;
		break;
	case Event::ROLL_OVER:
		flag |= ClipAction::EVENT_ROLL_OVER;
		break;
	case Event::PRESS:
		flag |= ClipAction::EVENT_PRESS;
		break;
	case Event::RELEASE:
		flag |= ClipAction::EVENT_RELEASE;
		break;
	case Event::RELEASE_OUTSIDE:
		flag |= ClipAction::EVENT_RELEASE_OUTSIDE;
		break;
	default:
		//else if (id.m_id == event_id::DRAG_OUT) c |= (button_action::OVER_DOWN_TO_OUT_DOWN);
		//else if (id.m_id == event_id::DRAG_OVER) c |= (button_action::OUT_DOWN_TO_OVER_DOWN);
		//else if (id.m_id == event_id::RELEASE_OUTSIDE) c |= (button_action::OUT_DOWN_TO_IDLE);
		break;
	}

#ifdef SWF_DEBUG
	const char*strEvent[] = {"N/A","PRESS","RELEASE","RELEASE_OUTSIDE","ROLL_OVER","ROLL_OUT","DRAG_OVER","DRAG_OUT","KEY_PRESS"};
	SWF_TRACE("event:%s on %x\n",strEvent[code],this);
#endif

	if (_definition)
		_definition->trigger(*this, flag);

}
