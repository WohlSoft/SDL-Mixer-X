#ifndef REVERB_H
#define REVERB_H

#include <stdint.h>

#ifndef AUDIO_U8 /* Copied from SDL2 heads */
#define AUDIO_U8        0x0008  /**< Unsigned 8-bit samples */
#define AUDIO_S8        0x8008  /**< Signed 8-bit samples */
#define AUDIO_U16LSB    0x0010  /**< Unsigned 16-bit samples */
#define AUDIO_S16LSB    0x8010  /**< Signed 16-bit samples */
#define AUDIO_U16MSB    0x1010  /**< As above, but big-endian byte order */
#define AUDIO_S16MSB    0x9010  /**< As above, but big-endian byte order */
#define AUDIO_U16       AUDIO_U16LSB
#define AUDIO_S16       AUDIO_S16LSB
#define AUDIO_S32LSB    0x8020  /**< 32-bit integer samples */
#define AUDIO_S32MSB    0x9020  /**< As above, but big-endian byte order */
#define AUDIO_S32       AUDIO_S32LSB
#define AUDIO_F32LSB    0x8120  /**< 32-bit floating point samples */
#define AUDIO_F32MSB    0x9120  /**< As above, but big-endian byte order */
#define AUDIO_F32       AUDIO_F32LSB
#endif

struct FxReverb;

struct ReverbSetup
{
    double gain         = 6.0;
    double roomScale    = 0.7;
    double balance      = 0.6;
    double hfDamping    = 0.8;
    double preDelayS    = 0.0;
    double stereoDepth  = 1.0;
};

extern FxReverb *reverbEffectInit(int rate, uint16_t format, int channels);
extern void reverbEffectFree(FxReverb *context);

extern void reverbEffect(int chan, void *stream, int len, void *context);

extern void reverbUpdateSetup(FxReverb *context, const ReverbSetup &setup);

#endif // REVERB_H
