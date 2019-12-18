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

#include "utils.h"

double str_to_float(const char *str)
{
    char str_buff[25];
    char float_buff[4];
    char *p;
    /* UGLY WORKAROUND: Replace dot with local character (for example, comma) */
    SDL_strlcpy(str_buff, str, 25);
    SDL_snprintf(float_buff, 4, "%.1f", 0.0);
    for (p = str_buff; (p = SDL_strchr(p, '.')); ++p) {
        *p = float_buff[1];
    }
    return SDL_strtod(str_buff, NULL);
}

static size_t _utf16_byte_len(const char *str)
{
    size_t len = 0;
    const char *cur = str;
    if (!cur)
        return 0;

    while (cur[0] != '\0' || cur[1] != '\0') {
        len += 2;
        cur += 2;
    }
    return len;
}

void meta_tags_set_from_midi(Mix_MusicMetaTags *tags, Mix_MusicMetaTag tag, const char *src)
{
    size_t src_len = SDL_strlen(src);
    char *dst = NULL;

    if (src_len >= 3 && (SDL_memcmp(src, "\xEF\xBB\xBF", 3) == 0)) {
        dst = SDL_strdup(src + 3);
    } else if (src_len >= 2 && (SDL_memcmp(src, "\xFF\xFE", 2) == 0)) {
        dst = SDL_iconv_string("UTF-8", "UCS-2LE", src, _utf16_byte_len(src) + 2);
    } else if (src_len >= 2 && (SDL_memcmp(src, "\xFE\xFF", 2) == 0)) {
        dst = SDL_iconv_string("UTF-8", "UCS-2BE", src, _utf16_byte_len(src) + 2);
    } else {
        dst = SDL_iconv_string("UTF-8", "ISO-8859-1", src, SDL_strlen(src) + 1);
    }

    if (dst) {
        meta_tags_set(tags, tag, dst);
        SDL_free(dst);
    }
}

/* Is given tag a loop tag? */
SDL_bool is_loop_tag(const char *tag)
{
    char buf[5];
    SDL_strlcpy(buf, tag, 5);
    return SDL_strcasecmp(buf, "LOOP") == 0;
}

/* Parse time string of the form HH:MM:SS.mmm and return equivalent sample
 * position */
Sint64 parse_time(char *time, long samplerate_hz)
{
    char *num_start, *p;
    Sint64 result = 0;
    char c; int val;

    /* Time is directly expressed as a sample position */
    if (SDL_strchr(time, ':') == NULL) {
        return SDL_strtoll(time, NULL, 10);
    }

    result = 0;
    num_start = time;

    for (p = time; *p != '\0'; ++p) {
        if (*p == '.' || *p == ':') {
            c = *p; *p = '\0';
            if ((val = SDL_atoi(num_start)) < 0)
                return -1;
            result = result * 60 + val;
            num_start = p + 1;
            *p = c;
        }

        if (*p == '.') {
            double val_f = str_to_float(p);
            if (val_f < 0) return -1;
            printf("ToF: %g from str: %s\n", val_f, p);
            fflush(stdout);
            return result * samplerate_hz + (Sint64) (val_f * samplerate_hz);
        }
    }

    if ((val = SDL_atoi(num_start)) < 0) return -1;
    return (result * 60 + val) * samplerate_hz;
}
