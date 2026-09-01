/* First output on real hardware: bootstrap sceKernelWrite from getpid and call it.
 *
 * elfldr passes getpid (a resolved libkernel_sys export) as word 0 of payload_args and
 * resolves nothing else. libkernel is callable but not readable from the sandbox, so
 * the export table cannot be walked at runtime - but the vaddrs are known from the
 * real 12.40 libkernel_sys.sprx, pulled over FTP and read with selfish (no SDK, real
 * file as oracle):
 *
 *   getpid          vaddr 0x005b0   (word0 runtime confirms base = word0 - 0x5b0)
 *   sceKernelWrite  vaddr 0x16e00
 *
 * So sceKernelWrite_runtime = (word0 - 0x5b0) + 0x16e00. elfldr dup'd the loader socket
 * onto fds 1 and 2, so a write there reaches the sender.
 */
void obscene_start(void);
typedef long (*write_t)(int, const void *, unsigned long);

void obscene_start(void) {
    unsigned long arg0;
    __asm__ volatile("mov %%rdi, %0" : "=r"(arg0));
    unsigned long getpid_addr = ((unsigned long *)arg0)[0];
    unsigned long base = getpid_addr - 0x5b0UL;
    write_t kwrite = (write_t)(base + 0x16e00UL);

    static const char msg[] = "OBSCENE ON PS5 FW12.40 -- first hardware output\n";
    kwrite(1, msg, sizeof msg - 1);
    kwrite(2, msg, sizeof msg - 1);

    /* int3 so the signal confirms we reached here even if the socket read races. */
    __asm__ volatile("int3");
}
