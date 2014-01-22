/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __DEFINE_SHAPE_H__
#define __DEFINE_SHAPE_H__

#include "tsTag.h"
#include "VectorEngine.h"

namespace tinyswf {

class DefineShapeTag;

//-----------------------------------------------------------------------------

class Gradient {
public:
	bool read( Reader* reader, bool support_32bit_color );
    //void configPaint( VGPaint paint, uint8_t type, MATRIX& m, bool support_32bit_color);
		
private:
	uint32_t		_spread_mode;			// 0 = Pad mode 1 = Reflect mode 2 = Repeat mode
	uint32_t		_interpolation_mode;	// 0 = Normal RGB mode interpolation 1 = Linear RGB mode interpolation
	uint32_t		_num_gradients;
		
	struct Record {
		uint8_t	_ratio;
		RGBA	_color;
	};
		
	typedef std::vector< Gradient::Record > GradientRecordArray;
	typedef GradientRecordArray::iterator GradientRecordArrayIter;
		
	GradientRecordArray		_gradient_records;
};

//-----------------------------------------------------------------------------

class Edge {
public:
	Edge( const POINT& anchor ) 
		:_anchor(anchor)
		,_control(anchor)
	{}

	Edge( const POINT& anchor, const POINT& ctrl ) 
		:_anchor(anchor)
		,_control(ctrl)
	{}

	const POINT& getAnchor() const {
		return _anchor;
	}

	const POINT& getControl() const {
		return _control;
	}

	void print() const {
		if (_anchor == _control) {
			SWF_TRACE("[line]  anchor:%.2f, %.2f\n", _anchor.x, _anchor.y);
		} else {
			SWF_TRACE("[curve] anchor:%.2f, %.2f;\tctrl:%.2f, %.2f\n", _anchor.x, _anchor.y, _control.x, _control.y);
		}
	}

protected:		
	POINT	_anchor;
	POINT	_control;
};
	
//-----------------------------------------------------------------------------

class Path {
public:
	typedef std::vector<Edge>	EdgeArray;

	const static int kINVALID = -1;

	Path()
		:_fill0( kINVALID )
		,_fill1( kINVALID )
		,_line( kINVALID )
		,_new_shape(false)
	{
		_start.x = 0.f;
		_start.y = 0.f;
	}

	void addEdge( const Edge& e ) {
		SWF_TRACE("add ");
		e.print();
		_edges.push_back( e );
	}
	
	bool isEmpty() const {
		return _edges.size() == 0;
	}

	POINT& getStart() { return _start; }

	EdgeArray& getEdges() { return _edges; }

	bool isClockWise(void) const;

	// style indices
	int _fill0;
	int _fill1;
	int _line;
	bool _new_shape;

private:
	POINT		_start;
	EdgeArray	_edges;
};

//-----------------------------------------------------------------------------

class ShapeWithStyle {
public:
	typedef std::vector<FillStyle>	FillStyleArray;
	typedef std::vector<LineStyle>	LineStyleArray;

	bool read( Reader& reader, SWF&, DefineShapeTag* define_shape_tag );
	
	void draw(SWF *owner);

	void addMesh(size_t fill_idx) {
		SWF_ASSERT(0 <= fill_idx && _fill_styles.size() > fill_idx);
		Mesh mesh = { &_fill_styles[fill_idx], kNULL_ASSET };
		_shapes.back()._meshes.push_back(mesh);
	}

	void addMeshVertex( const POINT& pt) {
		_shapes.back()._meshes.back()._vertices.push_back(pt);
	}

	void addLine(size_t line_idx) {
		SWF_ASSERT(0 <= line_idx && _line_styles.size() > line_idx);
		Line l = { &_line_styles[line_idx] };
		_shapes.back()._lines.push_back(l);
	}

	void addLineVertex( const POINT& pt) {
		_shapes.back()._lines.back()._vertices.push_back(pt);
	}

	bool isInside(float x, float y) const;

	const FillStyleArray& getFillStyles(void) const { return _fill_styles; }

private:
	struct Line {
		LineStyle	*_style;
		VertexArray	_vertices;
	};
	struct Mesh {
		FillStyle	*_style;
		Asset		_asset;
		RECT		_bound;
		VertexArray	_vertices;

		bool isInsideMesh(const POINT& pt) const;
	};
	typedef std::vector<Mesh>		MeshArray;
	typedef std::vector<Line>		LineArray;

	struct Shape {
		MeshArray		_meshes;
		LineArray		_lines;
	};
	typedef std::vector<Shape>		ShapeArray;
	typedef std::vector<Path>		PathArray;

	FillStyleArray	_fill_styles;
	LineStyleArray	_line_styles;		
	PathArray		_paths;
	ShapeArray		_shapes;

	bool readStyles(Reader* reader, bool lineStyle2, bool support_32bit_color);

	bool readShapeRecords(Reader* reader, bool lineStyle2, bool support_32bit_color);

	bool triangluate();
};
	
//-----------------------------------------------------------------------------

class DefineShapeTag : public ITag, public ICharacter {
public:
	DefineShapeTag( TagHeader& h )
		:ITag( h )
        ,_shape_id(0)
	{
    }
		
	virtual ~DefineShapeTag()
    {
	}
	
	virtual bool read( Reader& reader, SWF&, MovieFrames& data );
	virtual void print();
		
	// override ICharacter function
    virtual const RECT& getRectangle(void) const { return _bound; }
	virtual void draw(SWF *owner);
	virtual void update(void) {}
	virtual ICharacter* getTopMost(float localX, float localY, bool polygonTest);
	virtual void onEvent(Event::Code) {}
		
	virtual TYPE type() const { return TYPE_SHAPE; }

	static ITag* create( TagHeader& header );
	
protected:
	uint16_t		_shape_id;
	RECT			_bound;
	ShapeWithStyle	_shape_with_style;
};
	
//-----------------------------------------------------------------------------

} //namespace

#endif // __DEFINE_SHAPE_H__
