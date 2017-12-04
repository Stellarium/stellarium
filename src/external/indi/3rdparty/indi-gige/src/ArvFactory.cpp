/*
 GigE interface wrapper on araviss
 Copyright (C) 2016 Hendrik Beijeman (hbeyeman@gmail.com)

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
#include "ArvGeneric.h"
#include "BlackFly.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <string.h>

#define BLACKFLY_MODEL "BFLY-PGE-31S4M"

arv::ArvCamera *ArvFactory::find_first_available(void)
{
    ::ArvCamera *camera    = arv_camera_new(NULL);
    const char *model_name = arv_camera_get_model_name(camera);

    if ((camera == NULL) || (model_name == NULL))
        return NULL;

    if (memmem(model_name, strlen(model_name), BLACKFLY_MODEL, strlen(BLACKFLY_MODEL)))
    {
        printf("Creating BlackFly...\n");
        return new BlackFly((void *)camera);
    }
    else
    {
        printf("Creating Generic...\n");
        return new ArvGeneric((void *)camera);
    }
}
