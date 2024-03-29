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

static Image32 image_create(int w, int h);
static void image_free(Image32 img);
static void image_save(Image32 img, const char *base_path, int id);

static void make_block(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_block_top(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope0(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope1(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope2(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope3(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope4(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope5(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope6(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope7(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope8(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope9(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope10(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope11(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope12(Image32 img_in, int tile_size, const char *base_path, int base_id);
static void make_slope13(Image32 img_in, int tile_size, const char *base_path, int base_id);

static void add_wall_left(Image32 img, Image32 tilemap, int ox, int oy, int tile_size);
static void add_wall_right(Image32 img, Image32 tilemap, int ox, int oy, int tile_size);
static void add_wall_top(Image32 img, Image32 tilemap, int ox, int oy, int tile_size);
static void add_wall_left_upper(Image32 img, Image32 tilemap, int ox, int oy, int tile_size);
static void add_wall_right_upper(Image32 img, Image32 tilemap, int ox, int oy, int tile_size);
static void add_slope_left(Image32 img, Image32 tilemap, int ox, int oy, int tile_size, int limit_left, int limit_right);
static void add_slope_right(Image32 img, Image32 tilemap, int ox, int oy, int tile_size, int limit_left, int limit_right);
static void add_wall_slope_left(Image32 img, Image32 tilemap, int ox, int oy, int tile_size);
static void add_wall_slope_right(Image32 img, Image32 tilemap, int ox, int oy, int tile_size);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   const char *path_tilemap = NULL;
   int tile_size = 16;
   int base_id = 0;
   const char *path_base = "";

   //Parse arguments
   struct optparse_long longopts[] =
   {
      {"file", 'f', OPTPARSE_REQUIRED},
      {"tile", 't', OPTPARSE_REQUIRED},
      {"id", 'i', OPTPARSE_REQUIRED},
      {"base", 'b', OPTPARSE_REQUIRED},
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
      case 'b':
         path_base = options.optarg;
         break;
      case 't':
         tile_size = strtol(options.optarg,NULL,10);
         break;
      case 'i':
         base_id = strtol(options.optarg,NULL,10);
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

   make_block(img_in,tile_size,path_base,base_id);
   make_block_top(img_in,tile_size,path_base,base_id);
   make_slope0(img_in,tile_size,path_base,base_id);
   make_slope1(img_in,tile_size,path_base,base_id);
   make_slope2(img_in,tile_size,path_base,base_id);
   make_slope3(img_in,tile_size,path_base,base_id);
   make_slope4(img_in,tile_size,path_base,base_id);
   make_slope5(img_in,tile_size,path_base,base_id);
   make_slope6(img_in,tile_size,path_base,base_id);
   make_slope7(img_in,tile_size,path_base,base_id);
   make_slope8(img_in,tile_size,path_base,base_id);
   make_slope9(img_in,tile_size,path_base,base_id);
   make_slope10(img_in,tile_size,path_base,base_id);
   make_slope11(img_in,tile_size,path_base,base_id);
   make_slope12(img_in,tile_size,path_base,base_id);
   make_slope13(img_in,tile_size,path_base,base_id);

   return 0;
}

static Image32 image_create(int w, int h)
{
   Image32 img;
   img.w = w;
   img.h = h;
   img.data = malloc(sizeof(*img.data)*img.w*img.h);
   memset(img.data,0,sizeof(*img.data)*img.w*img.h);

   return img;
}

static void image_free(Image32 img)
{
   if(img.data==NULL)
      return;

   free(img.data);
   img.data = NULL;
}

static void image_save(Image32 img, const char *base_path, int id)
{
   if(img.data==NULL)
      return;

   cp_image_t img_save;
   img_save.w = img.w;
   img_save.h = img.h;
   img_save.pix = (cp_pixel_t *)img.data;

   char path[512];
   snprintf(path,512,"%s%05d.png",base_path,id);
   cp_save_png(path,&img_save);
}

static void make_block(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,tile_size*2);

   add_wall_left(img_out,img_in,0,0,tile_size);
   add_wall_right(img_out,img_in,0,0,tile_size);
   add_wall_top(img_out,img_in,0,0,tile_size);

   image_save(img_out,base_path,base_id+0);
   image_free(img_out);
}

static void make_block_top(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,tile_size+tile_size/4);

   add_wall_left_upper(img_out,img_in,0,0,tile_size);
   add_wall_right_upper(img_out,img_in,0,0,tile_size);
   add_wall_top(img_out,img_in,0,0,tile_size);
   
   image_save(img_out,base_path,base_id+1);
   image_free(img_out);
}

static void make_slope0(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_right_upper(img_out,img_in,0,0,tile_size);
   add_wall_right(img_out,img_in,0,tile_size/4,tile_size);
   add_slope_left(img_out,img_in,0,0,tile_size,0,0);
   
   image_save(img_out,base_path,base_id+2);
   image_free(img_out);
}

static void make_slope1(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_left_upper(img_out,img_in,0,0,tile_size);
   add_wall_left(img_out,img_in,0,tile_size/4,tile_size);
   add_slope_right(img_out,img_in,0,0,tile_size,0,0);

   image_save(img_out,base_path,base_id+3);
   image_free(img_out);
}

static void make_slope2(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_left_upper(img_out,img_in,0,0,tile_size);
   add_wall_left(img_out,img_in,0,tile_size/4,tile_size);
   add_wall_slope_right(img_out,img_in,0,0,tile_size);

   image_save(img_out,base_path,base_id+4);
   image_free(img_out);
}

static void make_slope3(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_right_upper(img_out,img_in,0,0,tile_size);
   add_wall_right(img_out,img_in,0,tile_size/4,tile_size);
   add_wall_slope_left(img_out,img_in,0,0,tile_size);

   image_save(img_out,base_path,base_id+5);
   image_free(img_out);
}

static void make_slope4(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_right_upper(img_out,img_in,0,0,tile_size);
   add_wall_right(img_out,img_in,0,tile_size/4,tile_size);
   add_wall_left_upper(img_out,img_in,0,0,tile_size);
   add_wall_left(img_out,img_in,0,tile_size/4,tile_size);
   add_slope_left(img_out,img_in,0,0,tile_size,tile_size,0);
   add_slope_right(img_out,img_in,0,0,tile_size,0,tile_size);

   image_save(img_out,base_path,base_id+6);
   image_free(img_out);
}

static void make_slope5(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_slope_right(img_out,img_in,0,0,tile_size,0,0);
   add_wall_left_upper(img_out,img_in,0,0,tile_size);
   add_wall_left(img_out,img_in,0,tile_size/4,tile_size);
   add_wall_slope_right(img_out,img_in,0,0,tile_size);

   image_save(img_out,base_path,base_id+7);
   image_free(img_out);
}

static void make_slope6(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_right_upper(img_out,img_in,0,0,tile_size);
   add_wall_right(img_out,img_in,0,tile_size/4,tile_size);
   add_wall_left_upper(img_out,img_in,0,0,tile_size);
   add_wall_left(img_out,img_in,0,tile_size/4,tile_size);

   image_save(img_out,base_path,base_id+8);
   image_free(img_out);
}

static void make_slope7(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_slope_left(img_out,img_in,0,0,tile_size,0,0);
   add_wall_slope_left(img_out,img_in,0,0,tile_size);
   add_wall_right_upper(img_out,img_in,0,0,tile_size);
   add_wall_right(img_out,img_in,0,tile_size/4,tile_size);

   image_save(img_out,base_path,base_id+9);
   image_free(img_out);
}

static void make_slope8(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_slope_left(img_out,img_in,0,0,tile_size,0,tile_size);
   add_slope_right(img_out,img_in,0,0,tile_size,tile_size,0);

   image_save(img_out,base_path,base_id+10);
   image_free(img_out);
}

static void make_slope9(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_left_upper(img_out,img_in,0,0,tile_size);
   add_wall_left(img_out,img_in,0,tile_size/4,tile_size);
   add_slope_right(img_out,img_in,0,0,tile_size,0,0);

   //Remove above
   int xs = 1;
   int ys = 6;
   int dx = 31-xs;
   int dy = 25-ys;
   int error = 2*(dy)-dx;
   for(;xs<img_out.w;xs++)
   {
      for(int y = ys;y>=0;y--)
         img_out.data[y*img_out.w+xs] = 0;

      error+=2*dy;
      if(error>0)
      {
         error-=2*(dx);
         ys++;
      }
   }

   image_save(img_out,base_path,base_id+11);
   image_free(img_out);
}

static void make_slope10(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_slope_left(img_out,img_in,0,0,tile_size);
   add_wall_slope_right(img_out,img_in,0,0,tile_size);

   image_save(img_out,base_path,base_id+12);
   image_free(img_out);
}

static void make_slope11(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_right_upper(img_out,img_in,0,0,tile_size);
   add_wall_right(img_out,img_in,0,tile_size/4,tile_size);
   add_slope_left(img_out,img_in,0,0,tile_size,0,0);

   //Remove above
   int xs = 1;
   int ys = 26;
   int dx = 30;
   int dy = 20;
   int error = 2*(dy)-dx;
   for(;xs<img_out.w;xs++)
   {
      for(int y = ys;y>=0;y--)
         img_out.data[y*img_out.w+xs] = 0;

      error+=2*dy;
      if(error>0)
      {
         error-=2*(dx);
         ys--;
      }
   }

   image_save(img_out,base_path,base_id+13);
   image_free(img_out);
}

static void make_slope12(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_slope_left(img_out,img_in,0,0,tile_size,0,tile_size);
   add_slope_right(img_out,img_in,0,0,tile_size,tile_size,0);

   //Remove top left
   int xs = 0;
   int ys = 26;
   int dx = 18-0;
   int dy = 11-0;
   int error = 2*(dy)-dx;
   for(;xs<tile_size;xs++)
   {
      for(int y = ys;y>=0;y--)
         img_out.data[y*img_out.w+xs] = 0;

      error+=2*dy;
      if(error>0)
      {
         error-=2*(dx);
         ys--;
      }
   }

   //Remove top right
   xs = 16;
   ys = 16;
   dx = 31-xs;
   dy = 25-ys;
   error = 2*(dy)-dx;
   for(;xs<2*tile_size;xs++)
   {
      for(int y = ys;y>=0;y--)
         img_out.data[y*img_out.w+xs] = 0;

      error+=2*dy;

      if(error>0)
      {
         error-=2*(dx);
         ys++;
      }
   }


   image_save(img_out,base_path,base_id+14);
   image_free(img_out);
}

static void make_slope13(Image32 img_in, int tile_size, const char *base_path, int base_id)
{
   Image32 img_out = image_create(tile_size*2,2*tile_size+tile_size/4);

   add_wall_top(img_out,img_in,0,0,tile_size);

   image_save(img_out,base_path,base_id+15);
   image_free(img_out);
}

static void print_help(char **argv)
{
   fprintf(stderr,"Usage: %s --file FILE --tile DIM\n"
          "Convert pixelart to isometric cubes\n"
          "   --file PATH     tile atlas\n"
          "   --id ID         base id\n"
          "   --tile DIM      dimension of tiles\n",
         argv[0]);
}

static void add_wall_left(Image32 img, Image32 tilemap, int ox, int oy, int tile_size)
{
   int ys = tile_size/2;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys++;
      for(int y = 0;y<tile_size;y++)
         img.data[(y+ys+oy)*img.w+x+ox] = tilemap.data[y*tilemap.w+x];
   }
}

static void add_wall_right(Image32 img, Image32 tilemap, int ox, int oy, int tile_size)
{
   int ys = tile_size;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys--;
      for(int y = 0;y<tile_size;y++)
         img.data[(y+ys+oy)*img.w+x+ox+tile_size] = tilemap.data[y*tilemap.w+x+(tile_size+1)];
   }
}

static void add_wall_top(Image32 img, Image32 tilemap, int ox, int oy, int tile_size)
{
   for(int i = 0;i<tile_size/2;i++)
   {
      for(int x = 0;x<tile_size-2*i;x++)
      {
         img.data[(x/2+i+oy)*img.w+x+(tile_size-1)+ox] = tilemap.data[i*tilemap.w+x+(tile_size+1)*2+i];
         img.data[(x/2+oy+tile_size/2)*img.w+x+1+i*2+ox] = tilemap.data[((tile_size-1)-i)*tilemap.w+x+(tile_size+1)*2+i];
      }

      for(int y = 0;y<tile_size-2*i-2;y++)
      {
         img.data[((tile_size/2-1)-(y/2)+oy)*img.w+y+1+2*i+ox] = tilemap.data[((tile_size-1)-y-i-1)*tilemap.w+i+(tile_size+1)*2];
         img.data[((tile_size-2)-(y/2)-i+oy)*img.w+y+(tile_size+1)+ox] = tilemap.data[((tile_size-1)-y-i-1)*tilemap.w+((tile_size-1)-i)+(tile_size+1)*2];
      }
   }
}

static void add_wall_left_upper(Image32 img, Image32 tilemap, int ox, int oy, int tile_size)
{
   int ys = tile_size/2;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys++;
      for(int y = 0;y<tile_size/4;y++)
         img.data[(y+ys+oy)*img.w+x+ox] = tilemap.data[y*tilemap.w+x+(tile_size+1)*3];
   }
}

static void add_wall_right_upper(Image32 img, Image32 tilemap, int ox, int oy, int tile_size)
{
   int ys = tile_size;
   for(int x = 0;x<tile_size;x++)
   {
      if(x&1) ys--;
      for(int y = 0;y<tile_size/4;y++)
         img.data[(y+ys+oy)*img.w+x+ox+tile_size] = tilemap.data[y*tilemap.w+x+(tile_size+1)*4];
   }
}

static void add_slope_left(Image32 img, Image32 tilemap, int ox, int oy, int tile_size, int limit_left, int limit_right)
{
   int xs = 0;
   int ys = 27;
   int dx = 15-0;
   int dy = 28-0;
   int error = 2*(dx)-dy;
   int ty = tile_size*2+tile_size/4-1;

   if(limit_right==0)
      limit_right = img.w;

   for(int i = 0;i<28;i++)
   {
      error+=2*dx;

      for(int x = HLH_max(0,limit_left-xs);x<tile_size&&x+xs<limit_right;x++)
         img.data[(ys+oy+(x+(xs&1)+1)/2)*img.w+xs+x+ox] = tilemap.data[ty*tilemap.w+x+(tile_size+1)*5];
      ty--;

      if(error>0)
      {
         error-=2*(dy);

         if(xs&1)
         {
            for(int x = HLH_max(0,limit_left-xs);x<tile_size&&x+xs<limit_right;x++)
               img.data[(ys-1+oy+(x+(xs&1)+1)/2)*img.w+xs+x+ox] = tilemap.data[ty*tilemap.w+x+(tile_size+1)*5];
            ty--;
         }

         xs++;
      }

      ys--;
   }

}

static void add_slope_right(Image32 img, Image32 tilemap, int ox, int oy, int tile_size, int limit_left, int limit_right)
{
   int xs = 32;
   int ys = 27;
   int dx = 15-0;
   int dy = 28-0;
   int error = 2*(dx)-dy;
   int ty = tile_size*2+tile_size/4-1;
   if(limit_right==0)
      limit_right = img.w;
   for(int i = 0;i<28;i++)
   {
      error+=2*dx;

      for(int x = HLH_max(0,-limit_right+xs);x<tile_size&&xs-x>limit_left;x++)
         img.data[(ys+oy+(x+(xs&1)+1)/2)*img.w+xs-x-1+ox] = tilemap.data[ty*tilemap.w+(tile_size-1-x)+(tile_size+1)*6];
      ty--;

      if(error>0)
      {
         error-=2*(dy);

         if(xs&1)
         {
            for(int x = HLH_max(0,xs-limit_right);x<tile_size&&xs-x>limit_left;x++)
               img.data[(ys-1+oy+(x+(xs&1)+1)/2)*img.w+xs-x-1+ox] = tilemap.data[ty*tilemap.w+(tile_size-1-x)+(tile_size+1)*6];
            ty--;
         }

         xs--;
      }

      ys--;
   }
}

static void add_wall_slope_left(Image32 img, Image32 tilemap, int ox, int oy, int tile_size)
{
   int xs = 0;
   int ys = 26;
   int dx = 18-0;
   int dy = 11-0;
   int error = 2*(dy)-dx;
   for(int x = 0;x<tile_size;x++)
   {
      for(int y = HLH_max(0,ys-(tile_size/2+(x+1)/2));y<tile_size/4;y++)
         img.data[(y+tile_size/2+(x+1)/2+oy)*img.w+x+ox] = tilemap.data[y*tilemap.w+x+(tile_size+1)*3];
      for(int y = HLH_max(0,ys-(tile_size/2+tile_size/4+(x+1)/2));y<tile_size;y++)
         img.data[(y+tile_size/2+tile_size/4+(x+1)/2+oy)*img.w+x+ox] = tilemap.data[y*tilemap.w+x];

      error+=2*dy;
      if(error>0)
      {
         error-=2*(dx);
         ys--;
      }

      xs++;
   }
}

static void add_wall_slope_right(Image32 img, Image32 tilemap, int ox, int oy, int tile_size)
{
   int xs = 16;
   int ys = 16;
   int dx = 31-xs;
   int dy = 25-ys;
   int error = 2*(dy)-dx;
   for(int x = 0;x<tile_size;x++)
   {
      for(int y = HLH_max(0,ys-(tile_size/4-(x+1)/2+tile_size));y<tile_size;y++)
         img.data[(y+oy+tile_size/4-(x+1)/2+tile_size)*img.w+x+ox+tile_size] = tilemap.data[y*tilemap.w+x+(tile_size+1)];
      for(int y = HLH_max(0,ys-(tile_size-(x+1)/2));y<tile_size/4;y++)
         img.data[(y+oy-(x+1)/2+tile_size)*img.w+x+ox+tile_size] = tilemap.data[y*tilemap.w+x+(tile_size+1)*4];

      error+=2*dy;

      if(error>0)
      {
         error-=2*(dx);
         ys++;
      }

      xs++;
   }
}
//-------------------------------------
