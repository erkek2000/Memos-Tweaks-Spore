#!/usr/bin/env python3
"""Verify PE section mapping for SporeApp.exe: dump bytes at known-good code
addresses (entry point, a symbol from additional_march2017.txt) and at the
crash site, to confirm RVA->file offset mapping is correct."""
import struct
import sys

path = r"C:\Program Files (x86)\Steam\steamapps\common\spore 24720\SporebinEP1\SporeApp.exe"
data = open(path, "rb").read()
pe_off = struct.unpack_from("<I", data, 0x3C)[0]
num_sec = struct.unpack_from("<H", data, pe_off + 6)[0]
opt_size = struct.unpack_from("<H", data, pe_off + 20)[0]
opt_off = pe_off + 24
entry_rva = struct.unpack_from("<I", data, opt_off + 16)[0]
image_base = struct.unpack_from("<I", data, opt_off + 28)[0]
print("image_base = 0x%08X, entry RVA = 0x%08X" % (image_base, entry_rva))
sec_off = opt_off + opt_size
secs = []
for i in range(num_sec):
    o = sec_off + i * 40
    name = data[o:o + 8].rstrip(b"\0").decode()
    vs = struct.unpack_from("<I", data, o + 8)[0]
    va = struct.unpack_from("<I", data, o + 12)[0]
    rs = struct.unpack_from("<I", data, o + 16)[0]
    rp = struct.unpack_from("<I", data, o + 20)[0]
    secs.append((name, va, vs, rp, rs))
    print("%-8s VA=0x%08X VS=0x%08X RP=0x%08X RS=0x%08X" % (name, va, vs, rp, rs))

def dump(rva, n, label):
    for name, va, vs, rp, rs in secs:
        if va <= rva < va + max(vs, rs):
            off = rp + (rva - va)
            b = data[off:off + n]
            print("%-18s rva 0x%08X -> file 0x%08X: %s" % (label, rva, off, " ".join("%02X" % x for x in b)))
            return
    print("%-18s rva 0x%08X: not in any section" % (label, rva))

dump(entry_rva, 32, "entrypoint")
dump(0x401010, 32, "GetBakeManager")
dump(0x55FC4C, 32, "crash-0x20")
dump(0x55FC6C, 32, "crashsite")
dump(0x962690, 32, "UI Window dtor")
dump(0x962950, 32, "UI Window ctor")
