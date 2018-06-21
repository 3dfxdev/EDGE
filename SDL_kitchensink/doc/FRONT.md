# SDL_kitchensink

## Overview

FFmpeg and SDL2 based library for audio and video playback, written in C99.
Please see [github.com/katajakasa/SDL_kitchensink](https://github.com/katajakasa/SDL_kitchensink)
for project details and source code.

Features:
* Decoding video & audio via FFmpeg
* Dumping video data on SDL_textures
* Dumping audio data in the usual mono/stereo interleaved formats
* Automatic audio and video conversion to SDL2 friendly formats
* Synchronizing video & audio to clock
* Seeking forwards and backwards
* Bitmap & libass subtitle support. No text (srt, sub) support yet.

Note that FFmpeg has a rather complex license. Please take a look at
[FFmpeg Legal page](http://ffmpeg.org/legal.html) for details.

## License

The MIT License (MIT)

Copyright (c) 2016 Tuomas Virtanen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
