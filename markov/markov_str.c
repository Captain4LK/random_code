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
   const char *path = NULL;
   int mode = 0;
   int gen = 1;
   for(int i = 1;i<argc;i++)
   {
      if(strcmp(argv[i],"--help")==0||
         strcmp(argv[i],"-help")==0||
         strcmp(argv[i],"-h")==0||
         strcmp(argv[i],"?")==0)
         print_help(argv);
      else if(strcmp(argv[i],"-i")==0)
         path = READ_ARG(i);
      else if(strcmp(argv[i],"-word")==0)
         mode = 0;
      else if(strcmp(argv[i],"-text")==0)
         mode = 1;
      else if(strcmp(argv[i],"-gen")==0)
         gen = atoi(READ_ARG(i));
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
         if(*ptr=='\n')
         {
            *ptr = '\0';
            HLH_markov_model_add(model_word,str);
            ptr++;
            str = ptr;
         }
      }
      free(file);

      for(int i = 0;i<1;i++)
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
          "   -text  text generation mode\n",
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
