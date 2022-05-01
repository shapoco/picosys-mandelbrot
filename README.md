# Mandelbrot Viewer for PICOSYSTEM

https://user-images.githubusercontent.com/40152459/166137489-871e3274-8d9b-48de-a276-9769f16b11cb.mp4

## Download Binary

https://drive.google.com/file/d/1iCvwUEkWxYbKTSbH08ddn1RtGgoKRNxY/view?usp=sharing

## How to build

Build has been verified with WSL2 on Windows 11.

1. Install [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) and [PicoSystem libraries and examples](https://github.com/pimoroni/picosystem).
2. Set the path to the PicoSystem libraries to the `PICOSYSTEM_DIR` environment variable.
3. Clone this repository.
4. Run `make -f Makefile.sample.mk all`
5. Once `picosys_mandelbrot.uf2` is generated under the `build/`, transfer it to PICOSYSTEM.

