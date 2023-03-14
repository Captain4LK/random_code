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
static int can_front(const Wall *wa, const Wall *wb);
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   Wall walls[7] = 
   {
      {293,39,319,32,10},
      {186,14,319,13,5},
      {179,42,186,14,4},
      {179,42,293,39,9},
      {310,55,319,52,2},
      {131,62,310,55,1},
      {0,15,131,62,0},
      /*{.x0 = -32,.y0 = 32,.x1 = -24,.y1 = 16,.yfront = 32},
      {.x0 = -64,.y0 = 0,.x1 = -48,.y1 = 64,.yfront = 64},
      {.x0 = 48,.y0 = 64,.x1 = 64,.y1 = 0,.yfront = 64},
      {.x0 = 24,.y0 = 16,.x1 = 32,.y1 = 32,.yfront =32},
      {.x0 = -56,.y0 = 16,.x1 = -50,.y1 = 16,.yfront = 16},
      {.x0 = -48,.y0 = 64,.x1 = 48,.y1 = 64,.yfront = 64},

      {.x0 = -24,.y0 = 16,.x1 = 24,.y1 = 16,.yfront = 16},
      {.x0 = -89,.y0 = 7,.x1 = -64,.y1 = 7,.yfront = 7},*/

   };

   int len = 7;
   for(int i = 0;i<len;i++)
   {
      if(walls[i].y0<walls[i].y1)
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
         if(can_front(walls+i,walls+j))
         {
            j++;
         }
         else if(i+swaps>j)
         {
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
         }
      }
   }
   for(int i = 0;i<len;i++)
      printf("(%d %d) --> (%d %d)\n",walls[i].x0,walls[i].y0,walls[i].x1,walls[i].y1);

   return 0;
}


static int comp_wall_front(const void *a, const void *b)
{
   const Wall *wa = a;
   const Wall *wb = b;

   return wa->yfront-wb->yfront;
}

static int can_front(const Wall *wa, const Wall *wb)
{
   int64_t x00 = wa->x0;
   int64_t y00 = wa->y0;
   int64_t x01 = wa->x1;
   int64_t y01 = wa->y1;
   int64_t x10 = wb->x0;
   int64_t y10 = wb->y0;
   int64_t x11 = wb->x1;
   int64_t y11 = wb->y1;

   int64_t dx0 = x01-x00;
   int64_t dy0 = y01-y00;
   int64_t dx1 = x11-x10;
   int64_t dy1 = y11-y10;

   int64_t cross00 = dx0*(y10-y00)-dy0*(x10-x00);
   int64_t cross01 = dx0*(y11-y00)-dy0*(x11-x00);
   int64_t cross10 = dx1*(y00-y10)-dy1*(x00-x10);
   int64_t cross11 = dx1*(y01-y10)-dy1*(x01-x10);

   //wb completely behind wa
   if(min(y10,y11)>max(y00,y01))
      return 1;

   //no overlap
   if(x00>=x11||x01<=x10)
      return 1;

   if(cross00>=0&&cross01>=0)
      return 1;

   if(cross10<=0&&cross11<=0)
      return 1;

   return 0;
}
//-------------------------------------
