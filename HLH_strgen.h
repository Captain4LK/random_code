#ifndef _HLH_STRGEN_H_

/*
   Markov chain string generator, wchar edition

   Written in 2021 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

   To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

/* 
   To create implementation (the function definitions) add
      #define HLH_STRGEN_IMPLEMENTATION
   before including this file in *one* C file (translation unit)
*/

/*
   malloc(), realloc(), free() and rand() can be overwritten by 
   defining the following macros:

   HLH_STRGEN_MALLOC
   HLH_STRGEN_FREE
   HLH_STRGEN_REALLOC
   HLH_STRGEN_RAND
*/

#define _HLH_STRGEN_H_

typedef struct
{
   uint32_t used;
   uint32_t size;
   void *data;
}HLH_strgen_dyn_array;

typedef struct
{
   HLH_strgen_dyn_array chain;
   HLH_strgen_dyn_array chain_start;
   uint32_t order;
}HLH_strgen;

//Create a new, empty model
HLH_strgen *HLH_strgen_new(uint32_t order);

//Destroy a model
void HLH_strgen_destroy(HLH_strgen *str);

//Parse a file and add every line to the model
void HLH_strgen_add_file(HLH_strgen *str, FILE *f);

//Add a null terminated string to the model,
int HLH_strgen_add_string(HLH_strgen *str, const char *s);

//Generate a string, must be free()'d by user
char *HLH_strgen_generate(HLH_strgen *str);

//Write a model to disk
void HLH_strgen_model_save(const HLH_strgen *str, FILE *f);

//Read a model from disk
HLH_strgen *HLH_strgen_model_load(FILE *f);

//Read a model from memory buffer
HLH_strgen *HLH_strgen_model_load_mem(const uint8_t *data, unsigned length);

//Returns the last error as a string
const char *HLH_strgen_get_error();

#endif

#ifdef HLH_STRGEN_IMPLEMENTATION
#ifndef HLH_STRGEN_IMPLEMENTATION_ONCE
#define HLH_STRGEN_IMPLEMENTATION_ONCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef HLH_STRGEN_MALLOC
#define HLH_STRGEN_MALLOC malloc
#endif

#ifndef HLH_STRGEN_FREE
#define HLH_STRGEN_FREE free
#endif

#ifndef HLH_STRGEN_REALLOC
#define HLH_STRGEN_REALLOC realloc
#endif

#ifndef HLH_STRGEN_RAND
#define HLH_STRGEN_RAND rand
#endif

#define HLH_dyn_array_init(type, array, space) \
   do { ((HLH_strgen_dyn_array *)(array))->size = (space); ((HLH_strgen_dyn_array *)(array))->used = 0; ((HLH_strgen_dyn_array *)(array))->data = HLH_STRGEN_MALLOC(sizeof(type)*(((HLH_strgen_dyn_array *)(array))->size)); } while(0)

#define HLH_dyn_array_free(type, array) \
   do { if(((HLH_strgen_dyn_array *)(array))->data!=NULL) { HLH_STRGEN_FREE(((HLH_strgen_dyn_array *)(array))->data); ((HLH_strgen_dyn_array *)(array))->data = NULL; ((HLH_strgen_dyn_array *)(array))->used = 0; ((HLH_strgen_dyn_array *)(array))->size = 0; }} while(0)

#define HLH_dyn_array_add(type, array, grow, element) \
   do { ((type *)((HLH_strgen_dyn_array *)(array)->data))[((HLH_strgen_dyn_array *)(array))->used] = (element); ((HLH_strgen_dyn_array *)(array))->used++; if(((HLH_strgen_dyn_array *)(array))->used==((HLH_strgen_dyn_array *)(array))->size) { ((HLH_strgen_dyn_array *)(array))->size+=grow; ((HLH_strgen_dyn_array *)(array))->data = HLH_STRGEN_REALLOC(((HLH_strgen_dyn_array *)(array))->data,sizeof(type)*(((HLH_strgen_dyn_array *)(array))->size)); } } while(0)

#define HLH_dyn_array_element(type, array, index) \
   (((type *)((HLH_strgen_dyn_array *)(array)->data))[index])

#define HLH_READ(m,p,l,t) \
   (((p)+sizeof(t)<=(l))?(*((t *)(m+p))):((t)0)); (p)+=sizeof(t)

#define HLH_EXPAND 128
#define HLH_SIZE 128

static void HLH_strgen_entry_add(HLH_strgen *str, HLH_strgen_dyn_array prefix, char suffix, int start);
static int HLH_strgen_read_line(FILE *f, HLH_strgen_dyn_array *line);
static char HLH_strgen_prefix_choose_random(HLH_strgen_dyn_array suffixes);
static int HLH_strgen_prefix_find(HLH_strgen *str, HLH_strgen_dyn_array prefix);

typedef struct
{
   HLH_strgen_dyn_array prefix;
   HLH_strgen_dyn_array suffix;
}HLH_strgen_entry;

typedef struct
{
   char entry;
   uint32_t weight;
}HLH_strgen_suffix;

HLH_strgen *HLH_strgen_new(uint32_t order)
{
   HLH_strgen *str = HLH_STRGEN_MALLOC(sizeof(*str));
   if(str==NULL)
      return NULL;

   str->order = order;
   HLH_dyn_array_init(HLH_strgen_entry,&str->chain,HLH_SIZE);
   HLH_dyn_array_init(uint32_t,&str->chain_start,HLH_SIZE);

   return str;
}

void HLH_strgen_destroy(HLH_strgen *str)
{
   if(str==NULL)
      return;

   for(unsigned i = 0;i<str->chain.used;i++)
   {
      HLH_strgen_entry *e = &HLH_dyn_array_element(HLH_strgen_entry,&str->chain,i);
      HLH_dyn_array_free(char,&e->prefix);
      HLH_dyn_array_free(HLH_strgen_suffix,&e->suffix);
   }

   HLH_dyn_array_free(HLH_strgen_entry,&str->chain);
   HLH_dyn_array_free(uint32_t,&str->chain_start);
   HLH_STRGEN_FREE(str);
}

void HLH_strgen_add_file(HLH_strgen *str, FILE *f)
{
   if(f==NULL)
      return;

   int status = 1;
   HLH_strgen_dyn_array line;
   HLH_dyn_array_init(char,&line,HLH_SIZE);

   do
   {
      status = HLH_strgen_read_line(f,&line);
      HLH_strgen_add_string(str,(char *)line.data);
   }
   while(status);

   HLH_dyn_array_free(char,&line);
}

int HLH_strgen_add_string(HLH_strgen *str, const char *s)
{
   unsigned max = strlen(s)+1;
   if(max<str->order)
      return 0;

   for(unsigned i = 0;i<max;i++)
   {
      if(i+str->order+1>max)
         continue;

      HLH_strgen_dyn_array prefix;
      HLH_dyn_array_init(char,&prefix,str->order);

      for(unsigned j = i;j<i+str->order;j++)
         HLH_dyn_array_add(char,&prefix,HLH_EXPAND,s[j]);

      char suffix = '\0';
      if(i+str->order<max)
         suffix = s[i+str->order];

      HLH_strgen_entry_add(str,prefix,suffix,i==0);

      HLH_dyn_array_free(char,&prefix);
   }

   return 1;
}

static void HLH_strgen_entry_add(HLH_strgen *str, HLH_strgen_dyn_array prefix, char suffix, int start)
{
   for(unsigned i = 0;i<str->chain.used;i++)
   {
      HLH_strgen_entry *entry = &HLH_dyn_array_element(HLH_strgen_entry,&str->chain,i);
      int found = 1;

      for(unsigned j = 0;j<prefix.used;j++)
      {
         if(HLH_dyn_array_element(char,&prefix,j)!=HLH_dyn_array_element(char,&entry->prefix,j))
         {
            found = 0;
            break;
         }
      }

      if(found)
      {
         found = 0;
         for(unsigned j = 0;j<entry->suffix.used;j++)
         {
            HLH_strgen_suffix *s = &HLH_dyn_array_element(HLH_strgen_suffix,&entry->suffix,j);
            if(s->entry==suffix)
            {
               found = 1;
               s->weight++;
               break;
            }
         }

         if(!found)
         {
            HLH_strgen_suffix suff = {.entry = suffix, .weight = 1};
            HLH_dyn_array_add(HLH_strgen_suffix,&entry->suffix,HLH_EXPAND,suff);
         }

         return;
      }
   }

   HLH_strgen_entry entry = {0};
   HLH_dyn_array_init(char,&entry.prefix,HLH_SIZE);
   for(unsigned i = 0;i<prefix.used;i++)
      HLH_dyn_array_add(char,&entry.prefix,HLH_EXPAND,HLH_dyn_array_element(char,&prefix,i));
   HLH_dyn_array_init(HLH_strgen_suffix,&entry.suffix,HLH_SIZE);
   HLH_strgen_suffix suff = {.entry = suffix, .weight = 1};
   HLH_dyn_array_add(HLH_strgen_suffix,&entry.suffix,HLH_SIZE,suff);
   HLH_dyn_array_add(HLH_strgen_entry,&str->chain,HLH_SIZE,entry);

   if(start)
      HLH_dyn_array_add(uint32_t,&str->chain_start,HLH_EXPAND,str->chain.used-1);
}

static int HLH_strgen_read_line(FILE *f, HLH_strgen_dyn_array *line)
{
   line->used = 0;
   char ch = fgetc(f);

   if(ch==EOF)
      return 0;

   while(ch!='\n'&&ch!=EOF)
   {
      HLH_dyn_array_add(char,line,HLH_EXPAND,ch);
      ch = fgetc(f);
   }

   HLH_dyn_array_add(char,line,HLH_EXPAND,'\0');
   
   return 1;
}

char *HLH_strgen_generate(HLH_strgen *str)
{
   HLH_strgen_dyn_array last_arr;
   HLH_strgen_dyn_array sentence;
   HLH_dyn_array_init(char,&last_arr,str->order);
   HLH_dyn_array_init(char,&sentence,HLH_SIZE);
   last_arr.used = str->order;
   char *last = (char *)last_arr.data;

   //Generate starting words
   uint32_t prefix = HLH_dyn_array_element(uint32_t,&str->chain_start,HLH_STRGEN_RAND()%str->chain_start.used);
   HLH_strgen_entry *e = &HLH_dyn_array_element(HLH_strgen_entry,&str->chain,prefix);
   for(unsigned i = 0;i<e->prefix.used;i++)
   {
      last[i] = HLH_dyn_array_element(char,&e->prefix,i);
      HLH_dyn_array_add(char,&sentence,HLH_EXPAND,last[i]);
   }

   while(last[str->order-1]!='\0')
   {
      e = &HLH_dyn_array_element(HLH_strgen_entry,&str->chain,HLH_strgen_prefix_find(str,last_arr));
      prefix = HLH_strgen_prefix_choose_random(e->suffix);
      if(prefix!='\0')
         HLH_dyn_array_add(char,&sentence,HLH_EXPAND,prefix);
      for(unsigned i = 0;i<str->order-1;i++)
         last[i] = last[i+1];
      last[str->order-1] = prefix;
   }

   HLH_dyn_array_add(char,&sentence,HLH_EXPAND,'\0');
   HLH_dyn_array_free(char,&last_arr);

   return (char *)sentence.data;
}

static int HLH_strgen_prefix_find(HLH_strgen *str, HLH_strgen_dyn_array prefix)
{
   for(unsigned i = 0;i<str->chain.used;i++)
   {
      HLH_strgen_entry *entry = &HLH_dyn_array_element(HLH_strgen_entry,&str->chain,i);
      int found = 1;

      for(unsigned j = 0;j<prefix.used;j++)
      {
         if(HLH_dyn_array_element(char,&prefix,j)!=HLH_dyn_array_element(char,&entry->prefix,j))
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

static char HLH_strgen_prefix_choose_random(HLH_strgen_dyn_array suffixes)
{
   HLH_strgen_dyn_array entries;
   HLH_dyn_array_init(char,&entries,HLH_SIZE);

   for(unsigned i = 0;i<suffixes.used;i++)
      for(unsigned j = 0;j<HLH_dyn_array_element(HLH_strgen_suffix,&suffixes,i).weight;j++)
         HLH_dyn_array_add(char,&entries,HLH_EXPAND,HLH_dyn_array_element(HLH_strgen_suffix,&suffixes,i).entry);

   char result = HLH_dyn_array_element(char,&entries,HLH_STRGEN_RAND()%entries.used);
   HLH_dyn_array_free(char,&entries);
   
   return result;
}

//TODO: handle big/little endian
void HLH_strgen_model_save(const HLH_strgen *str, FILE *f)
{
   fwrite(&str->order,sizeof(uint32_t),1,f);
   fwrite(&str->chain_start.used,sizeof(str->chain_start.used),1,f);
   for(unsigned i = 0;i<str->chain_start.used;i++)
      fwrite(&HLH_dyn_array_element(uint32_t,&str->chain_start,i),sizeof(uint32_t),1,f);
   fwrite(&str->chain.used,sizeof(str->chain.used),1,f);
   for(unsigned i = 0;i<str->chain.used;i++)
   {
      const HLH_strgen_entry *e = &HLH_dyn_array_element(HLH_strgen_entry,&str->chain,i);
      fwrite(&e->prefix.used,sizeof(e->prefix.used),1,f);
      for(unsigned j = 0;j<e->prefix.used;j++)
         fwrite(&HLH_dyn_array_element(char,&e->prefix,j),sizeof(char),1,f);
      fwrite(&e->suffix.used,sizeof(e->suffix.used),1,f);
      for(unsigned j = 0;j<e->suffix.used;j++)
      {
         const HLH_strgen_suffix *s = &HLH_dyn_array_element(HLH_strgen_suffix,&e->suffix,j);
         fwrite(&s->entry,sizeof(s->entry),1,f);
         fwrite(&s->weight,sizeof(s->weight),1,f);
      }
   }
}

HLH_strgen *HLH_strgen_model_load(FILE *f)
{
   if(f==NULL)
      return NULL;

   unsigned size = 0;
   fseek(f,0,SEEK_END);
   size = ftell(f);
   fseek(f,0,SEEK_SET);

   uint8_t *buffer = HLH_STRGEN_MALLOC(size+1);
   if(buffer==NULL)
      return NULL;

   fread(buffer,size,1,f);
   //Not necessary
   buffer[size] = '\0';

   HLH_strgen *out = HLH_strgen_model_load_mem(buffer,size);

   free(buffer);

   return out;
}

//TODO: handle big/little endian
HLH_strgen *HLH_strgen_model_load_mem(const uint8_t *data, unsigned length)
{
   unsigned pos = 0;
   uint32_t order = HLH_READ(data,pos,length,uint32_t);

   HLH_strgen *str = HLH_strgen_new(order);
   if(str==NULL)
      return NULL;

   uint32_t used = HLH_READ(data,pos,length,uint32_t);
   for(unsigned i = 0;i<used;i++)
   {
      uint32_t val = HLH_READ(data,pos,length,uint32_t);
      HLH_dyn_array_add(uint32_t,&str->chain_start,HLH_EXPAND,val);
   }

   used = HLH_READ(data,pos,length,uint32_t);
   for(unsigned i = 0;i<used;i++)
   {
      HLH_strgen_entry e = {0};
      HLH_dyn_array_init(char,&e.prefix,HLH_SIZE);
      HLH_dyn_array_init(HLH_strgen_suffix,&e.suffix,HLH_SIZE);

      uint32_t e_used = HLH_READ(data,pos,length,uint32_t); 
      for(unsigned j = 0;j<e_used;j++)
      {
         char val = HLH_READ(data,pos,length,char);
         HLH_dyn_array_add(char,&e.prefix,HLH_EXPAND,val);
      }

      e_used = HLH_READ(data,pos,length,uint32_t); 
      for(unsigned j = 0;j<e_used;j++)
      {
         HLH_strgen_suffix s = {0};
         s.entry = HLH_READ(data,pos,length,char);
         s.weight = HLH_READ(data,pos,length,uint32_t);

         HLH_dyn_array_add(HLH_strgen_suffix,&e.suffix,HLH_EXPAND,s);
      }

      HLH_dyn_array_add(HLH_strgen_entry,&str->chain,HLH_EXPAND,e);
   }

   return str;
}

#undef HLH_dyn_array_init
#undef HLH_dyn_array_free
#undef HLH_dyn_array_add
#undef HLH_dyn_array_element
#undef HLH_EXPAND
#undef HLH_SIZE
#undef HLH_READ

#endif
#endif
