import struct

def get_bytes(name, field):
    stream = bytearray()
    with open("reports/hardware/20260917-075641-eboot.obs.log") as f:
        for line in f:
            if field in line and name in line:
                parts = line.strip().split("|")
                stream.extend(bytes.fromhex(parts[-1]))
    return stream

vs_good = get_bytes("166-agc/primitive-draw|sceAgcDriverSubmitDcb", "vs-bytecode")
vs_p3 = get_bytes("166-agc/primitive-draw-param3|primary", "vs-bytecode")
ps_good = get_bytes("166-agc/primitive-draw|sceAgcDriverSubmitDcb", "ps-bytecode")
ps_p3 = get_bytes("166-agc/primitive-draw-param3|primary", "ps-bytecode")

print(f"vs_good len: {len(vs_good)}, vs_p3 len: {len(vs_p3)}")
print(f"ps_good len: {len(ps_good)}, ps_p3 len: {len(ps_p3)}")

def dump_dwords(name, b):
    print(f"--- {name} ({len(b)//4} dwords) ---")
    dws = struct.unpack(f"<{len(b)//4}I", b)
    for i, dw in enumerate(dws):
        print(f"  [{i:2d}] 0x{dw:08x}")

dump_dwords("vs_good", vs_good)
dump_dwords("vs_p3", vs_p3)
dump_dwords("ps_good", ps_good)
dump_dwords("ps_p3", ps_p3)
