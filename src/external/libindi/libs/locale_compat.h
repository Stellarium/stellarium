/*
    INDI LIB
    Utility routines for saving and restoring the current locale, handling platform differences
    Copyright (C) 2017 Andy Galasso

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/

#pragma once

#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif

// C interface
//
// usage:
//
//     locale_char_t *save = indi_locale_C_numeric_push();
//     ...
//     indi_locale_C_numeric_pop(save);
//

#if defined(_MSC_VER)

#include <string.h>
#include <malloc.h>

typedef wchar_t locale_char_t;
#define INDI_LOCALE(s) L#s

__inline static locale_char_t *indi_setlocale(int category, const locale_char_t *locale)
{
    return _wcsdup(_wsetlocale(category, locale));
}

__inline static void indi_restore_locale(int category, locale_char_t *prev)
{
    _wsetlocale(category, prev);
    free(prev);
}

# define _INDI_C_INLINE __inline

#else // _MSC_VER

typedef char locale_char_t;
#define INDI_LOCALE(s) s

inline static locale_char_t *indi_setlocale(int category, const locale_char_t *locale)
{
    return setlocale(category, locale);
}

inline static void indi_restore_locale(int category, locale_char_t *prev)
{
    setlocale(category, prev);
}

# define _INDI_C_INLINE inline

#endif // _MSC_VER

_INDI_C_INLINE static locale_char_t *indi_locale_C_numeric_push()
{
    return indi_setlocale(LC_NUMERIC, INDI_LOCALE("C"));
}

_INDI_C_INLINE static void indi_locale_C_numeric_pop(locale_char_t *prev)
{
    indi_restore_locale(LC_NUMERIC, prev);
}

#undef _INDI_C_INLINE

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

// C++ interface
//
// usage:
//
//     AutoCNumeric locale; // LC_NUMERIC locale set to "C" for object scope
//     ...
//

class AutoLocale
{
    int m_category;
    locale_char_t *m_orig;

public:

    AutoLocale(int category, const locale_char_t *locale)
        : m_category(category)
    {
        m_orig = indi_setlocale(category, locale);
    }

    // method Restore can be used to restore the original locale
    // before the object goes out of scope
    void Restore()
    {
        if (m_orig)
        {
            indi_restore_locale(m_category, m_orig);
            m_orig = nullptr;
        }
    }

    ~AutoLocale()
    {
        Restore();
    }
};

class AutoCNumeric : public AutoLocale
{
public:
    AutoCNumeric() : AutoLocale(LC_NUMERIC, INDI_LOCALE("C")) { }
};

#endif // __cplusplus
