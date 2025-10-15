#!/usr/bin/env python3

'''
Convert JSON schema files from UTF-8 encoding into char[] encoded C source files
It is equivalent to run xdd -i <SCHEMA>.json
This is useful to not require xxd as a dependency!
'''

import sys, os, re

BYTES_PER_LINE = 12

def derive_name(path_or_stdin: str) -> str:
    base = "stdin" if path_or_stdin in ("-", "") else os.path.basename(path_or_stdin)
    name = re.sub(r'[^0-9A-Za-z_]', '_', base)
    if not name:
        name = "data"
    if name[0].isdigit():
        name = "_" + name
    return name

def read_bytes(path: str) -> bytes:
    if path in ("-", ""):
        return sys.stdin.buffer.read()
    with open(path, "rb") as f:
        return f.read()

def emit_c_array(name: str, data: bytes) -> None:
    print(f"unsigned char {name}[] = {{")
    if data:
        for i in range(0, len(data), BYTES_PER_LINE):
            chunk = data[i:i+BYTES_PER_LINE]
            line = ", ".join(f"0x{b:02x}" for b in chunk)
            print(f"  {line},")
    print("};")
    print(f"unsigned int {name}_len = {len(data)};")

def main():
    # Usage: python xxd_i.py [FILE|-]
    path = sys.argv[1] if len(sys.argv) > 1 else "-"
    name = derive_name(path)
    data = read_bytes(path)
    emit_c_array(name, data)

if __name__ == "__main__":
    main()