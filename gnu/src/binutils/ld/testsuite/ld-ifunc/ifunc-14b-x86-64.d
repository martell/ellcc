#source: ifunc-14b.s
#source: ifunc-14a.s
#ld: -shared -m elf_x86_64 -z nocombreloc
#as: --64
#readelf: -d
#target: x86_64-*-*

#failif
#...
.*\(TEXTREL\).*
#...
