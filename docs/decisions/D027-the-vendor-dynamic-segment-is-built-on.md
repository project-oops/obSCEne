# D027 - The vendor dynamic segment is built on every module build, not behind a flag


Status: decided.

`mkmodule --dynlib` was a flag from when the segment was an experiment. A loader
ignores every standard dynamic tag, so a module without the vendor segment loads and
resolves nothing - the flag's only remaining power was to build something broken. It
is gone; `mkmodule` always builds the segment.

