#!/bin/bash
export PATH="$HOME/.cargo/bin:$PATH"
BIN="$HOME/obs-tool-target/debug/obscene-tool"
$BIN hw pull "$1" --into "$HOME/$(basename $1)" 2>&1 | tail -1
ls -la "$HOME/$(basename $1)" 2>/dev/null | awk '{print $5, $9}'
