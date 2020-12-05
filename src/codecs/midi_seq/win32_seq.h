#ifndef NATIVE_MIDI_WIN32_SEQ
#define NATIVE_MIDI_WIN32_SEQ

#ifdef __cplusplus
extern "C" {
#endif

extern void *win32_seq_init_interface();
extern void win32_seq_free(void *seq);

extern int win32_seq_init_midi_out(void *seq);
extern void win32_seq_close_midi_out(void *seq);

extern int win32_seq_openData(void *seq, void *bytes, unsigned long len);
extern int win32_seq_openFile(void *seq, const char *path);

extern const char *win32_seq_meta_title(void *seq);
extern const char *win32_seq_meta_copyright(void *seq);

extern const char *win32_seq_get_error(void *seq);

extern void win32_seq_rewind(void *seq);
extern void win32_seq_seek(void *seq, double pos);
extern double win32_seq_tell(void *seq);
extern double win32_seq_length(void *seq);
extern double win32_seq_loop_start(void *seq);
extern double win32_seq_loop_end(void *seq);
extern int win32_seq_at_end(void *seq);

extern void win32_seq_set_volume(void *seq, double volume);

extern void win32_seq_all_notes_off(void *seq);

extern double win32_seq_get_tempo_multiplier(void *seq);
extern void win32_seq_set_tempo_multiplier(void *seq, double tempo);
extern void win32_seq_set_loop_enabled(void *seq, int loopEn);

extern double win32_seq_tick(void *seq, double s, double granularity);

#ifdef __cplusplus
}
#endif

#endif /* NATIVE_MIDI_WIN32_SEQ */
