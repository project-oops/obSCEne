/* obSCEne report body. Uses ordinary imports; the selfish crt0 resolves them on-device.
 */
__attribute__((weak)) long sceKernelWrite(int, const void *, unsigned long);
__attribute__((weak)) long sceKernelGetProcessTime(void);
__attribute__((weak)) unsigned long sceKernelGetTscFrequency(void);
__attribute__((weak)) int sceKernelGetSystemSwVersion(void *);

static void emit(const char *s) {
    unsigned long n = 0;
    while (s[n])
        n++;
    sceKernelWrite(1, s, n);
}
static void emit_u(unsigned long v) {
    char b[21];
    int i = 20;
    b[i--] = 0;
    if (!v)
        b[i--] = '0';
    while (v) {
        b[i--] = (char)('0' + v % 10);
        v /= 10;
    }
    emit(&b[i + 1]);
}
static void emit_hex(unsigned long v) {
    static const char d[] = "0123456789abcdef";
    char b[17];
    for (int i = 0; i < 16; i++)
        b[15 - i] = d[(v >> (i * 4)) & 0xF];
    b[16] = 0;
    emit("0x");
    emit(b);
}

void obs_payload_main(void);
void obs_payload_main(void) {
    emit("OBS|meta|1|hardware|selfish\n");
    emit("OBS|build|dev|payload\n");
    emit("OBS|sink|socket\n");

    emit("OBS|try|000-hw/tsc-frequency|libkernel|sceKernelGetTscFrequency\n");
    emit("OBS|measure|000-hw/tsc-frequency|sceKernelGetTscFrequency|hz|");
    emit_u(sceKernelGetTscFrequency());
    emit("|hz\n");
    emit("OBS|res|000-hw/tsc-frequency|pass|||hardware\n");

    unsigned long t0 = (unsigned long)sceKernelGetProcessTime();
    for (volatile int i = 0; i < 1000000; i++) {
    }
    unsigned long t1 = (unsigned long)sceKernelGetProcessTime();
    emit("OBS|measure|000-hw/proc-time|sceKernelGetProcessTime|delta-us|");
    emit_u(t1 - t0);
    emit("|us\n");
    emit("OBS|res|000-hw/proc-time|pass|||hardware\n");

    unsigned char ver[48];
    for (int i = 0; i < 48; i++)
        ver[i] = 0xC7;
    sceKernelGetSystemSwVersion(ver);
    for (int off = 0; off < 48; off += 8) {
        unsigned long w = 0;
        for (int b = 0; b < 8; b++)
            w |= (unsigned long)ver[off + b] << (b * 8);
        emit("OBS|bytes|000-hw/sw-version|sceKernelGetSystemSwVersion|ver|");
        emit_u((unsigned long)off);
        emit("|");
        emit_hex(w);
        emit("\n");
    }
    emit("OBS|res|000-hw/sw-version|pass|||hardware\n");
    emit("OBS|end|sceKernelWrite\n");
}
