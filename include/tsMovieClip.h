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
	int				_clip_depth;
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

protected:
	void setupFrame(const TagList& tags, bool skipAction);

    void clearDisplayList(void);

    void createTransform(void) {
        _transform = new MATRIX;
        *_transform = kMatrixIdentity;
    }

    Button* createButton(DefineButton2Tag &tag);
    MovieClip* createMovieClip(const DefineSpriteTag &tag);

public:
	typedef std::map<uint16_t, MovieObject>	DisplayList;

	MovieClip( SWF* swf,  const MovieFrames& data );
    virtual ~MovieClip();

	ICharacter* getInstance(const PlaceObjectTag*);

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

    DisplayList& getDisplayList(void) { return _display_list; }

    SWF* getSWF(void) { return _owner; }

    static bool createFrames( Reader& reader, SWF& swf, MovieFrames& );
    static void destroyFrames( MovieFrames& );
	static bool sbCalculateRectangle;

protected:
	typedef std::map<const ITag*, ICharacter*>	CharacterCache;
	typedef std::vector< MovieClip* >			MovieClipArray;
	typedef std::vector< Button* >				ButtonArray;

	CharacterCache		_characters;
	MovieClipArray		_movies;
	ButtonArray			_buttons;

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
