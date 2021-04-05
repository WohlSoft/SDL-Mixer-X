#ifndef SPC_ECHO_HHHH
#define SPC_ECHO_HHHH

#include <SDL2/SDL_types.h>

extern void echoEffectInit(int rate, Uint16 format, int channels);
extern void echoEffectFree();

enum EchoSetup
{
    ECHO_EON = 0,
    ECHO_EDL,
    ECHO_EFB,

    ECHO_MVOLL,
    ECHO_MVOLR,
    ECHO_EVOLL,
    ECHO_EVOLR,

    ECHO_FIR0,
    ECHO_FIR1,
    ECHO_FIR2,
    ECHO_FIR3,
    ECHO_FIR4,
    ECHO_FIR5,
    ECHO_FIR6,
    ECHO_FIR7
};

extern void spcEchoEffect(int chan, void *stream, int len, void *udata);
extern void spcEchoEffectDone(int chan, void *udata);

extern void echoEffectResetDefaults();

extern void  echoEffectSetReg(EchoSetup key, Uint8 val);
extern int  echoEffectGetReg(EchoSetup key);

#endif // SPC_ECHO_HHHH
