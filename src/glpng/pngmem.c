
/* pngmem.c - stub functions for memory allocation
 *
 * libpng 1.0.2 - June 14, 1998
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, Glenn Randers-Pehrson
 *
 * This file provides a location for all memory allocation.  Users who
 * need special memory handling are expected to supply replacement
 * functions for png_malloc() and png_free(), and to use
 * png_create_read_struct_2() and png_create_write_struct_2() to
 * identify the replacement functions.
 */

#define PNG_INTERNAL
#include "png.h"

/* Borland DOS special memory handler */
#if defined(__TURBOC__) && !defined(_Windows) && !defined(__FLAT__)
/* if you change this, be sure to change the one in png.h also */

/* Allocate memory for a png_struct.  The malloc and memset can be replaced
   by a single call to calloc() if this is thought to improve performance. */
png_voidp
png_create_struct(int type)
{
#ifdef PNG_USER_MEM_SUPPORTED
   return (png_create_struct_2(type, NULL));
}

/* Alternate version of png_create_struct, for use with user-defined malloc. */
png_voidp
png_create_struct_2(int type, png_malloc_ptr malloc_fn)
{
#endif /* PNG_USER_MEM_SUPPORTED */
   png_size_t size;
   png_voidp struct_ptr;

   if (type == PNG_STRUCT_INFO)
     size = sizeof(png_info);
   else if (type == PNG_STRUCT_PNG)
     size = sizeof(png_struct);
   else
     return ((png_voidp)NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   if(malloc_fn != NULL)
   {
      if ((struct_ptr = (*(malloc_fn))(NULL, size)) != NULL)
         png_memset(struct_ptr, 0, size);
         return (struct_ptr);
   }
#endif /* PNG_USER_MEM_SUPPORTED */
   if ((struct_ptr = (png_voidp)farmalloc(size)) != NULL)
   {
      png_memset(struct_ptr, 0, size);
   }
   return (struct_ptr);
}


/* Free memory allocated by a png_create_struct() call */
void
png_destroy_struct(png_voidp struct_ptr)
{
#ifdef PNG_USER_MEM_SUPPORTED
   png_destroy_struct_2(struct_ptr, (png_free_ptr)NULL);
}

/* Free memory allocated by a png_create_struct() call */
void
png_destroy_struct_2(png_voidp struct_ptr, png_free_ptr free_fn)
{
#endif
   if (struct_ptr != NULL)
   {
#ifdef PNG_USER_MEM_SUPPORTED
      if(free_fn != NULL)
      {
         png_struct dummy_struct;
         png_structp png_ptr = &dummy_struct;
         (*(free_fn))(png_ptr, struct_ptr);
         struct_ptr = NULL;
         return;
      }
#endif /* PNG_USER_MEM_SUPPORTED */
      farfree (struct_ptr);
      struct_ptr = NULL;
   }
}

/* Allocate memory.  For reasonable files, size should never exceed
 * 64K.  However, zlib may allocate more then 64K if you don't tell
 * it not to.  See zconf.h and png.h for more information. zlib does
 * need to allocate exactly 64K, so whatever you call here must
 * have the ability to do that.
 *
 * Borland seems to have a problem in DOS mode for exactly 64K.
 * It gives you a segment with an offset of 8 (perhaps to store its
 * memory stuff).  zlib doesn't like this at all, so we have to
 * detect and deal with it.  This code should not be needed in
 * Windows or OS/2 modes, and only in 16 bit mode.  This code has
 * been updated by Alexander Lehmann for version 0.89 to waste less
 * memory.
 *
 * Note that we can't use png_size_t for the "size" declaration,
 * since on some systems a png_size_t is a 16-bit quantity, and as a
 * result, we would be truncating potentially larger memory requests
 * (which should cause a fatal error) and introducing major problems.
 */
png_voidp
png_malloc(png_structp png_ptr, png_uint_32 size)
{
#ifndef PNG_USER_MEM_SUPPORTED
   png_voidp ret;
#endif
   if (png_ptr == NULL || size == 0)
      return ((png_voidp)NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   if(png_ptr->malloc_fn != NULL)
       return ((png_voidp)(*(png_ptr->malloc_fn))(png_ptr, size));
   else
       return png_malloc_default(png_ptr, size);
}

png_voidp
png_malloc_default(png_structp png_ptr, png_uint_32 size)
{
   png_voidp ret;
#endif /* PNG_USER_MEM_SUPPORTED */

#ifdef PNG_MAX_MALLOC_64K
   if (size > (png_uint_32)65536L)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif

   if (size == (png_uint_32)65536L)
   {
      if (png_ptr->offset_table == NULL)
      {
         /* try to see if we need to do any of this fancy stuff */
         ret = farmalloc(size);
         if (ret == NULL || ((png_size_t)ret & 0xffff))
         {
            int num_blocks;
            png_uint_32 total_size;
            png_bytep table;
            int i;
            png_byte huge * hptr;

            if (ret != NULL)
            {
               farfree(ret);
               ret = NULL;
            }

            num_blocks = (int)(1 << (png_ptr->zlib_window_bits - 14));
            if (num_blocks < 1)
               num_blocks = 1;
            if (png_ptr->zlib_mem_level >= 7)
               num_blocks += (int)(1 << (png_ptr->zlib_mem_level - 7));
            else
               num_blocks++;

            total_size = ((png_uint_32)65536L) * (png_uint_32)num_blocks+16;

            table = farmalloc(total_size);

            if (table == NULL)
            {
               png_error(png_ptr, "Out Of Memory."); /* Note "O" and "M" */
            }

            if ((png_size_t)table & 0xfff0)
            {
               png_error(png_ptr, "Farmalloc didn't return normalized pointer");
            }

            png_ptr->offset_table = table;
            png_ptr->offset_table_ptr = farmalloc(num_blocks *
               sizeof (png_bytep));

            if (png_ptr->offset_table_ptr == NULL)
            {
               png_error(png_ptr, "Out Of memory.");
            }

            hptr = (png_byte huge *)table;
            if ((png_size_t)hptr & 0xf)
            {
               hptr = (png_byte huge *)((long)(hptr) & 0xfffffff0L);
               hptr += 16L;
            }
            for (i = 0; i < num_blocks; i++)
            {
               png_ptr->offset_table_ptr[i] = (png_bytep)hptr;
               hptr += (png_uint_32)65536L;
            }

            png_ptr->offset_table_number = num_blocks;
            png_ptr->offset_table_count = 0;
            png_ptr->offset_table_count_free = 0;
         }
      }

      if (png_ptr->offset_table_count >= png_ptr->offset_table_number)
         png_error(png_ptr, "Out of Memory.");

      ret = png_ptr->offset_table_ptr[png_ptr->offset_table_count++];
   }
   else
      ret = farmalloc(size);

   if (ret == NULL)
   {
      png_error(png_ptr, "Out of memory."); /* Note "o" and "m" */
   }

   return (ret);
}

/* free a pointer allocated by png_malloc().  In the default
   configuration, png_ptr is not used, but is passed in case it
   is needed.  If ptr is NULL, return without taking any action. */
void
png_free(png_structp png_ptr, png_voidp ptr)
{
   if (png_ptr == NULL || ptr == NULL)
      return;

#ifdef PNG_USER_MEM_SUPPORTED
   if (png_ptr->free_fn != NULL)
   {
      (*(png_ptr->free_fn))(png_ptr, ptr);
      ptr = NULL;
      return;
   }
   else png_free_default(png_ptr, ptr);
}

void
png_free_default(png_structp png_ptr, png_voidp ptr)
{
#endif /* PNG_USER_MEM_SUPPORTED */
      
   if (png_ptr->offset_table != NULL)
   {
      int i;

      for (i = 0; i < png_ptr->offset_table_count; i++)
      {
         if (ptr == png_ptr->offset_table_ptr[i])
         {
            ptr = NULL;
            png_ptr->offset_table_count_free++;
            break;
         }
      }
      if (png_ptr->offset_table_count_free == png_ptr->offset_table_count)
      {
         farfree(png_ptr->offset_table);
         farfree(png_ptr->offset_table_ptr);
         png_ptr->offset_table = NULL;
         png_ptr->offset_table_ptr = NULL;
      }
   }

   if (ptr != NULL)
   {
      farfree(ptr);
      ptr = NULL;
   }
}

#else /* Not the Borland DOS special memory handler */

/* Allocate memory for a png_struct or a png_info.  The malloc and
   memset can be replaced by a single call to calloc() if this is thought
   to improve performance noticably.*/
png_voidp
png_create_struct(int type)
{
#ifdef PNG_USER_MEM_SUPPORTED
   return (png_create_struct_2(type, NULL));
}

/* Allocate memory for a png_struct or a png_info.  The malloc and
   memset can be replaced by a single call to calloc() if this is thought
   to improve performance noticably.*/
png_voidp
png_create_struct_2(int type, png_malloc_ptr malloc_fn)
{
#endif /* PNG_USER_MEM_SUPPORTED */
   png_size_t size;
   png_voidp struct_ptr;

   if (type == PNG_STRUCT_INFO)
      size = sizeof(png_info);
   else if (type == PNG_STRUCT_PNG)
      size = sizeof(png_struct);
   else
      return ((png_voidp)NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   if(malloc_fn != NULL)
   {
      if ((struct_ptr = (*(malloc_fn))(NULL, size)) != NULL)
         png_memset(struct_ptr, 0, size);
      return (struct_ptr);
   }
#endif /* PNG_USER_MEM_SUPPORTED */

#if defined(__TURBOC__) && !defined(__FLAT__)
   if ((struct_ptr = (png_voidp)farmalloc(size)) != NULL)
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
   if ((struct_ptr = (png_voidp)halloc(size,1)) != NULL)
# else
   if ((struct_ptr = (png_voidp)malloc(size)) != NULL)
# endif
#endif
   {
      png_memset(struct_ptr, 0, size);
   }

   return (struct_ptr);
}


/* Free memory allocated by a png_create_struct() call */
void
png_destroy_struct(png_voidp struct_ptr)
{
#ifdef PNG_USER_MEM_SUPPORTED
   png_destroy_struct_2(struct_ptr, (png_free_ptr)NULL);
}

/* Free memory allocated by a png_create_struct() call */
void
png_destroy_struct_2(png_voidp struct_ptr, png_free_ptr free_fn)
{
#endif /* PNG_USER_MEM_SUPPORTED */
   if (struct_ptr != NULL)
   {
#ifdef PNG_USER_MEM_SUPPORTED
      if(free_fn != NULL)
      {
         png_struct dummy_struct;
         png_structp png_ptr = &dummy_struct;
         (*(free_fn))(png_ptr, struct_ptr);
         struct_ptr = NULL;
         return;
      }
#endif /* PNG_USER_MEM_SUPPORTED */
#if defined(__TURBOC__) && !defined(__FLAT__)
      farfree(struct_ptr);
      struct_ptr = NULL;
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
      hfree(struct_ptr);
      struct_ptr = NULL;
# else
      free(struct_ptr);
      struct_ptr = NULL;
# endif
#endif
   }
}


/* Allocate memory.  For reasonable files, size should never exceed
   64K.  However, zlib may allocate more then 64K if you don't tell
   it not to.  See zconf.h and png.h for more information.  zlib does
   need to allocate exactly 64K, so whatever you call here must
   have the ability to do that. */

png_voidp
png_malloc(png_structp png_ptr, png_uint_32 size)
{
#ifndef PNG_USER_MEM_SUPPORTED
   png_voidp ret;
#endif
   if (png_ptr == NULL || size == 0)
      return ((png_voidp)NULL);

#ifdef PNG_USER_MEM_SUPPORTED
   if(png_ptr->malloc_fn != NULL)
       return ((png_voidp)(*(png_ptr->malloc_fn))(png_ptr, size));
   else
       return (png_malloc_default(png_ptr, size));
}
png_voidp
png_malloc_default(png_structp png_ptr, png_uint_32 size)
{
   png_voidp ret;
#endif /* PNG_USER_MEM_SUPPORTED */

#ifdef PNG_MAX_MALLOC_64K
   if (size > (png_uint_32)65536L)
      png_error(png_ptr, "Cannot Allocate > 64K");
#endif

#if defined(__TURBOC__) && !defined(__FLAT__)
   ret = farmalloc(size);
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
   ret = halloc(size, 1);
# else
   ret = malloc((size_t)size);
# endif
#endif

   if (ret == NULL)
   {
      png_error(png_ptr, "Out of Memory");
   }

   return (ret);
}

/* Free a pointer allocated by png_malloc().  If ptr is NULL, return
   without taking any action. */
void
png_free(png_structp png_ptr, png_voidp ptr)
{
   if (png_ptr == NULL || ptr == NULL)
      return;

#ifdef PNG_USER_MEM_SUPPORTED
   if (png_ptr->free_fn != NULL)
   {
      (*(png_ptr->free_fn))(png_ptr, ptr);
      ptr = NULL;
      return;
   }
   else png_free_default(png_ptr, ptr);
}
void
png_free_default(png_structp png_ptr, png_voidp ptr)
{
#endif /* PNG_USER_MEM_SUPPORTED */

#if defined(__TURBOC__) && !defined(__FLAT__)
   farfree(ptr);
   ptr = NULL;
#else
# if defined(_MSC_VER) && defined(MAXSEG_64K)
   hfree(ptr);
   ptr = NULL;
# else
   free(ptr);
   ptr = NULL;
# endif
#endif
}

#endif /* Not Borland DOS special memory handler */

png_voidp
png_memcpy_check (png_structp png_ptr, png_voidp s1, png_voidp s2,
   png_uint_32 length)
{
   png_size_t size;

   size = (png_size_t)length;
   if ((png_uint_32)size != length)
      png_error(png_ptr,"Overflow in png_memcpy_check.");
  
   return(png_memcpy (s1, s2, size));
}

png_voidp
png_memset_check (png_structp png_ptr, png_voidp s1, int value,
   png_uint_32 length)
{
   png_size_t size;

   size = (png_size_t)length;
   if ((png_uint_32)size != length)
      png_error(png_ptr,"Overflow in png_memset_check.");

   return (png_memset (s1, value, size));

}

#ifdef PNG_USER_MEM_SUPPORTED
/* This function is called when the application wants to use another method
 * of allocating and freeing memory.
 */
void
png_set_mem_fn(png_structp png_ptr, png_voidp mem_ptr, png_malloc_ptr
  malloc_fn, png_free_ptr free_fn)
{
   png_ptr->mem_ptr = mem_ptr;
   png_ptr->malloc_fn = malloc_fn;
   png_ptr->free_fn = free_fn;
}

/* This function returns a pointer to the mem_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before png_write_destroy and png_read_destroy are called.
 */
png_voidp
png_get_mem_ptr(png_structp png_ptr)
{
   return ((png_voidp)png_ptr->mem_ptr);
}
#endif /* PNG_USER_MEM_SUPPORTED */
