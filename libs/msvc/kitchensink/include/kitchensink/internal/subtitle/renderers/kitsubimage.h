#ifndef KITSUBIMAGE_H
#define KITSUBIMAGE_H

#include "kitchensink/kitconfig.h"
#include "kitchensink/internal/kitdecoder.h"
#include "kitchensink/internal/subtitle/renderers/kitsubrenderer.h"

KIT_LOCAL Kit_SubtitleRenderer* Kit_CreateImageSubtitleRenderer(Kit_Decoder *dec, int w, int h);

#endif // KITSUBIMAGE_H
