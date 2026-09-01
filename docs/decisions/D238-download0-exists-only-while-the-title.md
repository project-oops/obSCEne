# D238 - `/download0` exists only while the title does


*status: measured*

The report is written to `/download0/obscene-report.txt`, which the platform mounts at
`/mnt/sandbox/download/<TITLEID>` outside the sandbox. **That mount is torn down when the title
exits.** A run that completes leaves no file behind to fetch; listing the directory afterwards
gives `No such file or directory`, and so does the directory above it.

Measured directly: `ls /mnt/sandbox/download/OBSC00001` listed both `obscene-boot.txt` and
`obscene-report.txt` while the suite was running, and the whole path was gone a minute later.

So retrieval is not something done after a run - it is something done *during* one, or through a
channel that leaves the sandbox as the run proceeds. The system log is that channel (D233), and
this is the second reason it is written unconditionally rather than chosen between: it is the
only destination whose output survives the process that produced it.

