/*
  SDL_mixer:  An audio mixer library based on the SDL library
  Copyright (C) 1997-2017 Sam Lantinga <slouken@libsdl.org>

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

#ifdef MUSIC_MP3_ID3TAG

#include "music_id3tag.h"

#include "id3tag.h"

Sint64 id3tag_fetchTags(Mix_MusicMetaTags *out_tags, SDL_RWops *src)
{
    struct id3_file *tags;
    Sint64 src_begin_pos = 0;

    tags = id3_file_from_rwops(src, ID3_FILE_MODE_READONLY);

    if (tags) {
        struct id3_tag  *tag = id3_file_tag(tags);
        struct id3_frame *pFrame;

        /* Attempt to escape a bug of MAD that causes a junk begin on some MP3 files */
        src_begin_pos = (Sint64)tag->paddedsize;

        /* Search for given frame by frame id */
        pFrame = id3_tag_findframe(tag, ID3_FRAME_TITLE, 0);
        if (pFrame != NULL) {
            union id3_field field = pFrame->fields[1];
            id3_ucs4_t const *pTemp = id3_field_getstrings(&field, 0);
            id3_latin1_t *pStrLatinl;
            if(pTemp != NULL) {
                pStrLatinl = id3_ucs4_latin1duplicate(pTemp);
                meta_tags_set(out_tags, MIX_META_TITLE, (const char*)pStrLatinl);
            }
        }
        pFrame = id3_tag_findframe(tag, ID3_FRAME_ARTIST, 0);
        if (pFrame != NULL) {
            union id3_field field = pFrame->fields[1];
            id3_ucs4_t const *pTemp = id3_field_getstrings(&field, 0);
            id3_latin1_t *pStrLatinl;
            if (pTemp != NULL) {
                pStrLatinl = id3_ucs4_latin1duplicate(pTemp);
                meta_tags_set(out_tags, MIX_META_ARTIST, (const char*)pStrLatinl);
            }
        }
        pFrame = id3_tag_findframe(tag, ID3_FRAME_ALBUM, 0);
        if (pFrame != NULL) {
            union id3_field field = pFrame->fields[1];
            id3_ucs4_t const *pTemp = id3_field_getstrings(&field, 0);
            id3_latin1_t *pStrLatinl;
            if (pTemp != NULL) {
                pStrLatinl = id3_ucs4_latin1duplicate(pTemp);
                meta_tags_set(out_tags, MIX_META_ALBUM, (const char*)pStrLatinl);
            }
        }
        pFrame = id3_tag_findframe(tag, "TCOP", 0);
        if (pFrame != NULL) {
            union id3_field field = pFrame->fields[1];
            id3_ucs4_t const *pTemp = id3_field_getstrings(&field, 0);
            id3_latin1_t *pStrLatinl;
            if (pTemp != NULL) {
                pStrLatinl = id3_ucs4_latin1duplicate(pTemp);
                meta_tags_set(out_tags, MIX_META_COPYRIGHT, (const char*)pStrLatinl);
            }
        }
        id3_file_close(tags);
        SDL_RWseek(src, src_begin_pos, RW_SEEK_SET);
    }

    return src_begin_pos;
}

#else
Sint64 id3tag_fetchTags(Mix_MusicMetaTags *out_tags, SDL_RWops *src)
{
    (void)out_tags;
    (void)src;
    return 0;
}
#endif
