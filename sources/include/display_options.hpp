#pragma once

struct display_options_t
{
  enum e_line_mode {
    LINE_NO_COLOR = 0,
    LINE_ENTRY,
    LINE_INDENT,
    LINE_EOL,
    LINE_NUMBER,
  };

  bool        show_grid     = false;
  bool        show_segments = false;
  bool        show_ws       = false;
  bool        show_layout   = true;
  e_line_mode show_lines    = LINE_ENTRY;
};
