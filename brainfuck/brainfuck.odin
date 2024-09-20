package main

import "core:fmt"
import "core:os"
import "core:io"
import "core:mem"
import "core:bufio"

MEM_SIZE:: 1<<24

OP_PTR :: 0
OP_VAL :: 1
OP_GET_VAL :: 2
OP_PUT_VAL :: 3
OP_WHILE_START :: 4
OP_WHILE_END :: 5
OP_CLEAR :: 6
OP_EXIT :: 7

Instruction :: struct
{
   opcode: u8,
   arg: i32,
}

memory: [MEM_SIZE]i8

compile :: proc(code: ^[dynamic]Instruction, reader: ^bufio.Reader)
{
   for
   {
      r: rune
      size: int
      err: io.Error
      r,size,err = bufio.reader_read_rune(reader)
      if err != nil
      {
         break
      }

      instr: Instruction
      switch r
      {
      case '>': 
         instr.opcode = OP_PTR
         instr.arg = 1
         append(code,instr)
      case '<': 
         instr.opcode = OP_PTR
         instr.arg = -1
         append(code,instr)
      case '+': 
         instr.opcode = OP_VAL
         instr.arg = 1
         append(code,instr)
      case '-': 
         instr.opcode = OP_VAL
         instr.arg = -1
         append(code,instr)
      case ',': 
         instr.opcode = OP_GET_VAL
         instr.arg = 0
         append(code,instr)
      case '.': 
         instr.opcode = OP_PUT_VAL
         instr.arg = 0
         append(code,instr)
      case '[': 
         instr.opcode = OP_WHILE_START
         instr.arg = 0
         append(code,instr)
      case ']': 
         instr.opcode = OP_WHILE_END
         instr.arg = 0
         append(code,instr)
      }
   }

   instr: Instruction
   instr.opcode = OP_EXIT
   instr.arg = 0
   append(code,instr)
}

optimize :: proc(code: ^[dynamic]Instruction)
{
   code2 : [dynamic]Instruction
   defer delete(code2)

   //Pass 1 --> rle
   //Additions and subtractions of pointer/value
   //get rle encoded
   //i.e.: +++++++ gets converted to *ptr+=7
   //-------------------------------------
   for i:=0; i<len(code);
   {
      switch(code[i].opcode)
      {
      case OP_PTR:
         instr:= Instruction{opcode = OP_PTR,arg = 0}
         for code[i].opcode==OP_PTR
         {
            instr.arg+=code[i].arg
            i+=1
         }
         append(&code2,instr)
      case OP_VAL:
         instr:= Instruction{opcode = OP_VAL,arg = 0}
         for code[i].opcode==OP_VAL
         {
            instr.arg+=code[i].arg
            i+=1
         }
         append(&code2,instr)
      case:
         append(&code2,code[i])
         i+=1
      }
   }
   //-------------------------------------

   //Pass 2 --> common patterns
   //Some snippets of code are reused often
   //Some of these get taken care of here
   //Currently implemented:
   //[-] Clears the cell to zero
   //-------------------------------------
   clear(code) 
   for i:=0; i<len(code2);
   {
      if i<len(code2)-3 &&
         code2[i].opcode==OP_WHILE_START &&
         code2[i+1].opcode==OP_VAL &&
         code2[i+2].opcode==OP_WHILE_END
      {
         append(code,Instruction{opcode=OP_CLEAR, arg = 0})
         i+=3
      }
      else
      {
         append(code,code2[i])
         i+=1
      }
   }
   //-------------------------------------

   //Pass 3 --> while
   //Find matching brackets
   //and store their positions
   //Needs to be done last
   //since memory layout of instructions
   //changes in previous passes
   //-------------------------------------
   resize(code,len(code2))
   copy(code[:],code2[:])
   for i:=0; i<len(code);i+=1
   {
      if code[i].opcode==OP_WHILE_START
      {
         balance: int = 1
         sptr: int = i+1

         for balance != 0
         {
            if code[sptr].opcode == OP_WHILE_START do balance+=1
            if code[sptr].opcode == OP_WHILE_END do balance-=1
            sptr+=1
         }

         code[i].arg = i32(sptr)
         code[sptr-1].arg = i32(i)
      }
   }
   //-------------------------------------
}

run :: proc(code: ^[dynamic]Instruction) #no_bounds_check
{
   ip: int = 0
   ptr: ^i8 = &memory[0]

   for
   {
      switch(code[ip].opcode)
      {
      case OP_PTR:
         ptr = mem.ptr_offset(ptr,code[ip].arg)
         ip+=1
      case OP_VAL:
         ptr^+=i8(code[ip].arg)
         ip+=1
      case OP_PUT_VAL:
         fmt.printf("%v",rune(ptr^))
         ip+=1
      case OP_GET_VAL:
         ip+=1
      case OP_CLEAR:
         ptr^ = 0
         ip+=1
      case OP_WHILE_START:
         if ptr^==0 do ip = int(code[ip].arg)
         else do ip+=1
      case OP_WHILE_END:
         ip = int(code[ip].arg)
      case OP_EXIT:
         return
      }
   }
}

main :: proc()
{
   code: [dynamic]Instruction 
   defer delete(code)

   f: os.Handle
   ferr: os.Error
   f,ferr = os.open("mandelbrot.bf")
   if ferr != 0 do return
   defer os.close(f)

   r: bufio.Reader
   bufio.reader_init(&r,os.stream_from_handle(f))
   defer bufio.reader_destroy(&r)


   compile(&code,&r)
   optimize(&code)
   run(&code)
}
