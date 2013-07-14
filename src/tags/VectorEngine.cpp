/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#include "VectorEngine.h"
#include "tsSWF.h"
#include "DefineShape.h"
#include "..\libtess2\tesselator.h"
#include <algorithm>

// Curve subdivision error tolerance.
float tinyswf::SWF::curve_error_tolerance = 4.0f;

namespace triangulation {

using namespace tinyswf;

struct Collector;

typedef std::vector<Collector*> CollectorArray;
typedef std::vector<POINT>	PointDataArray;

static CollectorArray	s_collectors;
static POINT			s_last_point;

const float SWF_EPSILON = 0.1f;

inline bool overlap(const POINT& p, const POINT& q) {
	if( (fabs( p.x - q.x ) < SWF_EPSILON) && 
		(fabs( p.y - q.y ) < SWF_EPSILON) )
		return true;
	return false;
}

//-----------------------------------------------------------------------------

struct Collector {
	Collector()
		:_left_style(Path::kINVALID)
		,_right_style(Path::kINVALID)
		,_line_style(Path::kINVALID)
		,_closed(false)
	{
		_bound = kRectangleInvalidate;
	}

	Collector(int left_style, int right_style, int line_style)
		:_left_style(left_style)
		,_right_style(right_style)
		,_line_style(line_style)
		,_closed(false)
	{
		_bound = kRectangleInvalidate;
	}

	int	_left_style;
	int _right_style;
	int _line_style;
	bool _closed;
	RECT _bound;

	PointDataArray	_points;

	bool checkClosed(void) const {
		if (_closed || _right_style == Path::kINVALID || _points.size() == 0) 
			return true;
		return false;
	}

};

//-----------------------------------------------------------------------------

#if 0
inline float calcQuadraticBezier( float p0, float p1, float p2, float t ) {
	const float oneMinusT = 1.0f - t;
	float p = oneMinusT * oneMinusT * p0
		+ 2.0f * oneMinusT * t * p1
		+ t * t * p2;
	return p;	
}

inline POINT calcQuadraticBezier( const POINT& x0, const POINT& x1, const POINT& x2, float t ) {
	POINT result;
	result.x = calcQuadraticBezier(x0.x, x1.x, x2.x, t);
	result.y = calcQuadraticBezier(x0.y, x1.y, x2.y, t);
	return result;	
}
#endif

void add_line_segment(const POINT& pt)
// Add a line running from the previous anchor point to the
// given new anchor point.
{
	s_collectors.back()->_points.push_back( pt ); // collect points
	s_collectors.back()->_bound.addPoint( pt ); // update boundary
	s_last_point = pt;
}

static void	curve(const POINT& p0, const POINT& p1, const POINT& p2)
// Recursive routine to generate bezier curve within tolerance.
{
#ifdef SWF_DEBUG
	static int	recursion_count = 0;
	recursion_count++;
	if (recursion_count > 256)
		SWF_ASSERT(0);	// probably a bug!
#endif//SWF_DEBUG

	// Midpoint on line between two endpoints.
	POINT mid = (p0 + p2) * 0.5f;
	// Midpoint on the curve.
	POINT sample = (mid + p1) * 0.5f;

	mid -= sample;
	if (mid.magnitude() < SWF::curve_error_tolerance) {
		add_line_segment(p2);
	} else {
		// Error is too large; subdivide.
		curve(p0,  (p0 + p1) * 0.5f, sample);
		curve(sample, (p1 + p2) * 0.5f, p2);
	}

#ifdef SWF_DEBUG
	recursion_count--;
#endif//SWF_DEBUG
}

void begin_path(Path& path)
// This call begins recording a sequence of segments, which
// all share the same fill & line styles.  Add segments to the
// shape using add_curve_segment() or add_line_segment(), and
// call end_path() when you're done with this sequence.
//
// Pass in -1 for styles that you want to disable.  Otherwise pass in
// the integral ID of the style for filling, to the left or right.
{
	Collector *set = new Collector(path._fill0, path._fill1, path._line);
	//set->_clockwise = path.isClockWise();
	s_collectors.push_back(set);
	add_line_segment( path.getStart() );
}

void add_segment( const POINT& anchor, const POINT& ctrl )
// Add a curve segment to the shape.  The curve segment is a
// quadratic bezier, running from the previous anchor point to
// the given new anchor point, with the control point in between.
{
	if (anchor == ctrl) {
		// Early-out for degenerate straight segments.
		add_line_segment(anchor);
	} else {
		// Subdivide, and add line segments...
		curve(s_last_point, ctrl, anchor);
	}
}

void end_path(tinyswf::ShapeWithStyle& shape)
// Mark the end of a set of edges that all use the same styles.
{
	Collector& it = *(s_collectors.back());

	// copy points to line vertices
	if (it._line_style > Path::kINVALID && it._points.size() > 1) {
		SWF_TRACE("add LINE: %d vertices\n", it._points.size());
		shape.addLine(it._line_style);
		for (PointDataArray::iterator pt = it._points.begin(); pt != it._points.end(); ++pt) {
			shape.addLineVertex( *pt );
		}
	}

	// Convert left-fill paths into new right-fill paths,
	// so we only have to deal with right-fill below.
	if (Path::kINVALID == it._right_style) {
		if (Path::kINVALID == it._left_style) {
			return;
		}
		//it._clockwise = !it._clockwise;
		it._right_style = it._left_style;
		it._left_style = Path::kINVALID;
		// gather points in reverse order
		std::reverse(it._points.begin(), it._points.end());
	} else {
		if (Path::kINVALID == it._left_style) {
			return;
		}
		//if (it._right_style == it._left_style) return;

		s_collectors.push_back( new Collector(Path::kINVALID, it._left_style, it._line_style) );
		Collector &clone = *(s_collectors.back());
		//clone._clockwise = !it._clockwise;
		clone._bound = it._bound;
		it._left_style = Path::kINVALID;
		clone._points.assign(it._points.rbegin(), it._points.rend());
	}
}//end_path()

//-----------------------------------------------------------------------------

void begin_shape(void) {
	// ensure we're not already in a shape or path.
	// make sure our shape state is cleared out.
	SWF_ASSERT(s_collectors.size() == 0);
	SWF_ASSERT(SWF::curve_error_tolerance > 0);
}

void add_collector(tinyswf::ShapeWithStyle& shape, tinyswf::Path& path) {
	tinyswf::Path::EdgeArray& _edges = path.getEdges();
	begin_path(path);
	for(tinyswf::Path::EdgeArray::iterator it = _edges.begin(); it != _edges.end(); ++it) {
		add_segment(it->getAnchor(), it->getControl());
	}
	end_path(shape);
}

//-----------------------------------------------------------------------------

CollectorArray::iterator find_connecting( CollectorArray::iterator& target, CollectorArray& input )
{
	CollectorArray::iterator ret = input.end();
	Collector* pp = *target;
	if ( pp->checkClosed() ) {
		return ret;
	}

	if (overlap( pp->_points.back(), pp->_points.front() ) ) {
		// poly2tri only accept polyline with non repeating points
		pp->_points.pop_back();
		pp->_closed = true;
		return ret;
	}

	// Look for another unclosed path of the same style,
	// which could join our begin or end point.
	for (CollectorArray::iterator it = input.begin(); it != input.end(); ++it) {
		Collector* po = (*it);
		if ( pp == po )
			continue;
		if ( po->checkClosed() ) 
			continue;

		// Can we join?
		PointDataArray::iterator pStart = pp->_points.begin();
		PointDataArray::iterator oStart = po->_points.begin();
		if (overlap( *oStart, pp->_points.back() ) ) {
			// Yes, po can be appended to pp.
			pp->_points.insert( pp->_points.end(), ++oStart, po->_points.end());
			po->_right_style = Path::kINVALID;
			MergeRectangle(pp->_bound, po->_bound);
			ret = it; // remove this iterator and restart 
			return ret;
		} else if (overlap( *pStart, po->_points.back() ) ) {
			// Yes, pp can be appended to po.
			po->_points.insert( po->_points.end(), ++pStart, pp->_points.end());
			pp->_right_style = Path::kINVALID;
			MergeRectangle(po->_bound, pp->_bound);
			ret = target; // remove this iterator and restart 
			return ret;
		}
	}

	return ret;
}

void get_contours_and_holes( CollectorArray& input ) {
	// join collectors together into closed paths.
	while (1) {
		bool found_connection = false;
		for (CollectorArray::iterator it = input.begin(); it != input.end(); ++it) {
			CollectorArray::iterator result = find_connecting( it, input );
			if (input.end() != result) {
				input.erase( result );
				found_connection = true;
				break;
			}
		}
		if (false == found_connection)
			break;
	}
#if 0
	// find holes and interior points
	while (1) {
		bool found = false;
		for (CollectorArray::iterator i = input.begin(); i != input.end(); ++i) {
			for (CollectorArray::iterator j = input.begin(); j != input.end(); ++j) {
				Collector* cA = *i;
				Collector* cB = *j;
				if ( cA == cB )	// don't check against self
					continue;

				if (!cA->_interior && cB->_interior) {
					cA->_steiner_points.push_back(cB);
					input.erase(j);
					found = true;
					break;
				}

				// inner has to be counter direction to outer
				if ( cA->_clockwise == cB->_clockwise ) 
					continue;
				
				if ( cB->_bound.isInside(cA->_bound) ) {
					cA->_holes.push_back(cB);
					input.erase(j);
					found = true;
					break;
				}
			}
			if ( found )
				break;
		}
		if (false == found)
			break;
	}
#endif
}

void* stdAlloc(void* userData, unsigned int size) {
	int* allocated = ( int*)userData;
	*allocated += (int)size;
	return malloc(size);
}
void stdFree(void* userData, void* ptr) {
	free(ptr);
}

struct MemoryPool {
	unsigned char* buffer;
	unsigned int capacity;
	unsigned int size;
};
typedef std::vector<MemoryPool> MemoryPoolArray;

static MemoryPoolArray svMemoryPools;

void* poolAlloc( void* userData, unsigned int size ) {
	MemoryPool* pool = &svMemoryPools.back();
	if (pool->size + size > pool->capacity) {
		MemoryPool newPool;
		const int capacity = pool->capacity;
		newPool.buffer = new unsigned char[capacity];
		newPool.capacity = capacity;
		newPool.size = 0;
		svMemoryPools.push_back(newPool);
		pool = &svMemoryPools.back();
		SWF_TRACE("*** reallocate memory pool: %.1f kB\n", capacity/1024.f);
	}
	if (userData) {
		int* allocated = (int*)userData;
		*allocated += size;
	}
	unsigned char* ptr = pool->buffer + pool->size;
	pool->size += size;
	return ptr;
}

void poolFree( void* userData, void* ptr ) {
	// empty
}

void reset_memory_pool(void) {
	MemoryPoolArray::iterator start = svMemoryPools.begin();
	if (svMemoryPools.end() == start)
		return;

	start->size = 0;
	MemoryPoolArray::reverse_iterator rit = svMemoryPools.rbegin();
	while (rit->buffer != start->buffer) {
		delete [] rit->buffer;
		svMemoryPools.pop_back();
		rit = svMemoryPools.rbegin();
	}
}

void create_memory_pool(int size) {
	SWF_ASSERT(0 == svMemoryPools.size());
	if (0 == size)
		return;
	svMemoryPools.resize(1);
	MemoryPool& pool = svMemoryPools.back();
	pool.buffer = new unsigned char[size];
	pool.capacity = size;
	pool.size = 0;
}

void destroy_memory_pool(void) {
	for (MemoryPoolArray::iterator it = svMemoryPools.begin(); it != svMemoryPools.end(); ++it) {
		delete it->buffer;
	}
	svMemoryPools.clear();
}

bool create_mesh( tinyswf::ShapeWithStyle& shape, const CollectorArray& contours, int style ) {
	reset_memory_pool();

	TESSalloc ma;
	TESStesselator* tess = 0;
	int allocated = 0;
	memset(&ma, 0, sizeof(ma));
	ma.userData = (void*)&allocated;
	ma.extraVertices = 256; // realloc not provided, allow 256 extra vertices.
	if (0 < svMemoryPools.size()) {
		ma.memalloc = poolAlloc;
		ma.memfree = poolFree;
	} else {
		ma.memalloc = stdAlloc;
		ma.memfree = stdFree;
	}

	// allocate a triangulator
	tess = tessNewTess(&ma);
	if (! tess)
		return false;

	// add a new mesh
	shape.addMesh(style);

	const int kMaximumVerticesPerPolygons = 8;
	const int kNumberCoordinatePerVertex = 2;
	for (CollectorArray::const_iterator oit = contours.begin(); oit != contours.end(); ++oit) {
		Collector* po = (*oit);
		if (NULL == po)
			continue;
		tessAddContour(tess, kNumberCoordinatePerVertex, &(po->_points.front()), sizeof(POINT), po->_points.size());
	}

	// process
	if (!tessTesselate(tess, TESS_WINDING_POSITIVE, TESS_POLYGONS, kMaximumVerticesPerPolygons, kNumberCoordinatePerVertex, NULL))
		return false;
	SWF_TRACE("Memory used: %.1f kB\n", allocated/1024.f);

	if (! tess)
		return false;

	const float* verts = tessGetVertices(tess);
	const int* vinds = tessGetVertexIndices(tess);
	const int nverts = tessGetVertexCount(tess);
	const int* elems = tessGetElements(tess);
	const int nelems = tessGetElementCount(tess);
	
	// TODO: convert to triangle-strip
	int count = 0;
	SWF_TRACE("add MESH [fill:%d]: %d elements, ", style, nelems);
	for (int i = 0; i < nelems; ++i) {
		const int* p = &elems[i * kMaximumVerticesPerPolygons];

		POINT p0 = {verts[p[0]*2], verts[p[0]*2+1]};
		POINT p1 = {verts[p[1]*2], verts[p[1]*2+1]};
		for (int j = 2; j < kMaximumVerticesPerPolygons && p[j] != TESS_UNDEF; ++j) {
			POINT p2 = {verts[p[j]*2], verts[p[j]*2+1]};
			shape.addMeshVertex( p0 );
			shape.addMeshVertex( p1 );
			shape.addMeshVertex( p2 );
			p1 = p2;
			++count;
		}
	}
	SWF_TRACE("%d triangles\n", count);

	// free the triangulator
	tessDeleteTess(tess);

	return true;
}

void end_shape(tinyswf::ShapeWithStyle& shape) {
	int size = shape.getFillStyles().size();
	for (int i = 0; i < size; ++i) {
		CollectorArray contours;
		contours.reserve(4);

		// collect same style contours
		for (CollectorArray::iterator it = s_collectors.begin(), end = s_collectors.end(); it != end; ++it) {
			Collector* current = *it;
			if ( current->_right_style == i ) {
				contours.push_back( current );
			}
		}
		if (0 == contours.size())
			continue;

		// connect contours and find holes
		get_contours_and_holes(contours);
		
		// Triangulate and emit.
		create_mesh( shape, contours, i );
	}

	// clean memory
	for (CollectorArray::iterator it = s_collectors.begin(); it != s_collectors.end(); ++it) {
		delete (*it);
	}
	s_collectors.clear();
}

//-----------------------------------------------------------------------------

}// end namespace triangulation
