/*
brainfuck to c compiler

Written in 2021 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines

//Memory size
#define MEM_SIZE (1<<24)

//Universal dynamic array
#define dyn_array_init(type, array, space) \
   do { ((dyn_array *)(array))->size = (space); ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->data = malloc(sizeof(type)*(((dyn_array *)(array))->size)); } while(0)

#define dyn_array_free(type, array) \
   do { if(((dyn_array *)(array))->data) { free(((dyn_array *)(array))->data); ((dyn_array *)(array))->data = NULL; ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->size = 0; }} while(0)

#define dyn_array_add(type, array, grow, element) \
   do { ((type *)((dyn_array *)(array)->data))[((dyn_array *)(array))->used] = (element); ((dyn_array *)(array))->used++; if(((dyn_array *)(array))->used==((dyn_array *)(array))->size) { ((dyn_array *)(array))->size+=grow; ((dyn_array *)(array))->data = realloc(((dyn_array *)(array))->data,sizeof(type)*(((dyn_array *)(array))->size)); } } while(0)

#define dyn_array_element(type, array, index) \
   (((type *)((dyn_array *)(array)->data))[index])

#define EXPAND 256

#define READ_ARG(I) \
   ((++(I))<argc?argv[(I)]:NULL)

#define PRINT_INDENT(a) \
   for(int print_indent_o = 0;print_indent_o<(a);print_indent_o++) fprintf(f,"   ");
//-------------------------------------

//Typedefs
typedef enum 
{
   PTR = 1, VAL = 2, GET_VAL = 3, PUT_VAL = 4, WHILE_START = 5, WHILE_END = 6, CLEAR = 7, EXIT = 8,
}Opcode;

typedef struct
{
   Opcode opc;
   int arg0;
   int arg1;
   int offset;
}Instruction;

typedef struct
{
   unsigned used;
   unsigned size;
   void *data;
}dyn_array;
//-------------------------------------

//Variables
//Resizing buffer for instructions
static dyn_array instr_array = {0};
static Instruction *instr = NULL;
//-------------------------------------

//Function prototypes
static void preprocess(FILE *in);
static void optimize();
static void out_c(FILE *f);
static void out_asm(FILE *f);
static void print_help(char **argv);
//-------------------------------------

//Function implementations

int main(int argc, char *argv[])
{
   //Parse cmd arguments
   const char *path = NULL;
   const char *path_out = NULL;
   int mode_out = 0;

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
      else if(strcmp(argv[i],"-m")==0)
      {
         const char *arg_mode = READ_ARG(i);
         if(strcmp(arg_mode,"c")==0)
            mode_out = 0;
         else if(strcmp(arg_mode,"x86_64")==0)
            mode_out = 1;
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
   optimize();

   //Output code in prefered language
   FILE *out = stdout;
   if(path_out!=NULL)
      out = fopen(path_out,"w");

   switch(mode_out)
   {
   case 0: out_c(out);   break; //c
   case 1: out_asm(out); break; //x86_64 asm
   }

   if(path_out!=NULL)
      fclose(out);

   dyn_array_free(Instruction,&instr_array);

   return 0;
}

static void preprocess(FILE *in)
{
   dyn_array_init(Instruction,&instr_array,EXPAND);
   while(!feof(in))
   {
      char c = fgetc(in);
      Instruction next = {0};
      switch(c)
      {
      case '>': next.opc = PTR; next.arg0 = 1; break;
      case '<': next.opc = PTR; next.arg0 = -1;break;
      case '+': next.opc = VAL; next.arg0 = 1;break;
      case '-': next.opc = VAL; next.arg0 = -1;break;
      case ',': next.opc = GET_VAL; break;
      case '.': next.opc = PUT_VAL; break;
      case '[': next.opc = WHILE_START; break;
      case ']': next.opc = WHILE_END; break;
      }

      if(next.opc)
         dyn_array_add(Instruction,&instr_array,EXPAND,next);
   }
   dyn_array_add(Instruction,&instr_array,EXPAND,(Instruction){.opc = EXIT});
   instr = (Instruction *)instr_array.data;
}

static void optimize()
{
   int input_len = instr_array.used;

   //Pass 1 --> rle
   //Additions and subtractions of pointer/value
   //get rle encoded
   //i.e.: +++++++ gets converted to *ptr+=7
   dyn_array old = instr_array;
   dyn_array_init(Instruction,&instr_array,EXPAND);
   unsigned i = 0;
   unsigned end = old.used;
   while(i<end)
   {
      switch(instr[i].opc)
      {
      case PTR:
         {
            int arg = 0;
            while(instr[i].opc==PTR) { arg+=instr[i].arg0; i++; }
            Instruction in = {.opc = PTR, .arg0 = arg, .offset = 0};
            dyn_array_add(Instruction,&instr_array,EXPAND,in);
         }
         break;
      case VAL:
         {
            int arg = 0;
            while(instr[i].opc==VAL) { arg+=instr[i].arg0; i++; }
            Instruction in = {.opc = VAL, .arg0 = arg, .offset = 0};
            dyn_array_add(Instruction,&instr_array,EXPAND,in);
         }
         break;
      default: dyn_array_add(Instruction,&instr_array,EXPAND,instr[i]); i++; break;
      }
   }
   dyn_array_free(Instruction,&old);
   instr = (Instruction *)instr_array.data;
   //-------------------------------------

   //Pass 2 --> common patterns
   //Some snippets of code are reused often
   //Some of these get taken care of here
   //Currently implemented:
   //[-] Clears the cell to zero
   old = instr_array;
   dyn_array_init(Instruction,&instr_array,EXPAND);
   i = 0;
   end = old.used;
   while(i<end)
   {
      //Pattern clear
      if(i<old.used-2&&instr[i].opc==WHILE_START&&instr[i+1].opc==VAL&&instr[i+2].opc==WHILE_END)
      {
         Instruction ni = {.opc = CLEAR, .arg0 = 0, .offset = 0};
         dyn_array_add(Instruction,&instr_array,EXPAND,ni);
         i+=3;
      }
      else
      {
         dyn_array_add(Instruction,&instr_array,EXPAND,instr[i]); i++;
      }
   }
   dyn_array_free(Instruction,&old);
   instr = (Instruction *)instr_array.data;
   //-------------------------------------

   //Pass 3 --> offsets
   //Sometimes the pointer gets moved to
   //a specific location and back for a single command,
   //this makes such occurrences execute
   //as a single execution
   //Currently implemented: '+';',';'.';'>';'[-]'
   old = instr_array;
   dyn_array_init(Instruction,&instr_array,EXPAND);
   i = 0;
   end = old.used;
   while(i<end)
   {
      //Offset add, val, clear, put, get
      if(i<old.used-2&&
         instr[i].opc==PTR&&instr[i+2].opc==PTR&&
         instr[i].arg0==-instr[i+2].arg0)
      {
         Instruction ni;
         switch(instr[i+1].opc)
         {
         case VAL: ni = (Instruction){.opc = VAL, .arg0 = instr[i+1].arg0, .offset = instr[i].arg0}; dyn_array_add(Instruction,&instr_array,EXPAND,ni); i+=3; break;
         case CLEAR: ni = (Instruction){.opc = CLEAR, .offset = instr[i].arg0}; dyn_array_add(Instruction,&instr_array,EXPAND,ni); i+=3; break;
         case PUT_VAL: ni = (Instruction){.opc = PUT_VAL, .offset = instr[i].arg0}; dyn_array_add(Instruction,&instr_array,EXPAND,ni); i+=3; break;
         case GET_VAL: ni = (Instruction){.opc = GET_VAL, .offset = instr[i].arg0}; dyn_array_add(Instruction,&instr_array,EXPAND,ni); i+=3; break;
         default: dyn_array_add(Instruction,&instr_array,EXPAND,instr[i]); i++; break;
         }
      }
      else
      {
         dyn_array_add(Instruction,&instr_array,EXPAND,instr[i]); i++;
      }
   }
   dyn_array_free(Instruction,&old);
   instr = (Instruction *)instr_array.data;
   //-------------------------------------

   //Pass 4 --> while
   //Find matching brackets
   //and store their positions
   //Needs to be done last
   //since memory layout of instructions
   //changes in previous passes
   old = instr_array;
   dyn_array_init(Instruction,&instr_array,EXPAND);
   i = 0;
   end = old.used;
   while(i<end)
   {
      switch(instr[i].opc)
      {
      case WHILE_END:
         {
            int sptr = instr_array.used;
            int balance = -1;
            while(balance) 
            {
               sptr--;
               if(((Instruction *)instr_array.data)[sptr].opc==WHILE_START) balance++;
               else if(((Instruction *)instr_array.data)[sptr].opc==WHILE_END) balance--;
            }
            dyn_array_add(Instruction,&instr_array,EXPAND,instr[i]);
            (&dyn_array_element(Instruction,&instr_array,sptr))->arg0 = instr_array.used-1;
            (&dyn_array_element(Instruction,&instr_array,instr_array.used-1))->arg0 = sptr;
            i++;
         }
         break;
      default: dyn_array_add(Instruction,&instr_array,EXPAND,instr[i]); i++; break;
      }
   }
   dyn_array_free(Instruction,&old);
   instr = (Instruction *)instr_array.data;
   //-------------------------------------

   printf("Optimization overview:\n\tInput length: %d\n\tOutput length: %d\n",input_len,instr_array.used);
}

static void out_c(FILE *f)
{
   fprintf(f,"#include <stdio.h>\n#include <stdint.h>\n\nuint8_t mem[%d];\nuint8_t *ptr = mem;\n\nint main(int argc, char **argv)\n{\n   FILE *in = stdin;\n   if(argc>1)\n      in = fopen(argv[1],\"r\");\n\n",MEM_SIZE);
   int indent = 1;
   for(unsigned i = 0;i<instr_array.used;i++)
   {
      switch(instr[i].opc)
      {
      case PTR: PRINT_INDENT(indent); fprintf(f,"ptr+=%d;\n",instr[i].arg0); break;
      case VAL: PRINT_INDENT(indent); fprintf(f,"*(ptr+%d)+=%d;\n",instr[i].offset,instr[i].arg0); break;
      case GET_VAL: PRINT_INDENT(indent); fprintf(f,"*(ptr+%d) = fgetc(in);\n",instr[i].offset); break;
      case PUT_VAL: PRINT_INDENT(indent); fprintf(f,"putchar(*(ptr+%d));\n",instr[i].offset); break;
      case WHILE_START: PRINT_INDENT(indent); fprintf(f,"while(*ptr)\n"); PRINT_INDENT(indent); fprintf(f,"{\n"); indent++; break;
      case WHILE_END: indent--; PRINT_INDENT(indent); fprintf(f,"}\n"); break;
      case CLEAR: PRINT_INDENT(indent); fprintf(f,"*(ptr+%d) = 0;\n",instr[i].offset); break;
      case EXIT: break;
      }
   }
   fprintf(f,"\n   if(argc>1)\n      fclose(in);\n\n   return 0;\n}");
}

static void out_asm(FILE *f)
{
   fprintf(f,"global _start\nsection .text\n\n_start:\nmov r8, stack\n");
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -i filename -o filename [-m mode]\n"
          "   -i\tfile to compile\n"
          "   -o\toutput path\n"
          "   -m\toutput format (c;asm)\n",
         argv[0],argv[0]);
}
//-------------------------------------
