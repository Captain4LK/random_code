/*
markov sentence generator

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
#define MORE_RANDOM 1
#define WEIGHTED_RANDOM 1

#define FNV_32_PRIME ((uint32_t)0x01000193)
//-------------------------------------

//Typedefs

typedef struct Context Context;
typedef struct Count Count;

typedef struct
{
   char **data;
   uint32_t data_used;
   uint32_t data_size;
}String_array;

typedef struct
{
   uint32_t *data;
   uint32_t data_used;
   uint32_t data_size;
}Uint32_array;

typedef struct
{
   Context *data;
   uint32_t data_used;
   uint32_t data_size;
}
Context_array;
typedef struct
{
   Count *data;
   uint32_t data_used;
   uint32_t data_size;
}Count_array;

typedef struct
{
   String_array words[256];
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
static void uint32_array_free(Uint32_array *array);

static void string_array_add(String_array *array, const char *str);
static uint32_t string_array_add_unique(String_array *array, const char *str);
static Context *context_find_or_create(Context_array *array, uint32_t context[ORDER], uint32_t context_size);
static Context *context_find(const Context_array *array, uint32_t context[ORDER], uint32_t context_size);
static void count_array_add(Count_array *array, uint32_t word);

static Markov_model *markov_model_create();
static void markov_model_generate(const Markov_model *model);
static uint32_t markov_model_add_word(Markov_model *model, const char *word);
static void markov_model_add_start(Markov_model *model, uint32_t start);
static const char *markov_model_get_word(const Markov_model *model, uint32_t word); 
static void markov_model_stats(const Markov_model *model);

static uint32_t fnv32a(const char *str);
static char *HLH_strtok(char *s, const char *sep, char **context); //strtok, but with a external pointer instead of a static variable for tracking internal state
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   //Parse cmd arguments
   const char *path = NULL;
   int print_stats = 0;
   int num_outputs = 1;
   for(int i = 1;i<argc;i++)
   {
      if(strcmp(argv[i],"--help")==0||
         strcmp(argv[i],"-help")==0||
         strcmp(argv[i],"-h")==0||
         strcmp(argv[i],"?")==0)
         print_help(argv);
      else if(strcmp(argv[i],"-i")==0)
         path = READ_ARG(i);
      else if(strcmp(argv[i],"-stats")==0)
         print_stats = 1;
      else if(strcmp(argv[i],"-gen")==0)
         num_outputs = atoi(READ_ARG(i));
   }

   if(path==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   srand(time(NULL));

   //Read file entry by entry
   //Entries are sepperated by a '#' symbol followed by a newline
   String_array lines = {0};
   char *file = file_read(path); //TODO: don't do this, won't work for gigantic files
   char *str = file;
   char *ptr = file;
   while(*ptr++!='\0')
   {
      if(*ptr=='#'&&*(ptr+1)=='\n')
      {
         *ptr = '\0';
         string_array_add(&lines,str);
         ptr+=2;
         str = ptr;
      }
   }
   free(file);

   //Collect words and convert sentences to integers
   Markov_model *model = markov_model_create();
   Uint32_array *sentences = malloc(sizeof(*sentences)*lines.data_used);
   memset(sentences,0,sizeof(*sentences)*lines.data_used);
   for(uint32_t i = 0;i<lines.data_used;i++)
   {
      char *str_line = malloc(sizeof(*str_line)*(strlen(lines.data[i])+1));
      strcpy(str_line,lines.data[i]);

      const char *token = " ";
      char *context = NULL;
      char *word = HLH_strtok(str_line,token,&context);
      int first = 1;
      while(word!=NULL)
      {
         uint32_t word_index = markov_model_add_word(model,word);
         uint32_array_add(&sentences[i],word_index);
         if(first)
         {
            first = 0;
            markov_model_add_start(model,word_index);
         }

         word = HLH_strtok(NULL,token,&context);
      }

      free(str_line);
   }
   free(lines.data);

   //Create markov chain
   for(uint32_t i = 0;i<lines.data_used;i++)
   {
      for(uint32_t j = 1;j<sentences[i].data_used;j++)
      {
         uint32_t context[ORDER] = {0};
         uint32_t event = sentences[i].data[j];
         
         uint8_t xor = 0;
         for(int m = 1;m<=ORDER;m++)
         {
            if((int)j-m<0)
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

      uint32_array_free(&sentences[i]);
   }
   free(sentences);

   if(print_stats)
      markov_model_stats(model);

   //Generate sentences
   for(int i = 0;i<num_outputs;i++)
   {
      markov_model_generate(model);
      puts("-------------------------------------------------------------------------------------");
   }

   return 0;
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -i filename [-stats] [-gen NUM]\n"
          "   -i     file to read input from\n"
          "   -stats print markov model stats\n"
          "   -gen   number of outputs to generate\n",
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

static void uint32_array_free(Uint32_array *array)
{
   if(array==NULL||array->data==NULL)
      return;

   free(array->data);
   array->data = NULL;
   array->data_used = 0;
   array->data_size = 0;
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

static Markov_model *markov_model_create()
{
   Markov_model *model = malloc(sizeof(*model));
   memset(model,0,sizeof(*model));
   return model;
}

static uint32_t markov_model_add_word(Markov_model *model, const char *word)
{
   uint8_t index = fnv32a(word)&255;
   return string_array_add_unique(&model->words[index],word)|(index<<24);
}

static void markov_model_add_start(Markov_model *model, uint32_t start)
{
   uint32_array_add(&model->starting_words,start);
}

static const char *markov_model_get_word(const Markov_model *model, uint32_t word)
{
   uint8_t index = (word>>24)&255;
   word^=(index<<24);
   return model->words[index].data[word];
}

static void markov_model_stats(const Markov_model *model)
{
   int num_words = 0;
   for(int i = 0;i<256;i++)
      num_words+=model->words[i].data_used;
   printf("%d unique words\n",num_words);
   printf("%d start words\n",model->starting_words.data_used);
   int num_contexts = 0;
   for(int i = 0;i<256;i++)
      num_contexts+=model->contexts[i].data_used;
   printf("%d contexts captured\n",num_contexts);

   printf("Word hashmap usage:\n");
   for(int i = 0;i<256;i++)
      printf("%d,",model->words[i].data_used);
   puts("");
   printf("Context hashmap usage:\n");
   for(int i = 0;i<256;i++)
      printf("%d,",model->contexts[i].data_used);
   puts("");
}

static void markov_model_generate(const Markov_model *model)
{
   Uint32_array sentence = {0};
   uint32_array_add(&sentence,model->starting_words.data[rand()%model->starting_words.data_used]);

   int done = 0;
   while(!done)
   {
      int start = sentence.data_used;
#if MORE_RANDOM
      int backoff = (rand()%ORDER)+1;
#else
      int backoff = ORDER;
#endif
      if(start-backoff<0)
         backoff = start;
      start--;

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
            break;
      }

      if(model_context!=NULL)
      {
#if WEIGHTED_RANDOM
         uint32_t num = rand()%model_context->total;
         uint32_t cur = 0;
         for(int i = 0;i<model_context->counts.data_used;i++)
         {
            cur+=model_context->counts.data[i].count;
            if(cur>=num)
            {
               uint32_array_add(&sentence,model_context->counts.data[i].word);
               break;
            }
         }
#else
         uint32_array_add(&sentence,model_context->counts.data[rand()%model_context->counts.data_used].word);
#endif
      }
      else
      {
         done = 1;
      }
   }

   for(int i = 0;i<sentence.data_used;i++)
      printf("%s ",markov_model_get_word(model,sentence.data[i]));
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

static uint32_t fnv32a(const char *str)
{
   uint32_t hval = 0x811c9dc5;
   unsigned char *s = (unsigned char *)str;
   while(*s) 
   {
      hval^=(uint32_t)*s++;
      hval *= FNV_32_PRIME;
   }

   return hval;
}
//-------------------------------------
