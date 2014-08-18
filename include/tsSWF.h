/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __TS_SWF_H__
#define __TS_SWF_H__

#include "tsReader.h"
#include "tsHeader.h"
#include "tsTag.h"
#include "tsMovieClip.h"

namespace tinyswf {

//-----------------------------------------------------------------------------

class Renderer {
    static Renderer *spRenderer;

public:
    virtual ~Renderer() { clear(); }

    virtual void applyTransform( const MATRIX3f& mtx ) = 0;

	virtual void drawTriangles(const VertexArray& vertices, const CXFORM& cxform, const FillStyle& style, const Asset& asset) = 0;
	virtual void drawLineStrip(const VertexArray& vertices, const CXFORM& cxform, const LineStyle& style) = 0;
    virtual void drawImportAsset(const RECT& rect, const CXFORM& cxform, const Asset& asset) = 0;

    virtual void drawBegin(void) = 0;
    virtual void drawEnd(void) = 0;

    virtual void maskBegin(void) = 0;
    virtual void maskEnd(void) = 0;
    virtual void maskOffBegin(void) = 0;
    virtual void maskOffEnd(void) = 0;

    virtual void clear( void ) {}

    static Renderer* getInstance(void) { return spRenderer; }
    static void setInstance(Renderer *r) { spRenderer = r; }
};

//-----------------------------------------------------------------------------

class Speaker {
	static Speaker *spSpeaker;

public:
    virtual ~Speaker() {}

    virtual unsigned int getSound( const char *filename ) = 0;
    virtual void playSound( unsigned int sound, bool stop, bool noMultiple, bool loop ) = 0;

    static Speaker* getInstance(void) { return spSpeaker; }
    static void setInstance(Speaker *r) { spSpeaker = r; }
};

//-----------------------------------------------------------------------------

class FontHandler {
public:
	virtual ~FontHandler() {}

	virtual void drawText(const VertexArray& vertices, uint32_t glyphs, const CXFORM& cxform, const TextStyle& style) = 0;

    virtual uint32_t formatText(VertexArray& vertices, float &lastX, float &lastY,
		const RECT& rect, const TextStyle& style, const std::wstring& text) = 0;

    static FontHandler* getInstance(void) { return spHandler; }
    static void setInstance(FontHandler *r) { spHandler = r; }

protected:
	static FontHandler *spHandler;
};

//-----------------------------------------------------------------------------

class SWF : public MovieClip {
public:
    // factory function prototype
	typedef ITag* (*TagFactoryFunc)( TagHeader& );
    typedef Asset (*LoadAssetCallback)( const char *name, const char *url );
    typedef void (*GetURLCallback)( MovieClip&, bool isFSCommand, const char *command, const char *target, void *param );

	// Curve subdivision error tolerance.
	static float curve_error_tolerance;
	static bool initialize(LoadAssetCallback, int memory_pool_size = 0);
	static bool terminate(void); // clean up

	static TagFactoryFunc getTagFactory( uint32_t tag_code ) {
		TagFactoryMap::iterator factory = _tag_factories.find( tag_code );
		if (_tag_factories.end() != factory)
			return factory->second;
		return NULL;
	}

private:
	static void addFactory( uint32_t tag_code, TagFactoryFunc factory ) { _tag_factories[ tag_code ] = factory;	}

public:
    SWF(); 
    virtual ~SWF();

	bool read( Reader &reader );

	void print();

#ifdef WIN32
	bool trimSkippedTags( const char* output_filename, Reader& reader );
#endif
		
	void  addCharacter( ITag* tag, uint16_t cid ) { _dictionary[cid] = tag; }
	ITag* getCharacter( uint16_t i ) {
		CharacterDictionary::iterator it = _dictionary.find( i );
		if (it == _dictionary.end() ) {
			return NULL;
		}
		return it->second;
	}

	uint16_t getCharacterID(const char *name) {
		SymbolDictionary::iterator it = _library.find( name );
		if (it == _library.end() ) {
			return 0xffff;
		}
		return it->second;
	}

	// duplicate movieclip for other purpose outside flash
	// will allocate memory for transform matrix
	// user need to free the matrix by himself
    MovieClip *duplicate(const char *name, bool allocateMatrix = false);
	// update all duplicated movieclips
    void updateDuplicate(float delta);
	// draw all duplicated movieclips
    void drawDuplicate(void);

	static void drawMovieClip(MovieClip *movie);

	MATRIX3f& getCurrentMatrix(void) { return _currentMatrix; }

	CXFORM& getCurrentCXForm(void) { return _currentCXForm; }

	// animate flash
    void update(float delta);
    void draw(void);

	// button: 0 for up, 1 for down
	bool notifyMouse(int button, float x, float y, bool touchScreen = false);
	void notifyReset(bool duplicate = false);
	bool notifyDuplicate(MovieClip& movie,int button, float x, float y, bool touchScreen = false);

	// flash information
	float getFrameWidth() const     { return _header.getFrameWidth(); }
	float getFrameHeight() const    { return _header.getFrameHeight(); }
	float getFrameRate() const      { return _header.getFrameRate(); }

    COLOR4f& getBackgroundColor(void) { return _bgColor; }

	// handle export/import assets, return NULL or imported character from other swf
	ICharacter* addAsset(uint16_t id, const char *name, const char* url);
    const Asset& getAsset(uint16_t id) const  {
        AssetDictionary::const_iterator it = moAssets.find(id);
        if (moAssets.end() != it)
            return it->second;
        return kNULL_ASSET;
    }
       
	// handle GetURL function (include fscommand)
    void setGetURL(GetURLCallback cb, void *param = NULL) { _getURL = cb; _urlParam = param; }
    void callGetURL(MovieClip& movie, bool fscommand, const char *url, const char *target);

	RECT calculateRectangle(uint16_t character, const MATRIX* xf);

	struct EventContext {
		ICharacter *activeEntity;
		int		lastButtonState;
		bool	lastInsideEntity;
	};

private:
	void notifyEvent(EventContext&, int button, float x, float y, ICharacter* target, bool touchScreen);

	typedef std::map< uint32_t, TagFactoryFunc >    TagFactoryMap;
	typedef std::map< uint16_t, ITag* >             CharacterDictionary;
    typedef std::map< uint16_t, Asset >             AssetDictionary;
    typedef std::map< std::string, uint16_t >       SymbolDictionary;

	//float	_mouseX, _mouseY;
	EventContext _eventContext;

	float               _elapsedAccumulator;
	float               _elapsedAccumulatorDuplicate;
	MovieFrames 	    _swf_data;

	CharacterArray      _duplicates;
	Header				_header;
    CharacterDictionary	_dictionary;
    SymbolDictionary    _library;

    AssetDictionary     moAssets;
    COLOR4f             _bgColor;
    GetURLCallback      _getURL;
	void*				_urlParam;

	MATRIX3f	_currentMatrix;
	CXFORM		_currentCXForm;

	static TagFactoryMap            _tag_factories;
    static LoadAssetCallback        _asset_loader;
};

//-----------------------------------------------------------------------------

}//namespace
#endif // __TS_SWF_H__
