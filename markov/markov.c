/*
markov sentence/word generator

Written in 2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
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
   char **data;
   unsigned data_used;
   unsigned data_size;
}String_array;
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
static void print_help(char **argv);
static char *file_read(const char *path);

static void string_array_add(String_array *array, const char *str);
static void string_array_add_unique(String_array *array, const char *str);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   //Parse cmd arguments
   const char *path = NULL;
   for(int i = 1;i<argc;i++)
   {
      if(strcmp(argv[i],"--help")==0||
         strcmp(argv[i],"-help")==0||
         strcmp(argv[i],"-h")==0||
         strcmp(argv[i],"?")==0)
         print_help(argv);
      else if(strcmp(argv[i],"-i")==0)
         path = READ_ARG(i);
   }

   if(path==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   srand(time(NULL));

   //Read file line by line
   String_array lines = {0};
   String_array support = {0};
   char *file = file_read(path); //TODO: don't do this, won't work for gigantic files
   const char *token = "\n"; 
   char *line = strtok(file,token);
   while(line!=NULL)
   {
      string_array_add(&lines,line);
      string_array_add_unique(&support,line);

      line = strtok(NULL,token);
   }
   free(file);

   //Create model


   return 0;
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -i filename\n"
          "   -i    file to read input from\n",
         argv[0],argv[0]);
}

static char *file_read(const char *path)
{
   FILE *f = fopen(path,"r");
   int32_t size = 0;
   fseek(f,0,SEEK_END);
   size = ftell(f);
   fseek(f,0,SEEK_SET);
   char *text = malloc(size+1);
   fread(text,size,1,f);
   text[size] = 0;
   fclose(f);

   return text;
}

static void string_array_add(String_array *array, const char *str)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = malloc(sizeof(*array->data)*array->data_size);
   }

   int len = strlen(str)+1;
   array->data[array->data_used] = malloc(sizeof(*array->data[array->data_used])*len);
   strcpy(array->data[array->data_used],str);
   array->data_used++;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = realloc(array->data,sizeof(*array->data)*array->data_size);
   }
}

static void string_array_add_unique(String_array *array, const char *str)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = malloc(sizeof(*array->data)*array->data_size);
   }

   for(int i = 0;i<array->data_used;i++)
      if(strcmp(array->data[i],str)==0)
         return;

   int len = strlen(str)+1;
   array->data[array->data_used] = malloc(sizeof(*array->data[array->data_used])*len);
   strcpy(array->data[array->data_used],str);
   array->data_used++;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = realloc(array->data,sizeof(*array->data)*array->data_size);
   }
}
//-------------------------------------
