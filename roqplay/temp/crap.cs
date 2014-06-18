
struct datalump_s
{
	const byte *data;

	size_t pos;
	size_t size;
};

// original: oggplayer_c    abstract_music_c
class roqplayer_c : public abstract_video_c
{
public:
	 roqplayer_c();
	~roqplayer_c();

private:

	enum status_e
	{
		NOT_LOADED, PLAYING, PAUSED, STOPPED
	};
	
	int status;
//	bool looping;

	FILE *roq_file;
	datalump_s roq_lump;
//original OggVorbis_File ogg_stream
	RoqVideo_File roq_stream;
//	vorbis_info *vorbis_inf;
	bool is_stereo;

	s16_t *mono_buffer;
    s16_t *stereo_buffer;
public:
	bool OpenLump(const char *lumpname);
	bool OpenFile(const char *filename);
// the virtual void close(void) maybe use an ESC key (like the SDL player?)

//===========================================LOOK ALL OF THESE UP!!!==============================
	virtual void Close(void);

	virtual void Play(bool loop);
	virtual void Stop(void);

	virtual void Pause(void);
	virtual void Resume(void);

	virtual void Ticker(void);
//	virtual void Volume(float gain);

private:
	const char *GetError(int code);

	void PostOpenInit(void);

	//streamintobuffer(epi::sound_data_c) used for audio stuff in ROQ?
	bool StreamIntoBuffer(epi::sound_data_c *buf);

};
//==============================================LOOOK ALL OF THE ABOVE UP!==========================
//
// oggplayer datalump operation functions
// roqplayer datalump operation functions
size_t roqplayer_memread(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	datalump_s *d = (datalump_s *)datasource;
	size_t rb = size*nmemb;

	if (d->pos >= d->size)
		return 0;

	if (d->pos + rb > d->size)
		rb = d->size - d->pos;

	memcpy(ptr, d->data + d->pos, rb);
	d->pos += rb;		
	
	return rb / size;
}


