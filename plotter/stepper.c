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
#include <stdint.h>
#include <wiringPi.h>

#define CUTE_SYNC_IMPLEMENTATION
#define CUTE_SYNC_POSIX
#include "../external/cute_sync.h"
//-------------------------------------

//Internal includes
#include "stepper.h"
//-------------------------------------

//#defines
//-------------------------------------

//Typedefs
typedef struct
{
   int steps;
   int speed;
   int offset;
}move_linear;
//-------------------------------------

//Variables
static const int stepper_sequence[] =
{
   1, 1, 0, 0,
   0, 1, 1, 0,
   0, 0, 1, 1,
   1, 0, 0, 1,
   1, 1, 0, 0,
   0, 1, 1, 0,
   0, 0, 1, 1,
   1, 0, 0, 1,
};

static int64_t steps[2] = {0,0};
static cute_cv_t wait_cv;
static cute_mutex_t wait_mutex;
static int wait = 0;
//-------------------------------------

//Function prototypes
static int thread_move(void *udata);
//-------------------------------------

//Function implementations

void stepper_init(void)
{
   wait_cv = cute_cv_create();
   wait_mutex = cute_mutex_create();

   pinMode(22,OUTPUT);
   pinMode(23,OUTPUT);
   pinMode(24,OUTPUT);
   pinMode(25,OUTPUT);
   pinMode(26,OUTPUT);
   pinMode(27,OUTPUT);
   pinMode(28,OUTPUT);
   pinMode(29,OUTPUT);
}

void stepper_move(int step0, int step1, int speed0, int speed1)
{
   move_linear *m0 = malloc(sizeof(*m0));
   move_linear *m1 = malloc(sizeof(*m1));
   m0->steps = step0;
   m0->speed = speed0;
   m0->offset = 22;
   m1->steps = step1;
   m1->speed = speed1;
   m1->offset = 26;

   steps[0]+=m0->steps;
   steps[1]+=m1->steps;
   wait = 1;
   cute_thread_t *t0 = cute_thread_create(thread_move,"(null)",m0);
   cute_thread_t *t1 = cute_thread_create(thread_move,"(null)",m1);

   delayMicroseconds(100000);
   cute_lock(&wait_mutex);
   wait = 0;
   cute_cv_wake_all(&wait_cv);
   cute_unlock(&wait_mutex);

   cute_thread_wait(t0);
   cute_thread_wait(t1);

   free(m0);
   free(m1);
}

void stepper_linear_move_to(int x, int y)
{
}

void stepper_move_home(void)
{
   stepper_move(-steps[0],-steps[1],3000,3000);
}

static int thread_move(void *udata)
{
   move_linear *mv = udata;
   
   cute_lock(&wait_mutex);
   while(wait)
      cute_cv_wait(&wait_cv,&wait_mutex);
   cute_unlock(&wait_mutex);

   puts("RUN");
   int phase = 0;
   for(int i = 0;i<abs(mv->steps);i++)
   {
      if(mv->steps<0)
      {
         phase--;
         if(phase<0)
            phase = 7;
      }
      else
      {
         phase++;
         if(phase==8)
            phase = 0;
      }
      
      for(int j = 0;j<4;j++)
        digitalWrite(mv->offset+j,stepper_sequence[phase*4+j]);
      delayMicroseconds(mv->speed);
   }

   for(int j = 0;j<4;j++)
     digitalWrite(mv->offset+j,0);

   return 0;
}
//-------------------------------------
