/*
RvnicRaven portal map processing

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
//-------------------------------------

//Typedefs
typedef struct
{
   int16_t wall_count;
   int16_t wall_first;
   int32_t floor;
   int32_t ceiling;
}Sector;

typedef struct
{
   int32_t x;
   int32_t y;
   int16_t p2;
   int16_t portal;
}Wall;
//-------------------------------------

//Variables
//-------------------------------------

//Function prototypes
//-------------------------------------

//Function implementations

int main(int argc, char **argv)
{
   if(argc<3)
      return 0;

   Wall *walls = NULL;
   Sector *sectors = NULL;
   Sector *sector_current = NULL;

   //Read sectors and walls
   FILE *fin = fopen(argv[1],"r");
   char buffer[1024];
   while(fgets(buffer,sizeof(buffer),fin)!=NULL)
   {
      if(buffer[0]=='#')
         continue;

      int32_t wx,wy;
      int16_t p2,portal;
      if(sscanf(buffer,"wall %d %d %d %d\n",&wx,&wy,&portal,&p2)==4)
      {
         if(sector_current==NULL)
            continue;

         Wall wn = {0};
         wn.x = wx;
         wn.y = wy;
         wn.portal = portal;
         wn.p2 = HLH_array_length(walls)+p2;
         HLH_array_push(walls,wn);
         sector_current->wall_count++;

         continue;
      }

      int32_t sfloor,sceiling;
      if(sscanf(buffer,"sector %d %d",&sfloor,&sceiling)==2)
      {
         Sector sn = {0};
         sn.floor = sfloor;
         sn.ceiling = sceiling;
         sn.wall_first = HLH_array_length(walls);
         sn.wall_count = 0;
         HLH_array_push(sectors,sn);
         sector_current = &sectors[HLH_array_length(sectors)-1];

         continue;
      }
   }
   fclose(fin);

   //Fix sector wall winding

   //Write map
   FILE *fout = fopen(argv[2],"wb");
   fclose(fout);

   return 0;
}
//-------------------------------------
