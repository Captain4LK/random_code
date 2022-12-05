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
#include <stdint.h>
#include <math.h>
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
   int phase;
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
static int phases[3];
static double pos[2] = {0,0};
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

   pinMode(0,OUTPUT);
   pinMode(1,OUTPUT);
   pinMode(2,OUTPUT);
   pinMode(3,OUTPUT);

   pinMode(22,OUTPUT);
   pinMode(23,OUTPUT);
   pinMode(24,OUTPUT);
   pinMode(25,OUTPUT);

   pinMode(26,OUTPUT);
   pinMode(27,OUTPUT);
   pinMode(28,OUTPUT);
   pinMode(29,OUTPUT);
}

void stepper_move(int step0, int step1, int step2, int speed0, int speed1, int speed2)
{
   move_linear *m0 = malloc(sizeof(*m0));
   move_linear *m1 = malloc(sizeof(*m1));
   move_linear *m2 = malloc(sizeof(*m2));
   m0->steps = step0;
   m0->speed = speed0;
   m0->offset = 22;
   m0->phase = phases[0];
   m1->steps = step1;
   m1->speed = speed1;
   m1->offset = 26;
   m1->phase = phases[1];
   m2->steps = step2;
   m2->speed = speed2;
   m2->offset = 0;
   m2->phase = phases[2];

   cute_lock(&wait_mutex);
   steps[0]+=m0->steps;
   steps[1]+=m1->steps;
   wait = 1;
   cute_thread_t *t0 = cute_thread_create(thread_move,"(null)",m0);
   cute_thread_t *t1 = cute_thread_create(thread_move,"(null)",m1);
   cute_thread_t *t2 = cute_thread_create(thread_move,"(null)",m2);

   //delayMicroseconds(50000);
   wait = 0;
   cute_cv_wake_all(&wait_cv);
   cute_unlock(&wait_mutex);

   cute_thread_wait(t0);
   cute_thread_wait(t1);
   cute_thread_wait(t2);

   phases[0] = m0->phase;
   phases[1] = m1->phase;
   phases[2] = m2->phase;

   free(m0);
   free(m1);
   free(m2);
}

void stepper_linear_move_to(float x, float y)
{
   double dx = x-pos[0];
   double dy = y-pos[1];

   double sx = (dx/55)*2048;
   double sy = (dy/55)*2048;

   int speed0;
   int speed1;

   if(fabs(sx)>fabs(sy))
   {
      speed0 = 6000;
      speed1 = (fabs(sx)/fabs(sy))*6000;
   }
   else
   {
      speed1 = 6000;
      speed0 = (fabs(sy)/fabs(sx))*6000;
   }

   stepper_move((int)-sx,(int)-sy,0,speed0,speed1,0);
   pos[0] = -steps[0]*0.02685546875;
   pos[1] = -steps[1]*0.02685546875;
}

void stepper_move_home(void)
{
   stepper_move(-steps[0],-steps[1],0,3000,3000,0);
}

static int thread_move(void *udata)
{
   move_linear *mv = udata;
   
   cute_lock(&wait_mutex);
   while(wait)
      cute_cv_wait(&wait_cv,&wait_mutex);
   cute_unlock(&wait_mutex);

   for(int i = 0;i<abs(mv->steps);i++)
   {
      if(mv->steps<0)
      {
         mv->phase--;
         if(mv->phase<0)
            mv->phase = 7;
      }
      else
      {
         mv->phase++;
         if(mv->phase==8)
            mv->phase = 0;
      }
      
      for(int j = 0;j<4;j++)
        digitalWrite(mv->offset+j,stepper_sequence[mv->phase*4+j]);
      delayMicroseconds(mv->speed);
   }

   for(int j = 0;j<4;j++)
     digitalWrite(mv->offset+j,0);

   return 0;
}
//-------------------------------------
