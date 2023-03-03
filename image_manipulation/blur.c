/*
Fast gaussian blur approximation

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
void boxblur_line(uint64_t *src, uint64_t *dst, int width, float rad);
static void blur(Image64 *img, float sz);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   const char *path_in = argv[1];
   const char *path_out = "out.png";

   //Load input
   int x,y,n;
   unsigned char *data = stbi_load(path_in, &x, &y, &n, 4);
   Image32 img_io;
   img_io.w = x;
   img_io.h = y;
   img_io.data = (uint32_t *)data;

   //Convert to Image64
   Image64 img;
   img.w = x;
   img.h = y;
   img.data = malloc(sizeof(*img.data)*img.w*img.h);
   for(int i = 0;i<img.w*img.h;i++)
      img.data[i] = color_u32_to_u64(img_io.data[i]);

   blur(&img,20.015f);

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

static void blur(Image64 *img, float sz)
{
   Image64 img2;
   img2.w = img->w;
   img2.h = img->h;
   img2.data = malloc(sizeof(*img2.data)*img2.w*img2.h);
   memcpy(img2.data,img->data,sizeof(*img2.data)*img2.w*img2.h);

   for(int y = 0;y<img->h;y++)
      boxblur_line(img2.data+y*img->w,img->data+y*img->w,img->w,sz);
}

void boxblur_line(uint64_t *src, uint64_t *dst, int width, float rad)
{
   int r = (int)rad;
   int32_t alpha = ((int32_t)(rad*64.f))&63;
   int32_t alpha1 = 64-alpha;
   int32_t alpha_total = alpha-alpha1;
   uint32_t mask = width-1;
   int32_t s1,s2,d;
   s1 = s2 = -((r+1)/2);
   d = 0;

   uint64_t pix = src[s2&mask];
   int32_t sum_r = ((pix>>0)&65535)*(alpha_total);
   int32_t sum_g = ((pix>>16)&65535)*(alpha_total);
   int32_t sum_b = ((pix>>32)&65535)*(alpha_total);
   int32_t sum_a = ((pix>>48)&65535)*(alpha_total);

   int32_t amp = (65536.*64)/HLH_max(1,r*64+alpha*2);
   int32_t amp_clip;
   if(amp>128)
      amp_clip = (((int64_t)65536*65536*64)/amp)-1;
   else
      amp_clip = 0x7fffffff;

   for(int i = 0;i<r;i++)
   {
      uint64_t pix0 = src[s1&mask];
      uint64_t pix1 = src[(s1+1)&mask];
      sum_r+=((pix0>>0)&65535)*alpha1+((pix1>>0)&65535)*alpha;
      sum_g+=((pix0>>16)&65535)*alpha1+((pix1>>16)&65535)*alpha;
      sum_b+=((pix0>>32)&65535)*alpha1+((pix1>>32)&65535)*alpha;
      sum_a+=((pix0>>48)&65535)*alpha1+((pix1>>48)&65535)*alpha;
      s1++;
   }

   for(int i = 0;i<width;i++)
   {
      uint64_t pix0 = src[s1&mask];
      uint64_t pix1 = src[(s1+1)&mask];
      sum_r+=((pix0>>0)&65535)*alpha1+((pix1>>0)&65535)*alpha;
      sum_g+=((pix0>>16)&65535)*alpha1+((pix1>>16)&65535)*alpha;
      sum_b+=((pix0>>32)&65535)*alpha1+((pix1>>32)&65535)*alpha;
      sum_a+=((pix0>>48)&65535)*alpha1+((pix1>>48)&65535)*alpha;
      s1++;

      uint64_t cr = (((int64_t)sum_r*amp)/(65536*64))&0x7fff;
      uint64_t cg = (((int64_t)sum_g*amp)/(65536*64))&0x7fff;
      uint64_t cb = (((int64_t)sum_b*amp)/(65536*64))&0x7fff;
      uint64_t ca = (((int64_t)sum_a*amp)/(65536*64))&0x7fff;
      //printf("%d %d %d %d\n",cr,cg,cb,ca);
      dst[d] = cr|(cg<<16)|(cb<<32)|(ca<<48);
      d++;

      pix0 = src[s2&mask];
      pix1 = src[(s2+1)&mask];
      sum_r-=((pix0>>0)&65535)*alpha+((pix1>>0)&65535)*alpha1;
      sum_g-=((pix0>>16)&65535)*alpha+((pix1>>16)&65535)*alpha1;
      sum_b-=((pix0>>32)&65535)*alpha+((pix1>>32)&65535)*alpha1;
      sum_a-=((pix0>>48)&65535)*alpha+((pix1>>48)&65535)*alpha1;
      s2++;
   }

   //int32_t frf = rad*256.-r*256.;
   //int32_t iarrf = (1./(rad+rad+1.))*256;
   //float fr = rad-r;
   //float iarr = 1.f/(rad+rad+1.f);

   //#pragma omp parallel for schedule(dynamic, 1)
   /*for(int i = 0;i<src->height;i++)
   {
      int ti = i*src->width;
      int li = ti;
      int ri = ti+r;

      uint64_t pix = src->data[ti];
      int16_t fv_r = (pix>>0)&65535;
      int16_t fv_g = (pix>>16)&65535;
      int16_t fv_b = (pix>>32)&65535;
      int16_t fv_a = (pix>>48)&65535;
      //int fv_r = ((int32_t)src->data[ti].rgb.r)<<8;
      //int fv_g = ((int32_t)src->data[ti].rgb.g)<<8;
      //int fv_b = ((int32_t)src->data[ti].rgb.b)<<8;

      pix = src->data[ti+src->w-1];
      int16_t lv_r = (pix>>0)&65535;
      int16_t lv_g = (pix>>16)&65535;
      int16_t lv_b = (pix>>32)&65535;
      int16_t lv_a = (pix>>48)&65535;
      //int lv_r = ((int32_t)src->data[ti+src->width-1].rgb.r)<<8;
      //int lv_g = ((int32_t)src->data[ti+src->width-1].rgb.g)<<8;
      //int lv_b = ((int32_t)src->data[ti+src->width-1].rgb.b)<<8;

      int32_t val_r = (r+1)*fv_r;
      int32_t val_g = (r+1)*fv_g;
      int32_t val_b = (r+1)*fv_b;

      for(int j = 0;j<r;j++)
      {
         val_r+=((int32_t)src->data[ti+j].rgb.r)<<8;
         val_g+=((int32_t)src->data[ti+j].rgb.g)<<8;
         val_b+=((int32_t)src->data[ti+j].rgb.b)<<8;
      }

      for(int j = 0;j<=r;j++)
      {
         val_r+=(((int32_t)src->data[ri].rgb.r)<<8)-fv_r;
         val_g+=(((int32_t)src->data[ri].rgb.g)<<8)-fv_g;
         val_b+=(((int32_t)src->data[ri].rgb.b)<<8)-fv_b;

         int32_t frac_r = frf*((fv_r>>8)+src->data[ri+1].rgb.r);
         int32_t frac_g = frf*((fv_g>>8)+src->data[ri+1].rgb.g);
         int32_t frac_b = frf*((fv_b>>8)+src->data[ri+1].rgb.b);
         dst->data[ti].rgb.r = (iarrf*(val_r+frac_r)+32768)>>16;
         dst->data[ti].rgb.g = (iarrf*(val_g+frac_g)+32768)>>16;
         dst->data[ti].rgb.b = (iarrf*(val_b+frac_b)+32768)>>16;
         //float frac_r = fr*(fv_r+src->data[ri+1].rgb.r);
         //float frac_g = fr*(fv_g+src->data[ri+1].rgb.g);
         //float frac_b = fr*(fv_b+src->data[ri+1].rgb.b);

         //dst->data[ti].rgb.r = (val_r+frac_r)*iarr+.5f;
         //dst->data[ti].rgb.g = (val_g+frac_g)*iarr+.5f;
         //dst->data[ti].rgb.b = (val_b+frac_b)*iarr+.5f;
         dst->data[ti].rgb.a = src->data[ti].rgb.a;

         ri++;
         ti++;
      }

      //printf("%d %d %d %d\n",li,ti,ri,r);
      for(int j = r+1;j<src->width-r;j++)
      {
         //val_r+=src->data[ri].rgb.r-src->data[li].rgb.r;
         //val_g+=src->data[ri].rgb.g-src->data[li].rgb.g;
         //val_b+=src->data[ri].rgb.b-src->data[li].rgb.b;

         val_r+=(((int32_t)(src->data[ri].rgb.r-src->data[li].rgb.r))<<8);
         val_g+=(((int32_t)(src->data[ri].rgb.g-src->data[li].rgb.g))<<8);
         val_b+=(((int32_t)(src->data[ri].rgb.b-src->data[li].rgb.b))<<8);

         //float frac_r = fr*(src->data[li].rgb.r+src->data[ri+1].rgb.r);
         //float frac_g = fr*(src->data[li].rgb.g+src->data[ri+1].rgb.g);
         //float frac_b = fr*(src->data[li].rgb.b+src->data[ri+1].rgb.b);
         int32_t frac_r = frf*(src->data[li].rgb.r+src->data[ri+1].rgb.r);
         int32_t frac_g = frf*(src->data[li].rgb.g+src->data[ri+1].rgb.g);
         int32_t frac_b = frf*(src->data[li].rgb.b+src->data[ri+1].rgb.b);

         dst->data[ti].rgb.r = (iarrf*(val_r+frac_r)+32768)>>16;
         dst->data[ti].rgb.g = (iarrf*(val_g+frac_g)+32768)>>16;
         dst->data[ti].rgb.b = (iarrf*(val_b+frac_b)+32768)>>16;
         //dst->data[ti].rgb.r = (val_r+frac_r)*iarr+.5f;
         //dst->data[ti].rgb.g = (val_g+frac_g)*iarr+.5f;
         //dst->data[ti].rgb.b = (val_b+frac_b)*iarr+.5f;
         dst->data[ti].rgb.a = src->data[ti].rgb.a;

         ri++;
         li++;
         ti++;
      }

      for(int j = src->width-r;j<src->width;j++)
      {
         //val_r+=lv_r-src->data[li].rgb.r;
         //val_g+=lv_g-src->data[li].rgb.g;
         //val_b+=lv_b-src->data[li].rgb.b;

         //float frac_r = fr*(src->data[li].rgb.r+lv_r);
         //float frac_g = fr*(src->data[li].rgb.g+lv_r);
         //float frac_b = fr*(src->data[li].rgb.b+lv_r);
         val_r+=-(((int32_t)src->data[li].rgb.r)<<8)+lv_r;
         val_g+=-(((int32_t)src->data[li].rgb.g)<<8)+lv_g;
         val_b+=-(((int32_t)src->data[li].rgb.b)<<8)+lv_b;

         int32_t frac_r = frf*((lv_r>>8)+src->data[li].rgb.r);
         int32_t frac_g = frf*((lv_g>>8)+src->data[li].rgb.g);
         int32_t frac_b = frf*((lv_b>>8)+src->data[li].rgb.b);

         dst->data[ti].rgb.r = (iarrf*(val_r+frac_r)+32768)>>16;
         dst->data[ti].rgb.g = (iarrf*(val_g+frac_g)+32768)>>16;
         dst->data[ti].rgb.b = (iarrf*(val_b+frac_b)+32768)>>16;
         //dst->data[ti].rgb.r = (val_r+frac_r)*iarr+.5f;
         //dst->data[ti].rgb.g = (val_g+frac_g)*iarr+.5f;
         //dst->data[ti].rgb.b = (val_b+frac_b)*iarr+.5f;
         dst->data[ti].rgb.a = src->data[ti].rgb.a;

         li++;
         ti++;
      }
   }*/
}
//-------------------------------------
