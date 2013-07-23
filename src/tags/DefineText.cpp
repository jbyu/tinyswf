/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "DefineText.h"
#include "DefineFont.h"
#include "tsSWF.h"
#include "..\rapidxml\rapidxml.hpp"

using namespace tinyswf;
using namespace rapidxml;

enum TEXT_STYLE {
	STYLE_HAS_FONT		= 0x08,
	STYLE_HAS_COLOR		= 0x04,
	STYLE_HAS_YOFFSET	= 0x02,
	STYLE_HAS_XOFFSET	= 0x01
};

enum EDITTEXT_INFO1 {
	INFO_HAS_TEXT		= 0x80,
	INFO_WORDWRAP		= 0x40,
	INFO_MULTILINE		= 0x20,
	INFO_PASSWORD		= 0x10,
	INFO_READONLY		= 0x08,
	INFO_HAS_COLOR		= 0x04,
	INFO_HAS_MAX_LEN	= 0x02,
	INFO_HAS_FONT		= 0x01,
};
enum EDITTEXT_INFO2 {
	INFO_HAS_FONT_CLASS	= 0x80,
	INFO_AUTO_SIZE		= 0x40,
	INFO_HAS_LAYOUT		= 0x20,
	INFO_NO_SELECT		= 0x10,
	INFO_BORDER			= 0x08,
	INFO_WAS_STATIC		= 0x04,
	INFO_HTML			= 0x02,
	INFO_USE_OUTLINES	= 0x01,
};

bool TextRecord::Style::read(Reader& reader, int tag_type, int flag) {
	if (flag & STYLE_HAS_FONT) {
		_font_id = reader.get<uint16_t>();
	}
	if (flag & STYLE_HAS_COLOR) {
		_color.r = reader.get<uint8_t>()*SWF_INV_COLOR;
		_color.g = reader.get<uint8_t>()*SWF_INV_COLOR;
		_color.b = reader.get<uint8_t>()*SWF_INV_COLOR;
		if (TAG_DEFINE_TEXT2 == tag_type)
			_color.a = reader.get<uint8_t>()*SWF_INV_COLOR;
		else
			_color.a = 1.0f;
	}
	if (flag & STYLE_HAS_XOFFSET) {
		_XOffset = reader.get<int16_t>() * SWF_INV_TWIPS;
	}
	if (flag & STYLE_HAS_YOFFSET) {
		_YOffset = reader.get<int16_t>() * SWF_INV_TWIPS;
	}
	if (flag & STYLE_HAS_FONT) {
		_font_height = reader.get<uint16_t>() * SWF_INV_TWIPS;
	}
	return true;
}

bool TextRecord::read(Reader& reader, int tag_type, int flag, int glyph_bits, int advance_bits, Style& prev) {
	prev.read(reader, tag_type, flag);
	_style = prev;

	int count = reader.get<uint8_t>();
	_glyphs.resize(count);
	for (int i = 0; i < count; ++i) {
		_glyphs[i]._index = reader.getbits(glyph_bits);
		_glyphs[i]._advance = reader.getsignedbits(advance_bits) * SWF_INV_TWIPS;
	}

	return true;
}

//-----------------------------------------------------------------------------

bool DefineTextTag::read( Reader& reader, SWF& swf, MovieFrames& ) {
	_character_id = reader.get<uint16_t>();
	reader.getRectangle(_bound);
	reader.align();
	reader.getMatrix(_transform);
	reader.align();
	int glyph_bits = reader.get<uint8_t>();
	int advance_bits = reader.get<uint8_t>();

	TextRecord::Style style = {0,0,0,0, {0,0,0,1}};
	while(1) {
		int flag = reader.get<uint8_t>();
		if (0 == flag)
			break;

		_texts.resize(_texts.size() + 1);
		_texts.back().read(reader, _header.code(), flag, glyph_bits, advance_bits, style);
	}

	//swf.addCharacter( this, _character_id );
    return true; // keep tag
}

void DefineTextTag::print() {
   	_header.print();
	SWF_TRACE("id=%d, ", _character_id);
	SWF_TRACE("RECT:{%.2f,%.2f,%.2f,%.2f}\n",
		_bound.xmin, _bound.ymin,
		_bound.xmax, _bound.ymax);
	SWF_TRACE("MTX:%3.2f,%3.2f,%3.2f,%3.2f,%3.2f,%3.2f\n",			
		_transform.m_[0][0], _transform.m_[0][1], _transform.m_[0][2],	
		_transform.m_[1][0], _transform.m_[1][1], _transform.m_[1][2]);
}

//-----------------------------------------------------------------------------

bool DefineEditTextTag::read( Reader& reader, SWF& swf, MovieFrames& ) {
	_character_id = reader.get<uint16_t>();
	reader.getRectangle(_bound);
	reader.align();

	int flag1 = reader.get<uint8_t>();
	int flag2 = reader.get<uint8_t>();
	if (flag1 & INFO_HAS_FONT) {
		_font_id = reader.get<uint16_t>();
		DefineFontTag* tag = (DefineFontTag*) swf.getCharacter(_font_id);
		SWF_ASSERT(tag);
		_style.font_name = tag->getFontName();
	}
	if (flag2 & INFO_HAS_FONT_CLASS) {
		_font_class.assign( reader.getString() );
	}
	if (flag1 & INFO_HAS_FONT) {
		_style.font_height = reader.get<uint16_t>() * SWF_INV_TWIPS;
#ifdef WIN32
		_style.font_height *= 1.1f;
#endif
	}
	if (flag1 & INFO_HAS_COLOR) {
		_style.color.r = reader.get<uint8_t>()*SWF_INV_COLOR;
		_style.color.g = reader.get<uint8_t>()*SWF_INV_COLOR;
		_style.color.b = reader.get<uint8_t>()*SWF_INV_COLOR;
		_style.color.a = reader.get<uint8_t>()*SWF_INV_COLOR;
	}
	if (flag1 & INFO_HAS_MAX_LEN) {
		_max_length = reader.get<uint16_t>();
	}
	if (flag2 & INFO_HAS_LAYOUT) {
		//0 = Left; 1 = Right; 2 = Center; 3 = Justify 
		_style.alignment = (TextStyle::ALIGNMENT)reader.get<uint8_t>();
		_style.left_margin = reader.get<uint16_t>() * SWF_INV_TWIPS;
		_style.right_margin = reader.get<uint16_t>() * SWF_INV_TWIPS;
		_style.indent = reader.get<uint16_t>() * SWF_INV_TWIPS;
		_style.leading = reader.get<int16_t>() * SWF_INV_TWIPS;
	}

	_html = (flag2 & INFO_HTML) > 0;
	_style.multiline = (flag1 & INFO_MULTILINE) > 0;

	_variable_name.assign( reader.getString() );
	if (flag1 & INFO_HAS_TEXT) {
		_initial_text.assign( reader.getString() );
	}
	
	swf.addCharacter( this, _character_id );
    return true; // keep tag
}

void DefineEditTextTag::print() {
   	_header.print();
	SWF_TRACE("id=%d, ", _character_id);
	SWF_TRACE("RECT:{%.2f,%.2f,%.2f,%.2f}\n", _bound.xmin, _bound.ymin,	_bound.xmax, _bound.ymax);
	SWF_TRACE("font=%d, height=%f, variable=%s\ntext=%s\n", _font_id, _style.font_height, _variable_name.c_str(), _initial_text.c_str());
}

//-----------------------------------------------------------------------------

void traverse(xml_node<> *node, std::string& output) {
	switch (node->type()) {
	case node_element:
		SWF_TRACE("name:%s\n", node->name());
		for (xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
			SWF_TRACE("attr:%s = %s\n", attr->name(), attr->value());
		}
		break;
	case node_data:
		SWF_TRACE("value:%s\n", node->value());
		output += node->value();
	default:
		break;
	}
	for (xml_node<> *child = node->first_node(); child; child = child->next_sibling()) {
		traverse(child, output);
	}

}

bool utf8_to_utf16(std::wstring& utf16, const std::string& utf8) {
    std::vector<unsigned long> unicode;
    size_t i = 0;
    while (i < utf8.size()) {
        unsigned long uni;
        size_t todo;
        bool error = false;
        unsigned char ch = utf8[i++];
        if (ch <= 0x7F) {
            uni = ch;
            todo = 0;
        } else if (ch <= 0xBF) {
            return false;
        } else if (ch <= 0xDF) {
            uni = ch&0x1F;
            todo = 1;
        } else if (ch <= 0xEF) {
            uni = ch&0x0F;
            todo = 2;
        } else if (ch <= 0xF7) {
            uni = ch&0x07;
            todo = 3;
        } else {
            return false;
        }
        for (size_t j = 0; j < todo; ++j) {
            if (i == utf8.size())
				return false;
            unsigned char ch = utf8[i++];
            if (ch < 0x80 || ch > 0xBF)
				return false;
            uni <<= 6;
            uni += ch & 0x3F;
        }
        if (uni >= 0xD800 && uni <= 0xDFFF)
			return false;
        if (uni > 0x10FFFF)
			return false;
        unicode.push_back(uni);
    }
	utf16.clear();
	utf16.reserve( unicode.size() );
    for (size_t i = 0; i < unicode.size(); ++i) {
        unsigned long uni = unicode[i];
        if (uni <= 0xFFFF) {
            utf16 += (wchar_t)uni;
        } else {
            uni -= 0x10000;
            utf16 += (wchar_t)((uni >> 10) + 0xD800);
            utf16 += (wchar_t)((uni & 0x3FF) + 0xDC00);
        }
    }
    return true;
}

Text::Text(const DefineEditTextTag &tag) 
	:_reference(tag)
{
	_style = tag._style;
	_bound = tag._bound;

	if (tag._html) {
		// parse initial html text
		char *in = (char*)tag._initial_text.c_str();
		xml_document<> doc;	// character type defaults to char
		doc.parse<0>(in);	// 0 means default parse flags
		std::string text;
		traverse(&doc, text);
		setString(text.c_str());
	} else {
		setString(tag._initial_text.c_str());
	}

}

void Text::draw(void) {
	Renderer::getRenderer()->drawText(_vertices, _bound, _style, _text);
}

const int kSIZE_PER_GLYPH = 12; // (xy,uv) * 6 per glyph

bool Text::setString(const char* str) {
	bool ret = utf8_to_utf16(_text, str);
	_vertices.resize(_text.size()*kSIZE_PER_GLYPH);
	Renderer::getRenderer()->formatText(_vertices, _bound, _style, _text);
	return ret;
}

// HTML content
// <p> ... </p> Defines a paragraph. The attribute align may be present, with value left, right, or center.
// <br> Inserts a line break.
// <a> ... </a> Defines a hyperlink.
// <font> ... </font> Defines a span of text that uses a given font. 
//		*face, which specifies a font name
//		*size, which is specified in twips, and may include a leading '+' or '-' for relative sizes
//		*color, which is specified as a #RRGGBB hex triplet
// <b> ... </b> Defines a span of bold text.
// <i> ... </i> Defines a span of italic text.
// <u> ... </u> Defines a span of underlined text.
// <li> ... </li> Defines a bulleted paragraph.
// <textformat> ... </textformat>
// <tab> Inserts a tab character
//
// sample:
// <p align="left"><font face="Times New Roman" size="74" color="#cccccc"
// letterSpacing="0.000000" kerning="0">aaa</font></p>

