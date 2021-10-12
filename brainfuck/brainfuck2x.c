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

/*//Universal dynamic array
#define dyn_array_init(type, array, space) \
   do { ((dyn_array *)(array))->size = (space); ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->data = malloc(sizeof(type)*(((dyn_array *)(array))->size)); } while(0)

#define dyn_array_free(type, array) \
   do { if(((dyn_array *)(array))->data) { free(((dyn_array *)(array))->data); ((dyn_array *)(array))->data = NULL; ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->size = 0; }} while(0)

#define dyn_array_add(type, array, grow, element) \
   do { ((type *)((dyn_array *)(array)->data))[((dyn_array *)(array))->used] = (element); ((dyn_array *)(array))->used++; if(((dyn_array *)(array))->used==((dyn_array *)(array))->size) { ((dyn_array *)(array))->size+=grow; ((dyn_array *)(array))->data = realloc(((dyn_array *)(array))->data,sizeof(type)*(((dyn_array *)(array))->size)); } } while(0)

#define dyn_array_element(type, array, index) \
   (((type *)((dyn_array *)(array)->data))[index])*/

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
//-------------------------------------

//Variables

//Resizing buffer for instructions
static struct
{
   unsigned used;
   unsigned size;
   Instruction *data;
}instr = {0};
//-------------------------------------

//Function prototypes
static void preprocess(FILE *in);
static void optimize();
static void out_c(FILE *f);
static void out_fortran(FILE *f);
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
         else if(strcmp(arg_mode,"fortran")==0)
            mode_out = 1;
      }
   }

   if(path==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   //Read input file, remove all invalid characters, build instr array
   FILE *in = fopen(path,"r");
   preprocess(in);
   fclose(in);

   //Performe some optimizations
   optimize();

   //Output code in prefered language
   FILE *out = stdout;
   if(path_out!=NULL)
      out = fopen(path_out,"w");

   switch(mode_out)
   {
   case 0: out_c(out);   break; //c
   case 1: out_fortran(out); break; //fortran
   }

   if(path_out!=NULL)
      fclose(out);

   //Cleanup
   free(instr.data);

   return 0;
}

static void preprocess(FILE *in)
{
   instr.size = EXPAND;
   instr.used = 0;
   instr.data = calloc(sizeof(*instr.data),instr.size);

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
      {
         instr.data[instr.used++] = next;

         if(instr.used==instr.size)
         {
            instr.size+=EXPAND;
            instr.data = realloc(instr.data,sizeof(*instr.data)*instr.size);
         }
      }
   }

   //There will always be enough space for at least one more element in the array since
   //the array gets expanded after adding an element in the above code
   instr.data[instr.used++] = (Instruction){.opc = EXIT};
}

static void optimize()
{
   int input_len = instr.used;
   unsigned fast;
   unsigned end;
   int arg;
   Instruction in;

   //Pass 1 --> rle
   //Additions and subtractions of pointer/value
   //get rle encoded
   //i.e.: +++++++ gets converted to *ptr+=7
   end = instr.used;
   instr.used = 0;
   for(fast = 0;fast<end;)
   {
      Instruction ins = instr.data[fast];
      switch(ins.opc)
      {
      case PTR:
         arg = 0;
         while(instr.data[fast].opc==PTR) { arg+=instr.data[fast].arg0; fast++; }
         in = (Instruction) {.opc = PTR, .arg0 = arg, .offset = 0};
         instr.data[instr.used++] = in;
         break;
      case VAL:
         arg = 0;
         while(instr.data[fast].opc==VAL) { arg+=instr.data[fast].arg0; fast++; }
         in = (Instruction) {.opc = VAL, .arg0 = arg, .offset = 0};
         instr.data[instr.used++] = in;
         break;
      default:
         instr.data[instr.used++] = ins;
         fast++;
         break;
      }
   }
   //-------------------------------------

   //Pass 2 --> common patterns
   //Some snippets of code are reused often
   //Some of these get taken care of here
   //Currently implemented:
   //[-] Clears the cell to zero
   end = instr.used;
   instr.used = 0;
   for(fast = 0;fast<end;)
   {
      Instruction ins = instr.data[fast];

      if(fast<end-2&&ins.opc==WHILE_START&&instr.data[fast+1].opc==VAL&&instr.data[fast+2].opc==WHILE_END)
      {
         in = (Instruction) {.opc = CLEAR, .arg0 = 0, .offset = 0};
         instr.data[instr.used++] = in;
         fast+=3;
      }
      else
      {
         instr.data[instr.used++] = ins;
         fast++;
      }
   }
   //-------------------------------------

   //Pass 3 --> offsets
   //Sometimes the pointer gets moved to
   //a specific location and back for a single command,
   //this makes such occurrences execute
   //as a single execution
   //Currently implemented: '+';',';'.';'>';'[-]'
   end = instr.used;
   instr.used = 0;
   for(fast = 0;fast<end;)
   {
      Instruction ins = instr.data[fast];

      //Offset add, val, clear, put, get
      if(fast<end-2&&
         ins.opc==PTR&&instr.data[fast+2].opc==PTR&&
         ins.arg0==-instr.data[fast+2].arg0)
      {
         Instruction ni;
         switch(instr.data[fast+1].opc)
         {
         case VAL: ni = (Instruction){.opc = VAL, .arg0 = instr.data[fast+1].arg0, .offset = instr.data[fast].arg0}; instr.data[instr.used++] = ni; fast+=3; break;
         case CLEAR: ni = (Instruction){.opc = CLEAR, .offset = instr.data[fast].arg0}; instr.data[instr.used++] = ni; fast+=3; break;
         case PUT_VAL: ni = (Instruction){.opc = PUT_VAL, .offset = instr.data[fast].arg0}; instr.data[instr.used++] = ni; fast+=3; break;
         case GET_VAL: ni = (Instruction){.opc = GET_VAL, .offset = instr.data[fast].arg0}; instr.data[instr.used++] = ni; fast+=3; break;
         default: instr.data[instr.used++] = ins; fast++; break;
         }
      }
      else
      {
         instr.data[instr.used++] = ins;
         fast++;
      }
   }
   //-------------------------------------

   //Pass 4 --> while
   //Find matching brackets
   //and store their positions
   //Needs to be done last
   //since memory layout of instructions
   //changes in previous passes
   end = instr.used;
   instr.used = 0;
   for(fast = 0;fast<end;)
   {
      Instruction ins = instr.data[fast];
      if(ins.opc==WHILE_END)
      {
         int sptr = instr.used;
         int balance = -1;
         while(balance) 
         {
            sptr--;
            if(((Instruction *)instr.data)[sptr].opc==WHILE_START) balance++;
            else if(((Instruction *)instr.data)[sptr].opc==WHILE_END) balance--;
         }
         instr.data[instr.used++] = instr.data[fast];;
         instr.data[sptr].arg0 = instr.used-1;
         instr.data[instr.used-1].arg0 = sptr;
         fast++;
      }
      else
      {
         instr.data[instr.used++] = ins;
         fast++;
      }
   }
   //-------------------------------------

   //Downsize array
   instr.data = realloc(instr.data,sizeof(*instr.data)*instr.used);

   printf("Optimization overview:\n\tInput length: %d\n\tOutput length: %d\n",input_len,instr.used);
}

static void out_c(FILE *f)
{
   fprintf(f,"#include <stdio.h>\n#include <stdint.h>\n\nuint8_t mem[%d];\nuint8_t *ptr = mem;\n\nint main(int argc, char **argv)\n{\n   FILE *in = stdin;\n   if(argc>1)\n      in = fopen(argv[1],\"r\");\n\n",MEM_SIZE);
   int indent = 1;
   for(unsigned i = 0;i<instr.used;i++)
   {
      switch(instr.data[i].opc)
      {
      case PTR: PRINT_INDENT(indent); fprintf(f,"ptr+=%d;\n",instr.data[i].arg0); break;
      case VAL: PRINT_INDENT(indent); fprintf(f,"*(ptr+%d)+=%d;\n",instr.data[i].offset,instr.data[i].arg0); break;
      case GET_VAL: PRINT_INDENT(indent); fprintf(f,"*(ptr+%d) = fgetc(in);\n",instr.data[i].offset); break;
      case PUT_VAL: PRINT_INDENT(indent); fprintf(f,"putchar(*(ptr+%d));\n",instr.data[i].offset); break;
      case WHILE_START: PRINT_INDENT(indent); fprintf(f,"while(*ptr)\n"); PRINT_INDENT(indent); fprintf(f,"{\n"); indent++; break;
      case WHILE_END: indent--; PRINT_INDENT(indent); fprintf(f,"}\n"); break;
      case CLEAR: PRINT_INDENT(indent); fprintf(f,"*(ptr+%d) = 0;\n",instr.data[i].offset); break;
      case EXIT: break;
      }
   }
   fprintf(f,"\n   if(argc>1)\n      fclose(in);\n\n   return 0;\n}");
}

static void out_fortran(FILE *f)
{
   fprintf(f,"program fortran\n   use iso_fortran_env\n   implicit none\n   character, dimension(:), allocatable::mem\n   integer(kind=4)::ptr\n   ptr = 0\n   allocate(mem(%d))\n\n",MEM_SIZE);
   int indent = 1;
   for(unsigned i = 0;i<instr.used;i++)
   {
      switch(instr.data[i].opc)
      {
      case PTR: PRINT_INDENT(indent); fprintf(f,"ptr = ptr+(%d)\n",instr.data[i].arg0); break;
      case VAL: PRINT_INDENT(indent); fprintf(f,"mem(ptr+%d) = CHAR(ICHAR(mem(ptr+(%d)))+(%d))\n",instr.data[i].offset,instr.data[i].offset,instr.data[i].arg0); break;
      case GET_VAL: PRINT_INDENT(indent); fprintf(f,"call fget(mem(ptr+(%d)))\n",instr.data[i].offset); break;
      case PUT_VAL: PRINT_INDENT(indent); fprintf(f,"call fput(mem(ptr+(%d)))\n",instr.data[i].offset); break;
      case WHILE_START: PRINT_INDENT(indent); fprintf(f,"do while(ICHAR(mem(ptr))/=0)\n"); indent++; break;
      case WHILE_END: indent--; PRINT_INDENT(indent); fprintf(f,"end do\n"); break;
      case CLEAR: PRINT_INDENT(indent); fprintf(f,"mem(ptr+(%d)) = CHAR(0)\n",instr.data[i].offset); break;
      case EXIT: break;
      }
   }
   fprintf(f,"\n   deallocate(mem)\nend program fortran");
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -i filename -o filename [-m mode]\n"
          "   -i\tfile to compile\n"
          "   -o\toutput path\n"
          "   -m\toutput format (c;fortran)\n",
         argv[0],argv[0]);
}
//-------------------------------------
