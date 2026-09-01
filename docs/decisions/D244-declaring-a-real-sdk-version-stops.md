# D244 - Declaring a real SDK version stops `sceKernelDlsym` answering


*status: measured*

Having corrected `sdk_version` to the value a launching title declares (D242), the correction
was measured rather than assumed. Two packages built from identical sources with one flag
differing, run on the same console:

| | `PROC_SDK=0` | `PROC_SDK=0x08008011` |
|---|---|---|
| imports linked | - | **no difference** |
| symbols resolvable at run time | 105 of 122 | **0 of 173** |
| suite tally | 218/74/190/40 | 117/6/363/41 |

**Import binding did not change at all.** Ninety-eight symbols were measured in both arms and
not one moved between linked and unlinked, so the SDK version does not gate it - which was the
hypothesis the change was made on, and it was wrong.

What it does gate is run-time module resolution. At `0x08008011` `sceKernelLoadStartModule`
still returns a handle and `sceKernelDlsym` returns nothing for every symbol, so the census
measures nothing and three hundred and sixty checks fail.

Zero is kept. It is now the measured choice rather than the inherited one, and `PROC_SDK`
exists so the experiment is one word rather than an edit - this is one console on one day, and
a firmware that changes it should be cheap to find.

**The order mattered.** Had the flag not existed, the census collapse would have arrived
together with four other changes and been attributed to any of them.

