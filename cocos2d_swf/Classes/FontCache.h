/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/
#ifndef __FONT_CACHE_H__
#define __FONT_CACHE_H__

#include "lru_cache.h"

const int kGLYPH_WIDTH  = 32;
const int kTEXTURE_SIZE = 1024;
const int kNUMBER_GLYPH_PER_ROW	= kTEXTURE_SIZE / kGLYPH_WIDTH; 
// number of glyph per row in the texture

namespace cocos2d {
	class Texture2D;
	class Dictionary;
}

struct GlyphInfo {
	float	advance;
	float	offsetX;
	float	offsetY;
	char	width;
	char	height;
	unsigned short index;
};

class FontData {
	friend class FontCache;

	typedef const void* Handle;
	typedef LRUCache<wchar_t, GlyphInfo> GlyphCache;

	// platform-depend
	static bool initialize(void);
	static bool createFont(const char *font_name, int fontsize, Handle& font);
	static void destroyFont(const Handle& font);
	static bool getGlyph(const Handle& font, wchar_t codepoint, GlyphInfo& entry);
	static void* getBitmap();
	static void terminate(void);

	cocos2d::Texture2D *_bitmap;
	GlyphCache *_cache;
	Handle _font;

	void updateTexture();

public:
	FontData(const char *font_name, int fontsize);
	~FontData();

	GlyphInfo* getGlyph(wchar_t code);
}; 

class FontCache {
private:

public:
	FontCache();
	virtual ~FontCache();

	static FontCache* _instance;
	
	typedef std::map<std::string, FontData*> CacheData;
	CacheData _font_cache;
	FontData* _selectedFont;

	cocos2d::Texture2D* selectFont(const char *font_name, int fontsize);
	GlyphInfo* getGlyph(wchar_t code);

	static FontCache* sharedGlyphCache() { return _instance; }
	static FontCache* createInstance() { _instance = new FontCache; return _instance; }
	static void destroyInstance() { delete _instance; _instance = NULL; }
};

#endif//__FONT_CACHE_H__