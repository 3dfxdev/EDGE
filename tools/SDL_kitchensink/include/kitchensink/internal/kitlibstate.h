#ifndef KITLIBSTATE_H
#define KITLIBSTATE_H

#include "kitchensink/internal/libass.h"
#include "kitchensink/kitconfig.h"

typedef struct Kit_LibraryState {
    unsigned int init_flags;
    ASS_Library *libass_handle;
    void *ass_so_handle;
} Kit_LibraryState;

KIT_LOCAL Kit_LibraryState* Kit_GetLibraryState();

#endif // KITLIBSTATE_H
