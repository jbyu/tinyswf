#include "FontCache.h"
#include "cocos2d.h"

using namespace cocos2d;
using namespace tinyswf;

const int kTEXTURE_SIZE = 2048;
const int kNUMBER_GLYPH_PER_ROW	= kTEXTURE_SIZE / kGLYPH_WIDTH; 
// number of glyph per row in the texture

OSFont::OSFont(const char *font_name, float fontsize, int style) {
	_font = create(font_name, fontsize, style);
	_size = fontsize;
	SWF_ASSERT(_font);
	_cache = new GlyphCache(kNUMBER_GLYPH_PER_ROW * kNUMBER_GLYPH_PER_ROW);
	_bitmap = new Texture2D;
	const float tex_width = kTEXTURE_SIZE;
	_bitmap->initWithData(0, 0, Texture2D::PixelFormat::A8, kTEXTURE_SIZE, kTEXTURE_SIZE, Size(tex_width,tex_width));
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
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
	GL::bindTexture2D( _bitmap->getName() );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, nOffsetX, nOffsetY, kGLYPH_WIDTH, kGLYPH_WIDTH, GL_ALPHA, GL_UNSIGNED_BYTE, getGlyphBitmap());

	return &feed;
}

//-----------------------------------------------------------------------------

const char fontShader_vert[] =		"\n\
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
const char fontShader_frag[] =			"\n\
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

const char fontShader_fragShadow[] =	"\n\
#ifdef GL_ES							\n\
precision mediump float;					\n\
#endif									\n\
										\n\
varying vec2 v_texCoord;				\n\
uniform sampler2D CC_Texture0;			\n\
uniform	vec4 u_color;					\n\
uniform	vec4 u_shadow;					\n\
uniform	vec2 u_offset;					\n\
										\n\
void main()								\n\
{										\n\
	float sAlpha = texture2D(CC_Texture0, v_texCoord + u_offset ).a; \n\
	float alpha = texture2D(CC_Texture0, v_texCoord).a; \n\
	vec4 shadow = u_shadow * sAlpha;	\n\
	gl_FragColor = mix(shadow, u_color, alpha);	\n\
}										\n\
";

CCFlashFontHandler::CCFlashFontHandler() {
	OSFont::initialize();
	// font shader
    _fontShader = new GLProgram();
	_fontShader->initWithVertexShaderByteArray(fontShader_vert, fontShader_frag);
	_fontShader->addAttribute(GLProgram::ATTRIBUTE_NAME_POSITION, GLProgram::VERTEX_ATTRIB_POSITION);
	_fontShader->addAttribute(GLProgram::ATTRIBUTE_NAME_TEX_COORD, GLProgram::VERTEX_ATTRIB_TEX_COORDS);
    _fontShader->link();
    _fontShader->updateUniforms();
	_uvScaleLocation = glGetUniformLocation( _fontShader->getProgram(), "u_uvScale");
	_colorLocation = glGetUniformLocation( _fontShader->getProgram(), "u_color");
    CHECK_GL_ERROR_DEBUG();
	// font shadow shader
    _fontShader_Shadow = new GLProgram();
	_fontShader_Shadow->initWithVertexShaderByteArray(fontShader_vert, fontShader_fragShadow);
	_fontShader_Shadow->addAttribute(GLProgram::ATTRIBUTE_NAME_POSITION, GLProgram::VERTEX_ATTRIB_POSITION);
	_fontShader_Shadow->addAttribute(GLProgram::ATTRIBUTE_NAME_TEX_COORD, GLProgram::VERTEX_ATTRIB_TEX_COORDS);
    _fontShader_Shadow->link();
    _fontShader_Shadow->updateUniforms();
	_uvScale_Shadow = glGetUniformLocation( _fontShader_Shadow->getProgram(), "u_uvScale");
	_uvOffset_Shadow = glGetUniformLocation( _fontShader_Shadow->getProgram(), "u_offset");
	_fonstColor_Shadow = glGetUniformLocation( _fontShader_Shadow->getProgram(), "u_color");
	_shadowColor_Shadow = glGetUniformLocation( _fontShader_Shadow->getProgram(), "u_shadow");
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
	CC_SAFE_RELEASE_NULL(_fontShader_Shadow);
}

OSFont* CCFlashFontHandler::selectFont(const char *font_name, float fontsize, int style) {
	CacheData::iterator it = _font_cache.find(font_name);
	if (it != _font_cache.end()) {
		_selectedFont = (it->second);
		_scale = fontsize / _selectedFont->getSize();
		return _selectedFont;
	}
	_selectedFont = new OSFont(font_name, fontsize, style);
	_font_cache[font_name] = _selectedFont;
	_scale = 1.f;
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

uint32_t format(FormatText& out, float &lastX, float &lastY,
				const tinyswf::RECT& rect,
				const TextStyle& style,
				const std::wstring& text,
				CCFlashFontHandler *handler, float scale)
{
	const float line_height = handler->getLineHeight() * scale;
	const float width = rect.xmax - rect.xmin - style.right_margin - style.left_margin;

	std::wstring::const_iterator start = text.begin();
	std::wstring::const_iterator it;

	uint32_t numGlyphs = 0;
	AlignData data = {rect.ymin + lastY, rect.xmin + style.left_margin + lastX, width, style.indent};
	for (it = start; it != text.end(); ++it) {
		GlyphInfo *glyph = handler->getGlyph(*it);
		if (! glyph) continue;
		//wchar_t ch = *it;
		const float advance = glyph->advance * scale;
		const float result = data.length + advance;
		const bool newline = (('\n' == *it)||('\r' == *it));
		if (result + lastX > width || newline) {
			if (! style.multiline) break;

			align(out, style, data);
			out.back().text.assign(start, it);
			numGlyphs += out.back().text.size();

			lastX = 0;
			data.positionX = rect.xmin + style.left_margin;
			data.positionY += line_height + style.leading*1.25f;
			if (newline) {
				data.length = style.indent;
				start = it+1;
			} else {
				data.length = style.indent + advance;
				start = it;
			}
		} else {
			data.length = result;
		}
	}
	lastX += data.length;
	lastY = data.positionY - rect.ymin;

	align(out, style, data);
	out.back().text.assign(start, it);
	numGlyphs += out.back().text.size();

	return numGlyphs;
}

const int kVERTICES_PER_GLYPH = 6; // (xy,uv) * 6 per glyph

uint32_t CCFlashFontHandler::formatText(VertexArray& vertices, float &lastX, float &lastY,
										const tinyswf::RECT& rect,
										const TextStyle& style,
										const std::wstring& text) {
	//select font to get glyph
	this->selectFont(style.font_name.c_str(), style.font_height, style.font_style);

	FormatText lines;
	uint32_t numGlyphs = format(lines, lastX, lastY, rect, style, text, this, _scale);

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
			float x = lx + glyph->offsetX *_scale;
			float y = ly + glyph->offsetY *_scale;
			float w = glyph->width *_scale;
			float h = glyph->height *_scale;

			tinyswf::POINT xy[4], uv[4];
			xy[0].x = x; xy[0].y = y;
			xy[1].x = x; xy[1].y = y + h;
			xy[2].x = x + w; xy[2].y = y;
			xy[3].x = x + w; xy[3].y = y + h;

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

			lx += glyph->advance*_scale;
		}
	}
	_areaHeight = lines.back().line_gap;

	return numGlyphs * kVERTICES_PER_GLYPH;
}

void CCFlashFontHandler::drawText(const VertexArray& vertices, uint32_t count, const CXFORM& cxform, const TextStyle& style) {
	if (0 == count)
		return;

	const float uv_scale = 1.f / kTEXTURE_SIZE;
	GL::enableVertexAttribs(GL::VERTEX_ATTRIB_FLAG_POSITION | GL::VERTEX_ATTRIB_FLAG_TEX_COORDS);

	if (style.filter) {
		float offsetx = style.filter->offsetX * uv_scale;
		float offsety = style.filter->offsetY * uv_scale;
		COLOR4f color = style.color;
		COLOR4f shadow = style.filter->color;
		color *= cxform.mult;
		shadow.a *= cxform.mult.a;
		_fontShader_Shadow->use();
		_fontShader_Shadow->setUniformsForBuiltins();
		_fontShader_Shadow->setUniformLocationWith1f(_uvScale_Shadow, uv_scale);
		_fontShader_Shadow->setUniformLocationWith2f(_uvOffset_Shadow, -offsetx, -offsety);
		_fontShader_Shadow->setUniformLocationWith4fv(_fonstColor_Shadow, (GLfloat*) &color.r, 1);
		_fontShader_Shadow->setUniformLocationWith4fv(_shadowColor_Shadow, (GLfloat*) &shadow.r, 1);
	} else {
		tinyswf::COLOR4f color = cxform.mult * style.color;
		color += cxform.add;
		_fontShader->use();
		_fontShader->setUniformsForBuiltins();
		_fontShader->setUniformLocationWith1f(_uvScaleLocation, uv_scale);
		_fontShader->setUniformLocationWith4fv(_colorLocation, (GLfloat*) &color.r, 1);
	}

	OSFont *font = selectFont(style.font_name.c_str(), style.font_height, style.font_style);
	GL::bindTexture2D(font->_bitmap->getName());
	
	float *data = (float*)&(vertices.front().x);
	glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, data);
	glVertexAttribPointer(GLProgram::VERTEX_ATTRIB_TEX_COORDS, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, data+2);
	glDrawArrays(GL_TRIANGLES, 0, count);
    CC_INCREMENT_GL_DRAWS(1);
}
