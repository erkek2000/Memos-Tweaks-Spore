#!/usr/bin/env python3
"""Parse a Windows minidump and list its streams + memory ranges.

Usage: python DumpInfo.py <minidump file>
"""
import struct
import sys

STREAM_NAMES = {
    0: "UnusedStream", 1: "ReservedStream0", 2: "ReservedStream1",
    3: "ThreadListStream", 4: "ModuleListStream", 5: "MemoryListStream",
    6: "ExceptionStream", 7: "SystemInfoStream", 8: "ThreadExListStream",
    9: "Memory64ListStream", 10: "CommentStreamA", 11: "CommentStreamW",
    12: "HandleDataStream", 13: "FunctionTableStream", 14: "UnloadedModuleListStream",
    15: "MiscInfoStream", 16: "MemoryInfoListStream", 17: "ThreadInfoListStream",
    18: "HandleOperationListStream", 19: "TokenStream", 20: "JavaScriptDataStream",
    21: "SystemMemoryInfoStream", 22: "ProcessVmCountersStream", 23: "IptTraceStream",
    24: "ThreadNamesStream", 25: "ceStreamNull", 26: "ceStreamSystemInfo",
    27: "ceStreamException", 28: "ceStreamModuleList", 29: "ceStreamProcessList",
    30: "ceStreamThreadList", 31: "ceStreamThreadContextList", 32: "ceStreamThreadCallStackList",
    33: "ceStreamMemoryVirtualList", 34: "ceStreamMemoryPhysicalList", 35: "ceStreamBucketParameters",
    36: "ceStreamProcessModuleMap", 37: "ceStreamDiagnosisList",
    0x8000: "LastReservedStream",
}


def main():
    path = sys.argv[1]
    data = open(path, "rb").read()
    sig = data[0:4]
    print("signature:", sig)
    if sig != b"MDMP":
        print("not a minidump")
        sys.exit(1)
    version = struct.unpack_from("<I", data, 4)[0]
    num_streams = struct.unpack_from("<I", data, 8)[0]
    dir_rva = struct.unpack_from("<I", data, 12)[0]
    flags = struct.unpack_from("<I", data, 16)[0]
    print("version 0x%X, streams=%d, dir_rva=0x%X, flags=0x%X" % (version, num_streams, dir_rva, flags))
    print("file size:", len(data))

    streams = []
    for i in range(num_streams):
        o = dir_rva + i * 12
        stype, dsize, rva = struct.unpack_from("<III", data, o)
        name = STREAM_NAMES.get(stype, "type_%d" % stype)
        streams.append((stype, dsize, rva))
        print("  stream %2d: %-28s size=%8d rva=0x%X" % (stype, name, dsize, rva))

    # memory ranges from MemoryListStream(5) or Memory64ListStream(9)
    for stype, dsize, rva in streams:
        if stype in (5, 9):
            print("\nMemory ranges from stream type %d (%s):" % (stype, STREAM_NAMES.get(stype)))
            if stype == 5:
                n = struct.unpack_from("<I", data, rva)[0]
                pos = rva + 4
                for i in range(n):
                    start, msize = struct.unpack_from("<QI", data, pos)
                    pos += 12
                    print("  0x%016X size 0x%X" % (start, msize))
            else:
                n = struct.unpack_from("<Q", data, rva)[0]
                base_rva = struct.unpack_from("<Q", data, rva + 8)[0]
                pos = rva + 16
                for i in range(n):
                    start, msize = struct.unpack_from("<QQ", data, pos)
                    pos += 16
                    print("  0x%016X size 0x%X (at file 0x%X)" % (start, msize, base_rva))


if __name__ == "__main__":
    main()
