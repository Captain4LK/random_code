/*
Markov chain sentence generator

Written in 2021 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines

//Universal dynamic array
#define dyn_array_init(type, array, space) \
   do { ((dyn_array *)(array))->size = (space); ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->data = malloc(sizeof(type)*(((dyn_array *)(array))->size)); } while(0)

#define dyn_array_free(type, array) \
   do { if(((dyn_array *)(array))->data) { free(((dyn_array *)(array))->data); ((dyn_array *)(array))->data = NULL; ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->size = 0; }} while(0)

#define dyn_array_add(type, array, grow, element) \
   do { ((type *)((dyn_array *)(array)->data))[((dyn_array *)(array))->used] = (element); ((dyn_array *)(array))->used++; if(((dyn_array *)(array))->used==((dyn_array *)(array))->size) { ((dyn_array *)(array))->size+=grow; ((dyn_array *)(array))->data = realloc(((dyn_array *)(array))->data,sizeof(type)*(((dyn_array *)(array))->size)); } } while(0)

#define dyn_array_element(type, array, index) \
   (((type *)((dyn_array *)(array)->data))[index])

#define EXPAND 128
#define SIZE 128

#define READ_ARG(I) \
   ((++(I))<argc?argv[(I)]:NULL)
//-------------------------------------

//Typedefs
typedef struct
{
   unsigned used;
   unsigned size;
   void *data;
}dyn_array;

typedef struct
{
   dyn_array prefix;
   dyn_array suffix;
}Markov_entry;

typedef struct
{
   int32_t entry;
   uint32_t weight;
}Suffix;
//-------------------------------------

//Variables
dyn_array sentences = {0};
dyn_array words = {0};
dyn_array markov_chain = {0};
//-------------------------------------

//Function prototypes
static void print_help(char **argv);
static void split_sentences(char *input);
static void dump_model(const char *path);
static void generate_chain(int state_size);
static int add_word(const char *word);
static void add_entry(dyn_array prefix,int32_t suffix);
static int cmp(const void *a, const void *b);
static void generate_sentence(int state_size);
static int find_prefix(dyn_array prefix);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   //Parse cmd arguments
   const char *path = NULL;
   const char *path_out = NULL;

   for(int i = 1;i<argc;i++)
   {
      if(strcmp(argv[i],"--help")==0||
         strcmp(argv[i],"-help")==0||
         strcmp(argv[i],"-h")==0||
         strcmp(argv[i],"?")==0)
         print_help(argv);
      else if(strcmp(argv[i],"-i")==0)
         path = READ_ARG(i);
      else if(strcmp(argv[i],"-o")==0)
         path_out = READ_ARG(i);
   }

   if(path==NULL)
   {
      printf("No input file specified, try %s --help\n",argv[0]);
      return -1;
   }

   srand(time(NULL));

   FILE *f = fopen(path,"rb");

   int32_t size = 0;
   fseek(f,0,SEEK_END);
   size = ftell(f);
   fseek(f,0,SEEK_SET);

   char *input = malloc(size+1);
   fread(input,size,1,f);
   input[size] = 0;

   fclose(f);

   split_sentences(input);
   free(input);

   generate_chain(2);

   dump_model("out.bin");

   for(int i = 0;i<50;i++)
      generate_sentence(2);

   return 0;
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -i filename [-o filename]\n"
          "   -i\tfile to generate chain from\n"
          "   -o\tfile to write output to (stdout if not provided)\n",
         argv[0],argv[0]);
}

static void split_sentences(char *input)
{
   const char *token = " \n";
   const char *punctuation = ".!?";
   int current = 0;

   //Should probably use a linked list, but who cares...
   dyn_array_init(dyn_array,&sentences,SIZE);
   dyn_array sentence;
   dyn_array_init(char *,&sentence,SIZE);
   dyn_array_add(dyn_array,&sentences,EXPAND,sentence);

   char *word = strtok(input,token);
   while(word!=NULL)
   {
      char *nword = malloc(strlen(word)+1);
      strcpy(nword,word);
      dyn_array_add(char *,&dyn_array_element(dyn_array,&sentences,current),EXPAND,nword);

      if(strchr(punctuation,word[strlen(word)-1])!=NULL)
      {
         //TODO: check if abbreviation
         current++;
         sentence.used = 0;
         sentence.size = 0;
         sentence.data = NULL;
         dyn_array_init(char *,&sentence,SIZE);
         dyn_array_add(dyn_array,&sentences,EXPAND,sentence);
      }

      word = strtok(NULL,token);
   }
}

static void dump_model(const char *path)
{
   FILE *f = fopen(path,"wb");

   fclose(f);
}

static void generate_chain(int state_size)
{
   dyn_array_init(Markov_entry,&markov_chain,SIZE);

   for(int i = 0;i<sentences.used;i++)
   {
      if(dyn_array_element(dyn_array,&sentences,i).used<state_size)
      {
         printf("Warning: sentence is shorter than state size\n");
         continue;
      }

      int max = dyn_array_element(dyn_array,&sentences,i).used;
      for(int j = 0;j<max;j++)
      {
         int32_t pos = 0;
         dyn_array prefix;
         dyn_array_init(uint32_t,&prefix,state_size);

         if(j+state_size>max)
            continue;
         for(int s = j;s<j+state_size;s++)
         {
            char *word = dyn_array_element(char *,&dyn_array_element(dyn_array,&sentences,i),s);
            pos = add_word(word);
            dyn_array_add(uint32_t,&prefix,EXPAND,pos);
         }

         char *suffix = NULL;
         pos = -1;
         if(j+state_size<max)
         {
            suffix = dyn_array_element(char *,&dyn_array_element(dyn_array,&sentences,i),j+state_size);
            pos = add_word(suffix);
         }

         //Add to markov chain
         add_entry(prefix,pos);

         dyn_array_free(uint32_t,&prefix);
      }
   }
}

static int add_word(const char *word)
{
   if(words.size==0)
      dyn_array_init(char *,&words,SIZE);

   //Check if in array
   for(int i = 0;i<words.used;i++)
   {
      if(strcmp(word,dyn_array_element(char *,&words,i))==0)
         return i;
   }

   //Add to array
   char *nword = malloc(strlen(word)+1);
   strcpy(nword,word);
   dyn_array_add(char *,&words,EXPAND,nword);

   return words.used-1;
}

static void add_entry(dyn_array prefix,int32_t suffix)
{
   for(int i = 0;i<markov_chain.used;i++)
   {
      Markov_entry *entry = &dyn_array_element(Markov_entry,&markov_chain,i);
      int found = 1;

      for(int j = 0;j<prefix.used;j++)
         if(dyn_array_element(uint32_t,&prefix,j)!=dyn_array_element(uint32_t,&entry->prefix,j))
            found = 0;

      if(found)
      {
         found = 0;
         for(int j = 0;j<entry->suffix.used;j++)
         {
            Suffix *s = &dyn_array_element(Suffix,&entry->suffix,j);
            if(s->entry==suffix)
            {
               found = 1;
               s->weight++;
               break;
            }
         }

         if(!found)
         {
            Suffix suff = {.entry = suffix, .weight = 1};
            dyn_array_add(Suffix,&entry->suffix,EXPAND,suff);
         }

         return;
      }
   }

   Markov_entry entry = {0};
   dyn_array_init(uint32_t,&entry.prefix,SIZE);
   for(int i = 0;i<prefix.used;i++)
      dyn_array_add(uint32_t,&entry.prefix,EXPAND,dyn_array_element(uint32_t,&prefix,i));
   dyn_array_init(Suffix,&entry.suffix,SIZE);
   Suffix suff = {.entry = suffix, .weight = 1};
   dyn_array_add(Suffix,&entry.suffix,SIZE,suff);
   dyn_array_add(Markov_entry,&markov_chain,SIZE,entry);
}

static int cmp(const void *a, const void *b)
{
   return (*(int *)a-*(int *)b);
}

static void generate_sentence(int state_size)
{
   dyn_array last_arr;
   dyn_array_init(uint32_t,&last_arr,state_size);
   last_arr.used = state_size;
   uint32_t *last = (uint32_t *)last_arr.data;

   int prefix = rand()%markov_chain.used;
   Markov_entry *e = &dyn_array_element(Markov_entry,&markov_chain,prefix);
   for(int i = 0;i<e->prefix.used;i++)
   {
      last[i] = dyn_array_element(uint32_t,&e->prefix,i);
      if(last[i]!=-1)
         printf("%s ",dyn_array_element(char *,&words,last[i]));
   }

   while(last[state_size-1]!=-1)
   {
      e = &dyn_array_element(Markov_entry,&markov_chain,find_prefix(last_arr));
      prefix = dyn_array_element(Suffix,&e->suffix,rand()%e->suffix.used).entry;
      if(prefix!=-1)
         printf("%s ",dyn_array_element(char *,&words,prefix));
      for(int i = 0;i<state_size-1;i++)
         last[i] = last[i+1];
      last[state_size-1] = prefix;
   }
   puts("");

   dyn_array_free(uint32_t,&last_arr);
}

static int find_prefix(dyn_array prefix)
{
   for(int i = 0;i<markov_chain.used;i++)
   {
      Markov_entry *entry = &dyn_array_element(Markov_entry,&markov_chain,i);
      int found = 1;

      for(int j = 0;j<prefix.used;j++)
         if(dyn_array_element(uint32_t,&prefix,j)!=dyn_array_element(uint32_t,&entry->prefix,j))
            found = 0;

      if(found)
         return i;
   }

   return -1;
}
//-------------------------------------
