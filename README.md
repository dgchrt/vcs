# vcs
VCS Emulator

In its current form, it partially emulates a headless [VCS](https://en.wikipedia.org/wiki/VCS) which can only boot standard 4K ROMs and then simply output every instruction being fetched. Still no RIOT, TIA, illegal 6507 instructions or custom cartridges. In other words, it does not play any games yet.

## Building

To build the emulator, you can use the provided build script:

```shell
./build.sh
```

Alternatively, you can run `make` directly. The final executable will be located in the `dist/` directory.

## Running

Run the emulator by providing a path to a standard 4K ROM file. You can use the provided run script:

```shell
./run.sh roms/adventure.a26
```

Alternatively, you can run the executable directly:

```shell
./dist/vcs roms/adventure.a26
```

During execution, the emulator will log each fetched instruction byte to the console. Press `ESC` to stop the emulator.

## Project History
This project was originally developed using Borland Turbo C for DOS and has been recently ported to modern POSIX/Linux environments using standard `termios` for terminal handling.
