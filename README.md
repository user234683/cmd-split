# cmd-split

Highly experimental rethinking of terminal emulators, written in C

## Features
- Input is entered into a box separate from where output is displayed. This box functions entirely like a normal text editor. Copy and pasting, editing, and mouse selection without hassle.
- Multiple lines of input can be entered at a time - handy for things like REPLs. For instance, this allows copy and pasting python code with multiple statements and running all of it interactively.

## Limitations
- No support at all for console API functions.
- No color support (ability to have separate colors for stdout and stderr is planned however)
- No terminal escape sequences or other special sequences. For instance, running cls will literally print the control character that normally tells the windows console to clear the screen.
- Does not play nice with programs which assume a normal console. Even trying to use the python interactive interpreter is broken because it assumes that input is mixed with the output window and stays in the output window.
- ASCII-only at the moment, but this will be fixed
- Windows-only right now
- No command history, but this is also planned
- No text-wrapping. This might be added.
- No selecting the output. But this is the next feature to be added.

There's different incompatible directions I'd like to take this project. One is as a replacement for using IDLE to run python code - that is, something that supports sensible copy and pasting like IDLE does, but without the performance issues and lack of command history. Another is to make it support the console API, escape sequences, and make it suitable for serious use as a replacement for conhost and other terminal emulators. This would likely mean giving up on splitting the input and output.

## Compiling
Compiling is extremely simple. Simply do `gcc cmd-split.c`, `tcc cmd-split.c`, or whichever compiler you use. No dependencies (besides the windows api). No makefiles or any of that crap.

## Usage
Just download the zip and run `cmd-split.exe`. If you have python installed you can issue `python repl.py` to use python interactively.