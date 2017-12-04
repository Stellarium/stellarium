/*
 * Copyright © 2008, Roland Roberts
 *
 */

#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <set>

template <class TValue, class T>
class NamedEnum
{
  protected:
    // Constructors
    NamedEnum(const std::string _name, const TValue &_value);

  private:
    // Predicate for finding the corresponding instance
    struct Enum_Predicate_Corresponds : public std::unary_function<const NamedEnum<TValue, T> *, bool>
    {
        Enum_Predicate_Corresponds(const TValue &Value) : m_value(Value) {}
        bool operator()(const NamedEnum<TValue, T> *E) { return E->value() == m_value; }

      private:
        const TValue &m_value;
    };

    // Comparison functor for the set of instances
    struct NamedEnum_Ptr_LessThan
        : public std::binary_function<const NamedEnum<TValue, T> *, const NamedEnum<TValue, T> *, bool>
    {
        bool operator()(const NamedEnum<TValue, T> *E_1, const NamedEnum<TValue, T> *E_2)
        {
            return E_1->value() < E_2->value();
        }
    };

  public:
    // Compiler-generated copy constructor and operator= are OK.

    typedef std::set<const NamedEnum<TValue, T> *, NamedEnum_Ptr_LessThan> instances_list;
    typedef typename instances_list::const_iterator const_iterator;

    bool operator==(const TValue _value) { return this->value() == _value; }
    bool operator!=(const TValue _value) { return !(this->value() == _value); }
    bool operator==(const T &d) { return this->value() == d.value(); }

    // Access to TValue value
    const TValue &value(void) const { return m_value; }

    const std::string &name(void) const { return m_name; }

    static const TValue &min(void) { return (*s_instances.begin())->m_value; }

    static const TValue &max(void) { return (*s_instances.rbegin())->m_value; }

    static const T *find(const TValue &Value)
    {
        const_iterator it = find_if(s_instances.begin(), s_instances.end(), Enum_Predicate_Corresponds(Value));
        return (it != s_instances.end()) ? (T *)*it : NULL;
    }

    static bool isValidValue(const TValue &Value) { return find(Value) != NULL; }

    bool operator==(const NamedEnum<TValue, T> &x) { return (this == &x); }

    bool operator!=(const NamedEnum<TValue, T> &x) { return !(this == &x); }

    // Number of elements
    static typename instances_list::size_type size(void) { return s_instances.size(); }

    // Iteration
    static const_iterator begin(void) { return s_instances.begin(); }
    static const_iterator end(void) { return s_instances.end(); }

  private:
    TValue m_value;
    std::string m_name;
    static instances_list s_instances;
};

template <class TValue, class T>
inline NamedEnum<TValue, T>::NamedEnum(std::string name, const TValue &value) : m_value(value), m_name(name)
{
    s_instances.insert(this);
}
