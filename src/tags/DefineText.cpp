/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "DefineText.h"
#include "DefineFont.h"
#include "PlaceObject.h"
#include "tsSWF.h"

/*
#define RAPIDXML_NO_EXCEPTIONS
void rapidxml::parse_error_handler(const char *what, void *where) {
	SWF_TRACE("xml error: %s", what);
}
*/
#include "rapidxml.hpp"

using namespace tinyswf;
using namespace rapidxml;

enum TEXT_STYLE {
	STYLE_HAS_FONT		= 0x08,
	STYLE_HAS_COLOR		= 0x04,
	STYLE_HAS_YOFFSET	= 0x02,
	STYLE_HAS_XOFFSET	= 0x01
};

enum EDITTEXT_INFO {
	INFO_HAS_TEXT		= 1<<7,
	INFO_WORDWRAP		= 1<<6,
	INFO_MULTILINE		= 1<<5,
	INFO_PASSWORD		= 1<<4,
	INFO_READONLY		= 1<<3,
	INFO_HAS_COLOR		= 1<<2,
	INFO_HAS_MAX_LEN	= 1<<1,
	INFO_HAS_FONT		= 1<<0,
	INFO_HAS_FONT_CLASS	= 1<<15,
	INFO_AUTO_SIZE		= 1<<14,
	INFO_HAS_LAYOUT		= 1<<13,
	INFO_NO_SELECT		= 1<<12,
	INFO_BORDER			= 1<<11,
	INFO_WAS_STATIC		= 1<<10,
	INFO_HTML			= 1<<9,
	INFO_USE_OUTLINES	= 1<<8,
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

bool DefineTextTag::read( Reader& reader, SWF& , MovieFrames& ) {
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

	int flag = reader.get<uint16_t>();
	if (flag & INFO_HAS_FONT) {
		_font_id = reader.get<uint16_t>();
		DefineFontTag* tag = (DefineFontTag*) swf.getCharacter(_font_id);
		SWF_ASSERT(tag);
		_style.font_name = tag->getFontName();
		_style.font_style = tag->getFontStyle();
	}
	if (flag & INFO_HAS_FONT_CLASS) {
		_font_class.assign( reader.getString() );
	}
	if (flag & INFO_HAS_FONT) {
		_style.font_height = reader.get<uint16_t>() * SWF_INV_TWIPS;
	}
	if (flag & INFO_HAS_COLOR) {
		_style.color.r = reader.get<uint8_t>()*SWF_INV_COLOR;
		_style.color.g = reader.get<uint8_t>()*SWF_INV_COLOR;
		_style.color.b = reader.get<uint8_t>()*SWF_INV_COLOR;
		_style.color.a = reader.get<uint8_t>()*SWF_INV_COLOR;
	}
	if (flag & INFO_HAS_MAX_LEN) {
		_max_length = reader.get<uint16_t>();
	}
	if (flag & INFO_HAS_LAYOUT) {
		//0 = Left; 1 = Right; 2 = Center; 3 = Justify 
		_style.alignment = (TextStyle::ALIGNMENT)reader.get<uint8_t>();
		_style.left_margin = reader.get<uint16_t>() * SWF_INV_TWIPS;
		_style.right_margin = reader.get<uint16_t>() * SWF_INV_TWIPS;
		_style.indent = reader.get<uint16_t>() * SWF_INV_TWIPS;
		_style.leading = reader.get<int16_t>() * SWF_INV_TWIPS;
	}

	_html = (flag & INFO_HTML) > 0;
	_style.multiline = (flag & INFO_MULTILINE) > 0;

	_variable_name.assign( reader.getString() );
	if (flag & INFO_HAS_TEXT) {
		_initial_text.assign( reader.getString() );
	}
	
	if ( FontHandler::getInstance() )
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

void traverse(xml_node<> *node, std::string& output, Text::ColorText& texts, TextStyle& style) {
	bool newline = false;
	bool colorbreak = false;
	const COLOR4f old_color = style.color;
	switch (node->type()) {
	case node_element:
		SWF_TRACE("name:%s\n", node->name());
		newline = (0 == strcmp("p", node->name()));
		for (xml_attribute<> *attr = node->first_attribute(); attr; attr = attr->next_attribute()) {
			SWF_TRACE("attr:%s = %s\n", attr->name(), attr->value());
			colorbreak = (0 == strcmp("color", attr->name()));
			if (colorbreak) {
				if (! output.empty()) {
					size_t count = texts.size();
					texts.resize(count+1);
					Text::ColorString& content = texts.back();
					content.color = style.color;
					content.string = output;
					output.clear();
				}
				unsigned long value = strtoul(attr->value()+1, NULL, 16);
				style.color.r = ((value & 0xff0000) >> 16) * SWF_INV_COLOR;
				style.color.g = ((value & 0xff00) >> 8) * SWF_INV_COLOR;
				style.color.b = ((value & 0xff)) * SWF_INV_COLOR;
			}
		}
		break;
	case node_data:
		SWF_TRACE("value:%s\n", node->value());
		output += node->value();
	default:
		break;
	}

	for (xml_node<> *child = node->first_node(); child; child = child->next_sibling()) {
		traverse(child, output, texts, style);
	}

	if (colorbreak) {
		size_t count = texts.size();
		texts.resize(count+1);
		Text::ColorString& content = texts.back();
		content.color = style.color;
		content.string = output;
		output.clear();
		style.color = old_color;
	}

	if (newline) {
		output += "\n";
	}
}

bool utf8_to_utf16(std::wstring& utf16, const std::string& utf8) {
    std::vector<unsigned long> unicode;
    size_t i = 0;
    while (i < utf8.size()) {
        unsigned long uni;
        size_t todo;
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

Text::Text(const DefineEditTextTag &tag, const PlaceObjectTag*def) 
	:_reference(tag)
{
	_style = tag._style;
	_bound = tag._bound;

	if (def) {
		_style.filter = def->getFilter();
	} else {
		_style.filter = NULL;
	}
	setString(tag._initial_text.c_str());
}

void Text::draw(SWF* owner) {
	//FontHandler::getInstance()->drawText(_vertices, _glyphs, owner->getCurrentCXForm(), _style);
	FontHandler *handler = FontHandler::getInstance();
	SWF_ASSERT( handler );
	TextStyle style = _style;
	ColorText::iterator it = _colorTexts.begin();
	while (it != _colorTexts.end()) {
		if (0 >= style.glyphs)
			return;
		int count = it->glyphs;
		if (count > style.glyphs)
			count = style.glyphs;
		style.glyphs -= count;
		style.color = it->color;
		handler->drawText(it->vertices, count, owner->getCurrentCXForm(), style);
		++it;
	}
}

bool Text::setString(const char* str) {
	FontHandler *handler = FontHandler::getInstance();
	if (! handler || ! str)
		return false;

	// draw all glyphs in str
	_style.glyphs =  0x7fffffff;

	TextStyle style = _style;
	// check html tag
	if (strstr(str,"<p")) {
		_colorTexts.clear();
		std::string input = str;
#ifdef RAPIDXML_NO_EXCEPTIONS
		xml_document<> doc;	// character type defaults to char
		doc.parse<0>(&input.front()); // 0 means default parse flags
		std::string output;
		traverse(&doc, output, _colorTexts, style);
		if (! output.empty()) {
			size_t count = _colorTexts.size();
			_colorTexts.resize(count+1);
			Text::ColorString& content = _colorTexts.back();
			content.color = style.color;
			content.string = output;
		}
#else
		try {
			xml_document<> doc;	// character type defaults to char
			doc.parse<0>(&input.front()); // 0 means default parse flags
			std::string output;
			traverse(&doc, output, _colorTexts, style);
			if (! output.empty()) {
				size_t count = _colorTexts.size();
				_colorTexts.resize(count+1);
				Text::ColorString& content = _colorTexts.back();
				content.color = style.color;
				content.string = output;
				style.alignment = TextStyle::ALIGN_LEFT;
			}
		} catch(std::exception e) {
			_colorTexts.resize(1);
			Text::ColorString& content = _colorTexts.back();
			content.color = style.color;
			content.string = str;
		}
#endif
	} else {
		_colorTexts.resize(1);
		Text::ColorString& content = _colorTexts.back();
		content.color = style.color;
		content.string = str;
	}

	float x = 0, y = 0;
	ColorText::iterator it = _colorTexts.begin();
	while (it != _colorTexts.end()) {
		std::wstring utf16;
		if (utf8_to_utf16(utf16, it->string.c_str())) {
			it->glyphs = handler->formatText(it->vertices, x, y, _bound, style, utf16);
		} else {
			return false;
		}
		++it;
	}
	return true;
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

