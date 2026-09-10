import ctypes, os, sys
from ctypes import wintypes

dist = r"C:\Users\nambu\Modding Projects\Spore\Memos-Tweaks-Spore\Memos-Tweaks-Spore\dist"
dll = os.path.join(dist, "ERKEK2000_QoL_Runtime.dll")

dbghelp = ctypes.WinDLL("dbghelp.dll")
SYMOPT_UNDNAME=0x02; SYMOPT_DEFERRED_LOADS=0x04; SYMOPT_LOAD_LINES=0x10; SYMOPT_LOAD_ANYTHING=0x40

class SYMBOL_INFO(ctypes.Structure):
    _fields_=[("SizeOfStruct",wintypes.ULONG),("TypeIndex",wintypes.ULONG),("Reserved",ctypes.c_ulonglong*2),
              ("Index",wintypes.ULONG),("Size",wintypes.ULONG),("ModBase",ctypes.c_ulonglong),
              ("Flags",wintypes.ULONG),("Value",ctypes.c_ulonglong),("Address",ctypes.c_ulonglong),
              ("Register",wintypes.ULONG),("Scope",wintypes.ULONG),("Tag",wintypes.ULONG),
              ("NameLen",wintypes.ULONG),("MaxNameLen",wintypes.ULONG),("Name",ctypes.c_char*2000)]

class IMAGEHLP_LINE64(ctypes.Structure):
    _fields_=[("SizeOfStruct",wintypes.DWORD),("Key",ctypes.c_void_p),("LineNumber",wintypes.DWORD),
              ("FileName",ctypes.c_char_p),("Address",ctypes.c_ulonglong)]

dbghelp.SymSetOptions(SYMOPT_UNDNAME|SYMOPT_DEFERRED_LOADS|SYMOPT_LOAD_LINES|SYMOPT_LOAD_ANYTHING)
dbghelp.SymInitialize.argtypes=[wintypes.HANDLE,wintypes.LPCWSTR,wintypes.BOOL]
dbghelp.SymLoadModuleExW.argtypes=[wintypes.HANDLE,wintypes.HANDLE,wintypes.LPCWSTR,wintypes.LPCWSTR,
                                   ctypes.c_ulonglong,wintypes.DWORD,ctypes.c_void_p,wintypes.DWORD]
dbghelp.SymFromAddr.argtypes=[wintypes.HANDLE,ctypes.c_ulonglong,ctypes.POINTER(ctypes.c_ulonglong),ctypes.POINTER(SYMBOL_INFO)]
dbghelp.SymGetLineFromAddr64.argtypes=[wintypes.HANDLE,ctypes.c_ulonglong,ctypes.POINTER(wintypes.DWORD),ctypes.POINTER(IMAGEHLP_LINE64)]

h=wintypes.HANDLE(1)
if not dbghelp.SymInitialize(h,dist,False):
    print("SymInitialize failed", ctypes.GetLastError())
BASE=0x10000000
r=dbghelp.SymLoadModuleExW(h,None,dll,None,BASE,0,None,0)
print("SymLoadModuleExW ->", hex(r or 0), "err", ctypes.GetLastError(), "pdb size", os.path.getsize(os.path.join(dist,'ERKEK2000_QoL_Runtime.pdb')))

cb_type=ctypes.WINFUNCTYPE(wintypes.BOOL,ctypes.POINTER(SYMBOL_INFO),wintypes.ULONG,ctypes.c_void_p)
count=[0]
def on_sym(psi,size,ctx):
    count[0]+=1
    return True
cb=cb_type(on_sym)
dbghelp.SymEnumSymbolsW.argtypes=[wintypes.HANDLE,ctypes.c_ulonglong,wintypes.LPCWSTR,cb_type,ctypes.c_void_p]
dbghelp.SymEnumSymbolsW(h,BASE,None,cb,None)
print("enum symbols:", count[0])

for a in sys.argv[1:]:
    off=int(a,16); addr=BASE+off
    disp=ctypes.c_ulonglong(0); si=SYMBOL_INFO(); si.SizeOfStruct=88; si.MaxNameLen=2000
    ok=dbghelp.SymFromAddr(h,addr,ctypes.byref(disp),ctypes.byref(si))
    line=IMAGEHLP_LINE64(); line.SizeOfStruct=ctypes.sizeof(IMAGEHLP_LINE64); ld=wintypes.DWORD(0)
    okline=dbghelp.SymGetLineFromAddr64(h,addr,ctypes.byref(ld),ctypes.byref(line))
    name=si.Name.decode(errors="replace") if ok else "<no symbol>"
    loc=f" [{'/'.join(line.FileName.decode(errors='replace').replace(chr(92),'/').split('/')[-1:])}:{line.LineNumber}]" if okline else ""
    print(f"0x{off:X} -> {name}{loc} +0x{disp.value:X}")
