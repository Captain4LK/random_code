/*
QOI implementation

Written in 2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <inttypes.h>
#include <time.h>

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

#define COLOR_EQ(c0,c1) \
   ((c0).r==(c1).r&&(c0).g==(c1).g&&(c0).b==(c1).b&&(c0).a==(c1).a)
//-------------------------------------

//Typedefs
typedef struct
{
   uint8_t r,g,b,a;
}QOI_color;

typedef struct
{
   uint32_t width;
   uint32_t height;
   uint8_t channels;
   uint8_t colorspace;

   QOI_color data[];
}QOI_image;

typedef struct
{
   FILE *stream;
   uint32_t buffer;
   uint32_t count;
}QOI_bitbuffer;
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
static void encode(const char *path_in, const char *path_out);

static void QOI_encode(const QOI_image *img, FILE *out);

static uint32_t QOI_endian_swap_u32(uint32_t n);

static void QOI_bitbuffer_init(QOI_bitbuffer *buf, FILE *stream);
static void QOI_bitbuffer_put(QOI_bitbuffer *buf, uint32_t n, uint32_t x);
static void QOI_bitbuffer_flush(QOI_bitbuffer *buf);
static uint32_t QOI_bitbuffer_get(QOI_bitbuffer *buf, uint32_t n);

static void print_help(char **argv);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   //Parse cmd arguments
   const char *path_in = NULL;
   const char *path_out = NULL;
   int mode = 0;
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
      else if(strcmp(argv[i],"-encode")==0)
         mode = 0;
      else if(strcmp(argv[i],"-decode")==0)
         mode = 1;
   }

   if(path_in==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   if(path_out==NULL)
   {
      printf("No output path specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   if(mode==0)
   {
      encode(path_in,path_out);
   }
   else
   {
   }

   return 0;
}

static void encode(const char *path_in, const char *path_out)
{
   int w = 0;
   int h = 0;
   int channels = 0;
   uint8_t *data = stbi_load(path_in,&w,&h,&channels,4);

   QOI_image *img = malloc(sizeof(*img)+w*h*sizeof(*img->data));
   img->width = w;
   img->height = h;
   img->channels = 4;
   img->colorspace = 0;
   memcpy(img->data,data,w*h*sizeof(*img->data));

   free(data);

   FILE *out = fopen(path_out,"wb");
   QOI_encode(img,out);

   fclose(out);
   free(img);
}

static void QOI_encode(const QOI_image *img, FILE *out)
{
   //Write header
   fprintf(out,"qoif");
   uint32_t ew = QOI_endian_swap_u32(img->width); fwrite(&ew,4,1,out);
   uint32_t eh = QOI_endian_swap_u32(img->height); fwrite(&eh,4,1,out);
   fwrite(&img->channels,1,1,out);
   fwrite(&img->colorspace,1,1,out);

   //Encoding
   QOI_bitbuffer bits;
   QOI_bitbuffer_init(&bits,out);
   QOI_color prev = {.r = 0, .g = 0, .b = 0, .a = 255};
   QOI_color prev_hash[64] = {0};

   QOI_color runc = prev;
   int run = 0;

   for(int i = 0;i<img->width*img->height;i++)
   {
      QOI_color cur = img->data[i];
      uint8_t hash = (cur.r*3+cur.g*5+cur.b*7+cur.a*11)%64;

      //QOI_OP_RUN
      if(run>0&&((!COLOR_EQ(cur,runc))||run==62))
      {
         QOI_bitbuffer_put(&bits,6,run-1);
         QOI_bitbuffer_put(&bits,2,3);
         run = 0;
      }

      if(COLOR_EQ(cur,runc))
      {
         run++;

         //Cry about it
         goto next;
      }
      else
      {
         runc = cur;
         run = 0;
      }
      //-------------------------------------

      //QOI_OP_INDEX
      if(COLOR_EQ(cur,prev_hash[hash]))
      {
         QOI_bitbuffer_put(&bits,6,hash);
         QOI_bitbuffer_put(&bits,2,0);

         goto next;
      }
      //-------------------------------------

      if(cur.a==prev.a)
      {
         //QOI_OP_DIFF
         int8_t dr = cur.r-prev.r;
         int8_t dg = cur.g-prev.g;
         int8_t db = cur.b-prev.b;
         if(dr>=-2&&dr<=1&&
            dg>=-2&&dg<=1&&
            db>=-2&&db<=1)
         {
            QOI_bitbuffer_put(&bits,2,db+2);
            QOI_bitbuffer_put(&bits,2,dg+2);
            QOI_bitbuffer_put(&bits,2,dr+2);
            QOI_bitbuffer_put(&bits,2,1);

            goto next;
         }
         //-------------------------------------

         //QOI_OP_LUMA
         int8_t dr_dg = dr-dg;
         int8_t db_dg = db-dg;
         if(dr_dg>=-8&&dr_dg<=7&&
            db_dg>=-8&&db_dg<=7&&
            dg>=-32&&dg<=31)
         {
            QOI_bitbuffer_put(&bits,6,dg+32);
            QOI_bitbuffer_put(&bits,2,2);
            QOI_bitbuffer_put(&bits,4,db_dg+8);
            QOI_bitbuffer_put(&bits,4,dr_dg+8);
            
            goto next;
         }
         //-------------------------------------

         //QOI_OP_RGB
         QOI_bitbuffer_put(&bits,8,254);
         QOI_bitbuffer_put(&bits,8,cur.r);
         QOI_bitbuffer_put(&bits,8,cur.g);
         QOI_bitbuffer_put(&bits,8,cur.b);
            
         goto next;
         //-------------------------------------
      }


      //QOI_OP_RGBA
      QOI_bitbuffer_put(&bits,8,255);
      QOI_bitbuffer_put(&bits,8,cur.r);
      QOI_bitbuffer_put(&bits,8,cur.g);
      QOI_bitbuffer_put(&bits,8,cur.b);
      QOI_bitbuffer_put(&bits,8,cur.a);
      //-------------------------------------

next:
      prev_hash[hash] = cur;
      prev = cur;
   }

   //Terminate run
   if(run>0)
   {
      QOI_bitbuffer_put(&bits,6,run-1);
      QOI_bitbuffer_put(&bits,2,3);
   }

   //Footer
   QOI_bitbuffer_flush(&bits);
   QOI_bitbuffer_put(&bits,8,0);
   QOI_bitbuffer_put(&bits,8,0);
   QOI_bitbuffer_put(&bits,8,0);
   QOI_bitbuffer_put(&bits,8,0);
   QOI_bitbuffer_put(&bits,8,0);
   QOI_bitbuffer_put(&bits,8,0);
   QOI_bitbuffer_put(&bits,8,0);
   QOI_bitbuffer_put(&bits,8,1);
}

static uint32_t QOI_endian_swap_u32(uint32_t n)
{
   return (n>>24)|(n<<24)|((n>>8)&0xff00)|((n&0xff00)<<8);
}

static void QOI_bitbuffer_init(QOI_bitbuffer *buf, FILE *stream)
{
   buf->buffer = 0;
   buf->count = 0;
   buf->stream = stream;
}

static void QOI_bitbuffer_put(QOI_bitbuffer *buf, uint32_t n, uint32_t x)
{
   buf->buffer|=x<<buf->count;
   buf->count+=n;

   //Write to stream
   while(buf->count>=8)
   {
      uint8_t val = buf->buffer&255;
      fwrite(&val,1,1,buf->stream);
      buf->buffer>>=8;
      buf->count-=8;
   }
}

static void QOI_bitbuffer_flush(QOI_bitbuffer *buf)
{
   QOI_bitbuffer_put(buf,7,0);
   buf->count = 0;
   buf->buffer = 0;
}

static uint32_t QOI_bitbuffer_get(QOI_bitbuffer *buf, uint32_t n)
{
   //Fill buffer
   while(buf->count<n)
   {
      uint8_t val = 0;
      fread(&val,1,1,buf->stream);
      buf->buffer|=val<<buf->count;
      buf->count+=8;
   }

   uint32_t x = buf->buffer&((1<<n)-1);
   buf->buffer>>=n;
   buf->count-=n;

   return x;
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -fin filename -fout filename (-encode|-decode) \n"
          "   -fin     image file to process\n"
          "   -fout    encoded/decoded image file\n"
          "   -encode  encode image\n"
          "   -decode  decode image\n",
         argv[0],argv[0]);
}
//-------------------------------------
