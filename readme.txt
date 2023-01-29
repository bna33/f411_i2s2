This  project that reads from memory, parses and plays OGG files using I2S from memory.
Uses a STM32F411CE "Black Pill" board .
For simplicity, only 8KHz, 16-bit stereo or mono  files are supported.
LRC    <- PB12
BCLK <- PB10
DIN    <- PB15

The audio file is in myaudio.h
Use  File to hexadecimal converter for new audio.