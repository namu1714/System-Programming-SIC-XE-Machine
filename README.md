# SIC/XE MACHINE
- shell prompt 
- 1MByte memory space 
- assembler    
- linking loader   
- program run   

## USAGE
```{.no-highlight}
$make   
$./20171101.out    
sicsim>
```

## COMMAND
- h[elp]  
- d[ir]   
- q[uit]   
- hi[story]   
- du[mp][start, end]   
- e[dit] address, value   
- f[ill] start, end, value   
- reset   
- opcode mnemonic   
- opcodelist   
- assemble filename   
- type filename   
- symbol   
- progaddr address   
- loader filenames   
- bp [clear][address]   
- run   

## Assemble Function
#### command: assemble filename
- type of source file must be .asm
- create .lst file and .obj file

## Linking Loader
#### command: loader filenames
- input: object file(.obj)
- link object files and load into memory

## Program Run
#### command: run
- program starts at current PC
- program stops at break point(bp)
- if there isn't any bp, run until program ends.
