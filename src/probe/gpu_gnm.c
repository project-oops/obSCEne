/*
 * The GPU backend on the console - deliberately not implemented yet.
 *
 * The console submits compute through libSceGnm (PS4) or libSceAgc (PS5): allocate GPU
 * memory, supply a shader binary, build a PM4 command buffer, submit, wait, read back. The
 * signatures and the command-buffer format are only partly public - shadPS4's source has
 * them, but OpenOrbis's headers are mostly stubs for the compute path and no public sample
 * dispatches compute - so much of it would be one source, not the two that let the socket
 * backend be written with confidence. D008 says not to guess at a submission format on a
 * target that took effort to reach.
 *
 * So this refuses, the way `net_target.c` refused before its signatures were confirmed.
 * `obs_gpu_backend_available` returns zero, the GPU section reports a skip with the reason,
 * and nothing announces a capability it cannot honour.
 *
 * The Vulkan backend (`gpu_vulkan.c`) is where the shader-execution oracle is built and
 * proven - on the Deck's RDNA2 that is ground truth already. This file is the console's
 * eventual second path to the same kernels, gated on confirming the Gnm/Agc submission
 * format and on whether GPU access is even reachable from an unsigned module.
 */

#include "obscene/gpu.h"

int obs_gpu_backend_available(void) {
    return 0;
}

const char *obs_gpu_device_name(void) {
    return "none";
}

const char *obs_gpu_backend_name(void) {
    return "none";
}

const char *obs_gpu_device_type(void) {
    return "none";
}

int obs_gpu_run(const uint32_t *spirv, size_t spirv_bytes, uint32_t *data,
                unsigned int count) {
    (void)spirv;
    (void)spirv_bytes;
    (void)data;
    (void)count;
    return -1;
}
