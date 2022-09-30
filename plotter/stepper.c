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
static float pos[2] = {39,34}; //53:11 fixed point number (one rotation is 2048 --> 11 bits)
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

   delayMicroseconds(50000);
   cute_lock(&wait_mutex);
   wait = 0;
   cute_cv_wake_all(&wait_cv);
   cute_unlock(&wait_mutex);

   cute_thread_wait(t0);
   cute_thread_wait(t1);

   free(m0);
   free(m1);
}

void stepper_linear_move_to(float x, float y)
{
   /*float wl = x;
   float wr = 68.f-x;
   float h = y;

   float ll = sqrt(wl*wl+h*h);
   float lr = sqrt(wr*wr+h*h);

   int sl = ll/0.0065;
   int sr = lr/0.0065;*/

   //double left = pos[0]*0.0065;
   //double right = pos[1]*0.0065;
   double spos[2];
   spos[1] = pos[1];
   spos[0] = pos[0];
   double dir[2];
   dir[0] = x-spos[0];
   dir[1] = y-spos[1];

   double cpos[2];
   cpos[0] = pos[0];
   cpos[1] = pos[1];
   double wl = pos[0];
   double wr = 68.f-pos[0];
   double h = pos[1];

   double ll = sqrt(wl*wl+h*h);
   double lr = sqrt(wr*wr+h*h);

   cpos[0] = ll/0.0065;;
   cpos[1] = lr/0.0065;;
   //printf("%f %f\n",dir[0],dir[1]);
   for(double t = 0;t<=1;t+=0.01f)
   {
      double x = spos[0]+t*dir[0];
      double y = spos[1]+t*dir[1];
      double l = sqrt(pow(spos[0]+t*dir[0],2)+pow(spos[1]+t*dir[1],2));
      double r = sqrt(pow(68.0-x,2)+pow(y,2));

      double sl = l/0.0065;
      double rl = r/0.0065;
      printf("%f %f\n",sl,rl);
      int dl = sl-cpos[0];
      int dr = rl-cpos[1];
      cpos[0]=sl;
      cpos[1]=rl;
      printf("%d %d\n",dl,dr);
      stepper_move(dl,-dr,3000,3000);
      //float l = sqrt(
      /*float l = pow((1-t),2)*pos[0]+2*t*(1-t)*pos[0]+pow(t,2)*sl;
      float r = pow((1-t),2)*pos[1]+2*t*(1-t)*pos[1]+pow(t,2)*sr;

      int dl = l-cpos[0];
      int dr = r-cpos[1];
      cpos[0]=l;
      cpos[1]=r;
      //printf("%d %d\n",dl,dr);
      stepper_move(dl,-dr,3000,3000);
      //printf("%f %f\n",l,r);*/
   }
   /*int dl = sl-pos[0];
   int dr = sr-pos[1];

   int speedl = 0;
   int speedr = 0;
   if(dl>dr)
   {
      speedr = 3000;
      speedl = (abs(dr)*3000)/abs(dl);
   }
   else
   {
      speedl = 3000;
      speedr = (abs(dl)*3000)/abs(dr);
   }

   printf("%d %d %d %d\n",dl,dr,speedl,speedr);
   stepper_move(dl,-dr,speedl,speedr);*/
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
