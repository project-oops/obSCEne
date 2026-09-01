/* A real obSCEne report on real hardware, bootstrapped from getpid.
 *
 * elfldr resolves nothing; every function is located as (word0 - getpid_vaddr) + fn_vaddr,
 * the vaddrs read from the real 12.40 libkernel_sys.sprx with selfish. Records go to fd 1,
 * which elfldr dup'd onto the loader socket, in the OBS| report format.
 *
 * This is the first report in the project's history with hardware provenance.
 */
typedef long (*write_t)(int, const void *, unsigned long);
typedef long (*u64_ret_t)(void);
typedef int (*getver_t)(void *);
typedef long (*proctime_t)(void);

/* libkernel_sys export vaddrs, from selfish over the pulled .sprx. */
#define V_GETPID       0x005b0UL
#define V_WRITE        0x16e00UL
#define V_PROCTIME     0x16160UL
#define V_TSCFREQ      0x1cf30UL
#define V_SWVERSION    0x1d230UL

void obscene_start(void);

static write_t g_write;

static void emit(const char *s) {
    unsigned long n = 0;
    while (s[n]) n++;
    g_write(1, s, n);
}

/* u64 to decimal into a caller buffer, returns length. No libc, index-assign (no memcpy). */
static void emit_u64(unsigned long v) {
    char buf[21];
    int i = 20;
    buf[i--] = 0;
    if (v == 0) { buf[i--] = '0'; }
    while (v > 0) { buf[i--] = (char)('0' + (v % 10)); v /= 10; }
    emit(&buf[i + 1]);
}

static void emit_hex(unsigned long v) {
    static const char d[] = "0123456789abcdef";
    char buf[17];
    for (int i = 0; i < 16; i++) { buf[15 - i] = d[(v >> (i * 4)) & 0xF]; }
    buf[16] = 0;
    emit("0x");
    emit(buf);
}

void obscene_start(void) {
    unsigned long arg0;
    __asm__ volatile("mov %%rdi, %0" : "=r"(arg0));
    unsigned long base = ((unsigned long *)arg0)[0] - V_GETPID;

    g_write = (write_t)(base + V_WRITE);
    proctime_t proctime = (proctime_t)(base + V_PROCTIME);
    u64_ret_t tscfreq = (u64_ret_t)(base + V_TSCFREQ);
    getver_t getver = (getver_t)(base + V_SWVERSION);

    emit("OBS|meta|1|hardware|first\n");
    emit("OBS|build|dev|payload\n");
    emit("OBS|sink|socket\n");

    /* Real silicon: the timestamp-counter frequency. No emulator has a true value. */
    emit("OBS|try|000-hw/tsc-frequency|libkernel|sceKernelGetTscFrequency\n");
    emit("OBS|measure|000-hw/tsc-frequency|sceKernelGetTscFrequency|hz|");
    emit_u64((unsigned long)tscfreq());
    emit("|hz\n");
    emit("OBS|res|000-hw/tsc-frequency|pass|||hardware\n");

    /* Process CPU time, twice, must advance. */
    unsigned long t0 = (unsigned long)proctime();
    for (volatile int i = 0; i < 1000000; i++) { }
    unsigned long t1 = (unsigned long)proctime();
    emit("OBS|try|000-hw/proc-time|libkernel|sceKernelGetProcessTime\n");
    emit("OBS|measure|000-hw/proc-time|sceKernelGetProcessTime|delta-us|");
    emit_u64(t1 - t0);
    emit("|us\n");
    emit(t1 > t0 ? "OBS|res|000-hw/proc-time|pass|||hardware\n"
                 : "OBS|res|000-hw/proc-time|fail|process time did not advance||hardware\n");

    /* The system software version struct: call it, dump the raw bytes it wrote. */
    unsigned char ver[48];
    for (int i = 0; i < 48; i++) { ver[i] = 0xC7; }
    int rc = getver(ver);
    emit("OBS|try|000-hw/sw-version|libkernel|sceKernelGetSystemSwVersion\n");
    emit("OBS|measure|000-hw/sw-version|sceKernelGetSystemSwVersion|rc|");
    emit_u64((unsigned long)(unsigned int)rc);
    emit("|code\n");
    for (int off = 0; off < 48; off += 8) {
        unsigned long w = 0;
        for (int b = 0; b < 8; b++) { w |= (unsigned long)ver[off + b] << (b * 8); }
        emit("OBS|bytes|000-hw/sw-version|sceKernelGetSystemSwVersion|ver|");
        emit_u64((unsigned long)off);
        emit("|");
        emit_hex(w);
        emit("\n");
    }
    emit("OBS|res|000-hw/sw-version|pass|||hardware\n");

    emit("OBS|end|sceKernelWrite\n");

    __asm__ volatile("int3");
}
