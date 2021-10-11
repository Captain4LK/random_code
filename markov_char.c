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
#include <wchar.h>
#include <locale.h>
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

#define FNV_64_PRIME ((uint64_t)0x100000001b3ULL)
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
   wint_t entry;
   uint32_t weight;
}Suffix;
//-------------------------------------

//Variables
dyn_array markov_chain = {0};
dyn_array markov_chain_start = {0};
//-------------------------------------

//Function prototypes
static void print_help(char **argv);
static void chain_generate(const char *path, int state_size);
static void wchar_read_line(FILE *f, dyn_array *line);
static void entry_add(dyn_array prefix, wint_t suffix, int start);
static void sentence_generate(int state_size);
static int prefix_find(dyn_array prefix);
static wint_t prefix_choose_random(dyn_array suffixes);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   //Set to utf locale to make sure utf8 is actually supported
   setlocale(LC_ALL,"en_US.utf8");

   //Parse cmd arguments
   const char *path = NULL;
   const char *path_model = NULL;
   const char *path_out = NULL;
   int state_size = 2;
   int sentence_mode = 0;
   int sentence_count = 1;

   for(int i = 1;i<argc;i++)
   {
      if(strcmp(argv[i],"--help")==0||
         strcmp(argv[i],"-help")==0||
         strcmp(argv[i],"-h")==0||
         strcmp(argv[i],"?")==0)
         print_help(argv);
      else if(strcmp(argv[i],"-i")==0)
         path = READ_ARG(i);
      else if(strcmp(argv[i],"-m")==0)
         path_model = READ_ARG(i);
      else if(strcmp(argv[i],"-o")==0)
         path_out = READ_ARG(i);
      else if(strcmp(argv[i],"-sz")==0)
         state_size = atoi(READ_ARG(i));
      else if(strcmp(argv[i],"-sm")==0)
         sentence_mode = atoi(READ_ARG(i));
      else if(strcmp(argv[i],"-s")==0)
         sentence_count = atoi(READ_ARG(i));
   }

   srand(time(NULL));

   chain_generate(path,state_size);

   wprintf(L"Stats:\n-------------------------------\n");
   wprintf(L"Prefixes: %d\n",markov_chain.used);
   int64_t count = 0;
   int32_t max = INT32_MIN;
   int32_t min = INT32_MAX;
   for(int i = 0;i<markov_chain.used;i++)
   {
      int32_t num = dyn_array_element(Markov_entry,&markov_chain,i).suffix.used;
      if(num>max) max = num;
      if(num<min) min = num;

      count+=num;
   }
   wprintf(L"Average suffixes: %lf\n",(double)count/(double)markov_chain.used);
   wprintf(L"Max suffixes: %d\n",max);
   wprintf(L"Min suffixes: %d\n",min);
   wprintf(L"-------------------------------\n");

   for(int i = 0;i<sentence_count;i++)
      sentence_generate(state_size);

   return 0;
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -i filename [-o filename] [-sz state_size]\n"
          "   -i\tfile to generate chain from\n"
          "   -o\tfile to write output to (stdout if not provided)\n"
          "   -sz\tstate size, number of words to consider, 2 by default\n",
         argv[0],argv[0]);
}

static void chain_generate(const char *path, int state_size)
{
   dyn_array_init(Markov_entry,&markov_chain,SIZE);
   dyn_array_init(int32_t,&markov_chain_start,SIZE);
   dyn_array line;
   dyn_array_init(wint_t,&line,SIZE);

   FILE *f = fopen(path,"rb");

   do
   {
      wchar_read_line(f,&line);
      //wprintf(L"%ls\n",(wchar_t *)line.data);

      int max = line.used;
      if(max<state_size)
         continue;

      for(int i = 0;i<max;i++)
      {
         if(i+state_size+1>max)
            continue;

         dyn_array prefix;
         dyn_array_init(wint_t,&prefix,state_size);

         for(int s = i;s<i+state_size;s++)
            dyn_array_add(wint_t,&prefix,EXPAND,dyn_array_element(wint_t,&line,s));

         wint_t suffix = btowc('\0');
         if(i+state_size<max)
            suffix = dyn_array_element(wint_t,&line,i+state_size);

         entry_add(prefix,suffix,i==0);

         dyn_array_free(wint_t,&prefix);
      }
   }while(line.used>1);


   fclose(f);
   dyn_array_free(wint_t,&line);
}

static void wchar_read_line(FILE *f, dyn_array *line)
{
   line->used = 0;
   wint_t ch = fgetwc(f);

   while(ch!=btowc('\n')&&ch!=WEOF)
   {
      dyn_array_add(wint_t,line,EXPAND,ch);
      ch = fgetwc(f);
   }
   dyn_array_add(wint_t,line,EXPAND,btowc('\0'));
}

static void entry_add(dyn_array prefix, wint_t suffix, int start)
{
   for(int i = 0;i<markov_chain.used;i++)
   {
      Markov_entry *entry = &dyn_array_element(Markov_entry,&markov_chain,i);
      int found = 1;

      for(int j = 0;j<prefix.used;j++)
      {
         if(dyn_array_element(wint_t,&prefix,j)!=dyn_array_element(wint_t,&entry->prefix,j))
         {
            found = 0;
            break;
         }
      }

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
   dyn_array_init(wint_t,&entry.prefix,SIZE);
   for(int i = 0;i<prefix.used;i++)
      dyn_array_add(wint_t,&entry.prefix,EXPAND,dyn_array_element(wint_t,&prefix,i));
   dyn_array_init(Suffix,&entry.suffix,SIZE);
   Suffix suff = {.entry = suffix, .weight = 1};
   dyn_array_add(Suffix,&entry.suffix,SIZE,suff);
   dyn_array_add(Markov_entry,&markov_chain,SIZE,entry);

   if(start)
      dyn_array_add(int32_t,&markov_chain_start,EXPAND,markov_chain.used-1);
}

static void sentence_generate(int state_size)
{
   dyn_array last_arr;
   dyn_array sentence;
   dyn_array_init(wint_t,&last_arr,state_size);
   dyn_array_init(wint_t,&sentence,SIZE);
   last_arr.used = state_size;
   wint_t *last = (wint_t *)last_arr.data;

   //Generate starting words
   int prefix = dyn_array_element(int32_t,&markov_chain_start,rand()%markov_chain_start.used);
   Markov_entry *e = &dyn_array_element(Markov_entry,&markov_chain,prefix);
   for(int i = 0;i<e->prefix.used;i++)
   {
      last[i] = dyn_array_element(wint_t,&e->prefix,i);
      dyn_array_add(wint_t,&sentence,EXPAND,last[i]);
   }

   while(last[state_size-1]!=btowc('\0'))
   {
      e = &dyn_array_element(Markov_entry,&markov_chain,prefix_find(last_arr));
      //prefix = dyn_array_element(Suffix,&e->suffix,rand()%e->suffix.used).entry;
      prefix = prefix_choose_random(e->suffix);
      if(prefix!=btowc('\0'))
         dyn_array_add(wint_t,&sentence,EXPAND,prefix);
      for(int i = 0;i<state_size-1;i++)
         last[i] = last[i+1];
      last[state_size-1] = prefix;
   }

   dyn_array_add(wint_t,&sentence,EXPAND,btowc('\0'));

   wprintf(L"%ls\n",(wchar_t *)sentence.data);

   dyn_array_free(wint_t,&sentence);
}

static int prefix_find(dyn_array prefix)
{
   for(int i = 0;i<markov_chain.used;i++)
   {
      Markov_entry *entry = &dyn_array_element(Markov_entry,&markov_chain,i);
      int found = 1;

      for(int j = 0;j<prefix.used;j++)
      {
         if(dyn_array_element(wint_t,&prefix,j)!=dyn_array_element(wint_t,&entry->prefix,j))
         {
            found = 0;
            break;
         }
      }

      if(found)
         return i;
   }

   return -1;
}

static wint_t prefix_choose_random(dyn_array suffixes)
{
   dyn_array entries;
   dyn_array_init(wint_t,&entries,SIZE);

   for(int i = 0;i<suffixes.used;i++)
      for(int j = 0;j<dyn_array_element(Suffix,&suffixes,i).weight;j++)
         dyn_array_add(wint_t,&entries,EXPAND,dyn_array_element(Suffix,&suffixes,i).entry);

   wint_t result = dyn_array_element(wint_t,&entries,rand()%entries.used);
   dyn_array_free(wint_t,&entries);
   
   return result;
}
//-------------------------------------
