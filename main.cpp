/** 
    Mono Catcher
    Matthew Kleitz, 2021
*/
#include "include/raylib.h"
#include "monoc.h"
#include <string>
#include <fstream>
#include <streambuf>
#include <regex>
#include <thread>
//#include "include/mingw-threads/mingw.thread.h"
#include <future>
#include <mutex>
using std::string;
using std::vector;
//using std::mutex;
std::mutex mtx;

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
    b.color = h ? DARKBLUE : BLUE;
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


int buttonThread(AppState& state) {
    
    
    while (true) {
        //mtx.lock();

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
       // mtx.unlock();
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
    
    auto ui_thread = std::thread(buttonThread, std::ref(state));
    
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
    ui_thread.join();
    
    CloseWindow();
    return 0;
}