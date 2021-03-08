#pragma once
#include "include/AudioFile.h"
//#include "include/pfd.h"
#include "include/tinyfiledialogs.h"
#include <string>
// Degree of accuracy for comparing floats
const double EPSILON = 0.0001;
using std::vector;
using std::string;

// A result for processing an audio file.
enum AudioResult {
    Stereo, // When audio file is true stereo
    FakeStereo, // When audio file is 'fake' stereo
    Mono // When audio file is mono
};

/**
 * Returns true is float a and float b are sufficiently close in value.
 * EPSILON constant defines the maximum difference between the two.
 */
bool compareFloat(float a, float b) {
    return abs(a - b) < EPSILON;
}

/**
 * Processes a given audio buffer to determine if it is truely stereo.
 * Returns 'Mono' if the buffer is already mono.
 * Returns 'Stereo' if buffer is found to be actually stereo.
 * Returns 'FakeStereo' if buffer is found to have sufficiently identical stereo channels.
 */
AudioResult isRealStereo(AudioFile<double> *w) {
    // Check if already mono
    if (w->isMono()) {
        return Mono;
    }
    // Default the result
    AudioResult result = FakeStereo;
    
    // Go through every sample in the buffer
    for (int i = 0; i < w->getNumSamplesPerChannel(); i++) {
        // Take one sample from left buffer
        double leftSample = w->samples[0][i];
        // Take one sample from right buffer
        double rightSample = w->samples[1][i];
        // Compare the two
        if (!compareFloat(leftSample, rightSample)) {
            // If the two samples do not match, set the result to stereo and break from the loop.
            result = Stereo;
            break;
        }
    }
    
    return result;
}

/**
 * Opens an Open File Dialog and returns a list of file paths selected.
 */ 
vector<string> showOpenDialog() {
    /*auto selection = pfd::open_file("Select a file", ".",
                                { "Wave Files", "*.wav" },
                                pfd::opt::multiselect).result();*/
    vector<string> selection;
    char const * filter[2] = {"*.wav", "*.aiff", "*.aif"};
    auto result = tinyfd_openFileDialog("Select Audio File(s)", "", 3, filter, "Audio Files", 1);

    if (result != NULL) {
        string str_result(result);
        str_result += "|";
        auto del_index = str_result.find("|");

        
            do {
                
                auto one_file = str_result.substr(0, del_index);
                selection.push_back(one_file);
                printf("%s\n", one_file.c_str());
                str_result = str_result.substr(del_index+1, str_result.length());
                del_index = str_result.find("|");
                
            } while(del_index != std::string::npos);
        
        
    }
    return selection;
}

/**
 * Opens a Choose Folder Dialog and returns the path of the folder chosen.
 */
string showSaveDialog() {
    //string selection = pfd::select_folder("Select a folder to save.").result();
    auto result = tinyfd_selectFolderDialog("Select a folder to save.", "");
    if (result != NULL) {
        return string(result);
    } else {
        return "";
    }
}

/**
 * Cleans a file name by removing the full directory.
 * Example: /User/Albums/Doolittle/debaser.wav --> debaser.wav
 */ 
string cleanFileName(string file) {
    size_t index = 0;
    index = file.find_last_of('/');
    if (index == string::npos) {
        index = file.find_last_of('\\');
    }
    file = file.substr(index+1, file.size());
    return file;
}

/**
 * Processes and saves an audio buffer from a given file path.
 * Saves to given savePath.
 */ 
AudioResult processSingle(string file, string savePath) {
    // Load the audio file
    AudioFile<double> wav;
    wav.load(file);
    // Do the stereo checking operation 
    AudioResult result = isRealStereo(&wav);
    // Build new save path string
    string saveTo = savePath + "/" + cleanFileName(file);
    
    // Check if that file name already exists.
    std::ifstream f(saveTo.c_str());
    if(f.good()) {
        // append a new to the save path to potentially prevent overriding user files.
        saveTo = savePath + "/NEW-" + cleanFileName(file);
    }
    f.close();

    // Set proper number of channels for the new wave file
    if (result != Stereo) {
        wav.setNumChannels(1);
    }
    // Save the processed file.
    wav.save(saveTo);

    return result;
}

/**
 * Processes a whole batch of audio files from given paths.
 * Saves to given savePath.
 * Returns the number of fake stereo files found.
 */ 
int processAll(vector<string> files, string savePath) {
    int numFakeStereo = 0;
    for (size_t i = 0; i < files.size(); i++) {
        AudioResult result = processSingle(files[i], savePath);
        if (result == FakeStereo) {
            numFakeStereo++;
        }
    }
    return numFakeStereo;
}