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

// Assuming readline 4.0+
#if HAVE_READLINE

#define completion_matches(x, y) \
    rl_completion_matches((x), ((rl_compentry_func_t *)(y)))

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

static char** completion_handler(const char* text, int start, int end) {
  rl_completion_append_character = '\0';
  rl_completion_suppress_append = 0;
  gReadline->begidx_ = start;
  gReadline->endidx_ = end;
  return completion_matches(text, *do_complete);
}

static void
display_matches_hook(char **matches, int num_matches, int max_length) {
  if (gReadline->display_ == nullptr) {
    return;
  }
  auto* gc_matches = Alloc<List<Str*>>();
  for (int i = 0; i < num_matches; i++) {
    gc_matches->append(StrFromC(matches[i]));
  }
  comp_ui::ExecutePrintCandidates(gReadline->display_, nullptr, gc_matches, max_length);
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
  using_history();
  rl_readline_name = "oil";
  /* Force rebind of TAB to insert-tab */
  rl_bind_key('\t', rl_insert);
  /* Bind both ESC-TAB and ESC-ESC to the completion function */
  rl_bind_key_in_map('\t', rl_complete, emacs_meta_keymap);
  rl_bind_key_in_map('\033', rl_complete, emacs_meta_keymap);
  rl_attempted_completion_function = completion_handler;
  rl_completion_display_matches_hook = display_matches_hook;
  rl_catch_signals = 0;
  rl_catch_sigwinch = 0;
  rl_initialize();
#else
  assert(0);  // not implemented
#endif
}

void Readline::parse_and_bind(Str* s) {
#if HAVE_READLINE
  // Make a copy -- rl_parse_and_bind() modifies its argument
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
  char* p = nullptr;
  if (path != nullptr) {
    p = path->data();
  }
  read_history(p);
#else
  assert(0);  // not implemented
#endif
}

void Readline::write_history_file(Str* path) {
#if HAVE_READLINE
  char* p = nullptr;
  if (path != nullptr) {
    p = path->data();
  }
  write_history(p);
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
  histdata_t data = free_history_entry(entry);
  free(data);
#else
  assert(0);  // not implemented
#endif
}

Str* Readline::get_history_item(int pos) {
#if HAVE_READLINE
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
