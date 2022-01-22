/*
brainfuck interpreter

Written in 2021,2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

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

//Configuration:

//Memory size
#define MEM_SIZE (1<<24)

//GCC-only optimization
#define COMPUTED_GOTO 0

//Macros:

#define READ_ARG(I) \
   ((++(I))<argc?argv[(I)]:NULL)

#define ABS(a) ((a)<0?(-a):(a))

//Hacky remapping
#if !COMPUTED_GOTO
#define case_OP_PTR case OP_PTR
#define case_OP_VAL case OP_VAL
#define case_OP_VAL_OFF case OP_VAL_OFF
#define case_OP_GET_VAL case OP_GET_VAL
#define case_OP_GET_VAL_OFF case OP_GET_VAL_OFF
#define case_OP_PUT_VAL case OP_PUT_VAL
#define case_OP_PUT_VAL_OFF case OP_PUT_VAL_OFF
#define case_OP_CLEAR case OP_CLEAR
#define case_OP_CLEAR_OFF case OP_CLEAR_OFF
#define case_OP_WHILE_START case OP_WHILE_START
#define case_OP_WHILE_END case OP_WHILE_END
#define case_OP_EXIT case OP_EXIT
#endif
//-------------------------------------

//Typedefs
typedef enum 
{
   OP_PTR = 0,

   OP_VAL = 1,
   OP_VAL_OFF = 2,

   OP_GET_VAL = 3,
   OP_GET_VAL_OFF = 4,

   OP_PUT_VAL = 5,
   OP_PUT_VAL_OFF = 6,

   OP_CLEAR = 7,
   OP_CLEAR_OFF = 8,

   OP_WHILE_START = 9,
   OP_WHILE_END = 10,

   OP_EXIT = 11,
}Opcode;

typedef struct
{
   uint8_t *code;
   unsigned code_used;
   unsigned code_size;
}Bytecode;
//-------------------------------------

//Variables
static uint8_t mem[MEM_SIZE] = {0};
static uint8_t *ptr = mem;
//-------------------------------------

//Function prototypes
static void compile(FILE *in, Bytecode *code);
static void optimize(Bytecode *code);
static void print_help(char **argv);

static void bytecode_init(Bytecode *code);
static void bytecode_write(Bytecode *code, uint8_t byte);
static void bytecode_free(Bytecode *code);
static void bytecode_disassemble(const Bytecode *code);
static void bytecode_run(const Bytecode *code, FILE *input);

static void dump_bf(const Bytecode *code);
static void dump_c(const Bytecode *code);
//-------------------------------------

//Function implementations

int main(int argc, char *argv[])
{
   //Parse cmd arguments
   const char *path = NULL;
   const char *path_io = NULL;
   const char *dump = NULL;
   for(int i = 1;i<argc;i++)
   {
      if(strcmp(argv[i],"--help")==0||
         strcmp(argv[i],"-help")==0||
         strcmp(argv[i],"-h")==0||
         strcmp(argv[i],"?")==0)
         print_help(argv);
      else if(strcmp(argv[i],"-f")==0)
         path = READ_ARG(i);
      else if(strcmp(argv[i],"-i")==0)
         path_io = READ_ARG(i);
      else if(strcmp(argv[i],"-dump")==0)
         dump = READ_ARG(i);
   }

   if(path==NULL)
   {
      printf("No input file specified, try %s -h for help\n",argv[0]);
      return 0;
   }

   Bytecode code = {0};
   bytecode_init(&code);

   //Read input file, remove all invalid characters, compile code
   FILE *in = fopen(path,"r");
   compile(in,&code);
   fclose(in);

   //Performe some optimizations
   optimize(&code);

   //Dump code in desired format (if at all)
   if(dump!=NULL)
   {
      if(strcmp(dump,"IR")==0)
         bytecode_disassemble(&code);
      else if(strcmp(dump,"bf")==0)
         dump_bf(&code);
      else if(strcmp(dump,"C")==0)
         dump_c(&code);

      return 0;
   }

   //Change input stream if specified in arguments
   FILE *input = stdin;
   if(path_io!=NULL)
      input = fopen(path_io,"r");

   bytecode_run(&code,input);

   //Cleanup
   bytecode_free(&code);

   return 0;
}

static void compile(FILE *in, Bytecode *code)
{
   while(!feof(in)&&!ferror(in))
   {
      switch(fgetc(in))
      {
      case '>': bytecode_write(code,OP_PTR); bytecode_write(code,1); break;
      case '<': bytecode_write(code,OP_PTR); bytecode_write(code,-1); break;
      case '+': bytecode_write(code,OP_VAL); bytecode_write(code,1); break;
      case '-': bytecode_write(code,OP_VAL); bytecode_write(code,-1); break;
      case ',': bytecode_write(code,OP_GET_VAL); break;
      case '.': bytecode_write(code,OP_PUT_VAL); break;
      case '[': bytecode_write(code,OP_WHILE_START); bytecode_write(code,0); bytecode_write(code,0); bytecode_write(code,0); bytecode_write(code,0); break;
      case ']': bytecode_write(code,OP_WHILE_END); bytecode_write(code,0); bytecode_write(code,0); bytecode_write(code,0); bytecode_write(code,0); break;
      }
   }

   bytecode_write(code,OP_EXIT);
}

static void optimize(Bytecode *code)
{
   unsigned fast = 0;
   unsigned end = 0;
   int arg = 0;

   //Pass 1 --> rle
   //Additions and subtractions of pointer/value
   //get rle encoded
   //i.e.: +++++++ gets converted to *ptr+=7
   end = code->code_used;
   code->code_used = 0;
   for(fast = 0;fast<end;)
   {
      switch(code->code[fast])
      {
      case OP_PTR:
         arg = 0;
         while(code->code[fast]==OP_PTR&&arg!=INT8_MIN&&arg!=INT8_MAX) { arg+=(int8_t)code->code[fast+1]; fast+=2; }
         bytecode_write(code,OP_PTR);
         bytecode_write(code,arg);
         break;
      case OP_VAL:
         arg = 0;
         while(code->code[fast]==OP_VAL&&arg!=INT8_MIN&&arg!=INT8_MAX) { arg+=(int8_t)code->code[fast+1]; fast+=2; }
         bytecode_write(code,OP_VAL);
         bytecode_write(code,arg);
         break;
      case OP_GET_VAL:
      case OP_PUT_VAL:
      case OP_EXIT:
         bytecode_write(code,code->code[fast++]);
         break;
      case OP_WHILE_START:
      case OP_WHILE_END:
         bytecode_write(code,code->code[fast++]);
         bytecode_write(code,code->code[fast++]);
         bytecode_write(code,code->code[fast++]);
         bytecode_write(code,code->code[fast++]);
         bytecode_write(code,code->code[fast++]);
         break;
      }
   }
   //-------------------------------------

   //Pass 2 --> common patterns
   //Some snippets of code are reused often
   //Some of these get taken care of here
   //Currently implemented:
   //[-] Clears the cell to zero
   end = code->code_used;
   code->code_used = 0;
   for(fast = 0;fast<end;)
   {
      if(fast<end-12&&code->code[fast]==OP_WHILE_START&&code->code[fast+5]==OP_VAL&&code->code[fast+7]==OP_WHILE_END)
      {
         bytecode_write(code,OP_CLEAR);
         fast+=12;
      }
      else
      {
         switch(code->code[fast])
         {
         case OP_GET_VAL:
         case OP_PUT_VAL:
         case OP_EXIT:
            bytecode_write(code,code->code[fast++]);
            break;
         case OP_VAL:
         case OP_PTR:
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            break;
         case OP_WHILE_START:
         case OP_WHILE_END:
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            break;
         }
      }
   }
   //-------------------------------------

   //Pass 3 --> offsets
   //Sometimes the pointer gets moved to
   //a specific location and back for a single command,
   //this makes such occurrences execute
   //as a single execution
   //Currently implemented: '+';',';'.';'[-]'
   end = code->code_used;
   code->code_used = 0;
   for(fast = 0;fast<end;)
   {
      if(fast<end-5&&code->code[fast]==OP_PTR&&code->code[fast+2]!=OP_WHILE_START&&code->code[fast+2]!=OP_WHILE_END&&code->code[fast+2]!=OP_PTR&&code->code[fast+2]!=OP_VAL&&code->code[fast+3]==OP_PTR&&((int)(int8_t)code->code[fast+1])==-((int)(int8_t)code->code[fast+4]))
      {
         switch(code->code[fast+2])
         {
         case OP_CLEAR: bytecode_write(code,OP_CLEAR_OFF); bytecode_write(code,code->code[fast+1]); break;
         case OP_GET_VAL: bytecode_write(code,OP_GET_VAL_OFF); bytecode_write(code,code->code[fast+1]); break;
         case OP_PUT_VAL: bytecode_write(code,OP_PUT_VAL_OFF); bytecode_write(code,code->code[fast+1]); break;
         }
         fast+=5;
      }
      else if(fast<end-6&&code->code[fast]==OP_PTR&&code->code[fast+2]==OP_VAL&&code->code[fast+4]==OP_PTR&&((int)(int8_t)code->code[fast+1])==-((int)(int8_t)code->code[fast+5]))
      {
         bytecode_write(code,OP_VAL_OFF);
         bytecode_write(code,code->code[fast+1]);
         bytecode_write(code,code->code[fast+3]);
         fast+=6;
      }
      else
      {
         switch(code->code[fast])
         {
         case OP_VAL:
         case OP_PTR:
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            break;
         case OP_GET_VAL:
         case OP_PUT_VAL:
         case OP_CLEAR:
         case OP_EXIT:
            bytecode_write(code,code->code[fast++]);
            break;
         case OP_WHILE_START:
         case OP_WHILE_END:
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            bytecode_write(code,code->code[fast++]);
            break;
         }
      }
   }
   //-------------------------------------

   //Pass 4 --> while
   //Find matching brackets
   //and store their positions
   //Needs to be done last
   //since memory layout of instructions
   //changes in previous passes
   end = code->code_used;
   for(fast = 0;fast<end;)
   {
      if(code->code[fast]==OP_WHILE_START)
      {
         int sptr = fast+5;
         int balance = 1;

         while(balance)
         {
            switch(code->code[sptr])
            {
            case OP_VAL:
            case OP_PTR:
            case OP_CLEAR_OFF:
            case OP_GET_VAL_OFF:
            case OP_PUT_VAL_OFF:
               sptr+=2;
               break;
            case OP_GET_VAL:
            case OP_PUT_VAL:
            case OP_CLEAR:
            case OP_EXIT:
               sptr+=1;
               break;
            case OP_VAL_OFF:
               sptr+=3;
               break;
            case OP_WHILE_START:
               balance++;
               sptr+=5;
               break;
            case OP_WHILE_END:
               balance--;
               sptr+=5;
               break;
            }
         }

         *((int32_t *)&code->code[fast+1]) = sptr;
         *((int32_t *)&code->code[sptr-4]) = fast;
      }

      switch(code->code[fast])
      {
      case OP_VAL_OFF:
         fast+=3;
         break;
      case OP_VAL:
      case OP_PTR:
      case OP_GET_VAL_OFF:
      case OP_PUT_VAL_OFF:
      case OP_CLEAR_OFF:
         fast+=2;
         break;
      case OP_GET_VAL:
      case OP_PUT_VAL:
      case OP_CLEAR:
      case OP_EXIT:
         fast+=1;
         break;
      case OP_WHILE_START:
      case OP_WHILE_END:
         fast+=5;
         break;
      }
   }
   //-------------------------------------
}

static void print_help(char **argv)
{
   printf("%s usage:\n"
          "%s -f filename [-i filename]\n"
          "   -f\tfile to execute\n"
          "   -i\tfile to read input from\n",
         argv[0],argv[0]);
}

static void bytecode_init(Bytecode *code)
{
   code->code = NULL;
   code->code_used = 0;
   code->code_size = 0;
}

static void bytecode_write(Bytecode *code, uint8_t byte)
{
   if(code->code==NULL)
   {
      code->code_used = 0;
      code->code_size = 256;
      code->code = malloc(sizeof(*code->code)*code->code_size);
   }

   code->code[code->code_used++] = byte;

   if(code->code_used>=code->code_size)
   {
      code->code_size+=256;
      code->code = realloc(code->code,sizeof(*code->code)*code->code_size);
   }
}

static void bytecode_free(Bytecode *code)
{
   if(code->code==NULL)
      return;

   free(code->code);
   bytecode_init(code);
}

static void bytecode_disassemble(const Bytecode *code)
{
   int end = code->code_used;
   puts("   INDEX|        OPC|     ARG|     OFF|");
   for(int i = 0;i<end;)
   {
      switch(code->code[i++])
      {
      case OP_PTR:
         printf("%8d|PTR        |%8d|        |\n",i-1,(int8_t)code->code[i]); i+=1;
         break;
      case OP_VAL:
         printf("%8d|VAL        |%8d|        |\n",i-1,(int8_t)code->code[i]); i+=1;
         break;
      case OP_VAL_OFF:
         printf("%8d|VAL_OFF    |%8d|%8d|\n",i-1,(int8_t)code->code[i+1],(int8_t)code->code[i]); i+=2;
         break;
      case OP_GET_VAL:
         printf("%8d|GET_VAL    |        |        |\n",i-1);
         break;
      case OP_GET_VAL_OFF:
         printf("%8d|GET_VAL_OFF|        |%8d|\n",i-1,(int8_t)code->code[i]); i+=1;
         break;
      case OP_PUT_VAL:
         printf("%8d|PUT_VAL    |        |        |\n",i-1);
         break;
      case OP_PUT_VAL_OFF:
         printf("%8d|PUT_VAL_OFF|        |%8d|\n",i-1,(int8_t)code->code[i]); i+=1;
         break;
      case OP_CLEAR:
         printf("%8d|CLEAR      |        |        |\n",i-1);
         break;
      case OP_CLEAR_OFF:
         printf("%8d|CLEAR_OFF  |        |%8d|\n",i-1,(int8_t)code->code[i]); i+=1;
         break;
      case OP_WHILE_START:
         printf("%8d|WHILE      |%8d|        |\n",i-1,*((int32_t *)&code->code[i])); i+=4;
         break;
      case OP_WHILE_END:
         printf("%8d|ELIHW      |%8d|        |\n",i-1,*((int32_t *)&code->code[i])); i+=4;
         break;
      case OP_EXIT:
         printf("%8d|EXIT       |        |        |\n",i-1);
         break;
      }
   }
}

static void bytecode_run(const Bytecode *code, FILE *input)
{
#if COMPUTED_GOTO
   const void *dispatch_table[] =
   {
      &&case_OP_PTR,
      &&case_OP_VAL,
      &&case_OP_VAL_OFF,
      &&case_OP_GET_VAL,
      &&case_OP_GET_VAL_OFF,
      &&case_OP_PUT_VAL,
      &&case_OP_PUT_VAL_OFF,
      &&case_OP_CLEAR,
      &&case_OP_CLEAR_OFF,
      &&case_OP_WHILE_START,
      &&case_OP_WHILE_END,
      &&case_OP_EXIT,
   };

#define DISPATCH() goto *dispatch_table[*ip++]
#else
#define DISPATCH() goto next
#endif

   uint8_t *ip = code->code;

#if COMPUTED_GOTO
   DISPATCH();
#endif

   for(;;)
   {
#if !COMPUTED_GOTO
   next:
#endif
      switch(*ip++)
      {
      case_OP_PTR: ptr+=(int8_t)(*ip++); DISPATCH();

      case_OP_VAL: *ptr+=(int8_t)(*ip++); DISPATCH();
      case_OP_VAL_OFF: *(ptr+(int8_t)(*ip))+=(int8_t)*(ip+1); ip+=2; DISPATCH();

      case_OP_GET_VAL: *ptr = fgetc(input); DISPATCH();
      case_OP_GET_VAL_OFF: *(ptr+(int8_t)(*ip++)) = fgetc(input); DISPATCH();

      case_OP_PUT_VAL: putchar(*ptr); fflush(stdout); DISPATCH();
      case_OP_PUT_VAL_OFF: putchar(*(ptr+(int8_t)(*ip++))); fflush(stdout); DISPATCH();

      case_OP_CLEAR: *ptr = 0; DISPATCH();
      case_OP_CLEAR_OFF: *(ptr+(int8_t)(*ip++)) = 0; DISPATCH();

      case_OP_WHILE_START: 
         if(!(*ptr))
            ip = code->code+(*((int32_t *)ip));
         else
            ip+=4;
         DISPATCH();
      case_OP_WHILE_END: ip = code->code+(*((int32_t *)ip)); DISPATCH();

      case_OP_EXIT: return;
      }
   }
}

static void dump_bf(const Bytecode *code)
{
   int end = code->code_used;
   for(int i = 0;i<end;)
   {
      switch(code->code[i++])
      {
      case OP_PTR:
         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'<':'>',stdout);
         i++;
         break;
      case OP_VAL:
         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'-':'+',stdout);
         i++;
         break;
      case OP_VAL_OFF:
         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'<':'>',stdout);

         for(int j = 0;j<ABS((int8_t)code->code[i+1]);j++)
            fputc((int8_t)code->code[i+1]<0?'-':'+',stdout);

         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'>':'<',stdout);

         i+=2;
         break;
      case OP_GET_VAL:
         fputc(',',stdout);
         break;
      case OP_GET_VAL_OFF:
         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'<':'>',stdout);

         fputc(',',stdout);

         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'>':'<',stdout);
         i++;
         break;
      case OP_PUT_VAL:
         fputc('.',stdout);
         break;
      case OP_PUT_VAL_OFF:
         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'<':'>',stdout);

         fputc('.',stdout);

         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'>':'<',stdout);

         i++;
         break;
      case OP_CLEAR:
         fputc('[',stdout);
         fputc('-',stdout);
         fputc(']',stdout);
         break;
      case OP_CLEAR_OFF:
         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'<':'>',stdout);

         fputc('[',stdout);
         fputc('-',stdout);
         fputc(']',stdout);

         for(int j = 0;j<ABS((int8_t)code->code[i]);j++)
            fputc((int8_t)code->code[i]<0?'>':'<',stdout);
         i++;
         break;
      case OP_WHILE_START:
         fputc('[',stdout);
         i+=4;
         break;
      case OP_WHILE_END:
         fputc(']',stdout);
         i+=4;
         break;
      }
   }
}

static void dump_c(const Bytecode *code)
{
#define PRINT_INDENT(a) \
   for(int print_indent_o = 0;print_indent_o<(a);print_indent_o++) printf("   ");

   printf("#include <stdio.h>\n#include <stdint.h>\n\nuint8_t mem[%d];\nuint8_t *ptr = mem;\n\nint main(int argc, char **argv)\n{\n   FILE *in = stdin;\n   if(argc>1)\n      in = fopen(argv[1],\"r\");\n\n",MEM_SIZE);
   int indent = 1;
   int end = code->code_used;
   for(int i = 0;i<end;)
   {
      switch(code->code[i++])
      {
      case OP_PTR: PRINT_INDENT(indent); printf("ptr+=%d;\n",(int8_t)code->code[i]); i++; break;
      case OP_VAL: PRINT_INDENT(indent); printf("*ptr+=%d;\n",(int8_t)code->code[i]); i++; break;
      case OP_VAL_OFF: PRINT_INDENT(indent); printf("*(ptr+%d)+=%d;\n",(int8_t)code->code[i],(int8_t)code->code[i+1]); i+=2; break;
      case OP_GET_VAL: PRINT_INDENT(indent); printf("*ptr = fgetc(in);\n"); break;
      case OP_GET_VAL_OFF: PRINT_INDENT(indent); printf("*(ptr+%d) = fgetc(in);\n",(int8_t)code->code[i]); i++; break;
      case OP_PUT_VAL: PRINT_INDENT(indent); printf("putchar(*ptr);\n"); break;
      case OP_PUT_VAL_OFF: PRINT_INDENT(indent); printf("putchar(*(ptr+%d));\n",(int8_t)code->code[i]); i++; break;
      case OP_CLEAR: PRINT_INDENT(indent); printf("*ptr = 0;\n"); break;
      case OP_CLEAR_OFF: PRINT_INDENT(indent); printf("*(ptr+%d) = 0;\n",(int8_t)code->code[i]); i++; break;
      case OP_WHILE_START: PRINT_INDENT(indent); printf("while(*ptr)\n"); PRINT_INDENT(indent); printf("{\n"); indent++; i+=4; break;
      case OP_WHILE_END: indent--; PRINT_INDENT(indent); printf("}\n"); i+=4; break;
      case OP_EXIT: break;
      }
   }
   printf("\n   if(argc>1)\n      fclose(in);\n\n   return 0;\n}");

#undef PRINT_INDENT
}

#undef MEM_SIZE
#undef READ_ARG
#undef ABS
//-------------------------------------
