/*
wafe function collapse

Written in 2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "../external/stb_image.h"

#define CUTE_PNG_IMPLEMENTATION
#include "../external/cute_png.h"
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
#define READ_ARG(I) \
   ((++(I))<argc?argv[(I)]:NULL)
//-------------------------------------

//Typedefs
typedef struct
{
   uint8_t r,g,b,a;
}Color;

typedef struct
{
   int width;
   int height;
   Color data[];
}Image;

typedef struct
{
   int hint;
   int frequency;
}Tile;
//-------------------------------------

//Variables
static struct
{
   unsigned data_size;
   unsigned data_used;

   Tile *data_meta;
   Color *data_color;
}tiles;

static int tile_size = 3;
static int *valid_adj[4] = {};
//-------------------------------------

//Function prototypes
static void print_help(char **argv);

static void tile_push(Image *input, int x, int y);
static int tile_overcmp(int ta, int tb, int dir);
//-------------------------------------

//Function implementations

int main(int argc, char *argv[])
{
   //Parse cmd arguments
   const char *path_in = NULL;
   const char *path_out = NULL;
   for(int i = 1;i<argc;i++)
   {
      if(strcmp(argv[i],"--help")==0||
         strcmp(argv[i],"-help")==0||
         strcmp(argv[i],"-h")==0||
         strcmp(argv[i],"?")==0)
         print_help(argv);
      else if(strcmp(argv[i],"-fin")==0)
         path_in = READ_ARG(i);
      else if(strcmp(argv[i],"-fout")==0)
         path_out = READ_ARG(i);
      else if(strcmp(argv[i],"-tz")==0)
         tile_size = atoi(READ_ARG(i));
   }

   if(path_in==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }
   if(path_out==NULL)
   {
      printf("No output file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   //Read input image
   int w = 0;
   int h = 0;
   int channels = 0;
   uint8_t *data = stbi_load(path_in,&w,&h,&channels,4);
   Image *input = malloc(sizeof(*input)+w*h*sizeof(*input->data));
   input->width = w;
   input->height = h;
   memcpy(input->data,data,w*h*sizeof(*input->data));
   free(data);

   //Build tiles
   for(int y = 0;y<input->height;y++)
   {
      for(int x = 0;x<input->height;x++)
      {
         tile_push(input,x,y);
      }
   }

   //Build adjacent list
   valid_adj[0] = malloc(sizeof(*valid_adj[0])*tiles.data_used*tiles.data_used);
   valid_adj[1] = malloc(sizeof(*valid_adj[1])*tiles.data_used*tiles.data_used);
   valid_adj[2] = malloc(sizeof(*valid_adj[2])*tiles.data_used*tiles.data_used);
   valid_adj[3] = malloc(sizeof(*valid_adj[3])*tiles.data_used*tiles.data_used);
   for(int a = 0;a<4;a++)
   {
      for(int i = 0;i<tiles.data_used;i++)
      {
         for(int j = 0;j<tiles.data_used;j++)
         {
            valid_adj[a][i*tiles.data_used+j] = tile_overcmp(i,j,a);
         }
      }
   }

   return 0;
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -f filename [-i filename] [-dump FORMAT]\n"
          "   -f    file to execute\n"
          "   -i    file to read input from\n"
          "   -dump dump bytecode in specified format (C, IR, bf)\n",
         argv[0],argv[0]);
}

static void tile_push(Image *input, int x, int y)
{
   Color tile_data[tile_size*tile_size];
   for(int iy = 0;iy<tile_size;iy++)
      for(int ix = 0;ix<tile_size;ix++)
         tile_data[iy*tile_size+ix] = input->data[((y+iy)%input->height)*input->width+(x+ix)%input->width];

   if(tiles.data_color==NULL)
   {
      tiles.data_size = 256;
      tiles.data_used = 0;
      tiles.data_meta = malloc(sizeof(*tiles.data_meta)*tiles.data_size);
      tiles.data_color = malloc(sizeof(*tiles.data_color)*tile_size*tile_size*tiles.data_size);
   }

   //Search existing tiles
   for(int i = 0;i<tiles.data_used;i++)
   {
      if(memcmp(tile_data,&tiles.data_color[i*tile_size*tile_size],tile_size*tile_size*sizeof(*tiles.data_color))==0)
      {
         tiles.data_meta[i].frequency++;
         return;
      }
   }

   //Add tile
   tiles.data_meta[tiles.data_used].frequency = 1;
   memcpy(&tiles.data_color[tiles.data_used*tile_size*tile_size],tile_data,tile_size*tile_size*sizeof(*tiles.data_color));
   if(tiles.data_used++>tiles.data_size)
   {
      tiles.data_size+=256;
      tiles.data_meta = realloc(tiles.data_meta,sizeof(*tiles.data_meta)*tiles.data_size);
      tiles.data_color = realloc(tiles.data_color,sizeof(*tiles.data_color)*tile_size*tile_size*tiles.data_size);
   }
}

static int tile_overcmp(int ta, int tb, int dir)
{
   int ta_offx = 0, ta_offy = 0;
   int tb_offx = 0, tb_offy = 0;
   int cmp_width = tile_size, cmp_height = tile_size;
   switch(dir)
   {
   case 0: //Up
      tb_offy = 1;
      cmp_height--;
      break;
   case 1: //Down
      ta_offy = 1;
      cmp_height--;
      break;
   case 2: //Left
      tb_offx = 1;
      cmp_width--;
      break;
   case 3: //Right
      ta_offx = 1;
      cmp_width--;
      break;
   }

   for(int y = 0;y<cmp_height;y++)
      if(memcmp(&tiles.data_color[ta*tile_size*tile_size+(ta_offy+y)*tile_size+ta_offx],&tiles.data_color[tb*tile_size*tile_size+(tb_offy+y)*tile_size+tb_offx],cmp_width*sizeof(*tiles.data_color))!=0)
         return 0;

   return 1;
}
//-------------------------------------
