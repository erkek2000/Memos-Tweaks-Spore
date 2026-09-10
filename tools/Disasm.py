#!/usr/bin/env python3
"""Minimal PE section dumper + x86-32 disassembler for SporeApp.exe crash analysis.

Usage:
  python Disasm.py <exe> <image_address> [byte_count]

Decodes instructions ending at <image_address> (the address of the instruction
following a faulting call) and prints a small window before and after it.
Image addresses assume the PE ImageBase (0x00400000 for SporeApp.exe).
"""

import struct
import sys


def parse_pe(path):
    with open(path, "rb") as f:
        data = f.read()
    pe_off = struct.unpack_from("<I", data, 0x3C)[0]
    if data[pe_off:pe_off + 4] != b"PE\0\0":
        raise ValueError("not a PE file")
    num_sections = struct.unpack_from("<H", data, pe_off + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe_off + 20)[0]
    opt_off = pe_off + 24
    image_base = struct.unpack_from("<I", data, opt_off + 28)[0]
    sec_off = opt_off + opt_size
    sections = []
    for i in range(num_sections):
        o = sec_off + i * 40
        name = data[o:o + 8].rstrip(b"\0").decode("ascii", "replace")
        virt_size = struct.unpack_from("<I", data, o + 8)[0]
        virt_addr = struct.unpack_from("<I", data, o + 12)[0]
        raw_size = struct.unpack_from("<I", data, o + 16)[0]
        raw_ptr = struct.unpack_from("<I", data, o + 20)[0]
        sections.append((name, virt_addr, virt_size, raw_ptr, raw_size))
    return data, image_base, sections


def rva_to_off(sections, rva):
    for name, va, vs, raw_ptr, raw_size in sections:
        if va <= rva < va + max(vs, raw_size):
            return raw_ptr + (rva - va), name
    return None, None


# --- minimal x86-32 decoder -------------------------------------------------

_REGS8 = ("al", "cl", "dl", "bl", "ah", "ch", "dh", "bh")
_REGS32 = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
_REGS16 = ("ax", "cx", "dx", "bx", "sp", "bp", "si", "di")


def decode(data, base):
    """Yield (address, size, mnemonic) starting at base."""
    off = 0
    n = len(data)
    while off < n:
        start = off
        b0 = data[off]
        off += 1
        asm = "db 0x%02X" % b0
        size = 1

        def modrm():
            nonlocal off
            b = data[off]
            off += 1
            mod, reg, rm = b >> 6, (b >> 3) & 7, b & 7
            disp = ""
            if mod == 0 and rm == 5:
                disp = "[0x%08X]" % struct.unpack_from("<I", data, off)[0]
                off += 4
            elif mod == 1:
                disp = "[+0x%02X]" % data[off]
                off += 1
            elif mod == 2:
                disp = "[+0x%08X]" % struct.unpack_from("<I", data, off)[0]
                off += 4
            base_r = _REGS32[rm]
            if mod == 3:
                rm_str = _REGS32[rm]
            else:
                if rm == 4:
                    sib = data[off]
                    off += 1
                    scale = (sib >> 6) & 3
                    idx = (sib >> 3) & 7
                    bs = sib & 7
                    parts = []
                    if bs != 5 or mod != 0:
                        parts.append(_REGS32[bs])
                    if idx != 4:
                        parts.append("%s*%d" % (_REGS32[idx], 1 << scale))
                    rm_str = "+".join(parts)
                    if not parts:
                        rm_str = "0x%08X" % struct.unpack_from("<I", data, off)[0]
                        off += 4
                        rm_str = "[%s%s]" % (rm_str, disp)
                    else:
                        rm_str = "[%s%s]" % (rm_str, disp)
                else:
                    rm_str = "[%s%s]" % (base_r, disp)
            return mod, reg, rm_str

        if b0 in (0x90,):
            asm = "nop"
        elif 0x50 <= b0 <= 0x57:
            asm = "push %s" % _REGS32[b0 - 0x50]
        elif 0x58 <= b0 <= 0x5F:
            asm = "pop %s" % _REGS32[b0 - 0x58]
        elif b0 == 0xCC:
            asm = "int3"
        elif b0 == 0xC3:
            asm = "ret"
        elif b0 == 0xC2:
            imm = struct.unpack_from("<H", data, off)[0]
            off += 2
            asm = "ret 0x%X" % imm
        elif b0 == 0xE8 or b0 == 0xE9:
            rel = struct.unpack_from("<i", data, off)[0]
            off += 4
            target = base + start + 5 + rel
            asm = ("call 0x%08X" if b0 == 0xE8 else "jmp 0x%08X") % target
        elif b0 in (0xEB,):
            rel = struct.unpack_from("<b", data, off)[0]
            off += 1
            asm = "jmp 0x%08X" % (base + start + 2 + rel)
        elif b0 in (0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
                    0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F):
            rel = struct.unpack_from("<b", data, off)[0]
            off += 1
            asm = "jcc 0x%08X" % (base + start + 2 + rel)
        elif b0 == 0x0F:
            b1 = data[off]
            off += 1
            if 0x80 <= b1 <= 0x8F:
                rel = struct.unpack_from("<i", data, off)[0]
                off += 4
                asm = "jcc32 0x%08X" % (base + start + 6 + rel)
            elif b1 in (0x84, 0x85):
                mod, reg, rm_str = modrm()
                asm = ("jz 0x0? " if b1 == 0x84 else "jnz ") + rm_str
            elif b1 == 0xB6:
                mod, reg, rm_str = modrm()
                asm = "movzx %s, byte %s" % (_REGS32[reg], rm_str)
            elif b1 == 0xB7:
                mod, reg, rm_str = modrm()
                asm = "movzx %s, word %s" % (_REGS32[reg], rm_str)
            elif b1 == 0xBE:
                mod, reg, rm_str = modrm()
                asm = "movsx %s, byte %s" % (_REGS32[reg], rm_str)
            elif b1 == 0xBF:
                mod, reg, rm_str = modrm()
                asm = "movsx %s, word %s" % (_REGS32[reg], rm_str)
            else:
                asm = "db 0x0F,0x%02X" % b1
        elif b0 in (0x68,):
            imm = struct.unpack_from("<I", data, off)[0]
            off += 4
            asm = "push 0x%08X" % imm
        elif b0 in (0x6A,):
            imm = data[off]
            off += 1
            asm = "push 0x%02X" % imm
        elif b0 == 0x8B:
            mod, reg, rm_str = modrm()
            asm = "mov %s, %s" % (_REGS32[reg], rm_str)
        elif b0 == 0x89:
            mod, reg, rm_str = modrm()
            asm = "mov %s, %s" % (rm_str, _REGS32[reg])
        elif b0 == 0x8D:
            mod, reg, rm_str = modrm()
            asm = "lea %s, %s" % (_REGS32[reg], rm_str)
        elif b0 in (0x8A, 0x88):
            mod, reg, rm_str = modrm()
            dst, src = (_REGS8[reg], rm_str) if b0 == 0x8A else (rm_str, _REGS8[reg])
            asm = "mov %s, %s" % (dst, src)
        elif b0 in (0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF):
            imm = struct.unpack_from("<I", data, off)[0]
            off += 4
            asm = "mov %s, 0x%08X" % (_REGS32[b0 - 0xB8], imm)
        elif b0 in (0x83,):
            mod, reg, rm_str = modrm()
            imm = struct.unpack_from("<b", data, off)[0]
            off += 1
            names = ("add", "or", "adc", "sbb", "and", "sub", "xor", "cmp")
            asm = "%s %s, 0x%02X" % (names[reg], rm_str, imm & 0xFF)
        elif b0 in (0x81,):
            mod, reg, rm_str = modrm()
            imm = struct.unpack_from("<I", data, off)[0]
            off += 4
            names = ("add", "or", "adc", "sbb", "and", "sub", "xor", "cmp")
            asm = "%s %s, 0x%08X" % (names[reg], rm_str, imm)
        elif b0 in (0x01, 0x03, 0x09, 0x0B, 0x2B, 0x33, 0x3B, 0x85, 0x39, 0x29):
            mod, reg, rm_str = modrm()
            names = {0x01: "add", 0x03: "add", 0x09: "or", 0x0B: "or",
                     0x2B: "sub", 0x33: "xor", 0x3B: "cmp", 0x85: "test",
                     0x39: "cmp", 0x29: "sub"}
            op = names[b0]
            if b0 in (0x03, 0x0B, 0x2B, 0x33, 0x3B, 0x85):
                asm = "%s %s, %s" % (op, _REGS32[reg], rm_str)
            else:
                asm = "%s %s, %s" % (op, rm_str, _REGS32[reg])
        elif b0 == 0xFF:
            mod, reg, rm_str = modrm()
            if reg == 2:
                asm = "call %s" % rm_str
            elif reg == 4:
                asm = "jmp %s" % rm_str
            elif reg == 6:
                asm = "push %s" % rm_str
            elif reg == 0:
                asm = "inc %s" % rm_str
            elif reg == 1:
                asm = "dec %s" % rm_str
            else:
                asm = "ff /%d %s" % (reg, rm_str)
        elif b0 == 0xF7:
            mod, reg, rm_str = modrm()
            asm = "f7 /%d %s" % (reg, rm_str)
        elif b0 == 0xC7:
            mod, reg, rm_str = modrm()
            imm = struct.unpack_from("<I", data, off)[0]
            off += 4
            asm = "mov %s, 0x%08X" % (rm_str, imm)
        elif b0 == 0x85:
            mod, reg, rm_str = modrm()
            asm = "test %s, %s" % (_REGS32[reg], rm_str)
        elif b0 == 0xE8:
            pass
        else:
            asm = "db 0x%02X" % b0

        size = off - start
        yield base + start, size, asm


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)
    exe, addr_str = sys.argv[1], sys.argv[2]
    count = int(sys.argv[3]) if len(sys.argv) > 3 else 96
    addr = int(addr_str, 16)

    data, image_base, sections = parse_pe(exe)
    rva = addr - image_base
    print("image_base=0x%08X  target rva=0x%08X" % (image_base, rva))
    for name, va, vs, raw_ptr, raw_size in sections:
        print("  %-8s VA=0x%08X VirtSize=0x%08X RawPtr=0x%08X RawSize=0x%08X"
              % (name, va, vs, raw_ptr, raw_size))

    # dump raw bytes around the target
    off, sec = rva_to_off(sections, rva)
    if off is None:
        print("target not in any section")
        sys.exit(1)
    window = data[off - 64:off + count]
    for i in range(0, len(window), 16):
        chunk = window[i:i + 16]
        hexs = " ".join("%02X" % b for b in chunk)
        asc = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        print("  0x%08X: %-47s %s" % (addr - 64 + i, hexs, asc))

    print("\n-- disassembly (window around target) --")
    for a, size, asm in decode(window, addr - 64):
        marker = " <== target" if a == addr else ""
        print("  0x%08X (%d): %s%s" % (a, size, asm, marker))


if __name__ == "__main__":
    main()
