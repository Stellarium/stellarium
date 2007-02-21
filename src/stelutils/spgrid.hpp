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

#ifndef _SPGRID_HPP_
#define _SPGRID_HPP_

#include <vector>
#include <list>
#include <set>
#include <algorithm>
#include <cassert>




template<class Object, class Shape, class Policy>
class SPGridNode
{
public:
    typedef SPGridNode<Object, Shape, Policy> self_t;
    typedef Shape shape_t;
    typedef Object object_t;
    typedef Policy policy_t;
    typedef std::vector<self_t> children_t;
    typedef std::vector<Object> objects_t;
    
    
    // Some accessors
    
    // !Return the objects at this level in the node
    const objects_t& objects() const {return _objects;}
    // !Return the objects at this level in the node
    objects_t& objects() {return _objects;}
    // !Return the children nodes
    const children_t& children() const {return _children;}
    // !Return the children nodes
    children_t& children() {return _children;}
    // !return the shape of this node
    const Shape& shape() const {return _shape;}
    
    //! Constructor
    SPGridNode(const Shape& shape):
        _shape(shape)
    {
    }

private:
    Shape _shape;
    children_t _children;
    objects_t _objects;
};


//! @brief Space Partitioning Grid container
//!
//! This class manage a tree of a set of geometrical objects
//! Every node is associated to a shape, and contains only objects 
//! and children nodes geometrically contained in this shape.
//!
//! This way we can dramatically improve the performance on 
//! search operations 
template<class Object, class Shape, class Policy, class Node = SPGridNode<Object, Shape, Policy> >
class SPGrid : public std::set<Object>
{
public:
    //typedef SPGridNode<Object, Shape, Policy> node_t;
    typedef Node node_t;
    
    //! contructor
    SPGrid(const Shape& s, Policy& policy):
        std::set<Object>(), _root_node(s), _policy(policy)
    {
    }
    
    //! filter the objects contained in a given shape
    template<class S>
    void filter_intersect(const S& s)
    {
        // This is a totaly non-optimized method
        // because we recompute everything each time, when we
        // should use the previous informations we have
        this->clear();
        insert_intersect(_root_node,
                         static_cast<std::set<Object>&>(*this),
                         s, _policy);
    }
    
    //! Add an object in the grid
    void insert(const Object& o)
    {
        bool ret = insert(_root_node, o, _policy);
        assert(ret);
    }
    
    //! Return the depth of the grid
    unsigned int depth() const {return depth(_root_node);}
    
protected:
    unsigned int depth(const node_t& node) const
    {
        if (node.children().empty()) {return 0;}
        unsigned int max = 0;
        typedef typename node_t::children_t::const_iterator CIter;
        for(CIter iter = node.children().begin(); iter != node.children().end(); ++iter) {
           max = std::max(depth(*iter), max);
        }
        return 1 + max;
    }
    
    //! Fill recursively the container with all the objects in the node.
    template<class C>
    void insert_all(const node_t& node, C& cont) const
    {
        // We add the objects in the node
        std::copy(node.objects().begin(), node.objects().end(), std::inserter(cont, cont.end()));
        
        // Then we make it for all the children nodes
        typedef typename node_t::children_t::const_iterator CIter;
        for(CIter iter = node.children().begin(); iter != node.children().end(); ++iter) {
            insert_all(*iter, cont);
        }
    }
    
    //! Fill rercursively the container with all the objects intersecting the given shape
    template<class C, class S>
    void insert_intersect(const node_t& node, C& cont, const S& shape, Policy& policy) const
    {
        // If our shape doesn't even intersect the other shape,
        // We don't try further
        if (!policy.intersect(shape, node.shape())) {return;}
        
        
        // We add the object in the node
        typedef typename node_t::objects_t::const_iterator Iter;
        for (Iter iter = node.objects().begin(); iter != node.objects().end(); ++iter) {
            if (policy.intersect(shape, *iter)) {
                cont.insert(cont.end(), *iter);
            }
        }
        // Then we make it for all the children nodes
        typedef typename node_t::children_t::const_iterator CIter;
        for(CIter iter = node.children().begin(); iter != node.children().end(); ++iter) {
            // If the node is included into the shape
            // we can just insert all the objects
            if (policy.contains(shape, iter->shape())) {
                insert_all(*iter, cont);
            }
            else
            {
                insert_intersect(*iter, cont, shape, policy);
            }
        }
    }
    
    //! Insert an object into the node
    bool insert(node_t& node, const Object& o, Policy& policy) {
        if (!policy.contains(node.shape(), o)) {return false;}
        // if we have children we try to insert the object in one of them
        if (!node.children().empty())
        {
            for (typename node_t::children_t::iterator iter = node.children().begin();
                 iter != node.children().end(); ++iter)
            {
                if (insert(*iter, o, policy)) return true;
            }
            // For the moment we say that a object has to fit into one of the children
            assert(false);
        }
        
        // We don't have any children, then we add the object here
        node.objects().push_back(o);
        
        // Now we check if we need to split the node
        if (policy.split_cond(node)) {
            std::list<Shape> shapes = policy.split(node.shape());
            for (typename std::list<Shape>::iterator iter = 
                 shapes.begin();
                 iter != shapes.end(); ++iter)
            {
                //assert(Policy::intersect(_shape, *iter));
                node.children().push_back(node_t(*iter));
            }
            
            // We have to re-insert all the objects !
            typename node_t::objects_t objs = node.objects();
            node.objects().clear();
            
            for (typename node_t::objects_t::iterator iter = objs.begin();
                 iter != objs.end(); ++iter)
            {
                insert(node, *iter, policy);
            }
        }
        return true;
    }

    
protected:
    node_t& root_node() {return _root_node;}
    
private:
     node_t _root_node;
     Policy& _policy;
};


#endif
