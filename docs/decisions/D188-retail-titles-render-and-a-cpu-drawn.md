# D188 - Retail titles render and a CPU-drawn framebuffer does not, and the difference is who wrote the pixels


The question was put plainly and repeatedly: *"it makes no sense that Kyty can launch retail
games but not our code."* It does not, and every explanation offered before this one described
a mechanism without answering it. The answer is four lines of `GpuMemory::Update`:

```cpp
bool mem_watch = false;                                    // hardcoded off
if ((mem_watch && o.cpu_update_time > o.gpu_update_time)
    || (!mem_watch && submit_id > o.submit_id))            // only this applies
```

A registered framebuffer is re-read from guest memory only when `submit_id` advances, and
`submit_id` counts **GPU submissions**. A title rendering through the graphics driver submits
constantly, so its buffer is re-hashed every frame, seen to have changed, and uploaded.

obSCEne draws with the CPU and calls flip. It submits nothing. The counter never moves,
`update_func` is never reached, and the Vulkan image keeps what it held at creation - which is
nothing. **Black window, every call succeeding.**

So it was never homebrew-versus-retail. It is GPU-written-versus-CPU-written, and the loader's
own `mem_watch` flag is the intended answer to exactly this case, switched off.

### The fix, and why it is small

`UINT64_MAX` is already reserved by that machinery for "check regardless": it passes the
comparison, and `Update` explicitly declines to record it as the new submit id. So asking for
the same object again at flip time finds the existing one, forces the hash, sees the writes and
uploads - creating nothing and costing one hash per flip.

The only work was keeping the image's description alive: it lived on the stack of
`register_buffers_internal` and was gone by the time anything flipped.

### Result

27 of 27 sections, 515 of 515 checks, `SUITE COMPLETE` on screen, at `frame: 29`. Kyty now
renders obSCEne's report, and **all five loaders in the toolkit display it**.

### What this nearly cost

Two turns before this, the recommendation here was to stop - on the grounds that three loaders
already render the report and a fourth picture adds nothing. That reasoning was sound about
*value* and wrong about *cost*: the remaining work was one afternoon's reading of a function
already open in front of me, and the finding underneath it is worth more than the screenshot.
A contradiction that survives every explanation - *retail works, ours does not* - is evidence
the explanations are wrong, not evidence the thing is hard. It was pushed on until it gave, and
that was the right call.

Status: **decided** - measured, and the mechanism is quotable from source.

