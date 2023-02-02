#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "img2p/libSLK.h"
#include "img2p/image2pixel.h"
#include "img2p/assets.h"
#include "img2p/cute_png.h"

#include <emscripten.h>

EMSCRIPTEN_KEEPALIVE
int version() {
  return 0x104;
}

EMSCRIPTEN_KEEPALIVE
uint8_t* create_buffer(int width, int height) {
  return malloc(width * height * 4 * sizeof(uint8_t));
}

EMSCRIPTEN_KEEPALIVE
void destroy_buffer(uint8_t* p) {
  free(p);
}

int result[2];
EMSCRIPTEN_KEEPALIVE
void process(uint8_t* img_in, int width, int height, float quality) {
  uint8_t* img_out;

   SLK_RGB_sprite sprite_in;
   sprite_in.width = width;
   sprite_in.height = height;
   sprite_in.data = (SLK_Color *)img_in;

   //Processing
   //-----------------------------------
   I2P_state state;
   memset(&state,0,sizeof(state));
   img2pixel_state_init(&state);
   img2pixel_set_palette(&state,assets_load_pal_default());

   img2pixel_lowpass_image(&state,&sprite_in,&sprite_in);
   img2pixel_sharpen_image(&state,&sprite_in,&sprite_in);
   int owidth;
   int oheight;
   if(img2pixel_get_scale_mode(&state)==0)
   {
      owidth = img2pixel_get_out_width(&state);
      oheight = img2pixel_get_out_height(&state);
   }
   else
   {
      owidth = sprite_in.width/img2pixel_get_out_swidth(&state);
      oheight = sprite_in.height/img2pixel_get_out_sheight(&state);
   }

   SLK_RGB_sprite *out = SLK_rgb_sprite_create(owidth,oheight);
   img2pixel_process_image(&state,&sprite_in,out);

   img2pixel_state_free(&state);
   free(state.palette);
   //-----------------------------------

   cp_image_t img;
   img.w = out->width;
   img.h = out->height;
   img.pix = (cp_pixel_t *)out->data;

   cp_saved_png_t saved = cp_save_png_to_memory(&img);
   SLK_rgb_sprite_destroy(out);

   result[0] = (int)(intptr_t)saved.data;
   result[1] = saved.size;
}

EMSCRIPTEN_KEEPALIVE
void free_result(uint8_t* result) {
   free(result);
}

EMSCRIPTEN_KEEPALIVE
int get_result_pointer() {
  return result[0];
}

EMSCRIPTEN_KEEPALIVE
int get_result_size() {
  return result[1];
}

