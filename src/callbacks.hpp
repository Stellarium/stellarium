//  boost Callbacks.hpp  -------------------------------------------//

//  (C) Copyright Jesse Jones 2000. Permission to copy, use, modify, sell
//  and distribute this software is granted provided this copyright
//  notice appears in all copies. This software is provided "as is" without
//  express or implied warranty, and with no claim as to its suitability for
//  any purpose.

//  Revision History
//   20 Aug 2003 by Fabien Chereau
//                Removed an implicitly typename warning that occured with gcc 3.2.2
//                and changed file names for consistency
//   22 Nov 2000  1) Introduced numbered base classes to get compile time errors with too few arguments
//                2) Sprinkled the code with typename keywords for gcc.
//                3) Renamed unused unused_arg.
//                4) Method ctors no longer require raw pointers.
//   21 Nov 2000  collapsed callback0, callback1, callback2 into one class
//                (this was inspired by Doug Gregor's callback classes)
//   18 Nov 2000  Initial version

#ifndef BOOST_CALLBACKS_HPP
#define BOOST_CALLBACKS_HPP

#include <cassert>
#include "callback_helpers.hpp"

namespace boost {


// ===============================================================================
//  class callback
// ===============================================================================
template <typename RETURN_TYPE, typename ARG1 = details::unused_arg, typename ARG2 = details::unused_arg>
class callback {

//-----------------------------------
//  Types
//
public:
    typedef RETURN_TYPE result_type;
    typedef ARG1        first_argument_type;
    typedef ARG2        second_argument_type;
    
private:
    typedef typename details::generate_base_functor<RETURN_TYPE, ARG1, ARG2>::RET Functor;

//-----------------------------------
//  Initialization/Destruction
//
public:
                        ~callback()                            {if (mFunctor != 0) mFunctor->RemoveReference();}

                        callback() : mFunctor(0)               {}

                        template <typename FUNCTOR>
                        callback(FUNCTOR functor) : mFunctor(new typename details::generate_functor<FUNCTOR, RETURN_TYPE, ARG1, ARG2>::RET(functor)) {}

                        template <typename OBJECT, typename METHOD>
                        callback(OBJECT object, METHOD method)
                        {
                            typedef typename details::generate_method<RETURN_TYPE, OBJECT, METHOD, ARG1, ARG2>::RET MethodFunctor;    // $$$ should outline this, but CW 5.3 has problems with outlined template member functions

                            MethodFunctor functor(object, method);
                            mFunctor = new typename details::generate_functor<MethodFunctor, RETURN_TYPE, ARG1, ARG2>::RET(functor);
                        }

                        callback(const callback& rhs)           {if (rhs.mFunctor != 0) rhs.mFunctor->AddReference(); mFunctor = rhs.mFunctor;}

            callback& operator=(const callback& rhs)            {if (mFunctor != rhs.mFunctor) {if (rhs.mFunctor != 0) rhs.mFunctor->AddReference(); if (mFunctor != 0) mFunctor->RemoveReference(); mFunctor = rhs.mFunctor;} return *this;}

//-----------------------------------
//  API
//
public:
            RETURN_TYPE operator()() const                      {assert(mFunctor != 0); return mFunctor->Call();}
            RETURN_TYPE operator()(ARG1 arg1) const             {assert(mFunctor != 0); return mFunctor->Call(arg1);}
            RETURN_TYPE operator()(ARG1 arg1, ARG2 arg2) const  {assert(mFunctor != 0); return mFunctor->Call(arg1, arg2);}
                        // $$$ use boost::call_traits for the args?

            bool        empty() const                           {return mFunctor == 0;}

//-----------------------------------
//  Member Data
//
private:
    Functor*    mFunctor;
};


}        // namespace boost

#endif   // BOOST_CALLBACKS_HPP

