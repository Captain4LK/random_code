/*
Build engine .art to png converter

Written in 2023 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define CUTE_PNG_IMPLEMENTATION
#include "../external/cute_png.h"

#define HLH_STREAM_IMPLEMENTATION
#include "../single_header/HLH_stream.h"

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "../external/optparse.h"
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
//-------------------------------------

//Typedefs
//-------------------------------------

//Variables
cp_pixel_t palette[256];
//-------------------------------------

//Function prototypes
static void print_help(char **argv);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   const char *path_img = NULL;
   const char *path_img_out = NULL;
   const char *path_pal = NULL;

   //Parse arguments
   struct optparse_long longopts[] =
   {
      {"palette", 'p', OPTPARSE_REQUIRED},
      {"file", 'f', OPTPARSE_REQUIRED},
      {"dir", 'd', OPTPARSE_REQUIRED},
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
      case 'p':
         path_pal = options.optarg;
         break;
      case 'f':
         path_img = options.optarg;
         break;
      case 'd':
         path_img_out = options.optarg;
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

   if(path_img==NULL)
   {
      fprintf(stderr,"No input image specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   //Read palette
   HLH_rw rw_pal = {0};
   FILE *f = fopen(path_pal,"rb");
   HLH_rw_init_file(&rw_pal,f);
   for(int i = 0;i<256;i++)
   {
      palette[i].r = HLH_rw_read_u8(&rw_pal);
      palette[i].g = HLH_rw_read_u8(&rw_pal);
      palette[i].b = HLH_rw_read_u8(&rw_pal);
      palette[i].a = 255;
   }
   HLH_rw_close(&rw_pal);
   fclose(f);

   //Read images and write to output dir
   HLH_rw rw_tile = {0};
   f = fopen(path_img,"rb");
   HLH_rw_init_file(&rw_tile,f);
   int32_t num_tiles = 0;
   HLH_rw_read_u32(&rw_tile);
   HLH_rw_read_u32(&rw_tile);
   int32_t tile_start = HLH_rw_read_u32(&rw_tile);
   int32_t tile_end = HLH_rw_read_u32(&rw_tile);
   num_tiles = tile_end-tile_start+1;
   int16_t *xdims = malloc(sizeof(*xdims)*num_tiles);
   int16_t *ydims = malloc(sizeof(*ydims)*num_tiles);
   for(int i = 0;i<num_tiles;i++)
      xdims[i] = HLH_rw_read_u16(&rw_tile);
   for(int i = 0;i<num_tiles;i++)
      ydims[i] = HLH_rw_read_u16(&rw_tile);
   //Skip attributes
   for(int i = 0;i<num_tiles;i++)
      HLH_rw_read_u32(&rw_tile);
   for(int i = 0;i<num_tiles;i++)
   {
      cp_image_t img;
      img.w = xdims[i];
      img.h = ydims[i];
      img.pix = malloc(sizeof(*img.pix)*img.w*img.h);
      for(int x = 0;x<img.w;x++)
         for(int y = 0;y<img.h;y++)
            img.pix[y*img.w+x] = palette[HLH_rw_read_u8(&rw_tile)];
      char path[512];
      snprintf(path,512,"%s/%d.png",path_img_out,tile_start+i);
      cp_save_png(path,&img);

      free(img.pix);
   }
   HLH_rw_read_u32(&rw_tile);
   HLH_rw_close(&rw_tile);
   fclose(f);
   free(xdims);
   free(ydims);

   return 0;
}

static void print_help(char **argv)
{
   fprintf(stderr,"Usage: %s --dir PATH --file PATH [--palette PATH]\n"
          "Convert .art files used by build engine to pngs\n"
          "   --palette PATH     PALETTE.DAT file, vga palette if not set\n"
          "   --file PATH        .ART file\n"
          "   --dir PATH         output directory\n",
         argv[0]);
}
//-------------------------------------
