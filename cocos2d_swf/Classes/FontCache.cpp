#include "FontCache.h"
#include "cocos2d.h"

using namespace cocos2d;
using namespace tinyswf;

const int kTEXTURE_SIZE = 512;
const int kNUMBER_GLYPH_PER_ROW	= kTEXTURE_SIZE / kGLYPH_WIDTH; 
// number of glyph per row in the texture

OSFont::OSFont(const char *font_name, float fontsize, int style) {
	_font = create(font_name, fontsize, style);
	SWF_ASSERT(_font);
	_cache = new GlyphCache(kNUMBER_GLYPH_PER_ROW * kNUMBER_GLYPH_PER_ROW);
	_bitmap = new Texture2D;
	const float tex_width = kTEXTURE_SIZE;
	_bitmap->initWithData(0, kCCTexture2DPixelFormat_A8, kTEXTURE_SIZE, kTEXTURE_SIZE, Size(tex_width,tex_width));
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    //glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
}

OSFont::~OSFont() {
	destroy(_font);
	_bitmap->release();
	delete _cache;
}

GlyphInfo* OSFont::getGlyph(wchar_t code) {
    GlyphInfo* glyph = _cache->fetch_ptr(code);
	if (glyph)
		return glyph;

    GlyphInfo entry;
	if (! makeGlyph(_font, code, entry))
		return NULL;

	const int count = _cache->size();
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
	ccGLBindTexture2D( _bitmap->getName() );
  	glTexSubImage2D(GL_TEXTURE_2D, 0, nOffsetX, nOffsetY, kGLYPH_WIDTH, kGLYPH_WIDTH, GL_ALPHA, GL_UNSIGNED_BYTE, getGlyphBitmap());

	return &feed;
}

//-----------------------------------------------------------------------------

const char fontShader_vert[] = "	\n\
attribute vec4 a_position;			\n\
attribute vec2 a_texCoord;			\n\
uniform float u_uvScale;			\n\
									\n\
#ifdef GL_ES						\n\
varying mediump vec2 v_texCoord;	\n\
#else								\n\
varying vec2 v_texCoord;			\n\
#endif								\n\
									\n\
void main()							\n\
{									\n\
    gl_Position = CC_MVPMatrix * a_position;	\n\
	v_texCoord = a_texCoord * u_uvScale;		\n\
}									\n\
";
const char fontShader_frag[] =		"   \n\
#ifdef GL_ES							\n\
precision lowp float;					\n\
#endif									\n\
										\n\
varying vec2 v_texCoord;				\n\
uniform sampler2D CC_Texture0;			\n\
uniform	vec4 u_color;					\n\
										\n\
void main()								\n\
{										\n\
	float a = texture2D(CC_Texture0, v_texCoord).a; \n\
	gl_FragColor = vec4(1.0,1.0,1.0,a) * u_color;	\n\
}										\n\
";

CCFlashFontHandler::CCFlashFontHandler() {
	OSFont::initialize();
	// font shader
    _fontShader = new GLProgram();
	_fontShader->initWithVertexShaderByteArray(fontShader_vert, fontShader_frag);
	_fontShader->addAttribute(kAttributeNamePosition, kVertexAttrib_Position);
    _fontShader->addAttribute(kAttributeNameTexCoord, kVertexAttrib_TexCoords);
    _fontShader->link();
    _fontShader->updateUniforms();
	_uvScaleLocation = glGetUniformLocation( _fontShader->getProgram(), "u_uvScale");
	_colorLocation = glGetUniformLocation( _fontShader->getProgram(), "u_color");
    CHECK_GL_ERROR_DEBUG();
}

CCFlashFontHandler::~CCFlashFontHandler() {
	CacheData::iterator it = _font_cache.begin();
	while (it != _font_cache.end()) {
		delete (it->second);
		++it;
	}
	OSFont::terminate();
	CC_SAFE_RELEASE_NULL(_fontShader);
}

OSFont* CCFlashFontHandler::selectFont(const char *font_name, float fontsize, int style) {
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
				CCFlashFontHandler *handler) {
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

uint32_t CCFlashFontHandler::formatText(VertexArray& vertices, 
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

void CCFlashFontHandler::drawText(const VertexArray& vertices, uint32_t count, const CXFORM& cxform, const TextStyle& style) {
	const float uv_scale = 1.f / kTEXTURE_SIZE;
	ccGLEnableVertexAttribs(kVertexAttribFlag_Position | kVertexAttribFlag_TexCoords);

	tinyswf::COLOR4f color = cxform.mult * style.color;
	color += cxform.add;

    _fontShader->use();
	_fontShader->setUniformsForBuiltins();
	_fontShader->setUniformLocationWith1f(_uvScaleLocation, uv_scale);
	_fontShader->setUniformLocationWith4fv(_colorLocation, (GLfloat*) &color.r, 1);

	OSFont *font = selectFont(style.font_name.c_str(), style.font_height, style.font_style);
	ccGLBindTexture2D(font->_bitmap->getName());
	
	float *data = (float*)&(vertices.front().x);
	glVertexAttribPointer(kVertexAttrib_Position, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, data);
	glVertexAttribPointer(kVertexAttrib_TexCoords, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, data+2);
	glDrawArrays(GL_TRIANGLES, 0, count);
    CC_INCREMENT_GL_DRAWS(1);
}
