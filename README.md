## bdfedit

A terminal-based, mouse-driven BDF font editor.

Capable of reading, writing, and editing bitmap font files fully within the terminal, and entirely with the mouse.

This is an application developed in parallel with my TUI library [ui.h](https://github.com/Cubified/bdfedit/blob/main/ui.h).

### Demo

![demo.gif](https://github.com/Cubified/bdfedit/blob/main/demo.gif)

### Compiling and Running

To compile:

     $ make

To run:

     $ ./bdfedit [font.bdf]

A sample .bdf file is included in `bdf/scientifica-11.bdf`.

### Libraries/Sample Font

- [uthash](https://troydhanson.github.io/uthash/)
- [vec](https://github.com/rxi/vec)
- [scientifica](https://github.com/nerdypepper/scientifica)
