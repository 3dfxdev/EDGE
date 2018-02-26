
/* WOLF GRAPHICS */

#include "i_defs.h"

#include <vector>

#include "epi/endianess.h"
#include "epi/basicimage.h"

#include "wf_local.h"

#include "r_image.h"


#define MAGIC  3     // 3 for WOLF,SPEAR  5 for BLAKE


static huffnode_t gfx_huffman[256];

class pic_stuff_c
{
public:
	int offset;
	int length;  // compressed

	short width;
	short height;

	pic_stuff_c() : offset(-1), length(0), width(0), height(0) { }
	~pic_stuff_c() { }
};

class graph_info_c
{
public: //// private:
	int num_chunks;

	pic_stuff_c *pics;

	FILE *fp;  // vgagraph
	int total_len;

public:
	graph_info_c() : num_chunks(0), pics(NULL) { }
	~graph_info_c() { }

	void Setup(int _chunks)
	{
		num_chunks = _chunks;
		pics = new pic_stuff_c[num_chunks];
	}
	
	void PicOffset(int P, int offset)
	{
		SYS_ASSERT(0 <= P && P < num_chunks);

		if (offset >= 0xFFFFFF)
			offset = -1;

		pics[P].offset = offset;
	}

	void PicLengths(int vgagraph_len)
	{
		int last_pos = vgagraph_len;

		for (int P = num_chunks - 1; P >= 0; P--)
		{
			if (pics[P].offset < 0)
				continue;

			if (pics[P].offset >= last_pos)
			{
				fprintf(stderr, "WARNING: PIC #%d bad length -- ignoring\n", P);
				pics[P].offset = -1;
				continue;
			}

			pics[P].length = last_pos - pics[P].offset;

			last_pos = pics[P].offset;
		}
	}

	void PicSize(int P, const raw_picsize_t *sz_p)
	{
		SYS_ASSERT(0 <= P && P < num_chunks);

		pics[P].width  = EPI_LE_S16(sz_p->width);
		pics[P].height = EPI_LE_S16(sz_p->height);

		if (pics[P].offset >= 0 && 
			(pics[P].width > 320 || pics[P].height > 200))
		{
			fprintf(stderr, "WARNING: PIC #%d bad size -- ignoring\n", P);
			pics[P].offset = -1;
		}

	}
};

graph_info_c vga_info;


void Load_VgaDict()
{
	FILE *fp = fopen("VGADICT.WL6", "rb");
	if (! fp) I_Error("Missing VGADICT file.\n");

	fread(&gfx_huffman, 1024, 1, fp);

	fclose(fp);

	// fix endianness
	for (int i=0; i < 256; i++)
	{
		huffnode_t *node = gfx_huffman + i;

		// AJA: improve nodes by using range 0-255 for nodes
		node->bit0 = EPI_LE_U16(node->bit0) ^ 256;
		node->bit1 = EPI_LE_U16(node->bit1) ^ 256;

		if (node->bit0 > 512 || node->bit1 > 512)
			throw "BAD VGADICT!";
	}
}

// FIXME: hack job, use proper EPI function
static int ut_file_length(FILE *fp)
{
	// get to the end of the file
	fseek(fp, 0, SEEK_END);

	// get the size
	long size = ftell(fp);

	// reset to beginning
	fseek(fp, 0, SEEK_SET);

	return (int)size;
}
static u32_t ut_fread_u32(FILE *fp)
{
	u32_t buf;
	fread((void*)&buf, 4, 1, fp);
	return EPI_LE_U32(buf);
}


void Load_VgaHead()
{
	FILE *fp = fopen("VGAHEAD.WL6", "rb");
	if (! fp) I_Error("Missing VGAHEAD file.\n");

	int len = ut_file_length(fp);

	if (len < 3) throw "fucked up vgahead"; //!!!

	len /= 3;
	vga_info.Setup(len);

	for (int i=0; i < len; i++)
	{
		byte temp[4];

		fread(temp, 3, 1, fp);

		vga_info.PicOffset(i, temp[0] + (temp[1]<<8) + (temp[2]<<16));
	}

	fclose(fp);

	vga_info.fp = fopen("VGAGRAPH.WL6", "rb");
	if (! vga_info.fp) I_Error("Missing VGAGRAPH file.\n");

	vga_info.total_len = ut_file_length(vga_info.fp);

	vga_info.PicLengths(vga_info.total_len);
}

byte *GraphReadChunk(int index, int *expanded)
{
	SYS_ASSERT(0 <= index && index < (int)vga_info.num_chunks);

	SYS_ASSERT(vga_info.pics[index].offset >= 0);
	SYS_ASSERT(vga_info.pics[index].length >  4);

	int comp_len = vga_info.pics[index].length;

	fseek(vga_info.fp, vga_info.pics[index].offset, SEEK_SET);

	*expanded = (int)ut_fread_u32(vga_info.fp);
	
fprintf(stderr, "GraphReadChunk[%d] : offset 0x%x complen %d expanded %d\n",
index, vga_info.pics[index].offset, comp_len, *expanded);

	if (*expanded <= 0) throw "GraphReadChunk fucked";

	comp_len -= 4;
	byte *compressed = new byte[comp_len];

	size_t actual = fread(compressed, comp_len, 1, vga_info.fp);
	if (actual != 1) I_Error("Error reading Graph chunk!\n");

	byte *data = new byte[*expanded];

	WF_Huff_Expand(compressed, comp_len, data, *expanded, gfx_huffman);

	return data;
}

void Load_VgaSizes()
{
	int length;
	int i;

	byte *data = GraphReadChunk(0 /* !!! SPECIAL */ , &length);

// fprintf(stderr, "Load_VgaSizes: length %d (num %d) -- chunk num %d\n",
// length, length / 4, vga_info.num_chunks);

//FIXME: handle length/num_chunks mismatch

	const raw_picsize_t *sz_p = (raw_picsize_t *) data;
	length /= sizeof(raw_picsize_t);

	for (i = 0; i < length; i++, sz_p++)
	{
		vga_info.PicSize(i, sz_p);

fprintf(stderr, "PIC[%d] size %dx%d\n", i+MAGIC, vga_info.pics[i].width, vga_info.pics[i].height);
	}

	delete[] data;
}

void WF_GraphicsOpen(void)
{
	Load_VgaDict();
	Load_VgaHead();
	Load_VgaSizes();
}


epi::basicimage_c *WF_GraphLoadPic(int chunk)
{
	int length;

	byte *data = GraphReadChunk(chunk, &length);
	if (! data) throw "BUMMER"; 

//!!!!!!!1
	int w = vga_info.pics[chunk-MAGIC].width;  // FIXME: magic 3
	int h = vga_info.pics[chunk-MAGIC].height;

	SYS_ASSERT(w > 0 && h > 0);

	if (w * h > length) throw "GraphReadChunk: image data too small";

	int tw = W_MakeValidSize(w);
	int th = W_MakeValidSize(h);

	epi::basicimage_c *img = new epi::basicimage_c(tw, th, 3); //!!!! PAL

	int qt = w*h / 4;

	for (int y = 0; y < h; y++)
	for (int x = 0; x < w; x++)
	{
		int k = ((    y)*w + x); k = (k%4) * qt + k/4;

		byte src = data[k];  // column-major order
		byte *pix = img->PixelAt(x, th-1-y);  // <-- UGH !!

		pix[0] = wolf_palette[src*3+0];
		pix[1] = wolf_palette[src*3+1];
		pix[2] = wolf_palette[src*3+2];
	}

	delete[] data;

	return img;
}

#if 0
GLuint TitlePic()
{
	static GLuint tex_id = 0;

	if (tex_id == 0)
	{
#if 0  // SOD
		int length;
		byte *data = GraphReadChunk(153, &length);
		if (! data) throw "SOD BUMMER"; 
		if (length != 768) throw "SOD BAD PALETTE";
		for (int j=0; j < 768; j++)
			sod_test::Palette[j] = data[j]*4;
		delete[] data;
#endif
		ut::basicimage_c *img = GraphLoadPic(107); //99
		SYS_ASSERT(img);

		glGenTextures(1, &tex_id);

		rend::UploadTexture(tex_id, img, 0);
	}

	return tex_id;
}
#endif


#if 0
void GraphTest()
{
	static GLuint tex_id = 0;

	if (tex_id == 0)
	{
		ut::basicimage_c *img = GraphLoadPic(101);
		SYS_ASSERT(img);

		glGenTextures(1, &tex_id);

		rend::UploadTexture(tex_id, img, 0);
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, tex_id);
	glColor3f(1,1,1);

	glBegin(GL_QUADS);

	glTexCoord2f(0, 0); glVertex2i(0, 0);
	glTexCoord2f(0, 1); glVertex2i(0, 200*2);
	glTexCoord2f(1, 1); glVertex2i(320*2, 200*2);
	glTexCoord2f(1, 0); glVertex2i(320*2, 0);

	glEnd();
}
#endif

