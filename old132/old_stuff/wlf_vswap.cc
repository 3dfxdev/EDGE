
/* VSWAP */

#include "i_defs.h"

#include <vector>

#include "epi/endianess.h"
#include "epi/basicimage.h"

#include "wf_local.h"

#include "r_image.h"


class vswap_info_c
{
public:
	FILE *fp;

	int first_wall,   num_walls;
	int first_sprite, num_sprites;
	int first_sound,  num_sounds;

	std::vector<raw_chunk_t> chunks;

public:
	vswap_info_c() : fp(NULL) { }
	~vswap_info_c() { }
};

vswap_info_c vswap;


void WF_VSwapOpen(void)
{
	I_Printf("Opening VSWAP...\n");

	vswap.fp = fopen("VSWAP.WL6", "rb");
	if (! vswap.fp)
		throw "FUCK1";

	raw_vswap_t vv;

	if (fread(&vv, sizeof(vv), 1, vswap.fp) != 1)
		throw "FUCK2";
	
	vswap.first_wall   = 0;  // implied
	vswap.first_sprite = EPI_LE_U16(vv.first_sprite);
	vswap.first_sound  = EPI_LE_U16(vv.first_sound);

	int total_chunks = EPI_LE_U16(vv.total_chunks);

	vswap.num_walls   = vswap.first_sprite;
	vswap.num_sprites = vswap.first_sound - vswap.first_sprite;
	vswap.num_sounds  = total_chunks - vswap.first_sound;

	I_Printf("TOTAL CHUNKS: %d\n", total_chunks);
	I_Printf("WALLS: +%d *%d\n", vswap.first_wall, vswap.num_walls);
	I_Printf("SPRITES: +%d *%d\n", vswap.first_sprite, vswap.num_sprites);
	I_Printf("SOUNDS: +%d *%d\n", vswap.first_sound, vswap.num_sounds);

	vswap.chunks.reserve(total_chunks);

	for (int i=0; i < total_chunks; i++)
	{
		raw_chunk_t CK;

		if (fread(&CK.offset, 4, 1, vswap.fp) != 1)
			throw "FUCK3";

		CK.offset = EPI_LE_U32(CK.offset);

		vswap.chunks.push_back(CK);
	}

	for (int i=0; i < total_chunks; i++)
	{
		raw_chunk_t& CK = vswap.chunks[i];

		if (fread(&CK.length, 2, 1, vswap.fp) != 1)
			throw "FUCK4";

		CK.length = EPI_LE_U16(CK.length);

		L_WriteDebug("[%d] : offset %d, length %d\n", i, CK.offset, CK.length);
	}
}

void WF_VSwapClose(void)
{
	if (vswap.fp)
		fclose(vswap.fp);

	vswap.fp = NULL;
}

byte *VSwapReadChunk(int index, int *length)
{
	SYS_ASSERT(0 <= index && index < (int)vswap.chunks.size());

	*length = vswap.chunks[index].length;

	if (*length == 0)
		return NULL;

	fseek(vswap.fp, vswap.chunks[index].offset, SEEK_SET);

	byte *data = new byte[*length];

	if (fread(data, *length, 1, vswap.fp) != 1)
		throw "SHIT";

	return data;
}

epi::basicimage_c *WF_VSwapLoadWall(int index)
{
	int length;
	byte *data = VSwapReadChunk(index, &length);

	if (! data || length != 4096) throw "BUMMER"; 

	epi::basicimage_c *img = new epi::basicimage_c(64, 64, 3); //!!!! PAL

	for (int y = 0; y < 64; y++)
	for (int x = 0; x < 64; x++)
	{
		byte src = data[x*64 + 63-y];  // column-major order
		byte *pix = img->PixelAt(x, y);

		pix[0] = wolf_palette[src*3+0];
		pix[1] = wolf_palette[src*3+1];
		pix[2] = wolf_palette[src*3+2];
	}

	delete[] data;

	return img;
}

epi::basicimage_c *WF_VSwapLoadSprite(int index)
{
	SYS_ASSERT(index < vswap.num_sprites);

	int length;
	byte *data = VSwapReadChunk(vswap.first_sprite + index, &length);

	if (! data || length <= 8) throw "NO SPRITE"; 

L_WriteDebug("DATA LEN %d\n", length);
L_WriteDebug("DATA: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
data[0], data[1], data[2], data[3], data[4],
data[5], data[6], data[7], data[8], data[9]);

	const raw_shape_t *shape = (raw_shape_t*) data;

	int left  = EPI_LE_U16(shape->leftpix);
	int right = EPI_LE_U16(shape->rightpix);

L_WriteDebug("LEFT %d RIGHT %d\n", left, right);
	if (left < 0 || right >= 64 || left >= right)
		throw "BAD SPRITE";

	int w = (right - left);
	int h = 64;

	epi::basicimage_c *img = new epi::basicimage_c(64, 64, 4); //!!!! PAL

 
// Very weird code by Mr. Bruce Lewis (btw creator of DoomGL) need to be rewritten!
 
  for (int x = 0; x < w; x++)
	{
		int offset = EPI_LE_U16(shape->offsets[x]);

		SYS_ASSERT((4+w*2) <= offset && offset < length);

		const byte *post = (const byte*)(data + offset);

L_WriteDebug("Column[%d]:", left+x);

		for (; *post; post += 6)
		{
			SYS_ASSERT(post < data + length);

			u16_t y1 = post[4]/2;
			u16_t y2 = post[0]/2;
			u16_t pix_off = post[2] + (post[3] << 8);

			SYS_ASSERT(y1 < y2 && y2 <= 64);

			L_WriteDebug(" (%d..%d,%d)", y1, y2, pix_off);

			const byte *pixel = data + y1 + pix_off;  // WEIRD!!

			for (; y1 < y2; y1++, pixel++)
			{
				if (pixel < data || pixel >= data + length) continue; //!!!!!!
				SYS_ASSERT(pixel < data + length);

				byte src = *pixel;
				byte *dest = img->PixelAt(left+x, (63-y1));

				dest[0] = wolf_palette[src*3+0];
				dest[1] = wolf_palette[src*3+1];
				dest[2] = wolf_palette[src*3+2];
				dest[3] = 255;
			}

		}
L_WriteDebug("\n");
	}

	delete[] data;

	return img;
}

#if 0
void DrawTest()
{
	static GLuint tex_id = 0;

	if (tex_id == 0)
	{
		epi::basicimage_c *wall = VSwapLoadSprite(10); /// Wall(296);
		SYS_ASSERT(wall);

		glGenTextures(1, &tex_id);

		rend::UploadTexture(tex_id, wall, rend::UTX_Mipmap);
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glColor3f(1,1,1);
	glEnable(GL_BLEND);

	glBegin(GL_QUADS);

	glTexCoord2f(0, 0); glVertex2i(0, 0);
	glTexCoord2f(0, 1); glVertex2i(0, 256);
	glTexCoord2f(1, 1); glVertex2i(188, 256);
	glTexCoord2f(1, 0); glVertex2i(188, 0);

	glDisable(GL_BLEND);
	glEnd();
}
#endif

