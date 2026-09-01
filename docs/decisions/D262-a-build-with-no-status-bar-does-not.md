# D262 - A build with no status bar does not gather what would fill one


*status: decided*

`obs_sysinfo_report()` runs in every build, including the elfldr payload, which draws no
header. Gathering the status fields there is not just wasted: some getters touch subsystems a
title sets up and a bare payload does not. The IP field calls `sceNetCtlInit`/`sceNetCtlGetInfo`,
and running that in an elfldr payload - where the network is not brought up the way it is for a
title - is a plausible way to wedge the process. A payload build carrying that code, launched
while another run was being killed, is the most likely cause of a console wedge that then cost a
recovery.

So the payload build defines `OBS_NO_UI` and `obs_sysinfo_report()` returns immediately under
it. The function still exists and is still called; it simply gathers nothing. The HUD path
(`screen.c`) is already gated at run time by `obs_live`, which a payload never sets, so no
status getter runs in a headless build from either direction.

The gate is the *build shape*, not a run-time check, because the acquisition happens before the
display is even attempted (`start.c`) - there is nothing to test at run time that early, and the
fact that settles it, "this build has no UI", is known at compile time.

