# 6. Build speed over the mount


`clang-format` and `make` both cross the Windows share for every file, and a full
`make check` takes minutes. Copying the tree into the VM, building there, and copying
back would cut it substantially. Only worth doing when it starts costing more than it
saves - but it is already noticeable.

