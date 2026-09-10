#!/usr/bin/env python3
"""Locate the symbol nearest to (and below) a given image address.

Reads the address map(s) shipped with the ModAPI SDK:
  - SDKtoGhidra/additional_march2017.txt   (NNNN=N with 0xADDR=Name lines)

Usage:
  python FindSymbol.py 0x95FC6C [0xADDR2 ...]
"""
import os
import sys

SDK = r"C:\Users\nambu\Modding Projects\Spore\Tools\Spore-ModAPI-SDK\SDKtoGhidra"
MAP_FILES = [
    "additional_march2017.txt",
]


def load_map():
    syms = []
    for name in MAP_FILES:
        path = os.path.join(SDK, name)
        if not os.path.exists(path):
            continue
        with open(path, "r", encoding="utf-8", errors="replace") as fh:
            for line in fh:
                line = line.strip()
                if not line or "=" not in line:
                    continue
                addr_s, sym = line.split("=", 1)
                try:
                    addr = int(addr_s.strip(), 16)
                except ValueError:
                    continue
                syms.append((addr, sym.strip(), name))
    syms.sort()
    return syms


def main():
    syms = load_map()
    print("loaded %d symbols from %s" % (len(syms), ", ".join(MAP_FILES)))
    if syms:
        print("address range: 0x%X .. 0x%X" % (syms[0][0], syms[-1][0]))
    if len(sys.argv) < 2:
        return
    for arg in sys.argv[1:]:
        target = int(arg, 16)
        # nearest symbol at or below target
        below = None
        above = None
        for addr, sym, src in syms:
            if addr <= target:
                below = (addr, sym, src)
            elif above is None:
                above = (addr, sym, src)
                break
        print("\n--- target 0x%X (RVA 0x%X) ---" % (target, target - 0x400000))
        if below:
            print("  <= 0x%X  %s   [+0x%X]" % (below[0], below[1], target - below[0]))
        if above:
            print("  >  0x%X  %s   [next, -0x%X]" % (above[0], above[1], above[0] - target))
        # a few symbols around it for context
        idx = 0
        for i, (addr, sym, src) in enumerate(syms):
            if addr > target:
                idx = i
                break
        print("  context:")
        for addr, sym, src in syms[max(0, idx - 4):idx + 4]:
            marker = " <== TARGET" if addr <= target < (addr + 0x1000) else ""
            print("    0x%X  %s%s" % (addr, sym, marker))


if __name__ == "__main__":
    main()
