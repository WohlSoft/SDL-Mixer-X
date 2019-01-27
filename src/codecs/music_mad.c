/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2019 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifdef MUSIC_MP3_MAD

#include "music_mad.h"
#include "music_id3tag.h"

#include "mad.h"

/* NOTE: The dithering functions are GPL, which should be fine if your
         application is GPL (which would need to be true if you enabled
         libmad support in SDL_mixer). If you're using libmad under the
         commercial license, you need to disable this code.
*/
/************************ dithering functions ***************************/

#ifdef MUSIC_MP3_MAD_GPL_DITHERING

/* All dithering done here is taken from the GPL'ed xmms-mad plugin. */

/* Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.       */
/* Any feedback is very welcome. For any question, comments,       */
/* see http://www.math.keio.ac.jp/matumoto/emt.html or email       */
/* matumoto@math.keio.ac.jp                                        */

/* Period parameters */
#define MP3_DITH_N 624
#define MP3_DITH_M 397
#define MATRIX_A 0x9908b0df   /* constant vector a */
#define UPPER_MASK 0x80000000 /* most significant w-r bits */
#define LOWER_MASK 0x7fffffff /* least significant r bits */

/* Tempering parameters */
#define TEMPERING_MASK_B 0x9d2c5680
#define TEMPERING_MASK_C 0xefc60000
#define TEMPERING_SHIFT_U(y)  (y >> 11)
#define TEMPERING_SHIFT_S(y)  (y << 7)
#define TEMPERING_SHIFT_T(y)  (y << 15)
#define TEMPERING_SHIFT_L(y)  (y >> 18)

static unsigned long mt[MP3_DITH_N]; /* the array for the state vector  */
static int mti=MP3_DITH_N+1; /* mti==MP3_DITH_N+1 means mt[MP3_DITH_N] is not initialized */

/* initializing the array with a NONZERO seed */
static void sgenrand(unsigned long seed)
{
    /* setting initial seeds to mt[MP3_DITH_N] using         */
    /* the generator Line 25 of Table 1 in          */
    /* [KNUTH 1981, The Art of Computer Programming */
    /*    Vol. 2 (2nd Ed.), pp102]                  */
    mt[0]= seed & 0xffffffff;
    for (mti=1; mti<MP3_DITH_N; mti++)
        mt[mti] = (69069 * mt[mti-1]) & 0xffffffff;
}

static unsigned long genrand(void)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0, MATRIX_A};
    /* mag01[x] = x * MATRIX_A  for x=0,1 */

    if (mti >= MP3_DITH_N) { /* generate MP3_DITH_N words at one time */
        int kk;

        if (mti == MP3_DITH_N+1)   /* if sgenrand() has not been called, */
            sgenrand(4357); /* a default initial seed is used   */

        for (kk=0;kk<MP3_DITH_N-MP3_DITH_M;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+MP3_DITH_M] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        for (;kk<MP3_DITH_N-1;kk++) {
            y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
            mt[kk] = mt[kk+(MP3_DITH_M-MP3_DITH_N)] ^ (y >> 1) ^ mag01[y & 0x1];
        }
        y = (mt[MP3_DITH_N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
        mt[MP3_DITH_N-1] = mt[MP3_DITH_M-1] ^ (y >> 1) ^ mag01[y & 0x1];

        mti = 0;
    }

    y = mt[mti++];
    y ^= TEMPERING_SHIFT_U(y);
    y ^= TEMPERING_SHIFT_S(y) & TEMPERING_MASK_B;
    y ^= TEMPERING_SHIFT_T(y) & TEMPERING_MASK_C;
    y ^= TEMPERING_SHIFT_L(y);

    return y;
}

static long triangular_dither_noise(int nbits) {
    /* parameter nbits : the peak-to-peak amplitude desired (in bits)
     *  use with nbits set to    2 + nber of bits to be trimmed.
     * (because triangular is made from two uniformly distributed processes,
     * it starts at 2 bits peak-to-peak amplitude)
     * see The Theory of Dithered Quantization by Robert Alexander Wannamaker
     * for complete proof of why that's optimal
     */
    long v = (genrand()/2 - genrand()/2); /* in ]-2^31, 2^31[ */
    long P = 1 << (32 - nbits); /* the power of 2 */
    v /= P;
    /* now v in ]-2^(nbits-1), 2^(nbits-1) [ */

    return v;
}

#endif /* MUSIC_MP3_MAD_GPL_DITHERING */


#define MAD_INPUT_BUFFER_SIZE   (5*8192)

enum {
    MS_input_eof      = 0x0001,
    MS_input_error    = 0x0001,
    MS_decode_error   = 0x0002,
    MS_error_flags    = 0x000f
};

typedef struct {
    int play_count;
    SDL_RWops *src;
    Sint64 position_begin, current_buffer_start_offset;
    int freesrc;
    struct mad_stream stream;
    struct mad_frame frame;
    struct mad_synth synth;
    mad_timer_t next_frame_start;
    int volume;
    int status;
    SDL_AudioStream *audiostream;

    SDL_bool is_VBR;
    SDL_bool is_Frankenstein; /* Is a file which has frames with different characteristics like sample rate */
    size_t frames_nb;
    double *frame_timestamps_ms;
    long *frame_start_offsets;
    double total_length;
    int sample_rate;
    int sample_position;
    int frame_position;
    Mix_MusicMetaTags tags;

    unsigned char input_buffer[MAD_INPUT_BUFFER_SIZE + MAD_BUFFER_GUARD];
} MAD_Music;


static void read_update_buffer(struct mad_stream *stream, MAD_Music *music);

static void calculate_total_time(MAD_Music *music)
{
    mad_timer_t time = mad_timer_zero;
    long bitrate = -1;
    long samplerate = -1;
    struct mad_header header;
    struct mad_stream stream;
    mad_header_init(&header);
    mad_stream_init(&stream);
    size_t frame_infos_allocated = 1024;

    music->is_VBR = SDL_FALSE;
    music->is_Frankenstein = SDL_FALSE;
    music->total_length = 0;
    music->frames_nb = 0;

    music->frame_start_offsets = NULL;
    music->frame_timestamps_ms = SDL_malloc(frame_infos_allocated * sizeof(mad_timer_t));
    if (!music->frame_timestamps_ms) goto cleanerr;
    music->frame_start_offsets = SDL_malloc(frame_infos_allocated * sizeof(long));
    if (!music->frame_start_offsets) goto cleanerr;

    music->current_buffer_start_offset = 0;
    SDL_RWseek(music->src, music->position_begin, RW_SEEK_SET);

    while (1) {
        read_update_buffer(&stream, music);

        if (mad_header_decode(&header, &stream) == -1) {
            if (MAD_RECOVERABLE(stream.error)) {
                if ((music->status & MS_input_error) == 0) {
                    continue;
                }
                break;
            } else if (stream.error == MAD_ERROR_BUFLEN) {
                if ((music->status & MS_input_error) == 0) {
                    continue;
                }
                break;
            } else {
                Mix_SetError("mad_frame_decode() failed, corrupt stream?");
                music->status |= MS_decode_error;
                break;
            }
        }

        /* check if we have enough room or if we need to allocate some more
         * for the storage of time information on frames.
         */
        if (music->frames_nb >= frame_infos_allocated) {
            frame_infos_allocated *= 2;
            music->frame_timestamps_ms = SDL_realloc(music->frame_timestamps_ms, frame_infos_allocated * sizeof(mad_timer_t));
            if (!music->frame_timestamps_ms) goto cleanerr;
            music->frame_start_offsets = SDL_realloc(music->frame_start_offsets, frame_infos_allocated * sizeof(long));
            if (!music->frame_start_offsets) goto cleanerr;
        }
        /* memorise the time of the start of each frame */
        music->frame_timestamps_ms[music->frames_nb] = mad_timer_count(time, MAD_UNITS_MILLISECONDS);
        /* memorise the position of the start of each frame in the source buffer */
        music->frame_start_offsets[music->frames_nb++] = music->current_buffer_start_offset + (stream.this_frame - stream.buffer);
        music->sample_rate = (int)header.samplerate;
        mad_timer_add(&time, header.duration);

        /* Detect VBR and Frankenstein streams */
        if (bitrate >= 0) { /* if any frame has a different bitrate, it is a VBR MP3 */
            if(bitrate != header.bitrate) {
                music->is_VBR = SDL_TRUE;
            }
        } else {
            bitrate = header.bitrate;
        }
        /* if any frame has a different sample rate, it is a Frankenstein MP3, normally not legal */
        if (samplerate >= 0) {
            if (samplerate != header.samplerate) {
                music->is_Frankenstein = SDL_TRUE;
            }
        } else {
            samplerate = header.samplerate;
        }
    }

    music->total_length = (double)(mad_timer_count(time, (enum mad_units)music->sample_rate)) / (double)music->sample_rate;
    mad_stream_finish(&stream);
    mad_header_finish(&header);
    SDL_memset(music->input_buffer, 0, sizeof(music->input_buffer));

    music->status = 0;

    SDL_RWseek(music->src, music->position_begin, RW_SEEK_SET);
    music->current_buffer_start_offset = 0;

    return;

cleanerr: /* cleaning allocated stuff in case of error */
    SDL_OutOfMemory();
    if (music->frame_timestamps_ms) {
        SDL_free(music->frame_timestamps_ms);
        music->frame_timestamps_ms = NULL;
    }
    if (music->frame_start_offsets) {
        SDL_free(music->frame_start_offsets);
        music->frame_start_offsets = NULL;
    }
    music->frames_nb = 0;
    mad_stream_finish(&stream);
    mad_header_finish(&header);
}

static int MAD_Seek(void *context, double position);

static void *MAD_CreateFromRW(SDL_RWops *src, int freesrc)
{
    MAD_Music *music;

    music = (MAD_Music *)SDL_calloc(1, sizeof(MAD_Music));
    if (!music) {
        SDL_OutOfMemory();
        return NULL;
    }
    music->src = src;
    music->volume = MIX_MAX_VOLUME;

    meta_tags_init(&music->tags);
    music->position_begin = id3tag_fetchTags(&music->tags, music->src);
    calculate_total_time(music);

    mad_stream_init(&music->stream);
    mad_frame_init(&music->frame);
    mad_synth_init(&music->synth);
    mad_timer_reset(&music->next_frame_start);

    music->freesrc = freesrc;
    return music;
}

static void MAD_SetVolume(void *context, int volume)
{
    MAD_Music *music = (MAD_Music *)context;
    music->volume = volume;
}

static int MAD_GetVolume(void *context)
{
    MAD_Music *music = (MAD_Music *)context;
    return music->volume;
}

/* Starts the playback. */
static int MAD_Play(void *context, int play_count)
{
    MAD_Music *music = (MAD_Music *)context;
    music->play_count = play_count;
    return MAD_Seek(music, 0.0);
}


/*************************** TAG HANDLING: ******************************/

static SDL_INLINE SDL_bool is_id3v1(const Uint8 *data, size_t length)
{
    /* http://id3.org/ID3v1 :  3 bytes "TAG" identifier and 125 bytes tag data */
    if (length < 3 || SDL_memcmp(data,"TAG",3) != 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}
static SDL_INLINE SDL_bool is_id3v2(const Uint8 *data, size_t length)
{
    /* ID3v2 header is 10 bytes:  http://id3.org/id3v2.4.0-structure */
    /* bytes 0-2: "ID3" identifier */
    if (length < 10 || SDL_memcmp(data,"ID3",3) != 0) {
        return SDL_FALSE;
    }
    /* bytes 3-4: version num (major,revision), each byte always less than 0xff. */
    if (data[3] == 0xff || data[4] == 0xff) {
        return SDL_FALSE;
    }
    /* bytes 6-9 are the ID3v2 tag size: a 32 bit 'synchsafe' integer, i.e. the
     * highest bit 7 in each byte zeroed.  i.e.: 7 bit information in each byte ->
     * effectively a 28 bit value.  */
    if (data[6] >= 0x80 || data[7] >= 0x80 || data[8] >= 0x80 || data[9] >= 0x80) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}
static SDL_INLINE SDL_bool is_apetag(const Uint8 *data, size_t length)
{
   /* http://wiki.hydrogenaud.io/index.php?title=APEv2_specification
    * APEv2 header is 32 bytes: bytes 0-7 ident, bytes 8-11 version,
    * bytes 12-17 size.  bytes 24-31 are reserved: must be all zeroes.
    * APEv1 has no header, so no luck.  */
    Uint32 v;

    if (length < 32 || SDL_memcmp(data,"APETAGEX",8) != 0) {
        return SDL_FALSE;
    }
    v = (data[11]<<24) | (data[10]<<16) | (data[9]<<8) | data[8]; /* version */
    if (v != 2000U) {
        return SDL_FALSE;
    }
    v = 0; /* reserved bits : */
    if (SDL_memcmp(&data[24],&v,4) != 0 || SDL_memcmp(&data[28],&v,4) != 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static size_t get_tagsize(const Uint8 *data, size_t length)
{
    size_t size;

    if (is_id3v1(data, length)) {
        return 128; /* fixed length */
    }
    if (is_id3v2(data, length)) {
        /* size is a 'synchsafe' integer (see above) */
        size = (data[6]<<21) + (data[7]<<14) + (data[8]<<7) + data[9];
        size += 10; /* header size */
        /* ID3v2 header[5] is flags (bits 4-7 only, 0-3 are zero).
         * bit 4 set: footer is present (a copy of the header but
         * with "3DI" as ident.)  */
        if (data[5] & 0x10) {
            size += 10; /* footer size */
        }
        /* optional padding (always zeroes) */
        while (size < length && data[size] == 0) {
            ++size;
        }
        return size;
    }
    if (is_apetag(data, length)) {
        size = (data[15]<<24) | (data[14]<<16) | (data[13]<<8) | data[12];
        size += 32; /* header size */
        return size;
    }
    return 0;
}

static int consume_tag(struct mad_stream *stream)
{
    /* FIXME: what if the buffer doesn't have the full tag ??? */
    size_t remaining = stream->bufend - stream->next_frame;
    size_t tagsize = get_tagsize(stream->this_frame, remaining);
    if (tagsize != 0) {
        mad_stream_skip(stream, tagsize);
        return 0;
    }
    return -1;
}

/* Reads the next frame from the file.
   Returns true on success or false on failure.
 */
static void read_update_buffer(struct mad_stream *stream, MAD_Music *music)
{
    if (stream->buffer == NULL || stream->error == MAD_ERROR_BUFLEN) {
        size_t read_size;
        size_t remaining, used;
        unsigned char *read_start;

        /* There might be some bytes in the buffer left over from last
           time.    If so, move them down and read more bytes following
           them. */
        if (stream->next_frame != NULL) {
            used = stream->next_frame - stream->buffer;
            remaining = stream->bufend - stream->next_frame;
            memmove(music->input_buffer, stream->next_frame, remaining);
            read_start = music->input_buffer + remaining;
            read_size = MAD_INPUT_BUFFER_SIZE - remaining;

        } else {
            read_size = MAD_INPUT_BUFFER_SIZE;
            read_start = music->input_buffer;
            remaining = 0;
            used = 0;
        }

        /* Now read additional bytes from the input file. */
        read_size = SDL_RWread(music->src, read_start, 1, read_size);

        if (read_size == 0) {
            if ((music->status & (MS_input_eof | MS_input_error)) == 0) {
                /* FIXME: how to detect error? */
                music->status |= MS_input_eof;

                /* At the end of the file, we must stuff MAD_BUFFER_GUARD
                   number of 0 bytes. */
                SDL_memset(read_start + read_size, 0, MAD_BUFFER_GUARD);
                read_size += MAD_BUFFER_GUARD;
            }
        }

        /* Now feed those bytes into the libmad stream. */
        mad_stream_buffer(stream, music->input_buffer, read_size + remaining);
        stream->error = MAD_ERROR_NONE;
        music->current_buffer_start_offset += used;
    }
}

/* Reads the next frame from the file.
   Returns true on success or false on failure.
 */
static SDL_bool read_next_frame(MAD_Music *music)
{
    read_update_buffer(&music->stream, music);

    /* Now ask libmad to extract a frame from the data we just put in
       its buffer. */
    if (mad_frame_decode(&music->frame, &music->stream)) {
        if (MAD_RECOVERABLE(music->stream.error)) {
            consume_tag(&music->stream); /* consume any ID3 tags */
            mad_stream_sync(&music->stream); /* to frame seek mode */
            return SDL_FALSE;

        } else if (music->stream.error == MAD_ERROR_BUFLEN) {
            return SDL_FALSE;

        } else {
            Mix_SetError("mad_frame_decode() failed, corrupt stream?");
            music->status |= MS_decode_error;
            return SDL_FALSE;
        }
    }

    mad_timer_add(&music->next_frame_start, music->frame.header.duration);

    return SDL_TRUE;
}

/* Scale a MAD sample to 16 bits for output. */
static Sint16 scale(mad_fixed_t sample)
{
    const int n_bits_to_loose = MAD_F_FRACBITS + 1 - 16;

    /* round */
    sample += (1L << (n_bits_to_loose - 1));

#ifdef MUSIC_MP3_MAD_GPL_DITHERING
    sample += triangular_dither_noise(n_bits_to_loose + 1);
#endif

    /* clip */
    if (sample >= MAD_F_ONE)
        sample = MAD_F_ONE - 1;
    else if (sample < -MAD_F_ONE)
        sample = -MAD_F_ONE;

    /* quantize */
    return (Sint16)(sample >> n_bits_to_loose);
}

/* Once the frame has been read, copies its samples into the output buffer. */
static SDL_bool decode_frame(MAD_Music *music)
{
    struct mad_pcm *pcm;
    unsigned int i, nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;
    Sint16 *buffer, *dst;
    int result;

    mad_synth_frame(&music->synth, &music->frame);
    pcm = &music->synth.pcm;

    if (!music->audiostream) {
        music->audiostream = SDL_NewAudioStream(AUDIO_S16, pcm->channels, pcm->samplerate, music_spec.format, music_spec.channels, music_spec.freq);
        if (!music->audiostream) {
            return SDL_FALSE;
        }
    }

    nchannels = pcm->channels;
    nsamples = pcm->length;
    left_ch = pcm->samples[0];
    right_ch = pcm->samples[1];
    buffer = SDL_stack_alloc(Sint16, nsamples*nchannels);
    if (!buffer) {
        SDL_OutOfMemory();
        return SDL_FALSE;
    }

    dst = buffer;
    if (nchannels == 1) {
        for (i = nsamples; i--;) {
            *dst++ = scale(*left_ch++);
        }
    } else {
        for (i = nsamples; i--;) {
            *dst++ = scale(*left_ch++);
            *dst++ = scale(*right_ch++);
        }
    }

    music->sample_position += nsamples;
    music->frame_position += 1;

    result = SDL_AudioStreamPut(music->audiostream, buffer, (nsamples * nchannels * sizeof(Sint16)));
    SDL_stack_free(buffer);

    if (result < 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static int MAD_GetSome(void *context, void *data, int bytes, SDL_bool *done)
{
    MAD_Music *music = (MAD_Music *)context;
    int filled;

    if (music->audiostream) {
        filled = SDL_AudioStreamGet(music->audiostream, data, bytes);
        if (filled != 0) {
            return filled;
        }
    }

    if (!music->play_count) {
        /* All done */
        *done = SDL_TRUE;
        return 0;
    }

    if (read_next_frame(music)) {
        if (!decode_frame(music)) {
            return -1;
        }
    } else if (music->status & MS_input_eof) {
        int play_count = -1;
        if (music->play_count > 0) {
            play_count = (music->play_count - 1);
        }
        if (MAD_Play(music, play_count) < 0) {
            return -1;
        }
    } else if (music->status & MS_decode_error) {
        return -1;
    }
    return 0;
}
static int MAD_GetAudio(void *context, void *data, int bytes)
{
    MAD_Music *music = (MAD_Music *)context;
    return music_pcm_getaudio(context, data, bytes, music->volume, MAD_GetSome);
}

static int MAD_Seek(void *context, double position)
{
    MAD_Music *music = (MAD_Music *)context;
    mad_timer_t target;
    int int_part;

    int_part = (int)position;
    mad_timer_set(&target, (unsigned long)int_part, (unsigned long)((position - int_part) * 1000000), 1000000);

    music->sample_position = (int)(position * music->sample_rate);
    music->frame_position = music->frames_nb * (position / music->total_length);

    if (mad_timer_compare(music->next_frame_start, target) > 0) {
        /* In order to seek backwards in a VBR file, we have to rewind and
           start again from the beginning.    This isn't necessary if the
           file happens to be CBR, of course; in that case we could seek
           directly to the frame we want.    But I leave that little
           optimization for the future developer who discovers she really
           needs it. */
        mad_timer_reset(&music->next_frame_start);
        music->status &= ~MS_error_flags;

        SDL_RWseek(music->src, music->position_begin, RW_SEEK_SET);
        SDL_memset(music->input_buffer, 0, sizeof(music->input_buffer));
    }

    /* Now we have to skip frames until we come to the right one.
       Again, only truly necessary if the file is VBR. */
    while (mad_timer_compare(music->next_frame_start, target) < 0) {
        if (!read_next_frame(music)) {
            if ((music->status & MS_error_flags) != 0) {
                /* Couldn't read a frame; either an error condition or
                     end-of-file.    Stop. */
                return Mix_SetError("Seek position out of range");
            }
        }
    }

    /* Here we are, at the beginning of the frame that contains the
       target time.    Ehh, I say that's close enough.    If we wanted to,
       we could get more precise by decoding the frame now and counting
       the appropriate number of samples out of it. */
    return 0;
}

static int MAD_SeekSec(void *context, double position)
{
    MAD_Music *music = (MAD_Music *)context;
    long goal_frame_i;

    if (music->frames_nb == 0) {
        Mix_SetError("Won't seek in an empty file");
        return -1;
    }

    if (position < 0 || position >= music->total_length) {
        Mix_SetError("Seek position out of range");
        return -1;
    }

    goal_frame_i = music->frames_nb * (position / music->total_length);

    /* protection against unlikely float rounding fantasies */
    if (goal_frame_i >= music->frames_nb) goal_frame_i = music->frames_nb - 1;

    if (music->is_Frankenstein) {
        /* In a frankenstein MP3, frames may have a different duration
           (this is normally not allowed) so the frame index is only
           an approximation supposing an average duration. */
        /* From there we move, if needed, forward or backward to
           find the right frame. Well, *around* the right frame... */
        if (music->frame_timestamps_ms[goal_frame_i] > position * 1000) {
            do {
                goal_frame_i--;
                if (goal_frame_i <= 0) break;
            } while(music->frame_timestamps_ms[goal_frame_i] > position * 1000);
        } else {
            while (music->frame_timestamps_ms[goal_frame_i] < position * 1000) {
                goal_frame_i++;
                if (goal_frame_i >= music->frames_nb-1) break;
            }
        }
    }

    /* Position the source file buffer */
    SDL_RWseek(music->src, music->position_begin + music->frame_start_offsets[goal_frame_i], RW_SEEK_SET);

    /* Clean various audio buffers from previous data */
    SDL_AudioStreamClear(music->audiostream);
    SDL_memset(music->input_buffer, 0, sizeof(music->input_buffer));
    music->stream.next_frame = NULL;
    music->stream.buffer = NULL;
    mad_frame_mute(&music->frame);
    mad_synth_mute(&music->synth);

    /* Position the indicators */
    /* warning: for now, assuming this is no frankenstein MP3,
     * otherwise we're in trouble, since sample_rate may vary */
    music->sample_position = (music->frame_timestamps_ms[goal_frame_i] * music->sample_rate) / 1000;
    music->frame_position = goal_frame_i;

    /* Proceed with next frame */
    read_next_frame(music);

    return 0;
}

static int MAD_SkipSec(void *context, double delay_s)
{
    MAD_Music *music = (MAD_Music *)context;
    double target_position_ms;

    if (music->frames_nb == 0) {
        Mix_SetError("Won't skip in an empty file");
        return -1;
    }
    target_position_ms = music->frame_timestamps_ms[music->frame_position] + delay_s*1000;
    return MAD_SeekSec(music, target_position_ms/1000);
}


static double MAD_tell(void *context)
{
    MAD_Music *music = (MAD_Music *)context;
    return music->frame_timestamps_ms[music->frame_position] / 1000.0;
/*
    return (double)music->sample_position / (double)music->sample_rate;
*/
}

static double MAD_total(void *context)
{
    MAD_Music *music = (MAD_Music *)context;
    return music->total_length;
}

static const char* MAD_GetMetaTag(void *context, Mix_MusicMetaTag tag_type)
{
    MAD_Music *music = (MAD_Music *)context;
    return meta_tags_get(&music->tags, tag_type);
}

static void MAD_Delete(void *context)
{
    MAD_Music *music = (MAD_Music *)context;
    mad_stream_finish(&music->stream);
    mad_frame_finish(&music->frame);
    mad_synth_finish(&music->synth);
    meta_tags_clear(&music->tags);

    if(music->frame_timestamps_ms) {
        SDL_free(music->frame_timestamps_ms);
    }
    if(music->frame_start_offsets) {
        SDL_free(music->frame_start_offsets);
    }

    if (music->audiostream) {
        SDL_FreeAudioStream(music->audiostream);
    }
    if (music->freesrc) {
        SDL_RWclose(music->src);
    }
    SDL_free(music);
}

Mix_MusicInterface Mix_MusicInterface_MAD =
{
    "MAD",
    MIX_MUSIC_MAD,
    MUS_MP3,
    SDL_FALSE,
    SDL_FALSE,

    NULL,   /* Load */
    NULL,   /* Open */
    MAD_CreateFromRW,
    NULL,   /* CreateFromRWex [MIXER-X]*/
    NULL,   /* CreateFromFile */
    NULL,   /* CreateFromFileEx [MIXER-X]*/
    MAD_SetVolume,
    MAD_GetVolume,   /* GetVolume [MIXER-X]*/
    MAD_Play,
    NULL,   /* IsPlaying */
    MAD_GetAudio,
    MAD_Seek,
    MAD_SeekSec, /* SeekSec [MIXER-X]*/
    MAD_SkipSec, /* SkipSec [MIXER-X]*/
    MAD_tell, /* Tell [MIXER-X]*/
    MAD_total,/* FullLength [MIXER-X]*/
    NULL,   /* LoopStart [MIXER-X]*/
    NULL,   /* LoopEnd [MIXER-X]*/
    NULL,   /* LoopLength [MIXER-X]*/
    MAD_GetMetaTag, /* GetMetaTag [MIXER-X]*/
    NULL,   /* Pause */
    NULL,   /* Resume */
    NULL,   /* Stop */
    MAD_Delete,
    NULL,   /* Close */
    NULL,   /* Unload */
};

#endif /* MUSIC_MP3_MAD */

/* vi: set ts=4 sw=4 expandtab: */
