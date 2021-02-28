
/** 
    Mono Catcher
    Matthew Kleitz, 2021
*/
#include "AudioFile.h"
#include "pfd.h"
#include "raylib.h"
#include <string>
#include <fstream>
#include <streambuf>
#include <regex>
#include <thread>
#include <future>
#include <mutex>
using std::string;
using std::vector;
// Degree of accuracy for comparing floats
const double EPSILON = 0.0001;

// Some other constants for ui colors
#define RAYBLUE (Color){ 10, 50, 200, 255 }
#define RAYDARKBLUE (Color){ 20, 20, 100, 255 }

// Represents a UI button on the screen
struct Button {
    Vector2 position;
    Vector2 size;
    bool clicked;
    bool hovered;
    bool enabled;
    Color color;
    bool changeMade;
};

// A result for processing an audio file.
enum AudioResult {
    Stereo, // When audio file is true stereo
    FakeStereo, // When audio file is 'fake' stereo
    Mono // When audio file is mono
};

/**
 * Draws a given button to the screen
 */
void drawButton(Button b, string text) {
    DrawRectangle(b.position.x, b.position.y, b.size.x, b.size.y, b.color);
    DrawText(text.c_str(), b.position.x + 5, b.position.y + b.size.y/2, b.size.y/2, RAYWHITE);
}

/**
 * Handles interaction between a button and the cursor.
 * Sets the clicked and hovered flags for a given button if necessary.
 */
Button handleMouse(Button b) {
    int mx = GetMouseX();
    int my = GetMouseY();
    int bx = b.position.x;
    int by = b.position.y;

    // Check if mouse is within the button's bounds
    bool h = (mx > bx && mx < bx + b.size.x && my > by && my < by + b.size.y);
    // Set hover color
    b.color = h ? RAYDARKBLUE : RAYBLUE;
    b.changeMade = b.hovered != h;
    b.hovered = h;
    // Check if clicked
    b.clicked = (h && IsMouseButtonPressed(MouseButton::MOUSE_LEFT_BUTTON));
    // Set properties for a disabled button
    if (!b.enabled) {
        b.color = LIGHTGRAY;
        b.clicked = false;
        b.hovered = false;
    }

    return b;
}

/**
 * Returns true is float a and float b are sufficiently close in value.
 * EPSILON constant defines the maximum difference between the two.
 */
bool compareFloat(float a, float b) {
    return abs(a - b) < EPSILON;
}

/**
 * Opens an Open File Dialog and returns a list of file paths selected.
 */ 
vector<string> showOpenDialog() {
    auto selection = pfd::open_file("Select a file", ".",
                                { "Wave Files", "*.wav" },
                                pfd::opt::multiselect).result();
    return selection;
}

/**
 * Opens a Choose Folder Dialog and returns the path of the folder chosen.
 */
string showSaveDialog() {
    string selection = pfd::select_folder("Select a folder to save.").result();
    return selection;
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
    for (int i = 0; i < files.size(); i++) {
        AudioResult result = processSingle(files[i], savePath);
        if (result == FakeStereo) {
            numFakeStereo++;
        }
    }
    return numFakeStereo;
}

class AppState {
public:
    // Setup the buttons for the GUI
    Button loadButton = { {10, 100}, {150, 50}, false, false, true};
    Button saveButton = { {10, 170}, {300, 50}, false, false, true};
    Button processButton = { {10, 300}, {150, 50}, false, false, false};
    Button resetButton = {  {300, 0}, {100, 25}, false, false, true};
    

    // Data for the app
    vector<string> files; // Stores audio files from file picker
    string savePath; // Stores save path from folder picker
    int numFake = -1; // Number of fakes found after the process completes.
    bool closingApp = false;
    bool processing = false;
};
std::mutex mtx;

int buttonThread(AppState& state) {
    
    
    while (true) {
        mtx.lock();

        // Handle mouse and button interactions.
        state.loadButton = handleMouse(state.loadButton);
        state.saveButton = handleMouse(state.saveButton);
        state.processButton = handleMouse(state.processButton);
        state.resetButton = handleMouse(state.resetButton);

        // Handle when load button is clicked.
        if (state.loadButton.clicked) {
            state.files = showOpenDialog();
            // Disable the load button if any files are chosen
            state.loadButton.enabled = state.files.empty();
        }

        // Handle when save button is clicked.
        if (state.saveButton.clicked) {
            state.savePath = showSaveDialog();
            // Disable the button if any path is chosen.
            state.saveButton.enabled = state.savePath.empty();
        }

        // When files and a save path have been chosen, enable processing option.
        if (state.files.size() > 0 && state.savePath.empty() == false && !state.loadButton.enabled && !state.saveButton.enabled) {
            state.processButton.enabled = true;
        }
        // Process all files if process button clicked
        if (state.processButton.clicked) {
            state.processButton.enabled = false;
            state.processing = true;
            state.numFake = processAll(state.files, state.savePath);
            state.processing = false;
        }

        // Reset app data if the reset button is clicked.
        if (state.resetButton.clicked) {
            state.processButton.enabled = false;
            state.saveButton.enabled = true;
            state.loadButton.enabled = true;
            state.numFake = -1;
            state.files.clear();
            state.savePath = "";
        }
        if (state.closingApp) {
            break;
        }
        mtx.unlock();
    }
    return 0;
}


int main(void)
{
    // Window initialization stuff
    const int screenWidth = 400;
    const int screenHeight = 400;
    InitWindow(screenWidth, screenHeight, "Mono Catcher");
    SetTargetFPS(60);
    
    AppState state;
    
    auto ui_thread = std::async(buttonThread, std::ref(state));
    
    int dotCount = 0;

    // Window loop
    while (!WindowShouldClose())
    {
        
        BeginDrawing();
        //mtx.lock();
        ClearBackground(RAYWHITE);

        // Display message for number of files chosen.
        if (state.files.size() > 0) {
            string msg = "Files chosen: " + std::to_string(state.files.size());
            DrawText(msg.c_str(), 200, 105, 24, BLACK);
        }
        // Display message for save directory chosen.
        if (state.savePath.empty() == false) {
            DrawText(state.savePath.c_str(), 10, 250, 12, BLACK);
        }
        // Display message for number of fake files found
        if (state.numFake > -1) {
            string msg = std::to_string(state.numFake) + " fake stereo files converted to mono.";
            DrawText(msg.c_str(), 10, 375, 18, DARKGREEN);
        }
        // Draw main UI components.
        DrawText("Mono Catcher", 15, 15, 20, BLACK);
        drawButton(state.loadButton, "Load files...");
        drawButton(state.saveButton, "Choose Save Folder...");
        drawButton(state.processButton, "Process!");
        drawButton(state.resetButton, "Reset");
        
        // Draw a little label for when it's processing.
        if (state.processing) {
            dotCount++;
            if (dotCount > 5) {
                dotCount = 0;
            }
            string msg = "Processing";
            for (int i = 0; i < dotCount; i++) {
                msg += ".";
            }
            DrawText(msg.c_str(), 15, 370, 20, BLUE);
        }
        
        EndDrawing(); 
    }
    state.closingApp = true;
    ui_thread.wait();
    
    CloseWindow();
    return 0;
}