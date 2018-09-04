#include <assert.h>
#include <stdlib.h>

#include <SDL2/SDL_surface.h>

#include "kitchensink/kiterror.h"
#include "kitchensink/internal/utils/kitlog.h"
#include "kitchensink/internal/subtitle/kitatlas.h"
#include "kitchensink/internal/subtitle/kitsubtitlepacket.h"
#include "kitchensink/internal/subtitle/renderers/kitsubimage.h"

static void ren_render_image_cb(Kit_SubtitleRenderer *ren, void *sub_src, double start_pts, double end_pts) {
    assert(ren != NULL);
    assert(sub_src != NULL);

    AVSubtitle *sub = sub_src;
    SDL_Surface *dst = NULL;
    SDL_Surface *src = NULL;

    // If this subtitle has no rects, we still need to clear screen from old subs
    if(sub->num_rects == 0) {
        Kit_WriteDecoderOutput(
            ren->dec, Kit_CreateSubtitlePacket(true, start_pts, end_pts, 0, 0, NULL));
        return;
    }

    // Convert subtitle images from paletted to RGBA8888
    for(int n = 0; n < sub->num_rects; n++) {
        AVSubtitleRect *r = sub->rects[n];
        if(r->type != SUBTITLE_BITMAP)
            continue;

        src = SDL_CreateRGBSurfaceWithFormatFrom(
            r->data[0], r->w, r->h, 8, r->linesize[0], SDL_PIXELFORMAT_INDEX8);
        SDL_SetPaletteColors(src->format->palette, (SDL_Color*)r->data[1], 0, 256);
        dst = SDL_CreateRGBSurfaceWithFormat(
            0, r->w, r->h, 32, SDL_PIXELFORMAT_RGBA32);
        
        // Blit source to target and free source surface.
        SDL_BlitSurface(src, NULL, dst, NULL);
        
        // Create a new packet and write it to output buffer
        Kit_WriteDecoderOutput(
            ren->dec, Kit_CreateSubtitlePacket(false, start_pts, end_pts, r->x, r->y, dst));

        // Free surfaces
        SDL_FreeSurface(src);
        SDL_FreeSurface(dst);
    }
}

static int ren_get_data_cb(Kit_SubtitleRenderer *ren, Kit_TextureAtlas *atlas, double current_pts) {
    // Read any new packets to atlas
    Kit_SubtitlePacket *packet = NULL;

    // Clean dead packets
    while((packet = Kit_PeekDecoderOutput(ren->dec)) != NULL) {
        if(packet->pts_end < current_pts) {
            Kit_AdvanceDecoderOutput(ren->dec);
            Kit_FreeSubtitlePacket(packet);
            continue;
        }
        if(packet->pts_start < current_pts) {
            if(packet->clear) {
                Kit_ClearAtlasContent(atlas);
            }
            if(packet->surface != NULL) {
                SDL_Rect target;
                target.x = packet->x;
                target.y = packet->y;
                target.w = packet->surface->w;
                target.h = packet->surface->h;
                Kit_AddAtlasItem(atlas, packet->surface, &target);
            }
            Kit_AdvanceDecoderOutput(ren->dec);
            Kit_FreeSubtitlePacket(packet);
            ren->dec->clock_pos = current_pts;
            continue;
        }
        break;
    }

    return 0;
}

Kit_SubtitleRenderer* Kit_CreateImageSubtitleRenderer(Kit_Decoder *dec, int w, int h) {
    assert(dec != NULL);
    assert(w >= 0);
    assert(h >= 0);

    // Allocate a new renderer
    Kit_SubtitleRenderer *ren = Kit_CreateSubtitleRenderer(dec);
    if(ren == NULL) {
        return NULL;
    }

    // Only renderer required, no other data.
    ren->ren_render = ren_render_image_cb;
    ren->ren_get_data = ren_get_data_cb;
    ren->ren_close = NULL;
    ren->userdata = NULL;
    return ren;
}
