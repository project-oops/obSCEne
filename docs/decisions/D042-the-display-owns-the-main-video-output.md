# D042 - The display owns the main video output, and the video checks yield to it


Status: decided, on evidence.

`080-video/open` opens the main output and hands it straight back, on the principle that
this program leaves the platform as it found it. With the display live, that closes the
output the report is being drawn on: the registration goes with it, every later flip is
refused, and one emulator answered the second refused flip by faulting inside its own
presenter thread. The run went from 511 records to 155.

Both checks now ask `obs_display_holds_output()` and skip with the reason. Opening the
display already proved the output opens, so nothing is lost but the duplication.

`obs_display_flip` also stops permanently after a refused flip. A lost screen should
cost a screen, not a run, and repeating a call that a platform has already refused is
how a diagnostic turns into a crash.

