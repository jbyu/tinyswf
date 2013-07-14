/******************************************************************************
Copyright (c) 2013 jbyu. All rights reserved.
******************************************************************************/

#ifndef __VECTOR_ENGINE__
#define __VECTOR_ENGINE__

//-----------------------------------------------------------------------------

namespace tinyswf {
	class ShapeWithStyle;
	class Path;
}

//-----------------------------------------------------------------------------

namespace triangulation {
	void create_memory_pool(int size);
	void destroy_memory_pool(void);
	void begin_shape(void);
	void add_collector(tinyswf::ShapeWithStyle& shape, tinyswf::Path&);
	void end_shape(tinyswf::ShapeWithStyle&);
}

//-----------------------------------------------------------------------------	

#endif//__VECTOR_ENGINE__