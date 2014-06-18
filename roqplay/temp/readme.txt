
Id Software's RoQ video file format decoder source code.

This will build to two programs: roqtoppm and sroqplay.

roqtoppm - Passing this an RoQ file and an output base name will create
  ppm files of each frame in the RoQ file and a wave file of the audio.

sroqplay - This must be compiled and linked with the SDL library
  (http://www.libsdl.org).  Passing this an RoQ file will play back the
  audio and video sequence.  This is a very primitive player that expects
  your system to have a 32-bit display and be fast (no frame dropping).

Included in this archive is a description of the coding technique: idroq.txt
For more details and updates on this and other algorithm visit:
     http://www.csse.monash.edu.au/~timf/videocodec.html

