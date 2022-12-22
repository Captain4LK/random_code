/*
Image quantization using k-means clustering

Written in 2021,2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//Based on: https://github.com/ogus/kmeans-quantizer (wtfpl)

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

#define CUTE_PNG_IMPLEMENTATION
#include "../external/cute_png.h"

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "../external/optparse.h"
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
#define dyn_array_init(type, array, space) \
   do { ((dyn_array *)(array))->size = (space); ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->data = malloc(sizeof(type)*(((dyn_array *)(array))->size)); } while(0)

#define dyn_array_free(type, array) \
   do { if(((dyn_array *)(array))->data) { free(((dyn_array *)(array))->data); ((dyn_array *)(array))->data = NULL; ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->size = 0; }} while(0)

#define dyn_array_add(type, array, grow, element) \
   do { ((type *)((dyn_array *)(array)->data))[((dyn_array *)(array))->used] = (element); ((dyn_array *)(array))->used++; if(((dyn_array *)(array))->used==((dyn_array *)(array))->size) { ((dyn_array *)(array))->size+=grow; ((dyn_array *)(array))->data = realloc(((dyn_array *)(array))->data,sizeof(type)*(((dyn_array *)(array))->size)); } } while(0)

#define dyn_array_element(type, array, index) \
   (((type *)((dyn_array *)(array)->data))[index])

#define MIN(a,b) \
   ((a)<(b)?(a):(b))

#define MAX(a,b) \
   ((a)>(b)?(a):(b))
//-------------------------------------

//Typedefs

typedef struct
{
   uint32_t used;
   uint32_t size;
   void *data;
}dyn_array;
//-------------------------------------

//Variables
static dyn_array *cluster_list = NULL;
static cp_pixel_t *centroid_list = NULL;
static int *assignment = NULL;
static int quant_k = 16;

static struct
{
   cp_pixel_t colors[256];
   int color_count;
}palette_in = {0};
//-------------------------------------

//Function prototypes
static void cluster_list_init();
static void cluster_list_free();
static void compute_kmeans(cp_image_t *data, int pal_in);
static void get_cluster_centroid(cp_image_t *data, int pal_in);
static cp_pixel_t colors_mean(dyn_array *color_list);
static cp_pixel_t pick_random_color(cp_image_t *data);
static int nearest_color_idx(cp_pixel_t color, cp_pixel_t *color_list);
static double distance(cp_pixel_t color0, cp_pixel_t color1);
static double colors_variance(dyn_array *color_list);

static void print_help(char **argv);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   const char *path_img = NULL;
   const char *path_img_out = NULL;
   const char *path_pal = NULL;
   const char *path_pal_out = NULL;

   //Parse arguments
   struct optparse_long longopts[] =
   {
      {"pal-out", 'O', OPTPARSE_REQUIRED},
      {"img-out", 'o', OPTPARSE_REQUIRED},
      {"pal", 'p', OPTPARSE_REQUIRED},
      {"img", 'i', OPTPARSE_REQUIRED},
      {"colors", 'c', OPTPARSE_REQUIRED},
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
      case 'O':
         path_pal_out = options.optarg;
         break;
      case 'o':
         path_img_out = options.optarg;
         break;
      case 'p':
         path_pal = options.optarg;
         break;
      case 'i':
         path_img = options.optarg;
         break;
      case 'h':
         print_help(argv);
         exit(EXIT_SUCCESS);
         break;
      case 'c':
         quant_k = options.optarg?atoi(options.optarg):16;
         break;
      case '?':
         fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
         exit(EXIT_FAILURE);
         break;
      }
   }

   if(path_img==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   cp_image_t img = cp_load_png(path_img);
   if(img.pix==NULL)
   {
      puts("Failed to load image");
      return 0;
   }

   if(path_pal!=NULL)
   {
      FILE *f = fopen(path_pal,"r");

      fscanf(f,"JASC-PAL\n0100\n%d\n",&palette_in.color_count);
      for(int i = 0;i<palette_in.color_count;i++)
      {
         fscanf(f,"%"SCNu8 "%"SCNu8 "%"SCNu8"\n",&palette_in.colors[i].r,&palette_in.colors[i].g,&palette_in.colors[i].b);
         palette_in.colors[i].a = 255;
      }

      quant_k = palette_in.color_count;

      fclose(f);
   }

   compute_kmeans(&img,path_pal!=NULL);

   for(int i = 0;i<img.w*img.h;i++)
   {
      if(path_pal!=NULL)
      {
         img.pix[i] = palette_in.colors[assignment[i]];
      }
      else
      {
         img.pix[i] = centroid_list[assignment[i]];
         img.pix[i].a = 255;
      }
   }

   if(path_img_out!=NULL)
      cp_save_png(path_img_out,&img);

   if(path_pal_out!=NULL)
   {
      FILE *f = fopen(path_pal_out,"w");

      fprintf(f,"JASC-PAL\n0100\n%d\n",quant_k);
      for(int i = 0;i<quant_k;i++)
         fprintf(f,"%d %d %d\n",centroid_list[i].r,centroid_list[i].g,centroid_list[i].b);

      fclose(f);
   }

   return 0;
}

static void cluster_list_init()
{
   cluster_list_free(quant_k);
   
   cluster_list = malloc(sizeof(*cluster_list)*quant_k);
   for(int i = 0;i<quant_k;i++)
      dyn_array_init(cp_pixel_t,&cluster_list[i],2);
}

static void cluster_list_free()
{
   if(cluster_list==NULL)
      return;

   for(int i = 0;i<quant_k;i++)
      dyn_array_free(cp_pixel_t,&cluster_list[i]);

   free(cluster_list);
   cluster_list = NULL;
}

static void compute_kmeans(cp_image_t *data, int pal_in)
{
   srand(time(NULL));
   cluster_list_init();
   centroid_list = malloc(sizeof(*centroid_list)*quant_k);
   assignment = malloc(sizeof(*assignment)*(data->w*data->h));
   for(int i = 0;i<(data->w*data->h);i++)
      assignment[i] = -1.0;

   int iter = 0;
   int max_iter = 16;
   double *previous_variance = malloc(sizeof(*previous_variance)*quant_k);
   double variance = 0.0;
   double delta = 0.0;
   double delta_max = 0.0;
   double threshold = 0.00005f;
   for(int i = 0;i<quant_k;i++)
      previous_variance[i] = 1.0;

   for(;;)
   {
      get_cluster_centroid(data,pal_in);
      cluster_list_init();
      for(int i = 0;i<data->w*data->h;i++)
      {
         cp_pixel_t color = data->pix[i];
         assignment[i] = nearest_color_idx(color,centroid_list);
         dyn_array_add(cp_pixel_t,&cluster_list[assignment[i]],1,color);
      }

      delta_max = 0.0;
      for(int i = 0;i<quant_k;i++)
      {
         variance = colors_variance(&cluster_list[i]);
         delta = fabs(previous_variance[i]-variance);
         delta_max = MAX(delta,delta_max);
         previous_variance[i] = variance;
      }

      if(delta_max<threshold||iter++>max_iter)
         break;
   }

   cluster_list_free();
   free(previous_variance);
}

static void get_cluster_centroid(cp_image_t *data, int pal_in)
{
   for(int i = 0;i<quant_k;i++)
   {
      if(cluster_list[i].used>0)
      {
         centroid_list[i] = colors_mean(&cluster_list[i]);
      }
      else
      {
         if(pal_in)
            centroid_list[i] = palette_in.colors[i];
         else
            centroid_list[i] = pick_random_color(data);
      }
   }
}

static cp_pixel_t colors_mean(dyn_array *color_list)
{
   int r = 0,g = 0,b = 0;
   int length = color_list->used;
   for(int i = 0;i<length;i++)
   {
      r+=dyn_array_element(cp_pixel_t,color_list,i).r;
      g+=dyn_array_element(cp_pixel_t,color_list,i).g;
      b+=dyn_array_element(cp_pixel_t,color_list,i).b;
   }

   if(length!=0)
   {
      r/=length;
      g/=length;
      b/=length;
   }

   return (cp_pixel_t){.r = r, .g = g, .b = b};
}

static cp_pixel_t pick_random_color(cp_image_t *data)
{
   return data->pix[(int)(((double)rand()/(double)RAND_MAX)*data->w*data->h)];
}

static int nearest_color_idx(cp_pixel_t color, cp_pixel_t *color_list)
{
   double dist_min = 0xfff;
   double dist = 0.0;
   int idx = 0;
   for(int i = 0;i<quant_k;i++)
   {
      dist = distance(color,color_list[i]);
      if(dist<dist_min)
      {
         dist_min = dist;
         idx = i;
      }
   }

   return idx;
}

static double distance(cp_pixel_t color0, cp_pixel_t color1)
{
   double mr = 0.5*(color0.r+color1.r),
      dr = color0.r-color1.r,
      dg = color0.g-color1.g,
      db = color0.b-color1.b;
   double distance = (2.0*dr*dr)+(4.0*dg*dg)+(3.0*db*db)+(mr*((dr*dr)-(db*db))/256.0);
   return sqrt(distance)/(3.0*255.0);
}

static double colors_variance(dyn_array *color_list)
{
   int length = color_list->used;
   cp_pixel_t mean = colors_mean(color_list);
   double dist = 0.0;
   double dist_sum = 0.0;
   for(int i = 0;i<length;i++)
   {
      dist = distance(dyn_array_element(cp_pixel_t,color_list,i),mean);
      dist_sum+=dist*dist;
   }

   return dist_sum/(double)length;
}

static void print_help(char **argv)
{
   printf("Usage: %s --img PATH [OPTIONS]\n"
          "   --img PATH     image file to process\n"
          "   --img-out PATH processed image\n"
          "   --pal PATH     palette to convert image to\n"
          "   --pal-out PATH generated palette\n"
          "   --colors NUM   targeted color amount\n",
         argv[0]);
}
//-------------------------------------
