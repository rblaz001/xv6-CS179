# Thread Support for xv6

This project adds Kernel and User level thread support to xv6.

## Installation

We suggest the use of docker and our Dockerfile to set up the xv6 environment.

```Docker_Instructions
docker build -t xv6 . && docker run -it xv6
```

## Building QEMU

```Qemu
make qemu-nox # builds QEMU and enters shell for xv6
```

## Using GDB

```Qemu
tmux # Use ctrl+b followed by % to split terminal horizontaly
     # Use ctrl+b followed by left or right arrow to switch between terminals

make qemu-nox-gdb # Use in first terminal
gdb -iex "set auto-load safe-path /xv6/.gdbinit" # Use in second terminal
```