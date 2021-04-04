#include <vector>
#include <deque>
#include <cmath>
#include <cstdio>
#ifdef USE_SDL_MIXER_X
#   include "SDL_mixer_ext.h"
#else
#   include "SDL_mixer.h"
#endif

#include "spc_echo.h"

inline void set_le16(void *p, unsigned n)
{
    ((unsigned char *) p) [1] = (unsigned char)(n >> 8);
    ((unsigned char *) p) [0] = (unsigned char) n;
}

inline unsigned get_le16(void const *p)
{
    return (unsigned)((unsigned char const *) p) [1] << 8 |
           (unsigned)((unsigned char const *) p) [0];
}

#ifndef GET_LE16
#define GET_LE16( addr )        get_le16( addr )
#define SET_LE16( addr, data )  set_le16( addr, data )
#endif

#define GET_LE16SA( addr )      ((int16_t) GET_LE16( addr ))
#define GET_LE16A( addr )       GET_LE16( addr )
#define SET_LE16A( addr, data ) SET_LE16( addr, data )

#define REG(n)          regs[r_##n]


#define CLAMP16( io )\
    {\
        if ( (int16_t) io != io )\
            io = (io >> 31) ^ 0x7FFF;\
    }
enum { echo_hist_size = 8 };
enum { register_count = 128 };
enum { native_sample_rate = 32000 };

enum { max_channels = 10 };

// Global registers
enum
{
    r_mvoll = 0x0C, r_mvolr = 0x1C,
    r_evoll = 0x2C, r_evolr = 0x3C,
    r_kon   = 0x4C, r_koff  = 0x5C,
    r_flg   = 0x6C, r_endx  = 0x7C,
    r_efb   = 0x0D, r_pmon  = 0x2D,
    r_non   = 0x3D, r_eon   = 0x4D,
    r_dir   = 0x5D, r_esa   = 0x6D,
    r_edl   = 0x7D,
    r_fir   = 0x0F // 8 coefficients at 0x0F, 0x1F ... 0x7F
};

static uint8_t const initial_regs[register_count] =
{
//   0      1     2     3    4     5     6     7     8      9     A    B     C      D     E     F
//   00
    0x45, 0x8B, 0x5A, 0x9A, 0xE4, 0x82, 0x1B, 0x78, 0x00, 0x00, 0xAA, 0x96, 0x89, 0x0E, 0xE0, 0x80,
//   10
    0x2A, 0x49, 0x3D, 0xBA, 0x14, 0xA0, 0xAC, 0xC5, 0x00, 0x00, 0x51, 0xBB, 0x9C, 0x4E, 0x7B, 0xFF,
//   20
    0xF4, 0xFD, 0x57, 0x32, 0x37, 0xD9, 0x42, 0x22, 0x00, 0x00, 0x5B, 0x3C, 0x9F, 0x1B, 0x87, 0x9A,
//   30
    0x6F, 0x27, 0xAF, 0x7B, 0xE5, 0x68, 0x0A, 0xD9, 0x00, 0x00, 0x9A, 0xC5, 0x9C, 0x4E, 0x7B, 0xFF,
//   40
    0xEA, 0x21, 0x78, 0x4F, 0xDD, 0xED, 0x24, 0x14, 0x00, 0x00, 0x77, 0xB1, 0xD1, 0x36, 0xC1, 0x67,
//   50
    0x52, 0x57, 0x46, 0x3D, 0x59, 0xF4, 0x87, 0xA4, 0x00, 0x00, 0x7E, 0x44, 0x9C, 0x4E, 0x7B, 0xFF,
//   60
    0x75, 0xF5, 0x06, 0x97, 0x10, 0xC3, 0x24, 0xBB, 0x00, 0x00, 0x7B, 0x7A, 0xE0, 0x60, 0x12, 0x0F,
//   70
    0xF7, 0x74, 0x1C, 0xE5, 0x39, 0x3D, 0x73, 0xC1, 0x00, 0x00, 0x7A, 0xB3, 0xFF, 0x4E, 0x7B, 0xFF
};

struct SpcEcho
{
    uint8_t regs[register_count];
    uint8_t echo_ram[64 * 1024 * max_channels];

    int echo_hist[echo_hist_size * max_channels][max_channels];
    int (*echo_hist_pos)[max_channels] = echo_hist; // &echo_hist [0 to 7]

    int every_other_sample; // toggles every sample
    int kon = 0;                // KON value when last checked
    int noise = 0;
    int echo_offset = 0;        // offset from ESA in echo buffer
    int echo_length = 0;        // number of bytes that echo_offset will stop at
    int phase = 0;              // next clock cycle to run (0-31)

    double  rate_factor = 1.0;
    double  channels_factor = 1.0;
    double  common_factor = 1.0;
    int     rate = native_sample_rate;
    int     channels = 2;
    int     sample_size = 2;
    Uint16  format = AUDIO_F32SYS;

    void setDefaultRegs()
    {
        REG(flg) = 0;//0x20;
        REG(mvoll) = 0x89;
        REG(mvolr) = 0x9C;
        REG(evoll) = 0x5F;//0x9F;
        REG(evolr) = 0x5C;//0x9C;

        // Defaults 80 FF 9A FF 67 FF 0F FF

        //$xf rw FFCx - Echo FIR Filter Coefficient (FFC) X
        REG(fir + 0x00) = 0x80; // 0x80
        REG(fir + 0x10) = 0xff; // 0xff
        REG(fir + 0x20) = 0x9a; // 0x9a
        REG(fir + 0x30) = 0xff; // 0xff
        REG(fir + 0x40) = 0x67; // 0x67
        REG(fir + 0x50) = 0xff; // 0xff
        REG(fir + 0x60) = 0x0f; // 0x0f
        REG(fir + 0x70) = 0xff; // 0xff

        // $0d rw EFB - Echo feedback volume
        REG(efb) = 0x0E;
        // $4d rw EON - Echo enable
        REG(eon) = 1;
        // $7d rw EDL - Echo delay (ring buffer size)
        REG(edl) = 3;
        // $6d rw ESA - Echo ring buffer address
        REG(esa) = 0;
        // $5d rw DIR - Sample table address
        //REG(dir) = 1;

        std::printf("mvoll=%02X mvolr=%02X evoll=%02X evolr=%02X\n", REG(mvoll), REG(mvolr), REG(evoll), REG(evoll));
        std::printf("fir=%02X %02X %02X %02X %02X %02X %02X %02X\n",
                    REG(fir + 0x00),
                    REG(fir + 0x10),
                    REG(fir + 0x20),
                    REG(fir + 0x30),
                    REG(fir + 0x40),
                    REG(fir + 0x50),
                    REG(fir + 0x60),
                    REG(fir + 0x70));
        std::fflush(stdout);
    }

    int (*readSample)(Uint8 *raw, int channel);
    void (*writeSample)(Uint8 **raw, int sample);

    static int getFloatLSBSample(Uint8 *raw, int c)
    {
        float *out = reinterpret_cast<float *>(raw);
        return int(out[c] * 32768.f) * 128;
    }
    static void setFloatLSBSample(Uint8 **raw, int ov)
    {
        float *out = reinterpret_cast<float *>(*raw);
        *out = (float(ov) / 32768.f);
        (*raw) += sizeof(float);
    }

    static int getInt16LSB(Uint8 *raw, int c)
    {
        Sint16 *out = reinterpret_cast<Sint16 *>(raw);
        return out[c] * 128;
    }
    static void setInt16LSB(Uint8 **raw, int ov)
    {
        Sint16 *out = reinterpret_cast<Sint16 *>(*raw);
        *out = Sint16(ov);
        (*raw) += sizeof(Sint16);
    }

    void init(int i_rate, Uint16 i_format, int i_channels)
    {
        rate = i_rate;
        format = i_format;
        channels = i_channels;
        rate_factor = double(i_rate) / native_sample_rate;
        channels_factor = double(i_channels) / 2.0;
        common_factor = rate_factor * channels_factor;

        SDL_memset(echo_ram, 0, sizeof(echo_ram));
        SDL_memset(echo_hist, 0, sizeof(echo_hist));
        SDL_memcpy(regs, initial_regs, sizeof(initial_regs));

        switch(format)
        {
        case AUDIO_S16LSB:
        case AUDIO_S16MSB:
            readSample = getInt16LSB;
            writeSample = setInt16LSB;
            sample_size = sizeof(Sint16);
            break;
        case AUDIO_F32LSB:
        case AUDIO_F32MSB:
            readSample = getFloatLSBSample;
            writeSample = setFloatLSBSample;
            sample_size = sizeof(float);
            break;
        }

        setDefaultRegs();
    }

    void process(Uint8 *stream, int len)
    {
        int frames = len / (sample_size * channels);

        int main_out[max_channels] = {0};
        int echo_out[max_channels] = {0};
        int c;

        int mvoll[2] = {(int8_t)REG(mvoll), (int8_t)REG(mvolr)};
        int evoll[2] = {(int8_t)REG(evoll), (int8_t)REG(evolr)};

        do
        {
            for(c = 0; c < channels; ++c)
                main_out[c] = readSample(stream, c);

            if(REG(eon) & 1)
            {
                for(c = 0; c < channels; c++)
                    echo_out[c] = main_out[c];
            }

            int e_offset = echo_offset;
            uint8_t *const echo_ptr = &echo_ram[(REG(esa) * 0x100 + echo_offset) & 0xFFFF];

            if(!echo_offset)
                echo_length = SDL_round(((REG(edl) & 0x0F) * (0x800)) * common_factor);
            e_offset += (2 * channels);
            if(e_offset >= echo_length)
                e_offset = 0;
            echo_offset = e_offset;

            // FIR
            int echo_in[max_channels];
            for(c = 0; c < channels; c++)
                echo_in[c] = GET_LE16SA(echo_ptr + (2 * c));

            int (*echohist_pos)[max_channels] = echo_hist_pos;
            if(++echohist_pos >= &echo_hist[echo_hist_size])
                echohist_pos = echo_hist;
            echo_hist_pos = echohist_pos;

            for(c = 0; c < channels; c++)
                echohist_pos[0][c] = echohist_pos[8][c] = echo_in[c];

#define CALC_FIR_(i, in)  ((in) * (int8_t) REG(fir + i * 0x10))

            for(c = 0; c < channels; ++c)
                echo_in[c] = CALC_FIR_(7, echo_in[c]);

#define CALC_FIR(i, ch)   CALC_FIR_( i, echo_hist_pos [i + 1][ch] )
#define DO_FIR( i )\
            for(c = 0; c < channels; ++c) \
                echo_in[c] += CALC_FIR(i, c);

            DO_FIR(0);
            DO_FIR(1);
            DO_FIR(2);
#if defined (__MWERKS__) && __MWERKS__ < 0x3200
            __eieio(); // keeps compiler from stupidly "caching" things in memory
#endif
            DO_FIR(3);
            DO_FIR(4);
            DO_FIR(5);
            DO_FIR(6);

            // Echo out
            if(!(REG(flg) & 0x20))
            {
                int v;
                for(c = 0; c < channels; c++)
                {
                    v = (echo_out[c] >> 7) + ((echo_in[c] * (int8_t) REG(efb)) >> 14);
                    CLAMP16(v);
                    SET_LE16A(echo_ptr + (2 * c), v);
                }
            }

            // Sound out
            int ov;
            for(c = 0; c < channels; c++)
            {
                ov = (main_out[c] * mvoll[c % 2] + echo_in[c] * evoll[c % 2]) >> 14;
                CLAMP16(ov);
                if((REG(flg) & 0x40))
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

void echoEffectSetReg(EchoSetup key, Uint8 val)
{
    if(!s_spc_echo)
        return;

    SDL_LockAudio();

    switch(key)
    {
    case ECHO_EON:
        s_spc_echo->REG(eon) = val;
        break;
    case ECHO_EDL:
        s_spc_echo->REG(edl) = val;
        break;
    case ECHO_EFB:
        s_spc_echo->REG(efb) = val;
        break;
    case ECHO_MVOLL:
        s_spc_echo->REG(mvoll) = val;
        break;
    case ECHO_MVOLR:
        s_spc_echo->REG(mvolr) = val;
        break;
    case ECHO_EVOLL:
        s_spc_echo->REG(evoll) = val;
        break;
    case ECHO_EVOLR:
        s_spc_echo->REG(evolr) = val;
        break;

    case ECHO_FIR0:
        s_spc_echo->REG(fir + 0x00) = val;
        break;
    case ECHO_FIR1:
        s_spc_echo->REG(fir + 0x10) = val;
        break;
    case ECHO_FIR2:
        s_spc_echo->REG(fir + 0x20) = val;
        break;
    case ECHO_FIR3:
        s_spc_echo->REG(fir + 0x30) = val;
        break;
    case ECHO_FIR4:
        s_spc_echo->REG(fir + 0x40) = val;
        break;
    case ECHO_FIR5:
        s_spc_echo->REG(fir + 0x50) = val;
        break;
    case ECHO_FIR6:
        s_spc_echo->REG(fir + 0x60) = val;
        break;
    case ECHO_FIR7:
        s_spc_echo->REG(fir + 0x70) = val;
        break;
    }

    SDL_UnlockAudio();
}

Uint8 echoEffectGetReg(EchoSetup key)
{
    if(!s_spc_echo)
        return 0;

    switch(key)
    {
    case ECHO_EON:
        return s_spc_echo->REG(eon);
    case ECHO_EDL:
        return s_spc_echo->REG(edl);
    case ECHO_EFB:
        return s_spc_echo->REG(efb);
    case ECHO_MVOLL:
        return s_spc_echo->REG(mvoll);
    case ECHO_MVOLR:
        return s_spc_echo->REG(mvolr);
    case ECHO_EVOLL:
        return s_spc_echo->REG(evoll);
    case ECHO_EVOLR:
        return s_spc_echo->REG(evolr);

    case ECHO_FIR0:
        return s_spc_echo->REG(fir + 0x00);
    case ECHO_FIR1:
        return s_spc_echo->REG(fir + 0x10);
    case ECHO_FIR2:
        return s_spc_echo->REG(fir + 0x20);
    case ECHO_FIR3:
        return s_spc_echo->REG(fir + 0x30);
    case ECHO_FIR4:
        return s_spc_echo->REG(fir + 0x40);
    case ECHO_FIR5:
        return s_spc_echo->REG(fir + 0x50);
    case ECHO_FIR6:
        return s_spc_echo->REG(fir + 0x60);
    case ECHO_FIR7:
        return s_spc_echo->REG(fir + 0x70);
    }
}

void echoEffectResetDefaults()
{
    if(!s_spc_echo)
        return;

    SDL_LockAudio();
    s_spc_echo->setDefaultRegs();
    SDL_UnlockAudio();
}
