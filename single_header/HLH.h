/*
Misc helper macros/functions/data structures etc.

Written in 2022 by Lukas Holzbeierlein (Captain4LK) email: captain4lk [at] tutanota [dot] com

To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to this software to the public domain worldwide. This software is distributed without any warranty.

You should have received a copy of the CC0 Public Domain Dedication along with this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>. 
*/

#ifndef _HLH_H_

#define _HLH_H_

#define HLH_max(a,b) ((a)>(b)?(a):(b))
#define HLH_min(a,b) ((a)<(b)?(a):(b))

typedef struct
{
   int length;
   int size;
}HLH_aheader;

static void *_HLH_array_grow(void *old, size_t size, size_t grow, size_t min);

#define HLH_array_push(a,o) (HLH_array_maygrow(a,1),(a)[HLH_array_header(a)->length++] = (o))
#define HLH_array_length(a) ((a)!=NULL?HLH_array_header(a)->length:0)
#define HLH_array_free(a) ((a)!=NULL?(free(a-HLH_array_header_offset(a)),0):0)

//Internal
#define HLH_array_header(a) ((HLH_aheader *)(a-HLH_array_header_offset(a)))
#define HLH_array_header_offset(a) ((sizeof(HLH_aheader)*2-1)/sizeof(*a))
#define HLH_array_maygrow(a,n) (((a)==NULL||HLH_array_header(a)->length+n>HLH_array_header(a)->size)?(HLH_array_grow(a,n,0),0):0)
#define HLH_array_grow(a,n,min) ((a) = _HLH_array_grow(a,sizeof(*a),n,min))

#endif

#ifdef HLH_IMPLEMENTATION
#ifndef HLH_IMPLEMENTATION_ONCE
#define HLH_IMPLEMENTATION_ONCE

static void *_HLH_array_grow(void *old, size_t size, size_t grow, size_t min)
{
   size_t header_off = (sizeof(HLH_aheader)*2-1)/size;

   if(min<4)
      min = 4;
   if(old==NULL)
   {
      //goodbye c++
      char *new = malloc(header_off*size+min*size);
      HLH_aheader *h = (HLH_aheader *)new;
      h->length = 0;
      h->size = min;

      return new+header_off*size;
   }

   HLH_aheader *h = (HLH_aheader *)(((char *)old)-header_off*size);
   if(h->size<min)
      h->size = min;
   while(h->size<h->length+grow)
      h->size*=2;
   h = realloc(h,header_off*size+h->size*size);

   return ((char *)h)+header_off*size;
}

#endif
#endif
