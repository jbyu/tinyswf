/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __TS_MOVIE_CLIP_H__
#define __TS_MOVIE_CLIP_H__

#include "tsReader.h"
#include "tsTag.h"

namespace tinyswf {

class Reader;
class SWF;
class Button;
class DefineSpriteTag;
class DefineButton2Tag;

//-----------------------------------------------------------------------------

class MovieObject {
public:
	ICharacter      *_character;
	uint16_t		_clip_depth;
	MATRIX          _transform;
    CXFORM          _cxform;		
	//std::string   _name;
	//int			_ratio;

	MovieObject() 
		:_character(NULL)
		,_clip_depth(0)
		,_transform(kMatrixIdentity)
		,_cxform(kCXFormIdentity)
	{
	}

	void draw(void);

	void update(void) {
		SWF_ASSERT (_character);
		_character->update();
	}
	/*
    void play(bool enable) {
		SWF_ASSERT (_character);
		_character->play(enable);
	}

	void gotoFrame(uint32_t frame, bool skipAction) {
		SWF_ASSERT (_character);
		_character->gotoFrame( frame,skipAction );
	}
	*/
};

//-----------------------------------------------------------------------------
class MovieClip : public ICharacter {
	friend class SWF;
	friend class PlaceObjectTag;
	friend class RemoveObjectTag;

protected:
	typedef std::map<uint16_t, MovieObject>	DisplayList;

	void setupFrame(const TagList& tags, bool skipAction);

    void clearDisplayList(void);

    void createTransform(void) {
        _transform = new MATRIX;
        *_transform = kMatrixIdentity;
    }

	ICharacter *createCharacter(const ITag*);
	ICharacter* getInstance(const PlaceObjectTag*);
    DisplayList& getDisplayList(void) { return _display_list; }

	static bool sbCalculateRectangle;

	// no copy constructor and assignment operator
	MovieClip& operator=(const MovieClip&);
	MovieClip(const MovieClip&);

public:
	MovieClip( SWF* swf,  const MovieFrames& data );
    virtual ~MovieClip();

	ICharacter* getInstance(const char *name);

	bool setString(const char *name, const char* text);

    MATRIX* getTransform(void) { return _transform; }

    void gotoLabel( const char* label );
    void gotoAndPlay( uint32_t frame );

	// override ICharacter function
    virtual const RECT& getRectangle(void) const { return _data._rectangle; }
	virtual void draw(void);
	virtual void update(void);
	virtual ICharacter* getTopMost(float localX, float localY, bool polygonTest);
	virtual void onEvent(Event::Code) {}

    void play( bool enable ) { 	_play = enable; }
	void gotoFrame( uint32_t frame, bool skipAction );

	uint32_t getCurrentFrame( void ) const { return _frame; }
    uint32_t getFrameCount( void ) const { return _data._frames.size(); }

	void step( int delta, bool skipAction ) { gotoFrame( this->getCurrentFrame() + delta, skipAction );  }

    bool isPlay(void) const { return _play; }

    SWF* getSWF(void) { return _owner; }

    static bool createFrames( Reader& reader, SWF& swf, MovieFrames& );
    static void destroyFrames( MovieFrames& );

protected:
	typedef std::map<const ITag*, ICharacter*>	CharacterCache;
	typedef std::vector<ICharacter*>			CharacterArray;

	CharacterCache		_cache;			// cache for character by tag
	CharacterArray		_characters;	// store character intances

	const MovieFrames   &_data;
    SWF                 *_owner;
    MATRIX              *_transform;
    bool        _play;
	uint32_t	_frame;
	DisplayList	_display_list;
};

//-----------------------------------------------------------------------------

}//namespace

#endif //__TS_MOVIE_CLIP_H__
