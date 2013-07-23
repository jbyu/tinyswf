#include "FontCache.h"
#include "cocos2d.h"

using namespace cocos2d;

FontData::FontData(const char *font_name, int fontsize) {
	bool ret = createFont(font_name, fontsize, _font);
	_cache = new GlyphCache(256);
	_bitmap = new CCTexture2D;
	_bitmap->initWithData(0, kCCTexture2DPixelFormat_A8, kTEXTURE_SIZE, kTEXTURE_SIZE, CCSizeMake(kTEXTURE_SIZE, kTEXTURE_SIZE));
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
}

FontData::~FontData() {
	destroyFont(_font);
	_bitmap->release();
	delete _cache;
}

GlyphInfo* FontData::getGlyph(wchar_t code) {
    GlyphInfo* glyph = _cache->fetch_ptr(code);
	if (glyph)
		return glyph;

    GlyphInfo entry;
	if (! getGlyph(_font, code, entry))
		return NULL;

	const int count = _cache->size();
	const bool bFull = _cache->full();
	//if (bFull) nIndex = -1;
	GlyphInfo lru = {0,0,0,0,0,0,0};
	if (bFull) {
		lru = _cache->getLRU();
	}

	GlyphInfo &feed = _cache->insert(code, entry);
	if (bFull) {
		feed.index = lru.index;
	} else {
		feed.index = count;
	}
	unsigned int nIndex = feed.index;

	// update texture
	int nOffsetY = nIndex / kNUMBER_GLYPH_PER_ROW;				// calculate y index first
	int nOffsetX = nIndex - (nOffsetY * kNUMBER_GLYPH_PER_ROW);	// calculate x index
	nOffsetY *= kGLYPH_WIDTH;      // translate to pixels offset
	nOffsetX *= kGLYPH_WIDTH;      // translate to pixels offset
	ccGLBindTexture2D( _bitmap->getName() );
  	glTexSubImage2D(GL_TEXTURE_2D, 0, nOffsetX, nOffsetY, kGLYPH_WIDTH, kGLYPH_WIDTH, GL_ALPHA, GL_UNSIGNED_BYTE, getBitmap());
	//++g_nGlyphCount;
	//if (TEXTURE_TABLE_NUM <= g_nGlyphCount)		NBDebug::trace("exceed glyph maximum!\n");
	return &feed;
}

FontCache* FontCache::_instance = NULL;

FontCache::FontCache()
	:_selectedFont(NULL)
{
	FontData::initialize();
}

FontCache::~FontCache() {
	CacheData::iterator it = _font_cache.begin();
	while (it != _font_cache.end()) {
		delete it->second;
		++it;
	}
	FontData::terminate();
}

cocos2d::CCTexture2D* FontCache::selectFont(const char *font_name, int fontsize) {
	CacheData::iterator it = _font_cache.find(font_name);
	if (it != _font_cache.end()) {
		_selectedFont = (it->second);
		return _selectedFont->_bitmap;
	}
	_selectedFont = new FontData(font_name, fontsize);
	_font_cache[font_name] = _selectedFont;
	return _selectedFont->_bitmap;
}

GlyphInfo* FontCache::getGlyph(wchar_t code) {
	return _selectedFont->getGlyph(code);
}
