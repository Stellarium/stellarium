/*******************************************************************************
 Copyright(c) 2016 Andy Kirkham. All rights reserved.
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include <gtest/gtest.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cstdlib>
#include <cstring>

#include "base64.h"

TEST(CORE_BASE64, Test_to64frombits)
{
    int len = 0, size = sizeof("FOOBARBAZ") - 1 * 4 / 3 + 4 + 1;
    const unsigned char convert[] = "FOOBARBAZ";
    unsigned char *p_outbuf       = nullptr;

    p_outbuf = (unsigned char *)calloc(1, size);
    ASSERT_TRUE(p_outbuf);

    len = to64frombits(p_outbuf, convert, sizeof(convert) - 1);
    ASSERT_EQ(sizeof("Rk9PQkFSQkFa") - 1, len);
    ASSERT_STREQ("Rk9PQkFSQkFa", (const char *)p_outbuf);

    free(p_outbuf);
}

TEST(CORE_BASE64, Test_from64tobits)
{
    int len = 0, size = sizeof("Rk9PQkFSQkFa") - 1 * 3 / 4 + 1;
    const char convert[] = "Rk9PQkFSQkFa";
    char *p_outbuf       = nullptr;

    p_outbuf = (char *)calloc(1, size);
    ASSERT_TRUE(p_outbuf);

    len = from64tobits(p_outbuf, convert);
    ASSERT_EQ(sizeof("FOOBARBAZ") - 1, len);
    ASSERT_STREQ("FOOBARBAZ", (char *)p_outbuf);

    free(p_outbuf);
}

TEST(CORE_BASE64, Test_from64tobits_fast)
{
    int len = 0, size = sizeof("Rk9PQkFSQkFa") - 1 * 3 / 4 + 1;
    const char convert[] = "Rk9PQkFSQkFa";
    char *p_outbuf       = nullptr;

    p_outbuf = (char *)calloc(1, size);
    ASSERT_TRUE(p_outbuf);

    len = from64tobits_fast(p_outbuf, convert, strlen(convert));
    ASSERT_EQ(sizeof("FOOBARBAZ") - 1, len);
    ASSERT_STREQ("FOOBARBAZ", (char *)p_outbuf);

    free(p_outbuf);
}
