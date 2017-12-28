This project is an implementation of the acoustic complexity index, originally theorised
and later developed by Pieretti.

#### How to use ####
Provided Make is installed, simply typing the command 'make' in the top level should suffice in compiling
the source code. The output file should be named "acousticComplexityIndex".
To obtain the acoustic complexity for any given wav file use:
    ./acousticComplexityIndex -f path/to/wav
This code was written on an osx platform and I can therefore not guarantee that it will compile
or execute as expected on other machines. If you have trouble compiling it feel free to contact me
and I will do my best to help.

#### Files ####
Files within this project:
  acousticComplexityIndex.c - The implementation of ACI
  kiss_fft130/ - Directory containing the FFT implementation (KISS) used by acousticComplexityIndex.c
  Makefile - The makefile which compiles the source code into an executable binary
  data/ - Directory containing 3 pieces of test .wav data:
           sineWave440Hz - A sine wave (expect extremely low ACI)
           traffic - recording of car trafiic (low ACI)
           bird song with background rain (high ACI)

#### Theory ####
I will summarise the key points made by Pieretti in "A new methodology to infer the singing activity of an avian community:
The Acoustic Complexity Index (ACI)". If it is a subject that interests you I suggest you find a copy of that piece of work.

All sounds can be categorised as belonging to one of three soundscapes - the biophony, geophony or the anthrophony.
The biophony contains all animal vocalisations (e.g. bird song), the geophony all environmental sounds (e.g. weather) and the
anthrophony contains all human-induced sound (e.g. traffic).
Within these three soundscapes there is a phenomenon which is exploited by ACI; all sounds within the biophony have rapidly
changing frequency content whilst all others do not.
The way which ACI utilises this is by splitting the recording into multiple FFTs (essentially extracting there frequency content)
and returning a value which is directly proportional to the variations within the frequency content of the recording.
A recording with rapidly changing frequency content will have a much larger value of ACI than something with slowly varying frequency
content.
