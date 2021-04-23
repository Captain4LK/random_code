/*
Image Steganography

Written in 2021 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

#define CUTE_PATH_IMPLEMENTATION
#include "external/cute_path.h"
#define CUTE_PNG_IMPLEMENTATION
#include "external/cute_png.h"
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
#define MAGIC_NUM ('H' + ('L'<<8) + ('H'<<16))
//-------------------------------------

//Typedefs
typedef struct
{
   uint32_t magic:24;
   uint32_t name:8;
   uint32_t size;
}Header;
//-------------------------------------

//Variables
static cp_image_t img;
static char *buffer = NULL;
static int bits = 1;
//-------------------------------------

//Function prototypes
static void write_u8(uint8_t byte);
static void write_bit(uint8_t bit);
static uint8_t read_u8();
static uint8_t read_bit();
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   //Parse cmd arguments
   const char *path = NULL;
   const char *image_path = NULL;
   int mode = 0;
   for(int i = 1;i<argc;i++)
   {
      if(argv[i][0]=='-')
      {
         switch(argv[i][1])
         {
         case 'h': puts("image steganography\nAvailible commandline options:\n\t-h\t\tprint this text\n\t-e\t\tencrypt mode\n\t-d\t\tdecrypt mode\n\t-b [BITS]\tamount of bits to use\n\t-f [PATH]\tinput file\n\t-i [PATH]\tinput image"); break;
         case 'e': mode = 0; break;
         case 'd': mode = 1; break;
         case 'b': bits = atoi(argv[++i]); break;
         case 'f': path = argv[++i]; break;
         case 'i': image_path = argv[++i]; break;
         }
      }
   }

   //Parse input file
   //Removes all invalid characters
   if(image_path==NULL)
   {
      printf("No image file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   //Encrypt
   if(mode==0)
   {
      //Read data to memory and check
      img = cp_load_png(image_path);
      if(img.pix==NULL)
      {
         puts("Failed to load image");
         return 0;
      }
      FILE *f = fopen(path,"rb");
      int size = 0;
      fseek(f,0,SEEK_END);
      size = ftell(f);
      fseek(f,0,SEEK_SET);
      buffer = malloc(size+1);
      fread(buffer,size,1,f);
      buffer[size] = 0;
      fclose(f);

      //Create header and check image size
      char name[256];
      path_concat("f",path,name,255);
      path_pop(name,NULL,name);
      if((size+8+strlen(name))*8>img.w*img.h*bits*3)
      {
         puts("Not enough space for data in image");
         return 0;
      }
      Header h;
      h.magic = MAGIC_NUM;
      h.name = strlen(name);
      h.size = size;

      //Write header
      write_u8(h.magic);
      write_u8(h.magic>>8);
      write_u8(h.magic>>16);
      write_u8(h.name);
      for(int i = 0;i<h.name;i++)
         write_u8(name[i]);
      write_u8(h.size);
      write_u8(h.size>>8);
      write_u8(h.size>>16);
      write_u8(h.size>>24);

      //Write data
      for(int i = 0;i<h.size;i++)
         write_u8(buffer[i]); 

      cp_save_png("out.png",&img);
      free(img.pix);
      free(buffer);
   }
   //Decrypt
   else
   {
      //Read image to memory
      img = cp_load_png(image_path);
      if(img.pix==NULL)
      {
         puts("Failed to load image");
         return 0;
      }

      //Read header
      Header h = {0};
      h.magic = read_u8();
      h.magic+=read_u8()<<8;
      h.magic+=read_u8()<<16;
      if(h.magic!=MAGIC_NUM)
      {
         puts("Magic num is not matching");
         return 0;
      }
      char name[256];
      h.name = read_u8();
      for(int i = 0;i<h.name;i++)
         name[i] = read_u8();
      name[h.name] = 0;
      printf("Original name: %s\n",name);
      h.size = read_u8();
      h.size+=read_u8()<<8;
      h.size+=read_u8()<<16;
      h.size+=read_u8()<<24;

      //Read data and write to file
      FILE *f = fopen("out.txt","w");
      for(int i = 0;i<h.size;i++)
         putc(read_u8(),f);
      fclose(f);
      
      free(img.pix);
   }

   return 0;
}

static void write_u8(uint8_t byte)
{
   write_bit((byte&1)>>0);
   write_bit((byte&2)>>1);
   write_bit((byte&4)>>2);
   write_bit((byte&8)>>3);
   write_bit((byte&16)>>4);
   write_bit((byte&32)>>5);
   write_bit((byte&64)>>6);
   write_bit((byte&128)>>7);
}

static void write_bit(uint8_t bit)
{
   static int byte = 0;
   static int component = 0;
   static int bits_used = 0;

   //Select byte to write to
   uint8_t *dst = NULL;
   switch(component)
   {
   case 0: dst = &img.pix[byte].r; break;
   case 1: dst = &img.pix[byte].g; break;
   case 2: dst = &img.pix[byte].b; break;
   }

   //Write bit
   switch(bits_used)
   {
   case 0: *dst = ((*dst)&0xfe)|(bit<<0); break;
   case 1: *dst = ((*dst)&0xfd)|(bit<<1); break;
   case 2: *dst = ((*dst)&0xfb)|(bit<<2); break;
   case 3: *dst = ((*dst)&0xf7)|(bit<<3); break;
   case 4: *dst = ((*dst)&0xef)|(bit<<4); break;
   case 5: *dst = ((*dst)&0xdf)|(bit<<5); break;
   case 6: *dst = ((*dst)&0xbf)|(bit<<6); break;
   case 7: *dst = ((*dst)&0x7f)|(bit<<7); break;
   }

   bits_used++;
   if(bits_used==bits)
   {
      bits_used = 0;
      component++;
      if(component==3)
      {
         component = 0;
         byte++;
      }
   }
}

static uint8_t read_u8()
{
   uint8_t byte = 0;
   byte+=read_bit()<<0;
   byte+=read_bit()<<1;
   byte+=read_bit()<<2;
   byte+=read_bit()<<3;
   byte+=read_bit()<<4;
   byte+=read_bit()<<5;
   byte+=read_bit()<<6;
   byte+=read_bit()<<7;

   return byte;
}

static uint8_t read_bit()
{
   static int byte = 0;
   static int component = 0;
   static int bits_used = 0;

   //Select byte to read from
   uint8_t *dst = NULL;
   switch(component)
   {
   case 0: dst = &img.pix[byte].r; break;
   case 1: dst = &img.pix[byte].g; break;
   case 2: dst = &img.pix[byte].b; break;
   }

   //Read bit
   uint8_t bit = 0;
   switch(bits_used)
   {
   case 0: bit = ((*dst)&1)>>0; break;
   case 1: bit = ((*dst)&2)>>1; break;
   case 2: bit = ((*dst)&4)>>2; break;
   case 3: bit = ((*dst)&8)>>3; break;
   case 4: bit = ((*dst)&16)>>4; break;
   case 5: bit = ((*dst)&32)>>5; break;
   case 6: bit = ((*dst)&64)>>6; break;
   case 7: bit = ((*dst)&128)>>7; break;
   }

   bits_used++;
   if(bits_used==bits)
   {
      bits_used = 0;
      component++;
      if(component==3)
      {
         component = 0;
         byte++;
      }
   }

   return bit;
}
//-------------------------------------
