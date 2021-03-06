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
   int32_t entry;
   int32_t weight;
}Suffix;

typedef struct
{
   uint64_t hash;
   char *word;
}Word;
//-------------------------------------

//Variables
dyn_array words = {0};
dyn_array markov_chain = {0};
dyn_array markov_chain_start = {0};
//-------------------------------------

//Function prototypes
static void print_help(char **argv);
static void dump_model(const char *path);
static void generate_chain(int state_size, dyn_array sentences);
static void load_model(int state_size, const char *path);
static int add_word(const char *word);
static void add_entry(dyn_array prefix,int32_t suffix, int start);
static void generate_sentence(char *input, int state_size);
static int find_prefix(dyn_array prefix);

static char *file_read(const char *path);
static dyn_array string_split_sentence(char *input);
static dyn_array string_split_line(char *input);
static void string_split_free(dyn_array *sp);
static int is_abbreviation(const char *word);
static char *HLH_strtok(char *s, const char *sep, char *t);
static uint64_t fnv64a(const char *str);
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
      char *input = file_read(path);
      dyn_array sentences;

      if(sentence_mode==0)
         sentences = string_split_sentence(input);
      else if(sentence_mode==1)
         sentences = string_split_line(input);
      free(input);

      generate_chain(state_size,sentences);
      string_split_free(&sentences);

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

   char *input = file_read(path);

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

static void dump_model(const char *path)
{
   FILE *f = fopen(path,"wb");

   //Dump words
   fwrite(&words.used,sizeof(int32_t),1,f);
   for(int i = 0;i<words.used;i++)
   {
      char *word = dyn_array_element(Word,&words,i).word;
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

static void generate_chain(int state_size, dyn_array sentences)
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
         if(j+state_size>max)
            continue;

         int32_t pos = 0;
         dyn_array prefix;
         dyn_array_init(int32_t,&prefix,state_size);

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
   uint64_t hash = fnv64a(word);

   if(words.size==0)
      dyn_array_init(Word,&words,SIZE);

   //Check if in array
   for(int i = 0;i<words.used;i++)
      if(dyn_array_element(Word,&words,i).hash==hash)
         return i;

   //Add to array
   Word nword;
   nword.hash = hash;
   nword.word = malloc(strlen(word)+1);
   strcpy(nword.word,word);
   dyn_array_add(Word,&words,EXPAND,nword);

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
            char *word = dyn_array_element(Word,&words,last[i]).word;
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
            char *word = dyn_array_element(Word,&words,prefix).word;
            for(;*word;word++)
               dyn_array_add(char,&sentence,EXPAND,*word);
            dyn_array_add(char,&sentence,EXPAND,' ');
         }
         for(int i = 0;i<state_size-1;i++)
            last[i] = last[i+1];
         last[state_size-1] = prefix;
      }
      //Remove last charecter --> will always be whitespace, but is not desired for strstr
      sentence.used--;
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
         if(text_end!=NULL)
         {
            replaced = *(text_end);
            *(text_end) = '\0';

            if(strstr(input,text_start)!=NULL)
               found = 1;

            *(text_end) = replaced;

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
   for(int i = 0;i<word_count;i++)
   {
      int32_t len = 0;
      fread(&len,sizeof(int32_t),1,f);
      char *nword = malloc(len+1);
      fread(nword,len,1,f);
      nword[len] = '\0';
      add_word(nword);
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

static char *file_read(const char *path)
{
   FILE *f = fopen(path,"rb");
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

static dyn_array string_split_sentence(char *input)
{
   dyn_array sp;
   dyn_array sentence;
   const char *token = " \n";
   const char *punctuation = ".!?";
   int current = 0;

   dyn_array_init(dyn_array,&sp,SIZE);
   dyn_array_init(char *,&sentence,SIZE);
   dyn_array_add(dyn_array,&sp,EXPAND,sentence);

   char *word = strtok(input,token);
   while(word!=NULL)
   {
      char *nword = malloc(strlen(word)+1);
      strcpy(nword,word);
      dyn_array_add(char *,&dyn_array_element(dyn_array,&sp,current),EXPAND,nword);

      if(strchr(punctuation,word[strlen(word)-1])!=NULL&&!is_abbreviation(word))
      {
         current++;
         sentence.used = 0;
         sentence.size = 0;
         sentence.data = NULL;
         dyn_array_init(char *,&sentence,SIZE);
         dyn_array_add(dyn_array,&sp,EXPAND,sentence);
      }

      word = strtok(NULL,token);
   }

   return sp;
}

static dyn_array string_split_line(char *input)
{
   dyn_array sp;
   dyn_array sentence;
   const char *token = " \n";
   char t;
   int current = 0;

   dyn_array_init(dyn_array,&sp,SIZE);
   dyn_array_init(char *,&sentence,SIZE);
   dyn_array_add(dyn_array,&sp,EXPAND,sentence);

   char *word = HLH_strtok(input,token,&t);
   while(word!=NULL)
   {
      char *nword = malloc(strlen(word)+1);
      strcpy(nword,word);
      dyn_array_add(char *,&dyn_array_element(dyn_array,&sp,current),EXPAND,nword);

      if(t=='\n')
      {
         current++;
         sentence.used = 0;
         sentence.size = 0;
         sentence.data = NULL;
         dyn_array_init(char *,&sentence,SIZE);
         dyn_array_add(dyn_array,&sp,EXPAND,sentence);
      }

      word = HLH_strtok(NULL,token,&t);
   }

   return sp;
}

static void string_split_free(dyn_array *sp)
{
   for(int i = 0;i<sp->used;i++)
   {
      dyn_array *s = &dyn_array_element(dyn_array,sp,i);

      for(int j = 0;j<s->used;j++)
         free(dyn_array_element(char *,s,j));

      dyn_array_free(char *,s);
   }
   
   dyn_array_free(dyn_array,sp);
}

static int is_abbreviation(const char *word)
{
   if(strcmp(word,"Mr.")==0)
      return 1;
   if(strcmp(word,"Ms.")==0)
      return 1;
   if(strcmp(word,"Mrs.")==0)
      return 1;
   if(strcmp(word,"Dr.")==0)
      return 1;
   if(strcmp(word,"Inc.")==0)
      return 1;
   if(strcmp(word,"Ltd.")==0)
      return 1;
   if(strcmp(word,"Jr.")==0)
      return 1;
   if(strcmp(word,"Sr.")==0)
      return 1;
   if(strcmp(word,"Co.")==0)
      return 1;

   return 0;
}

//Essentially standard strtok, but stores the token
//the text was separated by in a variable.
static char *HLH_strtok(char *s, const char *sep, char *t)
{
   static char *src = NULL;
   char *p;

   if(s==NULL)
      s = src;

   while(*s&&strchr(sep,*s)!=NULL)
      s++;

   if(!*s)
      return NULL;

   for(p = s;*s&&!strchr(sep,*s);s++);

   if(*s&&s[1])
   {
      if(t!=NULL)
         *t = *s;
      *(s++) = 0;
   }

   src = s;

   return p;
}

static uint64_t fnv64a(const char *str)
{
   uint64_t hval = 0xcbf29ce484222325ULL;
   unsigned char *s = (unsigned char *)str;
   while(*s)
   {
      hval^=(uint64_t)*s++;
      hval*=FNV_64_PRIME;
   }

   return hval;
}
//-------------------------------------
