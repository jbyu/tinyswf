/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "DefineShape.h"
#include "tsSWF.h"

#define PAD_SPREAD_MODE         0x0
#define REFLECT_SPREAD_MODE		0x1
#define REPEAT_SPREAD_MODE		0x2

using namespace tinyswf;

//-----------------------------------------------------------------------------
	
bool Gradient::read( Reader* reader, bool support_32bit_color  ) {
	_spread_mode = reader->getbits( 2 );
	_interpolation_mode = reader->getbits( 2 );
	_num_gradients = reader->getbits( 4 );
		
	for ( uint32_t i = 0; i < _num_gradients; i++ ) {
        Gradient::Record record;
		record._ratio = reader->get<uint8_t>();
		record._color.r = reader->get<uint8_t>(); 
		record._color.g = reader->get<uint8_t>(); 
		record._color.b = reader->get<uint8_t>();
		if( support_32bit_color )
			record._color.a = reader->get<uint8_t>();
		else
			record._color.a = 255;
		_gradient_records.push_back( record );
	}
        
	return true;
}
#if 0
void Gradient::configPaint( VGPaint paint, uint8_t type, MATRIX& m, bool support_32bit_color ) {
    float c[6];
    c[0] = m.sx / float(1<<16);
    c[1] = m.sy / float(1<<16);
    c[2] = m.tx * 0.05f;
    c[3] = m.ty * 0.05f;
    c[4] = m.r0 / float(1<<16);
    c[5] = m.r1 / float(1<<16);

    vgSetParameterfv(paint, VG_PAINT_2x3_GRADIENT, 6, &c[0]);
        
    if (type == LINEAR_GRADIENT_FILL) {
        vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_2x3_GRADIENT);
    } else if (type == RADIAL_GRADIENT_FILL) {
        vgSetParameteri(paint, VG_PAINT_TYPE, VG_PAINT_TYPE_RADIAL_2x3_GRADIENT);
    } else if (type == FOCAL_GRADIENT_FILL) {
        assert(0); // not supported, gradient would need to read the focal point
    }
        
    if (_spread_mode == PAD_SPREAD_MODE) {
        vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_PAD);
    } else if (_spread_mode == REFLECT_SPREAD_MODE) {
        vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_REFLECT);
    } else if (_spread_mode == REPEAT_SPREAD_MODE) {
        vgSetParameteri(paint, VG_PAINT_COLOR_RAMP_SPREAD_MODE, VG_COLOR_RAMP_SPREAD_REPEAT);
    }
        
    //VG_PAINT_COLOR_RAMP_STOPS
    //  One entry per color
    int rampStopsSize = _gradient_records.size() * 5;
    float* rampStops = new float[rampStopsSize];
        
    for (uint32_t i = 0; i < _gradient_records.size(); ++i) {
        int n = i * 5;
        rampStops[n]      = _gradient_records[i]._ratio / 255.0f;
        rampStops[n+1]    = _gradient_records[i]._color.r / 255.0f;
        rampStops[n+2]    = _gradient_records[i]._color.g / 255.0f;
        rampStops[n+3]    = _gradient_records[i]._color.b / 255.0f;
        if (support_32bit_color) {
            rampStops[n+4]    = _gradient_records[i]._color.a / 255.0f;
        } else {
            rampStops[n+4]    = 1.0f;
        }
    }
    vgSetParameterfv(paint, VG_PAINT_COLOR_RAMP_STOPS, rampStopsSize, &rampStops[0]);
        
    delete [] rampStops;
    rampStops = NULL;
}
#endif

//-----------------------------------------------------------------------------

bool LineStyle::read( Reader* reader, bool lineStyle2, bool support_32bit_color ) {
	_width = reader->get<uint16_t>()*SWF_INV_TWIPS;
	
	if (! lineStyle2) {
		_color.r = reader->get<uint8_t>()*SWF_INV_COLOR;
		_color.g = reader->get<uint8_t>()*SWF_INV_COLOR;
		_color.b = reader->get<uint8_t>()*SWF_INV_COLOR;
		if( support_32bit_color )
			_color.a = reader->get<uint8_t>()*SWF_INV_COLOR;
		else
			_color.a = 1.0f;
		return true;
	}

	// TODO: support LINESTYLE2
	uint32_t startCapStyle	= reader->getbits(2);
	uint32_t joinStyle		= reader->getbits(2);
	bool hasFill		= reader->getbit() > 0;
	bool noHScale		= reader->getbit() > 0;
	bool noVScale		= reader->getbit() > 0;
	bool pixelHint		= reader->getbit() > 0;
	uint32_t reserved	= reader->getbits(5);
	bool noClose		= reader->getbit() > 0;
	uint32_t endCapStyle		= reader->getbits(2);

	if (2 == joinStyle) {
		uint16_t flag = reader->get<uint16_t>();
	}

	if (hasFill) {
		FillStyle fill;
		fill.read(reader, support_32bit_color);
	} else {
		_color.r = reader->get<uint8_t>()*SWF_INV_COLOR;
		_color.g = reader->get<uint8_t>()*SWF_INV_COLOR;
		_color.b = reader->get<uint8_t>()*SWF_INV_COLOR;
		_color.a = reader->get<uint8_t>()*SWF_INV_COLOR;
	}

	return true;
}

//-----------------------------------------------------------------------------

bool FillStyle::read( Reader* reader, bool support_32bit_color ) {
	_type = (Type)reader->get<uint8_t>();
    switch(_type)
    {
    case TYPE_SOLID:
		_color.r = reader->get<uint8_t>()*SWF_INV_COLOR;
		_color.g = reader->get<uint8_t>()*SWF_INV_COLOR;
		_color.b = reader->get<uint8_t>()*SWF_INV_COLOR;
		if( support_32bit_color )
			_color.a = reader->get<uint8_t>()*SWF_INV_COLOR;
		else
			_color.a = 1.0f;
        break;

    case TYPE_LINEAR_GRADIENT:
    case TYPE_RADIAL_GRADIENT:
    case TYPE_FOCAL_GRADIENT:
		{
		MATRIX		_gradient_matrix;
		Gradient	_gradient;
		reader->getMatrix( _gradient_matrix );
		reader->align();
		_gradient.read( reader, support_32bit_color );
		if (TYPE_FOCAL_GRADIENT==_type)
			float focalPoint = reader->getFIXED8();
		}
        break;

    case TYPE_REPEATING_BITMAP:
    case TYPE_CLIPPED_BITMAP:
    case TYPE_NON_SMOOTHED_REPEATING_BITMAP:
    case TYPE_NON_SMOOTHED_CLIPPED_BITMAP:
		{
		MATRIX mtx;
		_bitmap_id = reader->get<uint16_t>();
		reader->getMatrix( mtx );
		reader->align();
		memcpy(_bitmap_matrix.f, mtx.m_, sizeof(MATRIX));
		}
        break;

    default:
        SWF_ASSERT(0);
	}
	return true;
}	
	
//-----------------------------------------------------------------------------

bool ShapeWithStyle::read( Reader& reader, SWF& swf, DefineShapeTag* define_shape_tag )
{
	bool useRGBA = false;
	bool userLineStyle2 = false;
	const TagHeader& shape_header = define_shape_tag->header();
	if (shape_header.code() == TAG_DEFINE_SHAPE4) {
		userLineStyle2 = true;
		useRGBA = true;
	} else if (shape_header.code() == TAG_DEFINE_SHAPE3) {
		useRGBA = true;
	}
    // all shape definitions except DEFINESHAPE & DEFINESHAPE2 support 32 bit color
	bool ret1 = readStyles(&reader, userLineStyle2, useRGBA);
	bool ret2 = readShapeRecords(&reader, userLineStyle2, useRGBA);

	// fetch asset handle and calculate texture matrix
	for (ShapeArray::iterator i = _shapes.begin(); i != _shapes.end(); ++i) {
	for (MeshArray::iterator it = i->_meshes.begin(); it != i->_meshes.end(); ++it) {
		FillStyle *style = it->_style;
		if (0 == (style->type() & FillStyle::TYPE_REPEATING_BITMAP) )
			continue;

		uint16_t bitmap = style->getBitmap();
		if (0 == bitmap) 
			continue;

		it->_asset = swf.getAsset(bitmap);
		if (it->_asset.import) {
			// compute the boundary of the mesh for import asset
			RECT bound = {FLT_MAX,FLT_MAX,-FLT_MAX,-FLT_MAX};
			for(VertexArray::const_iterator v = it->_vertices.begin(); v != it->_vertices.end(); ++v) {
				bound.addPoint(*v);
			}
			it->_bound = bound;
			continue;
		}

		// compute transform matrix for texture mapping
		MATRIX inv, mtx;
		memcpy(mtx.m_, style->_bitmap_matrix.f, sizeof(MATRIX));
		inv.setInverse(mtx);
		mtx.sx = it->_asset.param[0];// 20.f/w
		mtx.sy = it->_asset.param[1];// 20.f/h
		mtx.tx = it->_asset.param[2];// x/w
		mtx.ty = it->_asset.param[3];// y/h
		mtx.r0 = mtx.r1 = 0.f;
		mtx = MATRIX::concatenate(inv, mtx);
		MATRIX3fSet(style->_bitmap_matrix, mtx);
	}
	}

	return ret1 | ret2;
}

bool ShapeWithStyle::readStyles( Reader* reader, bool lineStyle2, bool support_32bit_color )
{
    // get the fill styles
	uint16_t num_fill_styles = reader->get<uint8_t>();
	if ( num_fill_styles == 0xff ) {
		num_fill_styles = reader->get<uint16_t>();
	}
	for ( int i = 0; i < num_fill_styles; i++ ) {
		FillStyle fill;
		fill.read( reader, support_32bit_color );
        fill.print();
		_fill_styles.push_back( fill );
	}

	// get the line styles
	uint16_t num_line_styles = reader->get<uint8_t>();
	if ( num_line_styles == 0xff ) {
		num_fill_styles = reader->get<uint16_t>();
	}
	for ( int i = 0; i < num_line_styles; i++ ) {
		LineStyle line;
		line.read( reader, lineStyle2, support_32bit_color );
		_line_styles.push_back( line );
	}

	return true;
}

const unsigned int SF_MOVETO	= 0x01;
const unsigned int SF_FILL0     = 0x02;
const unsigned int SF_FILL1     = 0x04;
const unsigned int SF_LINE      = 0x08;
const unsigned int SF_NEWSTYLE  = 0x10;

bool ShapeWithStyle::readShapeRecords( Reader* reader, bool lineStyle2, bool support_32bit_color )
{
	bool end = false;
	size_t fill_base = 0;
	size_t line_base = 0;
	uint32_t num_fill_bits = reader->getbits( 4 );
	uint32_t num_line_bits = reader->getbits( 4 );
	POINT start = { 0, 0 };
	Path current;

	while ( !end ) {
		// go through the shape records...
		uint32_t type_flag = reader->getbits( 1 );
		if ( 0 == type_flag ) {	
			// change or end record
			uint32_t flags = reader->getbits( 5 );
			// add path
			if (! current.isEmpty()) {
				_paths.push_back(current);
				current.getEdges().clear();
				current._new_shape = false;
				SWF_TRACE("new path\n");
			}
			if ( flags ) { // style change record
				if( flags & SF_MOVETO ) {
					uint32_t nbits = reader->getbits( 5 );
					start.x = reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
					start.y = reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
					current.getStart() = start;
					SWF_TRACE("move:  %.2f, %.2f\n", start.x, start.y);
				} 
				if( flags & SF_FILL0 ) { 
					int idx = reader->getbits( num_fill_bits ) - 1;
					current._fill0 = (0>idx) ? idx : idx + fill_base;
					current.getStart() = start;
					SWF_TRACE("fill0: %d\n", current._fill0);
				} 
				if( flags & SF_FILL1 ) {
					int idx = reader->getbits( num_fill_bits ) - 1;
					current._fill1 = (0>idx) ? idx : idx + fill_base;
					//current._fill1 = reader->getbits( num_fill_bits ) - 1 + fill_base;
					current.getStart() = start;
					SWF_TRACE("fill1: %d\n", current._fill1);
				} 
				if( flags & SF_LINE) { 
					current._line = reader->getbits( num_line_bits ) - 1 + line_base;
					current.getStart() = start;
					SWF_TRACE("line:  %d\n", current._line);
				} 
				if( flags & SF_NEWSTYLE ) { 
					current._fill0 = -1;
					current._fill1 = -1;
					current._line = -1;
					current.getStart() = start;
					current._new_shape = true;
					// get the fill/line styles
					fill_base = _fill_styles.size();
					line_base = _line_styles.size();
					readStyles(reader,  lineStyle2, support_32bit_color);
					num_fill_bits = reader->getbits( 4 );
					num_line_bits = reader->getbits( 4 );
					SWF_TRACE("new style\n");
				}
			} else { // end record
				end = true;
			} // if( flags )
		} else { // edge type record
			uint32_t isLine = reader->getbits( 1 );
			if ( isLine ) {
				const uint32_t nbits = reader->getbits( 4 ) + 2;
				const uint32_t general_line_flag = reader->getbits( 1 );
				POINT anchor = start;
				if ( general_line_flag ) {	
					anchor.x += reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
					anchor.y += reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
				} else {	// either a horizontal or vertical line
					uint32_t vert_line_flag = reader->getbits( 1 ); 
					if ( vert_line_flag ) {
						anchor.y += reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
					} else {	// horizontal line
						anchor.x += reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
					}
				}
				current.addEdge( Edge( anchor ) );
				start = anchor;
			} else { // curve
				uint32_t nbits = reader->getbits( 4 ) + 2;
				POINT anchor, ctrl;
				ctrl.x = start.x + reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
				ctrl.y = start.y + reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
				anchor.x = ctrl.x + reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
				anchor.y = ctrl.y + reader->getsignedbits( nbits ) *SWF_INV_TWIPS;
				current.addEdge( Edge(anchor, ctrl) );
				start = anchor;
			}
		}
	}

	triangluate();

	return true;
}

bool Path::isClockWise(void) const {
	float sum = 0;
	EdgeArray::const_iterator i = _edges.begin();
	const POINT* p0 = &_start;
	const POINT* p1 = &(i->getAnchor());
	++i;
	for ( ; i != _edges.end(); ++i ) {
		const POINT* p2 = &(i->getAnchor());
		POINT v1 = *p2 - *p1;
		POINT v2 = *p1 - *p0;
		sum += v1.x * v2.y - v2.x * v1.y;
		p0 = p1;
		p1 = p2;
	}
	return 0 >= sum;
}

bool ShapeWithStyle::triangluate(void) {
	Shape s;
	_shapes.push_back( s );
	triangulation::begin_shape();
	for (PathArray::iterator it = _paths.begin(); it != _paths.end(); ++it) {
		if (it->_new_shape) {
			triangulation::end_shape(*this);
			_shapes.push_back( s );
			triangulation::begin_shape();
		}
		triangulation::add_collector(*this, *it);
	}
	triangulation::end_shape(*this);
	_paths.clear();
	return true;
}

void ShapeWithStyle::draw() {
	for (ShapeArray::const_iterator i = _shapes.begin(); i != _shapes.end(); ++i) {
		const Shape& shape = *i;
		for (MeshArray::const_iterator j = shape._meshes.begin(); j != shape._meshes.end(); ++j) {
			if (j->_asset.import) {
				Renderer::getInstance()->drawImportAsset(j->_bound, SWF::getCurrentCXForm(), j->_asset);
			} else {
				Renderer::getInstance()->drawTriangles( j->_vertices, SWF::getCurrentCXForm(), *(j->_style), j->_asset);
			}
		}
		for (LineArray::const_iterator j = shape._lines.begin(); j != shape._lines.end(); ++j) {
			Renderer::getInstance()->drawLineStrip( j->_vertices, SWF::getCurrentCXForm(), *(j->_style));
		}
	}
}

void barycentric(const POINT& p, const POINT& a, const POINT& b, const POINT& c, float &u, float &v)
{
    POINT v0 = b - a, v1 = c - a, v2 = p - a;
    float d00 = v0.dot(v0);
    float d01 = v0.dot(v1);
	float d02 = v0.dot(v2);
    float d11 = v1.dot(v1);
    float d12 = v1.dot(v2);
	float invDenom = 1.0f / (d00 * d11 - d01 * d01);
    u = (d11 * d02 - d01 * d12) * invDenom;
    v = (d00 * d12 - d01 * d02) * invDenom;
    //w = 1.0f - v - w;
}

bool ShapeWithStyle::Mesh::isInsideMesh(const POINT& pt) const {
	float u, v;
	for (VertexArray::const_iterator it = _vertices.begin(); it != _vertices.end(); it+=3) {
		barycentric(pt, *it, *(it+1), *(it+2), u, v);
		if (0.f <= u && 0.f <= v &&	1.f >= u+v)
			return true;
	}
	return false;
}

bool ShapeWithStyle::isInside(float x, float y) const {
	POINT pt = {x,y};
	for (ShapeArray::const_iterator i = _shapes.begin(); i != _shapes.end(); ++i) {
	for (MeshArray::const_iterator it = i->_meshes.begin(); it != i->_meshes.end(); ++it) {
		if (it->isInsideMesh(pt))
			return true;
	}
	}
	return false;
}

//-----------------------------------------------------------------------------

bool DefineShapeTag::read( Reader& reader, SWF& swf, MovieFrames&  )
{
    int32_t start = reader.getCurrentPos();
	_shape_id = reader.get<uint16_t>();
	reader.getRectangle( _bound );
	reader.align();

	const TagHeader& shape_header = this->header();
	if (TAG_DEFINE_SHAPE4 == shape_header.code()) {
		RECT edgeBound;
		reader.getRectangle( edgeBound );
		reader.align();
		uint8_t flag = reader.get<uint8_t>();
	}
	_shape_with_style.read( reader, swf, this );

    //skip rest data
    int32_t end = reader.getCurrentPos();
    reader.skip( length() - end + start );

	swf.addCharacter( this, _shape_id );

	return true;
}
	
void DefineShapeTag::print() {
	_header.print();
	//SWF_TRACE("id=%d, w=%f, h=%f\n", _shape_id, _bound.xmax-_bound.xmin, _bound.ymax-_bound.ymin);
	SWF_TRACE("id=%d, RECT:{%.2f,%.2f,%.2f,%.2f}\n", _shape_id,
		_bound.xmin, _bound.ymin,
		_bound.xmax, _bound.ymax);
}
	
void DefineShapeTag::draw(void) {
	_shape_with_style.draw();
    //Renderer::getInstance()->drawQuad(_bound, SWF::getCurrentCXForm(), _asset.handle);
}
	
ITag* DefineShapeTag::create( TagHeader& header ) {
	return new DefineShapeTag( header );
}

ICharacter* DefineShapeTag::getTopMost(float localX, float localY, bool polygonTest) {
	if ( IsWithinRectangle(getRectangle(), localX, localY) ) {
		if (! polygonTest)
			return this;
		if (_shape_with_style.isInside(localX, localY))
			return this;
	}
	return NULL;
}
	
