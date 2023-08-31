/*
transform pixelart to isometric cubes

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
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   int wx,wy,n;
   unsigned char *data = stbi_load(argv[1], &wx, &wy, &n, 4);
   Image32 img_in;
   img_in.w = wx;
   img_in.h = wy;
   img_in.data = (uint32_t *)data;

   Image32 img_out;
   img_out.w = 32;
   img_out.h = 32;
   img_out.data = malloc(sizeof(*img_out.data)*img_out.w*img_out.h);
   memset(img_out.data,0,sizeof(*img_out.data)*img_out.w*img_out.h);

   //Left side
   int ys = 8;
   for(int x = 0;x<16;x++)
   {
      if(x&1) ys++;
      for(int y = 0;y<16;y++)
         img_out.data[(y+ys)*img_out.w+x] = img_in.data[y*img_in.w+x];
   }
   
   //Right side
   ys = 16;
   for(int x = 0;x<16;x++)
   {
      if(x&1) ys--;
      for(int y = 0;y<16;y++)
         img_out.data[(y+ys)*img_out.w+x+16] = img_in.data[y*img_in.w+x+17];
   }

   //Top
   for(int i = 0;i<8;i++)
   {
      for(int x = 0;x<16-2*i;x++)
      {
         img_out.data[(x/2+i)*img_out.w+x+15] = img_in.data[i*img_in.w+x+34+i];
         img_out.data[(x/2+8)*img_out.w+x+1+i*2] = img_in.data[(15-i)*img_in.w+x+34+i];
      }

      for(int y = 0;y<16-2*i-2;y++)
      {
         img_out.data[(7-(y/2))*img_out.w+y+1+2*i] = img_in.data[(15-y-i-1)*img_in.w+i+34];
         img_out.data[(14-(y/2)-i)*img_out.w+y+17] = img_in.data[(15-y-i-1)*img_in.w+(15-i)+34];
      }
   }
   /*int xs = 17;
   ys = 0;
   for(int y = 0;y<16;y++)
   {
      if((y&1)==0) xs-=2;
      else ys++;

      for(int x = 0;x<16;x++)
      {
         img_out.data[(x/2+ys)*img_out.w+x+xs] = img_in.data[y*img_in.w+x+34];
      }
   }*/

   cp_image_t img_save;
   img_save.w = img_out.w;
   img_save.h = img_out.h;
   img_save.pix = img_out.data;
   cp_save_png("out.png",&img_save);

   return 0;
}
//-------------------------------------
