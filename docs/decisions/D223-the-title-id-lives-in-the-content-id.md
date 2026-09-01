# D223 - The title id lives in the content id, and a fresh one routes around a stuck title


`build-pkg.sh` took `CONTENT_ID` from the environment and then passed a **hardcoded**
`--title-id "OBSC00001"` to the packer. Two copies of one fact, and only one of them moved: an
override produced a package whose licence was keyed to one title and whose `param.sfo` declared
another, which a console rejects for a reason unrelated to whatever was being tested. The id is
now extracted from the content id, with a shape check.

The immediate use is recovery. A crashed process the console will not reap holds its title id:

```text
461  ...  STOP  4018  OBSC00001  ...  eboot.bin
[SceLncService] checkExistingApp: LNC_ISOK::0x8094000c
```

and from then on every install against that id fetches the package header, refuses to overwrite,
and never asks for the body - which reads exactly like a malformed package. `kill` will not
touch it; the crash-reporting machinery has it stopped. Building under a fresh id sidesteps the
whole thing, and on a jailbroken console that is the difference between one command and an hour
of re-exploiting.

**Check `ps` for a stopped `eboot.bin` before concluding anything about a package that will not
install.** One range request and no body is the tell.

