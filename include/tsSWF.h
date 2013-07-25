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

    virtual uint32_t formatText(VertexArray& vertices, const RECT& rect, const TextStyle& style, const std::wstring& text) = 0;
    virtual void drawText(const VertexArray& vertices, uint32_t glyphs, const RECT& rect, const TextStyle& style, const std::wstring& text) = 0;

    virtual void drawBegin(void) = 0;
    virtual void drawEnd(void) = 0;

    virtual void maskBegin(void) = 0;
    virtual void maskEnd(void) = 0;
    virtual void maskOffBegin(void) = 0;
    virtual void maskOffEnd(void) = 0;

    virtual void clear( void ) {}

    static Renderer* getRenderer(void) { return spRenderer; }
    static void setRenderer(Renderer *r) { spRenderer = r; }
};

//-----------------------------------------------------------------------------

class Speaker {
	static Speaker *spSpeaker;

public:
    virtual ~Speaker() {}

    virtual unsigned int getSound( const char *filename ) = 0;
    virtual void playSound( unsigned int sound, bool stop, bool noMultiple, bool loop ) = 0;

    static Speaker* getSpeaker(void) { return spSpeaker; }
    static void setSpeaker(Speaker *r) { spSpeaker = r; }
};

//-----------------------------------------------------------------------------

class SWF : public MovieClip {
public:
    // factory function prototype
	typedef ITag* (*TagFactoryFunc)( TagHeader& );
    typedef Asset (*LoadAssetCallback)( const char *name, bool import );
    typedef void (*GetURLCallback)( MovieClip&, bool isFSCommand, const char *command, const char *target );

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
		
	// duplicate movieclip for other purpose outside flash
    MovieClip *duplicate(const char *name);
	// update all duplicated movieclips
    void updateDuplicate(float delta);
	// draw all duplicated movieclips
    void drawDuplicate(void);

	static void drawMovieClip(MovieClip *movie, float alpha=1.f);

	static MATRIX3f& getCurrentMatrix(void) { return _sCurrentMatrix; }

	static CXFORM& getCurrentCXForm(void) { return _sCurrentCXForm; }

	// animate flash
    void update(float delta);
    void draw(void);

	// button: 0 for up, 1 for down
	void notifyMouse(int button, int x, int y);
	void notifyDuplicate(int button, int x, int y);

	// flash information
	float getFrameWidth() const     { return _header.getFrameWidth(); }
	float getFrameHeight() const    { return _header.getFrameHeight(); }
	float getFrameRate() const      { return _header.getFrameRate(); }

    COLOR4f& getBackgroundColor(void) { return _bgColor; }

	// handle export/import assets
    bool addAsset(uint16_t id, const char *name, bool import);
    const Asset& getAsset(uint16_t id) const  {
        AssetDictionary::const_iterator it = moAssets.find(id);
        if (moAssets.end() != it)
            return it->second;
        return kNULL_ASSET;
    }
       
	// handle GetURL function (include fscommand)
    void setGetURL(GetURLCallback cb) { _getURL = cb; }
    GetURLCallback getGetURL(void) { return _getURL; }

	RECT calculateRectangle(uint16_t character, const MATRIX* xf);

private:
	void notifyEvent(int button, int x, int y, ICharacter* target);

	typedef std::map< uint32_t, TagFactoryFunc >    TagFactoryMap;
	typedef std::map< uint16_t, ITag* >             CharacterDictionary;
    typedef std::map< uint16_t, Asset >             AssetDictionary;
    typedef std::map< std::string, uint16_t >       SymbolDictionary;

	int		_mouseX, _mouseY;
	int		_mouseButtonStateCurr;
	int		_mouseButtonStateLast;
	bool	_mouseInsideEntityLast;
	ICharacter *_pActiveEntity;

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

	static TagFactoryMap            _tag_factories;
    static LoadAssetCallback        _asset_loader;
	static MATRIX3f					_sCurrentMatrix;
	static CXFORM					_sCurrentCXForm;
};

//-----------------------------------------------------------------------------

}//namespace
#endif // __TS_SWF_H__
