# 8080-emulator

An Intel 8080 emulator written in C. 

Passes the 8080 instruction test suite and comes with a small GDB-style debugger.

Interrupt handling is still missing, so Space Invaders will have to wait.

# Build & Run 

```sh
make 
./emu
```

# To run the debugger

```sh
./emu -d
```
Type 'h' in the debugger to get more info on debugger.

Still a work in progress.
