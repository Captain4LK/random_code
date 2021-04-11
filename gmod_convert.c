#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

enum 
{   
  G1_SECTION_MAP_DIMENSIONS_V1,

  G1_SECTION_TILE_MATCHUP_V1,

  OLD_G1_SECTION_CELL_V1,
  G1_SECTION_OBJECTS_V1,

  G1_SECTION_OBJECT_BASE_V1,
  G1_SECTION_PATHS_V1,

  G1_SECTION_GRAPH_NODES_V1,
  OLD_G1_SECTION_GRAPH_EDGES_V1,
  G1_SECTION_GRAPH_WEIRDS_V1,

  OLD_G1_SECTION_CELL_V2,
  G1_SECTION_GRAPH_EDGES_V2,

  OLD_G1_SECTION_CELL_V3,

  G1_SECTION_TICK,

  G1_SECTION_MOVIE,

  OLD_G1_SECTION_CELL_V4,    

  OLD_G1_SECTION_MAP_VERT_V1,

  OLD_G1_SECTION_MAP_LIGHTS_V1,

  OLD_G1_SECTION_MAP_VERT_V2,

  G1_SECTION_MODEL_QUADS,
  G1_SECTION_MODEL_TEXTURE_NAMES,
  G1_SECTION_MODEL_VERT_ANIMATION,
  
  G1_SECTION_PLAYER_INFO,

  OLD_G1_SECTION_MAP_VERT_V3,     // added flags

  G1_SECTION_SKY_V1,             // sky model and info used for level

  OLD_G1_SECTION_CELL_V5,

  OLD_G1_SECTION_CELL_V6,

  G1_SECTION_OBJECT_TYPES_V1, // saves the name of each object type so dynamic id's are matched
  G1_SECTION_MAP_SECTIONS_V1,

  OLD_G1_SECTION_MAP_VERT_V4,      // added selected prefix

  G1_SECTION_CRITICAL_POINTS_V1,
  G1_SECTION_CRITICAL_GRAPH_V1,
  G1_SECTION_CRITICAL_MAP_V1,
  
  

  G1_SECTION_OBJECT_LIST = 0x8000

};

#define MAX_PATH_LENGTH 512

typedef struct
{
   uint32_t id;
   uint32_t offset;
}Section;

typedef struct
{
   uint16_t length;
   char path[MAX_PATH_LENGTH];
}Texture_info;

typedef struct
{
   uint16_t index;
   float u;
   float v;
}Vertex;

typedef struct
{
   Vertex vertices[4];
   float scale;
   uint16_t flags;
   float x_normal;
   float y_normal;
   float z_normal;
}Face;

typedef struct
{
   float x_pos;
   float y_pos;
   float z_pos;
   float x_normal;
   float y_normal;
   float z_normal;
}Vertex_element;

typedef struct
{
   uint32_t number;
   uint32_t num_sections;
   Section *sections;

   uint16_t num_faces;
   Texture_info *textures;
   Face *faces;
   
   uint16_t num_vertices;
   uint16_t num_animations;
   uint16_t num_animation_frames;
   uint16_t animation_name_length;
   char animation_name[MAX_PATH_LENGTH];

   Vertex_element **vertices;
}Header;

static int pos = 0;
static char *buffer = NULL;
static Header h = {0};

static uint16_t read_u16();
static uint32_t read_u32();
static float read_f();
static uint8_t goto_section(uint32_t section);

int main(int argc, char **argv)
{
   if(argc<2)
      exit(-1);

   FILE *f = fopen(argv[1],"rb");
   int size = 0;
   fseek(f,0,SEEK_END);
   size = ftell(f);
   fseek(f,0,SEEK_SET);
   buffer = malloc(size+1);
   fread(buffer,size,1,f);
   buffer[size] = 0;
   fclose(f);

   pos = 0;
   h.number = read_u32();
   h.num_sections = read_u32();
   printf("Magic number: %x\n",h.number);
   printf("Sections: %x\n",h.num_sections);
   h.sections = malloc(sizeof(*h.sections)*h.num_sections);
   for(int i = 0;i<h.num_sections;i++)
   {
      h.sections[i].id = read_u32();
      h.sections[i].offset = read_u32();
      printf("Section %d: id: %d offset: %d\n",i,h.sections[i].id,h.sections[i].offset);
   }

   //Read texture data
   if(!goto_section(G1_SECTION_MODEL_TEXTURE_NAMES))
   {
      puts("No G1_SECTION_MODEL_TEXTURE_NAMES in file");
   }
   else
   {
      h.num_faces = read_u16();
      printf("Faces: %d\n",h.num_faces);
      h.textures = malloc(sizeof(*h.textures)*h.num_faces);
      for(int i = 0;i<h.num_faces;i++)
      {
         h.textures[i].length = read_u16();
         memcpy(h.textures[i].path,&buffer[pos],h.textures[i].length);
         h.textures[i].path[h.textures[i].length] = 0;
         pos+=h.textures[i].length;
         printf("Texture: %s\n",h.textures[i].path);
      }
   }

   //Read quads
   if(!goto_section(G1_SECTION_MODEL_QUADS))
   {
      puts("No G1_SECTION_MODEL_QUADS in file");
   }
   else
   {
      if(h.num_faces!=read_u16())
         puts("Potential error in file: face counts do not match");

      h.faces = malloc(sizeof(*h.faces)*h.num_faces); 
      for(int i = 0;i<h.num_faces;i++)
      {
         for(int j = 0;j<4;j++)
         {
            h.faces[i].vertices[j].index = read_u16();
            h.faces[i].vertices[j].u = read_f();
            h.faces[i].vertices[j].v = read_f();
            //printf("Vertex index: %d\n",h.faces[i].vertices[j].index);
         }
         h.faces[i].scale = read_f();
         h.faces[i].flags = read_u16();
         h.faces[i].x_normal = read_f();
         h.faces[i].y_normal = read_f();
         h.faces[i].z_normal = read_f();
      }
   }

   //Read vertices
   if(!goto_section(G1_SECTION_MODEL_VERT_ANIMATION))
   {
      puts("No G1_SECTION_MODEL_VERT_ANIMATION in file");
   }
   else
   {
      h.num_vertices = read_u16();
      h.num_animations = read_u16();
      h.animation_name_length = read_u16();
      printf("Vertices: %d\n",h.num_vertices);
      printf("Animations: %d\n",h.num_animations);
      memcpy(h.animation_name,&buffer[pos],h.animation_name_length);
      h.animation_name[h.animation_name_length] = 0;
      pos+=h.animation_name_length;
      printf("Animation: %s\n",h.animation_name);
      h.num_animation_frames = read_u16();
      printf("Animation Frames: %d\n",h.num_animation_frames);
      h.vertices = malloc(sizeof(*h.vertices)*h.num_animation_frames);
      for(int i = 0;i<h.num_animation_frames;i++)
      {
         h.vertices[i] = malloc(sizeof(*h.vertices[i])*h.num_vertices);

         for(int j = 0;j<h.num_vertices;j++)
         {
            h.vertices[i][j].x_pos = read_f();
            h.vertices[i][j].y_pos = read_f();
            h.vertices[i][j].z_pos = read_f();
            h.vertices[i][j].x_normal = read_f();
            h.vertices[i][j].y_normal = read_f();
            h.vertices[i][j].z_normal = read_f();
            printf("%f %f %f %f %f %f\n",h.vertices[i][j].x_pos,h.vertices[i][j].y_pos,h.vertices[i][j].z_pos,h.vertices[i][j].x_normal,h.vertices[i][j].y_normal,h.vertices[i][j].z_normal);
         }
      }
   }
   free(buffer);

   //Write to ply format
   FILE *fout = fopen("out.ply","w");
   fprintf(fout,"ply\nformat ascii 1.0\n");
   fprintf(fout,"comment author: Lukas Holzbeierlein\n");
   fprintf(fout,"comment object: %s\n",argv[1]);
   fprintf(fout,"element vertex %d\n",h.num_vertices);
   fprintf(fout,"property float x\nproperty float y\nproperty float z\n");
   fprintf(fout,"element face %d\n",h.num_faces);
   fprintf(fout,"property list uchar int vertex_index\n");
   fprintf(fout,"end_header\n");

   for(int i = 0;i<h.num_vertices;i++)
      fprintf(fout,"%f %f %f\n",h.vertices[0][i].x_pos,h.vertices[0][i].y_pos,h.vertices[0][i].z_pos);
   for(int i = 0;i<h.num_faces;i++)
      fprintf(fout,"4 %d %d %d %d\n",h.faces[i].vertices[0].index,h.faces[i].vertices[1].index,h.faces[i].vertices[2].index,h.faces[i].vertices[3].index);

   fclose(fout);

   return 0;
}

static uint16_t read_u16()
{
   uint16_t ret = *((uint16_t *)&buffer[pos]);
   pos+=2;
   return ret;
}

static uint32_t read_u32()
{
   uint32_t ret = *((uint32_t *)&buffer[pos]);
   pos+=4;
   return ret;
}

static float read_f()
{
   float ret = *((float *)&buffer[pos]);
   pos+=4;
   return ret;
}

static uint8_t goto_section(uint32_t section)
{
   for(int i = 0;i<h.num_sections;i++)
   {
      if(h.sections[i].id==section)
      {
         pos = h.sections[i].offset;
         return 1;
      }
   }

   return 0;
}
