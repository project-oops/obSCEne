import struct

def get_stream(name):
    stream = bytearray()
    with open("reports/hardware/20260917-075641-eboot.obs.log") as f:
        for line in f:
            if "dcb-stream" in line and name in line:
                parts = line.strip().split("|")
                stream.extend(bytes.fromhex(parts[-1]))
    return stream

s_good = get_stream("166-agc/primitive-draw|sceAgcDriverSubmitDcb")
s_p3 = get_stream("166-agc/primitive-draw-param3|primary")

print(f"good len: {len(s_good)}, p3 len: {len(s_p3)}")

dw_good = struct.unpack(f"<{len(s_good)//4}I", s_good)
dw_p3 = struct.unpack(f"<{len(s_p3)//4}I", s_p3)

diffs = []
for i in range(min(len(dw_good), len(dw_p3))):
    if dw_good[i] != dw_p3[i]:
        diffs.append((i, dw_good[i], dw_p3[i]))

print(f"Total diffs: {len(diffs)}")
for idx, g, p in diffs:
    print(f"[{idx:3d}] (0x{idx*4:03x}) good: 0x{g:08x}  param3: 0x{p:08x}")
