# Low Complexity Communication Codec (LC3)

The LC3 is an efficient low latency audio codec.

[_Low Complexity Communication Codec_](https://www.bluetooth.org/DocMan/handlers/DownloadDoc.ashx?doc_id=502107&vId=542963)

## Overview

The directory layout is as follows :

- include:      Library interface
- src:          Source files
- tools:        Standalone encoder/decoder tools
- build:        Building outputs
- bin:          Compilation output

## How to build

The default toolchain used is GCC. Invoke `make` to build the library.

```sh
make -j
```

Compiled library `liblc3.a` will be found in `bin` directory.

### Cross compilation

The cc, as, ld and ar can be selected with respective Makefile variables `CC`,
`AS`, `LD` and `AR`. The `AS` and `LD` selections are optionnal, and fallback
to `CC` selection when not defined.

The `LIBC` must be set to `bionic` for android cross-compilation. This switch
prevent link with `pthread` and `rt` libraries, that is included in the
bionic libc.

Following example build for android, using NDK toolset.

```sh
make -j CC=path_to_android_ndk_prebuilt/toolchain-prefix-clang LIBC=bionic
```

Compiled library will be found in `bin` directory.

## Tools

Tools can be all compiled, while involking `make` as follows :

```sh
make tools
```

The standalone encoder `elc3` take a `wave` file as input and encode it
according given parameter. The LC3 binary file format used is the non
standard format described by the reference encoder / decoder tools.
The standalone decoder `dlc3` do the inverse operation.

Refer to `elc3 -h` or `dlc3 -h` for options.

Note that `elc3` output bitstream to standard output when output file is
omitted. On the other side `dlc3` read from standard input when input output
file are omitted.
In such way you can easly test encoding / decoding loop with :

```sh
./elc3 <in.wav> -b <bitrate> | ./dlc3 > <out.wav>
```

Adding Linux `aplay` tools, you will be able to instant hear the result :

```sh
./elc3 <in.wav> -b <bitrate> | ./dlc3 | aplay
```

### Use examples

#### Encode

encode original.wav (16 bit, 48000 Hz)

mandatory flag:

- b : encoding output bitrate. values interval [16000; 320000]

other flags:

- m : Frame duration in ms (default 10)
- r : Encoder samplerate (default is input file samplerate). values {8000, 16000, 24000, 32000, 48000}

```sh
./elc3 -b <16000 - 320000> [-m <ms> -r {8000, 16000, 24000, 32000, 48000}] <in.wav> <out.lc3>
```

```sh
./elc3 -b 48000 original.wav out.lc3
```

#### Decode

decode out.lc3, decode can work without flags

flags:

- b : sample bit size (default 16) {16, 24}
- r : output samplerate (can be more than encoded samplerate) {8000, 16000, 24000, 32000, 48000}

```sh
./dlc3 [-b {16, 24} -r {8000, 16000, 24000, 32000, 48000}] <in.lc3> <out.wav>
```

```sh
./dlc3 out.lc3 decoded48000.wav
```
