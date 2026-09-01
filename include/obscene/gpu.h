/*
 * GPU compute: run a shader on the device, read back what it computed.
 *
 * # What this is for, and why it is the oracle
 *
 * The behavioural sections ask what a *function* returns. This asks what an *instruction*
 * computes - dispatch a compute shader over known inputs and read the result bits back.
 * That is the only way to answer "what does this GPU actually do", which is the question an
 * emulator's shader translation must be checked against and cannot answer about itself.
 *
 * On RDNA2 silicon (a Steam Deck's gfx1033, eventually a console) the read-back bits are
 * ground truth. On a software rasteriser (llvmpipe, in the build VM) they prove the
 * pipeline is correct but are *not* RDNA2 - which is exactly why every result carries the
 * device name (`obs_gpu_device_name`): a result measured on llvmpipe must never be read as
 * hardware, the same discipline the rest of the corpus keeps about provenance.
 *
 * # The split, same as net and sink
 *
 * The interface is shared; the backend is chosen by which file the build compiles.
 * `gpu_vulkan.c` (host and the Deck's native build) talks Vulkan compute - standard,
 * public, no vendor anything. `gpu_gnm.c` (the console module) is the vendor path, and
 * refuses until its submission format is confirmed, the way `net_target.c` began.
 *
 * # The primitive is deliberately one call
 *
 * One storage buffer, in place: the caller fills `data` with inputs, `obs_gpu_run`
 * dispatches the shader over it, and `data` holds the results on return. A single
 * in/out buffer is all a per-instruction probe needs, and a small primitive keeps the
 * thing generic - the specificity lives in the shader, which is where it belongs.
 */

#ifndef OBSCENE_GPU_H
#define OBSCENE_GPU_H

#include <stddef.h>
#include <stdint.h>

/* Whether this build has a working GPU backend.
 *
 * Not a constant: the Vulkan backend answers it by actually bringing up an instance and a
 * device, so a build on a machine with no usable driver reports absent rather than
 * faulting mid-run. The console stub answers it with a flat no. Either way a section gates
 * on this before dispatching anything. */
int obs_gpu_backend_available(void);

/* The device the results came from - "llvmpipe (LLVM 20.1.2)", "AMD Radeon ... gfx1033".
 *
 * This is the provenance that keeps a software result from being mistaken for silicon. It
 * lands on every GPU record. Returns a stable string, or "none" before the backend is up. */
const char *obs_gpu_device_name(void);

/* The transport-style name of the backend: "vulkan" or "none". */
const char *obs_gpu_backend_name(void);

/* The device type as a stable word: "integrated", "discrete", "cpu", "virtual", "other",
 * or "none". This is the machine-gradable half of provenance - a consumer rejects a `cpu`
 * result (llvmpipe) for a hardware claim without having to recognise device-name strings.
 * The Deck's APU reports `integrated`. */
const char *obs_gpu_device_type(void);

/* Dispatch `spirv` over `data` in place: `count` 32-bit lanes in, results out.
 *
 * `spirv_bytes` is the SPIR-V size in bytes (a multiple of four). The shader binds one
 * std430 storage buffer at binding 0 and is dispatched over `ceil(count / 64)` groups of
 * 64, so it must bounds-check against the buffer length.
 *
 * Returns 0 on success, negative on any failure. Data is treated as raw 32-bit words - the
 * caller decides whether they are floats or ints, because the GPU does too. */
int obs_gpu_run(const uint32_t *spirv, size_t spirv_bytes, uint32_t *data, unsigned int count);

/* ---- dispatch a compiled-in kernel by name ------------------------------------------
 *
 * For the `gpu` command over the socket: a driver names one of the embedded kernels and
 * supplies operands, and the probe runs it - no rebuild per question, which is the point of
 * an interactive mode. Only *named, compiled-in* shaders can be run this way, so unlike
 * `call` or `blob` it executes nothing the build did not already contain.
 *
 * Defined only under OBS_GPU; the socket's `gpu` verb is compiled under the same guard, so
 * an ordinary build references neither. */

/* The input count a kernel takes per lane, or 0 if the name is unknown. */
unsigned int obs_gpu_arity(const char *name);

/* Dispatch the named kernel over `data` in place, exactly as `obs_gpu_run` would. Returns
 * 0 on success, negative if the name is unknown or the dispatch failed. */
int obs_gpu_dispatch_named(const char *name, uint32_t *data, unsigned int count);

#endif /* OBSCENE_GPU_H */
