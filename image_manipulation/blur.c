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
void boxblur_line(const uint16_t * restrict src, uint16_t * restrict dst, int width, float rad);
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

   struct timespec tstart;
   clock_gettime(CLOCK_REALTIME, &tstart);
   blur(&img,10.015f);
   struct timespec tend;
   clock_gettime(CLOCK_REALTIME, &tend);
   double time_s = (tend.tv_sec-tstart.tv_sec)+(tend.tv_nsec-tstart.tv_nsec)*1e-9;
   printf("time: %lf s\n",time_s);

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
   uint64_t *buffer0 = malloc(sizeof(*buffer0)*img->w);
   uint64_t *buffer1 = malloc(sizeof(*buffer1)*img->w);

   //Horizontal
   for(int y = 0;y<img->h;y++)
   {
      boxblur_line((uint16_t *)(img2.data+y*img->w),(uint16_t *)(img->data+y*img->w),img->w,sz);
      boxblur_line((uint16_t *)(img->data+y*img->w),(uint16_t *)(img2.data+y*img->w),img->w,sz);
      boxblur_line((uint16_t *)(img2.data+y*img->w),(uint16_t *)(img->data+y*img->w),img->w,sz);
   }

   //Vertical
   for(int x = 0;x<img->w;x++)
   {
      for(int y = 0;y<img->h;y++)
         buffer0[y] = img->data[y*img->w+x];

      boxblur_line((uint16_t *)buffer0,(uint16_t *)buffer1,img->h,sz);
      boxblur_line((uint16_t *)buffer1,(uint16_t *)buffer0,img->h,sz);
      boxblur_line((uint16_t *)buffer0,(uint16_t *)buffer1,img->h,sz);

      for(int y = 0;y<img->h;y++)
         img->data[y*img->w+x] = buffer0[y];
   }

   free(img2.data);
   free(buffer0);
   free(buffer1);
}

void boxblur_line(const uint16_t * restrict src, uint16_t * restrict dst, int width, float rad)
{
   int r = (int)rad;
   int32_t alpha = ((int32_t)(rad*64.f))&63;
   int32_t alpha1 = 64-alpha;
   int32_t alpha_total = alpha-alpha1;
   int32_t s1,s2,d;
   s1 = s2 = -((2*r+2)/2)*4;
   d = 0;

   int32_t amp_div = HLH_max(1,(2*r+1)*64+alpha*2);
   int32_t amp = (65536.*64)/HLH_max(1,(2*r+1)*64+alpha*2);
   //printf("%d\n",amp);
   int32_t amp_clip;
   if(amp>128)
      amp_clip = (((int64_t)65536*65536*64)/amp)-1;
   else
      amp_clip = 0x7fffffff;

   int32_t sum_r = src[0]*alpha_total;
   int32_t sum_g = src[1]*alpha_total;
   int32_t sum_b = src[2]*alpha_total;
   int32_t sum_a = src[3]*alpha_total;

   for(int i = 0;i<r+1;i++)
   {
      sum_r+=src[0]*(alpha+alpha1);
      sum_g+=src[1]*(alpha+alpha1);
      sum_b+=src[2]*(alpha+alpha1);
      sum_a+=src[3]*(alpha+alpha1);
      s1+=4;
      //if(i==r) //TODO: maybe?
         //pix1 = src[1];
   }

   for(int i = 0;i<r;i++)
   {
      sum_r+=src[s1]*alpha1+src[s1+4]*alpha;
      sum_g+=src[s1+1]*alpha1+src[s1+5]*alpha;
      sum_b+=src[s1+2]*alpha1+src[s1+6]*alpha;
      sum_a+=src[s1+3]*alpha1+src[s1+7]*alpha;
      s1+=4;
   }

   for(int i = 0;i<=r;i++)
   {
      sum_r+=src[s1]*alpha1+src[s1+4]*alpha;
      sum_g+=src[s1+1]*alpha1+src[s1+5]*alpha;
      sum_b+=src[s1+2]*alpha1+src[s1+6]*alpha;
      sum_a+=src[s1+3]*alpha1+src[s1+7]*alpha;
      s1+=4;

      uint64_t cr = (((uint64_t)sum_r*amp)/(65536*64));
      uint64_t cg = (((uint64_t)sum_g*amp)/(65536*64));
      uint64_t cb = (((uint64_t)sum_b*amp)/(65536*64));
      uint64_t ca = (((uint64_t)sum_a*amp)/(65536*64));
      dst[d] = cr;
      dst[d+1] = cg;
      dst[d+2] = cb;
      dst[d+3] = ca;
      d+=4;

      sum_r-=src[0]*(alpha+alpha1);
      sum_g-=src[1]*(alpha+alpha1);
      sum_b-=src[2]*(alpha+alpha1);
      sum_a-=src[3]*(alpha+alpha1);
      s2+=4;
   }

   for(int i = r+1;i<width-r;i++)
   {
      sum_r+=src[s1]*alpha1+src[s1+4]*alpha;
      sum_g+=src[s1+1]*alpha1+src[s1+5]*alpha;
      sum_b+=src[s1+2]*alpha1+src[s1+6]*alpha;
      sum_a+=src[s1+3]*alpha1+src[s1+7]*alpha;
      s1+=4;

      uint64_t cr = (((uint64_t)sum_r*amp)/(65536*64));
      uint64_t cg = (((uint64_t)sum_g*amp)/(65536*64));
      uint64_t cb = (((uint64_t)sum_b*amp)/(65536*64));
      uint64_t ca = (((uint64_t)sum_a*amp)/(65536*64));
      dst[d] = cr;
      dst[d+1] = cg;
      dst[d+2] = cb;
      dst[d+3] = ca;
      d+=4;

      sum_r-=src[s2]*alpha+src[s2+4]*alpha1;
      sum_g-=src[s2+1]*alpha+src[s2+5]*alpha1;
      sum_b-=src[s2+2]*alpha+src[s2+6]*alpha1;
      sum_a-=src[s2+3]*alpha+src[s2+7]*alpha1;
      s2+=4;
   }

   for(int i = width-r;i<width;i++)
   {
      sum_r+=src[width*4-1]*alpha1+src[width*4-1+4]*alpha;
      sum_g+=src[width*4-1+1]*alpha1+src[width*4-1+5]*alpha;
      sum_b+=src[width*4-1+2]*alpha1+src[width*4-1+6]*alpha;
      sum_a+=src[width*4-1+3]*alpha1+src[width*4-1+7]*alpha;
      s1+=4;

      uint64_t cr = (((uint64_t)sum_r*amp)/(65536*64));
      uint64_t cg = (((uint64_t)sum_g*amp)/(65536*64));
      uint64_t cb = (((uint64_t)sum_b*amp)/(65536*64));
      uint64_t ca = (((uint64_t)sum_a*amp)/(65536*64));
      dst[d] = cr;
      dst[d+1] = cg;
      dst[d+2] = cb;
      dst[d+3] = ca;
      d+=4;

      sum_r-=src[s2]*alpha+src[s2+4]*alpha1;
      sum_g-=src[s2+1]*alpha+src[s2+5]*alpha1;
      sum_b-=src[s2+2]*alpha+src[s2+6]*alpha1;
      sum_a-=src[s2+3]*alpha+src[s2+7]*alpha1;
      s2+=4;
   }
}
//-------------------------------------
