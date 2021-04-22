/*
brainfuck interpreter

Written in 2021 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
//2^24
#define MEM_SIZE 16777216

//Universal dynamic array
typedef struct
{
   uint32_t used;
   uint32_t size;
   void *data;
}dyn_array;

#define dyn_array_init(type, array, space) \
   do { dyn_array_free(type, (array)); ((dyn_array *)(array))->size = (space); ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->data = malloc(sizeof(type)*(((dyn_array *)(array))->size)); } while(0)

#define dyn_array_free(type, array) \
   do { if(((dyn_array *)(array))->data) { free(((dyn_array *)(array))->data); ((dyn_array *)(array))->data = NULL; ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->size = 0; }} while(0)

#define dyn_array_clear(type, array) \
   do { ((dyn_array *)(array))->used = 0; } while(0)

#define dyn_array_add(type, array, grow, element) \
   do { ((type *)((dyn_array *)(array)->data))[((dyn_array *)(array))->used] = element; ((dyn_array *)(array))->used++; if(((dyn_array *)(array))->used==((dyn_array *)(array))->size) { ((dyn_array *)(array))->size+=grow; ((dyn_array *)(array))->data = realloc(((dyn_array *)(array))->data,sizeof(type)*(((dyn_array *)(array))->size)); } } while(0)

#define dyn_array_element(type, array, index) \
   (((type *)((dyn_array *)(array)->data))[index])
//-------------------------------------

//Typedefs
typedef enum 
{
   INSTR_NONE = 0,
   PTR_PLUS = 1, PTR_MINUS = 2,
   VAL_PLUS = 3, VAL_MINUS = 4,
   GET_VAL = 5, PUT_VAL = 6,
   WHILE_START = 7, WHILE_END = 8,
}Opcode;

typedef struct
{
   Opcode opc;
   int arg;
}Instruction;

typedef struct Stack
{
   int ptr;
   struct Stack *next; 
}Stack;
//-------------------------------------

//Variables
static uint8_t mem[MEM_SIZE] = {0};
static uint8_t *ptr = mem;

static Stack *stack = NULL;
static Stack *stack_reserve = NULL;

//"Pointers" to all procedures
static int proc_table[256];

//Resizing buffer for instructions
static unsigned instr_ptr = 0; //Index to instr
static dyn_array instr_array = {0};
static Instruction *instr = NULL;
//-------------------------------------

//Function prototypes
static void preprocess(FILE *in);
static inline void stack_push(int iptr);
static inline int stack_pull();
//-------------------------------------

//Function implementations

int main(int argc, char *argv[])
{
   //Parse cmd arguments
   const char *path = NULL;
   const char *io_path = NULL;
   for(int i = 1;i<argc;i++)
   {
      if(argv[i][0]=='-')
      {
         switch(argv[i][1])
         {
         case 'h': puts("brainfuck interpreter\nAvailible commandline options:\n\t-h\t\tprint this text\n\t-f [PATH]\tspecify the file to execute\n\t-i [PATH]\tspecify the file to use as input"); break;
         case 'f': path = argv[++i]; break;
         case 'i':  io_path = argv[++i]; break;
         }
      }
   }

   //Parse input file
   //Removes all invalid characters
   if(path==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }
   FILE *in = fopen(path,"r");
   preprocess(in);
   fclose(in);

   //Initialize procedure memory
   for(int i = 0;i<256;i++)
      proc_table[i] = -1;
      
   //Run code
   FILE *input = stdin;
   if(io_path!=NULL)
      input = fopen(io_path,"r");

   clock_t start = clock();
   while(instr_ptr<instr_array.used)
   {
      switch(instr[instr_ptr].opc)
      {
      case INSTR_NONE: instr_ptr++; break;
      case PTR_PLUS: ptr+=instr[instr_ptr].arg; instr_ptr++; break;
      case PTR_MINUS: ptr-=instr[instr_ptr].arg; instr_ptr++; break;
      case VAL_PLUS: *ptr+=instr[instr_ptr].arg; instr_ptr++; break;
      case VAL_MINUS: *ptr-=instr[instr_ptr].arg; instr_ptr++; break;
      case GET_VAL: *ptr = fgetc(input);instr_ptr++; break;
      case PUT_VAL: putchar(*ptr); instr_ptr++; fflush(stdout); break;
      case WHILE_END: instr_ptr = stack_pull(); break;
      case WHILE_START:
         stack_push(instr_ptr);
         instr_ptr++;
         if(!(*ptr))
         {
            instr_ptr = stack_pull();
            int balance = 1;
            while(balance) 
            {
               instr_ptr++;
               if(instr[instr_ptr].opc==WHILE_START) balance++;
               else if(instr[instr_ptr].opc==WHILE_END) balance--;
            }
            instr_ptr++;
         }
         break;
      }
   }
   printf("Time taken: %lf\n",(double)(clock()-start)/CLOCKS_PER_SEC);

   return 0;
}

static void preprocess(FILE *in)
{
   dyn_array_init(Instruction,&instr_array,256);
   while(!feof(in))
   {
      char c = fgetc(in);
      Instruction next = {0};
      switch(c)
      {
      case '>': next.opc = PTR_PLUS; next.arg = 1; break;
      case '<': next.opc = PTR_MINUS; next.arg = 1;break;
      case '+': next.opc = VAL_PLUS; next.arg = 1;break;
      case '-': next.opc = VAL_MINUS; next.arg = 1;break;
      case ',': next.opc = GET_VAL; break;
      case '.': next.opc = PUT_VAL; break;
      case '[': next.opc = WHILE_START; break;
      case ']': next.opc = WHILE_END; break;
      }

      if(next.opc)
         dyn_array_add(Instruction,&instr_array,256,next);
   }
   instr = (Instruction *)instr_array.data;
}

//Only allocates memory if nothing is 
//stored on the stack reserve.
static inline void stack_push(int iptr)
{
   if(stack_reserve==NULL)
   {
      Stack *s = malloc(sizeof(*s));
      s->ptr = iptr;
      s->next = stack;
      stack = s;

      return;
   }

   Stack *s = stack_reserve;
   stack_reserve = s->next;
   s->ptr = iptr;
   s->next = stack;
   stack = s;
}

//Instead of freeing unused memory, it gets
//stored on a second stack and used for the next push 
//instead of allocating new memory.
static inline int stack_pull()
{
   Stack *s = stack;
   stack = s->next;
   s->next = stack_reserve;
   stack_reserve = s;

   return s->ptr;
}
//-------------------------------------
