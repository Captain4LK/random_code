/*
plotter

Written in 2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wiringPi.h>
//-------------------------------------

//Internal includes
#include "gcode.h"
#include "stepper.h"
//-------------------------------------

//#defines
//-------------------------------------

//Typedefs
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
//-------------------------------------

//Function implementations

void gcode_run(const char *path)
{
   FILE *f = fopen(path,"r");
   int down = 0;

   char buffer[1024];
   while(fgets(buffer,1024,f))
   {
      const char *delim = " \n";
      char *tok = strtok(buffer,delim);

      //Move linear
      if(strcmp(tok,"G1")==0)
      {
         float x = 0,y = 0;
         while((tok = strtok(NULL,delim))!=NULL)
         {
            if(tok[0]=='X')
               sscanf(tok,"X%f",&x);
            if(tok[0]=='Y')
               sscanf(tok,"Y%f",&y);
         }
      
         stepper_linear_move_to(x,y);
      }
      //Lift pen
      else if(strcmp(tok,"M5;")==0&&down)
      {
         down = 0;
         stepper_move(0,0,128,0,0,3000);
      }
      else if(strcmp(tok,"M6;")==0&&!down)
      {
         down = 1;
         stepper_move(0,0,-128,0,0,3000);
      }
   }

   if(down)
      stepper_move(0,0,128,0,0,3000);

   fclose(f);
}
//-------------------------------------
