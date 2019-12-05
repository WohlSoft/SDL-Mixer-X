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

#include "SDL_log.h"

#define TAGS_INPUT_BUFFER_SIZE (5 * 8192)
#define ID3v2_BUFFER_SIZE       1024
#define APE_BUFFER_SIZE         256

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

static SDL_INLINE long id3v2_synchsafe_decode(const Uint8 *data)
{
    /* size is a 'synchsafe' integer (see above) */
    return (long)((data[0] << 21) + (data[1] << 14) + (data[2] << 7) + data[3]);
}

static SDL_INLINE long id3v2_byteint_decode(const Uint8 *data, size_t size)
{
    Uint32 result = 0;
    size_t i;

    for (i = 0; i < size; i++) {
        result <<= 8;
        result = result | (Uint8)data[i];
    }

    return (long)result;
}

static SDL_INLINE long get_id3v2_len(const Uint8 *data, long length)
{
    /* size is a 'synchsafe' integer (see above) */
    long size = id3v2_synchsafe_decode(data + 6);
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
static char *parse_id3v1_ansi_string(const Uint8 *buffer, size_t src_len)
{
    char *src_buffer = (char*)SDL_malloc(src_len + 1);
    char *ret;
    SDL_memset(src_buffer, 0, src_len);
    SDL_memcpy(src_buffer, buffer, src_len - 1);
    ret = SDL_iconv_string("UTF-8", "ISO-8859-1", src_buffer, src_len);
    SDL_free(src_buffer);
    return ret;
}

static void id3v1_set_tag(Mix_MusicMetaTags *out_tags, Mix_MusicMetaTag tag, const Uint8 *buffer, size_t len)
{
    char *src_buf = parse_id3v1_ansi_string(buffer, len);
    if (src_buf) {
        meta_tags_set(out_tags, tag, src_buf);
        SDL_free(src_buf);
    }
}

/* Parse content of ID3v1 tag */
static void parse_id3v1(Mix_MusicMetaTags *out_tags, const Uint8 *buffer)
{
    id3v1_set_tag(out_tags, MIX_META_TITLE, buffer + 3, 30);
    id3v1_set_tag(out_tags, MIX_META_ARTIST, buffer + 33, 30);
    id3v1_set_tag(out_tags, MIX_META_ALBUM, buffer + 63, 30);
    id3v1_set_tag(out_tags, MIX_META_COPYRIGHT, buffer + 97, 30);
}

/* Parse content of ID3v1 Enhanced tag */
static void parse_id3v1ext(Mix_MusicMetaTags *out_tags, const Uint8 *buffer)
{
    id3v1_set_tag(out_tags, MIX_META_TITLE, buffer + 4, 60);
    id3v1_set_tag(out_tags, MIX_META_ARTIST, buffer + 64, 60);
    id3v1_set_tag(out_tags, MIX_META_ALBUM, buffer + 124, 60);
}

/* Decode a string in the frame according to an encoding marker */
static char *id3v2_decode_string(const Uint8 *string, size_t size)
{
    char *str_buffer = NULL;
    char *src_buffer = NULL;
    size_t copy_size = size;

    if (size == 0) {
        SDL_Log("id3v2_decode_string: Bad string size: a string should have at least 1 byte");
        return NULL;
    }

    if (size < 2) {
        return NULL;
    }

    if (string[0] == '\x01') { /* UTF-16 string with a BOM */
        if (size <= 5) {
            if (size < 5) {
                SDL_Log("id3v2_decode_string: Bad BOM-UTF16 string size: %d < 5", (int)size);
            }
            return NULL;
        }

        copy_size = size - 3 + 2; /* exclude 3 bytes of encoding hint, append 2 bytes for a NULL termination */
        src_buffer = (char*)SDL_malloc(copy_size);
        SDL_memset(src_buffer, 0, copy_size);
        SDL_memcpy(src_buffer, (string + 3), copy_size - 2);

        if (SDL_memcmp(string, "\x01\xFE\xFF", 3) == 0) { /* UTF-16BE*/
            str_buffer = SDL_iconv_string("UTF-8", "UCS-2BE", src_buffer, copy_size);
        } else if (SDL_memcmp(string, "\x01\xFF\xFE", 3) == 0) {  /* UTF-16LE*/
            str_buffer = SDL_iconv_string("UTF-8", "UCS-2LE", src_buffer, copy_size);
        }
        SDL_free(src_buffer);

    } else if (string[0] == '\x02') { /* UTF-16BEstring without a BOM */
        if (size <= 3) {
            if (size < 3) {
                SDL_Log("id3v2_decode_string: Bad UTF16BE string size: %d < 3", (int)size);
            }
            return NULL; /* Blank string*/
        }

        copy_size = size - 1 + 2; /* exclude 1 byte of encoding hint, append 2 bytes for a NULL termination */
        src_buffer = (char*)SDL_malloc(copy_size);
        SDL_memset(src_buffer, 0, copy_size);
        SDL_memcpy(src_buffer, (string + 1), copy_size - 2);

        str_buffer = SDL_iconv_string("UTF-8", "UCS-2BE", src_buffer, copy_size);
        SDL_free(src_buffer);

    } else if (string[0] == '\x03') { /* UTF-8 string */
        if (size <= 2) {
            return NULL; /* Blank string*/
        }
        str_buffer = (char*)SDL_malloc(size);
        SDL_strlcpy(str_buffer, (const char*)(string + 1), size);

    } else if (string[0] == '\x00') { /* Latin-1 string */
        if (size <= 2) {
            return NULL; /* Blank string*/
        }
        str_buffer = parse_id3v1_ansi_string((string + 1), size);
    }

    return str_buffer;
}

/* Write a tag string into internal meta-tags storage */
static void write_id3v2_string(Mix_MusicMetaTags *out_tags, Mix_MusicMetaTag tag, const Uint8 *string, size_t size)
{
    char *str_buffer = id3v2_decode_string(string, size);

    if (str_buffer) {
        meta_tags_set(out_tags, tag, str_buffer);
        SDL_free(str_buffer);
    }

}

/* Identify a meta-key and decode the string (Note: input buffer should have at least 4 characters!) */
static void handle_id3v2_string(Mix_MusicMetaTags *out_tags, const char *key, const Uint8 *string, size_t size)
{
    if (SDL_memcmp(key, "TIT2", 4) == 0) {
        write_id3v2_string(out_tags, MIX_META_TITLE, string, size);
    } else if (SDL_memcmp(key, "TPE1", 4) == 0) {
        write_id3v2_string(out_tags, MIX_META_ARTIST, string, size);
    } else if (SDL_memcmp(key, "TALB", 4) == 0) {
        write_id3v2_string(out_tags, MIX_META_ALBUM, string, size);
    } else if (SDL_memcmp(key, "TCOP", 4) == 0) {
        write_id3v2_string(out_tags, MIX_META_COPYRIGHT, string, size);
    }
/* TODO: Extract "Copyright message" from TXXX value: a KEY=VALUE string divided by a zero byte:*/
/*
    else if (SDL_memcmp(key, "TXXX", 4) == 0) {
        write_id3v2_string(out_tags, MIX_META_COPYRIGHT, string, size);
    }
*/
}

/* Identify a meta-key and decode the string (Note: input buffer should have at least 4 characters!) */
static void handle_id3v2x2_string(Mix_MusicMetaTags *out_tags, const char *key, const Uint8 *string, size_t size)
{
    if (SDL_memcmp(key, "TT2", 3) == 0) {
        write_id3v2_string(out_tags, MIX_META_TITLE, string, size);
    } else if (SDL_memcmp(key, "TP1", 3) == 0) {
        write_id3v2_string(out_tags, MIX_META_ARTIST, string, size);
    } else if (SDL_memcmp(key, "TAL", 3) == 0) {
        write_id3v2_string(out_tags, MIX_META_ALBUM, string, size);
    } else if (SDL_memcmp(key, "TCR", 3) == 0) {
        write_id3v2_string(out_tags, MIX_META_COPYRIGHT, string, size);
    }
}

/* Parse a frame in ID3v2 format */
static size_t id3v2_parse_frame(Mix_MusicMetaTags *out_tags, SDL_RWops *src, Uint8 *buffer, Uint8 version)
{
    size_t size, head_size;
    char key[4];
    Uint8 flags[2];
    size_t read_size;
    Sint64 frame_begin = SDL_RWtell(src);

    if (version > 2) {
        head_size = 10;
        read_size = SDL_RWread(src, buffer, 1, head_size);

        if (read_size < head_size) {
            SDL_Log("id3v2_parse_frame: Buffer size that left is too small %d < 10", (int)read_size);
            SDL_RWseek(src, frame_begin, RW_SEEK_SET);
            return 0; /* Can't read frame header, possibly, a file size was reached */
        }

        if (SDL_memcmp(buffer, "\0\0\0\0", 4) == 0) {
            SDL_RWseek(src, frame_begin, RW_SEEK_SET);
            return 0;
        }

        SDL_memcpy(key, buffer, 4); /* Tag title (key) */

        if (version == 4) {
            size = (size_t)id3v2_synchsafe_decode(buffer + 4);
        } else {
            size = (size_t)id3v2_byteint_decode(buffer + 4, 4);
        }

        SDL_memcpy(flags, buffer + 8, 2);

        if (size < ID3v2_BUFFER_SIZE) {
            read_size = SDL_RWread(src, buffer, 1, size);
        } else {
            read_size = SDL_RWread(src, buffer, 1, ID3v2_BUFFER_SIZE);
            SDL_RWseek(src, frame_begin + (Sint64)size, RW_SEEK_SET);
        }

        handle_id3v2_string(out_tags, key, buffer, size);

    } else {
        head_size = 6;
        read_size = SDL_RWread(src, buffer, 1, head_size);

        if (read_size < head_size) {
            SDL_Log("id3v2_parse_frame: Buffer size that left is too small %d < 6", (int)read_size);
            SDL_RWseek(src, frame_begin, RW_SEEK_SET);
            return 0; /* Buffer size that left is too small */
        }

        if (SDL_memcmp(buffer, "\0\0\0", 3) == 0) {
            SDL_RWseek(src, frame_begin, RW_SEEK_SET);
            return 0;
        }

        SDL_memcpy(key, buffer, 3); /* Tag title (key) */

        size = (size_t)id3v2_byteint_decode(buffer + 3, 3);

        if (size < ID3v2_BUFFER_SIZE) {
            read_size = SDL_RWread(src, buffer, 1, size);
        } else {
            read_size = SDL_RWread(src, buffer, 1, ID3v2_BUFFER_SIZE);
            SDL_RWseek(src, frame_begin + (Sint64)size, RW_SEEK_SET);
        }

        handle_id3v2x2_string(out_tags, key, buffer, read_size);
    }

    return (size_t)(size + head_size); /* data size + size of the header */
}


/* Parse content of ID3v2 */
static SDL_bool parse_id3v2(Mix_MusicMetaTags *out_tags, SDL_RWops *src, Sint64 begin_pos)
{
    Uint8 version_major, flags;
    long total_length, tag_len, tag_extended_len = 0;
    Uint8 buffer[ID3v2_BUFFER_SIZE];
    size_t read_size;
    size_t frame_length;
    Sint64 file_size;

    total_length = 0;

    file_size = SDL_RWsize(src);
    SDL_RWseek(src, begin_pos, RW_SEEK_SET);
    read_size = SDL_RWread(src, buffer, 1, 10); /* Retrieve the header */
    if (read_size < 10) {
        SDL_Log("parse_id3v2: fail to read a header (%d < 10)", (int)read_size);
        SDL_RWseek(src, begin_pos, RW_SEEK_SET);
        return SDL_FALSE; /* Unsupported version of the tag */
    }

    total_length += 10;

    version_major = buffer[3]; /* Major version */
    /* version_minor = buffer[4]; * Minor version, UNUSED */
    flags = buffer[5]; /* Flags */
    tag_len = id3v2_synchsafe_decode(buffer + 6); /* Length of a tag */

    if (version_major != 2 && version_major != 3 && version_major != 4) {
        SDL_Log("parse_id3v2: Unsupported version %d", version_major);
        SDL_RWseek(src, begin_pos, RW_SEEK_SET);
        return SDL_FALSE; /* Unsupported version of the tag */
    }

    if ((version_major > 2) && ((flags & 0x40) == 0x40)) {
        tag_extended_len = id3v2_synchsafe_decode(buffer + 10); /* Length of an extended header */
    }

    if (tag_extended_len) {
        total_length += tag_extended_len + 4;
        SDL_RWseek(src, tag_extended_len + 4, RW_SEEK_CUR); /* Skip extended header and it's size value */
    }

    total_length += tag_len;

    if (flags & 0x10) {
        total_length += 10; /* footer size */
    }

    if ((SDL_RWtell(src) + tag_len) > file_size) {
        SDL_Log("parse_id3v2: Tag size bigger than actual file size");
        SDL_RWseek(src, begin_pos, RW_SEEK_SET);
        return SDL_FALSE; /* Tag size is bigger than actual buffer data */
    }

    while ((SDL_RWtell(src) >= 0) && (SDL_RWtell(src) < (begin_pos + total_length))) {
        frame_length = id3v2_parse_frame(out_tags, src, buffer, version_major);
        if (!frame_length) {
            break;
        }
    }

    SDL_RWseek(src, begin_pos, RW_SEEK_SET);

    return SDL_TRUE;
}


static SDL_INLINE long ape_byteint_decode(const Uint8 *data, size_t size)
{
    Uint32 result = 0;
    Sint32 i;

    for (i = size - 1; i >= 0; i--) {
        result <<= 8;
        result = result | (Uint8)data[i];
    }

    return (long)result;
}

static char *ape_find_value(char *key)
{
    char *end = (key + APE_BUFFER_SIZE - 4);

    while (*key && key != end) {
        key++;
    }

    if (key >= end) {
        return NULL;
    }

    return key + 1;
}

static Uint32 ape_handle_tag(Mix_MusicMetaTags *out_tags, Uint8 *data, size_t valsize)
{
    char *key = (char*)(data + 4);
    char *value = NULL;
    Uint32 key_len; /* Length of the key field */

    value = ape_find_value(key);

    if (!value) {
        return 0;
    }

    key_len = (Uint32)(value - key);

    if (valsize > APE_BUFFER_SIZE - key_len) {
        data[APE_BUFFER_SIZE] = '\0';
    } else {
        value[valsize] = '\0';
    }

    if (SDL_strncasecmp(key, "Title", 5) == 0) {
        meta_tags_set(out_tags, MIX_META_TITLE, (const char*)(value));
    } else if (SDL_memcmp(key, "Album", 5) == 0) {
        meta_tags_set(out_tags, MIX_META_ARTIST, (const char*)(value));
    } else if (SDL_memcmp(key, "Artist", 6) == 0) {
        meta_tags_set(out_tags, MIX_META_ALBUM, (const char*)(value));
    } else if (SDL_memcmp(key, "Copyright", 9) == 0) {
        meta_tags_set(out_tags, MIX_META_COPYRIGHT, (const char*)(value));
    }

    return 4 + valsize + key_len;
}

/* Parse content of APE tag  */
static SDL_bool parse_ape(Mix_MusicMetaTags *out_tags, SDL_RWops *src, Sint64 begin_pos, Uint32 version)
{
    Uint8 buffer[APE_BUFFER_SIZE + 1];
    Uint32 v, i, tag_size, tag_items_count, tag_item_size;
    Sint64 file_size, cur_tag;
    size_t read_size;

    file_size = SDL_RWsize(src);
    SDL_RWseek(src, begin_pos, RW_SEEK_SET);
    read_size = SDL_RWread(src, buffer, 1, 32); /* Retrieve the header */

    if (read_size < 32) {
        SDL_RWseek(src, begin_pos, RW_SEEK_SET);
        return SDL_FALSE;
    }

    v = (Uint32)ape_byteint_decode(buffer + 8, 4); /* version */
    if (v != 2000U && v != 1000U) {
        return SDL_FALSE;
    }

    tag_size = (Uint32)ape_byteint_decode(buffer + 12, 4); /* tag size */

    if (version == 1) { /* If version 1, we are at footer */
        if (begin_pos - (tag_size - 32) < 0) {
            SDL_RWseek(src, begin_pos, RW_SEEK_SET);
            return SDL_FALSE;
        }
        SDL_RWseek(src, begin_pos - (tag_size - 32), RW_SEEK_SET);
    } else if ((begin_pos + tag_size + 32) > file_size) {
        SDL_RWseek(src, begin_pos, RW_SEEK_SET);
        return SDL_FALSE;
    }

    tag_items_count = (Uint32)ape_byteint_decode(buffer + 16, 4); /* count tag items */

    /*flags = (Uint32)ape_byteint_decode(buffer + 20, 4);*/ /* global flags, unused */

    v = 0; /* reserved bits : */
    if (SDL_memcmp(&buffer[24], &v, 4) != 0 || SDL_memcmp(&buffer[28], &v, 4) != 0) {
        return SDL_FALSE;
    }

    for(i = 0; i < tag_items_count; i++) {
        cur_tag = SDL_RWtell(src);
        if (cur_tag < 0) {
            break;
        }
        read_size = SDL_RWread(src, buffer, 1, 4); /* Retrieve the size */
        if (read_size < 4) {
            SDL_RWseek(src, begin_pos, RW_SEEK_SET);
            return SDL_FALSE;
        }

        v = (Uint32)ape_byteint_decode(buffer, 4); /* size of the tag's value field */
        /* (we still need to find key size by a null termination) */

        /* Retrieve the tag's data with an aproximal size as we can */
        if (v + 40 < APE_BUFFER_SIZE) {
            read_size = SDL_RWread(src, buffer, 1, v + 40);
        } else {
            read_size = SDL_RWread(src, buffer, 1, APE_BUFFER_SIZE);
        }
        buffer[read_size] = '\0';

        tag_item_size = ape_handle_tag(out_tags, buffer, v);
        if (tag_item_size == 0) {
            break;
        }
        SDL_RWseek(src, cur_tag + tag_item_size + 4, RW_SEEK_SET);
    }

    SDL_RWseek(src, begin_pos, RW_SEEK_SET);

    return SDL_TRUE;
}

int id3tag_fetchTags(Mix_MusicMetaTags *out_tags, SDL_RWops *src, Id3TagLengthStrip *file_edges)
{
    Uint8 in_buffer[TAGS_INPUT_BUFFER_SIZE];
    long len;
    size_t readsize;
    Sint64 file_size, begin_pos, begin_offset = 0, tail_size = 0, ape_tag_pos;
    SDL_bool tag_handled = SDL_FALSE;

    if (file_edges) {
        file_edges->begin = 0;
        file_edges->end = 0;
    }

    if (!src) {
        return -1;
    }

    begin_pos = SDL_RWtell(src);
    file_size = SDL_RWsize(src);

    SDL_RWseek(src, begin_pos, RW_SEEK_SET);
    readsize = SDL_RWread(src, in_buffer, 1, TAGS_INPUT_BUFFER_SIZE);
    SDL_RWseek(src, begin_pos, RW_SEEK_SET);

    if (!readsize) {
        return -1;
    }

    /* ID3v2 tag is at the start */
    if (is_id3v2(in_buffer, readsize)) {
        len = get_id3v2_len(in_buffer, (long)readsize);
        if (len > file_size) {
            return -1;
        }
        tag_handled = parse_id3v2(out_tags, src, begin_pos);
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
        if (len >= file_size) {
            return -1;
        }
        parse_ape(out_tags, src, begin_pos, 2);
        tag_handled = SDL_TRUE;
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
        if (!tag_handled) {
            parse_id3v1(out_tags, in_buffer);
        }
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
            if (len >= file_size) {
                return -1;
            }
            if (v == 2000U) { /* verify header : */
                SDL_RWseek(src, -(tail_size + len), RW_SEEK_END);
                ape_tag_pos = SDL_RWtell(src);
                readsize = SDL_RWread(src, in_buffer, 1, 32);
                SDL_RWseek(src, begin_pos, RW_SEEK_SET);
                if (readsize != 32) {
                    return -1;
                }
                if (!is_apetag(in_buffer, 32)) {
                    return -1;
                }
                if (!tag_handled) {
                    parse_ape(out_tags, src, ape_tag_pos, 2);
                    SDL_RWseek(src, begin_pos, RW_SEEK_SET);
                }
            } else {
                SDL_RWseek(src, -(tail_size + 32), RW_SEEK_END);
                ape_tag_pos = SDL_RWtell(src);
                parse_ape(out_tags, src, ape_tag_pos, 1);
                SDL_RWseek(src, begin_pos, RW_SEEK_SET);
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
            if (!tag_handled) {
                parse_id3v1ext(out_tags, in_buffer);
            }
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
            if (len >= file_size) {
                return -1;
            }
            if (v == 2000U) { /* verify header : */
                SDL_RWseek(src, -(tail_size + len), RW_SEEK_END);
                ape_tag_pos = SDL_RWtell(src);
                readsize = SDL_RWread(src, in_buffer, 1, 32);
                SDL_RWseek(src, begin_pos, RW_SEEK_SET);
                if (readsize != 32) {
                    return -1;
                }
                if (!is_apetag(in_buffer, 32)) {
                    return -1;
                }
                if (!tag_handled) {
                    parse_ape(out_tags, src, ape_tag_pos, 2);
                    SDL_RWseek(src, begin_pos, RW_SEEK_SET);
                }
            } else if (!tag_handled) {
                SDL_RWseek(src, -(tail_size + 32), RW_SEEK_END);
                ape_tag_pos = SDL_RWtell(src);
                parse_ape(out_tags, src, ape_tag_pos, 1);
                SDL_RWseek(src, begin_pos, RW_SEEK_SET);
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
