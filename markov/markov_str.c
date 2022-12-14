/*
Markov chain string generator

Written in 2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdint.h>
#include <time.h>

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "../external/optparse.h"
//-------------------------------------

//Internal includes
#define HLH_MARKOV_ORDER_CHAR 3
#define HLH_MARKOV_ORDER_MIN_CHAR 2
#define HLH_MARKOV_ORDER_WORD 3
#define HLH_MARKOV_ORDER_MIN_WORD 2
#define HLH_MARKOV_IMPLEMENTATION
#include "HLH_markov.h"
//-------------------------------------

//#defines
#define READ_ARG(I) \
   ((++(I))<argc?argv[(I)]:NULL)
//-------------------------------------

//Typedefs
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
static void print_help(char **argv);
static char *file_read(const char *path);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   srand(time(NULL));

   //Parse cmd arguments
   struct optparse_long longopts[] =
   {
      {"in", 'i', OPTPARSE_REQUIRED},
      {"gen", 'g', OPTPARSE_REQUIRED},
      {"text", 't', OPTPARSE_NONE},
      {"word", 'w', OPTPARSE_NONE},
      {"help", 'h', OPTPARSE_NONE},
      {0},
   };
   const char *path = NULL;
   int mode = 0;
   int gen = 1;

   int option;
   struct optparse options;
   optparse_init(&options, argv);
   while((option = optparse_long(&options, longopts, NULL))!=-1)
   {
      switch(option)
      {
      case 'i':
         path = options.optarg;
         break;
      case 'g':
         gen = strtol(options.optarg,NULL,10);
         break;
      case 't':
         mode = 1;
         break;
      case 'w':
         mode = 0;
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

   if(path==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   if(mode==0)
   {
      HLH_markov_model *model_char = HLH_markov_model_new(HLH_MARKOV_CHAR);

      char *file = file_read(path);
      char *str = file;
      char *ptr = file;
      while(*ptr++!='\0')
      {
         if(*ptr=='\n')
         {
            *ptr = '\0';
            HLH_markov_model_add(model_char,str);
            ptr++;
            str = ptr;
         }
      }
      free(file);

      for(int i = 0;i<gen;i++)
      {
         char *text = HLH_markov_model_generate(model_char);
         puts(text);
         free(text);
      }
      HLH_markov_model_delete(model_char);
   }
   else if(mode==1)
   {
      HLH_markov_model *model_word = HLH_markov_model_new(HLH_MARKOV_WORD);

      char *file = file_read(path);
      char *str = file;
      char *ptr = file;
      while(*ptr++!='\0')
      {
         if(*ptr=='#')
         {
            *ptr = '\0';
            HLH_markov_model_add(model_word,str);
            ptr++;
            str = ptr;
         }
      }
      free(file);

      for(int i = 0;i<gen;i++)
      {
         char *text = HLH_markov_model_generate(model_word);
         puts(text);
         free(text);
      }
      HLH_markov_model_delete(model_word);
   }

   return 0;
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -i filename [-stats] [-gen NUM]\n"
          "   -i     file to read input from\n"
          "   -word  word generation mode\n"
          "   -text  text generation mode\n"
          "   -gen   amount of phrases to generate\n",
         argv[0],argv[0]);
}

static char *file_read(const char *path)
{
   FILE *f = fopen(path,"r");
   int size = 0;
   fseek(f,0,SEEK_END);
   size = ftell(f);
   fseek(f,0,SEEK_SET);
   char *text = malloc(sizeof(*text)*(size+1));
   fread(text,size,1,f);
   text[size] = '\0';
   fclose(f);

   return text;
}
//-------------------------------------
