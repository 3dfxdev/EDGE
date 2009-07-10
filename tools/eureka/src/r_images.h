/*
 *  r_images.h
 *  AJA 2002-04-27
 */


#ifndef YH_R_IMAGES  /* DO NOT INSERT ANYTHING BEFORE THIS LINE */
#define YH_R_IMAGES


#include <map>
#include <algorithm>
#include <string>

#include "im_img.h"


struct ImageCache
{
public:
   typedef std::map<std::string, Img *> flat_map_t;
   typedef std::map<std::string, Img *> tex_map_t;
   typedef std::map<wad_ttype_t, Img *> sprite_map_t;

   flat_map_t   flats;
   tex_map_t    textures;
   sprite_map_t sprites;

   static std::string WadToString(const wad_flat_name_t& fname)
   {
      int len;

      for (len = 0; len < WAD_NAME && fname[len]; len++)
      { }
        
      return std::string(fname, len);
   }

   static void DeleteFlat(const flat_map_t::value_type& P)
      {
      delete P.second;
      }

   static void DeleteTex(const tex_map_t::value_type& P)
      {
      delete P.second;
      }

   static void DeleteSprite(const sprite_map_t::value_type& P)
      {
      delete P.second;
      }

   ~ImageCache ()
      {
      std::for_each (flats.begin (), flats.end (), DeleteFlat);
      std::for_each (textures.begin (), textures.end (), DeleteTex);
      std::for_each (sprites.begin (), sprites.end (), DeleteSprite);

      flats.clear ();
      textures.clear ();
      sprites.clear ();
      }

   Img *GetFlat   (const wad_flat_name_t& fname);
   Img *GetTex    (const wad_tex_name_t& tname);
   Img *GetSprite (const wad_ttype_t& type);
};


extern ImageCache *image_cache;

// ick, remove
void GetWallTextureSize (s16_t *, s16_t *, const char *);


#endif  /* DO NOT ADD ANYTHING AFTER THIS LINE */
//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
