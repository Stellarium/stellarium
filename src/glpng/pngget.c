
/* pngget.c - retrieval of values from info struct
 *
 * libpng 1.0.2 - June 14, 1998
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, Glenn Randers-Pehrson
 */

#define PNG_INTERNAL
#include "png.h"

png_uint_32
png_get_rowbytes(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
      return(info_ptr->rowbytes);
   else
      return(0);
}

#ifdef PNG_EASY_ACCESS_SUPPORTED
/* easy access to info, added in libpng-0.99 */
png_uint_32
png_get_image_width(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->width;
   }
   return (0);
}

png_uint_32
png_get_image_height(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->height;
   }
   return (0);
}

png_byte
png_get_bit_depth(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->bit_depth;
   }
   return (0);
}

png_byte
png_get_color_type(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->color_type;
   }
   return (0);
}

png_byte
png_get_filter_type(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->filter_type;
   }
   return (0);
}

png_byte
png_get_interlace_type(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->interlace_type;
   }
   return (0);
}

png_byte
png_get_compression_type(png_structp png_ptr, png_infop info_ptr)
{
   if (png_ptr != NULL && info_ptr != NULL)
   {
      return info_ptr->compression_type;
   }
   return (0);
}

png_uint_32
png_get_x_pixels_per_meter(png_structp png_ptr, png_infop info_ptr)
{
#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_pHYs))
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_x_pixels_per_meter");
      if(info_ptr->phys_unit_type != PNG_RESOLUTION_METER)
          return (0);
      else return (info_ptr->x_pixels_per_unit);
   }
   else
#endif
   return (0);
}

png_uint_32
png_get_y_pixels_per_meter(png_structp png_ptr, png_infop info_ptr)
{
#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_pHYs))
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_y_pixels_per_meter");
      if(info_ptr->phys_unit_type != PNG_RESOLUTION_METER)
          return (0);
      else return (info_ptr->y_pixels_per_unit);
   }
   else
#endif
   return (0);
}

png_uint_32
png_get_pixels_per_meter(png_structp png_ptr, png_infop info_ptr)
{
#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_pHYs))
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_pixels_per_meter");
      if(info_ptr->phys_unit_type != PNG_RESOLUTION_METER ||
         info_ptr->x_pixels_per_unit != info_ptr->y_pixels_per_unit)
          return (0);
      else return (info_ptr->x_pixels_per_unit);
   }
   else
#endif
   return (0);
}

float
png_get_pixel_aspect_ratio(png_structp png_ptr, png_infop info_ptr)
   {
#if defined(PNG_READ_pHYs_SUPPORTED) || defined(PNG_WRITE_pHYs_SUPPORTED)
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_pHYs))
   {
      png_debug1(1, "in %s retrieval function\n", "png_get_aspect_ratio");
      if (info_ptr->x_pixels_per_unit == 0)
         return ((float)0.0);
      else
         return ((float)info_ptr->y_pixels_per_unit
            /(float)info_ptr->x_pixels_per_unit);
   }
   else
#endif
      return ((float)0.0);
}

#ifdef PNG_INCH_CONVERSIONS
png_uint_32
png_get_pixels_per_inch(png_structp png_ptr, png_infop info_ptr)
{
   return ((png_uint_32)((float)png_get_pixels_per_meter(png_ptr, info_ptr)
     *.03937 +.5)
}

png_uint_32
png_get_x_pixels_per_inch(png_structp png_ptr, png_infop info_ptr)
{
   return ((png_uint_32)((float)png_get_x_pixels_per_meter(png_ptr, info_ptr)
     *.03937 +.5)
}

png_uint_32
png_get_y_pixels_per_inch(png_structp png_ptr, png_infop info_ptr)
{
   return ((png_uint_32)((float)png_get_y_pixels_per_meter(png_ptr, info_ptr)
     *.03937 +.5)
}

float
png_get_x_offset_inches(png_structp png_ptr, png_infop info_ptr)
{
   return ((float)png_get_x_offset_microns(png_ptr, info_ptr)
     *.03937/1000000. +.5)
}

float
png_get_y_offset_inches(png_structp png_ptr, png_infop info_ptr)
{
   return ((float)png_get_y_offset_microns(png_ptr, info_ptr)
     *.03937/1000000. +.5)
}

#if defined(PNG_READ_pHYs_SUPPORTED)
png_uint_32
png_get_pHYs_dpi(png_structp png_ptr, png_infop info_ptr,
   png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type)
{
   png_uint_32 retval = 0;

   if (png_ptr != NULL && info_ptr != NULL && info_ptr->valid & PNG_INFO_pHYs)
   {
      png_debug1(1, "in %s retrieval function\n", "pHYs");
      if (res_x != NULL)
      {
         *res_x = info_ptr->x_pixels_per_unit;
         retval |= PNG_INFO_pHYs;
      }
      if (res_y != NULL)
      {
         *res_y = info_ptr->y_pixels_per_unit;
         retval |= PNG_INFO_pHYs;
      }
      if (unit_type != NULL)
      {
         *unit_type = (int)info_ptr->phys_unit_type;
         retval |= PNG_INFO_pHYs;
         if(unit_type == 1)
         {
            if (res_x != NULL) *res_x = (png_uint_32)(*res_x * 39.37 + .50);
            if (res_y != NULL) *res_y = (png_uint_32)(*res_y * 39.37 + .50);
         }
      }
   }
   return (retval);
}
#endif /* PNG_READ_pHYs_SUPPORTED */
#endif  /* PNG_INCH_CONVERSIONS */

/* png_get_channels really belongs in here, too, but it's been around longer */

#endif  /* PNG_EASY_ACCESS_SUPPORTED */

#if defined(PNG_READ_bKGD_SUPPORTED)
png_uint_32
png_get_bKGD(png_structp png_ptr, png_infop info_ptr,
   png_color_16p *background)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_bKGD)
      && background != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "bKGD");
      *background = &(info_ptr->background);
      return (PNG_INFO_bKGD);
   }
   return (0);
}
#endif

#if defined(PNG_READ_cHRM_SUPPORTED)
png_uint_32
png_get_cHRM(png_structp png_ptr, png_infop info_ptr,
   double *white_x, double *white_y, double *red_x, double *red_y,
   double *green_x, double *green_y, double *blue_x, double *blue_y)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_cHRM))
   {
      png_debug1(1, "in %s retrieval function\n", "cHRM");
      if (white_x != NULL)
         *white_x = (double)info_ptr->x_white;
      if (white_y != NULL)
         *white_y = (double)info_ptr->y_white;
      if (red_x != NULL)
         *red_x = (double)info_ptr->x_red;
      if (red_y != NULL)
         *red_y = (double)info_ptr->y_red;
      if (green_x != NULL)
         *green_x = (double)info_ptr->x_green;
      if (green_y != NULL)
         *green_y = (double)info_ptr->y_green;
      if (blue_x != NULL)
         *blue_x = (double)info_ptr->x_blue;
      if (blue_y != NULL)
         *blue_y = (double)info_ptr->y_blue;
      return (PNG_INFO_cHRM);
   }
   return (0);
}
#endif

#if defined(PNG_READ_gAMA_SUPPORTED)
png_uint_32
png_get_gAMA(png_structp png_ptr, png_infop info_ptr, double *file_gamma)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_gAMA)
      && file_gamma != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "gAMA");
      *file_gamma = (double)info_ptr->gamma;
      return (PNG_INFO_gAMA);
   }
   return (0);
}
#endif

#if defined(PNG_READ_sRGB_SUPPORTED)
png_uint_32
png_get_sRGB(png_structp png_ptr, png_infop info_ptr, int *file_srgb_intent)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_sRGB)
      && file_srgb_intent != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "sRGB");
      *file_srgb_intent = (int)info_ptr->srgb_intent;
      return (PNG_INFO_sRGB);
   }
   return (0);
}
#endif

#if defined(PNG_READ_hIST_SUPPORTED)
png_uint_32
png_get_hIST(png_structp png_ptr, png_infop info_ptr, png_uint_16p *hist)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_hIST)
      && hist != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "hIST");
      *hist = info_ptr->hist;
      return (PNG_INFO_hIST);
   }
   return (0);
}
#endif

png_uint_32
png_get_IHDR(png_structp png_ptr, png_infop info_ptr,
   png_uint_32 *width, png_uint_32 *height, int *bit_depth,
   int *color_type, int *interlace_type, int *compression_type,
   int *filter_type)
   
{
   if (png_ptr != NULL && info_ptr != NULL && width != NULL && height != NULL &&
      bit_depth != NULL && color_type != NULL)
   {
      int pixel_depth, channels;
      png_uint_32 rowbytes_per_pixel;

      png_debug1(1, "in %s retrieval function\n", "IHDR");
      *width = info_ptr->width;
      *height = info_ptr->height;
      *bit_depth = info_ptr->bit_depth;
      *color_type = info_ptr->color_type;
      if (compression_type != NULL)
         *compression_type = info_ptr->compression_type;
      if (filter_type != NULL)
         *filter_type = info_ptr->filter_type;
      if (interlace_type != NULL)
         *interlace_type = info_ptr->interlace_type;

      /* check for potential overflow of rowbytes */
      if (*color_type == PNG_COLOR_TYPE_PALETTE)
         channels = 1;
      else if (*color_type & PNG_COLOR_MASK_COLOR)
         channels = 3;
      else
         channels = 1;
      if (*color_type & PNG_COLOR_MASK_ALPHA)
         channels++;
      pixel_depth = *bit_depth * channels;
      rowbytes_per_pixel = (pixel_depth + 7) >> 3;
      if ((*width > (png_uint_32)2147483647L/rowbytes_per_pixel))
      {
         png_warning(png_ptr,
            "Width too large for libpng to process image data.");
      }
      return (1);
   }
   return (0);
}

#if defined(PNG_READ_oFFs_SUPPORTED)
png_uint_32
png_get_oFFs(png_structp png_ptr, png_infop info_ptr,
   png_uint_32 *offset_x, png_uint_32 *offset_y, int *unit_type)
{
   if (png_ptr != NULL && info_ptr != NULL && (info_ptr->valid & PNG_INFO_oFFs)
      && offset_x != NULL && offset_y != NULL && unit_type != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "oFFs");
      *offset_x = info_ptr->x_offset;
      *offset_y = info_ptr->y_offset;
      *unit_type = (int)info_ptr->offset_unit_type;
      return (PNG_INFO_oFFs);
   }
   return (0);
}
#endif

#if defined(PNG_READ_pCAL_SUPPORTED)
png_uint_32
png_get_pCAL(png_structp png_ptr, png_infop info_ptr,
   png_charp *purpose, png_int_32 *X0, png_int_32 *X1, int *type, int *nparams,
   png_charp *units, png_charpp *params)
{
   if (png_ptr != NULL && info_ptr != NULL && info_ptr->valid & PNG_INFO_pCAL &&
      purpose != NULL && X0 != NULL && X1 != NULL && type != NULL &&
      nparams != NULL && units != NULL && params != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "pCAL");
      *purpose = info_ptr->pcal_purpose;
      *X0 = info_ptr->pcal_X0;
      *X1 = info_ptr->pcal_X1;
      *type = (int)info_ptr->pcal_type;
      *nparams = (int)info_ptr->pcal_nparams;
      *units = info_ptr->pcal_units;
      *params = info_ptr->pcal_params;
      return (PNG_INFO_pCAL);
   }
   return (0);
}
#endif

#if defined(PNG_READ_pHYs_SUPPORTED)
png_uint_32
png_get_pHYs(png_structp png_ptr, png_infop info_ptr,
   png_uint_32 *res_x, png_uint_32 *res_y, int *unit_type)
{
   png_uint_32 retval = 0;

   if (png_ptr != NULL && info_ptr != NULL && info_ptr->valid & PNG_INFO_pHYs)
   {
      png_debug1(1, "in %s retrieval function\n", "pHYs");
      if (res_x != NULL)
      {
         *res_x = info_ptr->x_pixels_per_unit;
         retval |= PNG_INFO_pHYs;
      }
      if (res_y != NULL)
      {
         *res_y = info_ptr->y_pixels_per_unit;
         retval |= PNG_INFO_pHYs;
      }
      if (unit_type != NULL)
      {
         *unit_type = (int)info_ptr->phys_unit_type;
         retval |= PNG_INFO_pHYs;
      }
   }
   return (retval);
}
#endif

png_uint_32
png_get_PLTE(png_structp png_ptr, png_infop info_ptr, png_colorp *palette,
   int *num_palette)
{
   if (png_ptr != NULL && info_ptr != NULL && info_ptr->valid & PNG_INFO_PLTE &&
       palette != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "PLTE");
      *palette = info_ptr->palette;
      *num_palette = info_ptr->num_palette;
      png_debug1(3, "num_palette = %d\n", *num_palette);
      return (PNG_INFO_PLTE);
   }
   return (0);
}

#if defined(PNG_READ_sBIT_SUPPORTED)
png_uint_32
png_get_sBIT(png_structp png_ptr, png_infop info_ptr, png_color_8p *sig_bit)
{
   if (png_ptr != NULL && info_ptr != NULL && info_ptr->valid & PNG_INFO_sBIT &&
       sig_bit != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "sBIT");
      *sig_bit = &(info_ptr->sig_bit);
      return (PNG_INFO_sBIT);
   }
   return (0);
}
#endif

#if defined(PNG_READ_tEXt_SUPPORTED) || defined(PNG_READ_zTXt_SUPPORTED)
png_uint_32
png_get_text(png_structp png_ptr, png_infop info_ptr, png_textp *text_ptr,
   int *num_text)
{
   if (png_ptr != NULL && info_ptr != NULL && info_ptr->num_text > 0)
   {
      png_debug1(1, "in %s retrieval function\n",
         (png_ptr->chunk_name[0] == '\0' ? "text"
             : (png_const_charp)png_ptr->chunk_name));
      if (text_ptr != NULL)
         *text_ptr = info_ptr->text;
      if (num_text != NULL)
         *num_text = info_ptr->num_text;
      return ((png_uint_32)info_ptr->num_text);
   }
   return(0);
}
#endif

#if defined(PNG_READ_tIME_SUPPORTED)
png_uint_32
png_get_tIME(png_structp png_ptr, png_infop info_ptr, png_timep *mod_time)
{
   if (png_ptr != NULL && info_ptr != NULL && info_ptr->valid & PNG_INFO_tIME &&
       mod_time != NULL)
   {
      png_debug1(1, "in %s retrieval function\n", "tIME");
      *mod_time = &(info_ptr->mod_time);
      return (PNG_INFO_tIME);
   }
   return (0);
}
#endif

#if defined(PNG_READ_tRNS_SUPPORTED)
png_uint_32
png_get_tRNS(png_structp png_ptr, png_infop info_ptr,
   png_bytep *trans, int *num_trans, png_color_16p *trans_values)
{
   png_uint_32 retval = 0;
   if (png_ptr != NULL && info_ptr != NULL && info_ptr->valid & PNG_INFO_tRNS)
   {
      png_debug1(1, "in %s retrieval function\n", "tRNS");
      if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      {
          if (trans != NULL)
          {
             *trans = info_ptr->trans;
             retval |= PNG_INFO_tRNS;
          }
          if (trans_values != NULL)
             *trans_values = &(info_ptr->trans_values);
      }
      else /* if (info_ptr->color_type != PNG_COLOR_TYPE_PALETTE) */
      {
          if (trans_values != NULL)
          {
             *trans_values = &(info_ptr->trans_values);
             retval |= PNG_INFO_tRNS;
          }
          if(trans != NULL)
             *trans = NULL;
      }
      if(num_trans != NULL)
      {
         *num_trans = info_ptr->num_trans;
         retval |= PNG_INFO_tRNS;
      }
   }
   return (retval);
}
#endif

