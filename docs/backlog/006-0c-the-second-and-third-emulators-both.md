# 0c. The second and third emulators - both now run, one still resolves nothing


Rewritten. The previous version of this entry said Kyty's output was unreachable and
concluded "nothing to fix here". That was wrong, and it stopped anybody looking.

**Kyty** runs the module and binds real functions. Its C `stdout` is not the handle a
parent process redirects - true - but the binary takes `--printf-direction File
--printf-output-file`, so the report is reachable and always was. `scripts/run-kyty.sh`
drives it.

Its real value turned out to be different: it patches each unresolved import individually
and names it, so its log lists 238 functions it does not implement, including all 63 of
`libScePosix`. shadPS4 stubs everything and so reports none. `obscene-tool unresolved` turns
that log into names - 238 of 238 (D061).

**craziiEmu** builds from source with dotnet and loads the module. It reported
`Generation: Gen4` for a current-generation probe until `e_ident[EI_ABIVERSION]` was set,
and now reports Gen5 with one relocation descriptor where it previously found none - but
still `HasImportMetadata: False`, so it resolves nothing yet. Its constants carry eleven
vendor tags, every one describing a table, and none of the four that say who a module is.

**Still not run:** fpPS4, GPCS4, rpcsx, obliteration, orbital, SharpEMU. fpPS4 needs Free
Pascal 3.3.1 trunk plus Lazarus; its 3 MB release binary is tagged 2022 against an actively
developed tree. Its `ps4libdoc.pas` was worth more than running it would have been.

