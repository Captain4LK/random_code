/*
(Fast?) line sorting

Written in 2023 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

//External includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
//-------------------------------------

//Internal includes
//-------------------------------------

//#defines
#define sign_equal(a,b) (((a)^(b))>=0)
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))
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
   Wall walls[8] = 
   {
      {.x0 = -32,.y0 = 32,.x1 = -24,.y1 = 16,.yfront = 32},
      {.x0 = -64,.y0 = 0,.x1 = -48,.y1 = 64,.yfront = 64},
      {.x0 = 48,.y0 = 64,.x1 = 64,.y1 = 0,.yfront = 64},
      {.x0 = 24,.y0 = 16,.x1 = 32,.y1 = 32,.yfront =32},
      {.x0 = -56,.y0 = 16,.x1 = -50,.y1 = 16,.yfront = 16},
      {.x0 = -48,.y0 = 64,.x1 = 48,.y1 = 64,.yfront = 64},

      {.x0 = -24,.y0 = 16,.x1 = 24,.y1 = 16,.yfront = 16},
      {.x0 = -89,.y0 = 7,.x1 = -64,.y1 = 7,.yfront = 7},

   };

   int len = 8;
   for(int i = 0;i<len;i++)
   {
      if(walls[i].y0>walls[i].y1)
         walls[i].yfront = walls[i].y0;
      else
         walls[i].yfront = walls[i].y1;
   }

   qsort(walls,len,sizeof(*walls),comp_wall_front);
   for(int i = 0;i<len;i++)
      printf("(%d %d) --> (%d %d)\n",walls[i].x0,walls[i].y0,walls[i].x1,walls[i].y1);
   puts("---");

   for(int i = 0;i<len;i++)
   {
      int swaps = 0;

      int j = i+1;
      while(j<len)
      {
         if(wall_order(walls+i,walls+j)!=2)
         {
            j++;
         }
         else if(i+swaps>j)
         {
            puts("SHIT");
            j++;
         }
         else
         {
            Wall tmpi = walls[j];
            for(int w = j;w>i;w--)
               walls[w] = walls[w-1];
            walls[i] = tmpi;
            j = i+1;
            swaps++;
            puts("ROTATE");
         }
      }
   }
   for(int i = 0;i<len;i++)
      printf("(%d %d) --> (%d %d)\n",walls[i].x0,walls[i].y0,walls[i].x1,walls[i].y1);

   return 0;
}

//-1 no overlap
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

   //wa completely behind wb
   if(max(y00,y01)<min(y10,y11))
      return 1;

   //no overlap
   if(x00>x11||x01<x10)
      return 0;

   //if(min(y00>y11||y01<y10)
   //{
      //if(y00>y10)
         //return 1;
      //return 2;
   //}

   //walls on the same line (identicall or adjacent)
   if(t0==0&&t1==0)
      return 0;

   //(b,p0) on extension of wall a (shared corner, etc)
   //Set t0 = t1 to trigger RvR_sign_equal check (and for the return !RvR_sign_equal to be correct)
   if(t0==0)
   {
      //puts("CASE A");
      t0 = t1;
      //return 0;
   }

   //(b,p1) on extension of wall a
   //Set t0 = t1 to trigger RvR_sign_equal check
   if(t1==0)
   {
      //puts("CASE B");
      t1 = t0;
      //return 0;
   }

   //Wall either completely to the left or to the right of other wall
   if(sign_equal(t0,t1))
   {
      //Compare with player position relative to wall a
      //if wall b and the player share the same relation, wall a needs to be drawn first
      t1 = ((0-x00)*y-(0-y00)*x);
      //printf("%d %d\n",t0,t1);
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


static int comp_wall_front(const void *a, const void *b)
{
   const Wall *wa = a;
   const Wall *wb = b;

   return wb->yfront-wa->yfront;
}
//-------------------------------------
