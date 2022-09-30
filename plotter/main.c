/*
vertical plotter

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

int main(int argc, char **argv)
{
   wiringPiSetup();

   stepper_init();

   //stepper_move(0,0,3000,3000);
   stepper_linear_move_to(5,18);
   delayMicroseconds(100000);
   //stepper_move(-1024,0,3000,3000);
   //stepper_move(0,-1024,3000,3000);
   stepper_move_home();

   //gpioTerminate();
   return 0;
}
//-------------------------------------
