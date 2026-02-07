/*
 * Copyright (c) 2013 Dan Wilcox <danomatika@gmail.com>
 *
 * BSD Simplified License.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 *
 * See https://github.com/danomatika/ofxMidi for documentation
 *
 */

#pragma once

// Forward declaration
class ofApp;

// Forward declaration
class ofApp;

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxImGui.h"
#include <map>
#include <atomic>

#if defined(TARGET_WIN32)
#define OFAPP_HAS_SPOUT 1
#else
#define OFAPP_HAS_SPOUT 0
#endif

#define PARAMETER_ARRAY_LENGTH 16

// OSC Parameter types
enum class OscParamType {
    FLOAT,
    BOOL,
    INT
};

// Struct to bind a parameter to its OSC address
struct OscParameter {
    std::string address;
    OscParamType type;
    union {
        float* floatPtr;
        bool* boolPtr;
        int* intPtr;
    };

    // Constructors for each type
    OscParameter(const std::string& addr, float* ptr)
        : address(addr), type(OscParamType::FLOAT), floatPtr(ptr) {}
    OscParameter(const std::string& addr, bool* ptr)
        : address(addr), type(OscParamType::BOOL), boolPtr(ptr) {}
    OscParameter(const std::string& addr, int* ptr)
        : address(addr), type(OscParamType::INT), intPtr(ptr) {}
    OscParameter() : address(""), type(OscParamType::FLOAT), floatPtr(nullptr) {}

    // Get value as float (for sending)
    float getValueAsFloat() const {
        switch(type) {
            case OscParamType::FLOAT: return floatPtr ? *floatPtr : 0.0f;
            case OscParamType::BOOL: return boolPtr ? (*boolPtr ? 1.0f : 0.0f) : 0.0f;
            case OscParamType::INT: return intPtr ? static_cast<float>(*intPtr) : 0.0f;
        }
        return 0.0f;
    }

    // Set value from float (for receiving)
    void setValueFromFloat(float value) {
        switch(type) {
            case OscParamType::FLOAT:
                if(floatPtr) *floatPtr = value;
                break;
            case OscParamType::BOOL:
                if(boolPtr) *boolPtr = (value > 0.5f);
                break;
            case OscParamType::INT:
                if(intPtr) *intPtr = static_cast<int>(value);
                break;
        }
    }
};

// OSC-enabled ImGui wrapper macro
#define ImGuiSliderFloatOSC(label, var, min, max, address) \
	if (ImGui::SliderFloat(label, var, min, max)) { \
		if (mainApp) mainApp->sendOscParameter(address, *(var)); \
	}

class GuiApp: public ofBaseApp, public ofxMidiListener {
public:

	// Pointer to main app for OSC reloading
	class ofApp* mainApp;

	// Reference to GUI window for fullscreen/decoration toggling
	shared_ptr<ofAppBaseWindow> guiWindow;
	void sendOscIfChanged(const string& address, float value);
	void setup();
	void update();
	void draw();
	void exit();
	void keyPressed(int key);
	void keyReleased(int key);


	/**midi!**/
	void midibiz();
	void midiSetup();
	void newMidiMessage(ofxMidiMessage& eventArgs);
	ofxMidiIn* midiIn;
	std::vector<ofxMidiMessage> midiMessages;
	std::size_t maxMessages = 10;

	//new stuff 2-9-24
	//save states
	bool saveALL=0;
	bool loadALL=0;

	bool saveBLOCK1=0;
	bool loadBLOCK1=0;

	int saveStateSelectSwitch=0;
	int loadStateSelectSwitch=0;

	// ========== PRESET BANK SYSTEM ==========
	// Bank management
	std::vector<std::string> bankNames;
	std::vector<const char*> bankNamesChar;

	// Separate bank indices for save and load
	int saveBankIndex = 0;
	int loadBankIndex = 0;
	std::string saveBankPath = "presets/Default";
	std::string loadBankPath = "presets/Default";

	// Save bank presets (clean display names)
	std::vector<std::string> savePresetDisplayNames;
	std::vector<const char*> savePresetDisplayNamesChar;
	std::vector<std::string> savePresetFileNames;
	int savePresetCount = 0;

	// Load bank presets (clean display names)
	std::vector<std::string> loadPresetDisplayNames;
	std::vector<const char*> loadPresetDisplayNamesChar;
	std::vector<std::string> loadPresetFileNames;
	int loadPresetCount = 0;

	// Name input for save
	char newPresetNameBuffer[128] = "";
	std::string originalPresetName = "";  // Track original name for rename detection

	// UI Scale (labels: 200%/250%/300%, actual: 2.0/2.5/3.0)
	int uiScaleIndex = 0;  // Default "200%" (2.0x actual scale)
	float uiScaleValues[3] = {2.0f, 2.5f, 3.0f};

	// Preset bank functions
	void scanBanks();
	void switchSaveBank(int bankIndex);
	void switchLoadBank(int bankIndex);
	void indexSavePresets();
	void indexLoadPresets();
	void savePresetAs(const std::string& name);
	bool renamePreset(int presetIndex, const std::string& newName);
	std::string cleanDisplayName(const std::string& filename);
	void migrateOldSaveStates();

	// Legacy function (kept for compatibility)
	void indexSaveStateNames();

	void saveBLOCK_1();

	void saveEverything();

	void loadBLOCK_1();

	void loadEverything();

	void arrayToJsonList(ofJson jsonFile, string blockName, string listName, float inArray[]);
	void arrayToJsonList(string listName, float inArray[]);//assume: theres a global existing json file we refer to.
	void jsonListToArray(ofJson jsonFile, string listName, float outArray[]);
	//both of these above functions assume a globally defined constant or #defined arraylength
	// and neighter of them are being used as of 2-14-24 lol


	//midi macros
	bool macroDataMidiGui=0;
	bool macroDataLearnSwitch=0;
	bool macroDataReset=0;

	bool macroDataResetAssignments=0;

	void macroDataResetEverything();

	float macroData[PARAMETER_ARRAY_LENGTH];
	float * macroDataPointers[PARAMETER_ARRAY_LENGTH];
	bool macroDataMidiActive[PARAMETER_ARRAY_LENGTH];
	string macroDataNames[PARAMETER_ARRAY_LENGTH];

	//misc
	float fb1DelayTimeMacroBuffer=0;
	float fb2DelayTimeMacroBuffer=0;

	// OSC Settings
	bool oscEnabled = false;
	int oscReceivePort = 7000;
	char oscSendIP[64] = "127.0.0.1";
	int oscSendPort = 7001;
	bool oscConnected = false;
	bool sendAllOscValues = false;  // Trigger for sending all OSC values
	bool oscSettingsReloadRequested = false;  // Trigger for reloading OSC settings
	void updateLocalIP();
	std::vector<std::string> localIPs;  // All available network IPs

	// Video Input Settings
	std::vector<ofVideoDevice> videoDevices;
	std::vector<std::string> videoDeviceNames;
	int input1DeviceID = 0;
	int input2DeviceID = 1;
	bool reinitializeInputs = false;
	void refreshVideoDevices();

	// NDI Input Settings
#if OFAPP_HAS_SPOUT
	int input1SourceType = 0;  // 0 = Webcam, 1 = NDI, 2 = Spout
	int input2SourceType = 0;  // 0 = Webcam, 1 = NDI, 2 = Spout
#else
	int input1SourceType = 0;  // 0 = Webcam, 1 = NDI
	int input2SourceType = 0;  // 0 = Webcam, 1 = NDI
#endif
	std::vector<std::string> ndiSourceNames;
	int input1NdiSourceIndex = 0;
	int input2NdiSourceIndex = 0;
	bool refreshNdiSources = false;

	// Spout Input Settings (stub implementation needs spoutSourceNames vector on all platforms)
	std::vector<std::string> spoutSourceNames;  // Available Spout senders
#if OFAPP_HAS_SPOUT
	int input1SpoutSourceIndex = 0;
	int input2SpoutSourceIndex = 0;
	bool refreshSpoutSources = false;

	// Spout Output Settings
	bool spoutSendBlock1 = false;  // Enable Spout output for Block 1
	bool spoutSendBlock2 = false;  // Enable Spout output for Block 2
	bool spoutSendBlock3 = false;  // Enable Spout output for Block 3 (final)
#endif

	// NDI Output Settings
	bool ndiSendBlock1 = false;  // Enable NDI output for Block 1
	bool ndiSendBlock2 = false;  // Enable NDI output for Block 2
	bool ndiSendBlock3 = false;  // Enable NDI output for Block 3 (final)

	// NDI send resolution
	int ndiSendWidth = 1280;
	int ndiSendHeight = 720;

	// Performance Settings
	int targetFPS = 30;  // Target frame rate (1-60)
	bool fpsChangeRequested = false;  // Flag to apply FPS change in main app

	// Resolution Settings
	// Input resolutions (for webcam/NDI/Spout capture scaling)
	int input1Width = 640;
	int input1Height = 480;
	int input2Width = 640;
	int input2Height = 480;

	// Internal processing resolution (framebuffers, pastFrames)
	int internalWidth = 1280;
	int internalHeight = 720;

	// Output resolution (final render to window)
	int outputWidth = 1280;
	int outputHeight = 720;

#if OFAPP_HAS_SPOUT
	// Spout send resolution (separate from internal)
	int spoutSendWidth = 1280;
	int spoutSendHeight = 720;
#endif

	bool resolutionChangeRequested = false;

	//block1
	const int ch1AdjustLength=15;
	const int ch2MixAndKeyLength=6;
	const int ch2AdjustLength=15;

	const int ch1AdjustLfoLength=16;
	const int ch2MixAndKeyLfoLength=6;
	const int ch2AdjustLfoLength=16;

	const int fb1MixAndKeyLength=6;
	const int fb1Geo1Length=10;
	const int fb1Color1Length=11;
	const int fb1FiltersLength=9;

	const int fb1MixAndKeyLfoLength=6;
	const int fb1Geo1Lfo1Length=8;
	const int fb1Geo1Lfo2Length=10;
	const int fb1Color1Lfo1Length=6;

	//block2
	const int block2InputAdjustLength=15;
	const int block2InputAdjustLfoLength=16;

	const int fb2MixAndKeyLength=6;
	const int fb2Geo1Length=10;
	const int fb2Color1Length=11;
	const int fb2FiltersLength=9;

	const int fb2MixAndKeyLfoLength=6;
	const int fb2Geo1Lfo1Length=8;
	const int fb2Geo1Lfo2Length=10;
	const int fb2Color1Lfo1Length=6;

	//block3
	const int block1GeoLength=10;
	const int block1ColorizeLength=15;
	const int block1FiltersLength=5;

	const int block1Geo1Lfo1Length=8;
	const int block1Geo1Lfo2Length=10;
	const int block1ColorizeLfo1Length=12;
	const int block1ColorizeLfo2Length=12;
	const int block1ColorizeLfo3Length=6;

	const int block2GeoLength=10;
	const int block2ColorizeLength=15;
	const int block2FiltersLength=5;

	const int block2Geo1Lfo1Length=8;
	const int block2Geo1Lfo2Length=10;
	const int block2ColorizeLfo1Length=12;
	const int block2ColorizeLfo2Length=12;
	const int block2ColorizeLfo3Length=6;

	const int matrixMixLength=9;
	const int finalMixAndKeyLength=6;

	const int matrixMixLfo1Length=12;
	const int matrixMixLfo2Length=6;
	const int finalMixAndKeyLfoLength=6;


	//NEW macroMapping procedures

	//BLOCK1
	void macroBLOCK_1Ch1Adjust(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Ch2MixAndKey(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Ch2Adjust(int parameterIndex, int macroDataIndex);

	void macroBLOCK_1Ch1AdjustLfo(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Ch2MixAndKeyLfo(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Ch2AdjustLfo(int parameterIndex, int macroDataIndex);

	void macroBLOCK_1Fb1MixAndKey(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Fb1Geo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Fb1Color1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Fb1Filters(int parameterIndex, int macroDataIndex);

	void macroBLOCK_1Fb1MixAndKeyLfo(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Fb1Geo1Lfo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Fb1Geo1Lfo2(int parameterIndex, int macroDataIndex);
	void macroBLOCK_1Fb1Color1Lfo1(int parameterIndex, int macroDataIndex);

	//BLOCK2
	void macroBLOCK_2Block2InputAdjust(int parameterIndex, int macroDataIndex);
	void macroBLOCK_2Block2InputAdjustLfo(int parameterIndex, int macroDataIndex);

	void macroBLOCK_2Fb2MixAndKey(int parameterIndex, int macroDataIndex);
	void macroBLOCK_2Fb2Geo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_2Fb2Color1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_2Fb2Filters(int parameterIndex, int macroDataIndex);

	void macroBLOCK_2Fb2MixAndKeyLfo(int parameterIndex, int macroDataIndex);
	void macroBLOCK_2Fb2Geo1Lfo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_2Fb2Geo1Lfo2(int parameterIndex, int macroDataIndex);
	void macroBLOCK_2Fb2Color1Lfo1(int parameterIndex, int macroDataIndex);

	//BLOCK3
	void macroBLOCK_3Block1Geo(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block1Colorize(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block1Filters(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block1Geo1Lfo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block1Geo1Lfo2(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block1ColorizeLfo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block1ColorizeLfo2(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block1ColorizeLfo3(int parameterIndex, int macroDataIndex);

	void macroBLOCK_3Block2Geo(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block2Colorize(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block2Filters(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block2Geo1Lfo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block2Geo1Lfo2(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block2ColorizeLfo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block2ColorizeLfo2(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3Block2ColorizeLfo3(int parameterIndex, int macroDataIndex);

	void macroBLOCK_3MatrixMix(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3FinalMixAndKey(int parameterIndex, int macroDataIndex);

	void macroBLOCK_3MatrixMixLfo1(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3MatrixMixLfo2(int parameterIndex, int macroDataIndex);
	void macroBLOCK_3FinalMixAndKeyLfo(int parameterIndex, int macroDataIndex);


	void macroBLOCK_1Fb1DelayTime(int macroDataIndex);
	void macroBLOCK_1Fb2DelayTime(int macroDataIndex);

	//BLOCK1
	void initializeNames();

	string ch1AdjustNames[PARAMETER_ARRAY_LENGTH];
	string ch2MixAndKeyNames[PARAMETER_ARRAY_LENGTH];
	string ch2AdjustNames[PARAMETER_ARRAY_LENGTH];

	string ch1AdjustLfoNames[PARAMETER_ARRAY_LENGTH];
	string ch2MixAndKeyLfoNames[PARAMETER_ARRAY_LENGTH];
	string ch2AdjustLfoNames[PARAMETER_ARRAY_LENGTH];

	string fb1DelayTimeName="fb1 delay time";

	string fb1MixAndKeyNames[PARAMETER_ARRAY_LENGTH];
	string fb1Geo1Names[PARAMETER_ARRAY_LENGTH];
	string fb1Color1Names[PARAMETER_ARRAY_LENGTH];
	string fb1FiltersNames[PARAMETER_ARRAY_LENGTH];

	string fb1MixAndKeyLfoNames[PARAMETER_ARRAY_LENGTH];
	string fb1Geo1Lfo1Names[PARAMETER_ARRAY_LENGTH];
	string fb1Geo1Lfo2Names[PARAMETER_ARRAY_LENGTH];
	string fb1Color1Lfo1Names[PARAMETER_ARRAY_LENGTH];


	//block2
	string block2InputAdjustNames[PARAMETER_ARRAY_LENGTH];
	string block2InputAdjustLfoNames[PARAMETER_ARRAY_LENGTH];

	string fb2MixAndKeyNames[PARAMETER_ARRAY_LENGTH];
	string fb2Geo1Names[PARAMETER_ARRAY_LENGTH];
	string fb2Color1Names[PARAMETER_ARRAY_LENGTH];
	string fb2FiltersNames[PARAMETER_ARRAY_LENGTH];

	string fb2DelayTimeName="fb2 delay time";

	string fb2MixAndKeyLfoNames[PARAMETER_ARRAY_LENGTH];
	string fb2Geo1Lfo1Names[PARAMETER_ARRAY_LENGTH];
	string fb2Geo1Lfo2Names[PARAMETER_ARRAY_LENGTH];
	string fb2Color1Lfo1Names[PARAMETER_ARRAY_LENGTH];

	//block3
	string block1GeoNames[PARAMETER_ARRAY_LENGTH];
	string block1ColorizeNames[PARAMETER_ARRAY_LENGTH];
	string block1FiltersNames[PARAMETER_ARRAY_LENGTH];
	string block1Geo1Lfo1Names[PARAMETER_ARRAY_LENGTH];
	string block1Geo1Lfo2Names[PARAMETER_ARRAY_LENGTH];
	string block1ColorizeLfo1Names[PARAMETER_ARRAY_LENGTH];
	string block1ColorizeLfo2Names[PARAMETER_ARRAY_LENGTH];
	string block1ColorizeLfo3Names[PARAMETER_ARRAY_LENGTH];

	string block2GeoNames[PARAMETER_ARRAY_LENGTH];
	string block2ColorizeNames[PARAMETER_ARRAY_LENGTH];
	string block2FiltersNames[PARAMETER_ARRAY_LENGTH];
	string block2Geo1Lfo1Names[PARAMETER_ARRAY_LENGTH];
	string block2Geo1Lfo2Names[PARAMETER_ARRAY_LENGTH];
	string block2ColorizeLfo1Names[PARAMETER_ARRAY_LENGTH];
	string block2ColorizeLfo2Names[PARAMETER_ARRAY_LENGTH];
	string block2ColorizeLfo3Names[PARAMETER_ARRAY_LENGTH];

	string matrixMixNames[PARAMETER_ARRAY_LENGTH];
	string finalMixAndKeyNames[PARAMETER_ARRAY_LENGTH];
	string matrixMixLfo1Names[PARAMETER_ARRAY_LENGTH];
	string matrixMixLfo2Names[PARAMETER_ARRAY_LENGTH];
	string finalMixAndKeyLfoNames[PARAMETER_ARRAY_LENGTH];


	//macro0
	int selectMacro0Ch1Adjust=0;
	int selectMacro0Ch2MixAndKey=0;
	int selectMacro0Ch2Adjust=0;
	int selectMacro0Ch1AdjustLfo=0;
	int selectMacro0Ch2MixAndKeyLfo=0;
	int selectMacro0Ch2AdjustLfo=0;
	int selectMacro0Fb1MixAndKey=0;
	int selectMacro0Fb1Geo1=0;
	int selectMacro0Fb1Color1=0;
	int selectMacro0Fb1Filters=0;
	int selectMacro0Fb1MixAndKeyLfo=0;
	int selectMacro0Fb1Geo1Lfo1=0;
	int selectMacro0Fb1Geo1Lfo2=0;
	int selectMacro0Fb1Color1Lfo1=0;

	int selectMacro0Fb1DelayTime=0;
	//b2
	int selectMacro0Block2InputAdjust=0;
	int selectMacro0Block2InputAdjustLfo=0;
	int selectMacro0Fb2MixAndKey=0;
	int selectMacro0Fb2Geo1=0;
	int selectMacro0Fb2Color1=0;
	int selectMacro0Fb2Filters=0;
	int selectMacro0Fb2MixAndKeyLfo=0;
	int selectMacro0Fb2Geo1Lfo1=0;
	int selectMacro0Fb2Geo1Lfo2=0;
	int selectMacro0Fb2Color1Lfo1=0;

	int selectMacro0Fb2DelayTime=0;
	//b3
	int selectMacro0Block1Geo=0;
	int selectMacro0Block1Colorize=0;
	int selectMacro0Block1Filters=0;
	int selectMacro0Block1Geo1Lfo1=0;
	int selectMacro0Block1Geo1Lfo2=0;
	int selectMacro0Block1ColorizeLfo1=0;
	int selectMacro0Block1ColorizeLfo2=0;
	int selectMacro0Block1ColorizeLfo3=0;
	int selectMacro0Block2Geo=0;
	int selectMacro0Block2Colorize=0;
	int selectMacro0Block2Filters=0;
	int selectMacro0Block2Geo1Lfo1=0;
	int selectMacro0Block2Geo1Lfo2=0;
	int selectMacro0Block2ColorizeLfo1=0;
	int selectMacro0Block2ColorizeLfo2=0;
	int selectMacro0Block2ColorizeLfo3=0;
	int selectMacro0MatrixMix=0;
	int selectMacro0FinalMixAndKey=0;
	int selectMacro0MatrixMixLfo1=0;
	int selectMacro0MatrixMixLfo2=0;
	int selectMacro0FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro0Menu();
	void ifSelectMacro0();





	//macro1
	int selectMacro1Ch1Adjust=0;
	int selectMacro1Ch2MixAndKey=0;
	int selectMacro1Ch2Adjust=0;
	int selectMacro1Ch1AdjustLfo=0;
	int selectMacro1Ch2MixAndKeyLfo=0;
	int selectMacro1Ch2AdjustLfo=0;
	int selectMacro1Fb1MixAndKey=0;
	int selectMacro1Fb1Geo1=0;
	int selectMacro1Fb1Color1=0;
	int selectMacro1Fb1Filters=0;
	int selectMacro1Fb1MixAndKeyLfo=0;
	int selectMacro1Fb1Geo1Lfo1=0;
	int selectMacro1Fb1Geo1Lfo2=0;
	int selectMacro1Fb1Color1Lfo1=0;

	int selectMacro1Fb1DelayTime=0;
	//b2
	int selectMacro1Block2InputAdjust=0;
	int selectMacro1Block2InputAdjustLfo=0;
	int selectMacro1Fb2MixAndKey=0;
	int selectMacro1Fb2Geo1=0;
	int selectMacro1Fb2Color1=0;
	int selectMacro1Fb2Filters=0;
	int selectMacro1Fb2MixAndKeyLfo=0;
	int selectMacro1Fb2Geo1Lfo1=0;
	int selectMacro1Fb2Geo1Lfo2=0;
	int selectMacro1Fb2Color1Lfo1=0;

	int selectMacro1Fb2DelayTime=0;
	//b3
	int selectMacro1Block1Geo=0;
	int selectMacro1Block1Colorize=0;
	int selectMacro1Block1Filters=0;
	int selectMacro1Block1Geo1Lfo1=0;
	int selectMacro1Block1Geo1Lfo2=0;
	int selectMacro1Block1ColorizeLfo1=0;
	int selectMacro1Block1ColorizeLfo2=0;
	int selectMacro1Block1ColorizeLfo3=0;
	int selectMacro1Block2Geo=0;
	int selectMacro1Block2Colorize=0;
	int selectMacro1Block2Filters=0;
	int selectMacro1Block2Geo1Lfo1=0;
	int selectMacro1Block2Geo1Lfo2=0;
	int selectMacro1Block2ColorizeLfo1=0;
	int selectMacro1Block2ColorizeLfo2=0;
	int selectMacro1Block2ColorizeLfo3=0;
	int selectMacro1MatrixMix=0;
	int selectMacro1FinalMixAndKey=0;
	int selectMacro1MatrixMixLfo1=0;
	int selectMacro1MatrixMixLfo2=0;
	int selectMacro1FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro1Menu();
	void ifSelectMacro1();


	//macro2
	int selectMacro2Ch1Adjust=0;
	int selectMacro2Ch2MixAndKey=0;
	int selectMacro2Ch2Adjust=0;
	int selectMacro2Ch1AdjustLfo=0;
	int selectMacro2Ch2MixAndKeyLfo=0;
	int selectMacro2Ch2AdjustLfo=0;
	int selectMacro2Fb1MixAndKey=0;
	int selectMacro2Fb1Geo1=0;
	int selectMacro2Fb1Color1=0;
	int selectMacro2Fb1Filters=0;
	int selectMacro2Fb1MixAndKeyLfo=0;
	int selectMacro2Fb1Geo1Lfo1=0;
	int selectMacro2Fb1Geo1Lfo2=0;
	int selectMacro2Fb1Color1Lfo1=0;

	int selectMacro2Fb1DelayTime=0;
	//b2
	int selectMacro2Block2InputAdjust=0;
	int selectMacro2Block2InputAdjustLfo=0;
	int selectMacro2Fb2MixAndKey=0;
	int selectMacro2Fb2Geo1=0;
	int selectMacro2Fb2Color1=0;
	int selectMacro2Fb2Filters=0;
	int selectMacro2Fb2MixAndKeyLfo=0;
	int selectMacro2Fb2Geo1Lfo1=0;
	int selectMacro2Fb2Geo1Lfo2=0;
	int selectMacro2Fb2Color1Lfo1=0;

	int selectMacro2Fb2DelayTime=0;
	//b3
	int selectMacro2Block1Geo=0;
	int selectMacro2Block1Colorize=0;
	int selectMacro2Block1Filters=0;
	int selectMacro2Block1Geo1Lfo1=0;
	int selectMacro2Block1Geo1Lfo2=0;
	int selectMacro2Block1ColorizeLfo1=0;
	int selectMacro2Block1ColorizeLfo2=0;
	int selectMacro2Block1ColorizeLfo3=0;
	int selectMacro2Block2Geo=0;
	int selectMacro2Block2Colorize=0;
	int selectMacro2Block2Filters=0;
	int selectMacro2Block2Geo1Lfo1=0;
	int selectMacro2Block2Geo1Lfo2=0;
	int selectMacro2Block2ColorizeLfo1=0;
	int selectMacro2Block2ColorizeLfo2=0;
	int selectMacro2Block2ColorizeLfo3=0;
	int selectMacro2MatrixMix=0;
	int selectMacro2FinalMixAndKey=0;
	int selectMacro2MatrixMixLfo1=0;
	int selectMacro2MatrixMixLfo2=0;
	int selectMacro2FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro2Menu();
	void ifSelectMacro2();

	//macro3
	int selectMacro3Ch1Adjust=0;
	int selectMacro3Ch2MixAndKey=0;
	int selectMacro3Ch2Adjust=0;
	int selectMacro3Ch1AdjustLfo=0;
	int selectMacro3Ch2MixAndKeyLfo=0;
	int selectMacro3Ch2AdjustLfo=0;
	int selectMacro3Fb1MixAndKey=0;
	int selectMacro3Fb1Geo1=0;
	int selectMacro3Fb1Color1=0;
	int selectMacro3Fb1Filters=0;
	int selectMacro3Fb1MixAndKeyLfo=0;
	int selectMacro3Fb1Geo1Lfo1=0;
	int selectMacro3Fb1Geo1Lfo2=0;
	int selectMacro3Fb1Color1Lfo1=0;

	int selectMacro3Fb1DelayTime=0;
	//b2
	int selectMacro3Block2InputAdjust=0;
	int selectMacro3Block2InputAdjustLfo=0;
	int selectMacro3Fb2MixAndKey=0;
	int selectMacro3Fb2Geo1=0;
	int selectMacro3Fb2Color1=0;
	int selectMacro3Fb2Filters=0;
	int selectMacro3Fb2MixAndKeyLfo=0;
	int selectMacro3Fb2Geo1Lfo1=0;
	int selectMacro3Fb2Geo1Lfo2=0;
	int selectMacro3Fb2Color1Lfo1=0;

	int selectMacro3Fb2DelayTime=0;
	//b3
	int selectMacro3Block1Geo=0;
	int selectMacro3Block1Colorize=0;
	int selectMacro3Block1Filters=0;
	int selectMacro3Block1Geo1Lfo1=0;
	int selectMacro3Block1Geo1Lfo2=0;
	int selectMacro3Block1ColorizeLfo1=0;
	int selectMacro3Block1ColorizeLfo2=0;
	int selectMacro3Block1ColorizeLfo3=0;
	int selectMacro3Block2Geo=0;
	int selectMacro3Block2Colorize=0;
	int selectMacro3Block2Filters=0;
	int selectMacro3Block2Geo1Lfo1=0;
	int selectMacro3Block2Geo1Lfo2=0;
	int selectMacro3Block2ColorizeLfo1=0;
	int selectMacro3Block2ColorizeLfo2=0;
	int selectMacro3Block2ColorizeLfo3=0;
	int selectMacro3MatrixMix=0;
	int selectMacro3FinalMixAndKey=0;
	int selectMacro3MatrixMixLfo1=0;
	int selectMacro3MatrixMixLfo2=0;
	int selectMacro3FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro3Menu();
	void ifSelectMacro3();

	//macro4
	int selectMacro4Ch1Adjust=0;
	int selectMacro4Ch2MixAndKey=0;
	int selectMacro4Ch2Adjust=0;
	int selectMacro4Ch1AdjustLfo=0;
	int selectMacro4Ch2MixAndKeyLfo=0;
	int selectMacro4Ch2AdjustLfo=0;
	int selectMacro4Fb1MixAndKey=0;
	int selectMacro4Fb1Geo1=0;
	int selectMacro4Fb1Color1=0;
	int selectMacro4Fb1Filters=0;
	int selectMacro4Fb1MixAndKeyLfo=0;
	int selectMacro4Fb1Geo1Lfo1=0;
	int selectMacro4Fb1Geo1Lfo2=0;
	int selectMacro4Fb1Color1Lfo1=0;

	int selectMacro4Fb1DelayTime=0;
	//b2
	int selectMacro4Block2InputAdjust=0;
	int selectMacro4Block2InputAdjustLfo=0;
	int selectMacro4Fb2MixAndKey=0;
	int selectMacro4Fb2Geo1=0;
	int selectMacro4Fb2Color1=0;
	int selectMacro4Fb2Filters=0;
	int selectMacro4Fb2MixAndKeyLfo=0;
	int selectMacro4Fb2Geo1Lfo1=0;
	int selectMacro4Fb2Geo1Lfo2=0;
	int selectMacro4Fb2Color1Lfo1=0;

	int selectMacro4Fb2DelayTime=0;
	//b3
	int selectMacro4Block1Geo=0;
	int selectMacro4Block1Colorize=0;
	int selectMacro4Block1Filters=0;
	int selectMacro4Block1Geo1Lfo1=0;
	int selectMacro4Block1Geo1Lfo2=0;
	int selectMacro4Block1ColorizeLfo1=0;
	int selectMacro4Block1ColorizeLfo2=0;
	int selectMacro4Block1ColorizeLfo3=0;
	int selectMacro4Block2Geo=0;
	int selectMacro4Block2Colorize=0;
	int selectMacro4Block2Filters=0;
	int selectMacro4Block2Geo1Lfo1=0;
	int selectMacro4Block2Geo1Lfo2=0;
	int selectMacro4Block2ColorizeLfo1=0;
	int selectMacro4Block2ColorizeLfo2=0;
	int selectMacro4Block2ColorizeLfo3=0;
	int selectMacro4MatrixMix=0;
	int selectMacro4FinalMixAndKey=0;
	int selectMacro4MatrixMixLfo1=0;
	int selectMacro4MatrixMixLfo2=0;
	int selectMacro4FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro4Menu();
	void ifSelectMacro4();

	//macro5
	int selectMacro5Ch1Adjust=0;
	int selectMacro5Ch2MixAndKey=0;
	int selectMacro5Ch2Adjust=0;
	int selectMacro5Ch1AdjustLfo=0;
	int selectMacro5Ch2MixAndKeyLfo=0;
	int selectMacro5Ch2AdjustLfo=0;
	int selectMacro5Fb1MixAndKey=0;
	int selectMacro5Fb1Geo1=0;
	int selectMacro5Fb1Color1=0;
	int selectMacro5Fb1Filters=0;
	int selectMacro5Fb1MixAndKeyLfo=0;
	int selectMacro5Fb1Geo1Lfo1=0;
	int selectMacro5Fb1Geo1Lfo2=0;
	int selectMacro5Fb1Color1Lfo1=0;

	int selectMacro5Fb1DelayTime=0;
	//b2
	int selectMacro5Block2InputAdjust=0;
	int selectMacro5Block2InputAdjustLfo=0;
	int selectMacro5Fb2MixAndKey=0;
	int selectMacro5Fb2Geo1=0;
	int selectMacro5Fb2Color1=0;
	int selectMacro5Fb2Filters=0;
	int selectMacro5Fb2MixAndKeyLfo=0;
	int selectMacro5Fb2Geo1Lfo1=0;
	int selectMacro5Fb2Geo1Lfo2=0;
	int selectMacro5Fb2Color1Lfo1=0;

	int selectMacro5Fb2DelayTime=0;
	//b3
	int selectMacro5Block1Geo=0;
	int selectMacro5Block1Colorize=0;
	int selectMacro5Block1Filters=0;
	int selectMacro5Block1Geo1Lfo1=0;
	int selectMacro5Block1Geo1Lfo2=0;
	int selectMacro5Block1ColorizeLfo1=0;
	int selectMacro5Block1ColorizeLfo2=0;
	int selectMacro5Block1ColorizeLfo3=0;
	int selectMacro5Block2Geo=0;
	int selectMacro5Block2Colorize=0;
	int selectMacro5Block2Filters=0;
	int selectMacro5Block2Geo1Lfo1=0;
	int selectMacro5Block2Geo1Lfo2=0;
	int selectMacro5Block2ColorizeLfo1=0;
	int selectMacro5Block2ColorizeLfo2=0;
	int selectMacro5Block2ColorizeLfo3=0;
	int selectMacro5MatrixMix=0;
	int selectMacro5FinalMixAndKey=0;
	int selectMacro5MatrixMixLfo1=0;
	int selectMacro5MatrixMixLfo2=0;
	int selectMacro5FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro5Menu();
	void ifSelectMacro5();

	//macro6
	int selectMacro6Ch1Adjust=0;
	int selectMacro6Ch2MixAndKey=0;
	int selectMacro6Ch2Adjust=0;
	int selectMacro6Ch1AdjustLfo=0;
	int selectMacro6Ch2MixAndKeyLfo=0;
	int selectMacro6Ch2AdjustLfo=0;
	int selectMacro6Fb1MixAndKey=0;
	int selectMacro6Fb1Geo1=0;
	int selectMacro6Fb1Color1=0;
	int selectMacro6Fb1Filters=0;
	int selectMacro6Fb1MixAndKeyLfo=0;
	int selectMacro6Fb1Geo1Lfo1=0;
	int selectMacro6Fb1Geo1Lfo2=0;
	int selectMacro6Fb1Color1Lfo1=0;

	int selectMacro6Fb1DelayTime=0;
	//b2
	int selectMacro6Block2InputAdjust=0;
	int selectMacro6Block2InputAdjustLfo=0;
	int selectMacro6Fb2MixAndKey=0;
	int selectMacro6Fb2Geo1=0;
	int selectMacro6Fb2Color1=0;
	int selectMacro6Fb2Filters=0;
	int selectMacro6Fb2MixAndKeyLfo=0;
	int selectMacro6Fb2Geo1Lfo1=0;
	int selectMacro6Fb2Geo1Lfo2=0;
	int selectMacro6Fb2Color1Lfo1=0;

	int selectMacro6Fb2DelayTime=0;
	//b3
	int selectMacro6Block1Geo=0;
	int selectMacro6Block1Colorize=0;
	int selectMacro6Block1Filters=0;
	int selectMacro6Block1Geo1Lfo1=0;
	int selectMacro6Block1Geo1Lfo2=0;
	int selectMacro6Block1ColorizeLfo1=0;
	int selectMacro6Block1ColorizeLfo2=0;
	int selectMacro6Block1ColorizeLfo3=0;
	int selectMacro6Block2Geo=0;
	int selectMacro6Block2Colorize=0;
	int selectMacro6Block2Filters=0;
	int selectMacro6Block2Geo1Lfo1=0;
	int selectMacro6Block2Geo1Lfo2=0;
	int selectMacro6Block2ColorizeLfo1=0;
	int selectMacro6Block2ColorizeLfo2=0;
	int selectMacro6Block2ColorizeLfo3=0;
	int selectMacro6MatrixMix=0;
	int selectMacro6FinalMixAndKey=0;
	int selectMacro6MatrixMixLfo1=0;
	int selectMacro6MatrixMixLfo2=0;
	int selectMacro6FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro6Menu();
	void ifSelectMacro6();

	//macro7
	int selectMacro7Ch1Adjust=0;
	int selectMacro7Ch2MixAndKey=0;
	int selectMacro7Ch2Adjust=0;
	int selectMacro7Ch1AdjustLfo=0;
	int selectMacro7Ch2MixAndKeyLfo=0;
	int selectMacro7Ch2AdjustLfo=0;
	int selectMacro7Fb1MixAndKey=0;
	int selectMacro7Fb1Geo1=0;
	int selectMacro7Fb1Color1=0;
	int selectMacro7Fb1Filters=0;
	int selectMacro7Fb1MixAndKeyLfo=0;
	int selectMacro7Fb1Geo1Lfo1=0;
	int selectMacro7Fb1Geo1Lfo2=0;
	int selectMacro7Fb1Color1Lfo1=0;

	int selectMacro7Fb1DelayTime=0;
	//b2
	int selectMacro7Block2InputAdjust=0;
	int selectMacro7Block2InputAdjustLfo=0;
	int selectMacro7Fb2MixAndKey=0;
	int selectMacro7Fb2Geo1=0;
	int selectMacro7Fb2Color1=0;
	int selectMacro7Fb2Filters=0;
	int selectMacro7Fb2MixAndKeyLfo=0;
	int selectMacro7Fb2Geo1Lfo1=0;
	int selectMacro7Fb2Geo1Lfo2=0;
	int selectMacro7Fb2Color1Lfo1=0;

	int selectMacro7Fb2DelayTime=0;
	//b3
	int selectMacro7Block1Geo=0;
	int selectMacro7Block1Colorize=0;
	int selectMacro7Block1Filters=0;
	int selectMacro7Block1Geo1Lfo1=0;
	int selectMacro7Block1Geo1Lfo2=0;
	int selectMacro7Block1ColorizeLfo1=0;
	int selectMacro7Block1ColorizeLfo2=0;
	int selectMacro7Block1ColorizeLfo3=0;
	int selectMacro7Block2Geo=0;
	int selectMacro7Block2Colorize=0;
	int selectMacro7Block2Filters=0;
	int selectMacro7Block2Geo1Lfo1=0;
	int selectMacro7Block2Geo1Lfo2=0;
	int selectMacro7Block2ColorizeLfo1=0;
	int selectMacro7Block2ColorizeLfo2=0;
	int selectMacro7Block2ColorizeLfo3=0;
	int selectMacro7MatrixMix=0;
	int selectMacro7FinalMixAndKey=0;
	int selectMacro7MatrixMixLfo1=0;
	int selectMacro7MatrixMixLfo2=0;
	int selectMacro7FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro7Menu();
	void ifSelectMacro7();

	//macro8
	int selectMacro8Ch1Adjust=0;
	int selectMacro8Ch2MixAndKey=0;
	int selectMacro8Ch2Adjust=0;
	int selectMacro8Ch1AdjustLfo=0;
	int selectMacro8Ch2MixAndKeyLfo=0;
	int selectMacro8Ch2AdjustLfo=0;
	int selectMacro8Fb1MixAndKey=0;
	int selectMacro8Fb1Geo1=0;
	int selectMacro8Fb1Color1=0;
	int selectMacro8Fb1Filters=0;
	int selectMacro8Fb1MixAndKeyLfo=0;
	int selectMacro8Fb1Geo1Lfo1=0;
	int selectMacro8Fb1Geo1Lfo2=0;
	int selectMacro8Fb1Color1Lfo1=0;

	int selectMacro8Fb1DelayTime=0;
	//b2
	int selectMacro8Block2InputAdjust=0;
	int selectMacro8Block2InputAdjustLfo=0;
	int selectMacro8Fb2MixAndKey=0;
	int selectMacro8Fb2Geo1=0;
	int selectMacro8Fb2Color1=0;
	int selectMacro8Fb2Filters=0;
	int selectMacro8Fb2MixAndKeyLfo=0;
	int selectMacro8Fb2Geo1Lfo1=0;
	int selectMacro8Fb2Geo1Lfo2=0;
	int selectMacro8Fb2Color1Lfo1=0;

	int selectMacro8Fb2DelayTime=0;
	//b3
	int selectMacro8Block1Geo=0;
	int selectMacro8Block1Colorize=0;
	int selectMacro8Block1Filters=0;
	int selectMacro8Block1Geo1Lfo1=0;
	int selectMacro8Block1Geo1Lfo2=0;
	int selectMacro8Block1ColorizeLfo1=0;
	int selectMacro8Block1ColorizeLfo2=0;
	int selectMacro8Block1ColorizeLfo3=0;
	int selectMacro8Block2Geo=0;
	int selectMacro8Block2Colorize=0;
	int selectMacro8Block2Filters=0;
	int selectMacro8Block2Geo1Lfo1=0;
	int selectMacro8Block2Geo1Lfo2=0;
	int selectMacro8Block2ColorizeLfo1=0;
	int selectMacro8Block2ColorizeLfo2=0;
	int selectMacro8Block2ColorizeLfo3=0;
	int selectMacro8MatrixMix=0;
	int selectMacro8FinalMixAndKey=0;
	int selectMacro8MatrixMixLfo1=0;
	int selectMacro8MatrixMixLfo2=0;
	int selectMacro8FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro8Menu();
	void ifSelectMacro8();

	//macro9
	int selectMacro9Ch1Adjust=0;
	int selectMacro9Ch2MixAndKey=0;
	int selectMacro9Ch2Adjust=0;
	int selectMacro9Ch1AdjustLfo=0;
	int selectMacro9Ch2MixAndKeyLfo=0;
	int selectMacro9Ch2AdjustLfo=0;
	int selectMacro9Fb1MixAndKey=0;
	int selectMacro9Fb1Geo1=0;
	int selectMacro9Fb1Color1=0;
	int selectMacro9Fb1Filters=0;
	int selectMacro9Fb1MixAndKeyLfo=0;
	int selectMacro9Fb1Geo1Lfo1=0;
	int selectMacro9Fb1Geo1Lfo2=0;
	int selectMacro9Fb1Color1Lfo1=0;

	int selectMacro9Fb1DelayTime=0;
	//b2
	int selectMacro9Block2InputAdjust=0;
	int selectMacro9Block2InputAdjustLfo=0;
	int selectMacro9Fb2MixAndKey=0;
	int selectMacro9Fb2Geo1=0;
	int selectMacro9Fb2Color1=0;
	int selectMacro9Fb2Filters=0;
	int selectMacro9Fb2MixAndKeyLfo=0;
	int selectMacro9Fb2Geo1Lfo1=0;
	int selectMacro9Fb2Geo1Lfo2=0;
	int selectMacro9Fb2Color1Lfo1=0;

	int selectMacro9Fb2DelayTime=0;
	//b3
	int selectMacro9Block1Geo=0;
	int selectMacro9Block1Colorize=0;
	int selectMacro9Block1Filters=0;
	int selectMacro9Block1Geo1Lfo1=0;
	int selectMacro9Block1Geo1Lfo2=0;
	int selectMacro9Block1ColorizeLfo1=0;
	int selectMacro9Block1ColorizeLfo2=0;
	int selectMacro9Block1ColorizeLfo3=0;
	int selectMacro9Block2Geo=0;
	int selectMacro9Block2Colorize=0;
	int selectMacro9Block2Filters=0;
	int selectMacro9Block2Geo1Lfo1=0;
	int selectMacro9Block2Geo1Lfo2=0;
	int selectMacro9Block2ColorizeLfo1=0;
	int selectMacro9Block2ColorizeLfo2=0;
	int selectMacro9Block2ColorizeLfo3=0;
	int selectMacro9MatrixMix=0;
	int selectMacro9FinalMixAndKey=0;
	int selectMacro9MatrixMixLfo1=0;
	int selectMacro9MatrixMixLfo2=0;
	int selectMacro9FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro9Menu();
	void ifSelectMacro9();

	//macro10
	int selectMacro10Ch1Adjust=0;
	int selectMacro10Ch2MixAndKey=0;
	int selectMacro10Ch2Adjust=0;
	int selectMacro10Ch1AdjustLfo=0;
	int selectMacro10Ch2MixAndKeyLfo=0;
	int selectMacro10Ch2AdjustLfo=0;
	int selectMacro10Fb1MixAndKey=0;
	int selectMacro10Fb1Geo1=0;
	int selectMacro10Fb1Color1=0;
	int selectMacro10Fb1Filters=0;
	int selectMacro10Fb1MixAndKeyLfo=0;
	int selectMacro10Fb1Geo1Lfo1=0;
	int selectMacro10Fb1Geo1Lfo2=0;
	int selectMacro10Fb1Color1Lfo1=0;

	int selectMacro10Fb1DelayTime=0;
	//b2
	int selectMacro10Block2InputAdjust=0;
	int selectMacro10Block2InputAdjustLfo=0;
	int selectMacro10Fb2MixAndKey=0;
	int selectMacro10Fb2Geo1=0;
	int selectMacro10Fb2Color1=0;
	int selectMacro10Fb2Filters=0;
	int selectMacro10Fb2MixAndKeyLfo=0;
	int selectMacro10Fb2Geo1Lfo1=0;
	int selectMacro10Fb2Geo1Lfo2=0;
	int selectMacro10Fb2Color1Lfo1=0;

	int selectMacro10Fb2DelayTime=0;
	//b3
	int selectMacro10Block1Geo=0;
	int selectMacro10Block1Colorize=0;
	int selectMacro10Block1Filters=0;
	int selectMacro10Block1Geo1Lfo1=0;
	int selectMacro10Block1Geo1Lfo2=0;
	int selectMacro10Block1ColorizeLfo1=0;
	int selectMacro10Block1ColorizeLfo2=0;
	int selectMacro10Block1ColorizeLfo3=0;
	int selectMacro10Block2Geo=0;
	int selectMacro10Block2Colorize=0;
	int selectMacro10Block2Filters=0;
	int selectMacro10Block2Geo1Lfo1=0;
	int selectMacro10Block2Geo1Lfo2=0;
	int selectMacro10Block2ColorizeLfo1=0;
	int selectMacro10Block2ColorizeLfo2=0;
	int selectMacro10Block2ColorizeLfo3=0;
	int selectMacro10MatrixMix=0;
	int selectMacro10FinalMixAndKey=0;
	int selectMacro10MatrixMixLfo1=0;
	int selectMacro10MatrixMixLfo2=0;
	int selectMacro10FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro10Menu();
	void ifSelectMacro10();

	//macro11
	int selectMacro11Ch1Adjust=0;
	int selectMacro11Ch2MixAndKey=0;
	int selectMacro11Ch2Adjust=0;
	int selectMacro11Ch1AdjustLfo=0;
	int selectMacro11Ch2MixAndKeyLfo=0;
	int selectMacro11Ch2AdjustLfo=0;
	int selectMacro11Fb1MixAndKey=0;
	int selectMacro11Fb1Geo1=0;
	int selectMacro11Fb1Color1=0;
	int selectMacro11Fb1Filters=0;
	int selectMacro11Fb1MixAndKeyLfo=0;
	int selectMacro11Fb1Geo1Lfo1=0;
	int selectMacro11Fb1Geo1Lfo2=0;
	int selectMacro11Fb1Color1Lfo1=0;

	int selectMacro11Fb1DelayTime=0;
	//b2
	int selectMacro11Block2InputAdjust=0;
	int selectMacro11Block2InputAdjustLfo=0;
	int selectMacro11Fb2MixAndKey=0;
	int selectMacro11Fb2Geo1=0;
	int selectMacro11Fb2Color1=0;
	int selectMacro11Fb2Filters=0;
	int selectMacro11Fb2MixAndKeyLfo=0;
	int selectMacro11Fb2Geo1Lfo1=0;
	int selectMacro11Fb2Geo1Lfo2=0;
	int selectMacro11Fb2Color1Lfo1=0;

	int selectMacro11Fb2DelayTime=0;
	//b3
	int selectMacro11Block1Geo=0;
	int selectMacro11Block1Colorize=0;
	int selectMacro11Block1Filters=0;
	int selectMacro11Block1Geo1Lfo1=0;
	int selectMacro11Block1Geo1Lfo2=0;
	int selectMacro11Block1ColorizeLfo1=0;
	int selectMacro11Block1ColorizeLfo2=0;
	int selectMacro11Block1ColorizeLfo3=0;
	int selectMacro11Block2Geo=0;
	int selectMacro11Block2Colorize=0;
	int selectMacro11Block2Filters=0;
	int selectMacro11Block2Geo1Lfo1=0;
	int selectMacro11Block2Geo1Lfo2=0;
	int selectMacro11Block2ColorizeLfo1=0;
	int selectMacro11Block2ColorizeLfo2=0;
	int selectMacro11Block2ColorizeLfo3=0;
	int selectMacro11MatrixMix=0;
	int selectMacro11FinalMixAndKey=0;
	int selectMacro11MatrixMixLfo1=0;
	int selectMacro11MatrixMixLfo2=0;
	int selectMacro11FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro11Menu();
	void ifSelectMacro11();

	//macro12
	int selectMacro12Ch1Adjust=0;
	int selectMacro12Ch2MixAndKey=0;
	int selectMacro12Ch2Adjust=0;
	int selectMacro12Ch1AdjustLfo=0;
	int selectMacro12Ch2MixAndKeyLfo=0;
	int selectMacro12Ch2AdjustLfo=0;
	int selectMacro12Fb1MixAndKey=0;
	int selectMacro12Fb1Geo1=0;
	int selectMacro12Fb1Color1=0;
	int selectMacro12Fb1Filters=0;
	int selectMacro12Fb1MixAndKeyLfo=0;
	int selectMacro12Fb1Geo1Lfo1=0;
	int selectMacro12Fb1Geo1Lfo2=0;
	int selectMacro12Fb1Color1Lfo1=0;

	int selectMacro12Fb1DelayTime=0;
	//b2
	int selectMacro12Block2InputAdjust=0;
	int selectMacro12Block2InputAdjustLfo=0;
	int selectMacro12Fb2MixAndKey=0;
	int selectMacro12Fb2Geo1=0;
	int selectMacro12Fb2Color1=0;
	int selectMacro12Fb2Filters=0;
	int selectMacro12Fb2MixAndKeyLfo=0;
	int selectMacro12Fb2Geo1Lfo1=0;
	int selectMacro12Fb2Geo1Lfo2=0;
	int selectMacro12Fb2Color1Lfo1=0;

	int selectMacro12Fb2DelayTime=0;
	//b3
	int selectMacro12Block1Geo=0;
	int selectMacro12Block1Colorize=0;
	int selectMacro12Block1Filters=0;
	int selectMacro12Block1Geo1Lfo1=0;
	int selectMacro12Block1Geo1Lfo2=0;
	int selectMacro12Block1ColorizeLfo1=0;
	int selectMacro12Block1ColorizeLfo2=0;
	int selectMacro12Block1ColorizeLfo3=0;
	int selectMacro12Block2Geo=0;
	int selectMacro12Block2Colorize=0;
	int selectMacro12Block2Filters=0;
	int selectMacro12Block2Geo1Lfo1=0;
	int selectMacro12Block2Geo1Lfo2=0;
	int selectMacro12Block2ColorizeLfo1=0;
	int selectMacro12Block2ColorizeLfo2=0;
	int selectMacro12Block2ColorizeLfo3=0;
	int selectMacro12MatrixMix=0;
	int selectMacro12FinalMixAndKey=0;
	int selectMacro12MatrixMixLfo1=0;
	int selectMacro12MatrixMixLfo2=0;
	int selectMacro12FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro12Menu();
	void ifSelectMacro12();

	//macro13
	int selectMacro13Ch1Adjust=0;
	int selectMacro13Ch2MixAndKey=0;
	int selectMacro13Ch2Adjust=0;
	int selectMacro13Ch1AdjustLfo=0;
	int selectMacro13Ch2MixAndKeyLfo=0;
	int selectMacro13Ch2AdjustLfo=0;
	int selectMacro13Fb1MixAndKey=0;
	int selectMacro13Fb1Geo1=0;
	int selectMacro13Fb1Color1=0;
	int selectMacro13Fb1Filters=0;
	int selectMacro13Fb1MixAndKeyLfo=0;
	int selectMacro13Fb1Geo1Lfo1=0;
	int selectMacro13Fb1Geo1Lfo2=0;
	int selectMacro13Fb1Color1Lfo1=0;

	int selectMacro13Fb1DelayTime=0;
	//b2
	int selectMacro13Block2InputAdjust=0;
	int selectMacro13Block2InputAdjustLfo=0;
	int selectMacro13Fb2MixAndKey=0;
	int selectMacro13Fb2Geo1=0;
	int selectMacro13Fb2Color1=0;
	int selectMacro13Fb2Filters=0;
	int selectMacro13Fb2MixAndKeyLfo=0;
	int selectMacro13Fb2Geo1Lfo1=0;
	int selectMacro13Fb2Geo1Lfo2=0;
	int selectMacro13Fb2Color1Lfo1=0;

	int selectMacro13Fb2DelayTime=0;
	//b3
	int selectMacro13Block1Geo=0;
	int selectMacro13Block1Colorize=0;
	int selectMacro13Block1Filters=0;
	int selectMacro13Block1Geo1Lfo1=0;
	int selectMacro13Block1Geo1Lfo2=0;
	int selectMacro13Block1ColorizeLfo1=0;
	int selectMacro13Block1ColorizeLfo2=0;
	int selectMacro13Block1ColorizeLfo3=0;
	int selectMacro13Block2Geo=0;
	int selectMacro13Block2Colorize=0;
	int selectMacro13Block2Filters=0;
	int selectMacro13Block2Geo1Lfo1=0;
	int selectMacro13Block2Geo1Lfo2=0;
	int selectMacro13Block2ColorizeLfo1=0;
	int selectMacro13Block2ColorizeLfo2=0;
	int selectMacro13Block2ColorizeLfo3=0;
	int selectMacro13MatrixMix=0;
	int selectMacro13FinalMixAndKey=0;
	int selectMacro13MatrixMixLfo1=0;
	int selectMacro13MatrixMixLfo2=0;
	int selectMacro13FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro13Menu();
	void ifSelectMacro13();

	//macro14
	int selectMacro14Ch1Adjust=0;
	int selectMacro14Ch2MixAndKey=0;
	int selectMacro14Ch2Adjust=0;
	int selectMacro14Ch1AdjustLfo=0;
	int selectMacro14Ch2MixAndKeyLfo=0;
	int selectMacro14Ch2AdjustLfo=0;
	int selectMacro14Fb1MixAndKey=0;
	int selectMacro14Fb1Geo1=0;
	int selectMacro14Fb1Color1=0;
	int selectMacro14Fb1Filters=0;
	int selectMacro14Fb1MixAndKeyLfo=0;
	int selectMacro14Fb1Geo1Lfo1=0;
	int selectMacro14Fb1Geo1Lfo2=0;
	int selectMacro14Fb1Color1Lfo1=0;

	int selectMacro14Fb1DelayTime=0;
	//b2
	int selectMacro14Block2InputAdjust=0;
	int selectMacro14Block2InputAdjustLfo=0;
	int selectMacro14Fb2MixAndKey=0;
	int selectMacro14Fb2Geo1=0;
	int selectMacro14Fb2Color1=0;
	int selectMacro14Fb2Filters=0;
	int selectMacro14Fb2MixAndKeyLfo=0;
	int selectMacro14Fb2Geo1Lfo1=0;
	int selectMacro14Fb2Geo1Lfo2=0;
	int selectMacro14Fb2Color1Lfo1=0;

	int selectMacro14Fb2DelayTime=0;
	//b3
	int selectMacro14Block1Geo=0;
	int selectMacro14Block1Colorize=0;
	int selectMacro14Block1Filters=0;
	int selectMacro14Block1Geo1Lfo1=0;
	int selectMacro14Block1Geo1Lfo2=0;
	int selectMacro14Block1ColorizeLfo1=0;
	int selectMacro14Block1ColorizeLfo2=0;
	int selectMacro14Block1ColorizeLfo3=0;
	int selectMacro14Block2Geo=0;
	int selectMacro14Block2Colorize=0;
	int selectMacro14Block2Filters=0;
	int selectMacro14Block2Geo1Lfo1=0;
	int selectMacro14Block2Geo1Lfo2=0;
	int selectMacro14Block2ColorizeLfo1=0;
	int selectMacro14Block2ColorizeLfo2=0;
	int selectMacro14Block2ColorizeLfo3=0;
	int selectMacro14MatrixMix=0;
	int selectMacro14FinalMixAndKey=0;
	int selectMacro14MatrixMixLfo1=0;
	int selectMacro14MatrixMixLfo2=0;
	int selectMacro14FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro14Menu();
	void ifSelectMacro14();

	//macro15
	int selectMacro15Ch1Adjust=0;
	int selectMacro15Ch2MixAndKey=0;
	int selectMacro15Ch2Adjust=0;
	int selectMacro15Ch1AdjustLfo=0;
	int selectMacro15Ch2MixAndKeyLfo=0;
	int selectMacro15Ch2AdjustLfo=0;
	int selectMacro15Fb1MixAndKey=0;
	int selectMacro15Fb1Geo1=0;
	int selectMacro15Fb1Color1=0;
	int selectMacro15Fb1Filters=0;
	int selectMacro15Fb1MixAndKeyLfo=0;
	int selectMacro15Fb1Geo1Lfo1=0;
	int selectMacro15Fb1Geo1Lfo2=0;
	int selectMacro15Fb1Color1Lfo1=0;

	int selectMacro15Fb1DelayTime=0;
	//b2
	int selectMacro15Block2InputAdjust=0;
	int selectMacro15Block2InputAdjustLfo=0;
	int selectMacro15Fb2MixAndKey=0;
	int selectMacro15Fb2Geo1=0;
	int selectMacro15Fb2Color1=0;
	int selectMacro15Fb2Filters=0;
	int selectMacro15Fb2MixAndKeyLfo=0;
	int selectMacro15Fb2Geo1Lfo1=0;
	int selectMacro15Fb2Geo1Lfo2=0;
	int selectMacro15Fb2Color1Lfo1=0;

	int selectMacro15Fb2DelayTime=0;
	//b3
	int selectMacro15Block1Geo=0;
	int selectMacro15Block1Colorize=0;
	int selectMacro15Block1Filters=0;
	int selectMacro15Block1Geo1Lfo1=0;
	int selectMacro15Block1Geo1Lfo2=0;
	int selectMacro15Block1ColorizeLfo1=0;
	int selectMacro15Block1ColorizeLfo2=0;
	int selectMacro15Block1ColorizeLfo3=0;
	int selectMacro15Block2Geo=0;
	int selectMacro15Block2Colorize=0;
	int selectMacro15Block2Filters=0;
	int selectMacro15Block2Geo1Lfo1=0;
	int selectMacro15Block2Geo1Lfo2=0;
	int selectMacro15Block2ColorizeLfo1=0;
	int selectMacro15Block2ColorizeLfo2=0;
	int selectMacro15Block2ColorizeLfo3=0;
	int selectMacro15MatrixMix=0;
	int selectMacro15FinalMixAndKey=0;
	int selectMacro15MatrixMixLfo1=0;
	int selectMacro15MatrixMixLfo2=0;
	int selectMacro15FinalMixAndKeyLfo=0;

	//menu procedures
	void selectMacro15Menu();
	void ifSelectMacro15();


	/**gui**/
	ofxImGui::Gui gui;

	/*Arrays & midi 2 gui*/
	void allArrayClear();
	void midi2Gui(bool midiActive[], float params[], bool midiSwitch);

	//reset
	void resetAll();
	void allButtonsClear();

	// OSC-triggered reset switches
	bool resetAllSwitch = 0;
	bool block1ResetAllSwitch = 0;
	bool block1InputResetAllSwitch = 0;
	bool fb1ResetAllSwitch = 0;
	bool block2ResetAllSwitch = 0;
	bool block2InputResetAllSwitch = 0;
	bool fb2ResetAllSwitch = 0;
	bool block3ResetAllSwitch = 0;

	float sdFixX=0;
	float hdFixY=0;
	//check and delete all these later
	float testParameter=0;
	float cribX=0;
	float cribY=0;

	//OK LETS FIX ASPECT & POSITIONS FOR REAL THIS TIMEEE
	float input1XFix=0;
	float input1YFix=0;
	float input1ScaleFix=0;

	float input2XFix=0;
	float input2YFix=0;
	float input2ScaleFix=0;


	//global coefficients






	//i think we trash these?
	float chXDisplace=640.0;
	float chYDisplace=480.0;
	float chZDisplace=1.0;
	//delete?
	float ch1XDisplace=0;
	float ch1YDisplace=0;
	float ch1ZDisplace=0;



	void randomizeControls();

	bool secretMenuSwitch=1;
	void block1ResetAll();





	//ch1 and ch2 input resets
	void block1InputResetAll();

	//ch1 parameters
	int ch1InputSelect=0; //0 is input1, 1 is input2
	bool ch1AspectRatioSwitch=0; //0 is 4:3, 1 is 16:9

	//ch1 midi syncing & parameter bizness
	bool ch1AdjustMidiGui=0;
	bool ch1AdjustReset=0;
	float ch1Adjust[PARAMETER_ARRAY_LENGTH];
	bool ch1AdjustMidiActive[PARAMETER_ARRAY_LENGTH];

	bool ch1HMirror=0;
	bool ch1VMirror=0;
	bool ch1HueInvert=0;
	bool ch1SaturationInvert=0;
	bool ch1BrightInvert=0;
	int ch1GeoOverflow=0;
	bool ch1HFlip=0;
	bool ch1VFlip=0;
	bool ch1RGBInvert=0;
	bool ch1Solarize=0;


	//ch1 adjust lfo midi syncing & parameter bizness
	bool ch1AdjustLfoMidiGui=0;
	bool ch1AdjustLfoReset=0;
	float ch1AdjustLfo[PARAMETER_ARRAY_LENGTH];
	bool ch1AdjustLfoMidiActive[PARAMETER_ARRAY_LENGTH];

	//ch2 parameters
	int ch2InputSelect=1; //0 is input1, 1 is input2
	bool ch2AspectRatioSwitch=0; //0 is 4:3, 1 is 16:9

	//ch2 pmidi syncing & paramater bizness
	bool ch2MixAndKeyMidiGui=0;
	bool ch2MixAndKeyReset=0;
	float ch2MixAndKey[PARAMETER_ARRAY_LENGTH];
	bool ch2MixAndKeyMidiActive[PARAMETER_ARRAY_LENGTH];

	int ch2KeyOrder=0;
	int ch2MixType=0;
	int ch2KeyMode=0;
	int ch2MixOverflow=0;

	//ch2 pmidi syncing & paramater bizness
	bool ch2MixAndKeyLfoMidiGui=0;
	bool ch2MixAndKeyLfoReset=0;
	float ch2MixAndKeyLfo[PARAMETER_ARRAY_LENGTH];
	bool ch2MixAndKeyLfoMidiActive[PARAMETER_ARRAY_LENGTH];

	//ch2 midi syncing & parameter bizness
	bool ch2AdjustMidiGui=0;
	bool ch2AdjustReset=0;
	float ch2Adjust[PARAMETER_ARRAY_LENGTH];
	bool ch2AdjustMidiActive[PARAMETER_ARRAY_LENGTH];

	bool ch2HMirror=0;
	bool ch2VMirror=0;
	bool ch2HueInvert=0;
	bool ch2SaturationInvert=0;
	bool ch2BrightInvert=0;
	int ch2GeoOverflow=0;
	bool ch2HFlip=0;
	bool ch2VFlip=0;
	bool ch2RGBInvert=0;
	bool ch2Solarize=0;

	//ch2 adjust lfo midi syncing & parameter bizness
	bool ch2AdjustLfoMidiGui=0;
	bool ch2AdjustLfoReset=0;
	float ch2AdjustLfo[PARAMETER_ARRAY_LENGTH];
	bool ch2AdjustLfoMidiActive[PARAMETER_ARRAY_LENGTH];

	//fb1 reset
	void fb1ResetAll();
	bool fb1FramebufferClearSwitch=0;


	//fb1 mixnkey pmidi syncing & paramater bizness
	bool fb1MixAndKeyMidiGui=0;
	bool fb1MixAndKeyReset=0;
	float fb1MixAndKey[PARAMETER_ARRAY_LENGTH];
	bool fb1MixAndKeyMidiActive[PARAMETER_ARRAY_LENGTH];

	int fb1KeyOrder=0;
	int fb1MixType=0;
	int fb1KeyMode=0;
	int fb1MixOverflow=0;

	int fb1DelayTime=1;


	//fb1 geo1 pmidi syncing & paramater bizness
	bool fb1Geo1MidiGui=0;
	bool fb1Geo1Reset=0;
	float fb1Geo1[PARAMETER_ARRAY_LENGTH];
	bool fb1Geo1MidiActive[PARAMETER_ARRAY_LENGTH];

	bool fb1HMirror=0;
	bool fb1VMirror=0;
	bool fb1HFlip=0;
	bool fb1VFlip=0;
	bool fb1RotateMode=0;
	int fb1GeoOverflow=0;


	//fb1 color1 pmidi syncing & paramater bizness
	bool fb1Color1MidiGui=0;
	bool fb1Color1Reset=0;
	float fb1Color1[PARAMETER_ARRAY_LENGTH];
	bool fb1Color1MidiActive[PARAMETER_ARRAY_LENGTH];

	bool fb1HueInvert=0;
	bool fb1SaturationInvert=0;
	bool fb1BrightInvert=0;

	//fb1 filters pmidi syncing & paramater bizness
	bool fb1FiltersMidiGui=0;
	bool fb1FiltersReset=0;
	float fb1Filters[PARAMETER_ARRAY_LENGTH];
	bool fb1FiltersMidiActive[PARAMETER_ARRAY_LENGTH];


	//fb1 LFO
	//fb1 mixnkey pmidi syncing & paramater bizness
	bool fb1MixAndKeyLfoMidiGui=0;
	bool fb1MixAndKeyLfoReset=0;
	float fb1MixAndKeyLfo[PARAMETER_ARRAY_LENGTH];
	bool fb1MixAndKeyLfoMidiActive[PARAMETER_ARRAY_LENGTH];

	//fb1 Geo1Lfo1 pmidi syncing & paramater bizness
	bool fb1Geo1Lfo1MidiGui=0;
	bool fb1Geo1Lfo1Reset=0;
	float fb1Geo1Lfo1[PARAMETER_ARRAY_LENGTH];
	bool fb1Geo1Lfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	//fb1 Geo1Lfo2 pmidi syncing & paramater bizness
	bool fb1Geo1Lfo2MidiGui=0;
	bool fb1Geo1Lfo2Reset=0;
	float fb1Geo1Lfo2[PARAMETER_ARRAY_LENGTH];
	bool fb1Geo1Lfo2MidiActive[PARAMETER_ARRAY_LENGTH];

	//fb1 Color1Lfo1 pmidi syncing & paramater bizness
	bool fb1Color1Lfo1MidiGui=0;
	bool fb1Color1Lfo1Reset=0;
	float fb1Color1Lfo1[PARAMETER_ARRAY_LENGTH];
	bool fb1Color1Lfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	//geometrical animations
	//hypercube
	bool block1HypercubeSwitch=0;
	float hypercube_size = 1.0;
    float hypercube_theta_rate=0.01;
    float hypercube_phi_rate=0.01;

	//line
	bool block1LineSwitch=0;

	//seven star
	bool block1SevenStarSwitch=0;

	//lissaball
	bool block1LissaBallSwitch=0;
	int drawMode=3;

	// ============== LISSAJOUS CURVE - BLOCK 1 ==============
	bool block1LissajousCurveSwitch = 0;

	// Base parameters (16 floats)
	float lissajous1XFreq = 0.1f;
	float lissajous1YFreq = 0.2f;
	float lissajous1ZFreq = 0.3f;
	float lissajous1XAmp = 1.0f;
	float lissajous1YAmp = 1.0f;
	float lissajous1ZAmp = 0.5f;
	float lissajous1XPhase = 0.0f;
	float lissajous1YPhase = 0.0f;
	float lissajous1ZPhase = 0.0f;
	float lissajous1XOffset = 0.5f;
	float lissajous1YOffset = 0.5f;
	float lissajous1Speed = 0.5f;
	float lissajous1Size = 0.5f;
	float lissajous1NumPoints = 0.5f;
	float lissajous1LineWidth = 0.2f;
	float lissajous1ColorSpeed = 0.5f;

	// Curve waveshapes (3 ints)
	int lissajous1XShape = 0;
	int lissajous1YShape = 0;
	int lissajous1ZShape = 0;

	// LFO Amplitudes (16 floats)
	float lissajous1XFreqLfoAmp = 0.0f;
	float lissajous1YFreqLfoAmp = 0.0f;
	float lissajous1ZFreqLfoAmp = 0.0f;
	float lissajous1XAmpLfoAmp = 0.0f;
	float lissajous1YAmpLfoAmp = 0.0f;
	float lissajous1ZAmpLfoAmp = 0.0f;
	float lissajous1XPhaseLfoAmp = 0.0f;
	float lissajous1YPhaseLfoAmp = 0.0f;
	float lissajous1ZPhaseLfoAmp = 0.0f;
	float lissajous1XOffsetLfoAmp = 0.0f;
	float lissajous1YOffsetLfoAmp = 0.0f;
	float lissajous1SpeedLfoAmp = 0.0f;
	float lissajous1SizeLfoAmp = 0.0f;
	float lissajous1NumPointsLfoAmp = 0.0f;
	float lissajous1LineWidthLfoAmp = 0.0f;
	float lissajous1ColorSpeedLfoAmp = 0.0f;

	// LFO Rates (16 floats)
	float lissajous1XFreqLfoRate = 0.0f;
	float lissajous1YFreqLfoRate = 0.0f;
	float lissajous1ZFreqLfoRate = 0.0f;
	float lissajous1XAmpLfoRate = 0.0f;
	float lissajous1YAmpLfoRate = 0.0f;
	float lissajous1ZAmpLfoRate = 0.0f;
	float lissajous1XPhaseLfoRate = 0.0f;
	float lissajous1YPhaseLfoRate = 0.0f;
	float lissajous1ZPhaseLfoRate = 0.0f;
	float lissajous1XOffsetLfoRate = 0.0f;
	float lissajous1YOffsetLfoRate = 0.0f;
	float lissajous1SpeedLfoRate = 0.0f;
	float lissajous1SizeLfoRate = 0.0f;
	float lissajous1NumPointsLfoRate = 0.0f;
	float lissajous1LineWidthLfoRate = 0.0f;
	float lissajous1ColorSpeedLfoRate = 0.0f;

	// LFO Shapes (16 ints)
	int lissajous1XFreqLfoShape = 0;
	int lissajous1YFreqLfoShape = 0;
	int lissajous1ZFreqLfoShape = 0;
	int lissajous1XAmpLfoShape = 0;
	int lissajous1YAmpLfoShape = 0;
	int lissajous1ZAmpLfoShape = 0;
	int lissajous1XPhaseLfoShape = 0;
	int lissajous1YPhaseLfoShape = 0;
	int lissajous1ZPhaseLfoShape = 0;
	int lissajous1XOffsetLfoShape = 0;
	int lissajous1YOffsetLfoShape = 0;
	int lissajous1SpeedLfoShape = 0;
	int lissajous1SizeLfoShape = 0;
	int lissajous1NumPointsLfoShape = 0;
	int lissajous1LineWidthLfoShape = 0;
	int lissajous1ColorSpeedLfoShape = 0;

	// Color parameters
	float lissajous1Hue = 0.5f;
	float lissajous1HueSpread = 1.0f;
	float lissajous1HueLfoAmp = 0.0f;
	float lissajous1HueLfoRate = 0.0f;
	int lissajous1HueLfoShape = 0;
	float lissajous1HueSpreadLfoAmp = 0.0f;
	float lissajous1HueSpreadLfoRate = 0.0f;
	int lissajous1HueSpreadLfoShape = 0;


	//BLOCK2 input
	void block2ResetAll();

	void block2InputResetAll();

	int block2InputSelect=2;
	bool block2InputAspectRatioSwitch=0;


	//block2Input midi syncing & parameter bizness
	bool block2InputAdjustMidiGui=0;
	bool block2InputAdjustReset=0;
	float block2InputAdjust[PARAMETER_ARRAY_LENGTH];
	bool block2InputAdjustMidiActive[PARAMETER_ARRAY_LENGTH];

	bool block2InputHMirror=0;
	bool block2InputVMirror=0;
	bool block2InputHueInvert=0;
	bool block2InputSaturationInvert=0;
	bool block2InputBrightInvert=0;
	int block2InputGeoOverflow=0;
	bool block2InputHFlip=0;
	bool block2InputVFlip=0;
	bool block2InputRGBInvert=0;
	bool block2InputSolarize=0;


	//block2Input adjust lfo midi syncing & parameter bizness
	bool block2InputAdjustLfoMidiGui=0;
	bool block2InputAdjustLfoReset=0;
	float block2InputAdjustLfo[PARAMETER_ARRAY_LENGTH];
	bool block2InputAdjustLfoMidiActive[PARAMETER_ARRAY_LENGTH];



	void fb2ResetAll();
	bool fb2FramebufferClearSwitch=0;

	//fb2 mixnkey pmidi syncing & paramater bizness
	bool fb2MixAndKeyMidiGui=0;
	bool fb2MixAndKeyReset=0;
	float fb2MixAndKey[PARAMETER_ARRAY_LENGTH];
	bool fb2MixAndKeyMidiActive[PARAMETER_ARRAY_LENGTH];

	int fb2KeyOrder=0;
	int fb2MixType=0;
	int fb2KeyMode=0;
	int fb2MixOverflow=0;

	int fb2DelayTime=1;

	//fb2 geo1 pmidi syncing & paramater bizness
	bool fb2Geo1MidiGui=0;
	bool fb2Geo1Reset=0;
	float fb2Geo1[PARAMETER_ARRAY_LENGTH];
	bool fb2Geo1MidiActive[PARAMETER_ARRAY_LENGTH];

	bool fb2HMirror=0;
	bool fb2VMirror=0;
	bool fb2HFlip=0;
	bool fb2VFlip=0;
	bool fb2RotateMode=0;
	int fb2GeoOverflow=0;

	//fb2 color1 pmidi syncing & paramater bizness
	bool fb2Color1MidiGui=0;
	bool fb2Color1Reset=0;
	float fb2Color1[PARAMETER_ARRAY_LENGTH];
	bool fb2Color1MidiActive[PARAMETER_ARRAY_LENGTH];

	bool fb2HueInvert=0;
	bool fb2SaturationInvert=0;
	bool fb2BrightInvert=0;

	//fb2 filters pmidi syncing & paramater bizness
	bool fb2FiltersMidiGui=0;
	bool fb2FiltersReset=0;
	float fb2Filters[PARAMETER_ARRAY_LENGTH];
	bool fb2FiltersMidiActive[PARAMETER_ARRAY_LENGTH];


	//fb2 LFO
	//fb2 mixnkey pmidi syncing & paramater bizness
	bool fb2MixAndKeyLfoMidiGui=0;
	bool fb2MixAndKeyLfoReset=0;
	float fb2MixAndKeyLfo[PARAMETER_ARRAY_LENGTH];
	bool fb2MixAndKeyLfoMidiActive[PARAMETER_ARRAY_LENGTH];

	//fb2 Geo1Lfo1 pmidi syncing & paramater bizness
	bool fb2Geo1Lfo1MidiGui=0;
	bool fb2Geo1Lfo1Reset=0;
	float fb2Geo1Lfo1[PARAMETER_ARRAY_LENGTH];
	bool fb2Geo1Lfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	//fb2 Geo1Lfo2 pmidi syncing & paramater bizness
	bool fb2Geo1Lfo2MidiGui=0;
	bool fb2Geo1Lfo2Reset=0;
	float fb2Geo1Lfo2[PARAMETER_ARRAY_LENGTH];
	bool fb2Geo1Lfo2MidiActive[PARAMETER_ARRAY_LENGTH];

	//fb2 Color1Lfo1 pmidi syncing & paramater bizness
	bool fb2Color1Lfo1MidiGui=0;
	bool fb2Color1Lfo1Reset=0;
	float fb2Color1Lfo1[PARAMETER_ARRAY_LENGTH];
	bool fb2Color1Lfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	//block2 geometrical animations
	//hypercube
	bool block2HypercubeSwitch=0;

	//line
	bool block2LineSwitch=0;

	//seven star
	bool block2SevenStarSwitch=0;

	//lissaball
	bool block2LissaBallSwitch=0;

	// ============== LISSAJOUS CURVE - BLOCK 2 ==============
	bool block2LissajousCurveSwitch = 0;

	// Base parameters (16 floats)
	float lissajous2XFreq = 0.1f;
	float lissajous2YFreq = 0.2f;
	float lissajous2ZFreq = 0.3f;
	float lissajous2XAmp = 1.0f;
	float lissajous2YAmp = 1.0f;
	float lissajous2ZAmp = 0.5f;
	float lissajous2XPhase = 0.0f;
	float lissajous2YPhase = 0.0f;
	float lissajous2ZPhase = 0.0f;
	float lissajous2XOffset = 0.5f;
	float lissajous2YOffset = 0.5f;
	float lissajous2Speed = 0.5f;
	float lissajous2Size = 0.5f;
	float lissajous2NumPoints = 0.5f;
	float lissajous2LineWidth = 0.2f;
	float lissajous2ColorSpeed = 0.5f;

	// Curve waveshapes (3 ints)
	int lissajous2XShape = 0;
	int lissajous2YShape = 0;
	int lissajous2ZShape = 0;

	// LFO Amplitudes (16 floats)
	float lissajous2XFreqLfoAmp = 0.0f;
	float lissajous2YFreqLfoAmp = 0.0f;
	float lissajous2ZFreqLfoAmp = 0.0f;
	float lissajous2XAmpLfoAmp = 0.0f;
	float lissajous2YAmpLfoAmp = 0.0f;
	float lissajous2ZAmpLfoAmp = 0.0f;
	float lissajous2XPhaseLfoAmp = 0.0f;
	float lissajous2YPhaseLfoAmp = 0.0f;
	float lissajous2ZPhaseLfoAmp = 0.0f;
	float lissajous2XOffsetLfoAmp = 0.0f;
	float lissajous2YOffsetLfoAmp = 0.0f;
	float lissajous2SpeedLfoAmp = 0.0f;
	float lissajous2SizeLfoAmp = 0.0f;
	float lissajous2NumPointsLfoAmp = 0.0f;
	float lissajous2LineWidthLfoAmp = 0.0f;
	float lissajous2ColorSpeedLfoAmp = 0.0f;

	// LFO Rates (16 floats)
	float lissajous2XFreqLfoRate = 0.0f;
	float lissajous2YFreqLfoRate = 0.0f;
	float lissajous2ZFreqLfoRate = 0.0f;
	float lissajous2XAmpLfoRate = 0.0f;
	float lissajous2YAmpLfoRate = 0.0f;
	float lissajous2ZAmpLfoRate = 0.0f;
	float lissajous2XPhaseLfoRate = 0.0f;
	float lissajous2YPhaseLfoRate = 0.0f;
	float lissajous2ZPhaseLfoRate = 0.0f;
	float lissajous2XOffsetLfoRate = 0.0f;
	float lissajous2YOffsetLfoRate = 0.0f;
	float lissajous2SpeedLfoRate = 0.0f;
	float lissajous2SizeLfoRate = 0.0f;
	float lissajous2NumPointsLfoRate = 0.0f;
	float lissajous2LineWidthLfoRate = 0.0f;
	float lissajous2ColorSpeedLfoRate = 0.0f;

	// LFO Shapes (16 ints)
	int lissajous2XFreqLfoShape = 0;
	int lissajous2YFreqLfoShape = 0;
	int lissajous2ZFreqLfoShape = 0;
	int lissajous2XAmpLfoShape = 0;
	int lissajous2YAmpLfoShape = 0;
	int lissajous2ZAmpLfoShape = 0;
	int lissajous2XPhaseLfoShape = 0;
	int lissajous2YPhaseLfoShape = 0;
	int lissajous2ZPhaseLfoShape = 0;
	int lissajous2XOffsetLfoShape = 0;
	int lissajous2YOffsetLfoShape = 0;
	int lissajous2SpeedLfoShape = 0;
	int lissajous2SizeLfoShape = 0;
	int lissajous2NumPointsLfoShape = 0;
	int lissajous2LineWidthLfoShape = 0;
	int lissajous2ColorSpeedLfoShape = 0;

	// Color parameters
	float lissajous2Hue = 0.5f;
	float lissajous2HueSpread = 1.0f;
	float lissajous2HueLfoAmp = 0.0f;
	float lissajous2HueLfoRate = 0.0f;
	int lissajous2HueLfoShape = 0;
	float lissajous2HueSpreadLfoAmp = 0.0f;
	float lissajous2HueSpreadLfoRate = 0.0f;
	int lissajous2HueSpreadLfoShape = 0;

	//BLOCK3


	void block3ResetAll();




	//block1 Geo pmidi syncing & paramater bizness
	bool block1GeoMidiGui=0;
	bool block1GeoReset=0;
	float block1Geo[PARAMETER_ARRAY_LENGTH];
	bool block1GeoMidiActive[PARAMETER_ARRAY_LENGTH];

	int block1GeoOverflow=0;
	bool block1HMirror=0;
	bool block1VMirror=0;
	bool block1HFlip=0;
	bool block1VFlip=0;
	bool block1RotateMode=0;

	//block1 Colorize
	bool block1ColorizeMidiGui=0;
	bool block1ColorizeReset=0;
	float block1Colorize[PARAMETER_ARRAY_LENGTH];
	bool block1ColorizeMidiActive[PARAMETER_ARRAY_LENGTH];

	bool block1ColorizeSwitch=0;
	bool block1ColorizeHSB_RGB=0;//0 is hsb, 1 is rgb

	//block1 filters pmidi syncing & paramater bizness
	bool block1FiltersMidiGui=0;
	bool block1FiltersReset=0;
	float block1Filters[PARAMETER_ARRAY_LENGTH];
	bool block1FiltersMidiActive[PARAMETER_ARRAY_LENGTH];
	int block1DitherType=1;

	//block1 Geo1Lfo1 pmidi syncing & paramater bizness
	bool block1Geo1Lfo1MidiGui=0;
	bool block1Geo1Lfo1Reset=0;
	float block1Geo1Lfo1[PARAMETER_ARRAY_LENGTH];
	bool block1Geo1Lfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	//block1 Geo1Lfo2 pmidi syncing & paramater bizness
	bool block1Geo1Lfo2MidiGui=0;
	bool block1Geo1Lfo2Reset=0;
	float block1Geo1Lfo2[PARAMETER_ARRAY_LENGTH];
	bool block1Geo1Lfo2MidiActive[PARAMETER_ARRAY_LENGTH];

	//block1 ColorizeLfo1
	bool block1ColorizeLfo1MidiGui=0;
	bool block1ColorizeLfo1Reset=0;
	float block1ColorizeLfo1[PARAMETER_ARRAY_LENGTH];
	bool block1ColorizeLfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	//block1 ColorizeLfo2
	bool block1ColorizeLfo2MidiGui=0;
	bool block1ColorizeLfo2Reset=0;
	float block1ColorizeLfo2[PARAMETER_ARRAY_LENGTH];
	bool block1ColorizeLfo2MidiActive[PARAMETER_ARRAY_LENGTH];

	//block1 ColorizeLfo3
	bool block1ColorizeLfo3MidiGui=0;
	bool block1ColorizeLfo3Reset=0;
	float block1ColorizeLfo3[PARAMETER_ARRAY_LENGTH];
	bool block1ColorizeLfo3MidiActive[PARAMETER_ARRAY_LENGTH];







	//i think this is trash
	//block1 Color1Lfo1 pmidi syncing & paramater bizness
	bool block1Color1Lfo1MidiGui=0;
	bool block1Color1Lfo1Reset=0;
	float block1Color1Lfo1[PARAMETER_ARRAY_LENGTH];
	bool block1Color1Lfo1MidiActive[PARAMETER_ARRAY_LENGTH];






	//block2 Geo pmidi syncing & paramater bizness
	bool block2GeoMidiGui=0;
	bool block2GeoReset=0;
	float block2Geo[PARAMETER_ARRAY_LENGTH];
	bool block2GeoMidiActive[PARAMETER_ARRAY_LENGTH];

	int block2GeoOverflow=0;
	bool block2HMirror=0;
	bool block2VMirror=0;
	bool block2HFlip=0;
	bool block2VFlip=0;
	bool block2RotateMode=0;

	//block2 Colorize
	bool block2ColorizeMidiGui=0;
	bool block2ColorizeReset=0;
	float block2Colorize[PARAMETER_ARRAY_LENGTH];
	bool block2ColorizeMidiActive[PARAMETER_ARRAY_LENGTH];

	bool block2ColorizeSwitch=0;
	bool block2ColorizeHSB_RGB=0;

	//block2 filters pmidi syncing & paramater bizness
	bool block2FiltersMidiGui=0;
	bool block2FiltersReset=0;
	float block2Filters[PARAMETER_ARRAY_LENGTH];
	bool block2FiltersMidiActive[PARAMETER_ARRAY_LENGTH];
	int block2DitherType=1;

	//block2 Geo1Lfo1 pmidi syncing & paramater bizness
	bool block2Geo1Lfo1MidiGui=0;
	bool block2Geo1Lfo1Reset=0;
	float block2Geo1Lfo1[PARAMETER_ARRAY_LENGTH];
	bool block2Geo1Lfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	//block2 Geo1Lfo2 pmidi syncing & paramater bizness
	bool block2Geo1Lfo2MidiGui=0;
	bool block2Geo1Lfo2Reset=0;
	float block2Geo1Lfo2[PARAMETER_ARRAY_LENGTH];
	bool block2Geo1Lfo2MidiActive[PARAMETER_ARRAY_LENGTH];

	//block2 ColorizeLfo1
	bool block2ColorizeLfo1MidiGui=0;
	bool block2ColorizeLfo1Reset=0;
	float block2ColorizeLfo1[PARAMETER_ARRAY_LENGTH];
	bool block2ColorizeLfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	//block2 ColorizeLfo2
	bool block2ColorizeLfo2MidiGui=0;
	bool block2ColorizeLfo2Reset=0;
	float block2ColorizeLfo2[PARAMETER_ARRAY_LENGTH];
	bool block2ColorizeLfo2MidiActive[PARAMETER_ARRAY_LENGTH];

	//block2 ColorizeLfo3
	bool block2ColorizeLfo3MidiGui=0;
	bool block2ColorizeLfo3Reset=0;
	float block2ColorizeLfo3[PARAMETER_ARRAY_LENGTH];
	bool block2ColorizeLfo3MidiActive[PARAMETER_ARRAY_LENGTH];



	//matrix mixer
	bool matrixMixMidiGui=0;
	bool matrixMixReset=0;
	float matrixMix[PARAMETER_ARRAY_LENGTH];
	bool matrixMixMidiActive[PARAMETER_ARRAY_LENGTH];

	int matrixMixType=0;
	int matrixMixOverflow=0;

	//final mix pmidi syncing & paramater bizness
	bool finalMixAndKeyMidiGui=0;
	bool finalMixAndKeyReset=0;
	float finalMixAndKey[PARAMETER_ARRAY_LENGTH];
	bool finalMixAndKeyMidiActive[PARAMETER_ARRAY_LENGTH];

	int finalKeyOrder=0;
	int finalMixType=0;
	int finalKeyMode=0;
	int finalMixOverflow=0;

	//matrix mixer lfo
	bool matrixMixLfo1MidiGui=0;
	bool matrixMixLfo1Reset=0;
	float matrixMixLfo1[PARAMETER_ARRAY_LENGTH];
	bool matrixMixLfo1MidiActive[PARAMETER_ARRAY_LENGTH];

	bool matrixMixLfo2MidiGui=0;
	bool matrixMixLfo2Reset=0;
	float matrixMixLfo2[PARAMETER_ARRAY_LENGTH];
	bool matrixMixLfo2MidiActive[PARAMETER_ARRAY_LENGTH];

	//final mixnkey pmidi syncing & paramater bizness
	bool finalMixAndKeyLfoMidiGui=0;
	bool finalMixAndKeyLfoReset=0;
	float finalMixAndKeyLfo[PARAMETER_ARRAY_LENGTH];
	bool finalMixAndKeyLfoMidiActive[PARAMETER_ARRAY_LENGTH];

	// ============== LFO SHAPE ARRAYS ==============
	// LFO Shape arrays - one int per parameter pair
	// Shape values: 0=Sine, 1=Triangle, 2=Ramp, 3=Saw, 4=Square
	int ch1AdjustLfoShape[PARAMETER_ARRAY_LENGTH];         // 8 shapes used (indices 0-7)
	int ch2MixAndKeyLfoShape[PARAMETER_ARRAY_LENGTH];      // 3 shapes used (indices 0-2)
	int ch2AdjustLfoShape[PARAMETER_ARRAY_LENGTH];         // 8 shapes used
	int fb1MixAndKeyLfoShape[PARAMETER_ARRAY_LENGTH];      // 3 shapes used
	int fb1Geo1Lfo1Shape[PARAMETER_ARRAY_LENGTH];          // 4 shapes used
	int fb1Geo1Lfo2Shape[PARAMETER_ARRAY_LENGTH];          // 5 shapes used
	int fb1Color1Lfo1Shape[PARAMETER_ARRAY_LENGTH];        // 3 shapes used
	int block2InputAdjustLfoShape[PARAMETER_ARRAY_LENGTH]; // 8 shapes used
	int fb2MixAndKeyLfoShape[PARAMETER_ARRAY_LENGTH];      // 3 shapes used
	int fb2Geo1Lfo1Shape[PARAMETER_ARRAY_LENGTH];          // 4 shapes used
	int fb2Geo1Lfo2Shape[PARAMETER_ARRAY_LENGTH];          // 5 shapes used
	int fb2Color1Lfo1Shape[PARAMETER_ARRAY_LENGTH];        // 3 shapes used
	int block1Geo1Lfo1Shape[PARAMETER_ARRAY_LENGTH];       // 4 shapes used
	int block1Geo1Lfo2Shape[PARAMETER_ARRAY_LENGTH];       // 5 shapes used
	int block1ColorizeLfo1Shape[PARAMETER_ARRAY_LENGTH];   // 6 shapes used
	int block1ColorizeLfo2Shape[PARAMETER_ARRAY_LENGTH];   // 6 shapes used
	int block1ColorizeLfo3Shape[PARAMETER_ARRAY_LENGTH];   // 3 shapes used
	int block1Color1Lfo1Shape[PARAMETER_ARRAY_LENGTH];     // (legacy - may not be used)
	int block2Geo1Lfo1Shape[PARAMETER_ARRAY_LENGTH];       // 4 shapes used
	int block2Geo1Lfo2Shape[PARAMETER_ARRAY_LENGTH];       // 5 shapes used
	int block2ColorizeLfo1Shape[PARAMETER_ARRAY_LENGTH];   // 6 shapes used
	int block2ColorizeLfo2Shape[PARAMETER_ARRAY_LENGTH];   // 6 shapes used
	int block2ColorizeLfo3Shape[PARAMETER_ARRAY_LENGTH];   // 3 shapes used
	int matrixMixLfo1Shape[PARAMETER_ARRAY_LENGTH];        // 6 shapes used
	int matrixMixLfo2Shape[PARAMETER_ARRAY_LENGTH];        // 3 shapes used
	int finalMixAndKeyLfoShape[PARAMETER_ARRAY_LENGTH];    // 3 shapes used

	// ============== OSC PARAMETER REGISTRY ==============
	// Registry of all OSC-controllable parameters
	std::vector<OscParameter> oscRegistry;

	// Fast lookup map: OSC address -> pointer to registry entry
	std::map<std::string, OscParameter*> oscAddressMap;

	// Flag to pause receiving during sendAll
	std::atomic<bool> oscReceivePaused{false};

	// Initialize the OSC parameter registry (call in setup)
	void registerBlock1OscParameters();
	void registerBlock2OscParameters();
	void registerBlock3OscParameters();

	// Helper to add a parameter to the registry
	void registerOscParam(const std::string& address, float* ptr);
	void registerOscParam(const std::string& address, bool* ptr);
	void registerOscParam(const std::string& address, int* ptr);

	// Video/OSC Settings save/load
	void saveVideoOscSettings();
	void loadVideoOscSettings();

	// Saved source names (for matching on load)
	std::string savedInput1NdiName;
	std::string savedInput2NdiName;
#if OFAPP_HAS_SPOUT
	std::string savedInput1SpoutName;
	std::string savedInput2SpoutName;
#endif

	// Attributions popup
	bool showAttributionsPopup = false;

};

