# markov-music
MIDI music generated through markov chains

This project creates computer generated music using a markov chain. The user is able to choose what files are to be inputted and used either from pre-existing presents or by entering custom file names. These files are then processed to create a markov chain. Each note object in the inputted file is read to determine the pitch, duration, and when it occurs. It then builds a markov chain consisting of nodes which are each of the 128 midi pitches, and contain weighted links to probabilities for what the next pitch will be, as well as weights for the duration of the note and how long it will be until the next note. The program then has a fixed starting note and builds a song based on the probabilities in the markov chain until the user-specified length of the song is reached.

Created using C++11 in Codeblocks on Windows

The following are used in the project, but not included:
<ul>
<li>The C++ midifile library (midifile.sapp.org) used for reading and writing to midi files</li>
<li>An “input” folder in the source directory which contains a variety of midi files to be inputted into the project</li>
<li>An “output” folder in the source directory which contains all of the outputs from the program as midi files</li>
</ul>

A sample output "sample.mp3" has been included. This is a midi output from the program rendered using LABS Audio Soft Piano.
