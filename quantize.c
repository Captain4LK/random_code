/*
Image quantization using k-means clustering

Written in 2021 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

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

#define CUTE_PATH_IMPLEMENTATION
#include "external/cute_path.h"
#define CUTE_PNG_IMPLEMENTATION
#include "external/cute_png.h"
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
//-------------------------------------

//Function prototypes
static void cluster_list_init();
static void cluster_list_free();
static void compute_kmeans(cp_image_t *data);
static void get_cluster_centroid(cp_image_t *data);
static cp_pixel_t colors_mean(dyn_array *color_list);
static cp_pixel_t pick_random_color(cp_image_t *data);
static int nearest_color_idx(cp_pixel_t color, cp_pixel_t *color_list);
static float distance(cp_pixel_t color0, cp_pixel_t color1);
static float colors_variance(dyn_array *color_list);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   if(argc<2)
      return -1;

   cp_image_t img = cp_load_png(argv[1]);
   if(img.pix==NULL)
   {
      puts("Failed to load image");
      return 0;
   }

   compute_kmeans(&img);

   for(int i = 0;i<img.w*img.h;i++)
   {
      img.pix[i] = centroid_list[assignment[i]];
      img.pix[i].a = 255;
   }
   cp_save_png("test.png",&img);

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

static void compute_kmeans(cp_image_t *data)
{
   cluster_list_init();
   centroid_list = malloc(sizeof(*centroid_list)*quant_k);
   assignment = malloc(sizeof(*assignment)*(data->w*data->h));
   for(int i = 0;i<(data->w*data->h);i++)
      assignment[i] = -1.0f;

   int iter = 0;
   int max_iter = 16;
   float *previous_variance = malloc(sizeof(*previous_variance)*quant_k);
   float variance = 0.0f;
   float delta = 0.0f;
   float delta_max = 0.0f;
   float threshold = 0.00005f;
   for(int i = 0;i<quant_k;i++)
      previous_variance[i] = 1.0f;

   for(;;)
   {
      get_cluster_centroid(data);
      cluster_list_init();
      for(int i = 0;i<data->w*data->h;i++)
      {
         cp_pixel_t color = data->pix[i];
         assignment[i] = nearest_color_idx(color,centroid_list);
         dyn_array_add(cp_pixel_t,&cluster_list[assignment[i]],1,color);
      }

      delta_max = 0.0f;
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

   free(previous_variance);
}

static void get_cluster_centroid(cp_image_t *data)
{
   for(int i = 0;i<quant_k;i++)
   {
      if(cluster_list[i].used>0)
         centroid_list[i] = colors_mean(&cluster_list[i]);
      else
         centroid_list[i] = pick_random_color(data);
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

   r/=length;
   g/=length;
   b/=length;

   return (cp_pixel_t){.r = r, .g = g, .b = b};
}

static cp_pixel_t pick_random_color(cp_image_t *data)
{
   return data->pix[(int)(((float)rand()/(float)RAND_MAX)*data->w*data->h)];
}

static int nearest_color_idx(cp_pixel_t color, cp_pixel_t *color_list)
{
   float dist_min = 0xfff;
   float dist = 0.0f;
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

static float distance(cp_pixel_t color0, cp_pixel_t color1)
{
   float mr = 0.5f*(color0.r+color1.r),
      dr = color0.r-color1.r,
      dg = color0.g-color1.g,
      db = color0.b-color1.b;
   float distance = (2*dr*dr)+(4*dg*dg)+(3*db*db)+(mr*((dr*dr)-(db*db))/256.0f);
   return sqrt(distance)/(3.0f*255.0f);
}

static float colors_variance(dyn_array *color_list)
{
   int length = color_list->used;
   cp_pixel_t mean = colors_mean(color_list);
   float dist = 0.0f;
   float dist_sum = 0.0f;
   for(int i = 0;i<length;i++)
   {
      dist = distance(dyn_array_element(cp_pixel_t,color_list,i),mean);
      dist_sum+=dist*dist;
   }

   return dist_sum/(float)length;
}
//-------------------------------------
