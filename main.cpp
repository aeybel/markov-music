// Final Project
// Alanna Eybel
//
// This program generates music based on a markov chain and an input file

#include "MidiFile.h"
#include <iostream>
#include <fstream>
#include <random>
#include <chrono>
#include <algorithm>
#include <map>
#include <sstream>

using namespace std;
using namespace smf;

// Allows for printing vectors with std::cout
template < class T >
ostream& operator << (ostream& os, const vector<T>& v)
{
    os << "[";
    for (typename vector<T>::const_iterator ii = v.begin(); ii != v.end(); ++ii)
    {
        os << " " << *ii;
    }
    os << "]";
    return os;
}

// The class of notes
struct Note;

// Initialize the random seed and the random generator
unsigned seed = chrono::system_clock::now().time_since_epoch().count();
mt19937 generator (seed);

// The smallest note in the piece (used for durations and start times)
double smallestNote = 12.0; //quarter is 1.0, eighth is 2.0, sixteenth is 4.0
                            //corresponds to how many of them can fit in a quarter note

// The actual song which we are building
vector<Note> song;

// REFERENCES
/* TABLE OF NOTES AND PITCH #s
NOTES   PITCH #
C       0
C#/Db   1
D       2
D#/Eb   3
E       4
F       5
F#/Gb   6
G       7
G#/Ab   8
A       9
A#/Bb   10
B       11  */

/* CIRCLE OF FIFTHS
   fa fb fc fd fe ff 0 1 2 3  4  5  6
Cb Gb Db Ab Eb Bb F  C G D A  E  B  F# C#
ab eb bb f  c  g  d  a e b f# c# g# d# a#
(the key signature of a midi file is encoded using the circle of fifths)
*/

// The encoding for the key (based on the circle of fifths) coverted to the number of notes from C
map<int, int> fifthsToKey = {{0xfa,6}, {0xfb,1}, {0xfc,8}, {0xfd,3}, {0xfe,10}, {0xff,5}, {0x00,0}, {0x01,7}, {0x02,2}, {0x03,9}, {0x04,4}, {0x05,11}, {0x06,6}};

// A pitch class, which holds information for what the next note could be based on the pitch of this note
struct MidiPitch
{
    vector<double> nextPitches; // The pitch of the next note
    vector<double> durations;   // The duration of this note
    vector<double> tilNext;     // The time until the next note
};

// Another matrix for a markov chain of pitch items
vector<MidiPitch> pitchInfo = vector<MidiPitch>(128);

// The object which contains the simplified data for each of the notes
struct Note
{
    int midiPitch;      // The midi pitch (from 0 to 128)
    double startBeat;   // The number of beat divisions since the start of the song
    double duration;    // The number of beats divisions it takes up

    // Initialization functions
    Note() {}
    Note(int mp, double time) // To generate using the markov chain and a previously known note
    {
        generateFromNode(mp);
        startBeat = time;
    }
    Note (int p, double s, double d) // Set all of the variables
    {
        midiPitch=p;
        startBeat=s;
        duration=d;
    }

    // Generate the pitch and duration of this note using the markov chain
    generateFromNode(int mp)
    {
        // Make the pitch a random pitch from the list of pitches from the last note
        midiPitch = pitchInfo[mp].nextPitches[generator()%pitchInfo[mp].nextPitches.size()];
        // Make the duration a random duration from the list of durations for this pitch
        duration = pitchInfo[midiPitch].durations[generator()%pitchInfo[midiPitch].durations.size()];
    }
};

// The time information
double tpqSum = 0;
double tempoSum = 0;

// The function to process all of the information fromt the file
bool processFile(string fileName)
{
    // Read the file
    MidiFile testFile;
    fileName = "input\\" + fileName;
    if (!testFile.read(fileName))
    {
        return false;
    }

    // Analyze the file
    testFile.joinTracks();
    testFile.doTimeAnalysis();
    testFile.linkNotePairs();
    double testTpq = testFile.getTicksPerQuarterNote();
    tpqSum += testTpq;

    // Variables to store the information for the previous note
    double lastTime = 0;
    double lastPitch = 60;
    double tilNext = 0;

    // Variables storing formation about the song file itself
    int index = 0;
    int keySign = 0;
    bool tempoFound = false;

    // Go through the list of midievents
    for (int event = 0; event < testFile[0].size(); event++)
    {
        MidiEvent* mev = &testFile[0][event];

        // If it is a note on, we want to add the note to our notes vector
        if (mev->isNoteOn())
        {
            // Find the corresponding note off event
            MidiEvent* linked = mev->getLinkedEvent();

            // Get all of the data required for the note
            int newPitch = (int)(*mev)[1]-keySign;
            double newDur = (linked->tick-mev->tick)/testTpq;
            double newTime = mev->tick/testTpq;

            // Link the values in the markov chain
            if (index != 0)
            {
                // Add the information to the markov chain
                pitchInfo[lastPitch].nextPitches.push_back(newPitch);
                pitchInfo[lastPitch].durations.push_back(newDur);
                pitchInfo[lastPitch].tilNext.push_back(newTime-lastTime);
            }

            // Adjust the variables so the current note information is the previous note
            lastPitch = newPitch;
            lastTime = newTime;
            index++;
        }

        // This checks if the event type is a key change event
        if ((int)(*mev)[0] == 0xff && (int)(*mev)[1] == 0x59 && (int)(*mev)[2] == 0x02)
        {
            // Set the key adjustment factor (because we want everything in the key of C)
            keySign = fifthsToKey[(int)(*mev)[3]];
        }

        // This checks if the event type if a tempo change event
        if (!tempoFound && (int)(*mev)[0] == 0xff && (int)(*mev)[1] == 0x51 && (int)(*mev)[2] == 0x03)
        {
            // Read the bytes to figure out what the tempo is
            double invTemp = (int)(*mev)[3]*65536;
            invTemp += (int)(*mev)[4]*256;
            invTemp += (int)(*mev)[5];
            tempoSum += 60/invTemp*1000000;

            // Set that we found the tempo
            tempoFound = true;
        }

        // So it doesn't run for too long
        if (event > 5000)
        {
            break;
        }
    }

    return true;
}

// Check if it is a valid option (a single digit)
int checkInt(string str)
{
    if (str.length() == 1)
    {
        if (isdigit(str[0]))
        {
            return str[0]-'0';
        }
    }
    return -1;
}

int main(int argc, char** argv) {

    vector<string> fileNames = {}; // Array of names of the test files

    // GETS THE INPUT FROM THE USER AND BUILDS A LIST OF THE FILES
    string userChoice;

    cout << "What files would you like to use?" << endl;
    cout << "Enter a number between 1-9 for a present, or 0 for custom" << endl;

    // This loop will continue until there is proper input
    while (true)
    {
        // Prompt and get user input
        cout << ">";
        getline(cin, userChoice);

        // If the user chooses a custom option
        if (userChoice == "0")
        {
            string fileName;
            cout << "Enter a list of file names to import; 'q' to stop" << endl;
            cout << ">";
            cin >> fileName;

            // Get all of the file names until the user enters 'q'
            while (fileName != "q")
            {
                fileNames.push_back(fileName);
                cout << ">";
                cin >> fileName;
            }
            cin.ignore();

            break;
        }

        // Otherwise check if it is the number for a present
        else
        {
            int check = checkInt(userChoice);
            // If it is the number for a present
            if (check != -1)
            {
                // Read the file with the presents information
                string fileName;
                bool foundStart = false;
                ifstream fileList("filenames.txt");
                while (getline(fileList, fileName))
                {
                    // Continue until we find a flag indicating the present starts
                    if (fileName == userChoice)
                    {
                        // If we haven't found the flag yet
                        if (!foundStart)
                        {
                            // Set that we have found the flag, and can start reading
                            foundStart = true;
                            // Adjust the number so that we know when the stop
                            userChoice[0] += 1;
                            continue;
                        }
                        // If we have fond the flag, stop
                        else
                        {
                            break;
                        }
                    }
                    // If we are in the present, then add the name to the array
                    if (foundStart)
                    {
                        fileName += ".mid";
                        fileNames.push_back(fileName);
                    }
                }
                break;
            }
        }
        // If the input isn't any of the valid options
        cout << "Invalid option" << endl;
    }

    bool validFound = false;

    // READS THE INFORMATION FROM THE FILES
    for (int i = 0; i < fileNames.size(); i++)
    {
        if (processFile(fileNames[i]))
        {
            validFound = true;
        }
        cout << endl;
    }

    // If there were no good files, quit the program
    if (!validFound)
    {
        cout << "There were no valid files entered" << endl;
        return 0;
    }

    // Time analysis stuff
    double tpq  = tpqSum/fileNames.size();
    int tempo = tempoSum/fileNames.size();

    // GET THE LENGTH FROM THE USER

    int maxLength = 0;
    string lenStr;
    cout << "How long should the piece be in minutes?" << endl;

    // Try to get the input as long as the input is invalid
    do
    {
        cout << ">";
        getline(cin, lenStr);

        //Check if the input string is an integer
        bool validInt = true;
        for (int i = 0; i < lenStr.length(); i++)
        {
            if (!isdigit(lenStr[i]))
            {
                validInt = false;
                break;
            }
        }

        // If it is positive number
        if (validInt)
        {
            // To avoid building a really really long piece
            if (lenStr.length() > 2)
            {
                cout << "Number too large (must be 2 digits or less)" << endl;
            }
            else
            {
                // Convert it to an int
                stringstream ss;
                ss << lenStr;
                ss >> maxLength;

                // If the number is 0, we don't want it
                if (maxLength == 0)
                {
                    cout << "Length must be above zero" << endl;
                }
            }
        }
        // If it isn't a positive number
        else
        {
            cout << "Invalid postive integer" << endl;
        }
    }
    while (maxLength <= 0);

    // BUILD A SONG USING A TIMED MARKOV CHAIN

    // The information for the starting note
    double thisTime = 0;
    int thisPitch = 60;

    // Generate new notes until the the desired length is reached
    while (thisTime < maxLength*tempo)
    {
        Note newNote = Note(thisPitch, thisTime);
        song.push_back(newNote);
        thisTime += pitchInfo[thisPitch].tilNext[generator()%(pitchInfo[thisPitch].tilNext.size())];
        thisPitch = newNote.midiPitch;
    }

    // WRITE TO THE MIDI FILE

    // Set the initial settings
    MidiFile midifile;
    int track = 0;
    int channel = 0;
    int instr = 0;

    midifile.setTicksPerQuarterNote(tpq);
    midifile.addTempo(0, 0, tempo);

    // Add all of the notes to the file
    for (int i = 0; i < song.size(); i++)
    {
        midifile.addNoteOn(track, song[i].startBeat*tpq, channel, song[i].midiPitch, 100);
        midifile.addNoteOff(track, song[i].startBeat*tpq+song[i].duration*tpq, channel, song[i].midiPitch);
    }

    // NOTE: ARGUMENTS FOR ADDNOTEON AND ADDNOTEOFF
    // file.addNoteOn(track, tick, channel, pitch, velocity)
        // The TRACK is the track which we are editing (probably 0)
        // The TICK is the time which the event occurs (the time the note goes on)
        // This will be the beat number since the beginning of the piece times the TPQ (ticks per quarter)
        // The CHANNEL is the channel (usually 0)
        // The PITCH is the midi pitch (ex. C4 is 60)
        // The VELOCITY is the loudness of the pitch
    // file.addNoteoff(track, tiNote (int p, int s, int d) {pitch=p%12; octave=(p/12)-1; startBeat=s; duration=d;}ck, channel, pitch)
        // This is the same as above
        // This TICK is the time when the note goes off not when it goes on
        // Take the starting tick and add the length times the TPQ

    midifile.sortTracks();  // Need to sort tracks since added events are
                            // appended to track in random tick order.

    // OUTPUT THE FILE

    // Ask the user where to output the file
    cout << endl << "What would you like to call the output file? (do not include extension)" << endl;
    cout << ">";

    string filename;
    cin >> filename;
    filename = "output\\" + filename + ".mid";

    // Try to write to the file and deliever a success message
    if (midifile.write(filename))
    {
        cout << "Song succesfully written to " << filename << endl;
    }
    else
    {
        cout << "Song could not be written to " << filename << endl;
    }

    return 0;
}
