/*
Integer amount only shearing

Written in 2023 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <math.h>

#define CUTE_PNG_IMPLEMENTATION
#include "../external/cute_png.h"

#define STB_IMAGE_IMPLEMENTATION 
#include "../external/stb_image.h"

#define HLH_IMPLEMENTATION 
#include "../single_header/HLH.h"

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "../external/optparse.h"
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
//-------------------------------------

//Typedefs
typedef struct
{
   int w,h;
   uint32_t *data;
}Image32;

typedef struct
{
   int w,h;
   uint64_t *data;
}Image64;
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
static uint64_t color_u32_to_u64(uint32_t c);
static uint32_t color_u64_to_u32(uint64_t c);

static void image_xshear(Image64 *img, double shear, int width, int height);
static void image_yshear(Image64 *img, double shear, int width, int height);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   const char *path_in = argv[2];
   const char *path_out = "out.png";

   //Load input
   int wx,wy,n;
   unsigned char *data = stbi_load(path_in, &wx, &wy, &n, 4);
   Image32 img_io;
   img_io.w = wx;
   img_io.h = wy;
   img_io.data = (uint32_t *)data;

   double rot = strtod(argv[1],NULL);
   double shear_x = -tan(((rot/2.)/180.)*M_PI);
   double shear_y = sin((rot/180.)*M_PI);

   //int width = img_io.w+img_io.w*fabs(shear_x);
   //int height = img_io.h+img_io.h*fabs(shear_y);

   //Convert to Image64
   Image64 img;
   img.w = wx;
   img.h = wy;
   img.data = malloc(sizeof(*img.data)*img.w*img.h);
   for(int i = 0;i<img.w*img.h;i++)
      img.data[i] = color_u32_to_u64(img_io.data[i]);
   //printf("%lf\n",shear_x*wx);

   //double off_x = -((double)img->w-old_width)/(img->h-1);
   //double off_y = -((double)img->h-old_height)/(img->w-1);
   double step_x = -(shear_x*wx)/(wy-1);
   double step_y = -(shear_y*wy)/(wx-1);

   image_xshear(&img,shear_x,wx,wy);
   image_yshear(&img,shear_y,wx,wy);
   image_xshear(&img,shear_x,wx,wy);

   rot = -rot;
   shear_x = -tan(((rot/2.)/180.)*M_PI);
   shear_y = sin((rot/180.)*M_PI);
   //image_xshear(&img,shear_x,wx,wy);
   //image_yshear(&img,shear_y,wx,wy);
   //image_xshear(&img,shear_x,wx,wy);
   //image_xshear(&img,shear_x,wx,wy);
   //image_yshear(&img,shear_y*wy,step_y);
   //image_xshear(&img,shear_x*wx,step_x);

   free(img_io.data);
   img_io.w = img.w;
   img_io.h = img.h;
   img_io.data = malloc(sizeof(*img_io.data)*img_io.w*img_io.h);
   for(int i = 0;i<img.w*img.h;i++)
      img_io.data[i] = color_u64_to_u32(img.data[i]);
   free(img.data);
   cp_image_t img_out;
   img_out.w = img_io.w;
   img_out.h = img_io.h;
   img_out.pix = img_io.data;
   cp_save_png(path_out,&img_out);
   free(img_io.data);

   return 0;
}

static uint64_t color_u32_to_u64(uint32_t c)
{
   uint64_t color = c;

  color = ((color&0xff000000)<<24) 
      | ((color&0x00ff0000)<<16) 
      | ((color&0x0000ff00)<< 8) 
      | ((color&0x000000ff)    );
  color = ((color | (color<<8))>>1)&0x7fff7fff7fff7fffULL;

  return color;
}

static uint32_t color_u64_to_u32(uint64_t c)
{
   uint8_t c0 = (c>>7)&255;
   uint8_t c1 = (c>>23)&255;
   uint8_t c2 = (c>>39)&255;
   uint8_t c3 = (c>>55)&255;

   return c0|(c1<<8)|(c2<<16)|(c3<<24);
}

static void image_xshear(Image64 *img, double shear, int width, int height)
{
   int old_width = img->w;
   uint64_t *old_data = img->data;

   //img->w = old_width+fabs(shear)*img->h;
   img->data = malloc(sizeof(*img->data)*img->w*img->h);

   for(int y = 0;y<img->h;y++)
   {
      double sy = y-(double)img->h/2.;
      double skew = shear*(sy);
      int skewi = round(skew);
      /*if(shear<0)
      {
         skew = floor(-shear*(img->h-1))+ceil(skew);
         skewi = floor(skew);
      }*/
      double skewf = skew-skewi;
      double oleft = 0;

      for(int x = 0;x<img->w;x++)
      {
         if(x+skewi<0||x+skewi>=img->w)
            continue;
         uint64_t pix = old_data[y*old_width+x];
         img->data[y*img->w+x+skewi] = pix;
      }
   }

   free(old_data);
}

static void image_yshear(Image64 *img, double shear, int width, int height)
{
   //int old_height = img->h;
   uint64_t *old_data = img->data;

   //img->h = old_height+fabs(shear)*img->w;
   img->data = malloc(sizeof(*img->data)*img->w*img->h);

   for(int x = 0;x<img->w;x++)
   {
      double sx = x-(double)img->w/2.;
      double skew = shear*(sx);
      int skewi = round(skew);
      double skewf = skew-skewi;
      double oleft = 0;

      for(int y = 0;y<img->h;y++)
      {
         if(y+skewi<0||y+skewi>=img->h)
            continue;
         uint64_t pix = old_data[y*img->w+x];
         img->data[(y+skewi)*img->w+x] = pix;
      }
   }

   free(old_data);
}
//-------------------------------------
