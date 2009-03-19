#ifndef GC_PTR_H
#define GC_PTR_H

#include <hash_map>
#include <stdexcept>
#include <algorithm>

template<typename T>
class GCPtr
{
public:
	typedef T  ValueType;
	typedef T* PointerType;
	typedef T& ReferenceType;

	GCPtr();
	GCPtr(const GCPtr<T>& ptr);
	GCPtr(PointerType ptr);
	~GCPtr();

	GCPtr<T>& operator =(const GCPtr<T>& ptr);
	GCPtr<T>& operator =(PointerType ptr);

	PointerType operator ->() const;

	ReferenceType operator *() const;

	operator bool() const;
	bool operator ! () const;

	PointerType getBasePointer() const;
private:
	PointerType mPtr;

	void assignPointer(PointerType ptr);
	void releasePointer();

	class MemoryMap;

	static MemoryMap* sMemoryMap;
	static int sCount;
};

template<typename T>
class GCPtr<T>::MemoryMap
{
public:
	MemoryMap()
	{
	}
	~MemoryMap()
	{
		std::for_each(mMap.begin(), mMap.end(), delPtr());
	}

	int& operator [](PointerType ptr)
	{
		return mMap[ptr];
	}

	void erase(PointerType ptr)
	{
		mMap.erase(ptr);
	}
private:
	MemoryMap(const MemoryMap&);
	MemoryMap& operator =(const MemoryMap&);

	typedef stdext::hash_map<PointerType, int> MapType;
	MapType mMap;

	class delPtr 
		: public std::unary_function<typename MapType::value_type, void> 
	{
	public:
		result_type operator ()(const argument_type& val)const
		{
			delete val.first;
		}
	};
};

template<typename T>
typename GCPtr<T>::MemoryMap* GCPtr<T>::sMemoryMap = 0;

template<typename T>
int GCPtr<T>::sCount = 0;

template<typename T>
GCPtr<T>::GCPtr()
	: mPtr(0)
{
	if (++sCount == 1)
		sMemoryMap = new MemoryMap();
}

template<typename T>
GCPtr<T>::GCPtr(const GCPtr<T>& ptr)
{
	if (++sCount == 1)
		sMemoryMap = new MemoryMap();
	assignPointer(ptr.mPtr);
}

template<typename T>
GCPtr<T>::GCPtr(PointerType ptr)
{
	if (++sCount == 1)
		sMemoryMap = new MemoryMap();
	assignPointer(ptr);
}

template<typename T>
GCPtr<T>::~GCPtr()
{
	releasePointer();
	if (--sCount == 0)
		delete sMemoryMap;
}

template<typename T>
GCPtr<T>& GCPtr<T>::operator =(const GCPtr<T>& ptr)
{
	return (*this = ptr.mPtr);
}

template<typename T>
GCPtr<T>& GCPtr<T>::operator =(PointerType ptr)
{
	if (mPtr != ptr)
	{
		releasePointer();
		assignPointer(ptr);
	}
	return *this;
}

template<typename T>
typename GCPtr<T>::PointerType GCPtr<T>::operator ->() const
{
	if (mPtr)
		return mPtr;
	throw std::runtime_error("Access to NULL pointer.");
}

template<typename T>
typename GCPtr<T>::ReferenceType GCPtr<T>::operator *() const
{
	if (mPtr)
		return *mPtr;
	throw std::runtime_error("Access to NULL pointer.");
}

template<typename T>
void GCPtr<T>::assignPointer(PointerType ptr)
{
	mPtr = ptr;
	if (mPtr)
		++(*sMemoryMap)[mPtr];
}

template<typename T>
void GCPtr<T>::releasePointer()
{
	if (mPtr && --(*sMemoryMap)[mPtr] == 0)
	{
		sMemoryMap->erase(mPtr);
		delete mPtr;
	}
}

template<typename T>
GCPtr<T>::operator bool() const
{
	return mPtr != 0;
}

template<typename T>
bool GCPtr<T>::operator ! () const
{
	return !mPtr;
}

template<typename T>
typename GCPtr<T>::PointerType GCPtr<T>::getBasePointer() const
{
	return mPtr;
}

#endif