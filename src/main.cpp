#include "ofMain.h"
#include "ofApp.h"
#include "GuiApp.h"
#include "ofAppGLFWWindow.h"

int main() {
    ofGLFWWindowSettings settings;
    settings.setGLVersion(4, 6);

    // GUI WINDOW
    settings.setSize(1280, 720);
    settings.setPosition(glm::vec2(0, 0));
    settings.resizable = true;
    settings.decorated = true;
    shared_ptr<ofAppBaseWindow> guiWindow = ofCreateWindow(settings);
    guiWindow->setWindowTitle("Gravity Waaaves - Control");

    // OUTPUT WINDOW
    // For single monitor testing, offset to overlap or use smaller size
    settings.setSize(1280, 720);
    settings.setPosition(glm::vec2(100, 100));  // Adjust based on your setup
    settings.resizable = true;
    settings.decorated = true;  // Set to true for testing, false for fullscreen output
    shared_ptr<ofAppBaseWindow> mainWindow = ofCreateWindow(settings);
    mainWindow->setWindowTitle("Gravity Waaaves - Output");

    // Create and link apps
    shared_ptr<ofApp> mainApp(new ofApp);
    shared_ptr<GuiApp> guiApp(new GuiApp);
    mainApp->gui = guiApp;
    guiApp->mainApp = mainApp.get();
    mainApp->mainWindow = mainWindow;

    ofRunApp(guiWindow, guiApp);
    ofRunApp(mainWindow, mainApp);
    ofRunMainLoop();
}
