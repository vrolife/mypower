## MYPOWER

A simple memory scanner

## Usage

Attach process
```bash
attach -p pid
```

Scan number(I32)
```bash
scan -I =0x123
```

Filter scan session
```bash
filter <0x123
```

Scan pointer chain
```bash
ptr --mask 0xFFFFFFFFFFFFFC00 --depth-max 4 --offset-max 512 --result-max 512 0x7389f6a000 0x738d7d9000 0x73672e99f0

0x7389f6a000 0x738d7d9000 is the source memory region. For example the .bss segment of so/executable

0x73672e99f0 is the target pointer.
```

## Expression

### Math

```
"0x123+100"
"0x123-100"
"0x123*100"
"0x123/100"
"0x123%100"
"0x10+0x3*0o5"
"(0x10+0x3)*0o5"
"1+3&1"
"2&4|8"
"~1+2"
"1+3<<1"
"2>1|2"
"1?2:3"
"0?2:3"
"((10-2)+0x3)*((4+5)+(5-2))"
```

Example
```
scan -I "=(0x123+456)*2"
```

### Filter

```
=
!=
>
>=
<
<=
```

Example
```
filter "<0x123*2+1"
```

## Build

Ubuntu 22.04

CMake 3.22.1

Python 3.10.6

Install DSL parser generator
```bash
git clone git@github.com:vrolife/playlang.git
cd playlang
python3 setup.py install
```

Compile
```bash
cmake --preset dev-host-libc++ -B build
cmake --build build
```

Run
```bash
./build/src/mypower
```

## License

The main program `mypower` release under GPLv3 license.

The TUI library `tui` release under MIT license.
