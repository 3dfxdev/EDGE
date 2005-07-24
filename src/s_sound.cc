//----------------------------------------------------------------------------
//  EDGE Sound Handling Code (ENGINE LEVEL)
//----------------------------------------------------------------------------
// 
//  Copyright (c) 1999-2005  The EDGE Team.
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//----------------------------------------------------------------------------
//
//  Based on the DOOM source code, released by Id Software under the
//  following copyright:
//
//    Copyright (C) 1993-1996 by id Software, Inc.
//
//----------------------------------------------------------------------------
//

#include "i_defs.h"
#include "i_defs_al.h"

#include "s_sound.h"

#include "errorcodes.h"

#include "m_argv.h"
#include "m_random.h"
#include "p_mobj.h"
#include "r_defs.h"
#include "w_wad.h"

#include "epi/strings.h"

// If true, sound system is off/not working. Changed to false if sound init ok.
bool nosound = false;

// -AJA- 2005/02/26: table to convert slider position to GAIN.
//       Curve was hand-crafted to give useful distinctions of
//       volume levels at the quiet end.  Entry zero always
//       means total silence (in the table for completeness).
float slider_to_gain[20] =
{
	0.00000, 0.00200, 0.00400, 0.00800, 0.01600,
	0.03196, 0.05620, 0.08886, 0.12894, 0.17584,
	0.22855, 0.28459, 0.34761, 0.41788, 0.49553,
	0.58075, 0.67369, 0.77451, 0.88329, 1.00000
};

#define DEFAULT_REFERENCE_DISTANCE 128.0f

#define NO_BUFFER ((ALuint)-1)

#define MIN_SOURCES 12
#define DEFAULT_SOURCES 32
#define MAX_SOURCES 255

namespace sound
{
    // ==============================================================

    // Internal forward declarations
    typedef struct fx_s fx_t;

    void SetFXGain(fx_t* fx, sfxdef_c* def);
    void SetFXPos(fx_t* fx);

    // ==============================================================

    typedef enum { CX, CY, CZ, NUMCOORDS } coord_e; 
    typedef enum { UX, UY, UZ, AX, AY, AZ, ORI_NUMCOORDS } ori_coord_e; 

    typedef enum
    {
        PENDINGSTATE_NONE,
        PENDINGSTATE_PAUSE,
        PENDINGSTATE_RESUME,
        PENDINGSTATE_NUMTYPES
    }
    sfxpenstate_e;

    // ==============================================================

    typedef struct fx_data_s
    {
        ALuint buffer_id;
        int count;
    }
    fx_data_t;

    class fx_datacache_c : public epi::array_c
    {
    public:
        fx_datacache_c() : epi::array_c(sizeof(fx_data_t)) {}
        ~fx_datacache_c() { Clear(); }

    private:
        void CleanupObject(void *obj) 
        { 
            fx_data_t *data = (fx_data_t*)obj;

            if (data->buffer_id != NO_BUFFER)
                DoRelease(data);
        }

        void DoRelease(fx_data_t* data)
        {
            if (data->buffer_id != NO_BUFFER)
            {
                alDeleteBuffers(1, &data->buffer_id);
                data->buffer_id = NO_BUFFER;
            }

            data->count = 0;
        }
        
        void SetAllocatedSize(int size)
        {
            Size(size);
            SetCount(size);
        }

    public:
        void Flush()
        {
            epi::array_iterator_c it;
            fx_data_t *data;

            for (it = GetBaseIterator(); it.IsValid(); it++)
            {
                data = ITERATOR_TO_PTR(it, fx_data_t);
                if (data->buffer_id != NO_BUFFER && data->count == 0)
                    DoRelease(data);
            }
        }

        int GetAllocatedSize()
        {
            return array_max_entries;
        }

        fx_data_t* GetEntry(int idx) const
        {
            return (fx_data_t*)FetchObject(idx);
        }

        int GetSize()
        {
            epi::array_iterator_c it;
            fx_data_t *data;

            int count = 0;

            for (it = GetBaseIterator(); it.IsValid(); it++)
            {
                data = ITERATOR_TO_PTR(it, fx_data_t);
                if (data->buffer_id != NO_BUFFER)
                    count++;
            }

            return count;
        }

        fx_data_t* Load(int sfx_id, sfxdef_c *def)
        {
            if (sfx_id < 0)
                return NULL;

            fx_data_t* data;

            if (sfx_id >= array_entries)
            {
                int old_count = array_entries;
                
                SetAllocatedSize(sfx_id+1);

                // Default any created values
                epi::array_iterator_c it;
                for (it = GetIterator(old_count); it.IsValid(); it++)
                {
                    data = ITERATOR_TO_PTR(it, fx_data_t);

                    data->buffer_id = NO_BUFFER;
                    data->count = 0;
                }
            }

            data = GetEntry(sfx_id);
            if (data->buffer_id != NO_BUFFER)
            {
                throw epi::error_c(ERR_SOUND, 
                                   "AL buffer in use on Load() attempt");
            }

            // Allocate a buffer
            alGetError();  // clear any existing error
	
            alGenBuffers(1, &data->buffer_id);
            if (alGetError() != AL_NO_ERROR)
            {
                throw epi::error_c(ERR_SOUND, 
                                   "Couldn't generate AL buffer");
            }

            // Cache the lump
            int lumpnum = W_CheckNumForName(def->lump_name);
            const char *name = def->lump_name;
            if (lumpnum == -1)
            {
                if (::strict_errors)
                {
                    epi::string_c s;

                    s.Format("SFX '%s' doesn't exist", name);
                    throw epi::error_c(ERR_SOUND, s);
                }
                else
                {
                    I_Warning("Unknown sound lump %s, trying DSPISTOL.\n", 
                              name);

                    name = "DSPISTOL";
                    lumpnum = W_CheckNumForName(name);
                    if (lumpnum == -1)
                        return NULL; // No DSPISTOL, never mind
                }
            }

            // Load the data into a buffer
            {
                const byte* lump = ((byte*)W_CacheLumpNum(lumpnum));
                unsigned int freq = lump[2] + (lump[3] << 8);

                unsigned int length = W_LumpLength(lumpnum);
                if (length < 8)
                {
                    epi::string_c s;
                    s.Format("Lump '%s' is too short\n", name);
                    throw epi::error_c(ERR_SOUND, s);
                }

                alBufferData(data->buffer_id, 
                             AL_FORMAT_MONO8, 
                             (ALvoid*) ((byte*)(lump+8)), 
                             length - 8, freq);

                if (alGetError() != AL_NO_ERROR)
                {
                    alDeleteBuffers(1, &data->buffer_id);
                    data->buffer_id = NO_BUFFER;

                    epi::string_c s;
                    s.Format("Lump '%s' could not be loaded\n", name);
                    throw epi::error_c(ERR_SOUND, s);
                }                

                // the lump is particularly useless, since it won't be 
                // needed until the sound itself has been flushed. It should 
                // be flushed sometime before the sound, so why not just 
                // flush it as early as possible.
                W_DoneWithLump_Flushable(lump);
            }

            return data;
        }

        void Release(int sfx_id)
        {
            fx_data_t* data = GetEntry(sfx_id);
            if (data)
                DoRelease(data);
        }
    };

    // ==============================================================

    typedef union
    {
        mobj_t *mo;
        sec_sfxorig_t* sec;
    }
    fx_posdata_u;

    typedef enum
    {
        FXPOSTYPE_NOTUSED,
        FXPOSTYPE_OBJECT,
        FXPOSTYPE_SECTOR,
        FXPOSTYPE_STATIC,
        FXPOSTYPE_NUMTYPES
    }
    fx_postype_e;
            
    // ==============================================================

    typedef struct fx_s
    {
        float x, y, z;
        fx_posdata_u pos_data;
        int pos_type;
        int def;
        int category;
        int handle; // Valid if > 0
        ALuint source_id;
        int flags;
    }
    fx_t;

    class fx_list_c : public epi::array_c
    {
    public:
        fx_list_c()  : epi::array_c(sizeof(fx_t)) {}
        ~fx_list_c() { Clear(); }
        
    private:
        void CleanupObject(void *obj) 
        { 
            // Nothing to do 
        }
        
    public:
        int GetAllocatedSize()
        {
            return array_max_entries;
        }

        fx_t *GetSpareEntry()
        {
            epi::array_iterator_c it;
            fx_t *fx;

            for (it = GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, fx_t);
                if (fx->handle == 0)
                    return fx;
            }

            if (array_entries < array_max_entries)
                return (fx_t*)ExpandAtTail();

            return NULL;
        }

        int GetSize()
        {
            epi::array_iterator_c it;
            fx_t *fx;

            int count = 0;

            for (it = GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, fx_t);
                if (fx->handle > 0)
                    count++;
            }

            return count;
        }

        void SetAllocatedSize(int size)
        {
            Size(size);

            memset(array, 0, 
                   sizeof(epi::array_block_t)*
                   (array_block_objsize*array_entries));

            SetCount(size);
        }

        fx_t* operator[](int idx) const
        {
            return (fx_t*)FetchObject(idx);
        }
    };

    // ==============================================================

    typedef struct pending_fx_s
    {
        float x, y, z;
        fx_posdata_u pos_data;
        int pos_type;
        int def;
        int category;
        int handle;             // Valid if > 0
        int flags;
    }
    pending_fx_t;

    class pending_fx_c : public epi::array_c
    {
    public:
        pending_fx_c() : epi::array_c(sizeof(pending_fx_t)) 
        { 
        }
        
        ~pending_fx_c() 
        { 
            Clear(); 
        }
        
    private:
        void CleanupObject(void *obj) 
        { 
            // Nothing to do 
        }
        
    public:
        int GetActualSize()
        {
            return array_max_entries;
        }

        pending_fx_t* GetNew(void)
        {
            pending_fx_t *fx;

            fx = (pending_fx_t*)ExpandAtTail();
            memset(fx, 0, sizeof(pending_fx_t));
            return fx;
        }

        int GetSize()
        {
            return array_entries;
        }

        pending_fx_t* operator[](int idx) const
        {
            return (pending_fx_t*)FetchObject(idx);
        }
    };

    // ==============================================================

    typedef enum
    {
        FXLINKTYPE_PENDING,
        FXLINKTYPE_PLAYING,
        FXLINKTYPE_NUMTYPES
    }
    fxlinktype_e;    

    typedef enum
    {
        TESTFX_LACKSORIGIN    = 0x1,
        TESTFX_SINGULARLINKED = 0x2,
        TESTFX_TESTED         = 0x4,
        TESTFX_USE            = 0x8,
        TESTFX_MASK           = 0xFFFF
    }
    test_fx_flag_e;

    typedef struct test_fx_s
    {
        float dist;

        void *link;
        int link_type;

        struct test_fx_s *next;
        struct test_fx_s *prev;
        struct test_fx_s *left;
        struct test_fx_s *right;

        // Calculated at effect evaluation
        struct test_fx_s *tmp_next;  
        int flags;
    }
    test_fx_t;

    class test_fx_list_c : public epi::array_c
    {
    public:
        test_fx_list_c() : epi::array_c(sizeof(test_fx_t)) 
        { 
        }
        
        ~test_fx_list_c() 
        { 
            Clear(); 
        }
        
    private:
        test_fx_t *treehead;

        void CleanupObject(void *obj) 
        { 
            // Nothing to do 
        }

        test_fx_t* GetNew(void)
        {
            test_fx_t *oldhead;
            test_fx_t *fx;

            oldhead = (test_fx_t*)array;

            fx = (test_fx_t*)ExpandAtTail();
            memset(fx, 0, sizeof(test_fx_t));

            if (oldhead != (test_fx_t*)array) // Array moved for some reason
            {
                epi::array_iterator_c it;
                test_fx_t *fx2;
                size_t diff;

                diff = (size_t)array - (size_t)oldhead;
                
                if (treehead) 
                { 
                    treehead = (test_fx_t*)((size_t)treehead + (size_t)diff); 
                }

                // Fix all the pointers
                for (it = GetBaseIterator(); it.IsValid(); it++)
                {
                    fx2 = ITERATOR_TO_PTR(it, test_fx_t);
                    if (fx2->left)  
                    { 
                        fx2->left = 
                            (test_fx_t*)((size_t)fx2->left + (size_t)diff);  
                    }
 
                    if (fx2->right)  
                    { 
                        fx2->right = 
                            (test_fx_t*)((size_t)fx2->right + (size_t)diff);  
                    }

                    if (fx2->next)  
                    { 
                        fx2->next = 
                            (test_fx_t*)((size_t)fx2->next + (size_t)diff);  
                    }

                    if (fx2->prev)  
                    { 
                        fx2->prev = 
                            (test_fx_t*)((size_t)fx2->prev + (size_t)diff);  
                    }
                }
            }

            return fx;
        }

    public:
        void Add(float dist, void *link, int link_type)
        {
            test_fx_t *currnode, *node, *newnode;
            float cmp;
            enum { CREATE_HEAD, TO_LEFT, TO_RIGHT } act;
		
            // Convert to upper case
            act = CREATE_HEAD;
            currnode = NULL;

            // Due to the joys of having a managed array; we have
            // to rejig the pointers if it gets resized. (This
            // is a big FIXME as its wasteful). Therefore
            // we get the new node here.
            newnode = GetNew(); 

            node = treehead;
            while (node)
            {
                currnode = node;
			
                cmp = dist - node->dist;
                if (cmp < 0)
                {
                    act = TO_LEFT;
                    node = node->left;
                }
                else if (cmp >= 0)
                {
                    act = TO_RIGHT;
                    node = node->right;
                }
            }

            newnode->dist = dist;
            newnode->link = link;
            newnode->link_type = link_type;

            newnode->flags = TESTFX_USE;

            switch (act)
            {
			    case CREATE_HEAD:	
                { 
                    treehead = newnode; 
                    break;
                }
			
			    case TO_LEFT:		
                { 
                    // Update the binary tree
                    currnode->left = newnode;
				
                    // Update the linked list (Insert behind current node)
                    if (currnode->prev) 
                    { 
                        currnode->prev->next = newnode; 
                        newnode->prev = currnode->prev;
                    }
				
                    currnode->prev = newnode;
                    newnode->next = currnode; 
                    break;
                }
			
			    case TO_RIGHT:  	
                { 
                    // Update the binary tree
                    currnode->right = newnode; 
                    
                    // Update the linked list (Insert infront of current node)
                    if (currnode->next) 
                    { 
                        currnode->next->prev = newnode; 
                        newnode->next = currnode->next;
                    }
                    
                    currnode->next = newnode;
                    newnode->prev = currnode; 
                    break;
                }
			
			    default: 			
                { 
                    break; // FIXME!! Throw error
                }
            }
        }

        int GetActualSize()
        {
            return array_max_entries;
        }

        test_fx_t* GetLinkedListHead()
        {
            test_fx_t *curr = treehead;

            if (!treehead)
                return NULL;

            while(curr->prev)
                curr = curr->prev;

            return curr;
        }

        test_fx_t* GetTreeHead()
        {
            return treehead;
        }

        int GetLinkedListSize()
        {
            test_fx_t *fx = GetLinkedListHead();
            int count = 0;

            while (fx) { count++; fx = fx->next; }
            return count;
        }

        int GetSize()
        {
            return array_entries;
        }

        void Reset()
        {
            treehead = NULL;
            ZeroiseCount();
        }
    };
   
    // ==============================================================

    fx_datacache_c fx_datacache; // Effect data cache
    pending_fx_c pending_fx;     // List of pending effects
    fx_list_c playing_fx;        // List of playing effects
    test_fx_list_c test_fx;      // List of effects being tested

    int cat_limit[SNCAT_NUMTYPES];

    int pending_state;          

    mobj_t *listener = NULL;
    int last_fx_handle = -1;

    int master_fx_volume;

    // ==============================================================

    void CoordPlaysimToAudio(float *src, ALfloat *dest)
    {
        dest[0] = src[0];
        dest[1] = src[2];
        dest[2] = src[1] * -1.0f;
    }

    // ==============================================================

    //
    // GetNewFXHandle
    //
    int GetNewFXHandle()
    {
        epi::array_iterator_c it;
        pending_fx_t* pfx;
        fx_t* fx;

        int new_handle = last_fx_handle;
        int reset_count = 0;

        bool ok = false;

        while (!ok)
        {
            new_handle = new_handle + 1;
            if (new_handle < 1 || new_handle > 10240)
            {
                // Bail as opposed to going into an infinite loop
                if (reset_count > 1)
                    I_Error("[sound::GetNewFXHandle] Out of handles!");

                reset_count++;
                new_handle = 1;
            }

            ok = true;
            for (it = pending_fx.GetBaseIterator(); it.IsValid() && ok; it++)
            {
                pfx = ITERATOR_TO_PTR(it, pending_fx_t);
                if (pfx->handle > 0 && pfx->handle == new_handle)
                    ok = false;
            }            

            for (it = playing_fx.GetBaseIterator(); it.IsValid() && ok; it++)
            {
                fx = ITERATOR_TO_PTR(it, fx_t);
                if (fx->handle > 0 && fx->handle == new_handle)
                    ok = false;
            }            
        }

        last_fx_handle = new_handle;
        return new_handle;
    }

    //
    // GetNewPendingPlaybackFX
    //
    pending_fx_t *GetNewPendingPlaybackFX()
    {
        pending_fx_t *new_fx = pending_fx.GetNew();
        new_fx->handle = GetNewFXHandle();

        return new_fx;
    }

    // ==============================================================

    //
    // CalcApproxDistFromListener
    //
    float CalcApproxDistFromListener(float x, float y, float z)
    {
        if (!listener)
            return 0;

        double dx, dy; //, dz;

        // FIXME! The calc is clearly wrong!
        dx = fabs(x - listener->x); 
        dy = fabs(y - listener->y);
        //dz = fabs(z - listener->z);

        return (float)(dx * dx) + (dy * dy);
    }

    //
    // SetListener
    //
    void SetListener() 
    {
        if (::numplayers == 0)
        {
            listener = NULL;
            return;
        }
           
        ALfloat ori[ORI_NUMCOORDS];
        ALfloat pos[NUMCOORDS];

        ALfloat actpos[NUMCOORDS];
 
        listener = ::players[displayplayer]->mo;

        // Position
		pos[CX] = listener->x;
		pos[CY] = listener->y;
        if (listener->player)
            pos[CZ] = listener->player->viewz;
        else
            pos[CZ] = MO_MIDZ(listener);
        
        CoordPlaysimToAudio(pos, actpos);
        alListenerfv(AL_POSITION, actpos);

		//listen_veloc[CX] = listener->mom.x;
		//listen_veloc[CY] = listener->mom.y;
		//listen_veloc[CZ] = listener->mom.z;
        // SFX_FIXME: Velocity
        //CoordPlaysimToAudio(veloc, actpos);
        //alListenerfv(AL_VELOCITY, actpos);

        // Orientation
        pos[CX] = M_Cos(listener->angle);
        pos[CY] = M_Sin(listener->angle);
        pos[CZ] = 0.0f;
        CoordPlaysimToAudio(pos, ori + 3); // Horizontal adjustment (At)

        pos[CX] = 0.0f;
        pos[CY] = 0.0f;

        // swap-stereo by negating the UP vector
        pos[CZ] = swapstereo ? 1.0f : -1.0f; 

        CoordPlaysimToAudio(pos, ori);     // Vertical adjustment (Up)
        alListenerfv(AL_ORIENTATION, ori); 

        return;
    }

    // ==============================================================

    //
    // DoAllocSymbolicFX
    //
    void DoAllocSymbolicFX(int handle, int category)
    {
        fx_t *fx = playing_fx.GetSpareEntry();
        DEV_ASSERT2(fx != NULL);

        fx->pos_type = FXPOSTYPE_NOTUSED;
        fx->category = category;
        fx->handle = handle;
        fx->flags = FXFLAG_SYMBOLIC | FXFLAG_IGNOREPAUSE;
    }

    //
    // DoDeallocSymbolicFX
    //
    void DoDeallocSymbolicFX(fx_t* fx)
    {
        DEV_ASSERT2(fx->flags & FXFLAG_SYMBOLIC);
        fx->handle = 0;
    }

    //
    // DoStartFX
    //
    void DoStartFX(pending_fx_t* pfx)
    {
        DEV_ASSERT2(pfx != NULL);

        fx_t *fx = playing_fx.GetSpareEntry();
        DEV_ASSERT2(fx != NULL);

        ALfloat zeroes[] = { 0.0f, 0.0f, 0.0f };
        ALuint source_id;

        fx_data_t *data;
        sfxdef_c *def = sfxdefs[pfx->def];

        data = fx_datacache.GetEntry(pfx->def);
        if (!data || data->buffer_id == NO_BUFFER)
        {
            data = fx_datacache.Load(pfx->def, def);
        }
        DEV_ASSERT2(data != NULL);

        alGetError();
        alGenSources(1, &source_id);

        if (alGetError() != AL_NO_ERROR)
            return;

        bool looping;
	
        if (pfx->pos_type == FXPOSTYPE_STATIC ||
            pfx->pos_type == FXPOSTYPE_NOTUSED)
        {
            looping = false;
        }
        else
        {
            looping = def->looping;
        }

        fx->source_id = source_id;

        fx->x = pfx->x;
        fx->y = pfx->y;
        fx->z = pfx->z;

        fx->category = pfx->category;
        fx->pos_type = pfx->pos_type;

        if (fx->pos_type == FXPOSTYPE_OBJECT)
            fx->pos_data.mo = pfx->pos_data.mo;
        else if (fx->pos_type == FXPOSTYPE_SECTOR)
            fx->pos_data.sec = pfx->pos_data.sec;

        fx->def = pfx->def;
        fx->flags = pfx->flags;

        alSourcefv(source_id, AL_VELOCITY, zeroes); // SFX_FIXME: Velocity

        alSourcei(source_id, AL_SOURCE_RELATIVE, 
                  (pfx->pos_type == FXPOSTYPE_NOTUSED) ? AL_TRUE : AL_FALSE);
 
        alSourcei(source_id, AL_BUFFER, data->buffer_id);
        alSourcef(source_id, AL_ROLLOFF_FACTOR, 1.0f);

        alSourcef(source_id, AL_REFERENCE_DISTANCE, 
                  DEFAULT_REFERENCE_DISTANCE);

        alSourcef(source_id, AL_MIN_GAIN, 0.0f);
        alSourcef(source_id, AL_MAX_GAIN, 1.0f);

        alSourcei(source_id, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);

        SetFXGain(fx, def); // Set volume

        if (fx->pos_type != FXPOSTYPE_NOTUSED)
            SetFXPos(fx);
        else
            alSourcefv(source_id, AL_POSITION, zeroes);
  
        if (alGetError() != AL_NO_ERROR)
            return;

        alSourcePlay(source_id);

        if (alGetError() != AL_NO_ERROR)
            return;

        fx->handle = pfx->handle; // <-- This formalises this effect as valid

        data->count++;            // Increase the data reference count
        return;
    }

    //
    // DoStopFX
    //
    void DoStopFX(fx_t *fx)
    {
        DEV_ASSERT2(fx != NULL);
        DEV_ASSERT2(!(fx->flags & FXFLAG_SYMBOLIC));

        fx_data_t *data = fx_datacache.GetEntry(fx->def);

        alSourceStop(fx->source_id);
        alDeleteSources(1, &fx->source_id);

        alGetError(); // Clear any error
        
        fx->source_id = NO_SOURCE;
        fx->handle = 0;

        if (data)
            data->count--;
    }

    //
    // DoStopLoopingFX
    //
    void DoStopLoopingFX(fx_t *fx)
    {
        DEV_ASSERT2(fx != NULL);

        alSourcei(fx->source_id, AL_LOOPING, AL_FALSE);
    }

    //
    // DoStopPendingFX
    //
    void DoStopPendingFX(pending_fx_t *fx)
    {
        DEV_ASSERT2(fx != NULL);
        fx->handle = 0;
    }

    //
    // IsFXPlaying
    //
    bool IsFXPlaying(fx_t* fx)
    {
        if (fx->source_id < 0)
            return false;

		ALenum state;

		alGetSourcei(fx->source_id, AL_SOURCE_STATE, &state);

		if (state != AL_PLAYING && state != AL_PAUSED)
			return false;

        return true;
    }

    //
    // SetFXGain
    //
    void SetFXGain(fx_t* fx, sfxdef_c* def)
    {
        DEV_ASSERT2(fx != NULL && def != NULL);

        alSourcef(fx->source_id, 
                  AL_GAIN, 
                  (slider_to_gain[master_fx_volume] * 
                   PERCENT_2_FLOAT(def->volume)));
    }

    //
    // SetFXPos
    //
    void SetFXPos(fx_t* fx)
    {
        DEV_ASSERT2(fx != NULL);
        DEV_ASSERT2(fx->pos_type != FXPOSTYPE_NOTUSED);

        ALfloat actpos[NUMCOORDS];
        ALfloat pos[NUMCOORDS];
            
        pos[CX] = fx->x;
        pos[CY] = fx->y;
        pos[CZ] = fx->z;

        CoordPlaysimToAudio(pos, actpos);
        alSourcefv(fx->source_id, AL_POSITION, actpos);
    }

    // ==============================================================

    //
    // SetFXFlags
    //
    void SetFXFlags(int handle, int flags)
    {
        epi::array_iterator_c it;
        pending_fx_t* pfx;
        fx_t* fx;

        for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            pfx = ITERATOR_TO_PTR(it, pending_fx_t);
            if (pfx->handle > 0 && pfx->handle == handle)
            {
                pfx->flags = flags; 
                return; // There can only be one....
            }
        }            

        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle > 0 && fx->handle == handle)
            {
                fx->flags = flags;
                return; // There can only be one....
            }
        }            
    }

    //
    // StartFX (Basic Call)
    //
    int StartFX(sfx_t *info, int category, int flags)
    {
        if (!info)
            return 0; // SFX_FIXME: Should never happen

        pending_fx_t *fx = GetNewPendingPlaybackFX();

        fx->category = category;
        fx->def = LookupEffectDef(info);
        fx->pos_type = FXPOSTYPE_NOTUSED;
        fx->x = 0.0f;
        fx->y = 0.0f;
        fx->z = 0.0f;
        fx->flags = flags;

        return fx->handle;
    }

    //
    // StartFX (Position Based)
    //
    int StartFX(sfx_t *info, int category, const epi::vec3_c pos, int flags) 
    {
        if (!info)
            return 0; // SFX_FIXME: Should never happen

        pending_fx_t *fx = GetNewPendingPlaybackFX();

        fx->category = category;
        fx->def = LookupEffectDef(info);
        fx->pos_type = FXPOSTYPE_STATIC;
        fx->x = pos.x;
        fx->y = pos.y;
        fx->z = pos.z;
        fx->flags = flags;

        return fx->handle;
    }

    //
    // StartFX (Object Linkage)
    //
    int StartFX(sfx_t *info, int category, mobj_t *mo, int flags)
    {
        if (!info)
            return 0; // SFX_FIXME: Should never happen

        pending_fx_t *fx = GetNewPendingPlaybackFX();

        // SFX_FIXME: Insert dev_assert once testing complete
        if (!mo)
            return StartFX(info, category);

        fx->category = category;
        fx->def = LookupEffectDef(info);
        fx->pos_data.mo = mo;
        fx->pos_type = FXPOSTYPE_OBJECT;
        fx->x = mo->x;
        fx->y = mo->y;
        fx->z = mo->z;
        fx->flags = flags;

        return fx->handle;
    }

    //
    // StartFX (Sector Origin Linkage)
    //
    int StartFX(sfx_t *info, int category, sec_sfxorig_t *sec, int flags)
    {
        if (!info)
            return 0; // SFX_FIXME: Should never happen

        pending_fx_t *fx = GetNewPendingPlaybackFX();

        // SFX_FIXME: Insert dev_assert once testing complete
        if (!sec)
            return StartFX(info, category);

        fx->category = category;
        fx->def = LookupEffectDef(info);
        fx->pos_data.sec = sec;
        fx->pos_type = FXPOSTYPE_SECTOR;
        fx->x = sec->x;
        fx->y = sec->y;
        fx->z = sec->z;
        fx->flags = flags;

        return fx->handle;
    }

    //
    // StopFX (Using Handle)
    //
    void StopFX(int handle)
    {
        epi::array_iterator_c it;
        pending_fx_t* pfx;
        fx_t* fx;

        for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            pfx = ITERATOR_TO_PTR(it, pending_fx_t);
            if (pfx->handle > 0 && pfx->handle == handle)
            {
                DoStopPendingFX(pfx);
                return;
            }
        }            

        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle > 0 && fx->handle == handle)
            {
                DoStopFX(fx);
                return;
            }
        }            

        return;
    }

    //
    // StopFX (Using Object)
    //
    void StopFX(mobj_t *mo)
    {
        DEV_ASSERT2(mo != NULL);

        epi::array_iterator_c it;
        pending_fx_t* pfx;
        fx_t* fx;

        for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            pfx = ITERATOR_TO_PTR(it, pending_fx_t);
            if (pfx->handle <= 0)
                continue;

            if (pfx->pos_type == FXPOSTYPE_OBJECT &&
                pfx->pos_data.mo == mo)
            {
                DoStopPendingFX(pfx);
            }
        }            

        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle <= 0)
                continue;

            if (fx->pos_type == FXPOSTYPE_OBJECT &&
                fx->pos_data.mo == mo)
            {
                DoStopFX(fx); 
            }
        }            

        return;
    }

    //
    // StopFX (Using Sector Origin)
    //
    void StopFX(sec_sfxorig_t *orig)
    {
        DEV_ASSERT2(orig != NULL);

        epi::array_iterator_c it;
        pending_fx_t* pfx;
        fx_t* fx;

        for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            pfx = ITERATOR_TO_PTR(it, pending_fx_t);
            if (pfx->handle <= 0)
                continue;

            if (pfx->pos_type == FXPOSTYPE_SECTOR &&
                pfx->pos_data.sec == orig)
            {
                DoStopPendingFX(pfx);
            }
        }            

        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle <= 0)
                continue;

            if (fx->pos_type == FXPOSTYPE_SECTOR &&
                fx->pos_data.sec == orig)
            {
                DoStopFX(fx);
            }
        }            

        return;
    }

    //
    // StopLoopingFX (Using Handle)
    //
    void StopLoopingFX(int handle)
    {
        epi::array_iterator_c it;
        pending_fx_t* pfx;
        fx_t* fx;

        for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            pfx = ITERATOR_TO_PTR(it, pending_fx_t);
            if (pfx->handle > 0 && pfx->handle == handle)
            {
                DoStopPendingFX(pfx); // Never started, so don't ever start
                return;
            }
        }            

        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle > 0 && fx->handle == handle)
            {
                DoStopLoopingFX(fx);
                return;
            }
        }            

        return;
    }

    //
    // StopLoopingFX (Using Object)
    //
    void StopLoopingFX(mobj_t *mo)
    {
        DEV_ASSERT2(mo != NULL);

        epi::array_iterator_c it;
        pending_fx_t* pfx;
        fx_t* fx;

        for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            pfx = ITERATOR_TO_PTR(it, pending_fx_t);
            if (pfx->handle <= 0)
                continue;

            if (pfx->pos_type == FXPOSTYPE_OBJECT &&
                pfx->pos_data.mo == mo)
            {
                DoStopPendingFX(pfx); // Never started, so don't
            }
        }            

        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle <= 0)
                continue;

            if (fx->pos_type == FXPOSTYPE_OBJECT &&
                fx->pos_data.mo == mo)
            {
                DoStopLoopingFX(fx); 
            }
        }            

        return;
    }

    //
    // StopLoopingFX (Using Sector Origin)
    //
    void StopLoopingFX(sec_sfxorig_t *orig)
    {
        DEV_ASSERT2(orig != NULL);

        epi::array_iterator_c it;
        pending_fx_t* pfx;
        fx_t* fx;

        for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            pfx = ITERATOR_TO_PTR(it, pending_fx_t);
            if (pfx->handle <= 0)
                continue;

            if (pfx->pos_type == FXPOSTYPE_SECTOR &&
                pfx->pos_data.sec == orig)
            {
                DoStopPendingFX(pfx); // Never started, so don't start
            }
        }            

        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle <= 0)
                continue;

            if (fx->pos_type == FXPOSTYPE_SECTOR &&
                fx->pos_data.sec == orig)
            {
                DoStopLoopingFX(fx);
            }
        }            

        return;
    }

    //
    // PauseAllFX
    //
    void PauseAllFX()
    {
        if (pending_state == PENDINGSTATE_PAUSE)
            return; // Nothing to do

        if (pending_state == PENDINGSTATE_RESUME)
        {
            pending_state = PENDINGSTATE_NONE; // Cancel the resume
            return;
        }
        
        pending_state = PENDINGSTATE_PAUSE;
        return;
    }

    //
    // ResumeAllFX
    //
    void ResumeAllFX()
    {
        if (pending_state == PENDINGSTATE_RESUME)
            return; // Nothing to do

        if (pending_state == PENDINGSTATE_PAUSE)
        {
            pending_state = PENDINGSTATE_NONE; // Cancel the pause
            return;
        }
        
        pending_state = PENDINGSTATE_RESUME;
        return;
    }

    // ==============================================================

    //
    // ReserveFX
    //
    // Used by external entities to stick a reserved sign
    // on one of the playback entries. Intended to be
    // used by the music code which is pumped through
    // an OpenAL source.
    //
    int ReserveFX(int category)
    {
        if (category < 0 || category >= SNCAT_NUMTYPES)
            return 0;

        // The upper limit of reserved effects is the category 
        // limit since the sound system does not manage them. This
        // prevents reserve fx's hogging more effect channels
        // than is due to them.
        if (!cat_limit[category])
            return 0;

        // Look for a valid 
        if (playing_fx.GetSize())
        {
            epi::array_iterator_c it;
            fx_t *fx;
            int playing_fx_max = playing_fx.GetAllocatedSize();
            int count = 0;

            for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, fx_t);
                if (fx->handle <= 0)
                    continue;

                //DEV_ASSERT2(fx->category >= 0 && fx->category < SNCAT_NUMTYPES)                    
                //cat_count[fx->category]++;

                count++;
            }

            // Max'ed out?
            if (count >= playing_fx_max)
            {
                // Since this code is initially only going to be used
                // for reserving fx channels for music playback and
                // this only occurs at the beginning of a level, we
                // simple bail if the number of effects playing is
                // at its max. We should tyr to make space here by
                // removing an effect.
                return 0;
            }
        }

        int handle = GetNewFXHandle();
        DoAllocSymbolicFX(handle, category);
        return handle;
    }

    //
    // UnreserveFX
    //
    // Reversal of the above.
    //
    void UnreserveFX(int handle)
    {
        epi::array_iterator_c it;
        fx_t* fx;

        // Reserved FXs are only carried in the playing effect list
        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle > 0 && fx->handle == handle)
            {
                DoDeallocSymbolicFX(fx);
                return;
            }
        }            

        return;

    }

    // ==============================================================

    //
    // ClearTestFXFlag
    //
    void ClearTestFXFlag(test_fx_t* head)
    {
        test_fx_t* tfx;

        tfx = head;
        while (tfx)
        {
            tfx->flags &= ~TESTFX_TESTED;
            tfx = tfx->next;
        }
    }

#if 0
    //
    // DumpTestFXLink
    //
    void DumpTestFXLink(test_fx_t* head)
    {
        test_fx_t* tfx;

        tfx = head;
        while (tfx)
        {
            if (tfx->flags & TESTFX_USE &&
                tfx->flags & TESTFX_SINGULARLINKED &&
                !(tfx->flags & TESTFX_TESTED))
            {
                test_fx_t *tfx2 = tfx;
                int count = 0;

                while (tfx2)
                {
                    if (count)
                    {
                        I_Printf("->");
                    }


                    I_Printf("[%d]", count);

                    count++;

                    tfx2->flags |= TESTFX_TESTED;
                    tfx2 = tfx2->tmp_next;
                }

                I_Printf("\n");
            }
            

            tfx = tfx->next;
        }
    }
#endif

    //
    // HandleSingularity
    //
    void HandleSingularity(test_fx_t* head)
    {
        test_fx_t* tfx;

        tfx = head;
        while (tfx)
        {
            if (tfx->flags & TESTFX_USE)
            {
                int pos_type = FXPOSTYPE_NOTUSED;

                // Since symbolic effects are not linked, we
                // need not take any special provisions for
                // them here.

                if (tfx->link_type == FXLINKTYPE_PENDING)
                {
                    pending_fx_t* pfx = (pending_fx_t*)tfx->link;
                    pos_type = pfx->pos_type;
                }
                else if (tfx->link_type == FXLINKTYPE_PLAYING)
                {
                    fx_t* fx = (fx_t*)tfx->link;
                    pos_type = fx->pos_type;
                }

                if (pos_type != FXPOSTYPE_OBJECT &&
                    pos_type != FXPOSTYPE_SECTOR)
                {
                    tfx->flags |= TESTFX_LACKSORIGIN;
                }
            }

            tfx = tfx->next;
        }

        // Link objects
        tfx = head;
        while (tfx)
        {
            if (tfx->flags & TESTFX_USE &&
                !(tfx->flags & TESTFX_LACKSORIGIN || 
                  tfx->flags & TESTFX_SINGULARLINKED))
            {
                fx_posdata_u pos_data;
                int pos_type = 0;
                int def_id = 0;

                test_fx_t* last = tfx;

                if (tfx->link_type == FXLINKTYPE_PENDING)
                {
                    pending_fx_t* pfx = (pending_fx_t*)tfx->link;
                    memcpy(&pos_data, &pfx->pos_data, sizeof(fx_posdata_u));
                    pos_type = pfx->pos_type;
                    def_id = pfx->def;
                }
                else // implied: if (tfx->link_type == FXLINKTYPE_PLAYING)
                {
                    fx_t* fx = (fx_t*)tfx->link;
                    memcpy(&pos_data, &fx->pos_data, sizeof(fx_posdata_u));
                    pos_type = fx->pos_type;
                    def_id = fx->def;
                }

                if (!(tfx->flags & TESTFX_LACKSORIGIN))
                {
                    sfxdef_c* def = sfxdefs[def_id];
                    if (def->singularity)
                    {
                        tfx->flags |= TESTFX_SINGULARLINKED;
                        tfx->tmp_next = NULL;
                
                        test_fx_t* tfx2 = tfx->next;

                        fx_posdata_u pos_data2;
                        int pos_type2 = 0;
                        int def_id2 = 0;

                        while (tfx2)
                        {
                            if (tfx2->flags & TESTFX_USE &&
                                !(tfx2->flags & TESTFX_LACKSORIGIN || 
                                  tfx2->flags & TESTFX_SINGULARLINKED))
                            {
                                if (tfx2->link_type == FXLINKTYPE_PENDING)
                                {
                                    pending_fx_t* pfx = 
                                        (pending_fx_t*)tfx2->link;

                                    memcpy(&pos_data2, &pfx->pos_data, 
                                           sizeof(fx_posdata_u));

                                    pos_type2 = pfx->pos_type;
                                    def_id2 = pfx->def;
                                }
                                else 
                                {
                                    // implied: 
                                    // if (tfx->link_type == 
                                    //     FXLINKTYPE_PLAYING)
                                    fx_t* fx = (fx_t*)tfx2->link;

                                    memcpy(&pos_data2, &fx->pos_data, 
                                           sizeof(fx_posdata_u));

                                    pos_type2 = fx->pos_type;
                                    def_id2 = fx->def;
                                }

                                if (pos_type == pos_type2)
                                {
                                    bool match = false; 
                                    
                                    if (pos_type == FXPOSTYPE_OBJECT)
                                        match = (pos_data.mo == pos_data2.mo);
                                    else if (pos_type == FXPOSTYPE_SECTOR)
                                        match = (pos_data.sec == pos_data2.sec);

                                    if (match)
                                    {
                                        if (def_id != def_id2)
                                        {
                                            sfxdef_c* def2 = sfxdefs[def_id2];
                                            
                                            match = (def->singularity == 
                                                     def2->singularity);
                                        }
                                    
                                        if (match) 
                                        {
                                            tfx2->flags |= 
                                                TESTFX_SINGULARLINKED;

                                            tfx2->tmp_next = NULL;

                                            // Add to the previous entry
                                            last->tmp_next = tfx2; 

                                            // Now this our last entry
                                            last = tfx2;
                                        } // match[2]
                                    } // match[1]
                                } // pos_type == pos_type2
                            } //tfx2->flags

                            tfx2 = tfx2->next;
                        }//while(tfx2)
                    }//if(def->singularity)
                } //LACKSORIGIN
            } //tfx->flags
            tfx = tfx->next;
        }//while(tfx)

        tfx = head;
        while(tfx)
        {
            if (tfx->flags & TESTFX_USE &&
                !(tfx->flags & TESTFX_LACKSORIGIN) &&
                (tfx->flags & TESTFX_SINGULARLINKED))
            {
                test_fx_t *best_tfx_pending;
                test_fx_t *best_tfx_playing;

                best_tfx_pending = NULL;
                best_tfx_playing = NULL;

                test_fx_t* tfx2 = tfx;
                while (tfx2)
                {
                    if (tfx2->link_type == FXLINKTYPE_PENDING)
                    {
                        if (best_tfx_pending)
                        {
                            pending_fx_t* pfx = 
                                (pending_fx_t*)best_tfx_pending->link;
                            pending_fx_t* pfx2 = 
                                (pending_fx_t*)tfx2->link;

                            if (pfx->def != pfx2->def)
                            {
                                // Note that we only care if these 
                                // effects differ, there is currently little 
                                // point in evaluating the same effect being
                                // played at exactly the same point by the 
                                // same thing.
                                sfxdef_c* def = sfxdefs[pfx->def];
                                if (!def->precious)
                                {
                                    // The last entry is not precious 
                                    // therefore this is now the best
                                    best_tfx_pending = tfx2;
                                }
                            }
                        }
                        else
                        {
                            // This is currently the only candidate
                            best_tfx_pending = tfx2;
                        }

                        // If its not our best choice, we discard
                        if (best_tfx_pending != tfx2)
                        {
                            tfx2->flags &= ~TESTFX_USE;
                        }
                    }
                    else // implied: if (tfx->link_type == FXLINKTYPE_PLAYING)
                    {
                        if (best_tfx_playing)
                        {
                            fx_t* fx = (fx_t*)best_tfx_playing->link;
                            fx_t* fx2 = (fx_t*)tfx2->link;

                            if (fx->def != fx2->def)
                            {
                                // Note that we only care if these 
                                // effects differ, there is currently little 
                                // point in evaluating the same effect being
                                // played at exactly the same point by the 
                                // same thing.
                                sfxdef_c* def = sfxdefs[fx->def];
                                if (!def->precious)
                                {
                                    // The last entry is not precious 
                                    // therefore this is now the best
                                    best_tfx_playing = tfx2;
                                }
                            }
                        }
                        else
                        {
                            // This is currently the only candidate
                            best_tfx_playing = tfx2;
                        }

                        // If its not our best choice, we discard
                        if (best_tfx_playing != tfx2)
                        {
                            tfx2->flags &= ~TESTFX_USE;
                        }
                    }
                    
                    // No need to test twice
                    tfx2->flags &= ~TESTFX_SINGULARLINKED; 
                    
                    tfx2 = tfx2->tmp_next;
                }//while(tfx2)

                if (best_tfx_playing && best_tfx_pending)
                {
                    fx_t *fx = (fx_t*)best_tfx_playing;
                    sfxdef_c *def = sfxdefs[fx->def];
                    
                    //
                    // If the playing effect is precious then 
                    // it must remain playing, else the pending 
                    // effect should play.
                    //
                    if (def->precious) 
                    {
                        // Kill this pending effect
                        best_tfx_pending->flags &= ~TESTFX_USE;
                    }
                    else
                    {
                        // Disable this playing effect
                        best_tfx_playing->flags &= ~TESTFX_USE;
                    }
                }
            }//if(tfx->flags)
            tfx = tfx->next;
        }//while(tfx)
    }

    //
    // Ticker
    //
    void Ticker(void)
    {
        epi::array_iterator_c it;
        fx_t *fx;

        SetListener();

        // Kill off all sounds which have stopped 
        // playing and update the position of those
        // that are still playing
        if (playing_fx.GetSize())
        {
            for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, fx_t);
                if (fx->handle <= 0 || fx->flags & FXFLAG_SYMBOLIC)
                    continue;

                if (!IsFXPlaying(fx))
                {
                    // Ensure we cleanup the source and tidy up structs
                    DoStopFX(fx); 
                    continue;
                }

                // Sync object position with effect position
                if (fx->pos_type == FXPOSTYPE_OBJECT)
                {
                    fx->x = fx->pos_data.mo->x;
                    fx->y = fx->pos_data.mo->y;
                    fx->z = fx->pos_data.mo->z;
                } 
                else if (fx->pos_type == FXPOSTYPE_SECTOR)
                {
                    fx->x = fx->pos_data.sec->x;
                    fx->y = fx->pos_data.sec->y;
                    fx->z = fx->pos_data.sec->z;
                }
            }
        }
        
        test_fx.Reset();

        if (pending_fx.GetSize())
        {
            float dist;
            pending_fx_t* pfx;
            bool use;
        
            // Add Pending Effects to the test array
            for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
            {
                pfx = ITERATOR_TO_PTR(it, pending_fx_t);
                if (pfx->handle <= 0)
                    continue;

                use = true;
                if (pfx->pos_type != FXPOSTYPE_NOTUSED)
                {
                    // Sync object position with effect position
                    if (pfx->pos_type == FXPOSTYPE_OBJECT)
                    {
                        pfx->x = pfx->pos_data.mo->x;
                        pfx->y = pfx->pos_data.mo->y;
                        pfx->z = pfx->pos_data.mo->z;
                    } 
                    else if (pfx->pos_type == FXPOSTYPE_SECTOR)
                    {
                        pfx->x = pfx->pos_data.sec->x;
                        pfx->y = pfx->pos_data.sec->y;
                        pfx->z = pfx->pos_data.sec->z;
                    }

                    dist = CalcApproxDistFromListener(pfx->x, pfx->y, pfx->z);

                    sfxdef_c* def = sfxdefs[pfx->def];
                    if (dist > ((def->max_distance*def->max_distance)*2))
                        use = false; // Out of hearing range
                }
                else
                {
                    dist = 0.0f;
                }

                if (use)
                    test_fx.Add(dist, (void*)pfx, FXLINKTYPE_PENDING);
            }            
        }

        if (test_fx.GetSize())
        {
            // At this point we know that there are pending effects
            // which have not been discounted by the initial
            // distance check. Now we add all the existing effects
            // into the test_fx object for comparison.
            float dist;
            test_fx_t* tfx;

            for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, fx_t);
                if (fx->handle <= 0)
                    continue;

                if (fx->pos_type != FXPOSTYPE_NOTUSED)
                    dist = CalcApproxDistFromListener(fx->x, fx->y, fx->z);
                else
                    dist = 0.0f;
                
                test_fx.Add(dist, (void*)fx, FXLINKTYPE_PLAYING);
            }

            // Singularity Handling
            HandleSingularity(test_fx.GetLinkedListHead());
            ClearTestFXFlag(test_fx.GetLinkedListHead());

            // Count the effects used
            int count = 0;
            int playing_fx_max = playing_fx.GetAllocatedSize();

            tfx = test_fx.GetLinkedListHead();
            while (tfx)
            {
                if (tfx->flags & TESTFX_USE)
                    count++;

                tfx = tfx->next;
            }

            if (count > playing_fx_max)
            {
                // The number of effects we want to play (or continue playing) 
                // has exceeded the number we can play. Therefore we need to
                // prioritise as appropriate.
                int cat_count[SNCAT_NUMTYPES];
                int cat_rescount[SNCAT_NUMTYPES];
                int i;

                // Zeroise the category counts
                for (i=0; i<SNCAT_NUMTYPES; i++)
                {
                    cat_count[i] = 0;
                    cat_rescount[i] = 0;
                }

                // Populate the category count array
                tfx = test_fx.GetLinkedListHead();
                while (tfx)
                {
                    if (tfx->flags & TESTFX_USE)
                    {
                        int category = -1; 

                        if (tfx->link_type == FXLINKTYPE_PENDING)
                        {
                            pending_fx_t* pfx = (pending_fx_t*)tfx->link;
                            category = pfx->category;

                            DEV_ASSERT2(category >= 0 && category < SNCAT_NUMTYPES);
                        }
                        else if (tfx->link_type == FXLINKTYPE_PLAYING)
                        {
                            fx_t* fx = (fx_t*)tfx->link;
                            category = fx->category;

                            DEV_ASSERT2(category >= 0 && category < SNCAT_NUMTYPES);

                            if (fx->flags & FXFLAG_SYMBOLIC)
                            { 
                                // Keep a count of inert "effects"
                                cat_rescount[category]++;
                            }
                        }
                        
                        cat_count[category]++;
                    }

                    tfx = tfx->next;
                }

#if 0
                I_Printf("[A] Category Count: [%d] ", count);
                for (i=0; i<SNCAT_NUMTYPES; i++)
                    I_Printf("%d(%d) ", cat_count[i], cat_rescount[i]);
                I_Printf("\n");
#endif

                i = SNCAT_NUMTYPES - 1;
                while (i >= 0 && count > playing_fx_max)
                {
                    if (cat_count[i] > cat_limit[i])
                    {
                        int fx_gap = count - playing_fx_max;
                        int pos_gap = cat_count[i] - cat_limit[i];
                        int adj = MIN(pos_gap, fx_gap);

                        cat_count[i] -= adj;
                        count -= adj;

                        // Reserved effects can't be dealloced, so we have a problem
                        // if the category count is smaller than the reserved count.
                        DEV_ASSERT2(cat_count[i] >= cat_rescount[i]);
                    }

                    i--;
                }

                // Strip out the effects
                tfx = test_fx.GetLinkedListHead();
                while (tfx)
                {
                    if (tfx->flags & TESTFX_USE)
                    {
                        int category = -1; 

                        if (tfx->link_type == FXLINKTYPE_PENDING)
                        {
                            pending_fx_t* pfx = (pending_fx_t*)tfx->link;
                            category = pfx->category;
                            DEV_ASSERT2(category >= 0 && category < SNCAT_NUMTYPES);
                        }
                        else if (tfx->link_type == FXLINKTYPE_PLAYING)
                        {
                            fx_t* fx = (fx_t*)tfx->link;
                            category = fx->category;

                            DEV_ASSERT2(category >= 0 && category < SNCAT_NUMTYPES);

                            if (fx->flags & FXFLAG_SYMBOLIC)
                            {
                                // Got one the reserved effects
                                DEV_ASSERT2(cat_rescount[category]);
                                cat_rescount[category]--;
                            }
                        }
                
                        
                        // A potential minefield here.
                        if (cat_count[category] > cat_rescount[category])
                            cat_count[category]--;
                        else
                            tfx->flags &= ~TESTFX_USE;
         
                    }
                    tfx = tfx->next;
                }

#if 0
                // Zeroise the category counts
                for (i=0; i<SNCAT_NUMTYPES; i++)
                {
                    cat_count[i] = 0;
                    cat_rescount[i] = 0;
                }

                // Populate the category count array
                tfx = test_fx.GetLinkedListHead();
                while (tfx)
                {
                    if (tfx->flags & TESTFX_USE)
                    {
                        int category = -1; 

                        if (tfx->link_type == FXLINKTYPE_PENDING)
                        {
                            pending_fx_t* pfx = (pending_fx_t*)tfx->link;
                            category = pfx->category;
                        }
                        else if (tfx->link_type == FXLINKTYPE_PLAYING)
                        {
                            fx_t* fx = (fx_t*)tfx->link;
                            category = fx->category;

                            if (fx->flags & FXFLAG_SYMBOLIC)
                                cat_rescount[category]++;
                        }
                
                        cat_count[category]++;
                    }

                    tfx = tfx->next;
                }

                I_Printf("[B] Category Count: [%d] ", count);
                for (i=0; i<SNCAT_NUMTYPES; i++)
                    I_Printf("%d(%d) ", cat_count[i], cat_rescount[i]);
                I_Printf("\n\n");
#endif
            }

            // Destroy those playing but deselected
            tfx = test_fx.GetLinkedListHead();
            while (tfx)
            {
                if (!(tfx->flags & TESTFX_USE) && 
                    tfx->link_type == FXLINKTYPE_PLAYING)
                {
                    DoStopFX((fx_t*)tfx->link);
                }

                tfx = tfx->next;
            }

            // Create those not playing but selected
            tfx = test_fx.GetLinkedListHead();
            while (tfx)
            {
                if ((tfx->flags & TESTFX_USE) && 
                    tfx->link_type == FXLINKTYPE_PENDING)
                {
                    DoStartFX((pending_fx_t*)tfx->link);
                }

                tfx = tfx->next;
            }
        }

        pending_fx.ZeroiseCount(); // Empty the list

        // Post 
        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle <= 0)
                continue;

            // SFX_FIXME: Support updates for static 
            if (fx->pos_type == FXPOSTYPE_OBJECT ||
                fx->pos_type == FXPOSTYPE_SECTOR) 
            {
                SetFXPos(fx);
            }

            // Handle pause/resume
            if (pending_state == PENDINGSTATE_PAUSE)
            {
                if (!(fx->flags & FXFLAG_IGNOREPAUSE))
                {
                    // Pause the effect
                    ALenum state;

                    alGetSourcei(fx->source_id, AL_SOURCE_STATE, &state);

                    if (state == AL_PLAYING)
                        alSourcePause(fx->source_id);
                }
            }
            else if (pending_state == PENDINGSTATE_RESUME)
            {
                // Resume the effect
                ALenum state;

                alGetSourcei(fx->source_id, AL_SOURCE_STATE, &state);

                if (state == AL_PAUSED)
                    alSourcePlay(fx->source_id);
            }
        }

        if (pending_state != PENDINGSTATE_NONE) 
        { 
            pending_state = PENDINGSTATE_NONE; 
        }

        return;
    }

    // ==============================================================

    //
    // UnlinkFX
    //
    // Removes the link between object and effect. Useful
    // prior to object deletion, but you want the sound
    // to continue to completion.
    //
    void UnlinkFX(mobj_t* mo)
    {
        if (pending_fx.GetSize())
        {
            epi::array_iterator_c it;
            pending_fx_t* fx;

            for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, pending_fx_t);
                if (fx->handle > 0 && fx->pos_type == FXPOSTYPE_OBJECT)
                {
                    if(fx->pos_data.mo && fx->pos_data.mo == mo)
                    {
                        fx->pos_data.mo = NULL;
                        fx->pos_type = FXPOSTYPE_STATIC;
                    }
                }
            }
        }

        if (playing_fx.GetSize())
        {
            epi::array_iterator_c it;
            fx_t* fx;

            for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, fx_t);
                if (fx->handle > 0 && fx->pos_type == FXPOSTYPE_OBJECT)
                {
                    if(fx->pos_data.mo && fx->pos_data.mo == mo)
                    {
                        fx->pos_data.mo = NULL;
                        fx->pos_type = FXPOSTYPE_STATIC;
                    }
                }
            }
        }

        return;
    }

    //
    // UnlinkFX
    //
    // Removes the link between object and effect. Useful
    // prior to object deletion, but you want the sound
    // to continue to completion.
    //
    void UnlinkFX(sec_sfxorig_t* orig)
    {
        if (pending_fx.GetSize())
        {
            epi::array_iterator_c it;
            pending_fx_t* fx;

            for (it = pending_fx.GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, pending_fx_t);
                if (fx->handle > 0 && fx->pos_type == FXPOSTYPE_SECTOR)
                {
                    if(fx->pos_data.sec && fx->pos_data.sec == orig)
                    {
                        fx->pos_data.sec = NULL;
                        fx->pos_type = FXPOSTYPE_STATIC;
                    }
                }
            }
        }

        if (playing_fx.GetSize())
        {
            epi::array_iterator_c it;
            fx_t* fx;

            for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
            {
                fx = ITERATOR_TO_PTR(it, fx_t);
                if (fx->handle > 0 && fx->pos_type == FXPOSTYPE_SECTOR)
                {
                    if(fx->pos_data.sec && fx->pos_data.sec == orig)
                    {
                        fx->pos_data.sec = NULL;
                        fx->pos_type = FXPOSTYPE_STATIC;
                    }
                }
            }
        }

        return;
    }

    // ==============================================================

    //
    // GetVolume
    //
    int GetVolume(void) 
    { 
        return master_fx_volume; 
    }

    //
    // SetVolume
    //
    void SetVolume(int volume) 
    { 
        if (volume < 0 || volume >= SND_SLIDER_NUM)
            return;

        master_fx_volume = volume;

        // Adjust the gain of any playing fx
        epi::array_iterator_c it;
        fx_t* fx;
            
        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle > 0)
                SetFXGain(fx, sfxdefs[fx->def]);
        }
        return; 
    }

    // ==============================================================

    //
    // Init
    //
    void Init(void)
    {
        pending_fx.Clear();

        // Work out the number of sources required
        int num_sources;
        const char* p = M_GetParm("-maxaudiochan");
        if (p)
            num_sources = atoi(p);
        else
            num_sources = DEFAULT_SOURCES;
        
        if (num_sources < MIN_SOURCES || num_sources > MAX_SOURCES) 
        {
            I_Printf("I_StartupSound: Requested number of channels is out of range\n"); 

            I_Printf("I_StartupSound: Requested channels %d, Min is %d, Max is %d\n",
                     num_sources, MIN_SOURCES, MAX_SOURCES);
             
            I_Printf("sound::Init : Using default %d\n", DEFAULT_SOURCES); 
        }
        

        playing_fx.SetAllocatedSize(num_sources); // <-- Maximum playing fx

        // Sort out category limits from the source count
        int i;
        for (i=0; i<SNCAT_NUMTYPES; i++)
            cat_limit[i] = 0;

        while (num_sources)
        {
            for (i=0; i<SNCAT_NUMTYPES && num_sources; i++)
            {
                cat_limit[i]++;
                num_sources--;
            }
        }

        for (i=0; i<SNCAT_NUMTYPES; i++)
            I_Printf("Category %d - Limit %d\n", i, cat_limit[i]);

        pending_state = PENDINGSTATE_NONE;
        return;
    }

    //
    // Shutdown
    //
    void Shutdown(void)
    {
        fx_datacache.Clear();
        return;
    }

    //
    // Reset
    //
    void Reset(void)
    {
        // Stop all playing effects
        epi::array_iterator_c it;
        fx_t *fx;

        for (it = playing_fx.GetBaseIterator(); it.IsValid(); it++)
        {
            fx = ITERATOR_TO_PTR(it, fx_t);
            if (fx->handle > 0 && !(fx->flags & FXFLAG_SYMBOLIC))
                DoStopFX(fx);
        }            

        // Clear down the cache
        fx_datacache.Clear();       
        return;
    }

    // ==============================================================

    // Not-rejigged-yet stuff..
	int LookupEffectDef(const sfx_t *s) 
    { 
		int num;

		DEV_ASSERT2(s->num >= 1);
	
		// -KM- 1999/01/31 Using P_Random here means demos and net games 
		//  get out of sync.
		// -AJA- 1999/07/19: That's why we use M_Random instead :).

        if (s->num > 1)
            num = s->sounds[M_Random() % s->num];
        else
            num = s->sounds[0];

		DEV_ASSERT2(0 <= num && num < sfxdefs.GetSize());
	
		return num;
    }
};


