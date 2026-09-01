# 0b. Reading a report out of an emulator - done


Resolved, and the earlier conclusion here was wrong. "No emulator implements a write
path" was not true: the write path was there and we were asking for it in a way no
loader could match, because every import was typed `STT_NOTYPE` (D039). One emulator now
prints the whole report on its own hardware channel, and the module also draws it to a
framebuffer (D041) so a run says something even where no write works.

