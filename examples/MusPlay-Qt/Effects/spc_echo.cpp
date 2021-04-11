#include <tgmath.h>
#include "SDL_audio.h"
#include "SDL_assert.h"
#ifdef USE_SDL_MIXER_X
#   include "SDL_mixer_ext.h"
#else
#   include "SDL_mixer.h"
#endif

#include "spc_echo.h"

//#define WAVE_DEEP_DEBUG
#ifdef WAVE_DEEP_DEBUG // Dirty debug
#define WAVE_PATH "/home/vitaly/Музыка/spc_echo_dump-"
#include "wave_writer.h"
#endif


#ifndef INT8_MIN
#define INT8_MIN    (-0x7f - 1)
#endif
#ifndef INT16_MAX
#define INT16_MAX   0x7fff
#endif


// Float32-LE
static int getFloatLSBSample(Uint8 *raw, int c)
{
    Uint32 r;
    float f;
    void *t;
    raw += (c * sizeof(float));
    r = (((Uint32)raw[0] <<  0) & 0x000000FF) |
        (((Uint32)raw[1] <<  8) & 0x0000FF00) |
        (((Uint32)raw[2] << 16) & 0x00FF0000) |
        (((Uint32)raw[3] << 24) & 0xFF000000);
    t = &r;
    f = *(float*)t;
    return (int)(f * 32768.f);
}
static void setFloatLSBSample(Uint8 **raw, int ov)
{
    float f = (float(ov) / 32768.f);
    void *t = &f;
    Uint32 r = *(Uint32*)t;
    *(*raw)++ = (Uint8)((r >> 0) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 8) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 16) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 24) & 0xFF);
}

// Float32-BE
static int getFloatMSBSample(Uint8 *raw, int c)
{
    Uint32 r;
    float f;
    void *t;
    raw += (c * sizeof(float));
    r = (((Uint32)raw[3] <<  0) & 0x000000FF) |
        (((Uint32)raw[2] <<  8) & 0x0000FF00) |
        (((Uint32)raw[1] << 16) & 0x00FF0000) |
        (((Uint32)raw[0] << 24) & 0xFF000000);
    t = &r;
    f = *(float*)t;
    return (int)(f * 32768.f);
}
static void setFloatMSBSample(Uint8 **raw, int ov)
{
    float f = (float(ov) / 32768.f);
    void *t = &f;
    Uint32 r = *(Uint32*)t;
    *(*raw)++ = (Uint8)((r >> 24) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 16) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 8) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 0) & 0xFF);
}

// Sint32-LE
static int getInt32LSB(Uint8 *raw, int c)
{
    Uint32 r;
    Sint32 f;
    raw += (c * sizeof(Sint32));
    r = (((Uint32)raw[0] <<  0) & 0x000000FF) |
        (((Uint32)raw[1] <<  8) & 0x0000FF00) |
        (((Uint32)raw[2] << 16) & 0x00FF0000) |
        (((Uint32)raw[3] << 24) & 0xFF000000);
    f = *(Sint32*)(&r);
    return (int)(f / 65536);
}
static void setInt32LSB(Uint8 **raw, int ov)
{
    Sint32 f = (Sint32(ov) * 65536);
    Uint32 r = *(Uint32*)(&f);
    *(*raw)++ = (Uint8)((r >> 0) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 8) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 16) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 24) & 0xFF);
}

// Sint32-BE
static int getInt32MSB(Uint8 *raw, int c)
{
    Uint32 r;
    Sint32 f;
    raw += (c * sizeof(Sint32));
    r = (((Uint32)raw[3] <<  0) & 0x000000FF) |
        (((Uint32)raw[2] <<  8) & 0x0000FF00) |
        (((Uint32)raw[1] << 16) & 0x00FF0000) |
        (((Uint32)raw[0] << 24) & 0xFF000000);
    f = *(Sint32*)(&r);
    return (int)(f / 65536);
}
static void setInt32MSB(Uint8 **raw, int ov)
{
    Sint32 f = (Sint32(ov) * 65536);
    Uint32 r = *(Uint32*)(&f);
    *(*raw)++ = (Uint8)((r >> 24) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 16) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 8) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 0) & 0xFF);
}

// Sint16-LE
static int getInt16LSB(Uint8 *raw, int c)
{
    Uint16 r;
    Sint16 f;
    raw += (c * sizeof(Sint16));
    r = (((Uint16)raw[0] <<  0) & 0x00FF) |
        (((Uint16)raw[1] <<  8) & 0xFF00);
    f = *(Sint16*)(&r);
    return f;
}
static void setInt16LSB(Uint8 **raw, int ov)
{
    Sint16 f = Sint16(ov);
    Uint16 r = *(Uint16*)(&f);
    *(*raw)++ = (Uint8)((r >> 0) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 8) & 0xFF);
}

// Sint16-BE
static int getInt16MSB(Uint8 *raw, int c)
{
    Uint16 r;
    Sint16 f;
    raw += (c * sizeof(Sint16));
    r = (((Uint16)raw[1] <<  0) & 0x00FF) |
        (((Uint16)raw[0] <<  8) & 0xFF00);
    f = *(Sint16*)(&r);
    return f;
}
static void setInt16MSB(Uint8 **raw, int ov)
{
    Sint16 f = Sint16(ov);
    Uint16 r = *(Uint16*)(&f);
    *(*raw)++ = (Uint8)((r >> 8) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 0) & 0xFF);
}

// Uint16-LE
static int getUInt16LSB(Uint8 *raw, int c)
{
    Uint16 r;
    int f;
    raw += (c * sizeof(Uint16));
    r = (((Uint16)raw[0] <<  0) & 0x00FF) |
        (((Uint16)raw[1] <<  8) & 0xFF00);
    f = (int)r + INT16_MIN;
    return f;
}
static void setUInt16LSB(Uint8 **raw, int ov)
{
    Sint16 f = Sint16(ov - INT16_MIN);
    Uint16 r = *(Uint16*)(&f);
    *(*raw)++ = (Uint8)((r >> 0) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 8) & 0xFF);
}

// Uint16-BE
static int getUInt16MSB(Uint8 *raw, int c)
{
    Uint16 r;
    int f;
    raw += (c * sizeof(Uint16));
    r = (((Uint16)raw[1] <<  0) & 0x00FF) |
        (((Uint16)raw[0] <<  8) & 0xFF00);
    f = (int)r + INT16_MIN;
    return f;
}
static void setUInt16MSB(Uint8 **raw, int ov)
{
    Sint16 f = Sint16(ov - INT16_MIN);
    Uint16 r = *(Uint16*)(&f);
    *(*raw)++ = (Uint8)((r >> 8) & 0xFF);
    *(*raw)++ = (Uint8)((r >> 0) & 0xFF);
}

// Sint8
static int getInt8(Uint8 *raw, int c)
{
    int f;
    raw += (c * sizeof(Sint8));
    f = (int)(Sint8)(*raw) * 256;
    return f;
}
static void setInt8(Uint8 **raw, int ov)
{
    *(*raw)++ = (Uint8)(ov / 256);
}

// Uint8
static int getUInt8(Uint8 *raw, int c)
{
    int f;
    raw += (c * sizeof(Sint8));
    f = ((int)*raw + INT8_MIN) * 256;
    return f;
}
static void setUInt8(Uint8 **raw, int ov)
{
    *(*raw)++ = (Uint8)((ov / 256) - INT8_MIN);
}


#define CLAMP16( io )\
    {\
        if ( (Sint16) io != io )\
            io = (io >> 31) ^ 0x7FFF;\
    }

#define ECHO_HIST_SIZE  8
#define SDSP_RATE       32000
#define MAX_CHANNELS    10
#define ECHO_BUFFER_SIZE (32 * 1024 * MAX_CHANNELS)

//// Global registers
//enum
//{
//    r_mvoll = 0x0C, r_mvolr = 0x1C,
//    r_evoll = 0x2C, r_evolr = 0x3C,
//    r_kon   = 0x4C, r_koff  = 0x5C,
//    r_flg   = 0x6C, r_endx  = 0x7C,
//    r_efb   = 0x0D, r_pmon  = 0x2D,
//    r_non   = 0x3D, r_eon   = 0x4D,
//    r_dir   = 0x5D, r_esa   = 0x6D,
//    r_edl   = 0x7D,
//    r_fir   = 0x0F // 8 coefficients at 0x0F, 0x1F ... 0x7F
//};

//static uint8_t const initial_regs[register_count] =
//{
///*      0      1     2     3    4     5     6     7     8      9     A    B     C      D     E     F  */
///*00*/ 0x45, 0x8B, 0x5A, 0x9A, 0xE4, 0x82, 0x1B, 0x78, 0x00, 0x00, 0xAA, 0x96, 0x89, 0x0E, 0xE0, 0x80, /*00*/
///*10*/ 0x2A, 0x49, 0x3D, 0xBA, 0x14, 0xA0, 0xAC, 0xC5, 0x00, 0x00, 0x51, 0xBB, 0x9C, 0x4E, 0x7B, 0xFF, /*10*/
///*20*/ 0xF4, 0xFD, 0x57, 0x32, 0x37, 0xD9, 0x42, 0x22, 0x00, 0x00, 0x5B, 0x3C, 0x9F, 0x1B, 0x87, 0x9A, /*20*/
///*30*/ 0x6F, 0x27, 0xAF, 0x7B, 0xE5, 0x68, 0x0A, 0xD9, 0x00, 0x00, 0x9A, 0xC5, 0x9C, 0x4E, 0x7B, 0xFF, /*30*/
///*40*/ 0xEA, 0x21, 0x78, 0x4F, 0xDD, 0xED, 0x24, 0x14, 0x00, 0x00, 0x77, 0xB1, 0xD1, 0x36, 0xC1, 0x67, /*40*/
///*50*/ 0x52, 0x57, 0x46, 0x3D, 0x59, 0xF4, 0x87, 0xA4, 0x00, 0x00, 0x7E, 0x44, 0x9C, 0x4E, 0x7B, 0xFF, /*50*/
///*60*/ 0x75, 0xF5, 0x06, 0x97, 0x10, 0xC3, 0x24, 0xBB, 0x00, 0x00, 0x7B, 0x7A, 0xE0, 0x60, 0x12, 0x0F, /*60*/
///*70*/ 0xF7, 0x74, 0x1C, 0xE5, 0x39, 0x3D, 0x73, 0xC1, 0x00, 0x00, 0x7A, 0xB3, 0xFF, 0x4E, 0x7B, 0xFF  /*70*/
///*      0      1     2     3    4     5     6     7     8      9     A    B     C      D     E     F  */
//};

#define RESAMPLED_FIR

struct SpcEcho
{
    SDL_bool is_valid = SDL_FALSE;
    int echo_ram[ECHO_BUFFER_SIZE];

    // Echo history keeps most recent 8 samples (twice the size to simplify wrap handling)
    int echo_hist[ECHO_HIST_SIZE * 2 * MAX_CHANNELS][MAX_CHANNELS];
    int (*echo_hist_pos)[MAX_CHANNELS] = echo_hist; // &echo_hist [0 to 7]

    //! offset from ESA in echo buffer
    int echo_offset = 0;
    //! number of bytes that echo_offset will stop at
    int echo_length = 0;

    double  rate_factor = 1.0;
    double  channels_factor = 1.0;
    int     rate = SDSP_RATE;
    int     channels = 2;
    int     sample_size = 2;
    Uint16  format = AUDIO_F32SYS;

    //! Flags
    Uint8 reg_flg = 0;
    //! $4d rw EON - Echo enable
    Uint8 reg_eon = 0;
    //! $7d rw EDL - Echo delay (ring buffer size)
    Uint8 reg_edl = 0;
    //! $0d rw EFB - Echo feedback volume
    Sint8 reg_efb = 0;

    Sint8 reg_mvoll = 0;
    Sint8 reg_mvolr = 0;
    Sint8 reg_evoll = 0;
    Sint8 reg_evolr = 0;

#ifdef RESAMPLED_FIR
#define RSM_FRAC        10
    //! FIR resampler
    double fir_stream_rateratio = 0;
    double fir_stream_samplecnt = 0;
    int    fir_stream_old_samples[MAX_CHANNELS];
    int    fir_stream_samples[MAX_CHANNELS];
    double fir_stream_rateratio_back = 0;
    double fir_stream_samplecnt_back = 0;
    int    fir_stream_old_samples_back[MAX_CHANNELS];
    int    fir_stream_samples_back[MAX_CHANNELS];
    int    fir_stream_midbuffer_in[20][MAX_CHANNELS];
    int    fir_stream_midbuffer_out[20][MAX_CHANNELS];
    int    fir_buffer_size = 0;
    int    fir_buffer_read = 0;
#endif


#ifdef WAVE_DEEP_DEBUG
    void *debugIn;
    void *debugInRes;
    void *debugOutRes;
    void *debugOut;
#endif

    //! FIR Defaults: 80 FF 9A FF 67 FF 0F FF
    const Uint8 reg_fir_initial[8] = {0x80, 0xFF, 0x9A, 0xFF, 0x67, 0xFF, 0x0F, 0xFF};
    //! $xf rw FFCx - Echo FIR Filter Coefficient (FFC) X
    Sint8 reg_fir[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    void setDefaultFir()
    {
        SDL_memcpy(reg_fir, reg_fir_initial, 8);
    }

    void setDefaultRegs()
    {
        reg_flg = 0;

        reg_mvoll = (Sint8)0x89;
        reg_mvolr = (Sint8)0x9C;
        reg_evoll = (Sint8)0x9F;
        reg_evolr = (Sint8)0x9C;

        setDefaultFir();

        // $0d rw EFB - Echo feedback volume
        reg_efb = (Sint8)0x0E;
        // $4d rw EON - Echo enable
        reg_eon = 1;
        // $7d rw EDL - Echo delay (ring buffer size)
        reg_edl = 3;
    }

    int (*readSample)(Uint8 *raw, int channel);
    void (*writeSample)(Uint8 **raw, int sample);

    int init(int i_rate, Uint16 i_format, int i_channels)
    {
        is_valid = SDL_FALSE;
        rate = i_rate;
        format = i_format;
        channels = i_channels;
        rate_factor = double(i_rate) / SDSP_RATE;
        channels_factor = double(i_channels) / 2.0;

        if(i_rate < 4000)
            return -1; /* Too small sample rate */

#ifdef RESAMPLED_FIR
        fir_stream_rateratio = (double)SDSP_RATE / i_rate;
        fir_stream_rateratio_back = (double)i_rate / SDSP_RATE;
        fir_stream_samplecnt = 0;
        fir_stream_samplecnt_back = 0;
        SDL_memset(fir_stream_old_samples, 0, sizeof(fir_stream_old_samples));
        SDL_memset(fir_stream_samples, 0, sizeof(fir_stream_samples));
        SDL_memset(fir_stream_old_samples_back, 0, sizeof(fir_stream_old_samples_back));
        SDL_memset(fir_stream_samples_back, 0, sizeof(fir_stream_samples_back));
#endif

        SDL_memset(echo_ram, 0, sizeof(echo_ram));
        SDL_memset(echo_hist, 0, sizeof(echo_hist));

#ifdef WAVE_DEEP_DEBUG
        debugIn = ctx_wave_open(channels, rate, sizeof(int16_t), WAVE_FORMAT_PCM, 1, 0, WAVE_PATH "in.wav");
        debugInRes = ctx_wave_open(channels, SDSP_RATE, sizeof(int16_t), WAVE_FORMAT_PCM, 1, 0, WAVE_PATH "in-res.wav");
        debugOutRes = ctx_wave_open(channels, SDSP_RATE, sizeof(int16_t), WAVE_FORMAT_PCM, 1, 0, WAVE_PATH "out-res.wav");
        debugOut = ctx_wave_open(channels, rate, sizeof(int16_t), WAVE_FORMAT_PCM, 1, 0, WAVE_PATH "out.wav");
        ctx_wave_enable_stereo(debugIn);
        ctx_wave_enable_stereo(debugInRes);
        ctx_wave_enable_stereo(debugOutRes);
        ctx_wave_enable_stereo(debugOut);
#endif

        switch(format)
        {
        case AUDIO_U8:
            readSample = getUInt8;
            writeSample = setUInt8;
            sample_size = sizeof(Uint8);
            break;
        case AUDIO_S8:
            readSample = getInt8;
            writeSample = setInt8;
            sample_size = sizeof(Sint8);
            break;
        case AUDIO_S16LSB:
            readSample = getInt16LSB;
            writeSample = setInt16LSB;
            sample_size = sizeof(Sint16);
            break;
        case AUDIO_S16MSB:
            readSample = getInt16MSB;
            writeSample = setInt16MSB;
            sample_size = sizeof(Sint16);
            break;
        case AUDIO_U16LSB:
            readSample = getUInt16LSB;
            writeSample = setUInt16LSB;
            sample_size = sizeof(Uint16);
            break;
        case AUDIO_U16MSB:
            readSample = getUInt16MSB;
            writeSample = setUInt16MSB;
            sample_size = sizeof(Uint16);
            break;
        case AUDIO_S32LSB:
            readSample = getInt32LSB;
            writeSample = setInt32LSB;
            sample_size = sizeof(Sint32);
            break;
        case AUDIO_S32MSB:
            readSample = getInt32MSB;
            writeSample = setInt32MSB;
            sample_size = sizeof(Sint32);
            break;
        case AUDIO_F32LSB:
            readSample = getFloatLSBSample;
            writeSample = setFloatLSBSample;
            sample_size = sizeof(float);
            break;
        case AUDIO_F32MSB:
            readSample = getFloatMSBSample;
            writeSample = setFloatMSBSample;
            sample_size = sizeof(float);
            break;
        default:
            return -1; /* Unsupported format */
        }

        setDefaultRegs();

        is_valid = SDL_TRUE;
        return 0;
    }

    void close()
    {
#ifdef WAVE_DEEP_DEBUG
        ctx_wave_close(debugIn);
        ctx_wave_close(debugInRes);
        ctx_wave_close(debugOutRes);
        ctx_wave_close(debugOut);
#endif
    }

#ifdef RESAMPLED_FIR
    void sub_process_echo(int *out, int *echo_out)
    {
        int echo_in[MAX_CHANNELS];
        int *echo_ptr;
        int c, f, e_offset, v;
        int (*echohist_pos)[MAX_CHANNELS];

        e_offset = echo_offset;
        SDL_assert(echo_offset < ECHO_BUFFER_SIZE);
        echo_ptr = echo_ram + echo_offset;

        if(!echo_offset)
            echo_length = ((reg_edl & 0x0F) * 0x400 * channels) / 2;
        e_offset += channels;
        if(e_offset >= echo_length)
            e_offset = 0;
        echo_offset = e_offset;

        /* FIR */
        for(c = 0; c < channels; c++)
            echo_in[c] = echo_ptr[c];

        echohist_pos = echo_hist_pos;
        if(++echohist_pos >= &echo_hist[ECHO_HIST_SIZE])
            echohist_pos = echo_hist;
        echo_hist_pos = echohist_pos;

        /* --------------- FIR filter-------------- */
        for(c = 0; c < channels; c++)
            echohist_pos[0][c] = echohist_pos[8][c] = echo_in[c];

        for(c = 0; c < channels; ++c)
            echo_in[c] *= reg_fir[7];

        for(f = 0; f <= 6; ++f)
        {
            for(c = 0; c < channels; ++c)
                echo_in[c] += echohist_pos[f + 1][c] * reg_fir[f];
        }
        /* ---------------------------------------- */

        /* Echo out */
        if(!(reg_flg & 0x20))
        {
            for(c = 0; c < channels; c++)
            {
                v = (echo_out[c] >> 7) + ((echo_in[c] * reg_efb) >> 14);
                CLAMP16(v);
                echo_ptr[c] = v;
            }
        }

        // Do out
        for(c = 0; c < channels; ++c)
            out[c] = echo_in[c];
    }

    void pre_process_echo(int *echo_out)
    {
        int c, q = 0;

        while(fir_stream_samplecnt >= fir_stream_rateratio)
        {
            if(q > 0)
                return;
            for(c = 0; c < channels; c++)
            {
                fir_stream_old_samples[c] = fir_stream_samples[c];
                fir_stream_samples[c] = echo_out[c];
            }
            fir_stream_samplecnt -= fir_stream_rateratio;
            q++;
        }

        fir_buffer_size = 0;
        while(fir_stream_samplecnt < fir_stream_rateratio)
        {
            for(c = 0; c < channels; c++)
            {
                fir_stream_midbuffer_in[fir_buffer_size][c] = (((double)fir_stream_old_samples[c] * (fir_stream_rateratio - fir_stream_samplecnt)
                                                              + (double)fir_stream_samples[c] * fir_stream_samplecnt) / fir_stream_rateratio);
            }

            sub_process_echo(fir_stream_midbuffer_out[fir_buffer_size], fir_stream_midbuffer_in[fir_buffer_size]);
#ifdef WAVE_DEEP_DEBUG
            {
                Sint16 outw[MAX_CHANNELS];
                for(c = 0; c < channels; ++c)
                    outw[c] = fir_stream_midbuffer_in[fir_buffer_size][c] / 128;
                ctx_wave_write(debugInRes, (const Uint8*)outw, channels * sizeof(Sint16));
            }
            {
                Sint16 outw[MAX_CHANNELS];
                for(c = 0; c < channels; ++c)
                    outw[c] = fir_stream_midbuffer_out[fir_buffer_size][c] / 128;
                ctx_wave_write(debugOutRes, (const Uint8*)outw, channels * sizeof(Sint16));
            }
#endif

            fir_stream_samplecnt += 1.0;
            fir_buffer_size++;
        }
    }

    void process_echo(int *out, int *echo_out)
    {
        int c, f;

#ifdef WAVE_DEEP_DEBUG
        {
            Sint16 outw[MAX_CHANNELS];
            for(c = 0; c < channels; ++c)
                outw[c] = echo_out[c] / 128;
            ctx_wave_write(debugIn, (const Uint8*)outw, channels * sizeof(Sint16));
        }
#endif

        // Process directly if no resampling needed
        if(rate == SDSP_RATE)
        {
            sub_process_echo(out, echo_out);

#ifdef WAVE_DEEP_DEBUG
            {
                Sint16 outw[MAX_CHANNELS];
                for(c = 0; c < channels; ++c)
                    outw[c] = out[c] / 128;
                ctx_wave_write(debugOut, (const Uint8*)outw, channels * sizeof(Sint16));
            }
#endif
            return;
        }

        pre_process_echo(echo_out);

        f = 0;
        while(fir_stream_samplecnt_back >= fir_stream_rateratio_back && fir_buffer_size > 0)
        {
            for(c = 0; c < channels; c++)
            {
                fir_stream_old_samples_back[c] = fir_stream_samples_back[c];
                fir_stream_samples_back[c] = fir_stream_midbuffer_out[f][c];
            }
            fir_stream_samplecnt_back -= fir_stream_rateratio_back;
            f++;
        }

        for(c = 0; c < channels; c++)
        {
            out[c] = (((double)fir_stream_old_samples_back[c] * (fir_stream_rateratio_back - fir_stream_samplecnt_back)
                     + (double)fir_stream_samples_back[c] * fir_stream_samplecnt_back) / fir_stream_rateratio_back);
        }

#ifdef WAVE_DEEP_DEBUG
        {
            Sint16 outw[MAX_CHANNELS];
            for(c = 0; c < channels; ++c)
                outw[c] = out[c] / 128;
            ctx_wave_write(debugOut, (const Uint8*)outw, channels * sizeof(Sint16));
        }
#endif

        if(fir_buffer_size > 0)
            fir_stream_samplecnt_back += 1.0;
    }
#endif

    void process(Uint8 *stream, int len)
    {
        int frames = len / (sample_size * channels);

        int main_out[MAX_CHANNELS];
        int echo_out[MAX_CHANNELS];
        int echo_in[MAX_CHANNELS];

        int c, ov;
#ifndef RESAMPLED_FIR
        int f, e_offset, v;
#endif

        int mvoll[2] = {reg_mvoll, reg_mvolr};
        int evoll[2] = {reg_evoll, reg_evolr};

#ifndef RESAMPLED_FIR
        int (*echohist_pos)[MAX_CHANNELS];
        int *echo_ptr;
#endif

        SDL_memset(main_out, 0, sizeof(main_out));
        SDL_memset(echo_out, 0, sizeof(echo_out));
        SDL_memset(echo_in, 0, sizeof(echo_in));

        if(!is_valid)
            return;

        do
        {
            for(c = 0; c < channels; ++c)
                main_out[c] = readSample(stream, c) * 128;

            if(reg_eon & 1)
            {
                for(c = 0; c < channels; c++)
                    echo_out[c] = main_out[c];
            }

#ifdef RESAMPLED_FIR
            process_echo(echo_in, echo_out);
#else
            e_offset = echo_offset;
            SDL_assert(echo_offset < ECHO_BUFFER_SIZE);
            echo_ptr = echo_ram + echo_offset;

            if(!echo_offset)
                echo_length = (int)round((((reg_edl & 0x0F) * 0x400 * channels) / 2) * rate_factor);
            e_offset += channels;
            if(e_offset >= echo_length)
                e_offset = 0;
            echo_offset = e_offset;

            /* FIR */
            for(c = 0; c < channels; c++)
                echo_in[c] = echo_ptr[c];

            echohist_pos = echo_hist_pos;
            if(++echohist_pos >= &echo_hist[ECHO_HIST_SIZE])
                echohist_pos = echo_hist;
            echo_hist_pos = echohist_pos;

            /* --------------- FIR filter-------------- */
            for(c = 0; c < channels; c++)
                echohist_pos[0][c] = echohist_pos[8][c] = echo_in[c];

            for(c = 0; c < channels; ++c)
                echo_in[c] *= reg_fir[7];

            for(f = 0; f <= 6; ++f)
            {
                for(c = 0; c < channels; ++c)
                    echo_in[c] += echo_hist_pos[f + 1][c] * reg_fir[f];
            }
            /* ---------------------------------------- */

            /* Echo out */
            if(!(reg_flg & 0x20))
            {
                for(c = 0; c < channels; c++)
                {
                    v = (echo_out[c] >> 7) + ((echo_in[c] * reg_efb) >> 14);
                    CLAMP16(v);
                    echo_ptr[c] = v;
                }
            }
#endif
            /* Sound out */
            for(c = 0; c < channels; c++)
            {
                ov = (main_out[c] * mvoll[c % 2] + echo_in[c] * evoll[c % 2]) >> 14;
                CLAMP16(ov);
                if((reg_flg & 0x40))
                    ov = 0;
                writeSample(&stream, ov);
            }
        }
        while(--frames);
    }
};

static SpcEcho *s_spc_echo = nullptr;

void echoEffectInit(int rate, Uint16 format, int channels)
{
    echoEffectFree();
    if(!s_spc_echo)
    {
        s_spc_echo = new SpcEcho();
        s_spc_echo->init(rate, format, channels);
    }
}

void echoEffectFree()
{
    if(s_spc_echo)
    {
        s_spc_echo->close();
        delete s_spc_echo;
    }
    s_spc_echo = nullptr;
}

void spcEchoEffect(int, void *stream, int len, void *)
{
    if(!s_spc_echo)
        return; // Effect doesn't working
    s_spc_echo->process((Uint8*)stream, len);
}

void spcEchoEffectDone(int, void *)
{
    echoEffectFree();
}

void echoEffectSetReg(EchoSetup key, int val)
{
    if(!s_spc_echo)
        return;

    SDL_LockAudio();

    switch(key)
    {
    case ECHO_EON:
        s_spc_echo->reg_eon = (Uint8)val;
        break;
    case ECHO_EDL:
        s_spc_echo->reg_edl = (Uint8)val;
        break;
    case ECHO_EFB:
        s_spc_echo->reg_efb = (Sint8)val;
        break;
    case ECHO_MVOLL:
        s_spc_echo->reg_mvoll = (Sint8)val;
        break;
    case ECHO_MVOLR:
        s_spc_echo->reg_mvolr = (Sint8)val;
        break;
    case ECHO_EVOLL:
        s_spc_echo->reg_evoll = (Sint8)val;
        break;
    case ECHO_EVOLR:
        s_spc_echo->reg_evolr = (Sint8)val;
        break;

    case ECHO_FIR0:
        s_spc_echo->reg_fir[0]= (Sint8)val;
        break;
    case ECHO_FIR1:
        s_spc_echo->reg_fir[1]= (Sint8)val;
        break;
    case ECHO_FIR2:
        s_spc_echo->reg_fir[2]= (Sint8)val;
        break;
    case ECHO_FIR3:
        s_spc_echo->reg_fir[3]= (Sint8)val;
        break;
    case ECHO_FIR4:
        s_spc_echo->reg_fir[4]= (Sint8)val;
        break;
    case ECHO_FIR5:
        s_spc_echo->reg_fir[5]= (Sint8)val;
        break;
    case ECHO_FIR6:
        s_spc_echo->reg_fir[6]= (Sint8)val;
        break;
    case ECHO_FIR7:
        s_spc_echo->reg_fir[7]= (Sint8)val;
        break;
    }

    SDL_UnlockAudio();
}

int echoEffectGetReg(EchoSetup key)
{
    if(!s_spc_echo)
        return 0;

    switch(key)
    {
    case ECHO_EON:
        return s_spc_echo->reg_eon;
    case ECHO_EDL:
        return s_spc_echo->reg_edl;
    case ECHO_EFB:
        return s_spc_echo->reg_efb;
    case ECHO_MVOLL:
        return s_spc_echo->reg_mvoll;
    case ECHO_MVOLR:
        return s_spc_echo->reg_mvolr;
    case ECHO_EVOLL:
        return s_spc_echo->reg_evoll;
    case ECHO_EVOLR:
        return s_spc_echo->reg_evolr;

    case ECHO_FIR0:
        return s_spc_echo->reg_fir[0];
    case ECHO_FIR1:
        return s_spc_echo->reg_fir[1];
    case ECHO_FIR2:
        return s_spc_echo->reg_fir[2];
    case ECHO_FIR3:
        return s_spc_echo->reg_fir[3];
    case ECHO_FIR4:
        return s_spc_echo->reg_fir[4];
    case ECHO_FIR5:
        return s_spc_echo->reg_fir[5];
    case ECHO_FIR6:
        return s_spc_echo->reg_fir[6];
    case ECHO_FIR7:
        return s_spc_echo->reg_fir[7];
    }

    return 0;
}

void echoEffectResetFir()
{
    if(!s_spc_echo)
        return;

    SDL_LockAudio();
    s_spc_echo->setDefaultFir();
    SDL_UnlockAudio();
}

void echoEffectResetDefaults()
{
    if(!s_spc_echo)
        return;

    SDL_LockAudio();
    s_spc_echo->setDefaultRegs();
    SDL_UnlockAudio();
}
