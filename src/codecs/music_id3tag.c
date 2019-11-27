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

#include "music_id3tag.h"

#define TAGS_INPUT_BUFFER_SIZE (5 * 8192)

static SDL_INLINE SDL_bool is_id3v1(const Uint8 *data, size_t length)
{
    /* http://id3.org/ID3v1 :  3 bytes "TAG" identifier and 125 bytes tag data */
    if (length < 128 || SDL_memcmp(data,"TAG", 3) != 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static SDL_INLINE SDL_bool is_id3v1ext(const Uint8 *data, size_t length)
{
    /* ID3v1 extended tag: just before ID3v1, always 227 bytes.
     * https://www.getid3.org/phpBB3/viewtopic.php?t=1202
     * https://en.wikipedia.org/wiki/ID3v1#Enhanced_tag
     * Not an official standard, is only supported by few programs. */
    if (length < 227 || SDL_memcmp(data,"TAG+",4) != 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static SDL_INLINE SDL_bool is_id3v2(const Uint8 *data, size_t length)
{
    /* ID3v2 header is 10 bytes:  http://id3.org/id3v2.4.0-structure */
    /* bytes 0-2: "ID3" identifier */
    if (length < 10 || SDL_memcmp(data, "ID3",3) != 0) {
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

static SDL_INLINE long get_id3v2_len(const Uint8 *data, long length)
{
    /* size is a 'synchsafe' integer (see above) */
    long size = (long)((data[6]<<21) + (data[7]<<14) + (data[8]<<7) + data[9]);
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

static SDL_INLINE SDL_bool is_apetag(const Uint8 *data, size_t length)
{
   /* http://wiki.hydrogenaud.io/index.php?title=APEv2_specification
    * Header/footer is 32 bytes: bytes 0-7 ident, bytes 8-11 version,
    * bytes 12-17 size. bytes 24-31 are reserved: must be all zeroes. */
    Uint32 v;

    if (length < 32 || SDL_memcmp(data,"APETAGEX",8) != 0) {
        return SDL_FALSE;
    }
    v = (Uint32)((data[11]<<24) | (data[10]<<16) | (data[9]<<8) | data[8]); /* version */
    if (v != 2000U && v != 1000U) {
        return SDL_FALSE;
    }
    v = 0; /* reserved bits : */
    if (SDL_memcmp(&data[24],&v,4) != 0 || SDL_memcmp(&data[28],&v,4) != 0) {
        return SDL_FALSE;
    }
    return SDL_TRUE;
}

static SDL_INLINE long get_ape_len(const Uint8 *data, size_t datalen, Uint32 *version)
{
    long size = (long)((data[15]<<24) | (data[14]<<16) | (data[13]<<8) | data[12]);
    *version = (Uint32)((data[11]<<24) | (data[10]<<16) | (data[9]<<8) | data[8]);
    (void)datalen;
    return size; /* caller will handle the additional v2 header length */
}

/* Parse ASCII string and clean it up from non-ASCII characters (replace them with a question mark '?') */
static void parse_id3v1_ascii_string(char *dst, Uint8 *buffer, size_t dst_len)
{
    char *beg = dst, *end = (dst + dst_len);
    SDL_memset(dst, 0, dst_len);
    SDL_strlcpy(dst, (const char*)buffer, dst_len);
    for( ; beg != end; ++beg) {
        if (*beg < 0) {
            *beg = '?';
        }
    }
}

/* Parse content of ID3v1 tag */
static void parse_id3v1(Mix_MusicMetaTags *out_tags, Uint8 *buffer)
{
    char id3v1_buf[30];
    parse_id3v1_ascii_string(id3v1_buf, buffer + 3, 30);
    meta_tags_set(out_tags, MIX_META_TITLE, id3v1_buf);

    parse_id3v1_ascii_string(id3v1_buf, buffer + 33, 30);
    meta_tags_set(out_tags, MIX_META_ARTIST, id3v1_buf);

    parse_id3v1_ascii_string(id3v1_buf, buffer + 63, 30);
    meta_tags_set(out_tags, MIX_META_ALBUM, id3v1_buf);

    parse_id3v1_ascii_string(id3v1_buf, buffer + 97, 30);
    meta_tags_set(out_tags, MIX_META_COPYRIGHT, id3v1_buf);
}

/* Parse content of ID3v1 Enhanced tag */
static void parse_id3v1ext(Mix_MusicMetaTags *out_tags, Uint8 *buffer)
{
    char id3v1_buf[60];
    parse_id3v1_ascii_string(id3v1_buf, buffer + 4, 60);
    meta_tags_set(out_tags, MIX_META_TITLE, id3v1_buf);

    parse_id3v1_ascii_string(id3v1_buf, buffer + 64, 60);
    meta_tags_set(out_tags, MIX_META_ARTIST, id3v1_buf);

    parse_id3v1_ascii_string(id3v1_buf, buffer + 124, 60);
    meta_tags_set(out_tags, MIX_META_ALBUM, id3v1_buf);
}

int id3tag_fetchTags(Mix_MusicMetaTags *out_tags, SDL_RWops *src, Id3TagLengthStrip *file_edges)
{
    Uint8 in_buffer[TAGS_INPUT_BUFFER_SIZE];
    long len;
    size_t readsize, file_size;
    Sint64 begin_pos, begin_offset = 0, tail_size = 0;

    if (file_edges) {
        file_edges->begin = 0;
        file_edges->end = 0;
    }

    if (!src) {
        return -1;
    }

    begin_pos = SDL_RWtell(src);
    SDL_RWseek(src, 0, RW_SEEK_END);
    file_size = (size_t)(SDL_RWtell(src) - begin_pos);
    SDL_RWseek(src, begin_pos, RW_SEEK_SET);

    SDL_RWseek(src, -128, RW_SEEK_END);
    readsize = SDL_RWread(src, in_buffer, 1, TAGS_INPUT_BUFFER_SIZE);
    SDL_RWseek(src, begin_pos, RW_SEEK_SET);

    if (!readsize) {
        return -1;
    }

    /* ID3v2 tag is at the start */
    if (is_id3v2(in_buffer, readsize)) {
        len = get_id3v2_len(in_buffer, (long)readsize);
        if ((size_t)len >= file_size) {
            return -1;
        }
        begin_offset += len;
        file_size -= (size_t)len;
        if (file_edges) {
            file_edges->begin += len;
        }
    }
    /* APE tag _might_ be at the start: read the header */
    else if (is_apetag(in_buffer, readsize)) {
        Uint32 v;
        len = get_ape_len(in_buffer, readsize, &v);
        len += 32; /* we're at top: have a header. */
        if ((size_t)len >= file_size) {
            return -1;
        }
        begin_offset += len;
        file_size -= (size_t)len;
        if (file_edges) {
            file_edges->begin += len;
        }
    }

    if (file_size < 128) {
        goto ape;
    }

    SDL_RWseek(src, -(tail_size + 128), RW_SEEK_END);
    readsize = SDL_RWread(src, in_buffer, 1, 128);
    SDL_RWseek(src, begin_pos, RW_SEEK_SET);

    /* ID3v1 tag is at the end */
    if (is_id3v1(in_buffer, readsize)) {
        parse_id3v1(out_tags, in_buffer);
        tail_size += 128;
        file_size -= 128;
        if (file_edges) {
            file_edges->end += 128;
        }

        /* APE tag may be before the ID3v1: read the footer */
        if (file_size < 32) {
            goto end;
        }

        SDL_RWseek(src, -(tail_size + 32), RW_SEEK_END);
        readsize = SDL_RWread(src, in_buffer, 1, 32);
        SDL_RWseek(src, begin_pos, RW_SEEK_SET);

        if (is_apetag(in_buffer, 32)) {
            Uint32 v;
            len = get_ape_len(in_buffer, readsize, &v);
            if (v == 2000U) {
                len += 32; /* header */
            }
            if ((size_t)len >= file_size) {
                return -1;
            }
            if (v == 2000U) { /* verify header : */
                SDL_RWseek(src, -(tail_size + len), RW_SEEK_END);
                readsize = SDL_RWread(src, in_buffer, 1, 32);
                SDL_RWseek(src, begin_pos, RW_SEEK_SET);
                if (readsize != 32) {
                    return -1;
                }
                if (!is_apetag(in_buffer, 32)) {
                    return -1;
                }
            }
            file_size -= (size_t)len;
            tail_size += len;
            if (file_edges) {
                file_edges->end += len;
            }
            goto end;
        }

        /* extended ID3v1 just before the ID3v1 tag? (unlikely)  */
        if (file_size < 227) {
            return 0;
        }
        SDL_RWseek(src, -(tail_size + 227), RW_SEEK_END);
        readsize = SDL_RWread(src, in_buffer, 1, 227);
        SDL_RWseek(src, begin_pos, RW_SEEK_SET);
        if (readsize != 227) {
            return 0;
        }
        if (is_id3v1ext(in_buffer, readsize)) {
            parse_id3v1ext(out_tags, in_buffer);
            file_size -= 227;
            tail_size += 227;
            if (file_edges) {
                file_edges->end += 227;
            }
            goto end;
        }
    }

ape: /* APE tag may be at the end: read the footer */
    if (file_size >= 32) {
        SDL_RWseek(src, -(tail_size + 32), RW_SEEK_END);
        readsize = SDL_RWread(src, in_buffer, 1, 32);
        SDL_RWseek(src, begin_pos, RW_SEEK_SET);
        if (readsize != 32) {
            return -1;
        }
        if (is_apetag(in_buffer, 32)) {
            Uint32 v;
            len = get_ape_len(in_buffer, readsize, &v);
            if (v == 2000U) {
                len += 32; /* header */
            }
            if ((size_t)len >= file_size) {
                return -1;
            }
            if (v == 2000U) { /* verify header : */
                SDL_RWseek(src, -(tail_size + len), RW_SEEK_END);
                readsize = SDL_RWread(src, in_buffer, 1, 32);
                SDL_RWseek(src, begin_pos, RW_SEEK_SET);
                if (readsize != 32) {
                    return -1;
                }
                if (!is_apetag(in_buffer, 32)) {
                    return -1;
                }
            }
            file_size -= (size_t)len;
            tail_size += len;
            if (file_edges) {
                file_edges->end += len;
            }
        }
    }

end:
    return (file_size > 0) ? 0 :-1;
}

int id3tag_fetchTagsFromMemory(Mix_MusicMetaTags *out_tags, Uint8 *data, size_t length, Id3TagLengthStrip *file_edges)
{
    SDL_RWops *src = SDL_RWFromConstMem(data, (int)length);
    if (src) {
        int ret = id3tag_fetchTags(out_tags, src, file_edges);
        SDL_RWclose(src);
        return ret;
    }
    return -1;
}
