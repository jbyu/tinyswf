#include "stdafx.h"
#include "FontCache.h"
#include <GL/freeglut.h>
#include "SOIL.h"

using namespace tinyswf;

const int kTEXTURE_SIZE = 512;
const int kNUMBER_GLYPH_PER_ROW	= kTEXTURE_SIZE / kGLYPH_WIDTH; 
// number of glyph per row in the texture

OSFont::OSFont(const char *font_name, float fontsize, int style) {
	_font = create(font_name, fontsize, style);
	SWF_ASSERT(_font);
	_cache = new GlyphCache(kNUMBER_GLYPH_PER_ROW * kNUMBER_GLYPH_PER_ROW);

	glGenTextures(1, &_bitmap);
	glBindTexture(GL_TEXTURE_2D, _bitmap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, kTEXTURE_SIZE, kTEXTURE_SIZE, 0, GL_ALPHA, GL_UNSIGNED_BYTE, 0); 

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
}

OSFont::~OSFont() {
	destroy(_font);
	glDeleteTextures(1, &_bitmap);
	delete _cache;
}

GlyphInfo* OSFont::getGlyph(wchar_t code) {
    GlyphInfo* glyph = _cache->fetch_ptr(code);
	if (glyph)
		return glyph;

    GlyphInfo entry;
	if (! makeGlyph(_font, code, entry))
		return NULL;

	const uint16_t count = _cache->size();
	const bool bFull = _cache->full();
	//if (bFull) nIndex = -1;
	GlyphInfo lru = {0,0,0,0,0,0};
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
    glBindTexture(GL_TEXTURE_2D, _bitmap);
  	glTexSubImage2D(GL_TEXTURE_2D, 0, nOffsetX, nOffsetY, kGLYPH_WIDTH, kGLYPH_WIDTH, GL_ALPHA, GL_UNSIGNED_BYTE, getGlyphBitmap());

	return &feed;
}

//-----------------------------------------------------------------------------

GLFontHandler::GLFontHandler() {
	OSFont::initialize();
}

GLFontHandler::~GLFontHandler() {
	CacheData::iterator it = _font_cache.begin();
	while (it != _font_cache.end()) {
		delete (it->second);
		++it;
	}
	OSFont::terminate();
}

OSFont* GLFontHandler::selectFont(const char *font_name, float fontsize, int style) {
	CacheData::iterator it = _font_cache.find(font_name);
	if (it != _font_cache.end()) {
		_selectedFont = (it->second);
		return _selectedFont;
	}
	_selectedFont = new OSFont(font_name, fontsize, style);
	_font_cache[font_name] = _selectedFont;
	return _selectedFont;
}

//-----------------------------------------------------------------------------

struct FormatString {
	float indent;
	float line_gap;
	std::wstring text;
};
typedef std::vector<FormatString> FormatText;

struct AlignData {
	float positionY;
	float positionX;
	float width;
	float length;
};

void align(FormatText& out, const TextStyle& style, const AlignData& data) {
	float indent = 0;
	switch(style.alignment) {
	case TextStyle::ALIGN_CENTER:
		indent = data.positionX + (data.width - data.length) * 0.5f;
		break;
	case TextStyle::ALIGN_RIGHT:
		indent = data.positionX + (data.width - data.length);
		break;
	default:
		indent = data.positionX;
		break;
	}
	if (0 == out.size()) indent += style.indent;
	out.resize(out.size() + 1);
	out.back().indent = indent;
	out.back().line_gap = data.positionY;
}

uint32_t format(FormatText& out,
				const tinyswf::RECT& rect,
				const TextStyle& style,
				const std::wstring& text,
				GLFontHandler *handler) {
	const float line_height = handler->getLineHeight();
	const float width = rect.xmax - rect.xmin - style.right_margin - style.left_margin;

	std::wstring::const_iterator start = text.begin();
	std::wstring::const_iterator it;

	uint32_t numGlyphs = 0;
	AlignData data = {rect.ymin, rect.xmin + style.left_margin, width, style.indent};
	for (it = start; it != text.end(); ++it) {
		GlyphInfo *glyph = handler->getGlyph(*it);
		if (! glyph) continue;
		//wchar_t ch = *it;
		float result = data.length + glyph->advance;
		if (result > width) {
			if (! style.multiline) break;

			align(out, style, data);
			out.back().text.assign(start, it);
			numGlyphs += out.back().text.size();

			data.positionY += line_height + style.leading;
			data.length = glyph->advance;
			start = it;
		} else {
			data.length = result;
		}
	}
	align(out, style, data);
	out.back().text.assign(start, it);
	numGlyphs += out.back().text.size();

	return numGlyphs;
}

const int kVERTICES_PER_GLYPH = 6; // (xy,uv) * 6 per glyph

uint32_t GLFontHandler::formatText(VertexArray& vertices, 
										const tinyswf::RECT& rect,
										const TextStyle& style,
										const std::wstring& text) {
	//select font to get glyph
	this->selectFont(style.font_name.c_str(), style.font_height, style.font_style);

	FormatText lines;
	uint32_t numGlyphs = format(lines, rect, style, text, this);

	vertices.resize( numGlyphs * kVERTICES_PER_GLYPH * 2 );
	VertexArray::iterator it = vertices.begin();
	for (FormatText::iterator line = lines.begin(); line != lines.end(); ++line) {
		float lx = line->indent;
		float ly = line->line_gap;
		for (std::wstring::iterator ch = line->text.begin(); ch != line->text.end(); ++ch) {
			GlyphInfo *glyph = this->getGlyph(*ch);
			if (! glyph) continue;

			int gy = glyph->index / kNUMBER_GLYPH_PER_ROW;			// calculate y index first
			int gx = glyph->index - (gy * kNUMBER_GLYPH_PER_ROW);	// calculate x index
			float uvX = float(gx * kGLYPH_WIDTH);
			float uvY = float(gy * kGLYPH_WIDTH);
			float x = lx + glyph->offsetX;
			float y = ly + glyph->offsetY;

			tinyswf::POINT xy[4], uv[4];
			xy[0].x = x; xy[0].y = y;
			xy[1].x = x; xy[1].y = y + glyph->height;
			xy[2].x = x + glyph->width; xy[2].y = y;
			xy[3].x = x + glyph->width; xy[3].y = y + glyph->height;

			uv[0].x = uvX; uv[0].y = uvY;
			uv[1].x = uvX; uv[1].y = uvY + glyph->height;
			uv[2].x = uvX + glyph->width; uv[2].y = uvY;
			uv[3].x = uvX + glyph->width; uv[3].y = uvY + glyph->height;

			*it++ = xy[0]; *it++ = uv[0];
			*it++ = xy[1]; *it++ = uv[1];
			*it++ = xy[2]; *it++ = uv[2];
			*it++ = xy[1]; *it++ = uv[1];
			*it++ = xy[2]; *it++ = uv[2];
			*it++ = xy[3]; *it++ = uv[3];

			lx += glyph->advance;
		}
	}
	return numGlyphs * kVERTICES_PER_GLYPH;
}

void GLFontHandler::drawText(const VertexArray& vertices, uint32_t count, const tinyswf::CXFORM& cxform, const TextStyle& style) {
	const float uv_scale = 1.f / kTEXTURE_SIZE;
	const float texMtx[] = {uv_scale,0,0,0, 0,uv_scale,0,0, 0,0,1,0, 0,0,0,1};

	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf(texMtx);
	glMatrixMode(GL_MODELVIEW);

	OSFont *font = selectFont(style.font_name.c_str(), style.font_height, style.font_style);
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, font->_bitmap);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	tinyswf::COLOR4f color = cxform.mult * style.color;
	color += cxform.add;

	glColor4f(color.r, color.g, color.b, color.a);

	glBegin(GL_TRIANGLES);
	tinyswf::VertexArray::const_iterator it = vertices.begin(); 
	while(it != vertices.end()) {
		float x = it->x;
		float y = it->y;
		++it;
		glTexCoord2f(it->x, it->y);
		glVertex2f(x, y);
		++it;
	}
	glEnd();
}
