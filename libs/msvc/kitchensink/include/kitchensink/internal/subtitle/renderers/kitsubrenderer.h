#ifndef KITSUBRENDERER_H
#define KITSUBRENDERER_H

#include "kitchensink/kitsource.h"
#include "kitchensink/kitformats.h"

typedef struct Kit_SubtitleRenderer Kit_SubtitleRenderer;
typedef struct Kit_TextureAtlas Kit_TextureAtlas;
typedef struct Kit_Decoder Kit_Decoder;

typedef void (*ren_render_cb)(Kit_SubtitleRenderer *ren, void *src, double start_pts, double end_pts);
typedef int (*ren_get_data_db)(Kit_SubtitleRenderer *ren, Kit_TextureAtlas *atlas, double current_pts);
typedef void (*ren_close_cb)(Kit_SubtitleRenderer *ren);

struct Kit_SubtitleRenderer {
    Kit_Decoder *dec;
    void *userdata;
    ren_render_cb ren_render; ///< Subtitle rendering function callback
    ren_get_data_db ren_get_data; ///< Subtitle data getter function callback
    ren_close_cb ren_close; ///< Subtitle renderer close function callback
};

KIT_LOCAL Kit_SubtitleRenderer* Kit_CreateSubtitleRenderer(Kit_Decoder *dec);
KIT_LOCAL void Kit_RunSubtitleRenderer(Kit_SubtitleRenderer *ren, void *src, double start_pts, double end_pts);
KIT_LOCAL int Kit_GetSubtitleRendererData(Kit_SubtitleRenderer *ren, Kit_TextureAtlas *atlas, double current_pts);
KIT_LOCAL void Kit_CloseSubtitleRenderer(Kit_SubtitleRenderer *ren);

#endif // KITSUBRENDERER_H
