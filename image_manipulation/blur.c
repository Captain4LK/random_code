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
void boxblur_line(const uint64_t * restrict src, uint64_t * restrict dst, int width, float rad);
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
      boxblur_line(img2.data+y*img->w,img->data+y*img->w,img->w,sz);
      boxblur_line(img->data+y*img->w,img2.data+y*img->w,img->w,sz);
      boxblur_line(img2.data+y*img->w,img->data+y*img->w,img->w,sz);
   }

   //Vertical
   for(int x = 0;x<img->w;x++)
   {
      for(int y = 0;y<img->h;y++)
         buffer0[y] = img->data[y*img->w+x];

      boxblur_line(buffer0,buffer1,img->h,sz);
      boxblur_line(buffer1,buffer0,img->h,sz);
      boxblur_line(buffer0,buffer1,img->h,sz);

      for(int y = 0;y<img->h;y++)
         img->data[y*img->w+x] = buffer0[y];
   }

   free(img2.data);
   free(buffer0);
   free(buffer1);
}

void boxblur_line(const uint64_t * restrict src, uint64_t * restrict dst, int width, float rad)
{
   int r = (int)rad;
   int32_t alpha = ((int32_t)(rad*64.f))&63;
   int32_t alpha1 = 64-alpha;
   int32_t alpha_total = alpha-alpha1;
   int32_t s1,s2,d;
   s1 = s2 = -((2*r+2)/2);
   d = 0;

   int32_t amp_div = HLH_max(1,(2*r+1)*64+alpha*2);
   int32_t amp = (65536.*64)/HLH_max(1,(2*r+1)*64+alpha*2);
   //printf("%d\n",amp);
   int32_t amp_clip;
   if(amp>128)
      amp_clip = (((int64_t)65536*65536*64)/amp)-1;
   else
      amp_clip = 0x7fffffff;

   uint64_t pix = src[0];
   int32_t sum_r = ((pix>>0)&65535)*(alpha_total);
   int32_t sum_g = ((pix>>16)&65535)*(alpha_total);
   int32_t sum_b = ((pix>>32)&65535)*(alpha_total);
   int32_t sum_a = ((pix>>48)&65535)*(alpha_total);

   for(int i = 0;i<r+1;i++)
   {
      uint64_t pix0 = src[0];
      uint64_t pix1 = src[0];
      //if(i==r) //TODO: maybe?
         //pix1 = src[1];
      sum_r+=((pix0>>0)&65535)*alpha1+((pix1>>0)&65535)*alpha;
      sum_g+=((pix0>>16)&65535)*alpha1+((pix1>>16)&65535)*alpha;
      sum_b+=((pix0>>32)&65535)*alpha1+((pix1>>32)&65535)*alpha;
      sum_a+=((pix0>>48)&65535)*alpha1+((pix1>>48)&65535)*alpha;
      s1++;
   }

   for(int i = 0;i<r;i++)
   {
      uint64_t pix0 = src[s1];
      uint64_t pix1 = src[s1+1];
      sum_r+=((pix0>>0)&65535)*alpha1+((pix1>>0)&65535)*alpha;
      sum_g+=((pix0>>16)&65535)*alpha1+((pix1>>16)&65535)*alpha;
      sum_b+=((pix0>>32)&65535)*alpha1+((pix1>>32)&65535)*alpha;
      sum_a+=((pix0>>48)&65535)*alpha1+((pix1>>48)&65535)*alpha;
      s1++;
   }

   for(int i = 0;i<=r;i++)
   {
      uint64_t pix0 = src[s1];
      uint64_t pix1 = src[s1+1];
      sum_r+=((pix0>>0)&65535)*alpha1+((pix1>>0)&65535)*alpha;
      sum_g+=((pix0>>16)&65535)*alpha1+((pix1>>16)&65535)*alpha;
      sum_b+=((pix0>>32)&65535)*alpha1+((pix1>>32)&65535)*alpha;
      sum_a+=((pix0>>48)&65535)*alpha1+((pix1>>48)&65535)*alpha;
      s1++;

      uint64_t cr = (((uint64_t)sum_r*amp)/(65536*64));
      uint64_t cg = (((uint64_t)sum_g*amp)/(65536*64));
      uint64_t cb = (((uint64_t)sum_b*amp)/(65536*64));
      uint64_t ca = (((uint64_t)sum_a*amp)/(65536*64));
      dst[d] = cr|(cg<<16)|(cb<<32)|(ca<<48);
      d++;

      pix0 = src[0];
      pix1 = src[0];
      sum_r-=((pix0>>0)&65535)*alpha+((pix1>>0)&65535)*alpha1;
      sum_g-=((pix0>>16)&65535)*alpha+((pix1>>16)&65535)*alpha1;
      sum_b-=((pix0>>32)&65535)*alpha+((pix1>>32)&65535)*alpha1;
      sum_a-=((pix0>>48)&65535)*alpha+((pix1>>48)&65535)*alpha1;
      s2++;
   }

   for(int i = r+1;i<width-r;i++)
   {
      uint64_t pix0 = src[s1];
      uint64_t pix1 = src[s1+1];
      sum_r+=((pix0>>0)&65535)*alpha1+((pix1>>0)&65535)*alpha;
      sum_g+=((pix0>>16)&65535)*alpha1+((pix1>>16)&65535)*alpha;
      sum_b+=((pix0>>32)&65535)*alpha1+((pix1>>32)&65535)*alpha;
      sum_a+=((pix0>>48)&65535)*alpha1+((pix1>>48)&65535)*alpha;
      s1++;

      uint64_t cr = (((uint64_t)sum_r*amp)/(65536*64));
      uint64_t cg = (((uint64_t)sum_g*amp)/(65536*64));
      uint64_t cb = (((uint64_t)sum_b*amp)/(65536*64));
      uint64_t ca = (((uint64_t)sum_a*amp)/(65536*64));
      dst[d] = cr|(cg<<16)|(cb<<32)|(ca<<48);
      d++;

      pix0 = src[s2];
      pix1 = src[s2+1];
      sum_r-=((pix0>>0)&65535)*alpha+((pix1>>0)&65535)*alpha1;
      sum_g-=((pix0>>16)&65535)*alpha+((pix1>>16)&65535)*alpha1;
      sum_b-=((pix0>>32)&65535)*alpha+((pix1>>32)&65535)*alpha1;
      sum_a-=((pix0>>48)&65535)*alpha+((pix1>>48)&65535)*alpha1;
      s2++;
   }

   for(int i = width-r;i<width;i++)
   {
      uint64_t pix0 = src[width-1];
      uint64_t pix1 = src[width-1];
      sum_r+=((pix0>>0)&65535)*alpha1+((pix1>>0)&65535)*alpha;
      sum_g+=((pix0>>16)&65535)*alpha1+((pix1>>16)&65535)*alpha;
      sum_b+=((pix0>>32)&65535)*alpha1+((pix1>>32)&65535)*alpha;
      sum_a+=((pix0>>48)&65535)*alpha1+((pix1>>48)&65535)*alpha;
      s1++;

      uint64_t cr = (((uint64_t)sum_r*amp)/(65536*64));
      uint64_t cg = (((uint64_t)sum_g*amp)/(65536*64));
      uint64_t cb = (((uint64_t)sum_b*amp)/(65536*64));
      uint64_t ca = (((uint64_t)sum_a*amp)/(65536*64));
      dst[d] = cr|(cg<<16)|(cb<<32)|(ca<<48);
      d++;

      pix0 = src[s2];
      pix1 = src[s2+1];
      sum_r-=((pix0>>0)&65535)*alpha+((pix1>>0)&65535)*alpha1;
      sum_g-=((pix0>>16)&65535)*alpha+((pix1>>16)&65535)*alpha1;
      sum_b-=((pix0>>32)&65535)*alpha+((pix1>>32)&65535)*alpha1;
      sum_a-=((pix0>>48)&65535)*alpha+((pix1>>48)&65535)*alpha1;
      s2++;
   }
}
//-------------------------------------
