/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "DefineFont.h"
#include "tsSWF.h"

using namespace tinyswf;

enum FONT_FLAG {
	FLAG_HAS_LAYOUT		= 0x80,
	FLAG_SHIFT_JIS		= 0x40,
	FLAG_SMALL_TEXT		= 0x20,
	FLAG_ANSI			= 0x10,
	FLAG_WIDE_OFFSETS	= 0x08,
	FLAG_WIDE_CODES		= 0x04,
	FLAG_ITALIC			= 0x02,
	FLAG_BOLD			= 0x01
};

bool DefineFontTag::read( Reader& reader, SWF& swf, MovieFrames& ) {
	_character_id = reader.get<uint16_t>();
	int flag = reader.get<uint8_t>();
	int langcode = reader.get<uint8_t>();

	int name_length = reader.get<uint8_t>();
	_font_name.assign(reader.getData(), name_length);
	reader.skip(name_length);

	uint32_t code_table_offset;
	int glyph_count = reader.get<uint16_t>();

	_font_style = flag & (FLAG_ITALIC | FLAG_BOLD);

	if (0 == glyph_count) {
		swf.addCharacter( this, _character_id );
		return true;
	}

	if (flag & FLAG_WIDE_OFFSETS) {
		int skip = glyph_count * 4;
		reader.skip(skip); // skip offset table
		code_table_offset = reader.get<uint32_t>() - skip - 4;
	} else {
		int skip = glyph_count * 2;
		reader.skip(skip); // skip offset table
		code_table_offset = reader.get<uint16_t>() - skip - 2;
	}

	// GlyphShapeTable
	reader.skip(code_table_offset); // skip offset table

	// CodeTable
	_code_table.resize(glyph_count);
	if (flag & FLAG_WIDE_CODES) {
		for (int i = 0; i < glyph_count; ++i) {
			_code_table[i] = reader.get<uint16_t>();
		}
	} else {
		for (int i = 0; i < glyph_count; ++i) {
			_code_table[i] = reader.get<uint8_t>();
		}
	}

	if (flag & FLAG_HAS_LAYOUT) {
		float ascent = reader.get<uint16_t>() * SWF_INV_TWIPS;
		float descent = reader.get<uint16_t>() * SWF_INV_TWIPS;
		float leading = reader.get<int16_t>() * SWF_INV_TWIPS;
		// advance table
		for (int i = 0; i < glyph_count; ++i) {
			float advance = reader.get<int16_t>() * SWF_INV_TWIPS;
		}
		// bound table
		RECT bound;
		for (int i = 0; i < glyph_count; ++i) {
			reader.align();
			reader.getRectangle(bound);
		}
		// kerning
		uint16_t kerning_count = reader.get<uint16_t>();
		if (flag & FLAG_WIDE_CODES) {
			for (int i = 0; i < kerning_count; ++i) {
				uint16_t code1 = reader.get<uint16_t>();
				uint16_t code2 = reader.get<uint16_t>();
				int16_t adjust = reader.get<int16_t>();
			}
		} else {
			for (int i = 0; i < kerning_count; ++i) {
				uint16_t code1 = reader.get<uint8_t>();
				uint16_t code2 = reader.get<uint8_t>();
				int16_t adjust = reader.get<int16_t>();
			}
		}
	}

	swf.addCharacter( this, _character_id );
    return true; // keep tag
}

void DefineFontTag::print() {
   	_header.print();
	SWF_TRACE("id=%d, name:'%s', glyph:%d\n", _character_id, _font_name.c_str(), _code_table.size());
}

//-----------------------------------------------------------------------------

bool DefineFontNameTag::read( Reader& reader, SWF&, MovieFrames& data )
{
	_font_id = reader.get<uint16_t>();
	_name.assign( reader.getString() );
	_copyright.assign( reader.getString() );
	return false;//delete tag
}

void DefineFontNameTag::print() {
   	_header.print();
	SWF_TRACE("id=%d, name:'%s', copyright='%s'\n", _font_id, _name.c_str(), _copyright.c_str());
}
