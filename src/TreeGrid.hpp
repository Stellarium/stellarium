/*
 * Stellarium
 * Copyright (C) 2007 Guillaume Chereau
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
 
#ifndef _TREEGRID_HPP_
#define _TREEGRID_HPP_

#include <list>
#include <vector>

#include "spgrid.hpp"
#include "SphereGeometry.hpp"
#include "Navigator.hpp"

template<class Obj>
class TreeGrid;

//! This a convenient class that contains the methods of StelGridPolicy<T>
//! that don't depend of the template type, so we can put them in the .cpp file
class TreeGridPolicyBase
{
public:
    //! This method is called when we need to split a triangle
    //! into smaller sub triangles
    std::list<const ConvexPolygon*> split(const Polygon* p);
};

//! This class define the way the StelGrid<Obj> should behave
template<class Obj>
class TreeGridPolicy : public TreeGridPolicyBase
{
public:
    typedef SPGrid<Obj*, const ConvexPolygon*, TreeGridPolicy<Obj> > Grid_t;

    //! Constructor
    TreeGridPolicy(TreeGrid<Obj>* grid) : _grid(grid)
    {}

    // The contains method for normal shapes
    template<class T1, class T2>
    bool contains(const T1* s1 ,const T2* s2) const
    {
        return ::contains(*s1, *s2);
    }
    
    // Now for the objects
    template<class T>
    bool contains(const T* s, const Obj* o) const
    {
        // TODO: use the FOV information,
        // Because it is not correct to consider the object as a point
        return ::contains(*s, getObsJ2000Pos(o));
    }
    
    // The intersect method for normal shapes
    template<class T1, class T2>
    bool intersect(const T1* s1, const T2* s2) const
    {
        return ::intersect(*s1, *s2);
    }

    // Now for StelObjects
    template<class T>
    bool intersect(const T* s, const Obj* o) const
    {
        return ::contains(*s, getObsJ2000Pos(o));
    }
    
    bool split_cond(const typename Grid_t::node_t& node) const
    {
        return node.children().empty() && 
               node.objects().size() > 1000;
    }
    
private:
    Vec3d getObsJ2000Pos(const Obj* o) const
    {
        return o->getObsJ2000Pos(_grid->_nav);
    }
    
private:
    // The TreeGrid object :
    TreeGrid<Obj>* _grid;

};

class TreeGridBase
{
public:
    // destructor
    virtual ~TreeGridBase();
protected:
    //! The only empty ConvexPolygon
    static const ConvexPolygon _empty_convex;
    
    std::list<const ConvexPolygon*> create_tetrahedron() const;
    
    //! All the shapes
    std::vector<const ConvexPolygon*> _shapes;
};

//! The TreeGrid can be used to store Obj in a optimized way
template<class Obj>
class TreeGrid : public SPGrid<Obj*, const ConvexPolygon*, TreeGridPolicy<Obj> >, public TreeGridBase
{
    friend class TreeGridPolicy<Obj>;
public:
    //! Constructor
    TreeGrid(const Navigator* nav = NULL) :
        SPGrid<Obj*, const ConvexPolygon*, TreeGridPolicy<Obj> >(
            &_empty_convex, _policy
        ),
        _policy(TreeGridPolicy<Obj>(this))
    {
        // We insert the initial tetrahedron
        typedef std::list<const ConvexPolygon*> Tetrahedron;
        Tetrahedron tetrahedron = create_tetrahedron();
        for (Tetrahedron::const_iterator i = tetrahedron.begin();
                i != tetrahedron.end(); ++i) {
            _shapes.push_back(*i);
            this->root_node().children().push_back(typename TreeGrid<Obj>::node_t(*i));
        }
    }
    
private:
    //! The navigator
    // We need it to get the StelObject positions
    // TODO: why do we need it to get the position ???
    const Navigator* _nav;
    
    //! The grid policy
    TreeGridPolicy<Obj> _policy;
    
};


#endif // _TREEGRID_HPP_
