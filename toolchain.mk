# The compiler pin, enforced rather than described.
#
# Included by this repository's `Makefile` before anything is compiled. It exists because the
# pin used to be ambient: every build file here said bare `CC := clang`, and which compiler
# that was depended on where you were standing. Under WSL `oops-builder` it was clang 21;
# under Docker `silkeh/clang:18` it was clang 18. Two runners, one spelling, two compilers,
# and nothing anywhere said which was correct. (oops-mesa#D013)
#
# A pin that is a sentence in a document is not a pin. This file is the same statement in a
# form the build cannot proceed past.
#
# # What is checked, and what deliberately is not
#
# The **major** version, because that is what the pin is about: libc++ 21's headers use
# compiler-internal constructs that a clang older than itself does not implement, which is the
# whole reason the collection moved (oops-mesa#D013, oops-mesa#D006). A patch-level difference
# between two runners is not that problem and refusing it would make the guard a nuisance
# nobody keeps.
#
# `-dumpversion` is the spelling every clang answers identically. The first line of
# `--version` carries a vendor prefix that differs between the Debian container
# ("Debian clang version 21.1.8") and Ubuntu's apt build ("Ubuntu clang version 21.1.8"),
# so parsing that would compare packaging rather than compilers.
#
# # Why the error and not a warning
#
# CONVENTIONS section 3: a wrong answer costs more than no answer. A build that silently used
# the other compiler is exactly the failure this is here to end, and it is invisible in the
# output - the objects compile, the link succeeds, and the difference shows up as a C++
# standard library that will not build or a fixture that churns.

OOPS_CLANG_MAJOR := 21

# `clean` and `help` must work on a machine with no compiler at all: a guard that stops you
# tidying up is one people route around, and a routed-around guard is not a guard. Every goal
# that actually compiles something is checked.
OOPS_TOOLCHAIN_SKIP_GOALS := clean distclean help

ifeq ($(filter $(OOPS_TOOLCHAIN_SKIP_GOALS),$(MAKECMDGOALS)),)

# `$(CC)` may carry a cache prefix - `make CC="sccache clang"` is a documented use here - so
# the version is asked of the last word rather than of the whole variable.
OOPS_CC_BIN  := $(lastword $(CC))
OOPS_CC_FULL := $(shell $(OOPS_CC_BIN) -dumpversion 2>/dev/null)
OOPS_CC_MAJOR := $(firstword $(subst ., ,$(OOPS_CC_FULL)))

ifeq ($(OOPS_CC_FULL),)
$(error toolchain: `$(OOPS_CC_BIN)` is not runnable, so the compiler version cannot be \
checked. The collection pins clang $(OOPS_CLANG_MAJOR); see oops-mesa#D013 for the runners that \
carry it)
endif

ifneq ($(OOPS_CC_MAJOR),$(OOPS_CLANG_MAJOR))
$(error toolchain: this build is pinned to clang $(OOPS_CLANG_MAJOR) and `$(OOPS_CC_BIN)` is \
$(OOPS_CC_FULL). The two runners that carry the pin are WSL `oops-builder` and the container \
`silkeh/clang:$(OOPS_CLANG_MAJOR)`; see oops-mesa#D013. Do not work around this by passing CC - the \
split between runners is the defect it was written for)
endif

endif
