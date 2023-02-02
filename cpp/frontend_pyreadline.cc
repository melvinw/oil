// This file is largely derived from pyext/line_input.c
#include "frontend_pyreadline.h"

#include <assert.h>

#include "_build/detected-cpp-config.h"

#if HAVE_READLINE
  #include <readline/history.h>
  #include <readline/readline.h>
#endif

namespace py_readline {

static Readline* gReadline = nullptr;

#if HAVE_READLINE
  /* -------------------------------------------------------------------------
   */

  /* OVM_MAIN: This section copied from autotool-generated pyconfig.h.
   * We're not detecting any of it in Oil's configure script.  They are for
   * ancient readline versions.
   * */

  /* Define if you have readline 2.1 */
  #define HAVE_RL_CALLBACK 1

  /* Define if you can turn off readline's signal handling. */
  #define HAVE_RL_CATCH_SIGNAL 1

  /* Define if you have readline 2.2 */
  #define HAVE_RL_COMPLETION_APPEND_CHARACTER 1

  /* Define if you have readline 4.0 */
  #define HAVE_RL_COMPLETION_DISPLAY_MATCHES_HOOK 1

  /* Define if you have readline 4.2 */
  #define HAVE_RL_COMPLETION_MATCHES 1

  /* Define if you have rl_completion_suppress_append */
  #define HAVE_RL_COMPLETION_SUPPRESS_APPEND 1

  /* Define if you have readline 4.0 */
  #define HAVE_RL_PRE_INPUT_HOOK 1

  /* Define if you have readline 4.0 */
  #define HAVE_RL_RESIZE_TERMINAL 1

/* ------------------------------------------------------------------------- */

  #ifdef __APPLE__
/*
 * It is possible to link the readline module to the readline
 * emulation library of editline/libedit.
 *
 * On OSX this emulation library is not 100% API compatible
 * with the "real" readline and cannot be detected at compile-time,
 * hence we use a runtime check to detect if we're using libedit
 *
 * Currently there is one known API incompatibility:
 * - 'get_history' has a 1-based index with GNU readline, and a 0-based
 *   index with older versions of libedit's emulation.
 * - Note that replace_history and remove_history use a 0-based index
 *   with both implementations.
 */
static int using_libedit_emulation = 0;
static const char libedit_version_tag[] = "EditLine wrapper";

static int libedit_history_start = 0;
  #endif /* __APPLE__ */

#ifdef HAVE_RL_COMPLETION_MATCHES
#define completion_matches(x, y) \
    rl_completion_matches((x), ((rl_compentry_func_t *)(y)))
#else
#if defined(_RL_FUNCTION_TYPEDEF)
extern char **completion_matches(char *, rl_compentry_func_t *);
#else

#if !defined(__APPLE__)
extern char **completion_matches(char *, CPFunction *);
#endif
#endif
#endif

static char* do_complete(const char* text, int state) {
  if (gReadline->completer_ == nullptr) {
    return nullptr;
  }
  rl_attempted_completion_over = 1;
  Str *gc_text = StrFromC(text);
  Str *result = completion::ExecuteReadlineCallback(gReadline->completer_, gc_text, state);
  if (result == nullptr) {
    return nullptr;
  }

  // According to https://web.mit.edu/gnu/doc/html/rlman_2.html#SEC37, readline
  // will free any memory we return to it.
  int n = len(result) + 1;
  auto* ret = static_cast<char*>(malloc(n));
  DCHECK(ret != nullptr);
  memcpy(ret, result->data(), n);
  return ret;
}

static void
on_completion_display_matches_hook(char **matches,
                                   int num_matches, int max_length) {
  if (gReadline->display_ == nullptr) {
    return;
  }
  auto* gc_matches = Alloc<List<Str*>>();
  for (int i = 0; i < num_matches; i++) {
    gc_matches->append(StrFromC(matches[i]));
  }
  comp_ui::ExecutePrintCandidates(gReadline->display_, nullptr, gc_matches, max_length);
}

static char** flex_complete(const char* text, int start, int end) {
  rl_completion_append_character = '\0';
  rl_completion_suppress_append = 0;
  gReadline->begidx_ = start;
  gReadline->endidx_ = end;
  return completion_matches(text, *do_complete);
}
#endif

Readline::Readline()
    : GC_CLASS_FIXED(header_, Readline::field_mask(), sizeof(Readline)),
      begidx_(),
      endidx_(),
      completer_delims_(StrFromC(" \t\n`~!@#$%^&*()-=+[{]}\\|;:'\",<>/?")),
      completer_(),
      display_() {
#if HAVE_READLINE
  #ifdef SAVE_LOCALE
  char* saved_locale = strdup(setlocale(LC_CTYPE, NULL));
  if (!saved_locale) Py_FatalError("not enough memory to save locale");
  #endif

  #ifdef __APPLE__
  /* the libedit readline emulation resets key bindings etc
   * when calling rl_initialize.  So call it upfront
   */
  if (using_libedit_emulation) rl_initialize();

  /* Detect if libedit's readline emulation uses 0-based
   * indexing or 1-based indexing.
   */
  add_history("1");
  if (history_get(1) == NULL) {
    libedit_history_start = 0;
  } else {
    libedit_history_start = 1;
  }
  clear_history();
  #endif /* __APPLE__ */

  using_history();

  rl_readline_name = "python";
  #if defined(PYOS_OS2) && defined(PYCC_GCC)
  /* Allow $if term= in .inputrc to work */
  rl_terminal_name = getenv("TERM");
  #endif
  /* Force rebind of TAB to insert-tab */
  rl_bind_key('\t', rl_insert);
  /* Bind both ESC-TAB and ESC-ESC to the completion function */
  rl_bind_key_in_map('\t', rl_complete, emacs_meta_keymap);
  rl_bind_key_in_map('\033', rl_complete, emacs_meta_keymap);
  rl_attempted_completion_function = flex_complete;
  rl_completion_display_matches_hook = on_completion_display_matches_hook;
  rl_catch_signals = 0;
  rl_catch_sigwinch = 0;
  rl_initialize();
#else
  assert(0);  // not implemented
#endif
}

void Readline::parse_and_bind(Str* s) {
#if HAVE_READLINE
  /* Make a copy -- rl_parse_and_bind() modifies its argument */
  /* Bernard Herzog */
  Str* copy = StrFromC(s->data(), len(s));
  rl_parse_and_bind(copy->data());
#else
  assert(0);  // not implemented
#endif
}

void Readline::add_history(Str* line) {
#if HAVE_READLINE
  assert(line != nullptr);
  ::add_history(line->data());
#else
  assert(0);  // not implemented
#endif
}

void Readline::read_history_file(Str* path) {
#if HAVE_READLINE
  if (path != nullptr) {
    read_history(path->data());
  } else {
    read_history(nullptr);
  }
#else
  assert(0);  // not implemented
#endif
}

void Readline::write_history_file(Str* path) {
#if HAVE_READLINE
  if (path != nullptr) {
    write_history(path->data());
  } else {
    write_history(nullptr);
  }
#else
  assert(0);  // not implemented
#endif
}

void Readline::set_completer(completion::ReadlineCallback* completer) {
#if HAVE_READLINE
  completer_ = completer;
#else
  assert(0);  // not implemented
#endif
}

void Readline::set_completer_delims(Str* delims) {
#if HAVE_READLINE
  completer_delims_ = StrFromC(delims->data(), len(delims));
  rl_completer_word_break_characters = completer_delims_->data();
#else
  assert(0);  // not implemented
#endif
}

void Readline::set_completion_display_matches_hook(
      comp_ui::_IDisplay* display) {
#if HAVE_READLINE
  display_ = display;
#else
  assert(0);  // not implemented
#endif
}

Str* Readline::get_line_buffer() {
#if HAVE_READLINE
  return StrFromC(rl_line_buffer);
#else
  assert(0);  // not implemented
#endif
}

int Readline::get_begidx() {
#if HAVE_READLINE
  return begidx_;
#else
  assert(0);  // not implemented
#endif
}

int Readline::get_endidx() {
#if HAVE_READLINE
  return endidx_;
#else
  assert(0);  // not implemented
#endif
}

void Readline::clear_history() {
#if HAVE_READLINE
  rl_clear_history();
#else
  assert(0);  // not implemented
#endif
}

void Readline::remove_history_item(int pos) {
#if HAVE_READLINE
  HIST_ENTRY* entry = remove_history(pos);
  #if defined(RL_READLINE_VERSION) && RL_READLINE_VERSION >= 0x0500
  histdata_t data = free_history_entry(entry);
  free(data);
  #else
  if (entry->line) {
    free((void*)entry->line);
  }
  if (entry->data) {
    free(entry->data);
  }
  free(entry);
  #endif
#else
  assert(0);  // not implemented
#endif
}

Str* Readline::get_history_item(int pos) {
#if HAVE_READLINE
  #ifdef __APPLE__
  if (using_libedit_emulation) {
    /* Older versions of libedit's readline emulation
     * use 0-based indexes, while readline and newer
     * versions of libedit use 1-based indexes.
     */
    int length = get_current_history_length();

    pos = pos - 1 + libedit_history_start;

    /*
     * Apple's readline emulation crashes when
     * the index is out of range, therefore
     * test for that and fail gracefully.
     */
    if (pos < (0 + libedit_history_start) ||
        pos >= (length + libedit_history_start)) {
      return nullptr;
    }
  }
  #endif
  HIST_ENTRY* hist_ent = history_get(pos);
  if (hist_ent != nullptr) {
    return StrFromC(hist_ent->line);
  }
  return nullptr;
#else
  assert(0);  // not implemented
#endif
}

int Readline::get_current_history_length() {
#if HAVE_READLINE
  HISTORY_STATE* hist_st = history_get_history_state();
  int length = hist_st->length;
  free(hist_st);
  return length;
#else
  assert(0);  // not implemented
#endif
}

void Readline::resize_terminal() {
#if HAVE_READLINE
  rl_resize_terminal();
#else
  assert(0);  // not implemented
#endif
}

Readline* MaybeGetReadline() {
  // TODO: incorporate OIL_READLINE into the build config
#ifdef HAVE_READLINE
  gReadline = Alloc<Readline>();
  gHeap.RootGlobalVar(gReadline);
  return gReadline;
#else
  return nullptr;
#endif
}

}  // namespace py_readline
