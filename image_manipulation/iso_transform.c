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
static void print_help(char **argv);

static void make_block(Image32 img_in, int tile_size, const char *base_path);
static void make_block_top(Image32 img_in, int tile_size, const char *base_path);
static void make_slope0(Image32 img_in, int tile_size, const char *base_path);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   const char *path_tilemap = NULL;
   int tile_size = 16;

   //Parse arguments
   struct optparse_long longopts[] =
   {
      {"file", 'f', OPTPARSE_REQUIRED},
      {"tile", 't', OPTPARSE_REQUIRED},
      {"help", 'h', OPTPARSE_NONE},
      {0},
   };

   int option;
   struct optparse options;
   optparse_init(&options, argv);
   while((option = optparse_long(&options, longopts, NULL))!=-1)
   {
      switch(option)
      {
      case 'f':
         path_tilemap = options.optarg;
         break;
      case 't':
         tile_size = strtol(options.optarg,NULL,10);
         break;
      case 'h':
         print_help(argv);
         exit(EXIT_SUCCESS);
         break;
      case '?':
         fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
         exit(EXIT_FAILURE);
         break;
      }
   }

   if(path_tilemap==NULL)
   {
      print_help(argv);
      exit(EXIT_FAILURE);
   }

   int wx,wy,n;
   unsigned char *data = stbi_load(path_tilemap, &wx, &wy, &n, 4);
   Image32 img_in;
   img_in.w = wx;
   img_in.h = wy;
   img_in.data = (uint32_t *)data;

   make_block(img_in,tile_size,"out");
   make_block_top(img_in,tile_size,"out");
   make_block_slope0(img_in,tile_size,"out");

   return 0;
}

static void make_block(Image32 img_in, int tile_size, const char *base_path)
{
   Image32 img_out;
   img_out.w = tile_size*2;
   img_out.h = tile_size*2;
   img_out.data = malloc(sizeof(*img_out.data)*img_out.w*img_out.h);
   memset(img_out.data,0,sizeof(*img_out.data)*img_out.w*img_out.h);

   //Left side
   int ys = tile_size/2;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys++;
      for(int y = 0;y<tile_size;y++)
         img_out.data[(y+ys)*img_out.w+x] = img_in.data[y*img_in.w+x];
   }
   
   //Right side
   ys = tile_size;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys--;
      for(int y = 0;y<tile_size;y++)
         img_out.data[(y+ys)*img_out.w+x+tile_size] = img_in.data[y*img_in.w+x+(tile_size+1)];
   }

   //Top
   for(int i = 0;i<tile_size/2;i++)
   {
      for(int x = 0;x<tile_size-2*i;x++)
      {
         img_out.data[(x/2+i)*img_out.w+x+(tile_size-1)] = img_in.data[i*img_in.w+x+(tile_size+1)*2+i];
         img_out.data[(x/2+tile_size/2)*img_out.w+x+1+i*2] = img_in.data[((tile_size-1)-i)*img_in.w+x+(tile_size+1)*2+i];
      }

      for(int y = 0;y<tile_size-2*i-2;y++)
      {
         img_out.data[((tile_size/2-1)-(y/2))*img_out.w+y+1+2*i] = img_in.data[((tile_size-1)-y-i-1)*img_in.w+i+(tile_size+1)*2];
         img_out.data[((tile_size-2)-(y/2)-i)*img_out.w+y+(tile_size+1)] = img_in.data[((tile_size-1)-y-i-1)*img_in.w+((tile_size-1)-i)+(tile_size+1)*2];
      }
   }

   cp_image_t img_save;
   img_save.w = img_out.w;
   img_save.h = img_out.h;
   img_save.pix = img_out.data;
   char path[512];
   snprintf(path,512,"%s0.png",base_path);
   cp_save_png(path,&img_save);

   free(img_out.data);
}

static void make_block_top(Image32 img_in, int tile_size, const char *base_path)
{
   Image32 img_out;
   img_out.w = tile_size*2;
   img_out.h = tile_size+tile_size/4;
   img_out.data = malloc(sizeof(*img_out.data)*img_out.w*img_out.h);
   memset(img_out.data,0,sizeof(*img_out.data)*img_out.w*img_out.h);

   //Left side
   int ys = tile_size/2;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys++;
      for(int y = 0;y<tile_size/4;y++)
         img_out.data[(y+ys)*img_out.w+x] = img_in.data[y*img_in.w+x+(tile_size+1)*3];
   }
   
   //Right side
   ys = tile_size;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys--;
      for(int y = 0;y<tile_size/4;y++)
         img_out.data[(y+ys)*img_out.w+x+tile_size] = img_in.data[y*img_in.w+x+(tile_size+1)*4];
   }

   //Top
   for(int i = 0;i<tile_size/2;i++)
   {
      for(int x = 0;x<tile_size-2*i;x++)
      {
         img_out.data[(x/2+i)*img_out.w+x+(tile_size-1)] = img_in.data[i*img_in.w+x+(tile_size+1)*2+i];
         img_out.data[(x/2+tile_size/2)*img_out.w+x+1+i*2] = img_in.data[((tile_size-1)-i)*img_in.w+x+(tile_size+1)*2+i];
      }

      for(int y = 0;y<tile_size-2*i-2;y++)
      {
         img_out.data[((tile_size/2-1)-(y/2))*img_out.w+y+1+2*i] = img_in.data[((tile_size-1)-y-i-1)*img_in.w+i+(tile_size+1)*2];
         img_out.data[((tile_size-2)-(y/2)-i)*img_out.w+y+(tile_size+1)] = img_in.data[((tile_size-1)-y-i-1)*img_in.w+((tile_size-1)-i)+(tile_size+1)*2];
      }
   }

   cp_image_t img_save;
   img_save.w = img_out.w;
   img_save.h = img_out.h;
   img_save.pix = img_out.data;
   char path[512];
   snprintf(path,512,"%s1.png",base_path);
   cp_save_png(path,&img_save);

   free(img_out.data);
}

static void make_slope0(Image32 img_in, int tile_size, const char *base_path)
{
   Image32 img_out;
   img_out.w = tile_size*2;
   img_out.h = 2*tile_size+tile_size/4;
   img_out.data = malloc(sizeof(*img_out.data)*img_out.w*img_out.h);
   memset(img_out.data,0,sizeof(*img_out.data)*img_out.w*img_out.h);

   //Right side
   /*ys = tile_size;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys--;
      for(int y = 0;y<tile_size;y++)
         img_out.data[(y+ys)*img_out.w+x+tile_size] = img_in.data[y*img_in.w+x+(tile_size+1)];
   }*/

   //Slope

   cp_image_t img_save;
   img_save.w = img_out.w;
   img_save.h = img_out.h;
   img_save.pix = img_out.data;
   char path[512];
   snprintf(path,512,"%s2.png",base_path);
   cp_save_png(path,&img_save);

   free(img_out.data);
}

static void print_help(char **argv)
{
   fprintf(stderr,"Usage: %s --file FILE --tile DIM\n"
          "Convert pixelart to isometric cubes\n"
          "   --file PATH     tile atlas\n"
          "   --tile DIM      dimension of tiles\n",
         argv[0]);
}
//-------------------------------------
