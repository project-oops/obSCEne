# D072 - `obscene-tool consensus`: agreement between implementations, used where hardware is not available


Status: decided, from BACKLOG §12.

Nothing carries `hardware`, so when obSCEne and an emulator disagree the report cannot say
which is wrong, and `fail [assumed]` is a verdict the recipient can reasonably decline.

Several implementations agreeing is an oracle that needs no console. "You are the only one
of four that fails this" is actionable in a way "you failed check X" is not.

**A skip is not an opinion, and getting that wrong made the tool useless.** The first run
reported 80 disagreements out of 126 - almost all of them one platform having a function
and another not. True, and enough noise to bury the handful of real behavioural
differences. Skips are now counted separately as *no opinion*, and a check fewer than two
implementations attempted is not comparable. That took it to 61 genuine differences.

**It names implementations and never counts them.** These projects read each other's
source, so four agreeing is not four witnesses. This project already made exactly that
mistake while writing up how well corroborated something was (D064), and a bare number
would hide the correlation a reader needs to discount.

**OUTLIER and SPLIT are different findings.** One implementation differing from a
unanimous rest is actionable. An even split means the behaviour is genuinely unsettled
between implementations, which is a question for hardware rather than a bug report - and
saying otherwise would be the tool inventing a majority.

**The host build turns out to be the most useful second opinion available today.** It is a
real implementation of the POSIX and C library surface, so disagreement against it is
disagreement with something known to work. The prerequisite the backlog named - a second
*emulator* producing reports - is still outstanding, but it was never the only way to get
a second opinion.

