/* ROTT UTIL */

enum { LT_UNKNOWN=0, LT_TEXTURE, LT_FONT, LT_MODEL };

#pragma pack(push,1)
typedef struct {//__attribute__((packed)) {
    unsigned __int16 transused, trans[4];
    unsigned __int32 * pal;
    byte colorsused[MAXPLAYERCOLORS];
}texauxinfo_t;
#pragma pack(pop)

#pragma pack(push,2)//size = 28
typedef struct {//__attribute__((packed)) {
    unsigned __int16 w, h;
    unsigned __int16 rw, rh;
    __int16 lofs, tofs;
    __int16 osize;
    unsigned __int32 rep;
} texbufinfo_t;
#pragma pack(pop) 
