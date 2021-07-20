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
   int32_t weight;
}Suffix;
//-------------------------------------

//Variables
dyn_array sentences = {0};
dyn_array words = {0};
dyn_array markov_chain = {0};
dyn_array markov_chain_start = {0};
//-------------------------------------

//Function prototypes
static void print_help(char **argv);
static void split_sentences(char *input);
static void split_sentences_lines(char *input);
static void dump_model(const char *path);
static void generate_chain(int state_size);
static void load_model(int state_size, const char *path);
static int add_word(const char *word);
static void add_entry(dyn_array prefix,int32_t suffix, int start);
static void generate_sentence(char *input, int state_size);
static int find_prefix(dyn_array prefix);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
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

   if(path==NULL)
   {
      printf("No input file specified, try %s --help\n",argv[0]);
      return -1;
   }

   srand(time(NULL));

   if(path_model==NULL)
   {
      FILE *f = fopen(path,"rb");
      int32_t size = 0;
      fseek(f,0,SEEK_END);
      size = ftell(f);
      fseek(f,0,SEEK_SET);
      char *input = malloc(size+1);
      fread(input,size,1,f);
      input[size] = 0;
      fclose(f);

      if(sentence_mode==0)
         split_sentences(input);
      else if(sentence_mode==1)
         split_sentences_lines(input);
      free(input);

      generate_chain(state_size);

      dump_model("out.bin");

      //Print stats
      printf("Stats:\n-------------------------------\n");
      printf("Unique words: %d\n",words.used);
      printf("Prefixes: %d\n",markov_chain.used);
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
      printf("Average suffixes: %lf\n",(double)count/(double)markov_chain.used);
      printf("Max suffixes: %d\n",max);
      printf("Min suffixes: %d\n",min);
      printf("-------------------------------\n");
   }
   else
   {
      load_model(state_size,path_model);
   }

   FILE *f = fopen(path,"rb");
   int32_t size = 0;
   fseek(f,0,SEEK_END);
   size = ftell(f);
   fseek(f,0,SEEK_SET);
   char *input = malloc(size+1);
   fread(input,size,1,f);
   input[size] = 0;
   fclose(f);

   for(int i = 0;i<sentence_count;i++)
      generate_sentence(input,state_size);

   free(input);

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

static void split_sentences_lines(char *input)
{
   dyn_array input_processed;
   dyn_array_init(char,&input_processed,SIZE);
   while(*input)
   {
      dyn_array_add(char,&input_processed,EXPAND,*input);
      if((*input)=='\n')
         dyn_array_add(char,&input_processed,EXPAND,' ');
      input++;
   }
   dyn_array_add(char,&input_processed,EXPAND,'\0');

   const char *token = " ";
   int current = 0;
   input = (char *)input_processed.data;

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
      if(nword[strlen(nword)-1]=='\n')
         nword[strlen(nword)-1] = '\0';
      dyn_array_add(char *,&dyn_array_element(dyn_array,&sentences,current),EXPAND,nword);

      if(word[strlen(word)-1]=='\n')
      {
         current++;
         sentence.used = 0;
         sentence.size = 0;
         sentence.data = NULL;
         dyn_array_init(char *,&sentence,SIZE);
         dyn_array_add(dyn_array,&sentences,EXPAND,sentence);
      }

      word = strtok(NULL,token);
   }

   dyn_array_free(char,&input_processed);
}

static void dump_model(const char *path)
{
   FILE *f = fopen(path,"wb");

   //Dump words
   fwrite(&words.used,sizeof(int32_t),1,f);
   for(int i = 0;i<words.used;i++)
   {
      char *word = dyn_array_element(char *,&words,i);
      int32_t len = strlen(word);
      fwrite(&len,sizeof(int32_t),1,f);
      fwrite(word,len,1,f);
   }

   //Dump markov chain start
   fwrite(&markov_chain_start.used,sizeof(int32_t),1,f);
   for(int i = 0;i<markov_chain_start.used;i++)
      fwrite(&dyn_array_element(int32_t,&markov_chain_start,i),sizeof(int32_t),1,f);

   //Dump markov chain
   fwrite(&markov_chain.used,sizeof(int32_t),1,f);
   for(int i = 0;i<markov_chain.used;i++)
   {
      Markov_entry *e = &dyn_array_element(Markov_entry,&markov_chain,i);
      
      fwrite(&e->prefix.used,sizeof(int32_t),1,f);
      for(int j = 0;j<e->prefix.used;j++)
         fwrite(&dyn_array_element(int32_t,&e->prefix,j),sizeof(int32_t),1,f);
      fwrite(&e->suffix.used,sizeof(int32_t),1,f);
      for(int j = 0;j<e->suffix.used;j++)
      {
         fwrite(&dyn_array_element(Suffix,&e->suffix,j).entry,sizeof(int32_t),1,f);
         fwrite(&dyn_array_element(Suffix,&e->suffix,j).weight,sizeof(int32_t),1,f);
      }
   }

   fclose(f);
}

static void generate_chain(int state_size)
{
   dyn_array_init(Markov_entry,&markov_chain,SIZE);
   dyn_array_init(int32_t,&markov_chain_start,SIZE);

   for(int i = 0;i<sentences.used;i++)
   {
      if(dyn_array_element(dyn_array,&sentences,i).used<state_size)
         continue;

      int max = dyn_array_element(dyn_array,&sentences,i).used;
      for(int j = 0;j<max;j++)
      {
         int32_t pos = 0;
         dyn_array prefix;
         dyn_array_init(int32_t,&prefix,state_size);

         if(j+state_size>max)
            continue;
         for(int s = j;s<j+state_size;s++)
         {
            char *word = dyn_array_element(char *,&dyn_array_element(dyn_array,&sentences,i),s);
            pos = add_word(word);
            dyn_array_add(int32_t,&prefix,EXPAND,pos);
         }

         char *suffix = NULL;
         pos = -1;
         if(j+state_size<max)
         {
            suffix = dyn_array_element(char *,&dyn_array_element(dyn_array,&sentences,i),j+state_size);
            pos = add_word(suffix);
         }

         //Add to markov chain
         add_entry(prefix,pos,j==0);

         dyn_array_free(int32_t,&prefix);
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

static void add_entry(dyn_array prefix, int32_t suffix, int start)
{
   for(int i = 0;i<markov_chain.used;i++)
   {
      Markov_entry *entry = &dyn_array_element(Markov_entry,&markov_chain,i);
      int found = 1;

      for(int j = 0;j<prefix.used;j++)
      {
         if(dyn_array_element(int32_t,&prefix,j)!=dyn_array_element(int32_t,&entry->prefix,j))
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
   dyn_array_init(int32_t,&entry.prefix,SIZE);
   for(int i = 0;i<prefix.used;i++)
      dyn_array_add(int32_t,&entry.prefix,EXPAND,dyn_array_element(int32_t,&prefix,i));
   dyn_array_init(Suffix,&entry.suffix,SIZE);
   Suffix suff = {.entry = suffix, .weight = 1};
   dyn_array_add(Suffix,&entry.suffix,SIZE,suff);
   dyn_array_add(Markov_entry,&markov_chain,SIZE,entry);

   if(start)
      dyn_array_add(int32_t,&markov_chain_start,EXPAND,markov_chain.used-1);
}

static void generate_sentence(char *input, int state_size)
{
   dyn_array last_arr;
   dyn_array sentence;
   dyn_array_init(int32_t,&last_arr,state_size);
   dyn_array_init(char,&sentence,SIZE);
   last_arr.used = state_size;
   int32_t *last = (int32_t *)last_arr.data;

   //Try to generate valid sentence ten times
   for(int t = 0;t<10;t++)
   {
      sentence.used = 0;

      //Generate starting words
      int prefix = dyn_array_element(int32_t,&markov_chain_start,rand()%markov_chain_start.used);
      Markov_entry *e = &dyn_array_element(Markov_entry,&markov_chain,prefix);
      for(int i = 0;i<e->prefix.used;i++)
      {
         last[i] = dyn_array_element(int32_t,&e->prefix,i);
         if(last[i]!=-1)
         {
            char *word = dyn_array_element(char *,&words,last[i]);
            for(;*word;word++)
               dyn_array_add(char,&sentence,EXPAND,*word);
            dyn_array_add(char,&sentence,EXPAND,' ');
         }
      }

      while(last[state_size-1]!=-1)
      {
         e = &dyn_array_element(Markov_entry,&markov_chain,find_prefix(last_arr));
         prefix = dyn_array_element(Suffix,&e->suffix,rand()%e->suffix.used).entry;
         if(prefix!=-1)
         {
            char *word = dyn_array_element(char *,&words,prefix);
            for(;*word;word++)
               dyn_array_add(char,&sentence,EXPAND,*word);
            dyn_array_add(char,&sentence,EXPAND,' ');
         }
         for(int i = 0;i<state_size-1;i++)
            last[i] = last[i+1];
         last[state_size-1] = prefix;
      }
      dyn_array_add(char,&sentence,EXPAND,'\n');
      //Add two to make sure there is enough space in string 
      //to correctly replace when checking
      dyn_array_add(char,&sentence,EXPAND,'\0');
      dyn_array_add(char,&sentence,EXPAND,'\0');

      //Check sentence
      const char *breakset = ".!?\n";
      char *text = (char *)sentence.data;
      char replaced = '\0';
      char *text_start = text;
      char *text_end = text;
      int found = 0;

      do
      {
         text_end = strpbrk(text_end,breakset);
         if(text_end!=NULL&&*text_end!='\n')
         {
            replaced = *(text_end+1);
            *(text_end+1) = '\0';

            if(strstr(input,text_start)!=NULL)
               found = 1;

            *(text_end+1) = replaced;

         }
         if(text_end!=NULL)
            text_end+=strspn(text_end,breakset);
         text_start = text_end;
      }
      while(text_end!=NULL&&*text_end!='\0');

      if(!found)
         break;

      if(t==9)
         printf("NU!:");
   }

   printf("%s",((char *)sentence.data));

   dyn_array_free(int32_t,&last_arr);
   dyn_array_free(char,&sentence);
}

static int find_prefix(dyn_array prefix)
{
   for(int i = 0;i<markov_chain.used;i++)
   {
      Markov_entry *entry = &dyn_array_element(Markov_entry,&markov_chain,i);
      int found = 1;

      for(int j = 0;j<prefix.used;j++)
      {
         if(dyn_array_element(int32_t,&prefix,j)!=dyn_array_element(int32_t,&entry->prefix,j))
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

static void load_model(int state_size, const char *path)
{
   FILE *f = fopen(path,"rb");

   int32_t word_count = 0;
   fread(&word_count,sizeof(int32_t),1,f);
   dyn_array_init(char *,&words,word_count);
   for(int i = 0;i<word_count;i++)
   {
      int32_t len = 0;
      fread(&len,sizeof(int32_t),1,f);
      char *nword = malloc(len+1);
      fread(nword,len,1,f);
      nword[len] = '\0';
      dyn_array_add(char *,&words,EXPAND,nword);
   }


   int32_t entry_count = 0;
   fread(&entry_count,sizeof(int32_t),1,f);
   dyn_array_init(int32_t,&markov_chain_start,entry_count);
   for(int i = 0;i<entry_count;i++)
   {
      int32_t en = 0;
      fread(&en,sizeof(int32_t),1,f);
      dyn_array_add(int32_t,&markov_chain_start,EXPAND,en);
   }

   entry_count = 0;
   fread(&entry_count,sizeof(int32_t),1,f);
   dyn_array_init(Markov_entry,&markov_chain,entry_count);
   for(int i = 0;i<entry_count;i++)
   {
      Markov_entry e;
      int32_t used = 0;
      fread(&used,sizeof(int32_t),1,f);
      dyn_array_init(int32_t,&e.prefix,used);
      for(int j = 0;j<used;j++)
      {
         int32_t el = 0;
         fread(&el,sizeof(int32_t),1,f);
         dyn_array_add(int32_t,&e.prefix,EXPAND,el);
      }

      fread(&used,sizeof(int32_t),1,f);
      dyn_array_init(Suffix,&e.suffix,used);
      for(int j = 0;j<used;j++)
      {
         Suffix s;
         int32_t el = 0;
         fread(&el,sizeof(int32_t),1,f);
         s.entry = el;
         fread(&el,sizeof(int32_t),1,f);
         s.weight = el;
         dyn_array_add(Suffix,&e.suffix,EXPAND,s);
      }

      dyn_array_add(Markov_entry,&markov_chain,EXPAND,e);
   }

   fclose(f);
}
//-------------------------------------
