#pragma once
#include <stddef.h>
#include <stdint.h>
typedef void (*zc_output_fn)(const char *);
/* A native-session key source returns one MESSAGE_KEY_DOWN/UP event, or zero.
 * reset discards shell-era device events before a guest call takes input. */
typedef int (*zc_key_event_fn)(uint8_t *type, uint64_t *arg1, uint64_t *arg2);
typedef void (*zc_key_reset_fn)(void);
void zc_set_key_reader(zc_key_event_fn read, zc_key_reset_fn reset);
/* Return 0 and file length. A NULL destination asks for the length only. */
typedef int (*zc_source_read_fn)(const char *name, char *dst, size_t cap, size_t *length);
void zc_set_source_reader(zc_source_read_fn read);
/* Read-only source archive. The bytes must stay alive for the session. */
void zc_init(const void *archive, size_t size, zc_output_fn output);
/* Load the pinned ZealOS kernel/window/graphics source for normal UTM boot. */
int zc_start_graphics(void);
/* Start the loaded upstream WinMgrTask once, leaving the bootstrap shell active. */
int zc_start_winmgr(void);
/* Stop that task and resume bootstrap-owned framebuffer refreshes. */
int zc_stop_winmgr(void);
/* Load and run the interactive source-backed two-window UTM fixture. */
int zc_start_window_demo(void);
/* Stop the fixture windows and return to the bootstrap shell display. */
int zc_stop_window_demo(void);
int zc_load(const char *path);
int zc_exec(const char *source);
int zc_call(const char *name, int64_t *result);
/* Give one cooperative guest task an opportunity to run while the shell idles. */
void zc_idle_step(void);
/* Best-effort name for a PC in the current session's generated code arena. */
const char *zc_debug_function_for_pc(uintptr_t pc, uintptr_t *offset);
/* Packed absolute tablet sample for ZealC window-control polling, or -1. */
int64_t zc_tablet_sample(void);
/* True while a live child owns input; the shell must not read device keys. */
int zc_focus_owns_input(void);
int zc_selftest(void);
void zc_status(void);
