

#define CUTE_PNG_IMPLEMENTATION
#define CUTE_PNG_ALLOC backend_malloc
#define CUTE_PNG_CALLOC backend_calloc
#define CUTE_PNG_FREE backend_free
#include "cute_png.h"
//https://github.com/RandyGaul/cute_headers

#define HLH_SLK_IMPLEMENTATION
#define HLH_SLK_MALLOC backend_malloc
#define HLH_SLK_FREE backend_free
#include "HLH_slk.h"

#define MAX_CONTROLLERS 4


static void *(*bmalloc)(size_t size) = backend_system_malloc;
static void (*bfree)(void *ptr) = backend_system_free;
static void *(*brealloc)(void *ptr, size_t size) = backend_system_realloc;

static int get_gamepad_index(int which);

//(should) center the viewport.
void backend_update_viewport()
{
 
}

//Handles window and input events.
void backend_handle_events()
{
  
}

//Creates the window, etc.
void backend_setup(int width, int height, int layer_num, const char *title, int fullscreen, int scale, int resizable)
{
  
}

//Clears the window and redraws the scene.
//Drawing is performed from back to front, layer 0 is always drawn last.
void backend_render_update()
{

}

void backend_create_layer(unsigned index, int type)
{

}

//Set the window title.
void backend_set_title(const char *title)
{

}

//Toggles fullscreen.
void backend_set_fullscreen(int fullscreen)
{
  
}

//Sets wether the window is visible.
void backend_set_visible(int visible)
{
  
}

//Sets the window icon.
void backend_set_icon(const SLK_RGB_sprite *icon)
{
 
}

//Returns the viewport width adjusted to pixel scale.
int backend_get_width()
{
   return 0;
}

//Returns the viewport height adjusted to pixel scale.
int backend_get_height()
{
   return 0;
}

//Returns the view width.
int backend_get_view_width()
{
   return 0;
}

//Returns the view height.
int backend_get_view_height()
{
   return 0;
}

//Returns the view x pos.
int backend_get_view_x()
{
   return 0;
}

//Returns the view y pos.
int backend_get_view_y()
{
   return 0;
}

//Returns the window width.
int backend_get_win_width()
{
   return 0;
}

//Returns the window height.
int backend_get_win_height()
{
   return 0;
}

//Sets the target/maximum fps.
void backend_set_fps(int FPS)
{
   
}

//Returns the current target fps.
int backend_get_fps()
{
   return 0;
}

//Limits the fps to the target fps.
void backend_timer_update()
{
   
}

//Returns the delta time of the last frame.
float backend_timer_get_delta()
{
   return 0;
}

//Creates the keymap.
void backend_input_init()
{
  
}

//Shows or hides the mouse cursor.
void backend_show_cursor(int shown)
{

}

//Sets wether the mouse cursor is captured and only relative
//mouse motion is registerd.
void backend_mouse_set_relative(int relative)
{
 }

//Sets wether to track mouse events globally.
void backend_mouse_capture(int capture)
{
  
}

//Starts text input.
void backend_start_text_input(char *text, int max_length)
{
  
}

//Stops text input.
void backend_stop_text_input()
{
 
}

int backend_key_down(int key)
{
 return 0;
}

int backend_key_pressed(int key)
{
   return 0;
}

int backend_key_released(int key)
{
   return 0;
}

SLK_Button backend_key_get_state(int key)
{
   return (SLK_Button){0};
}

int backend_mouse_down(int key)
{
   return 0;
}

int backend_mouse_pressed(int key)
{
   return 0;
}

int backend_mouse_released(int key)
{
   return 0;
}

SLK_Button backend_mouse_get_state(int key)
{
   
   return (SLK_Button){0};
}

int backend_mouse_wheel_get_scroll()
{
   return 0;
}

int backend_gamepad_down(int index, int key)
{
   return 0;
}

int backend_gamepad_pressed(int index, int key)
{
   return 0;
}

int backend_gamepad_released(int index, int key)
{
   return 0;
}

SLK_Button backend_gamepad_get_state(int index, int key)
{
  
   return (SLK_Button){0};
}

int backend_get_gamepad_count()
{
   return 0;
}

void backend_mouse_get_pos(int *x, int *y)
{
   *x = 0; 
   *y = 0; 
}

void backend_mouse_get_relative_pos(int *x, int *y)
{
   *x = 0; 
   *y = 0; 
}

static int get_gamepad_index(int which)
{
   return -1;
}

SLK_RGB_sprite *backend_load_rgb(const char *path)
{
   cp_image_t img = cp_load_png(path);
   SLK_RGB_sprite *out = NULL;
   if(img.pix==NULL)
   {
      SLK_log("failed to load png %s\n",path);
      return NULL;
   }

   out = SLK_rgb_sprite_create(img.w,img.h);
   memcpy(out->data,img.pix,img.w*img.h*sizeof(*out->data));
   cp_free_png(&img);

   return out;
}

SLK_RGB_sprite *backend_load_rgb_file(FILE *f)
{
   int size = 0;
   fseek(f,0,SEEK_END);
   size = ftell(f);
   fseek(f,0,SEEK_SET);
   char *data = backend_malloc(size+1);
   fread(data,size,1,f);
   data[size] = 0;
   SLK_RGB_sprite *out = backend_load_rgb_mem(data,size);
   backend_free(data);

   return out;
}

SLK_RGB_sprite *backend_load_rgb_mem(const void *data, int length)
{
   cp_image_t img = cp_load_png_mem(data,length);
   SLK_RGB_sprite *out = NULL;
   if(img.pix==NULL)
   {
      SLK_log("failed to load png from memory buffer");
      return NULL;
   }

   out = SLK_rgb_sprite_create(img.w,img.h);
   memcpy(out->data,img.pix,img.w*img.h*sizeof(*out->data));
   cp_free_png(&img);

   return out;
}


void backend_save_rgb(const SLK_RGB_sprite *s, const char *path)
{
   if(path==NULL)
      return;
   FILE *f = fopen(path,"wb");
   if(f==NULL)
      SLK_log("failed to open %s for writing",path);

   backend_save_rgb_file(s,f);

   fclose(f);
}

void backend_save_rgb_file(const SLK_RGB_sprite *s, FILE *f)
{
   if(f==NULL)
   {
      SLK_log("file pointer is NULL, can't write png to disk");
      return;
   }

   cp_image_t img;
   img.w = s->width;
   img.h = s->height;
   img.pix = (cp_pixel_t *)s->data;
   cp_save_png(f,&img);
}

SLK_Pal_sprite *backend_load_pal(const char *path)
{
   FILE *f = fopen(path,"rb");
   if(f==NULL)
   {
      SLK_log("failed to open %s for reading",path);
      return NULL;
   }

   SLK_Pal_sprite *out = backend_load_pal_file(f);
   fclose(f);
   return out;
}

SLK_Pal_sprite *backend_load_pal_file(FILE *f)
{
   if(f==NULL)
   {
      SLK_log("file pointer is NULL, can't read .slk file");
      return NULL;
   }

   int size = 0;
   char *data = NULL;
   fseek(f,0,SEEK_END);
   size = ftell(f);
   fseek(f,0,SEEK_SET);
   data = backend_malloc(size+1);
   fread(data,size,1,f);
   data[size] = 0;
   SLK_Pal_sprite *s = backend_load_pal_mem(data,size);
   backend_free(data);

   return s;
}

SLK_Pal_sprite *backend_load_pal_mem(const void *data, int length)
{
   HLH_slk *img = HLH_slk_image_load_mem_buffer(data,length);
   SLK_Pal_sprite *s = SLK_pal_sprite_create(img->width,img->height);
   memcpy(s->data,img->data,sizeof(*s->data)*s->width*s->height);
   HLH_slk_image_free(img);

   return s;
}

void backend_save_pal(const SLK_Pal_sprite *s, const char *path, int rle)
{
   FILE *f = fopen(path,"wb");

   if(f==NULL)
   {
      SLK_log("failed to open %s for writing",path);
      return;
   }

   backend_save_pal_file(s,f,rle);
      
   fclose(f);
}

void backend_save_pal_file(const SLK_Pal_sprite *s, FILE *f, int rle)
{
   if(f==NULL)
   {
      SLK_log("file pointer is NULL, can't write palette to disk");
      return;
   }

   HLH_slk img;
   img.width = s->width;
   img.height = s->height;
   img.data = (uint8_t *)s->data;
   HLH_slk_image_write(&img,f,rle);
}

SLK_Palette *backend_load_palette(const char *path)
{
   FILE *f = fopen(path,"r");
   if(f==NULL)
   {
      SLK_log("failed to open %s for reading",path);
      return NULL; 
   }

   SLK_Palette *palette = backend_load_palette_file(f);
   fclose(f);

   return palette;
}

void backend_save_palette(const char *path, const SLK_Palette *pal)
{
   FILE *f = fopen(path,"w");
   if(f==NULL)
   {
      SLK_log("failed to open %s for writing",path);
      return;
   }

   backend_save_palette_file(f,pal);

  fclose(f);
}

SLK_Palette *backend_load_palette_file(FILE *f)
{
   if(f==NULL)
   {
      SLK_log("file pointer is NULL, can't load palette");
      return NULL;
   }

   char buffer[512];
   int colors = 0,i,found;
   int r,g,b,a;

   SLK_Palette *palette = backend_malloc(sizeof(*palette));
   if(palette==NULL)
   {
      SLK_log("malloc of size %zu failed, out of memory!",sizeof(*palette));
      return NULL;
   }

   memset(palette,0,sizeof(*palette));
   for(i = 0;i<259&&fgets(buffer,512,f);i++)
   {
      if(i==2)
      {
         sscanf(buffer,"%d",&found);
         palette->used = found;
      }
      else if(i>2&&buffer[0]!='\0')
      {
         if(sscanf(buffer,"%d %d %d %d",&r,&g,&b,&a)!=4)
         {
            sscanf(buffer,"%d %d %d",&r,&g,&b);
            a = 255;
         }

         palette->colors[colors].rgb.r = r;
         palette->colors[colors].rgb.g = g;
         palette->colors[colors].rgb.b = b;
         palette->colors[colors].rgb.a = a;
         colors++;
      }
   }

   return palette;
}

void backend_save_palette_file(FILE *f, const SLK_Palette *pal)
{
   if(f==NULL)
   {
      SLK_log("file pointer is NULL, can't write palette to disk");
      return;
   }

   fprintf(f,"JASC-PAL\n0100\n%d\n",pal->used);
   for(int i = 0;i<pal->used;i++)
   {
      if(pal->colors[i].rgb.a!=255)
         fprintf(f,"%d %d %d %d\n",pal->colors[i].rgb.r,pal->colors[i].rgb.g,pal->colors[i].rgb.b,pal->colors[i].rgb.a);
      else
         fprintf(f,"%d %d %d\n",pal->colors[i].rgb.r,pal->colors[i].rgb.g,pal->colors[i].rgb.b);
   }
}

void *backend_system_malloc(size_t size)
{
   return malloc(size);
}

void backend_system_free(void *ptr)
{
   free(ptr);
}

void *backend_system_realloc(void *ptr, size_t size)
{
   return realloc(ptr,size);
}

void backend_set_malloc(void *(*func)(size_t size))
{
   bmalloc = func;
}

void backend_set_free(void (*func)(void *ptr))
{
   bfree = func;
}

void backend_set_realloc(void *(*func)(void *ptr, size_t size))
{
   brealloc = func;
}

void *backend_malloc(size_t size)
{
   return bmalloc(size);
}

void *backend_calloc(size_t num, size_t size)
{
   void *mem = backend_malloc(num*size);
   memset(mem,0,num*size);
   return mem;
}

void backend_free(void *ptr)
{
   bfree(ptr);
}

void *backend_realloc(void *ptr, size_t size)
{
   return brealloc(ptr,size);
}

void backend_log(const char *w, va_list v)
{
   vprintf(w,v);
}

#undef MAX_CONTROLLERS
