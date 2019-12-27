
#include "SDL_test.h"

#define ENABLE_ALL_MP3_TAGS
#include "mp3utils.h"

static void verify_file(const char *path, SDL_bool keep_id3v2,
                       Sint64 begin, Sint64 length,
                       const char *title,
                       const char *artist,
                       const char *album,
                       const char *copyright)
{
    Mix_MusicMetaTags tags;
    struct mp3file_t f;
    int ret = 0;
    const char *t_title, *t_artist, *t_album, *t_copyright;

    meta_tags_init(&tags);
    SDL_memset(&f, 0, sizeof(struct mp3file_t));

    f.src = SDL_RWFromFile(path, "rb");

    SDLTest_AssertCheck(f.src != NULL, "Check that file opening is valid");

    if (!f.src) {
        meta_tags_clear(&tags);
        return;
    }

    f.length = SDL_RWsize(f.src);

    ret = mp3_read_tags(&tags, &f, keep_id3v2);
    SDLTest_AssertCheck(ret >= 0, "Check that tag reading is valid (%d)", ret);

    SDLTest_AssertCheck(f.start == begin,   "Check that start position is matches %d==%d value", (int)f.start, (int)begin);
    SDLTest_AssertCheck(f.length == length, "Check that audio chunk length is matches to %d==%d value", (int)f.length, (int)length);

    t_title = meta_tags_get(&tags, MIX_META_TITLE);
    t_artist = meta_tags_get(&tags, MIX_META_ARTIST);
    t_album = meta_tags_get(&tags, MIX_META_ALBUM);
    t_copyright = meta_tags_get(&tags, MIX_META_COPYRIGHT);

    SDLTest_AssertCheck(SDL_strncmp(t_title, title, SDL_strlen(title)) == 0,
                        "Check that title tag is matches \"%s\" == \"%s\" value", t_title, title);
    SDLTest_AssertCheck(SDL_strncmp(t_artist, artist, SDL_strlen(artist)) == 0,
                        "Check that artist tag is matches \"%s\" == \"%s\" value", t_artist, artist);
    SDLTest_AssertCheck(SDL_strncmp(t_album, album, SDL_strlen(album)) == 0,
                        "Check that album tag is matches \"%s\" == \"%s\" value", t_album, album);
    SDLTest_AssertCheck(SDL_strncmp(t_copyright, copyright, SDL_strlen(copyright)) == 0,
                        "Check that copyright tag is matches \"%s\" == \"%s\" value", t_copyright, copyright);

    meta_tags_clear(&tags);
}


static int tag_ap2v1_with_id3v1(void *arg)
{
    (void)arg;
    verify_file("APEv1+ID3v1tag.mp3", SDL_FALSE,
                0, 2869,
                "Всего лишь мусор",
                "Местный житель",
                "Байки из склепа",
                "Copyburp (|) All rights sucks");

    return TEST_COMPLETED;
}


static int tag_ap2v1(void *arg)
{
    (void)arg;
    verify_file("APEv1tag.mp3", SDL_FALSE,
                0, 2869,
                "Всего лишь мусор",
                "Местный житель",
                "Байки из склепа",
                "Copyburp (|) All rights sucks");

    return TEST_COMPLETED;
}

static int tag_ap2v2_at_begin(void *arg)
{
    (void)arg;
    verify_file("APEv2AtBegin+ID3v1tag.mp3", SDL_FALSE,
                162, 2869,
                "Abrakudbra",
                "Some odd jerk",
                "Odd things of Kek",
                "");
    return TEST_COMPLETED;
}

static int tag_ap2v2_with_id3v1(void *arg)
{
    (void)arg;
    verify_file("APEv2+ID3v1tag.mp3", SDL_FALSE,
                0, 2869,
                "Abrakudbra",
                "Some odd jerk",
                "Odd things of Kek",
                "");
    return TEST_COMPLETED;
}

/* TODO: Implement other test cases */

static const SDLTest_TestCaseReference mp3tagTest1 =
        { (SDLTest_TestCaseFp)tag_ap2v1_with_id3v1,"tag_ap2v1_with_id3v1",  "Tests APEv1 tag with leading ID3v1", TEST_ENABLED };
static const SDLTest_TestCaseReference mp3tagTest2 =
        { (SDLTest_TestCaseFp)tag_ap2v1,"tag_ap2v1",  "Tests APEv1 tag", TEST_ENABLED };
static const SDLTest_TestCaseReference mp3tagTest3 =
        { (SDLTest_TestCaseFp)tag_ap2v2_at_begin,"tag_ap2v2_at_begin",  "Tests APEv2 tag at file begin", TEST_ENABLED };
static const SDLTest_TestCaseReference mp3tagTest4 =
        { (SDLTest_TestCaseFp)tag_ap2v2_with_id3v1,"tag_ap2v2_with_id3v1",  "Tests APEv2 tag with leading ID3v1", TEST_ENABLED };

static const SDLTest_TestCaseReference *mp3Tests[] =  {
    &mp3tagTest1, &mp3tagTest2, &mp3tagTest3, &mp3tagTest4,
    NULL
};

/* Rect test suite (global) */
SDLTest_TestSuiteReference mp3utilsTestSuite = {
    "mp3utils",
    NULL,
    mp3Tests,
    NULL
};


/* All test suites */
SDLTest_TestSuiteReference *testSuites[] =  {
    &mp3utilsTestSuite,
    NULL
};

/* Call this instead of exit(), so we can clean up SDL: atexit() is evil. */
static void quit(int rc)
{
    exit(rc);
}

int
main(int argc, char *argv[])
{
    int result;
    (void)argc; (void)argv;

    /* Call Harness */
    result = SDLTest_RunSuites(testSuites, NULL, 0, NULL, 1);

    /* Shutdown everything */
    quit(result);
    return(result);
}
