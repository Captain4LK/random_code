#ifndef _HLH_MARKOV_H_

/*
   Markov chain string generator

   Written in 2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

   To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

/* 
   To create implementation (the function definitions) add
      #define HLH_MARKOV_IMPLEMENTATION
   before including this file in *one* C file (translation unit)
*/

/*
   malloc(), realloc(), free() and rand() can be overwritten by 
   defining the following macros:

   HLH_MARKOV_MALLOC
   HLH_MARKOV_FREE
   HLH_MARKOV_REALLOC
   HLH_MARKOV_RAND
*/

#define _HLH_MARKOV_H_

#include <stdint.h>

typedef struct HLH_markov_context_char HLH_markov_context_char;
typedef struct HLH_markov_count HLH_markov_count;

typedef struct HLH_markov_word_node HLH_markov_word_node;
typedef struct HLH_markov_char_node HLH_markov_char_node;

typedef enum
{
   HLH_MARKOV_CHAR,
   HLH_MARKOV_WORD,
}HLH_markov_model_type;

//Dynamic array types
//I used to use macors for dynamic arrays, but
//a lot of the time you need just a little
//more flexibility, so I have seperate implementations instead
typedef struct
{
   char **data;
   int32_t data_used;
   int32_t data_size;
}HLH_markov_str_array;

typedef struct
{
   int32_t *data;
   int32_t data_used;
   int32_t data_size;
}HLH_markov_u32_array;

typedef struct
{
   char *data;
   int32_t data_used;
   int32_t data_size;
}HLH_markov_char_array;

typedef struct
{
   HLH_markov_context_char *data;
   int32_t data_used;
   int32_t data_size;
}HLH_markov_context_char_array;

typedef struct
{
   HLH_markov_count *data;
   int32_t data_used;
   int32_t data_size;
}HLH_markov_count_array;

typedef struct
{
   HLH_markov_char_array start_chars;
   HLH_markov_context_char_array contexts[256];
}HLH_markov_model_char;

typedef struct
{
   HLH_markov_str_array words[256];
   HLH_markov_u32_array start_words;
   HLH_markov_word_node *root;
}HLH_markov_model_word;

typedef struct
{
   HLH_markov_model_type type;
   union
   {
      HLH_markov_model_word mword;
      HLH_markov_model_char mchar;
   }as;
}HLH_markov_model;

HLH_markov_model *HLH_markov_model_new(HLH_markov_model_type type);
void HLH_markov_model_delete(HLH_markov_model *model);

void HLH_markov_model_add(HLH_markov_model *model, const char *str);

char *HLH_markov_model_generate(const HLH_markov_model *model);

int HLH_markov_model_size(const HLH_markov_model *model);

#endif

#ifdef HLH_MARKOV_IMPLEMENTATION
#ifndef HLH_MARKOV_IMPLEMENTATION_ONCE
#define HLH_MARKOV_IMPLEMENTATION_ONCE

#ifndef HLH_MARKOV_MALLOC
#define HLH_MARKOV_MALLOC malloc
#endif

#ifndef HLH_MARKOV_FREE
#define HLH_MARKOV_FREE free
#endif

#ifndef HLH_MARKOV_REALLOC
#define HLH_MARKOV_REALLOC realloc
#endif

#ifndef HLH_MARKOV_RAND
#define HLH_MARKOV_RAND rand
#endif

#ifndef HLH_MARKOV_ORDER_CHAR
#define HLH_MARKOV_ORDER_CHAR 3
#endif

#ifndef HLH_MARKOV_ORDER_WORD
#define HLH_MARKOV_ORDER_WORD 3
#endif

//Minimum order used during generation
//The maximum order gets restricted to 
//make the generated text more random
//set to ORDER (default) to disable
#ifndef HLH_MARKOV_ORDER_MIN_CHAR
#define HLH_MARKOV_ORDER_MIN_CHAR (HLH_MARKOV_ORDER_CHAR)
#endif

#ifndef HLH_MARKOV_ORDER_MIN_WORD
#define HLH_MARKOV_ORDER_MIN_WORD (HLH_MARKOV_ORDER_WORD)
#endif

//Use number of occurences as weight when generating
#ifndef HLH_MARKOV_RANDOM_WEIGHT
#define HLH_MARKOV_RANDOM_WEIGHT 1
#endif

//Save guard in case of endless loops (due to circular markov chain)
#ifndef HLH_MARKOV_MAX_LENGTH
#define HLH_MARKOV_MAX_LENGTH 2048
#endif

#define HLH_FNV_32_PRIME ((uint32_t)0x01000193)

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct HLH_markov_context_char
{
   char context[HLH_MARKOV_ORDER_CHAR];
   uint32_t context_size;
   uint32_t total;
   uint16_t counts[256];
};

struct HLH_markov_word_node
{
   //Data
   uint32_t context[HLH_MARKOV_ORDER_WORD];
   int32_t context_size;
   int32_t total;
   HLH_markov_count_array counts;

   HLH_markov_word_node *left;
   HLH_markov_word_node *right;
   int height;
};

struct HLH_markov_char_node
{
   //Data
   char context[HLH_MARKOV_ORDER_CHAR];
   int32_t context_size;
   int32_t total;
   HLH_markov_count_array counts;

   HLH_markov_char_node *left;
   HLH_markov_char_node *right;
   int height;
};

static int32_t _HLH_markov_word_node_cmp(const HLH_markov_word_node * restrict a, const HLH_markov_word_node * restrict b);
static HLH_markov_word_node *_HLH_markov_word_node_rotl(HLH_markov_word_node *root);
static HLH_markov_word_node *_HLH_markov_word_node_rotr(HLH_markov_word_node *root);
static int _HLH_markov_word_node_height(const HLH_markov_word_node *root);
static HLH_markov_word_node *_HLH_markov_word_node_insert(HLH_markov_word_node *root, HLH_markov_word_node *node, HLH_markov_word_node **insert);
static int _HLH_markov_word_node_balance(const HLH_markov_word_node *root);
static HLH_markov_word_node *_HLH_markov_word_node_search(HLH_markov_word_node *root, uint32_t context[HLH_MARKOV_ORDER_WORD], uint32_t context_size);
static int _HLH_markov_word_node_count(HLH_markov_word_node *root);

struct HLH_markov_count
{
   uint32_t count;
   uint32_t item;
};

static char *_HLH_markov_strtok(char *s, const char *sep);

static void _HLH_markov_model_delete_char(HLH_markov_model *model);
static void _HLH_markov_model_delete_word(HLH_markov_model *model);

static void _HLH_markov_model_add_char(HLH_markov_model *model, const char *str);
static void _HLH_markov_model_add_word(HLH_markov_model *model, const char *str);

static char *_HLH_markov_model_generate_word(const HLH_markov_model *model);
static char *_HLH_markov_model_generate_char(const HLH_markov_model *model);

static void _HLH_markov_context_char_array_free(HLH_markov_context_char_array *array);
static HLH_markov_context_char *_HLH_markov_context_char_find_or_create(HLH_markov_context_char_array *array, char context[HLH_MARKOV_ORDER_CHAR], uint32_t context_size);
static HLH_markov_context_char *_HLH_markov_context_char_find(const HLH_markov_context_char_array *array, char context[HLH_MARKOV_ORDER_CHAR], uint32_t context_size);

static HLH_markov_word_node *_HLH_markov_context_word_find_or_create(HLH_markov_word_node **root, uint32_t context[HLH_MARKOV_ORDER_WORD], uint32_t context_size);
static HLH_markov_word_node *_HLH_markov_context_word_find(HLH_markov_word_node *root, uint32_t context[HLH_MARKOV_ORDER_WORD], uint32_t context_size);

static const char *_HLH_markov_model_word_get_word(const HLH_markov_model *model, uint32_t word);
static uint32_t _HLH_markov_model_word_add_word(HLH_markov_model *model, const char *word);

static int _HLH_markov_model_word_size(const HLH_markov_model *model);
static int _HLH_markov_model_char_size(const HLH_markov_model *model);

//FowlerNollVo Hash
static uint32_t _HLH_markov_fnv32a(const char *str);

//Dynamic arrays
static void     _HLH_markov_u32_array_add(HLH_markov_u32_array *array, uint32_t num);
static void     _HLH_markov_u32_array_free(HLH_markov_u32_array *array);
static void     _HLH_markov_char_array_add(HLH_markov_char_array *array, char ch);
static void     _HLH_markov_char_array_free(HLH_markov_char_array *array);
static uint32_t _HLH_markov_str_array_add(HLH_markov_str_array *array, const char *str);
static void     _HLH_markov_str_array_free(HLH_markov_str_array *array);
static void     _HLH_markov_count_array_add(HLH_markov_count_array *array, uint32_t item);
static void     _HLH_markov_count_array_free(HLH_markov_count_array *array);

HLH_markov_model *HLH_markov_model_new(HLH_markov_model_type type)
{
   HLH_markov_model *model = HLH_MARKOV_MALLOC(sizeof(*model));
   memset(model,0,sizeof(*model));
   model->type = type;

   return model;
}

void HLH_markov_model_delete(HLH_markov_model *model)
{
   if(model==NULL)
      return;

   if(model->type==HLH_MARKOV_CHAR)
      _HLH_markov_model_delete_char(model);
   else if(model->type==HLH_MARKOV_WORD)
      _HLH_markov_model_delete_word(model);

   HLH_MARKOV_FREE(model);
}

static void _HLH_markov_model_delete_char(HLH_markov_model *model)
{
   _HLH_markov_char_array_free(&model->as.mchar.start_chars);
   for(int i = 0;i<256;i++)
   {
      //for(int j = 0;j<model->as.mchar.contexts[i].data_used;j++)
         //_HLH_markov_count_array_free(&model->as.mchar.contexts[i].data[j].counts);
      _HLH_markov_context_char_array_free(&model->as.mchar.contexts[i]);
   }
}

static void _HLH_markov_model_delete_word(HLH_markov_model *model)
{
   //TODO
   /*for(int i = 0;i<256;i++)
      _HLH_markov_str_array_free(&model->as.mword.words[i]);

   _HLH_markov_u32_array_free(&model->as.mword.start_words);
   for(int i = 0;i<256;i++)
   {
      for(int j = 0;j<model->as.mword.contexts[i].data_used;j++)
         _HLH_markov_count_array_free(&model->as.mword.contexts[i].data[j].counts);
      _HLH_markov_context_word_array_free(&model->as.mword.contexts[i]);
   }*/
}

void HLH_markov_model_add(HLH_markov_model *model, const char *str)
{
   if(model->type==HLH_MARKOV_CHAR)
      _HLH_markov_model_add_char(model,str);
   else if(model->type==HLH_MARKOV_WORD)
      _HLH_markov_model_add_word(model,str);
}

char *HLH_markov_model_generate(const HLH_markov_model *model)
{
   if(model->type==HLH_MARKOV_CHAR)
      return _HLH_markov_model_generate_char(model);
   else if(model->type==HLH_MARKOV_WORD)
      return _HLH_markov_model_generate_word(model);
   return NULL;
}

int HLH_markov_model_size(const HLH_markov_model *model)
{
   if(model->type==HLH_MARKOV_CHAR)
      return _HLH_markov_model_char_size(model);
   else if(model->type==HLH_MARKOV_WORD)
      return _HLH_markov_model_word_size(model);
   return 0;
}

static char *_HLH_markov_model_generate_word(const HLH_markov_model *model)
{
   if(model->as.mword.start_words.data_used==0)
      return NULL;

   HLH_markov_u32_array sentence = {0};
   _HLH_markov_u32_array_add(&sentence,model->as.mword.start_words.data[HLH_MARKOV_RAND()%model->as.mword.start_words.data_used]);

   int done = 0;
   while(!done)
   {
      int start = sentence.data_used;

#if HLH_MARKOV_ORDER_MIN_WORD!=HLH_MARKOV_ORDER_WORD
      int backoff = HLH_MARKOV_ORDER_MIN_WORD+HLH_MARKOV_RAND()%(HLH_MARKOV_ORDER_WORD-HLH_MARKOV_ORDER_MIN_WORD+1);
#else
      int backoff = HLH_MARKOV_ORDER_WORD;
#endif

      if(start-backoff<0)
         backoff = start;
      start--;

      uint32_t context[HLH_MARKOV_ORDER_WORD] = {0};
      for(int i = 0;i<backoff;i++)
         context[i] = sentence.data[start-i];
      
      HLH_markov_word_node *model_context = NULL;
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

         model_context = _HLH_markov_context_word_find(model->as.mword.root,context,i);
         if(model_context!=NULL)
            break;
      }

      if(model_context!=NULL)
      {
#if HLH_MARKOV_RANDOM_WEIGHT
         uint32_t num = HLH_MARKOV_RAND()%model_context->total;
         uint32_t cur = 0;
         for(int i = 0;i<model_context->counts.data_used;i++)
         {
            cur+=model_context->counts.data[i].count;
            if(cur>=num)
            {
               _HLH_markov_u32_array_add(&sentence,model_context->counts.data[i].item);
               break;
            }
         }
#else
         _HLH_markov_u32_array_add(&sentence,model_context->counts.data[HLH_MARKOV_RAND()%model_context->counts.data_used].item);
#endif
      }
      else
      {
         done = 1;
      }

      if(sentence.data_used>=HLH_MARKOV_MAX_LENGTH)
         done = 1;
   }

   HLH_markov_char_array str = {0};
   for(int i = 0;i<sentence.data_used;i++)
   {
      const char *s = _HLH_markov_model_word_get_word(model,sentence.data[i]);
      for(;*s!='\0';s++)
         _HLH_markov_char_array_add(&str,*s);
      _HLH_markov_char_array_add(&str,' ');
   }
   _HLH_markov_char_array_add(&str,'\0');

   _HLH_markov_u32_array_free(&sentence);

   return str.data;
}

static char *_HLH_markov_model_generate_char(const HLH_markov_model *model)
{
   if(model->as.mchar.start_chars.data_used==0)
      return NULL;

   HLH_markov_char_array str = {0};
   _HLH_markov_char_array_add(&str,model->as.mchar.start_chars.data[HLH_MARKOV_RAND()%model->as.mchar.start_chars.data_used]);

   int done = 0;
   while(!done)
   {
      int start = str.data_used;

#if HLH_MARKOV_ORDER_MIN_CHAR!=HLH_MARKOV_ORDER_CHAR
      int backoff = HLH_MARKOV_ORDER_MIN_CHAR+HLH_MARKOV_RAND()%(HLH_MARKOV_ORDER_CHAR-HLH_MARKOV_ORDER_MIN_CHAR+1);
#else
      int backoff = HLH_MARKOV_ORDER_CHAR;
#endif

      if(start-backoff<0)
         backoff = start;
      start--;

      char context[HLH_MARKOV_ORDER_CHAR] = {0};
      for(int i = 0;i<backoff;i++)
         context[i] = str.data[start-i];
      
      HLH_markov_context_char *model_context = NULL;
      for(int i = backoff;i>0;i--)
      {
         uint8_t xor = 0;
         for(int j = 0;j<i;j++)
         {
            char ch = str.data[start-j];

            xor^=ch;
         }

         model_context = _HLH_markov_context_char_find(&model->as.mchar.contexts[xor],context,i);
         if(model_context!=NULL)
            break;
      }

      if(model_context!=NULL)
      {
#if HLH_MARKOV_RANDOM_WEIGHT
         uint32_t num = HLH_MARKOV_RAND()%model_context->total;
         uint32_t cur = 0;
         for(int i = 0;i<128;i++)
         {
            cur+=model_context->counts[i];
            if(model_context->counts[i]>0&&cur>=num)
            {
               _HLH_markov_char_array_add(&str,(char)i);
               break;
            }
         }
#else
         _HLH_markov_char_array_add(&str,model_context->counts.data[HLH_MARKOV_RAND()%model_context->counts.data_used].item);
#endif
      }
      else
      {
         done = 1;
      }

      if(str.data_used>=HLH_MARKOV_MAX_LENGTH)
         done = 1;
   }

   _HLH_markov_char_array_add(&str,'\0');

   return str.data;
}

static void _HLH_markov_model_add_char(HLH_markov_model *model, const char *str)
{
   int len = strlen(str)+1;

   _HLH_markov_char_array_add(&model->as.mchar.start_chars,str[0]);
   for(int i = 1;i<len;i++)
   {

      char context[HLH_MARKOV_ORDER_CHAR] = {0};
      char event = str[i];

      uint8_t xor = 0;
      for(int m = 1;m<=HLH_MARKOV_ORDER_CHAR;m++)
      {
         if(i-m<0)
            break;

         context[m-1] = str[i-m];
         xor^=context[m-1];

         HLH_markov_context_char *model_context = _HLH_markov_context_char_find_or_create(&model->as.mchar.contexts[xor],context,m);
         model_context->total++;

         //Cap
         if(model_context->counts[(unsigned char)event]<UINT16_MAX)
            model_context->counts[(unsigned char)event]++;
      }
   }
}

static void _HLH_markov_model_add_word(HLH_markov_model *model, const char *str)
{
   char *str_line = HLH_MARKOV_MALLOC(sizeof(*str_line)*(strlen(str)+1));
   strcpy(str_line,str);
   HLH_markov_u32_array sentence = {0};

   //Collect words and convert sentence to integers
   const char *token = " ";
   char *word = _HLH_markov_strtok(str_line,token);
   int first = 1;
   while(word!=NULL)
   {
      uint32_t word_index = _HLH_markov_model_word_add_word(model,word);
      _HLH_markov_u32_array_add(&sentence,word_index);
      if(first)
      {
         first = 0;
         _HLH_markov_u32_array_add(&model->as.mword.start_words,word_index);
      }

      word = _HLH_markov_strtok(NULL,token);
   }
   free(str_line);

   //Analyze sentence
   for(int i = 1;i<sentence.data_used;i++)
   {
      uint32_t context[HLH_MARKOV_ORDER_WORD] = {0};
      uint32_t event = sentence.data[i];

      for(int m = 1;m<=HLH_MARKOV_ORDER_WORD;m++)
      {
         if((int)i-m<0)
            break;

         context[m-1] = sentence.data[i-m];

         HLH_markov_word_node *model_context = _HLH_markov_context_word_find_or_create(&model->as.mword.root,context,m);
         model_context->total++;
         _HLH_markov_count_array_add(&model_context->counts,event);
      }
   }

   _HLH_markov_u32_array_free(&sentence);
}

static void _HLH_markov_u32_array_add(HLH_markov_u32_array *array, uint32_t num)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = HLH_MARKOV_MALLOC(sizeof(*array->data)*array->data_size);
   }

   array->data[array->data_used++] = num;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = HLH_MARKOV_REALLOC(array->data,sizeof(*array->data)*array->data_size);
   }
}

static void _HLH_markov_u32_array_free(HLH_markov_u32_array *array)
{
   if(array==NULL||array->data==NULL)
      return;

   HLH_MARKOV_FREE(array->data);
   array->data = NULL;
   array->data_used = 0;
   array->data_size = 0;
}

static void _HLH_markov_char_array_add(HLH_markov_char_array *array, char ch)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = HLH_MARKOV_MALLOC(sizeof(*array->data)*array->data_size);
   }

   array->data[array->data_used++] = ch;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = HLH_MARKOV_REALLOC(array->data,sizeof(*array->data)*array->data_size);
   }
}

static void _HLH_markov_char_array_free(HLH_markov_char_array *array)
{
   if(array==NULL||array->data==NULL)
      return;

   HLH_MARKOV_FREE(array->data);
   array->data = NULL;
   array->data_used = 0;
   array->data_size = 0;
}

static uint32_t _HLH_markov_model_word_add_word(HLH_markov_model *model, const char *word)
{
   uint8_t index = _HLH_markov_fnv32a(word)&255;
   return _HLH_markov_str_array_add(&model->as.mword.words[index],word)|(index<<24);
}

static const char *_HLH_markov_model_word_get_word(const HLH_markov_model *model, uint32_t word)
{
   uint8_t index = (word>>24)&255;
   word^=(index<<24);
   return model->as.mword.words[index].data[word];
}

static uint32_t _HLH_markov_str_array_add(HLH_markov_str_array *array, const char *str)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = HLH_MARKOV_MALLOC(sizeof(*array->data)*array->data_size);
   }

   for(int i = 0;i<array->data_used;i++)
      if(strcmp(array->data[i],str)==0)
         return i;

   int len = strlen(str)+1;
   array->data[array->data_used] = HLH_MARKOV_MALLOC(sizeof(*array->data[array->data_used])*len);
   strcpy(array->data[array->data_used],str);
   array->data_used++;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = HLH_MARKOV_REALLOC(array->data,sizeof(*array->data)*array->data_size);
   }
   
   return array->data_used-1;
}

static void _HLH_markov_str_array_free(HLH_markov_str_array *array)
{
   if(array==NULL||array->data==NULL)
      return;

   for(int i = 0;i<array->data_used;i++)
      HLH_MARKOV_FREE(array->data[i]);

   HLH_MARKOV_FREE(array->data);
   array->data = NULL;
   array->data_used = 0;
   array->data_size = 0;
}

static void _HLH_markov_count_array_add(HLH_markov_count_array *array, uint32_t item)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = HLH_MARKOV_MALLOC(sizeof(*array->data)*array->data_size);
   }

   for(int i = 0;i<array->data_used;i++)
   {
      if(array->data[i].item==item)
      {
         array->data[i].count++;
         return;
      }
   }

   array->data[array->data_used].item = item;
   array->data[array->data_used].count = 1;
   array->data_used++;

   if(array->data_used>=array->data_size)
   {
      array->data_size+=16;
      array->data = HLH_MARKOV_REALLOC(array->data,sizeof(*array->data)*array->data_size);
   }
}

static void _HLH_markov_count_array_free(HLH_markov_count_array *array)
{
   if(array==NULL||array->data==NULL)
      return;

   HLH_MARKOV_FREE(array->data);
   array->data = NULL;
   array->data_used = 0;
   array->data_size = 0;
}

static void _HLH_markov_context_char_array_free(HLH_markov_context_char_array *array)
{
   if(array==NULL||array->data==NULL)
      return;

   HLH_MARKOV_FREE(array->data);
   array->data = NULL;
   array->data_used = 0;
   array->data_size = 0;
}

static HLH_markov_context_char *_HLH_markov_context_char_find_or_create(HLH_markov_context_char_array *array, char context[HLH_MARKOV_ORDER_CHAR], uint32_t context_size)
{
   if(array->data==NULL)
   {
      array->data_used = 0;
      array->data_size = 16;
      array->data = HLH_MARKOV_MALLOC(sizeof(*array->data)*array->data_size);
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
      array->data = HLH_MARKOV_REALLOC(array->data,sizeof(*array->data)*array->data_size);
   }
   
   return &array->data[array->data_used-1];
}

static HLH_markov_context_char *_HLH_markov_context_char_find(const HLH_markov_context_char_array *array, char context[HLH_MARKOV_ORDER_CHAR], uint32_t context_size)
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

static HLH_markov_word_node *_HLH_markov_context_word_find_or_create(HLH_markov_word_node **root, uint32_t context[HLH_MARKOV_ORDER_WORD], uint32_t context_size)
{
   HLH_markov_word_node *n;
   HLH_markov_word_node new = {0};
   memcpy(&new.context,context,sizeof(*context)*context_size);
   new.context_size = context_size;
   new.total = 0;
   memset(&new.counts,0,sizeof(new.counts));

   *root = _HLH_markov_word_node_insert(*root,&new,&n);
   return n;
}

static HLH_markov_word_node *_HLH_markov_context_word_find(HLH_markov_word_node *root, uint32_t context[HLH_MARKOV_ORDER_WORD], uint32_t context_size)
{
   return _HLH_markov_word_node_search(root,context,context_size);
}

static int _HLH_markov_model_word_size(const HLH_markov_model *model)
{
   return 0;
}

static int _HLH_markov_model_char_size(const HLH_markov_model *model)
{
   int size = 0;
   for(int i = 0;i<256;i++)
      size+=model->as.mchar.contexts[i].data_used*sizeof(model->as.mchar.contexts[i].data[0]);
   return size;
}

static uint32_t _HLH_markov_fnv32a(const char *str)
{
   uint32_t hval = 0x811c9dc5;
   unsigned char *s = (unsigned char *)str;
   while(*s) 
   {
      hval^=(uint32_t)*s++;
      hval *= HLH_FNV_32_PRIME;
   }

   return hval;
}

static char *_HLH_markov_strtok(char *s, const char *sep)
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
      *(s++) = 0;

   src = s;

   return p;
}

static int32_t _HLH_markov_word_node_cmp(const HLH_markov_word_node * restrict a, const HLH_markov_word_node * restrict b)
{
   if(a->context_size!=b->context_size)
      return a->context_size<b->context_size?-1:1;

   for(int i = 0;i<a->context_size;i++)
      if(a->context[i]!=b->context[i])
         return a->context[i]<b->context[i]?-1:1;

   return 0;
}

static HLH_markov_word_node *_HLH_markov_word_node_rotl(HLH_markov_word_node *root)
{
   HLH_markov_word_node *r = root->right;
   root->right = r->left;
   r->left = root;

   root->height = _HLH_markov_word_node_height(root);
   r->height = _HLH_markov_word_node_height(r);

   return r;
}

static HLH_markov_word_node *_HLH_markov_word_node_rotr(HLH_markov_word_node *root)
{
   HLH_markov_word_node *l = root->left;
   root->left = l->right;
   l->right = root;

   root->height = _HLH_markov_word_node_height(root);
   l->height = _HLH_markov_word_node_height(l);

   return l;
}

static int _HLH_markov_word_node_height(const HLH_markov_word_node *root)
{
   if(root==NULL)
      return 0;

   int hl;
   int hr;

   if(root->left==NULL)
      hl = 0;
   else
      hl = 1+root->left->height;

   if(root->right==NULL)
      hr = 0;
   else
      hr = 1+root->right->height;

   if(hl>hr)
      return hl;
   return hr;
}

static HLH_markov_word_node *_HLH_markov_word_node_insert(HLH_markov_word_node *root, HLH_markov_word_node *node, HLH_markov_word_node **insert)
{
   if(root==NULL)
   {
      root = HLH_MARKOV_MALLOC(sizeof(*root));
      memcpy(root,node,sizeof(*node));
      *insert = root;
   }
   else if(_HLH_markov_word_node_cmp(root,node)<0)
   {
      root->right = _HLH_markov_word_node_insert(root->right,node,insert);

      if(_HLH_markov_word_node_balance(root)==-2)
      {
         if(_HLH_markov_word_node_cmp(node,root->right)>0)
         {
            root = _HLH_markov_word_node_rotl(root);
         }
         else
         {
            root->right = _HLH_markov_word_node_rotr(root->right);
            root = _HLH_markov_word_node_rotl(root);
         }
      }
   }
   else if(_HLH_markov_word_node_cmp(root,node)>0)
   {
      root->left = _HLH_markov_word_node_insert(root->left,node,insert);

      if(_HLH_markov_word_node_balance(root)==2)
      {
         if(_HLH_markov_word_node_cmp(node,root->left)<0)
         {
            root = _HLH_markov_word_node_rotr(root);
         }
         else
         {
            root->left = _HLH_markov_word_node_rotl(root->left);
            root = _HLH_markov_word_node_rotr(root);
         }
      }
   }
   else
   {
      *insert = root;
   }

   root->height = _HLH_markov_word_node_height(root);
   return root;
}

static int _HLH_markov_word_node_balance(const HLH_markov_word_node *root)
{
   int hl;
   int hr;

   if(root->left==NULL)
      hl = 0;
   else
      hl = 1+root->left->height;

   if(root->right==NULL)
      hr = 0;
   else
      hr = 1+root->right->height;

   return hl-hr;
}

static HLH_markov_word_node *_HLH_markov_word_node_search(HLH_markov_word_node *root, uint32_t context[HLH_MARKOV_ORDER_WORD], uint32_t context_size)
{
   HLH_markov_word_node n = {0};
   memcpy(&n.context,context,sizeof(*context)*context_size);
   n.context_size = context_size;
   n.total = 0;
   memset(&n.counts,0,sizeof(n.counts));

   if(root==NULL)
      return NULL;
   int cmp = _HLH_markov_word_node_cmp(&n,root);

   if(cmp>0)
      return _HLH_markov_word_node_search(root->right,context,context_size);
   else if(cmp<0)
      return _HLH_markov_word_node_search(root->left,context,context_size);
   return root;
}

static int _HLH_markov_word_node_count(HLH_markov_word_node *root)
{
   int count = 0;
   if(root->left!=NULL)
      count+=_HLH_markov_word_node_count(root->left);
   if(root->right!=NULL)
      count+=_HLH_markov_word_node_count(root->right);
   return count+1;
}

#undef HLH_FNV_32_PRIME 
#endif
#endif
