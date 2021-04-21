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
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
//2^24
#define MEM_SIZE 16777216
//-------------------------------------

//Typedefs
typedef enum 
{
   INSTR_NONE = 0,
   PTR_PLUS = 1, PTR_MINUS = 2,
   VAL_PLUS = 3, VAL_MINUS = 4,
   GET_VAL = 5, PUT_VAL = 6,
   WHILE_START = 7, WHILE_END = 8,
   PROC_START = 9, PROC_END = 10,
   PROC_CALL = 11,
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
static int instr_used = 0;
static int instr_space = 512;
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

   while(instr_ptr<instr_used)
   {
      switch(instr[instr_ptr])
      {
      case INSTR_NONE: instr_ptr++; break;
      case PTR_PLUS: ++ptr; instr_ptr++; break;
      case PTR_MINUS: --ptr; instr_ptr++; break;
      case VAL_PLUS: ++*ptr; instr_ptr++; break;
      case VAL_MINUS: --*ptr; instr_ptr++; break;
      case GET_VAL: *ptr = fgetc(input);instr_ptr++; break;
      case PUT_VAL: putchar(*ptr); instr_ptr++; break;
      case WHILE_END: instr_ptr = stack_pull(); break;
      case PROC_CALL: stack_push(instr_ptr+1); instr_ptr = proc_table[*ptr]; break;
      case PROC_END: instr_ptr = stack_pull(); break;
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
               if(instr[instr_ptr]==WHILE_START) balance++;
               else if(instr[instr_ptr]==WHILE_END) balance--;
            }
            instr_ptr++;
         }
         break;
      case PROC_START:
         proc_table[*ptr] = instr_ptr+1;
         {
            int balance = 1;
            while(balance) 
            {
               instr_ptr++;
               if(instr[instr_ptr]==PROC_START) balance++;
               else if(instr[instr_ptr]==PROC_END) balance--;
            }
            instr_ptr++;
         }
         break;
      }
      fflush(stdout);
   }

   return 0;
}

static void preprocess(FILE *in)
{
   instr = malloc(sizeof(*instr)*instr_space);
   while(!feof(in))
   {
      char c = fgetc(in);
      Instruction next = 0;
      switch(c)
      {
      case '>': next = PTR_PLUS; break;
      case '<': next = PTR_MINUS; break;
      case '+': next = VAL_PLUS; break;
      case '-': next = VAL_MINUS; break;
      case ',': next = GET_VAL; break;
      case '.': next = PUT_VAL; break;
      case '[': next = WHILE_START; break;
      case ']': next = WHILE_END; break;
      case '(': next = PROC_START; break;
      case ')': next = PROC_END; break;
      case ':': next = PROC_CALL; break;
      }

      if(next)
      {
         instr[instr_used++] = next; 
         if(instr_used>=instr_space)
         {
            instr_space+=512;
            instr = realloc(instr,sizeof(*instr)*instr_space);
         }
      }
   }
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
