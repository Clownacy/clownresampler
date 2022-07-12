# clownresampler

This is a single-file header-only library for resampling audio. It is written in C89 
and dual-licenced under the terms of The Unlicence and the Zero-Clause BSD licence.
In particular, this library implements a windowed-sinc resampler, using a
Lanczos window.

To install this header into the standard include directory from the command line, 
run the following commands:

```
cmake .
make install
```
