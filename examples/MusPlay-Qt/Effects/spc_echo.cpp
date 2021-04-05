#include <tgmath.h>
#include "SDL_audio.h"
#ifdef USE_SDL_MIXER_X
#   include "SDL_mixer_ext.h"
#else
#   include "SDL_mixer.h"
#endif

#include "spc_echo.h"

#ifndef INT8_MIN
#define INT8_MIN    (-0x7f - 1)
#endif
#ifndef INT16_MAX
#define INT16_MAX   0x7fff
#endif

static SDL_INLINE void set_le16(Uint8 *p, Sint16 f)
{
    Uint16 n = (Uint16)f;
    p[1] = (Uint8)((n >> 8) & 0xFF);
    p[0] = (Uint8)((n >> 0) & 0xFF);
}

static SDL_INLINE Sint16 get_le16(Uint8 const *p)
{
    return (Sint16)(
               ((Uint16(p[1]) << 8) & 0xFF00) |
               ((Uint16(p[0]) << 0) & 0x00FF)
           );
}

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

//#define RESAMPLED_FIR

struct SpcEcho
{
    SDL_bool is_valid = SDL_FALSE;
    Uint8 echo_ram[64 * 1024 * MAX_CHANNELS];

    // Echo history keeps most recent 8 samples (twice the size to simplify wrap handling)
    int echo_hist[ECHO_HIST_SIZE * 2 * MAX_CHANNELS][MAX_CHANNELS];
    int (*echo_hist_pos)[MAX_CHANNELS] = echo_hist; // &echo_hist [0 to 7]

#ifdef RESAMPLED_FIR
    int echo_cvt_buffer[MAX_CHANNELS * 1024];
    float echo_cvt_buffer_buf[MAX_CHANNELS * 1024];
#endif

    //! offset from ESA in echo buffer
    int echo_offset = 0;
    //! number of bytes that echo_offset will stop at
    int echo_length = 0;

    double  rate_factor = 1.0;
    double  channels_factor = 1.0;
    double  common_factor = 1.0;
    int     rate = SDSP_RATE;
    int     channels = 2;
    int     sample_size = 2;
    Uint16  format = AUDIO_F32SYS;

#ifdef RESAMPLED_FIR
    SDL_AudioCVT fir_cvt_i;
    SDL_AudioCVT fir_cvt_o;
    int fir_cvt_src_size = 0;
#endif

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
        common_factor = rate_factor * channels_factor;
#ifdef RESAMPLED_FIR
        fir_cvt_src_size = ceil(9.0 * rate_factor);
#endif

        if(i_rate < 4000)
            return -1; /* Too small sample rate */

        SDL_memset(echo_ram, 0, sizeof(echo_ram));
        SDL_memset(echo_hist, 0, sizeof(echo_hist));
#ifdef RESAMPLED_FIR
        SDL_memset(echo_cvt_buffer, 0, sizeof(echo_cvt_buffer));
        SDL_BuildAudioCVT(&fir_cvt_i,
                          AUDIO_F32, channels, rate,
                          AUDIO_F32, channels, SDSP_RATE);
        SDL_BuildAudioCVT(&fir_cvt_o,
                          AUDIO_F32, channels, SDSP_RATE,
                          AUDIO_F32, channels, rate);
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

    void process(Uint8 *stream, int len)
    {
        int frames = len / (sample_size * channels);

        int main_out[MAX_CHANNELS] = {0};
        int echo_out[MAX_CHANNELS] = {0};
        int echo_in[MAX_CHANNELS];
        int c, f, e_offset, ov, v;

        int mvoll[2] = {reg_mvoll, reg_mvolr};
        int evoll[2] = {reg_evoll, reg_evolr};

        int (*echohist_pos)[MAX_CHANNELS];
        uint8_t *echo_ptr;

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

            e_offset = echo_offset;
            /* echo_ptr = &echo_ram[(reg_esa * 0x100 + echo_offset) & 0xFFFF]; */
            echo_ptr = &echo_ram[echo_offset & 0xFFFF];

            if(!echo_offset)
                echo_length = (int)round(((reg_edl & 0x0F) * (0x800)) * common_factor);
            e_offset += (2 * channels);
            if(e_offset >= echo_length)
                e_offset = 0;
            echo_offset = e_offset;

            /* FIR */
            for(c = 0; c < channels; c++)
                echo_in[c] = get_le16(echo_ptr + (2 * c));

            echohist_pos = echo_hist_pos;
            if(++echohist_pos >= &echo_hist[ECHO_HIST_SIZE])
                echohist_pos = echo_hist;
            echo_hist_pos = echohist_pos;

            /* --------------- FIR filter-------------- */
#ifdef RESAMPLED_FIR
            for(f = 0; f <= fir_cvt_src_size; f++)
            {
                for(c = 0; c < channels; c++)
                    echo_cvt_buffer[(f * channels) + c] = echohist_pos[f][c];
            }

            if(fir_cvt_i.needed)
            {
                for(f = 0; f < fir_cvt_src_size * channels; f++)
                    echo_cvt_buffer_buf[f] = (float)echo_cvt_buffer[f] / 32768.f;
                fir_cvt_i.buf = (Uint8*)echo_cvt_buffer_buf;
                fir_cvt_i.len = fir_cvt_src_size * sizeof(int) * channels;
                SDL_ConvertAudio(&fir_cvt_i);
                for(f = 0; f < 10 * channels; f++)
                    echo_cvt_buffer[f] = (int)(echo_cvt_buffer_buf[f] * 32768.f);
            }

            for(c = 0; c < channels; c++)
            {
                echo_cvt_buffer[c] = echo_cvt_buffer[(8 * channels) + c] = echo_in[c];
                echo_in[c] *= reg_fir[7];
            }

            for(f = 0; f <= 6; ++f)
            {
                for(c = 0; c < channels; ++c)
                    echo_in[c] += echo_cvt_buffer[((f + 1) * channels) + c] * reg_fir[f];
            }

            if(fir_cvt_o.needed)
            {
                for(f = 0; f < 10 * channels; f++)
                    echo_cvt_buffer_buf[f] = (float)echo_cvt_buffer[f] / 32768.f;
                fir_cvt_o.buf = (Uint8*)echo_cvt_buffer_buf;
                fir_cvt_o.len = 9 * sizeof(int) * channels;
                SDL_ConvertAudio(&fir_cvt_o);
                for(f = 0; f < fir_cvt_src_size * channels; f++)
                    echo_cvt_buffer[f] = (int)(echo_cvt_buffer_buf[f] * 32768.f);
            }

            for(f = 0; f <= fir_cvt_src_size; f++)
            {
                for(c = 0; c < channels; c++)
                    echohist_pos[f][c] = echo_cvt_buffer[(f * channels) + c];
            }
#else
            for(c = 0; c < channels; c++)
                echohist_pos[0][c] = echohist_pos[8][c] = echo_in[c];

            for(c = 0; c < channels; ++c)
                echo_in[c] = echo_in[c] * reg_fir[7];

            for(f = 0; f <= 6; ++f)
            {
                for(c = 0; c < channels; ++c)
                    echo_in[c] += echo_hist_pos[f + 1][c] * reg_fir[f];
            }
#endif
            /* ---------------------------------------- */

            /* Echo out */
            if(!(reg_flg & 0x20))
            {
                for(c = 0; c < channels; c++)
                {
                    v = (echo_out[c] >> 7) + ((echo_in[c] * reg_efb) >> 14);
                    CLAMP16(v);
                    set_le16(echo_ptr + (2 * c), v);
                }
            }

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
        delete s_spc_echo;
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
