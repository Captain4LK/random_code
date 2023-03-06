/*
Fast line sorting

Written in 2023 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HLH_IMPLEMENTATION 
#include "../single_header/HLH.h"
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
#define sign_equal(a,b) (((a)^(b))>=0)
//-------------------------------------

//Typedefs
typedef struct
{
   int32_t x0;
   int32_t y0;
   int32_t x1;
   int32_t y1;
   int32_t yfront;
}Wall;
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
static int comp_wall_front(const void *a, const void *b);
static int wall_order(const Wall *wa, const Wall *wb);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   Wall walls[6] = 
   {
      {.x0 = -64,.y0 = 0,.x1 = -48,.y1 = 64,.yfront  = 0},
      {.x0 = 64,.y0 = 0,.x1 = 48,.y1 = 64,.yfront  = 0},
      {.x0 = -48,.y0 = 64,.x1 = 48,.y1 = 64,.yfront  = 64},

      {.x0 = -32,.y0 = 16,.x1 = -24,.y1 = 32,.yfront  = 16},
      {.x0 = 24,.y0 = 16,.x1 = 32,.y1 = 32,.yfront  = 16},
      {.x0 = -24,.y0 = 16,.x1 = 24,.y1 = 16,.yfront  = 16},
   };

   int len = 6;
   qsort(walls,len,sizeof(*walls),comp_wall_front);

   Wall *stack = NULL;
   Wall *order = NULL;
   HLH_array_push(stack,walls[0]);

   for(int i = 0;i<6;i++)
      printf("(%d %d) --> (%d %d)\n",walls[i].x0,walls[i].y0,walls[i].x1,walls[i].y1);
   puts("--------------------------");

   int next = 1;

   int iters = 0;
   while(HLH_array_length(stack)>0)
   {
      iters++;
      Wall wc = stack[HLH_array_length(stack)-1];

      //Trivially accept
      if(next>=len||walls[next].yfront>wc.yfront)
      {
         HLH_array_push(order,wc);
         HLH_array_length_set(stack,HLH_array_length(stack)-1);
      }
      else
      {
         //Check order of walls
         int order = wall_order(&wc,&walls[next]);
         if(order==0||order==2)
         {
            stack[HLH_array_length(stack)-1] = walls[next];
            HLH_array_push(stack,wc);
         }
         else if(order==1)
         {
            HLH_array_push(stack,walls[next]);
         }
         next++;
      }

      if(HLH_array_length(stack)==0&&next<len)
      {
         HLH_array_push(stack,walls[next]);
         next++;
      }
   }

   for(int i = 0;i<HLH_array_length(order);i++)
      printf("(%d %d) --> (%d %d)\n",order[i].x0,order[i].y0,order[i].x1,order[i].y1);
   printf("%d Iterations\n",iters);

   return 0;
}

static int comp_wall_front(const void *a, const void *b)
{
   const Wall *wa = a;
   const Wall *wb = b;

   return wa->yfront-wb->yfront;
}

//0 --> no obstruction --> order doesn't matter
//1 --> b first
//2 --> a first
static int wall_order(const Wall *wa, const Wall *wb)
{
   int32_t x00 = wa->x0;
   int32_t y00 = wa->y0;
   int32_t x01 = wa->x1;
   int32_t y01 = wa->y1;
   int32_t x10 = wb->x0;
   int32_t y10 = wb->y0;
   int32_t x11 = wb->x1;
   int32_t y11 = wb->y1;

   //(x00/y00) is origin, all calculations centered arround it
   //t0 is relation of (b,p0) to wall a
   //t1 is relation of (b,p1) to wall a
   //See RvR_port_sector_inside for more in depth explanation
   int32_t x = x01-x00;
   int32_t y = y01-y00;
   //int32_t t0 = ((x10-x00)*y-(y10-y00)*x)/1024;
   //int32_t t1 = ((x11-x00)*y-(y11-y00)*x)/1024;
   int32_t t0 = ((x10-x00)*y-(y10-y00)*x);
   int32_t t1 = ((x11-x00)*y-(y11-y00)*x);

   //walls on the same line (identicall or adjacent)
   if(t0==0&&t1==0)
      return 0;

   //(b,p0) on extension of wall a (shared corner, etc)
   //Set t0 = t1 to trigger RvR_sign_equal check (and for the return !RvR_sign_equal to be correct)
   if(t0==0)
      t0 = t1;

   //(b,p1) on extension of wall a
   //Set t0 = t1 to trigger RvR_sign_equal check
   if(t1==0)
      t1 = t0;

   //Wall either completely to the left or to the right of other wall
   if(sign_equal(t0,t1))
   {
      //Compare with player position relative to wall a
      //if wall b and the player share the same relation, wall a needs to be drawn first
      t1 = ((0-x00)*y-(0-y00)*x)/1024;
      return (!sign_equal(t0,t1))+1;
   }

   //Extension of wall a intersects with wall b
   //--> check wall b instead
   //(x10/y10) is origin, all calculations centered arround it
   x = x11-x10;
   y = y11-y10;
   t0 = ((x00-x10)*y-(y00-y10)*x)/1024;
   t1 = ((x01-x10)*y-(y01-y10)*x)/1024;

   //(a,p0) on extension of wall b
   if(t0==0)
      t0 = t1;

   //(a,p1) on extension of wall b
   if(t1==0)
      t1 = t0;

   //Wall either completely to the left or to the right of other wall
   if(sign_equal(t0,t1))
   {
      //Compare with player position relative to wall b
      //if wall a and the player share the same relation, wall b needs to be drawn first
      t1 = ((0-x10)*y-(0-y10)*x)/1024;
      return sign_equal(t0,t1)+1;
   }

   //Invalid case (walls are intersecting), expect rendering glitches
   return 0;
}
//-------------------------------------
