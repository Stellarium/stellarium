#ifndef CONVEXCOLLECTION_H_
#define CONVEXCOLLECTION_H_

#include <functional>
#include <vector>
#include <boost/iterator/iterator_facade.hpp>
#include "SphereGeometry.h"


//! Template class used to store elements which can be assimilated to a Convex.
//! GetConvexFunc is a Functor std::unary_function<const T&, ConvexPolygon> returning 
//! the convex for an element of class T


//! Implementation based on a simple std::vector
template <class  T, class GetConvexFunc>
class ConvexPolygonVector
{
public:
	ConvexPolygonVector(GetConvexFunc getConvexFunc) : getConvex(getConvexFunc) {;}
	
	class SearchIterator : public boost::iterator_facade < SearchIterator , T, boost::forward_traversal_tag >
	{
	 public:
	 	SearchIterator();
	 private:
	 	friend class ConvexPolygonVector;
	    friend class boost::iterator_core_access;
		
		SearchIterator(const ConvexPolygonVector<T, GetConvexFunc>* collection, const ConvexPolygon& zone, typename std::vector<T>::iterator iter) : coll(collection), searchZone(zone), vecIter(iter) {;}
		
	    void increment()
	    {
	    	for(;++vecIter!=coll->elements.end();)
	    	{
	    		if (searchZone.intersect(coll->getConvex(*vecIter))==true)
	    			return;
	    	}
	    }
	
	    bool equal(SearchIterator const& other) const
	    {
	        return (vecIter==other.vecIter && searchZone==other.searchZone); 
	    }
	
	    T& dereference() const { return *vecIter; }

	    // The matching collection
	    const ConvexPolygonVector<T, GetConvexFunc>* coll;
	    // The ConvexPolygon used for the search
	    const ConvexPolygon searchZone;
	    // Specific to this implementation
	    typename std::vector<T>::iterator vecIter;
	};

	//! Get a range of iterators pointing on elements which intersect the given zone
	//! @param a convex polygon defining the region to intersect	
	std::pair<SearchIterator, SearchIterator> getIntersectingElems(const ConvexPolygon& searchZone)
	{
		SearchIterator end(this, searchZone, elements.end());
		if (elements.empty())
			return std::pair<SearchIterator, SearchIterator>(end, end);
		SearchIterator begin(this, searchZone, elements.begin());
		if (searchZone.intersect(getConvex(*begin))==false)
			++begin;
		return std::pair<SearchIterator, SearchIterator>(begin, end);
	}
	
	//! Insert a new element into the collection 
	void insert(const T& e) {elements.push_back(e);}
	
	typedef typename std::vector<T>::iterator Iterator;
	Iterator begin() {return elements.begin();}
	Iterator end() {return elements.end();}
private:
	
	// We use a simple vector in this implementation
	std::vector<T> elements;
	
	//! The functor used to get the convex matching one element
	GetConvexFunc getConvex;
};

#endif /*CONVEXCOLLECTION_H_*/
