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
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
#define MEM_SIZE (1<<24)

//Universal dynamic array
#define dyn_array_init(type, array, space) \
   do { ((dyn_array *)(array))->size = (space); ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->data = malloc(sizeof(type)*(((dyn_array *)(array))->size)); } while(0)

#define dyn_array_free(type, array) \
   do { if(((dyn_array *)(array))->data) { free(((dyn_array *)(array))->data); ((dyn_array *)(array))->data = NULL; ((dyn_array *)(array))->used = 0; ((dyn_array *)(array))->size = 0; }} while(0)

#define dyn_array_add(type, array, grow, element) \
   do { ((type *)((dyn_array *)(array)->data))[((dyn_array *)(array))->used] = element; ((dyn_array *)(array))->used++; if(((dyn_array *)(array))->used==((dyn_array *)(array))->size) { ((dyn_array *)(array))->size+=grow; ((dyn_array *)(array))->data = realloc(((dyn_array *)(array))->data,sizeof(type)*(((dyn_array *)(array))->size)); } } while(0)

#define dyn_array_element(type, array, index) \
   (((type *)((dyn_array *)(array)->data))[index])

#define PRINT_INDENT(a) \
 for(int print_indent_o = 0;print_indent_o<(a);print_indent_o++) fprintf(out,"   ");
//-------------------------------------

//Typedefs
typedef enum 
{
   PTR = 1, VAL = 2, GET_VAL = 3, PUT_VAL = 4, WHILE_START = 5, WHILE_END = 6,
}Opcode;

typedef struct
{
   Opcode opc;
   int arg;
}Instruction;

typedef struct
{
   uint32_t used;
   uint32_t size;
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
//-------------------------------------

//Function implementations

int main(int argc, char *argv[])
{
   //Parse cmd arguments
   const char *path = NULL;
   const char *path_out = NULL;
   for(int i = 1;i<argc;i++)
   {
      if(argv[i][0]=='-')
      {
         switch(argv[i][1])
         {
         case 'h': puts("brainfuck to c compiler\nAvailible commandline options:\n\t-h\t\tprint this text\n\t-i [PATH]\tspecify the file to compile\n\t-o [PATH]\tspecify the output path"); break;
         case 'i': path = argv[++i]; break;
         case 'o': path_out = argv[++i]; break;
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
   optimize();

   //'Compile' code
   FILE *out = stdout;
   if(path_out!=NULL)
      out = fopen(path_out,"w");
   fprintf(out,"#include <stdio.h>\n#include <stdint.h>\n\nuint8_t mem[%d];\nuint8_t *ptr = mem;\n\nint main(int argc, char **argv)\n{\n   FILE *in = stdin;\n   if(argc>1)\n      in = fopen(argv[1],\"r\");\n\n",MEM_SIZE);
   int indent = 1;
   for(int i = 0;i<instr_array.used;i++)
   {
      switch(instr[i].opc)
      {
      case PTR: PRINT_INDENT(indent); fprintf(out,"ptr+=%d;\n",instr[i].arg); break;
      case VAL: PRINT_INDENT(indent); fprintf(out,"*ptr+=%d;\n",instr[i].arg); break;
      case GET_VAL: PRINT_INDENT(indent); fprintf(out,"*ptr = fgetc(in);\n"); break;
      case PUT_VAL: PRINT_INDENT(indent); fprintf(out,"putchar(*ptr);\n"); break;
      case WHILE_START: PRINT_INDENT(indent); fprintf(out,"while(*ptr)\n"); PRINT_INDENT(indent); fprintf(out,"{\n"); indent++; break;
      case WHILE_END: indent--; PRINT_INDENT(indent); fprintf(out,"}\n"); break;
      }
   }
   fprintf(out,"\n   if(argc>1)\n      fclose(in);\n   return 0;\n}");
   if(path_out!=NULL)
      fclose(out);
   dyn_array_free(Instruction,&instr_array);

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
      case '>': next.opc = PTR; next.arg = 1; break;
      case '<': next.opc = PTR; next.arg = -1;break;
      case '+': next.opc = VAL; next.arg = 1;break;
      case '-': next.opc = VAL; next.arg = -1;break;
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

static void optimize()
{
   dyn_array old = instr_array;
   dyn_array_init(Instruction,&instr_array,256);

   int i = 0;
   int end = old.used;
   while(i<end)
   {
      switch(instr[i].opc)
      {
      //Non optimized instructions
      case GET_VAL: dyn_array_add(Instruction,&instr_array,256,instr[i]); i++; break;
      case PUT_VAL: dyn_array_add(Instruction,&instr_array,256,instr[i]); i++; break;
      case WHILE_START: dyn_array_add(Instruction,&instr_array,256,instr[i]); i++; break;
      case WHILE_END: dyn_array_add(Instruction,&instr_array,256,instr[i]); i++; break;

      //Optimized instructions
      case PTR:
         {
            int arg = 0;
            while(instr[i].opc==PTR) { arg+=instr[i].arg; i++; }
            Instruction in = {.opc = PTR, .arg = arg};
            dyn_array_add(Instruction,&instr_array,256,in);
         }
         break;
      case VAL:
         {
            int arg = 0;
            while(instr[i].opc==VAL) { arg+=instr[i].arg; i++; }
            Instruction in = {.opc = VAL, .arg = arg};
            dyn_array_add(Instruction,&instr_array,256,in);
         }
         break;
      }
   }

   dyn_array_free(Instruction,&old);
   printf("Optimization overview:\n\tInput length: %d\n\tOutput length: %d\n",old.used,instr_array.used);
   instr = (Instruction *)instr_array.data;
}
//-------------------------------------
