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
#include <inttypes.h>

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
   int grid = 65536;
   while(fgets(buffer,sizeof(buffer),fin)!=NULL)
   {
      if(buffer[0]=='#')
         continue;

      if(sscanf(buffer," grid %d",&grid)==1)
         continue;

      int32_t wx,wy;
      int16_t p2,portal;
      if(sscanf(buffer," wall %" SCNd32 " %" SCNd32 " %" SCNd16 " %" SCNd16 "",&wx,&wy,&portal,&p2)==4)
      {
         if(sector_current==NULL)
            continue;

         Wall wn = {0};
         wn.x = wx*grid;
         wn.y = wy*grid;
         wn.portal = portal;
         wn.p2 = HLH_array_length(walls)+p2;
         HLH_array_push(walls,wn);
         sector_current->wall_count++;

         continue;
      }

      int32_t sfloor,sceiling;
      if(sscanf(buffer," sector %" SCNd32 " %" SCNd32 "",&sfloor,&sceiling)==2)
      {
         Sector sn = {0};
         sn.floor = sfloor*grid;
         sn.ceiling = sceiling*grid;
         sn.wall_first = HLH_array_length(walls);
         sn.wall_count = 0;
         HLH_array_push(sectors,sn);
         sector_current = &sectors[HLH_array_length(sectors)-1];

         continue;
      }
   }
   fclose(fin);

   //Fix sector wall winding
   for(int i = 0;i<HLH_array_length(sectors);i++)
   {
      Sector *s = sectors+i;
      int wall = 0;
      while(wall<s->wall_count)
      {
         int64_t sum = 0;
         int first = wall;

         for(;wall==0||walls[s->wall_first+wall-1].p2==s->wall_first+wall;wall++)
         {
            int64_t x0 = walls[s->wall_first+wall].x;
            int64_t y0 = walls[s->wall_first+wall].y;
            int64_t x1 = walls[walls[s->wall_first+wall].p2].x;
            int64_t y1 = walls[walls[s->wall_first+wall].p2].y;
            printf("%d %d %d %d\n",x0,y0,x1,y1);
            sum+=(x1-x0)*(y0+y1);
         }

         printf("%d\n",sum);

         if((first==0&&sum>0)||(first!=0&&sum<0))
         {
            //Need reversing
            for(int j = 0;j<(wall-first)/2;j++)
            {
               Wall tmp = walls[s->wall_first+first+j];
               int pt20 = tmp.p2;
               int pt21 = walls[s->wall_first+wall-j-1].p2;
               walls[s->wall_first+first+j] = walls[s->wall_first+wall-j-1];

               walls[s->wall_first+wall-j-1] = tmp;
               walls[s->wall_first+first+j].p2 = pt20;
               walls[s->wall_first+wall-j-1].p2 = pt21;
            }
         }

         wall++;
      }
   }

   for(int i = 0;i<HLH_array_length(walls);i++)
   {
      int64_t x0 = walls[i].x;
      int64_t y0 = walls[i].y;
      int64_t x1 = walls[walls[i].p2].x;
      int64_t y1 = walls[walls[i].p2].y;
      printf("%d %d %d %d\n",x0,y0,x1,y1);
   }

   //Write map
   //-------------------------------------
   FILE *fout = fopen(argv[2],"wb");

   //Write sectors
   uint32_t sector_count = HLH_array_length(sectors);
   fwrite(&sector_count,1,sizeof(sector_count),fout);
   for(int i = 0;i<HLH_array_length(sectors);i++)
   {
      int16_t t16 = sectors[i].wall_count;
      fwrite(&t16,1,sizeof(t16),fout);
      t16 = sectors[i].wall_first;
      fwrite(&t16,1,sizeof(t16),fout);
      int32_t t32 = sectors[i].floor;
      fwrite(&t32,1,sizeof(t32),fout);
      t32 = sectors[i].ceiling;
      fwrite(&t32,1,sizeof(t32),fout);
   }

   //Write walls
   uint32_t wall_count = HLH_array_length(walls);
   fwrite(&wall_count,1,sizeof(wall_count),fout);
   for(int i = 0;i<HLH_array_length(walls);i++)
   {
      int32_t t32 = walls[i].x;
      fwrite(&t32,1,sizeof(t32),fout);
      t32 = walls[i].y;
      fwrite(&t32,1,sizeof(t32),fout);
      int16_t t16 = walls[i].p2;
      fwrite(&t16,1,sizeof(t16),fout);
      t16 = walls[i].portal;
      fwrite(&t16,1,sizeof(t16),fout);
   }

   fclose(fout);
   //-------------------------------------

   return 0;
}
//-------------------------------------
