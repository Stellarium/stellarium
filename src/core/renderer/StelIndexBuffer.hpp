#ifndef _STELINDEXBUFFER_HPP_
#define _STELINDEXBUFFER_HPP_

//! Generic index buffer interface usable with all Renderer backends.
//!
//! Used to specify order in which vertices from a vertex buffer are drawn
//! (allowing to e.g. draw the same vertex in multiple triangles without 
//! duplicating it, saving memory and RAM-VRAM bandwidth).
//!
//! stub - TODO implement
class StelIndexBuffer
{
	public:
		StelIndexBuffer()
		{
			Q_ASSERT_X(false, Q_FUNC_INFO, "StelIndexBuffer stub - TODO implement");
		}
}

#endif //_STELINDEXBUFFER_HPP_

