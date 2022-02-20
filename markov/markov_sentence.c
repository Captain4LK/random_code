/*
markov sentence

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

#define ORDER 3
//-------------------------------------

//Typedefs

typedef struct Context Context;
typedef struct Count Count;

typedef struct
{
   char **data;
   unsigned data_used;
   unsigned data_size;
}String_array;

typedef struct
{
   uint32_t *data;
   unsigned data_used;
   unsigned data_size;
}Uint32_array;

typedef struct
{
   Context *data;
   unsigned data_used;
   unsigned data_size;
}
Context_array;
typedef struct
{
   Count *data;
   unsigned data_used;
   unsigned data_size;
}Count_array;

typedef struct
{
   String_array words;
   Uint32_array starting_words;
   Context_array contexts[256];
}Markov_model;

struct Count
{
   uint32_t count;
   uint32_t word;
};

struct Context
{
   uint32_t context[ORDER];
   uint32_t context_size;
   uint32_t total;
   Count_array counts;
};
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
static void print_help(char **argv);
static char *file_read(const char *path);

static void uint32_array_add(Uint32_array *array, uint32_t num);
static void string_array_add(String_array *array, const char *str);
static void string_array_add_words(String_array *array, const char *str);
static uint32_t string_array_add_unique(String_array *array, const char *str);
static Context *context_find_or_create(Context_array *array, uint32_t context[ORDER], uint32_t context_size);
static Context *context_find(const Context_array *array, uint32_t context[ORDER], uint32_t context_size);
static void count_array_add(Count_array *array, uint32_t word);

static Markov_model *markov_model_create(String_array *words, Uint32_array *starting_words);
static void markov_model_generate(const Markov_model *model);

static uint8_t xor_string(const char *str);

static char *HLH_strtok(char *s, const char *sep, char **context);
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
   char *file = file_read(path); //TODO: don't do this, won't work for gigantic files
   const char *token = "\n"; 
   char *context = NULL;
   char *line = HLH_strtok(file,token,&context);
   while(line!=NULL)
   {
      string_array_add(&lines,line);

      line = HLH_strtok(NULL,token,&context);
   }
   free(file);

   //Collect words and convert sentences to integers
   String_array words = {0};
   Uint32_array starting_words = {0};
   Uint32_array *sentences = malloc(sizeof(*sentences)*lines.data_used);
   memset(sentences,0,sizeof(*sentences)*lines.data_used);
   for(int i = 0;i<lines.data_used;i++)
   {
      char *str_line = malloc(sizeof(*str_line)*(strlen(lines.data[i])+1));
      strcpy(str_line,lines.data[i]);

      const char *token = " ";
      char *context = NULL;
      char *word = HLH_strtok(str_line,token,&context);
      int first = 1;
      while(word!=NULL)
      {
         uint32_t word_index = string_array_add_unique(&words,word);
         uint32_array_add(&sentences[i],word_index);
         if(first)
         {
            first = 0;
            uint32_array_add(&starting_words,word_index);
         }

         word = HLH_strtok(NULL,token,&context);
      }

      free(str_line);
   }
   
   printf("Finished parsing '%s'\n%d inputs\n%d unique words\n",path,lines.data_used,words.data_used);
   free(lines.data);

   //Create markov chain
   Markov_model *model = markov_model_create(&words,&starting_words);
   for(int i = 0;i<lines.data_used;i++)
   {
      for(int j = 1;j<sentences[i].data_used;j++)
      {
         uint32_t context[ORDER] = {0};
         uint32_t event = sentences[i].data[j];
         
         uint8_t xor = 0;
         for(int m = 1;m<=ORDER;m++)
         {
            if(j-m<0)
               break;

            context[m-1] = sentences[i].data[j-m];
            xor^=context[m-1]&255;
            xor^=(context[m-1]>>8)&255;
            xor^=(context[m-1]>>16)&255;
            xor^=(context[m-1]>>24)&255;
            Context *model_context = context_find_or_create(&model->contexts[xor],context,m);
            model_context->total++;
            count_array_add(&model_context->counts,event);
         }
      }
   }

   for(int i = 0;i<32;i++)
      markov_model_generate(model);

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

static void uint32_array_add(Uint32_array *array, uint32_t num)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = malloc(sizeof(*array->data)*array->data_size);
   }

   array->data[array->data_used++] = num;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = realloc(array->data,sizeof(*array->data)*array->data_size);
   }
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

static void string_array_add_words(String_array *array, const char *str)
{
   char *str_line = malloc(sizeof(*str_line)*(strlen(str)+1));
   strcpy(str_line,str);

   const char *token = " ";
   char *context = NULL;
   char *word = HLH_strtok(str_line,token,&context);
   while(word!=NULL)
   {
      string_array_add_unique(array,word);

      word = HLH_strtok(NULL,token,&context);
   }

   free(str_line);
}

static uint32_t string_array_add_unique(String_array *array, const char *str)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = malloc(sizeof(*array->data)*array->data_size);
   }

   for(int i = 0;i<array->data_used;i++)
      if(strcmp(array->data[i],str)==0)
         return i;

   int len = strlen(str)+1;
   array->data[array->data_used] = malloc(sizeof(*array->data[array->data_used])*len);
   strcpy(array->data[array->data_used],str);
   array->data_used++;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = realloc(array->data,sizeof(*array->data)*array->data_size);
   }
   
   return array->data_used-1;
}
static Context *context_find_or_create(Context_array *array, uint32_t context[ORDER], uint32_t context_size)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = malloc(sizeof(*array->data)*array->data_size);
   }

   for(int i = 0;i<array->data_used;i++)
   {
      if(array->data[i].context_size!=context_size)
         continue;
      if(memcmp(array->data[i].context,context,sizeof(context[0])*context_size)==0)
         return &array->data[i];
   }

   memcpy(array->data[array->data_used].context,context,sizeof(context[0])*context_size);
   array->data[array->data_used].context_size = context_size;
   array->data[array->data_used].total = 0;
   memset(&array->data[array->data_used].counts,0,sizeof(array->data[array->data_used].counts));
   array->data_used++;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = realloc(array->data,sizeof(*array->data)*array->data_size);
   }
   
   return &array->data[array->data_used-1];
}

static Context *context_find(const Context_array *array, uint32_t context[ORDER], uint32_t context_size)
{
   if(array->data==NULL)
      return NULL;

   for(int i = 0;i<array->data_used;i++)
   {
      if(array->data[i].context_size!=context_size)
         continue;
      if(memcmp(array->data[i].context,context,sizeof(context[0])*context_size)==0)
         return &array->data[i];
   }

   return NULL;
}

static void count_array_add(Count_array *array, uint32_t word)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = malloc(sizeof(*array->data)*array->data_size);
   }

   for(int i = 0;i<array->data_used;i++)
   {
      if(array->data[i].word==word)
      {
         array->data[i].count++;
         return;
      }
   }

   array->data[array->data_used].word = word;
   array->data[array->data_used].count = 1;
   array->data_used++;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = realloc(array->data,sizeof(*array->data)*array->data_size);
   }
}

static uint8_t xor_string(const char *str)
{
   uint8_t result = 0;
   for(;*str!='\0';str++)
      result^=(uint8_t)*str;

   return result;
}

static Markov_model *markov_model_create(String_array *words, Uint32_array *starting_words)
{
   Markov_model *model = malloc(sizeof(*model));
   memset(model,0,sizeof(*model));
   model->words = *words;
   model->starting_words = *starting_words;
   return model;
}

static void markov_model_generate(const Markov_model *model)
{
   Uint32_array sentence = {0};
   uint32_array_add(&sentence,model->starting_words.data[rand()%model->starting_words.data_used]);

   int done = 0;
   while(!done)
   {
      int start = sentence.data_used;
      int backoff = ORDER;
      if(start-backoff<0)
         backoff = start;

      uint32_t context[ORDER] = {0};
      for(int i = 0;i<backoff;i++)
         context[i] = sentence.data[start-i];
      
      Context *model_context = NULL;
      for(int i = backoff;i>0;i--)
      {
         uint8_t xor = 0;
         for(int j = 0;j<i;j++)
         {
            uint32_t word = sentence.data[start-j];

            xor^=word&255;
            xor^=(word>>8)&255;
            xor^=(word>>16)&255;
            xor^=(word>>24)&255;
         }

         model_context = context_find(&model->contexts[xor],context,i);
         if(model_context!=NULL)
         {
            printf("Order used: %d\n",i);
            break;
         }
      }

      if(model_context!=NULL)
      {
         uint32_array_add(&sentence,model_context->counts.data[rand()%model_context->counts.data_used].word);
      }
      else
      {
         done = 1;
      }
   }

   for(int i = 0;i<sentence.data_used;i++)
      printf("%s ",model->words.data[sentence.data[i]]);
   puts("");
}

static char *HLH_strtok(char *s, const char *sep, char **context)
{
   char *p;

   if(s==NULL)
      s = *context;

   while(*s&&strchr(sep,*s)!=NULL)
      s++;

   if(!*s)
      return NULL;

   for(p = s;*s&&!strchr(sep,*s);s++);

   if(*s&&s[1])
   {
      *(s++) = 0;
   }

   *context = s;

   return p;
}
//-------------------------------------
