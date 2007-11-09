//  boost CallbackHelpers.hpp  -------------------------------------------//

//  (C) Copyright Jesse Jones 2000. Permission to copy, use, modify, sell
//  and distribute this software is granted provided this copyright
//  notice appears in all copies. This software is provided "as is" without
//  express or implied warranty, and with no claim as to its suitability for
//  any purpose.

//  Revision History
//   20 Aug 2003 by Fabien Chéreau
//                Changed a ARG1 into ARG2 which was causing weird bugs..!
//                Removed an implicitly typename warning that occured with gcc 3.2.2
//                and changed file names for consistency
//   22 Nov 2000  1) Introduced numbered base classes to get compile time errors with too few arguments
//              2) Sprinkled the code with typename keywords for gcc.
//              3) Renamed unused unused_arg.
//              4) Method ctors no longer require raw pointers.
//   21 Nov 2000  collapsed callback0, callback1, callback2 into one class
//              (this was inspired by Doug Gregor's callback classes)
//   18 Nov 2000  Initial version

#ifndef BOOST_CALLBACKHELPERS_HPP
#define BOOST_CALLBACKHELPERS_HPP

namespace boost {
namespace details {

typedef int atomic_int;   // $$$ use something from the thread library here


// ===============================================================================
//  struct unused_arg
// ===============================================================================
struct unused_arg{};


// ===============================================================================
//  class base_call_functor
// ===============================================================================
template <typename RETURN_TYPE, typename ARG1, typename ARG2>
class base_call_functor {

public:
   virtual        ~base_call_functor()         {}
                  base_call_functor()          {mRefCount = 1;}

         void     AddReference()               {++mRefCount;}
         void     RemoveReference()            {if (--mRefCount == 0) delete this;}
                  // $$$ cloning would be a bit safer for functors with mutable state...

private:
                  base_call_functor(const base_call_functor& rhs);   // $$$ use non_copyable
         base_call_functor& operator=(const base_call_functor& rhs);
private:
   atomic_int   mRefCount;
};


// ===============================================================================
//  0-arguments
// ===============================================================================
template <typename RETURN_TYPE>
class base_call_functor0 : public base_call_functor<RETURN_TYPE, unused_arg, unused_arg> {

public:
   virtual RETURN_TYPE Call() = 0;
};


template <typename FUNCTOR, typename RETURN_TYPE>
class call_functor0 : public base_call_functor0<RETURN_TYPE> {

public:
                  call_functor0(FUNCTOR functor)   : mFunctor(functor) {}

   virtual RETURN_TYPE Call()                      {return mFunctor();}

private:
   FUNCTOR      mFunctor;
};


// ===============================================================================
//  1-argument
// ===============================================================================
template <typename RETURN_TYPE, typename ARG1>
class base_call_functor1 : public base_call_functor<RETURN_TYPE, ARG1, unused_arg> {

public:
   virtual RETURN_TYPE Call(ARG1 arg1) = 0;
};


template <typename FUNCTOR, typename RETURN_TYPE, typename ARG1>
class call_functor1 : public base_call_functor1<RETURN_TYPE, ARG1> {

public:
                  call_functor1(FUNCTOR functor)   : mFunctor(functor) {}

   virtual RETURN_TYPE Call(ARG1 arg1)             {return mFunctor(arg1);}

private:
   FUNCTOR      mFunctor;
};


// ===============================================================================
//  2-arguments
// ===============================================================================
template <typename RETURN_TYPE, typename ARG1, typename ARG2>
class base_call_functor2 : public base_call_functor<RETURN_TYPE, ARG1, ARG2> {

public:
   virtual RETURN_TYPE Call(ARG1 arg1, ARG2 arg2) = 0;
};


template <typename FUNCTOR, typename RETURN_TYPE, typename ARG1, typename ARG2>
class call_functor2 : public base_call_functor2<RETURN_TYPE, ARG1, ARG2> {

public:
                  call_functor2(FUNCTOR functor)   : mFunctor(functor) {}

   virtual RETURN_TYPE Call(ARG1 arg1, ARG2 arg2) {return mFunctor(arg1, arg2);}

private:
   FUNCTOR      mFunctor;
};

// $$$ and call_functor3, call_functor4, etc


// ===============================================================================
//  Method Functors
//    $$$ note that the standard library only suffices for method_functor1
//    $$$ we can probably replace these with either the binder library or the
//    $$$ lambda library...
// ===============================================================================
template <typename RETURN_TYPE, typename OBJECT, typename METHOD>
class method_functor0 {
public:
                  method_functor0(OBJECT object, METHOD method) : mObject(object), mMethod(method) {}

         RETURN_TYPE operator()() const      {return (mObject->*mMethod)();}
private:
   OBJECT    mObject;
   METHOD    mMethod;
};

template <typename RETURN_TYPE, typename OBJECT, typename METHOD, typename ARG1>
class method_functor1 {
public:
                  method_functor1(OBJECT object, METHOD method) : mObject(object), mMethod(method) {}

         RETURN_TYPE operator()(ARG1 arg1) const      {return (mObject->*mMethod)(arg1);}
private:
   OBJECT    mObject;
   METHOD    mMethod;
};

template <typename RETURN_TYPE, typename OBJECT, typename METHOD, typename ARG1, typename ARG2>
class method_functor2 {
public:
                  method_functor2(OBJECT object, METHOD method) : mObject(object), mMethod(method) {}

         RETURN_TYPE operator()(ARG1 arg1, ARG2 arg2) const      {return (mObject->*mMethod)(arg1, arg2);}
private:
   OBJECT    mObject;
   METHOD    mMethod;
};


// ===============================================================================
//  struct IF
// ===============================================================================
struct SelectThen {
   template<class Then, class Else>
    struct Result {
      typedef Then RET;
    };
};


struct SelectElse {
   template<class Then, class Else>
    struct Result {
       typedef Else RET;
    };
};


template<bool Condition>
struct Selector {
   typedef SelectThen RET;
};


template<>
struct Selector<false> {
   typedef SelectElse RET;
};


template<bool Condition, class Then, class Else>
struct IF {
   typedef typename Selector<Condition>::RET select;
   typedef typename boost::details::Selector<Condition>::RET::template Result<Then,Else>::RET RET;
};


// ===============================================================================
//  struct SWITCH
// ===============================================================================
const int DEFAULT = -32767;

const int NilValue = -32768;

struct NilCase  {
    enum {tag = NilValue};
    typedef NilCase RET;
};


template <int Tag,class Statement,class Next = NilCase>
struct CASE {
       enum {tag = Tag};
      typedef Statement statement;
      typedef Next next;
};


template <int Tag,class aCase>      // non partial specialization version...
struct SWITCH {
   typedef typename aCase::next nextCase;
      enum {  tag = aCase::tag,               // VC++ 5.0 doesn't operate directly on aCase::value in IF<>
                      nextTag = nextCase::tag,// Thus we need a little cheat
                      found = (tag == Tag || tag == DEFAULT)
               };
      typedef typename IF<(nextTag == NilValue),
                              NilCase,
                              SWITCH<Tag,nextCase> >
                      ::RET nextSwitch;
      typedef typename IF<(found != 0),
                              typename aCase::statement,
                              typename nextSwitch::RET>
                      ::RET RET;
};


// ===============================================================================
//  struct generate_base_functor
// ===============================================================================
template <typename T>
struct is_used {
   enum {RET = 1};
};

template <>
struct is_used<unused_arg> {
   enum {RET = 0};
};


template <typename RETURN_TYPE, typename ARG1, typename ARG2>
struct generate_base_functor {
   enum {type = is_used<ARG1>::RET + is_used<ARG2>::RET};

   typedef base_call_functor0<RETURN_TYPE> f0;
   typedef base_call_functor1<RETURN_TYPE, ARG1> f1;
   typedef base_call_functor2<RETURN_TYPE, ARG1, ARG2> f2;

   typedef typename SWITCH<(type),
      CASE<0, f0,
      CASE<1, f1,
      CASE<2, f2> > > >::RET RET;
};


// ===============================================================================
//  struct generate_functor
// ===============================================================================
template <typename FUNCTOR, typename RETURN_TYPE, typename ARG1, typename ARG2>
struct generate_functor {
   enum {type = is_used<ARG1>::RET + is_used<ARG2>::RET};

   typedef call_functor0<FUNCTOR, RETURN_TYPE> f0;
   typedef call_functor1<FUNCTOR, RETURN_TYPE, ARG1> f1;
   typedef call_functor2<FUNCTOR, RETURN_TYPE, ARG1, ARG2> f2;

   typedef typename SWITCH<(type),
      CASE<0, f0,
      CASE<1, f1,
      CASE<2, f2> > > >::RET RET;
};


// ===============================================================================
//  struct generate_method
// ===============================================================================
template <typename RETURN_TYPE, typename OBJECT, typename METHOD, typename ARG1, typename ARG2>
struct generate_method {
   enum {type = is_used<ARG1>::RET + is_used<ARG2>::RET};

   typedef method_functor0<RETURN_TYPE, OBJECT, METHOD> f0;
   typedef method_functor1<RETURN_TYPE, OBJECT, METHOD, ARG1> f1;
   typedef method_functor2<RETURN_TYPE, OBJECT, METHOD, ARG1, ARG2> f2;

   typedef typename SWITCH<type,
      CASE<0, f0,
      CASE<1, f1,
      CASE<2, f2> > > >::RET RET;
};


}        // namespace details
}        // namespace boost

#endif   // BOOST_CALLBACKHELPERS_HPP

