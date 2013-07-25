/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "tsSWF.h"
#include "tags/DefineShape.h"
#include "tags/PlaceObject.h"
#include "tags/ShowFrame.h"
#include "tags/RemoveObject.h"
#include "tags/DefineSprite.h"
#include "tags/SetBackgroundColor.h"
#include "tags/ExportAssets.h"
#include "tags/ImportAssets2.h"
#include "tags/DefineSound.h"
#include "tags/DoAction.h"
#include "tags/FrameLabel.h"
#include "tags/DefineButton.h"
#include "tags/VectorEngine.h"
#include "tags/DefineScalingGrid.h"
#include "tags/DefineText.h"
#include "tags/DefineFont.h"

using namespace tinyswf;
	
Speaker *Speaker::spSpeaker = NULL;
Renderer *Renderer::spRenderer = NULL;
FontHandler *FontHandler::spHandler = NULL;

SWF::LoadAssetCallback SWF::_asset_loader = NULL;
SWF::TagFactoryMap SWF::_tag_factories;

MATRIX3f SWF::_sCurrentMatrix = kMatrix3fIdentity;
CXFORM SWF::_sCurrentCXForm = kCXFormIdentity;

SWF::SWF()
    :MovieClip(this, _swf_data)
	,_mouseX(0)
	,_mouseY(0)
	,_mouseButtonStateCurr(0)
	,_mouseButtonStateLast(0)
	,_mouseInsideEntityLast(false)
	,_pActiveEntity(NULL)
    ,_elapsedAccumulator(0.0f)
    ,_elapsedAccumulatorDuplicate(0.0f)
    ,_getURL(0)
{
  //_owner = this;
}

SWF::~SWF()
{
    MovieClip::destroyFrames(_swf_data);
}

//static function
bool SWF::initialize(LoadAssetCallback func, int memory_pool_size) {
    _asset_loader = func;
	// setup the factories
	addFactory( TAG_END,			EndTag::create );
		
	addFactory( TAG_DEFINE_SHAPE,	DefineShapeTag::create );		// DefineShape
	addFactory( TAG_DEFINE_SHAPE2,	DefineShapeTag::create );		// DefineShape2
	addFactory( TAG_DEFINE_SHAPE3,	DefineShapeTag::create );		// DefineShape3
	addFactory( TAG_DEFINE_SHAPE4,	DefineShapeTag::create );		// DefineShape4

	addFactory( TAG_DEFINE_BUTTON,	DefineButtonTag::create );
	addFactory( TAG_DEFINE_BUTTON2,	DefineButton2Tag::create );

	addFactory( TAG_DEFINE_SPRITE,	DefineSpriteTag::create );
	addFactory( TAG_PLACE_OBJECT2,	PlaceObjectTag::create );
	addFactory( TAG_REMOVE_OBJECT2,	RemoveObjectTag::create );
	addFactory( TAG_SHOW_FRAME,		ShowFrameTag::create );
		
    addFactory( TAG_EXPORT_ASSETS,  ExportAssetsTag::create );
    addFactory( TAG_IMPORT_ASSETS2, ImportAssets2Tag::create );

    addFactory( TAG_DEFINE_SOUND,   DefineSoundTag::create );
    addFactory( TAG_START_SOUND,    StartSoundTag::create );

    addFactory( TAG_DO_ACTION,      DoActionTag::create );
    addFactory( TAG_FRAME_LABEL,    FrameLabelTag::create );

	addFactory( TAG_SET_BACKGROUND_COLOR, SetBackgroundColorTag::create );
	addFactory( TAG_DEFINE_SCALING_GRID, DefineScalingGridTag::create );

	//addFactory( TAG_DEFINE_TEXT, DefineTextTag::create );
	//addFactory( TAG_DEFINE_TEXT2, DefineTextTag::create );
	addFactory( TAG_DEFINE_EDIT_TEXT, DefineEditTextTag::create );

	addFactory( TAG_DEFINE_FONT3, DefineFontTag::create );
	addFactory( TAG_DEFINE_FONT_NAME, DefineFontNameTag::create );

	triangulation::create_memory_pool(memory_pool_size);
	return true;
}

bool SWF::terminate(void) {
	triangulation::destroy_memory_pool();
	return true;
}

bool SWF::read( Reader& reader ) {
	bool ret = _header.read( reader );
    if (false == ret)
        return false;

	createFrames(reader, *this, _swf_data);

    SWF_ASSERT(getFrameCount() == _header.getFrameCount());
	this->gotoFrame( 0, false );
	return true;
}
	
void SWF::print()
{
	_header.print();
}

bool SWF::addAsset(uint16_t id, const char *name, bool import)
{
    if (_asset_loader)
    {
        Asset asset = _asset_loader( name, import );
        if (0 != asset.handle) {
            moAssets[id] = asset;
        } else {
            _library[name] = id;
        }
        return true;
    }
    return false;
}

MovieClip *SWF::duplicate(const char *name) {
    MovieClip *movie = NULL;
    SymbolDictionary::const_iterator it = _library.find(name);
    if (_library.end() != it) {
        ITag *tag = _dictionary[it->second];
		SWF_ASSERT(TAG_DEFINE_SPRITE == tag->code() || TAG_DEFINE_BUTTON2 == tag->code());
        // create a new instance
		movie = (MovieClip*)createCharacter(tag);
		movie->createTransform();
		_duplicates.push_back(movie);
    }
    return movie;
}

void SWF::updateDuplicate( float delta ) {
    _elapsedAccumulatorDuplicate += delta;
    const float secondPerFrame = getFrameRate();
    while (secondPerFrame <= _elapsedAccumulatorDuplicate)
    {
        _elapsedAccumulatorDuplicate -= secondPerFrame;
		CharacterArray::iterator it = _duplicates.begin();
		while (_duplicates.end() != it)
        {
            (*it)->update();
            ++it;
        }
    }
}

void SWF::drawDuplicate(void) {
    Renderer::getInstance()->drawBegin();
	CharacterArray::iterator it = _duplicates.begin();
	while (_duplicates.end() != it) {
		MovieClip *movie = (MovieClip*)(*it);
        MATRIX *xform = movie->getTransform();
        SWF_ASSERT(xform);
		MATRIX3f origMTX = _sCurrentMatrix, mtx;
        MATRIX3fSet(mtx, *xform); // convert matrix format
        MATRIX3fMultiply(_sCurrentMatrix, mtx, _sCurrentMatrix);
        movie->draw();
        ++it;
    	// restore old matrix
        _sCurrentMatrix = origMTX;
    }
    Renderer::getInstance()->drawEnd();
}

void SWF::drawMovieClip(MovieClip *movie, float alpha)
{
	if (movie == NULL)
		return;

    MATRIX *xform = movie->getTransform();
    SWF_ASSERT(xform);
    MATRIX3f origMTX = _sCurrentMatrix, mtx;
    MATRIX3fSet(mtx, *xform); // convert matrix format
    MATRIX3fMultiply(_sCurrentMatrix, mtx, _sCurrentMatrix);
    float origAlpha = _sCurrentCXForm.mult.a;
    _sCurrentCXForm.mult.a = alpha;

	Renderer::getInstance()->drawBegin();
    movie->draw();
    Renderer::getInstance()->drawEnd();

    // restore old matrix
    _sCurrentMatrix = origMTX;
    _sCurrentCXForm.mult.a = origAlpha;
}

void SWF::update( float delta )
{
    _elapsedAccumulator += delta;
    const float secondPerFrame = getFrameRate();
    while (secondPerFrame <= _elapsedAccumulator)
    {
        _elapsedAccumulator -= secondPerFrame;
		MovieClip::update();
    }
}
    
void SWF::draw(void) {
    Renderer::getInstance()->drawBegin();
	MovieClip::draw();
    Renderer::getInstance()->drawEnd();
}

RECT SWF::calculateRectangle(uint16_t character, const MATRIX* xf) {
	RECT rect = {0,0,-0,-0};
	ITag *tag = this->getCharacter( character );
	if(! tag)
		return rect;

    switch( tag->code()) {
	case TAG_DEFINE_BUTTON2:
		{
			DefineButton2Tag *btn = (DefineButton2Tag*)tag;
			rect = btn->getRectangle();
		}
		break;
	case TAG_DEFINE_SPRITE:
		{
			DefineSpriteTag *sprite = (DefineSpriteTag*)tag;
			rect = sprite->getMovieFrames()._rectangle;
		}
		break;
	case TAG_DEFINE_SHAPE:
	case TAG_DEFINE_SHAPE2:
	case TAG_DEFINE_SHAPE3:
	case TAG_DEFINE_SHAPE4:
		{
			DefineShapeTag *shape = (DefineShapeTag*)tag;
			rect = shape->getRectangle();
		}
		break;
	default:
		break;
    }
	if (xf) {
		xf->transform(rect, rect);
	}
	return rect;
}

void SWF::notifyMouse(int button, int x, int y) {
	ICharacter *pTopMost = this->getTopMost(float(x), float(y), false);
	notifyEvent(button,  x,  y, pTopMost);
}

void SWF::notifyDuplicate(int button, int x, int y) {
	ICharacter* pTopMost = NULL;
	CharacterArray::reverse_iterator rit = _duplicates.rbegin();
	while( rit != _duplicates.rend() ) {
		MovieClip *movie = (MovieClip*)(*rit);
		if (movie) {
			MATRIX m;
			POINT local, world = {float(x),float(y)};
			m.setInverse( *movie->getTransform() );
			m.transform(local, world);
			pTopMost = movie->getTopMost(local.x, local.y, false);
			if (pTopMost)
				break;
		}
        ++rit;
	}
	notifyEvent(button,  x,  y, pTopMost);
}

void SWF::notifyEvent(int button, int x, int y, ICharacter *pTopMost) {
	_mouseX = x;
	_mouseY = y;
	_mouseButtonStateCurr = button;

	if (0 < _mouseButtonStateLast) {
		// Mouse button was down.
#if 0
		// Handle trackAsMenu dragOver
		if (NULL == _pActiveEntity) { //|| _pActiveEntity->get_track_as_menu() )
			if ( pTopMost && (pTopMost != _pActiveEntity) )  { // && pTopMost->get_track_as_menu() )
				// Transfer to topmost entity, dragOver
				_pActiveEntity = pTopMost;
				//_pActiveEntity->on_event( event_id::DRAG_OVER );
				_mouseInsideEntityLast = true;
			}
		}

		// Handle onDragOut, onDragOver
		if (_mouseInsideEntityLast) {
			if (pTopMost != _pActiveEntity) {
				if (_pActiveEntity)	{
					//active_entity->on_event( event_id::DRAG_OUT );
				}
				_mouseInsideEntityLast = false;
			}
		} else {
			if (pTopMost == _pActiveEntity) {
				if (_pActiveEntity) {
				//_pActiveEntity->on_event( event_id::DRAG_OVER );
				}
				_mouseInsideEntityLast = true;
			}
		}
#endif
		// Handle onRelease, onReleaseOutside
		if (0 == _mouseButtonStateCurr) {
			// Mouse button is up.
			//m_mouse_listener.notify(event_id::MOUSE_UP);
			if (pTopMost != _pActiveEntity) {
				// onReleaseOutside
				if (_pActiveEntity) { //&& !active_entity->get_track_as_menu() )
					_pActiveEntity->onEvent( Event::RELEASE_OUTSIDE );
				}
			} else {
				// onRelease
				if (_pActiveEntity && _mouseInsideEntityLast) {
					_pActiveEntity->onEvent( Event::RELEASE );
				}
			}
		}
	} else {
		// Mouse button was up.
		if (0 == _mouseButtonStateCurr) {
			// Mouse button is up.
			// New active entity is whatever is below the mouse right now.
			if (pTopMost != _pActiveEntity) {
				// onRollOut
				if (_pActiveEntity && _mouseInsideEntityLast) {
					_pActiveEntity->onEvent( Event::ROLL_OUT );
					_mouseInsideEntityLast = false;
				}
				// onRollOver
				if (pTopMost) {
					pTopMost->onEvent( Event::ROLL_OVER );
					_mouseInsideEntityLast = true;
				}
				_pActiveEntity = pTopMost;
			}
		} else {
			// Mouse button is down.
#if 0
			//m_mouse_listener.notify(event_id::MOUSE_DOWN);
			// set/kill focus
			// It's another entity ?
			if (m_current_active_entity != active_entity) {
				// First to clean focus
				if (m_current_active_entity != NULL) {
					m_current_active_entity->on_event(event_id::KILLFOCUS);
					m_current_active_entity = NULL;
				}
				// Then to set focus
				if (active_entity != NULL) {
					if (active_entity->on_event(event_id::SETFOCUS)){
						m_current_active_entity = active_entity;
					}
				}
			}
#endif
			// onPress
			if (_pActiveEntity && _mouseInsideEntityLast) {
				_pActiveEntity->onEvent(Event::PRESS);
			}
		}
	}
	_mouseButtonStateLast = _mouseButtonStateCurr;
}

#ifdef WIN32
bool SWF::trimSkippedTags( const char* output, Reader& reader ) {
	ITag* tag = NULL;

	FILE* fp = NULL;
	errno_t error = fopen_s(&fp, output, "wb");
	if (0 != error)
		return false;
	if (0 == fp)
		return false;

	const char *data = reader.getData();
	int size, start = reader.getCurrentPos();

	// swf header
	bool ret = _header.read( reader );
    if (false == ret)
        return false;

	// write back
	size = reader.getCurrentPos() - start;
	fwrite(data, size, 1, fp);
	data = reader.getData();
	start = reader.getCurrentPos();

	// get the first tag
    TagHeader header;
	header.read( reader );
	while( header.code() != TAG_END ) { 
        const uint32_t code = header.code();
		SWF::TagFactoryFunc factory = SWF::getTagFactory( code );
		if ( factory ) {
			// write back tag
			size = reader.getCurrentPos() - start + header.length();
			fwrite(data, size, 1, fp);
			SWF_TRACE("*** WRITE *** ");
		} else {
			SWF_TRACE("*** SKIP *** ");
		}
		header.print();
		reader.skip( header.length() );
		data = reader.getData();
		start = reader.getCurrentPos();
		header.read( reader );
	}
	// write back final tag
	size = reader.getCurrentPos() - start;
	fwrite(data, size, 1, fp);

	fclose(fp);
    return true;
}
#endif

