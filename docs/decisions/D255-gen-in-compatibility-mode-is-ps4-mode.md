# D255 - GEN in compatibility mode is ps4_mode, not unknown or gen4


*status: measured*

Running on a PS5 as a `ps4_game` title means the current-generation AGC driver is inaccessible (`obs_current_present()` cannot look), but reporting `unknown` or `gen4` misidentifies the execution context. The title runs inside PS4 compatibility mode (`ps4_mode`).

