#include "ofApp.h"
#if OFAPP_HAS_SPOUT
#include "SpoutReceiver.h"  // For Spout sender enumeration
#endif
#include <GLFW/glfw3.h>  // For window decoration toggling (F10)
//globals


float ch1HdAspectXFix=1.0;  // Inputs are now pre-scaled to internal resolution
float ch1HdAspectYFix=1.0;

float ch2HdAspectXFix=1.0;  // Inputs are now pre-scaled to internal resolution
float ch2HdAspectYFix=1.0;


const int pastFramesSize=120;
ofFbo pastFrames1[pastFramesSize];
ofFbo pastFrames2[pastFramesSize];
int pastFramesOffset=0;
unsigned int pastFramesCount=0;


ofTexture dummyTex;
//testing variables
int testSwitch1=1;

// Helper function to allocate GPU-only FBOs (no CPU backing = less RAM usage)
void allocateGpuOnlyFbo(ofFbo& fbo, int width, int height) {
	ofFboSettings settings;
	settings.width = width;
	settings.height = height;
	settings.internalformat = GL_RGBA8;
	settings.useDepth = false;
	settings.useStencil = false;
	fbo.allocate(settings);
	fbo.begin();
	ofClear(0, 0, 0, 255);
	fbo.end();
}

//--------------------------------------------------------------
void ofApp::setup(){
	// Shaders use sampler2D (not sampler2DRect), so we need GL_TEXTURE_2D target
	ofDisableArbTex();
	// Enable normalized texture coordinates (0-1 range) for sampler2D
	ofEnableNormalizedTexCoords();
	ofLogNotice("Shader") << "Normalized Tex Coords: " << (ofGetUsingNormalizedTexCoords() ? "ON" : "OFF");
	ofSetFrameRate(30);
	ofBackground(0);
	ofHideCursor();

	// Initialize resolutions from GUI defaults
	input1Width = 640;
	input1Height = 480;
	input2Width = 640;
	input2Height = 480;
	outputWidth = 1280;
	outputHeight = 720;
	internalWidth = 1280;
	internalHeight = 720;
#if OFAPP_HAS_SPOUT
	spoutSendWidth = 1280;
	spoutSendHeight = 720;
#endif

	inputSetup();

	framebufferSetup();

	//keep this last in setup for easier debugging
	std::string shaderDir = "shadersGL4";
	bool useGLES = false;

#if defined(TARGET_OPENGLES)
	useGLES = true;
#endif

	const char* glVersionCStr = reinterpret_cast<const char*>(glGetString(GL_VERSION));
	std::string glVersionStr = glVersionCStr ? std::string(glVersionCStr) : std::string();
	if (glVersionStr.find("OpenGL ES") != std::string::npos) {
		useGLES = true;
	}

	if (!useGLES && ofGetGLRenderer() && ofGetGLRenderer()->getGLVersionMajor() < 4) {
		useGLES = true;
	}

	if (useGLES) {
		shaderDir = "shadersGLES2";
	}

	if (ofGetGLRenderer()) {
		ofLogNotice("Shader") << "GL Version: "
			<< ofGetGLRenderer()->getGLVersionMajor() << "."
			<< ofGetGLRenderer()->getGLVersionMinor()
			<< " (" << glVersionStr << ")";
	}
	ofLogNotice("Shader") << "Using shader directory: " << shaderDir;
	shader1.load(shaderDir + "/shader1");
	shader2.load(shaderDir + "/shader2");
	shader3.load(shaderDir + "/shader3");

	dummyTex.allocate(internalWidth, internalHeight, GL_RGBA);

	sevenStar1Setup();
	setupOsc();
}

//--------------------------------------------------------------
void ofApp::update(){
	processOscMessages();

	// Check if video inputs need to be reinitialized
	if(gui->reinitializeInputs){
		reinitializeInputs();
		gui->reinitializeInputs = false;
	}

	// Check if NDI sources need to be refreshed
	if(gui->refreshNdiSources){
		refreshNdiSources();
		gui->refreshNdiSources = false;
	}

#if OFAPP_HAS_SPOUT
	// Check if Spout sources need to be refreshed
	if(gui->refreshSpoutSources){
		refreshSpoutSources();
		gui->refreshSpoutSources = false;
	}
#endif

	// Check if resolution needs to be changed
	if(gui->resolutionChangeRequested){
		input1Width = gui->input1Width;
		input1Height = gui->input1Height;
		input2Width = gui->input2Width;
		input2Height = gui->input2Height;
		internalWidth = gui->internalWidth;
		internalHeight = gui->internalHeight;
		outputWidth = gui->outputWidth;
		outputHeight = gui->outputHeight;
#if OFAPP_HAS_SPOUT
		spoutSendWidth = gui->spoutSendWidth;
		spoutSendHeight = gui->spoutSendHeight;
#endif
		reinitializeResolutions();
		gui->resolutionChangeRequested = false;
	}

	// Check if FPS needs to be changed
	if(gui->fpsChangeRequested){
		ofSetFrameRate(gui->targetFPS);
		gui->fpsChangeRequested = false;
	}

	inputUpdate();
	lfoUpdate();

}
//------------------------------------------------------------
float ofApp::lfo(float amp, float rate,int shape){
    float waveValue = 0.0f;

    switch(shape) {
        case 0: // Sine (default)
            waveValue = sin(rate);
            break;

        case 1: // Triangle
            waveValue = (2.0f / PI) * asin(sin(rate));
            break;

        case 2: // Ramp (rising sawtooth)
            waveValue = (2.0f / TWO_PI) * fmod(rate + PI, TWO_PI) - 1.0f;
            break;

        case 3: // Saw (falling sawtooth)
            waveValue = 1.0f - (2.0f / TWO_PI) * fmod(rate + PI, TWO_PI);
            break;

        case 4: // Square (50% duty cycle)
            waveValue = (sin(rate) >= 0.0f) ? 1.0f : -1.0f;
            break;

        default: // Fallback to sine
            waveValue = sin(rate);
            break;
    }

    return amp * waveValue;
}


//reset thetas
//-------------------------------------------------------------
void ofApp::lfoUpdate(){

	//ch1 adjust
	ch1XDisplaceTheta+=lfoRateC*(gui->ch1AdjustLfo[1]);
	ch1YDisplaceTheta+=lfoRateC*(gui->ch1AdjustLfo[3]);
	ch1ZDisplaceTheta+=lfoRateC*(gui->ch1AdjustLfo[5]);
	ch1RotateTheta+=lfoRateC*(gui->ch1AdjustLfo[7]);
	ch1HueAttenuateTheta+=lfoRateC*(gui->ch1AdjustLfo[9]);
	ch1SaturationAttenuateTheta+=lfoRateC*(gui->ch1AdjustLfo[11]);
	ch1BrightAttenuateTheta+=lfoRateC*(gui->ch1AdjustLfo[13]);
	ch1KaleidoscopeSliceTheta+=lfoRateC*(gui->ch1AdjustLfo[15]);

	//ch2 mix and key
	ch2MixAmountTheta+=lfoRateC*(gui->ch2MixAndKeyLfo[1]);
	ch2KeyThresholdTheta+=lfoRateC*(gui->ch2MixAndKeyLfo[3]);
	ch2KeySoftTheta+=lfoRateC*(gui->ch2MixAndKeyLfo[5]);

	//ch2 adjust
	ch2XDisplaceTheta+=lfoRateC*(gui->ch2AdjustLfo[1]);
	ch2YDisplaceTheta+=lfoRateC*(gui->ch2AdjustLfo[3]);
	ch2ZDisplaceTheta+=lfoRateC*(gui->ch2AdjustLfo[5]);
	ch2RotateTheta+=lfoRateC*(gui->ch2AdjustLfo[7]);
	ch2HueAttenuateTheta+=lfoRateC*(gui->ch2AdjustLfo[9]);
	ch2SaturationAttenuateTheta+=lfoRateC*(gui->ch2AdjustLfo[11]);
	ch2BrightAttenuateTheta+=lfoRateC*(gui->ch2AdjustLfo[13]);
	ch2KaleidoscopeSliceTheta+=lfoRateC*(gui->ch2AdjustLfo[15]);

	//fb1 mix and key
	fb1MixAmountTheta+=lfoRateC*(gui->fb1MixAndKeyLfo[1]);
	fb1KeyThresholdTheta+=lfoRateC*(gui->fb1MixAndKeyLfo[3]);
	fb1KeySoftTheta+=lfoRateC*(gui->fb1MixAndKeyLfo[5]);

	//fb1 geo1
	fb1XDisplaceTheta+=lfoRateC*(gui->fb1Geo1Lfo1[1]);
	fb1YDisplaceTheta+=lfoRateC*(gui->fb1Geo1Lfo1[3]);
	fb1ZDisplaceTheta+=lfoRateC*(gui->fb1Geo1Lfo1[5]);
	fb1RotateTheta+=lfoRateC*(gui->fb1Geo1Lfo1[7]);

	//fb1 geo2
	fb1ShearMatrix1Theta+=lfoRateC*(gui->fb1Geo1Lfo2[1]);
	fb1ShearMatrix2Theta+=lfoRateC*(gui->fb1Geo1Lfo2[5]);
	fb1ShearMatrix3Theta+=lfoRateC*(gui->fb1Geo1Lfo2[7]);
	fb1ShearMatrix4Theta+=lfoRateC*(gui->fb1Geo1Lfo2[3]);
	fb1KaleidoscopeSliceTheta+=lfoRateC*(gui->fb1Geo1Lfo2[9]);

	fb1HueAttenuateTheta+=lfoRateC*(gui->fb1Color1Lfo1[1]);
	fb1SaturationAttenuateTheta+=lfoRateC*(gui->fb1Color1Lfo1[3]);
	fb1BrightAttenuateTheta+=lfoRateC*(gui->fb1Color1Lfo1[5]);

	//block2Input adjust
	block2InputXDisplaceTheta+=lfoRateC*(gui->block2InputAdjustLfo[1]);
	block2InputYDisplaceTheta+=lfoRateC*(gui->block2InputAdjustLfo[3]);
	block2InputZDisplaceTheta+=lfoRateC*(gui->block2InputAdjustLfo[5]);
	block2InputRotateTheta+=lfoRateC*(gui->block2InputAdjustLfo[7]);
	block2InputHueAttenuateTheta+=lfoRateC*(gui->block2InputAdjustLfo[9]);
	block2InputSaturationAttenuateTheta+=lfoRateC*(gui->block2InputAdjustLfo[11]);
	block2InputBrightAttenuateTheta+=lfoRateC*(gui->block2InputAdjustLfo[13]);
	block2InputKaleidoscopeSliceTheta+=lfoRateC*(gui->block2InputAdjustLfo[15]);

	//fb2 mix and key
	fb2MixAmountTheta+=lfoRateC*(gui->fb2MixAndKeyLfo[1]);
	fb2KeyThresholdTheta+=lfoRateC*(gui->fb2MixAndKeyLfo[3]);
	fb2KeySoftTheta+=lfoRateC*(gui->fb2MixAndKeyLfo[5]);

	//fb2 geo1
	fb2XDisplaceTheta+=lfoRateC*(gui->fb2Geo1Lfo1[1]);
	fb2YDisplaceTheta+=lfoRateC*(gui->fb2Geo1Lfo1[3]);
	fb2ZDisplaceTheta+=lfoRateC*(gui->fb2Geo1Lfo1[5]);
	fb2RotateTheta+=lfoRateC*(gui->fb2Geo1Lfo1[7]);
	//fb2 geo2
	fb2ShearMatrix1Theta+=lfoRateC*(gui->fb2Geo1Lfo2[1]);
	fb2ShearMatrix2Theta+=lfoRateC*(gui->fb2Geo1Lfo2[5]);
	fb2ShearMatrix3Theta+=lfoRateC*(gui->fb2Geo1Lfo2[7]);
	fb2ShearMatrix4Theta+=lfoRateC*(gui->fb2Geo1Lfo2[3]);
	fb2KaleidoscopeSliceTheta+=lfoRateC*(gui->fb2Geo1Lfo2[9]);

	//fb2 color
	fb2HueAttenuateTheta+=lfoRateC*(gui->fb2Color1Lfo1[1]);
	fb2SaturationAttenuateTheta+=lfoRateC*(gui->fb2Color1Lfo1[3]);
	fb2BrightAttenuateTheta+=lfoRateC*(gui->fb2Color1Lfo1[5]);

	//BLOCK 3

	//block1 geo
	block1XDisplaceTheta+=lfoRateC*(gui->block1Geo1Lfo1[1]);
	block1YDisplaceTheta+=lfoRateC*(gui->block1Geo1Lfo1[3]);
	block1ZDisplaceTheta+=lfoRateC*(gui->block1Geo1Lfo1[5]);
	block1RotateTheta+=lfoRateC*(gui->block1Geo1Lfo1[7]);

	block1ShearMatrix1Theta+=lfoRateC*(gui->block1Geo1Lfo2[1]);
	block1ShearMatrix2Theta+=lfoRateC*(gui->block1Geo1Lfo2[5]);
	block1ShearMatrix3Theta+=lfoRateC*(gui->block1Geo1Lfo2[7]);
	block1ShearMatrix4Theta+=lfoRateC*(gui->block1Geo1Lfo2[3]);
	block1KaleidoscopeSliceTheta+=lfoRateC*(gui->block1Geo1Lfo2[9]);

	//block1 colorize
	block1ColorizeHueBand1Theta+=lfoRateC*(gui->block1ColorizeLfo1[3]);
	block1ColorizeSaturationBand1Theta+=lfoRateC*(gui->block1ColorizeLfo1[4]);
	block1ColorizeBrightBand1Theta+=lfoRateC*(gui->block1ColorizeLfo1[5]);
	block1ColorizeHueBand2Theta+=lfoRateC*(gui->block1ColorizeLfo1[9]);
	block1ColorizeSaturationBand2Theta+=lfoRateC*(gui->block1ColorizeLfo1[10]);
	block1ColorizeBrightBand2Theta+=lfoRateC*(gui->block1ColorizeLfo1[11]);

	block1ColorizeHueBand3Theta+=lfoRateC*(gui->block1ColorizeLfo2[3]);;
	block1ColorizeSaturationBand3Theta+=lfoRateC*(gui->block1ColorizeLfo2[4]);
	block1ColorizeBrightBand3Theta+=lfoRateC*(gui->block1ColorizeLfo2[5]);
	block1ColorizeHueBand4Theta+=lfoRateC*(gui->block1ColorizeLfo2[9]);
	block1ColorizeSaturationBand4Theta+=lfoRateC*(gui->block1ColorizeLfo2[10]);
	block1ColorizeBrightBand4Theta+=lfoRateC*(gui->block1ColorizeLfo2[11]);

	block1ColorizeHueBand5Theta+=lfoRateC*(gui->block1ColorizeLfo3[3]);
	block1ColorizeSaturationBand5Theta+=lfoRateC*(gui->block1ColorizeLfo3[4]);
	block1ColorizeBrightBand5Theta+=lfoRateC*(gui->block1ColorizeLfo3[5]);

	//block2 geo
	block2XDisplaceTheta+=lfoRateC*(gui->block2Geo1Lfo1[1]);
	block2YDisplaceTheta+=lfoRateC*(gui->block2Geo1Lfo1[3]);
	block2ZDisplaceTheta+=lfoRateC*(gui->block2Geo1Lfo1[5]);
	block2RotateTheta+=lfoRateC*(gui->block2Geo1Lfo1[7]);

	block2ShearMatrix1Theta+=lfoRateC*(gui->block2Geo1Lfo2[1]);
	block2ShearMatrix2Theta+=lfoRateC*(gui->block2Geo1Lfo2[5]);
	block2ShearMatrix3Theta+=lfoRateC*(gui->block2Geo1Lfo2[7]);
	block2ShearMatrix4Theta+=lfoRateC*(gui->block2Geo1Lfo2[3]);
	block2KaleidoscopeSliceTheta+=lfoRateC*(gui->block2Geo1Lfo2[9]);

	//block2 colorize
	block2ColorizeHueBand1Theta+=lfoRateC*(gui->block2ColorizeLfo1[3]);
	block2ColorizeSaturationBand1Theta+=lfoRateC*(gui->block2ColorizeLfo1[4]);
	block2ColorizeBrightBand1Theta+=lfoRateC*(gui->block2ColorizeLfo1[5]);
	block2ColorizeHueBand2Theta+=lfoRateC*(gui->block2ColorizeLfo1[9]);
	block2ColorizeSaturationBand2Theta+=lfoRateC*(gui->block2ColorizeLfo1[10]);
	block2ColorizeBrightBand2Theta+=lfoRateC*(gui->block2ColorizeLfo1[11]);

	block2ColorizeHueBand3Theta+=lfoRateC*(gui->block2ColorizeLfo2[3]);;
	block2ColorizeSaturationBand3Theta+=lfoRateC*(gui->block2ColorizeLfo2[4]);
	block2ColorizeBrightBand3Theta+=lfoRateC*(gui->block2ColorizeLfo2[5]);
	block2ColorizeHueBand4Theta+=lfoRateC*(gui->block2ColorizeLfo2[9]);
	block2ColorizeSaturationBand4Theta+=lfoRateC*(gui->block2ColorizeLfo2[10]);
	block2ColorizeBrightBand4Theta+=lfoRateC*(gui->block2ColorizeLfo2[11]);

	block2ColorizeHueBand5Theta+=lfoRateC*(gui->block2ColorizeLfo3[3]);
	block2ColorizeSaturationBand5Theta+=lfoRateC*(gui->block2ColorizeLfo3[4]);
	block2ColorizeBrightBand5Theta+=lfoRateC*(gui->block2ColorizeLfo3[5]);

	//matrix mixer
	matrixMixBgRedIntoFgRedTheta+=lfoRateC*(gui->matrixMixLfo1[3]);
	matrixMixBgGreenIntoFgRedTheta+=lfoRateC*(gui->matrixMixLfo1[4]);
	matrixMixBgBlueIntoFgRedTheta+=lfoRateC*(gui->matrixMixLfo1[5]);

	matrixMixBgRedIntoFgGreenTheta+=lfoRateC*(gui->matrixMixLfo1[9]);
	matrixMixBgGreenIntoFgGreenTheta+=lfoRateC*(gui->matrixMixLfo1[10]);
	matrixMixBgBlueIntoFgGreenTheta+=lfoRateC*(gui->matrixMixLfo1[11]);

	matrixMixBgRedIntoFgBlueTheta+=lfoRateC*(gui->matrixMixLfo2[3]);
	matrixMixBgGreenIntoFgBlueTheta+=lfoRateC*(gui->matrixMixLfo2[4]);
	matrixMixBgBlueIntoFgBlueTheta+=lfoRateC*(gui->matrixMixLfo2[5]);

	//final mix and key
	finalMixAmountTheta+=lfoRateC*(gui->finalMixAndKeyLfo[1]);
	finalKeyThresholdTheta+=lfoRateC*(gui->finalMixAndKeyLfo[3]);
	finalKeySoftTheta+=lfoRateC*(gui->finalMixAndKeyLfo[5]);

	// Lissajous 1 LFO theta updates
	lissajous1XFreqLfoTheta += lfoRateC * gui->lissajous1XFreqLfoRate;
	lissajous1YFreqLfoTheta += lfoRateC * gui->lissajous1YFreqLfoRate;
	lissajous1ZFreqLfoTheta += lfoRateC * gui->lissajous1ZFreqLfoRate;
	lissajous1XAmpLfoTheta += lfoRateC * gui->lissajous1XAmpLfoRate;
	lissajous1YAmpLfoTheta += lfoRateC * gui->lissajous1YAmpLfoRate;
	lissajous1ZAmpLfoTheta += lfoRateC * gui->lissajous1ZAmpLfoRate;
	lissajous1XPhaseLfoTheta += lfoRateC * gui->lissajous1XPhaseLfoRate;
	lissajous1YPhaseLfoTheta += lfoRateC * gui->lissajous1YPhaseLfoRate;
	lissajous1ZPhaseLfoTheta += lfoRateC * gui->lissajous1ZPhaseLfoRate;
	lissajous1XOffsetLfoTheta += lfoRateC * gui->lissajous1XOffsetLfoRate;
	lissajous1YOffsetLfoTheta += lfoRateC * gui->lissajous1YOffsetLfoRate;
	lissajous1SpeedLfoTheta += lfoRateC * gui->lissajous1SpeedLfoRate;
	lissajous1SizeLfoTheta += lfoRateC * gui->lissajous1SizeLfoRate;
	lissajous1NumPointsLfoTheta += lfoRateC * gui->lissajous1NumPointsLfoRate;
	lissajous1LineWidthLfoTheta += lfoRateC * gui->lissajous1LineWidthLfoRate;
	lissajous1ColorSpeedLfoTheta += lfoRateC * gui->lissajous1ColorSpeedLfoRate;
	lissajous1HueLfoTheta += lfoRateC * gui->lissajous1HueLfoRate;
	lissajous1HueSpreadLfoTheta += lfoRateC * gui->lissajous1HueSpreadLfoRate;

	// Lissajous 2 LFO theta updates
	lissajous2XFreqLfoTheta += lfoRateC * gui->lissajous2XFreqLfoRate;
	lissajous2YFreqLfoTheta += lfoRateC * gui->lissajous2YFreqLfoRate;
	lissajous2ZFreqLfoTheta += lfoRateC * gui->lissajous2ZFreqLfoRate;
	lissajous2XAmpLfoTheta += lfoRateC * gui->lissajous2XAmpLfoRate;
	lissajous2YAmpLfoTheta += lfoRateC * gui->lissajous2YAmpLfoRate;
	lissajous2ZAmpLfoTheta += lfoRateC * gui->lissajous2ZAmpLfoRate;
	lissajous2XPhaseLfoTheta += lfoRateC * gui->lissajous2XPhaseLfoRate;
	lissajous2YPhaseLfoTheta += lfoRateC * gui->lissajous2YPhaseLfoRate;
	lissajous2ZPhaseLfoTheta += lfoRateC * gui->lissajous2ZPhaseLfoRate;
	lissajous2XOffsetLfoTheta += lfoRateC * gui->lissajous2XOffsetLfoRate;
	lissajous2YOffsetLfoTheta += lfoRateC * gui->lissajous2YOffsetLfoRate;
	lissajous2SpeedLfoTheta += lfoRateC * gui->lissajous2SpeedLfoRate;
	lissajous2SizeLfoTheta += lfoRateC * gui->lissajous2SizeLfoRate;
	lissajous2NumPointsLfoTheta += lfoRateC * gui->lissajous2NumPointsLfoRate;
	lissajous2LineWidthLfoTheta += lfoRateC * gui->lissajous2LineWidthLfoRate;
	lissajous2ColorSpeedLfoTheta += lfoRateC * gui->lissajous2ColorSpeedLfoRate;
	lissajous2HueLfoTheta += lfoRateC * gui->lissajous2HueLfoRate;
	lissajous2HueSpreadLfoTheta += lfoRateC * gui->lissajous2HueSpreadLfoRate;

}

//--------------------------------------------------------------
void ofApp::draw(){


	//coefficients for parameters
	//namespacing shit up here
	//since everything is coming in as just arrays
	//we should properly name things here i think
	//coefficients will just be the name plus C at the end
	//eventually we will want to use gui stuff to rescale Coefficients

	//ch1 coefficients


	//ch1 adjust parameters
	float ch1XDisplace=ch1XDisplaceC*(gui->ch1Adjust[0]);
	float ch1YDisplace=ch1YDisplaceC*(gui->ch1Adjust[1]);
	float ch1ZDisplace=(1.0f+gui->ch1Adjust[2]);
	float ch1Rotate=ch1RotateC*(gui->ch1Adjust[3]);
	float ch1HueAttenuate=1.0f+(gui->ch1Adjust[4]);
	float ch1SaturationAttenuate=1.0f+(gui->ch1Adjust[5]);
	float ch1BrightAttenuate=1.0f+(gui->ch1Adjust[6]);
	float ch1Posterize=ch1PosterizeC*(1.0f-gui->ch1Adjust[7])+1.0;
	float ch1PosterizeInvert=1.0f/ch1Posterize;
	bool ch1PosterizeSwitch=0;
	float ch1KaleidoscopeAmount=floor(ch1KaleidoscopeAmountC*(gui->ch1Adjust[8]));
	float ch1KaleidoscopeSlice=ch1KaleidoscopeSliceC*(gui->ch1Adjust[9]);
	float ch1BlurAmount=gui->ch1Adjust[10];
	float ch1BlurRadius=ch1FilterRadiusC*(gui->ch1Adjust[11])+1.0;
	float ch1SharpenAmount=ch1SharpenAmountC*(gui->ch1Adjust[12]);
	float ch1SharpenRadius=ch1FilterRadiusC*(gui->ch1Adjust[13])+1.0;
	float ch1FiltersBoost=(gui->ch1Adjust[14]);
	//ch2 mix and key
	float ch2MixAmount=mixAmountC*(gui->ch2MixAndKey[0]);
	float ch2KeyValueRed=keyC*(gui->ch2MixAndKey[1]);
	float ch2KeyValueGreen=keyC*(gui->ch2MixAndKey[2]);
	float ch2KeyValueBlue=keyC*(gui->ch2MixAndKey[3]);
	float ch2KeyThreshold=keyThresholdC*(gui->ch2MixAndKey[4]);
	float ch2KeySoft=(gui->ch2MixAndKey[5]);

	//ch2 adjust parameters
	float ch2XDisplace=ch2XDisplaceC*(gui->ch2Adjust[0]);
	float ch2YDisplace=ch2YDisplaceC*(gui->ch2Adjust[1]);
	float ch2ZDisplace=(1.0f+gui->ch2Adjust[2]);
	float ch2Rotate=ch2RotateC*(gui->ch2Adjust[3]);
	float ch2HueAttenuate=1.0f+(gui->ch2Adjust[4]);
	float ch2SaturationAttenuate=1.0f+(gui->ch2Adjust[5]);
	float ch2BrightAttenuate=1.0f+(gui->ch2Adjust[6]);
	float ch2Posterize=ch2PosterizeC*(1.0f-gui->ch2Adjust[7])+1.0;
	float ch2PosterizeInvert=1.0f/ch2Posterize;
	bool ch2PosterizeSwitch=0;
	float ch2KaleidoscopeAmount=floor(ch2KaleidoscopeAmountC*(gui->ch2Adjust[8]));
	float ch2KaleidoscopeSlice=ch2KaleidoscopeSliceC*(gui->ch2Adjust[9]);
	float ch2BlurAmount=gui->ch2Adjust[10];
	float ch2BlurRadius=ch2FilterRadiusC*(gui->ch2Adjust[11])+1.0;
	float ch2SharpenAmount=ch2SharpenAmountC*(gui->ch2Adjust[12]);
	float ch2SharpenRadius=ch2FilterRadiusC*(gui->ch2Adjust[13])+1.0;
	float ch2FiltersBoost=(gui->ch2Adjust[14]);

	//fb1 mix and key
	float fb1MixAmount=mixAmountC*(gui->fb1MixAndKey[0]);
	float fb1KeyValueRed=keyC*(gui->fb1MixAndKey[1]);
	float fb1KeyValueGreen=keyC*(gui->fb1MixAndKey[2]);
	float fb1KeyValueBlue=keyC*(gui->fb1MixAndKey[3]);
	float fb1KeyThreshold=keyThresholdC*(gui->fb1MixAndKey[4]);
	float fb1KeySoft=(gui->fb1MixAndKey[5]);

	//fb1 geo1
	float fb1XDisplace=fb1XDisplaceC*(gui->fb1Geo1[0]);
	float fb1YDisplace=fb1YDisplaceC*(gui->fb1Geo1[1]);
	float fb1ZDisplace=1.0f+fb1ZDisplaceC*(gui->fb1Geo1[2]);
	float fb1Rotate=fb1RotateC*(gui->fb1Geo1[3]);
	float fb1ShearMatrix1=fb1ShearMatrix1C*( 1.0f/fb1ShearMatrix1C + gui->fb1Geo1[4] );
	float fb1ShearMatrix2=fb1ShearMatrix2C*(gui->fb1Geo1[6]);
	float fb1ShearMatrix3=fb1ShearMatrix3C*(gui->fb1Geo1[7]);
	float fb1ShearMatrix4=fb1ShearMatrix4C*( 1.0f/fb1ShearMatrix1C + gui->fb1Geo1[5] );
	float fb1KaleidoscopeAmount= floor(fb1KaleidoscopeAmountC*(gui->fb1Geo1[8]));
	float fb1KaleidoscopeSlice=fb1KaleidoscopeSliceC*(gui->fb1Geo1[9]);

	//fb1 color1
	float fb1HueOffset=fb1HueOffsetC*(gui->fb1Color1[0]);
	float fb1SaturationOffset=fb1SaturationOffsetC*(gui->fb1Color1[1]);
	float fb1BrightOffset=fb1BrightOffsetC*(gui->fb1Color1[2]);
	float fb1HueAttenuate=1.0f+fb1HueAttenuateC*(gui->fb1Color1[3]);
	float fb1SaturationAttenuate=1.0f+fb1SaturationAttenuateC*(gui->fb1Color1[4]);
	float fb1BrightAttenuate=1.0f+fb1BrightAttenuateC*(gui->fb1Color1[5]);
	float fb1HuePowmap=1.0f+fb1HuePowmapC*(gui->fb1Color1[6]);
	float fb1SaturationPowmap=1.0f+fb1SaturationPowmapC*(gui->fb1Color1[7]);
	float fb1BrightPowmap=1.0f+fb1BrightPowmapC*(gui->fb1Color1[8]);
	float fb1HueShaper=fb1HueShaperC*(gui->fb1Color1[9]);
	float fb1Posterize=fb1PosterizeC*(1.0f-gui->fb1Color1[10])+1.0;
	float fb1PosterizeInvert=1.0f/fb1Posterize;
	bool fb1PosterizeSwitch=0;

	//fb1 filters
	float fb1BlurAmount=gui->fb1Filters[0];
	float fb1BlurRadius=fb1FilterRadiusC*(gui->fb1Filters[1])+1.0;
	float fb1SharpenAmount=fb1SharpenAmountC*(gui->fb1Filters[2]);
	float fb1SharpenRadius=fb1FilterRadiusC*(gui->fb1Filters[3])+1.0;
	float fb1TemporalFilter1Amount=fb1TemporalFilterAmountC*(gui->fb1Filters[4]);
	float fb1TemporalFilter1Resonance=(gui->fb1Filters[5]);
	float fb1TemporalFilter2Amount=fb1TemporalFilterAmountC*(gui->fb1Filters[6]);
	float fb1TemporalFilter2Resonance=(gui->fb1Filters[7]);
	float fb1FiltersBoost=(gui->fb1Filters[8]);

	//BLOCK 2

	//block2Input adjust parameters
	float block2InputXDisplace=block2InputXDisplaceC*(gui->block2InputAdjust[0]);
	float block2InputYDisplace=block2InputYDisplaceC*(gui->block2InputAdjust[1]);
	float block2InputZDisplace=(1.0f+gui->block2InputAdjust[2]);
	float block2InputRotate=block2InputRotateC*(gui->block2InputAdjust[3]);
	float block2InputHueAttenuate=1.0f+(gui->block2InputAdjust[4]);
	float block2InputSaturationAttenuate=1.0f+(gui->block2InputAdjust[5]);
	float block2InputBrightAttenuate=1.0f+(gui->block2InputAdjust[6]);
	float block2InputPosterize=block2InputPosterizeC*(1.0f-gui->block2InputAdjust[7])+1.0;
	float block2InputPosterizeInvert=1.0f/block2InputPosterize;
	bool block2InputPosterizeSwitch=0;
	float block2InputKaleidoscopeAmount=floor(block2InputKaleidoscopeAmountC*(gui->block2InputAdjust[8]));
	float block2InputKaleidoscopeSlice=block2InputKaleidoscopeSliceC*(gui->block2InputAdjust[9]);
	float block2InputBlurAmount=gui->block2InputAdjust[10];
	float block2InputBlurRadius=block2InputFilterRadiusC*(gui->block2InputAdjust[11])+1.0;
	float block2InputSharpenAmount=block2InputSharpenAmountC*(gui->block2InputAdjust[12]);
	float block2InputSharpenRadius=block2InputFilterRadiusC*(gui->block2InputAdjust[13])+1.0;
	float block2InputFiltersBoost=(gui->block2InputAdjust[14]);

	//fb2 mix and key
	float fb2MixAmount=mixAmountC*(gui->fb2MixAndKey[0]);
	float fb2KeyValueRed=keyC*(gui->fb2MixAndKey[1]);
	float fb2KeyValueGreen=keyC*(gui->fb2MixAndKey[2]);
	float fb2KeyValueBlue=keyC*(gui->fb2MixAndKey[3]);
	float fb2KeyThreshold=keyThresholdC*(gui->fb2MixAndKey[4]);
	float fb2KeySoft=(gui->fb2MixAndKey[5]);

	//fb2 geo1
	float fb2XDisplace=fb2XDisplaceC*(gui->fb2Geo1[0]);
	float fb2YDisplace=fb2YDisplaceC*(gui->fb2Geo1[1]);
	float fb2ZDisplace=1.0f+fb2ZDisplaceC*(gui->fb2Geo1[2]);
	float fb2Rotate=fb2RotateC*(gui->fb2Geo1[3]);
	float fb2ShearMatrix1=fb2ShearMatrix1C*( 1.0f/fb2ShearMatrix1C + gui->fb2Geo1[4] );
	float fb2ShearMatrix2=fb2ShearMatrix2C*(gui->fb2Geo1[6]);
	float fb2ShearMatrix3=fb2ShearMatrix3C*(gui->fb2Geo1[7]);
	float fb2ShearMatrix4=fb2ShearMatrix4C*( 1.0f/fb2ShearMatrix1C + gui->fb2Geo1[5] );
	float fb2KaleidoscopeAmount=floor(fb2KaleidoscopeAmountC*(gui->fb2Geo1[8]));
	float fb2KaleidoscopeSlice=fb2KaleidoscopeSliceC*(gui->fb2Geo1[9]);

	//fb2 color1
	float fb2HueOffset=fb2HueOffsetC*(gui->fb2Color1[0]);
	float fb2SaturationOffset=fb2SaturationOffsetC*(gui->fb2Color1[1]);
	float fb2BrightOffset=fb2BrightOffsetC*(gui->fb2Color1[2]);
	float fb2HueAttenuate=1.0f+fb2HueAttenuateC*(gui->fb2Color1[3]);
	float fb2SaturationAttenuate=1.0f+fb2SaturationAttenuateC*(gui->fb2Color1[4]);
	float fb2BrightAttenuate=1.0f+fb2BrightAttenuateC*(gui->fb2Color1[5]);
	float fb2HuePowmap=1.0f+fb2HuePowmapC*(gui->fb2Color1[6]);
	float fb2SaturationPowmap=1.0f+fb2SaturationPowmapC*(gui->fb2Color1[7]);
	float fb2BrightPowmap=1.0f+fb2BrightPowmapC*(gui->fb2Color1[8]);
	float fb2HueShaper=fb2HueShaperC*(gui->fb2Color1[9]);
	float fb2Posterize=fb2PosterizeC*(1.0f-gui->fb2Color1[10])+1.0;
	float fb2PosterizeInvert=1.0f/fb2Posterize;
	bool fb2PosterizeSwitch=0;

	//fb2 filters
	float fb2BlurAmount=gui->fb2Filters[0];
	float fb2BlurRadius=fb2FilterRadiusC*(gui->fb2Filters[1])+1.0;
	float fb2SharpenAmount=fb2SharpenAmountC*(gui->fb2Filters[2]);
	float fb2SharpenRadius=fb2FilterRadiusC*(gui->fb2Filters[3])+1.0;
	float fb2TemporalFilter1Amount=fb2TemporalFilterAmountC*(gui->fb2Filters[4]);
	float fb2TemporalFilter1Resonance=(gui->fb2Filters[5]);
	float fb2TemporalFilter2Amount=fb2TemporalFilterAmountC*(gui->fb2Filters[6]);
	float fb2TemporalFilter2Resonance=(gui->fb2Filters[7]);
	float fb2FiltersBoost=(gui->fb2Filters[8]);


	//BLOCK3

	//block1 Geo
	float block1XDisplace=block1XDisplaceC*(gui->block1Geo[0]);
	float block1YDisplace=block1YDisplaceC*(gui->block1Geo[1]);
	float block1ZDisplace=1.0f+block1ZDisplaceC*(gui->block1Geo[2]);
	float block1Rotate=block1RotateC*(gui->block1Geo[3]);
	float block1ShearMatrix1=block1ShearMatrix1C*( 1.0f/block1ShearMatrix1C + gui->block1Geo[4] );
	float block1ShearMatrix2=block1ShearMatrix2C*(gui->block1Geo[6]);
	float block1ShearMatrix3=block1ShearMatrix3C*(gui->block1Geo[7]);
	float block1ShearMatrix4=block1ShearMatrix4C*( 1.0f/block1ShearMatrix1C + gui->block1Geo[5] );
	float block1KaleidoscopeAmount=floor(block1KaleidoscopeAmountC*(gui->block1Geo[8]));
	float block1KaleidoscopeSlice=block1KaleidoscopeSliceC*(gui->block1Geo[9]);

	//block1 colorize
	//yes these are poorly named b/c HSB and RGB switching.  deal with it
	float block1ColorizeHueBand1=(gui->block1Colorize[0]);
	float block1ColorizeSaturationBand1=(gui->block1Colorize[1]);
	float block1ColorizeBrightBand1=(gui->block1Colorize[2]);
	float block1ColorizeHueBand2=(gui->block1Colorize[3]);
	float block1ColorizeSaturationBand2=(gui->block1Colorize[4]);
	float block1ColorizeBrightBand2=(gui->block1Colorize[5]);
	float block1ColorizeHueBand3=(gui->block1Colorize[6]);
	float block1ColorizeSaturationBand3=(gui->block1Colorize[7]);
	float block1ColorizeBrightBand3=(gui->block1Colorize[8]);
	float block1ColorizeHueBand4=(gui->block1Colorize[9]);
	float block1ColorizeSaturationBand4=(gui->block1Colorize[10]);
	float block1ColorizeBrightBand4=(gui->block1Colorize[11]);
	float block1ColorizeHueBand5=(gui->block1Colorize[12]);
	float block1ColorizeSaturationBand5=(gui->block1Colorize[13]);
	float block1ColorizeBrightBand5=(gui->block1Colorize[14]);

	//block1 filters
	float block1BlurAmount=gui->block1Filters[0];
	float block1BlurRadius=block1FilterRadiusC*(gui->block1Filters[1])+1.0;
	float block1SharpenAmount=block1SharpenAmountC*(gui->block1Filters[2]);
	float block1SharpenRadius=block1FilterRadiusC*(gui->block1Filters[3])+1.0;
	float block1FiltersBoost=(gui->block1Filters[4]);
	float block1Dither=(block1DitherC-1.0)*(1.0-gui->block1Filters[5])+2.0;

	//block2 Geo
	float block2XDisplace=block2XDisplaceC*(gui->block2Geo[0]);
	float block2YDisplace=block2YDisplaceC*(gui->block2Geo[1]);
	float block2ZDisplace=1.0f+block2ZDisplaceC*(gui->block2Geo[2]);
	float block2Rotate=block2RotateC*(gui->block2Geo[3]);
	float block2ShearMatrix1=block2ShearMatrix1C*( 1.0f/block2ShearMatrix1C + gui->block2Geo[4] );
	float block2ShearMatrix2=block2ShearMatrix2C*(gui->block2Geo[6]);
	float block2ShearMatrix3=block2ShearMatrix3C*(gui->block2Geo[7]);
	float block2ShearMatrix4=block2ShearMatrix4C*( 1.0f/block2ShearMatrix1C + gui->block2Geo[5] );
	float block2KaleidoscopeAmount=floor(block2KaleidoscopeAmountC*(gui->block2Geo[8]));
	float block2KaleidoscopeSlice=block2KaleidoscopeSliceC*(gui->block2Geo[9]);

	// block2 colorize
	float block2ColorizeHueBand1=(gui->block2Colorize[0]);
	float block2ColorizeSaturationBand1=(gui->block2Colorize[1]);
	float block2ColorizeBrightBand1=(gui->block2Colorize[2]);
	float block2ColorizeHueBand2=(gui->block2Colorize[3]);
	float block2ColorizeSaturationBand2=(gui->block2Colorize[4]);
	float block2ColorizeBrightBand2=(gui->block2Colorize[5]);
	float block2ColorizeHueBand3=(gui->block2Colorize[6]);
	float block2ColorizeSaturationBand3=(gui->block2Colorize[7]);
	float block2ColorizeBrightBand3=(gui->block2Colorize[8]);
	float block2ColorizeHueBand4=(gui->block2Colorize[9]);
	float block2ColorizeSaturationBand4=(gui->block2Colorize[10]);
	float block2ColorizeBrightBand4=(gui->block2Colorize[11]);
	float block2ColorizeHueBand5=(gui->block2Colorize[12]);
	float block2ColorizeSaturationBand5=(gui->block2Colorize[13]);
	float block2ColorizeBrightBand5=(gui->block2Colorize[14]);

	//block2 filters
	float block2BlurAmount=gui->block2Filters[0];
	float block2BlurRadius=block2FilterRadiusC*(gui->block2Filters[1])+1.0;
	float block2SharpenAmount=block2SharpenAmountC*(gui->block2Filters[2]);
	float block2SharpenRadius=block2FilterRadiusC*(gui->block2Filters[3])+1.0;
	float block2FiltersBoost=(gui->block2Filters[4]);
	float block2Dither=(block2DitherC-1.0)*(1.0-gui->block2Filters[5])+2.0;

	//matrixMixer
	float matrixMixBgRedIntoFgRed=matrixMixC*(gui->matrixMix[0]);
	float matrixMixBgGreenIntoFgRed=matrixMixC*(gui->matrixMix[1]);
	float matrixMixBgBlueIntoFgRed=matrixMixC*(gui->matrixMix[2]);

	float matrixMixBgRedIntoFgGreen=matrixMixC*(gui->matrixMix[3]);
	float matrixMixBgGreenIntoFgGreen=matrixMixC*(gui->matrixMix[4]);
	float matrixMixBgBlueIntoFgGreen=matrixMixC*(gui->matrixMix[5]);

	float matrixMixBgRedIntoFgBlue=matrixMixC*(gui->matrixMix[6]);
	float matrixMixBgGreenIntoFgBlue=matrixMixC*(gui->matrixMix[7]);
	float matrixMixBgBlueIntoFgBlue=matrixMixC*(gui->matrixMix[8]);

	//final mix and key
	float finalMixAmount=mixAmountC*(gui->finalMixAndKey[0]);
	float finalKeyValueRed=keyC*(gui->finalMixAndKey[1]);
	float finalKeyValueGreen=keyC*(gui->finalMixAndKey[2]);
	float finalKeyValueBlue=keyC*(gui->finalMixAndKey[3]);
	float finalKeyThreshold=keyThresholdC*(gui->finalMixAndKey[4]);
	float finalKeySoft=(gui->finalMixAndKey[5]);

	//lfo shit up

	//BLOCK1 inputs lfo

	//ch1
	ch1XDisplace+=lfo(ch1XDisplaceC*(gui->ch1AdjustLfo[0]),ch1XDisplaceTheta,gui->ch1AdjustLfoShape[0]);
	ch1YDisplace+=lfo(ch1YDisplaceC*(gui->ch1AdjustLfo[2]),ch1YDisplaceTheta,gui->ch1AdjustLfoShape[1]);
	ch1ZDisplace+=lfo(ch1ZDisplaceC*(gui->ch1AdjustLfo[4]),ch1ZDisplaceTheta,gui->ch1AdjustLfoShape[2]);
	ch1Rotate+=lfo(ch1RotateC*(gui->ch1AdjustLfo[6]),ch1RotateTheta,gui->ch1AdjustLfoShape[3]);
	ch1HueAttenuate+=lfo((gui->ch1AdjustLfo[8]),ch1HueAttenuateTheta,gui->ch1AdjustLfoShape[4]);
	ch1SaturationAttenuate+=lfo((gui->ch1AdjustLfo[10]),ch1SaturationAttenuateTheta,gui->ch1AdjustLfoShape[5]);
	ch1BrightAttenuate+=lfo((gui->ch1AdjustLfo[12]),ch1BrightAttenuateTheta,gui->ch1AdjustLfoShape[6]);
	ch1KaleidoscopeSlice+=lfo( ch1KaleidoscopeSliceC*(gui->ch1AdjustLfo[14]),ch1KaleidoscopeSliceTheta ,gui->ch1AdjustLfoShape[7] );

	//ch2 lfo add on
	ch2MixAmount+=lfo(mixAmountC*(gui->ch2MixAndKeyLfo[0]),ch2MixAmountTheta,gui->ch2MixAndKeyLfoShape[0]);
	ch2KeyThreshold+=lfo(keyThresholdC*(gui->ch2MixAndKeyLfo[2]),ch2KeyThresholdTheta,gui->ch2MixAndKeyLfoShape[1]);
	ch2KeySoft+=lfo((gui->ch2MixAndKeyLfo[4]),ch2KeySoftTheta,gui->ch2MixAndKeyLfoShape[2]);

	ch2XDisplace+=lfo(ch2XDisplaceC*(gui->ch2AdjustLfo[0]),ch2XDisplaceTheta,gui->ch2AdjustLfoShape[0]);
	ch2YDisplace+=lfo(ch2YDisplaceC*(gui->ch2AdjustLfo[2]),ch2YDisplaceTheta,gui->ch2AdjustLfoShape[1]);
	ch2ZDisplace+=lfo(ch2ZDisplaceC*(gui->ch2AdjustLfo[4]),ch2ZDisplaceTheta,gui->ch2AdjustLfoShape[2]);
	ch2Rotate+=lfo(ch2RotateC*(gui->ch2AdjustLfo[6]),ch2RotateTheta,gui->ch2AdjustLfoShape[3]);
	ch2HueAttenuate+=lfo((gui->ch2AdjustLfo[8]),ch2HueAttenuateTheta,gui->ch2AdjustLfoShape[4]);
	ch2SaturationAttenuate+=lfo((gui->ch2AdjustLfo[10]),ch2SaturationAttenuateTheta,gui->ch2AdjustLfoShape[5]);
	ch2BrightAttenuate+=lfo((gui->ch2AdjustLfo[12]),ch2BrightAttenuateTheta,gui->ch2AdjustLfoShape[6]);
	ch2KaleidoscopeSlice+=lfo( ch2KaleidoscopeSliceC*(gui->ch2AdjustLfo[14]),ch2KaleidoscopeSliceTheta ,gui->ch2AdjustLfoShape[7] );

	//fb1 lfo add on
	fb1MixAmount+=lfo(mixAmountC*(gui->fb1MixAndKeyLfo[0]),fb1MixAmountTheta,gui->fb1MixAndKeyLfoShape[0]);
	fb1KeyThreshold+=lfo(keyThresholdC*(gui->fb1MixAndKeyLfo[2]),fb1KeyThresholdTheta,gui->fb1MixAndKeyLfoShape[1]);
	fb1KeySoft+=lfo((gui->fb1MixAndKeyLfo[4]),fb1KeySoftTheta,gui->fb1MixAndKeyLfoShape[2]);

	fb1XDisplace+=lfo(fb1XDisplaceC*(gui->fb1Geo1Lfo1[0]),fb1XDisplaceTheta,gui->fb1Geo1Lfo1Shape[0]);
	fb1YDisplace+=lfo(fb1YDisplaceC*(gui->fb1Geo1Lfo1[2]),fb1YDisplaceTheta,gui->fb1Geo1Lfo1Shape[1]);
	fb1ZDisplace+=lfo(fb1ZDisplaceC*(gui->fb1Geo1Lfo1[4]),fb1ZDisplaceTheta,gui->fb1Geo1Lfo1Shape[2]);
	fb1Rotate+=lfo(fb1RotateC*(gui->fb1Geo1Lfo1[6]),fb1RotateTheta,gui->fb1Geo1Lfo1Shape[3]);

	fb1ShearMatrix1+=lfo(fb1ShearMatrix1C*(gui->fb1Geo1Lfo2[0]),fb1ShearMatrix1Theta,gui->fb1Geo1Lfo2Shape[0]);
	fb1ShearMatrix2+=lfo(fb1ShearMatrix2C*(gui->fb1Geo1Lfo2[4]),fb1ShearMatrix2Theta,gui->fb1Geo1Lfo2Shape[2]);
	fb1ShearMatrix3+=lfo(fb1ShearMatrix3C*(gui->fb1Geo1Lfo2[6]),fb1ShearMatrix3Theta,gui->fb1Geo1Lfo2Shape[3]);
	fb1ShearMatrix4+=lfo(fb1ShearMatrix4C*(gui->fb1Geo1Lfo2[2]),fb1ShearMatrix4Theta,gui->fb1Geo1Lfo2Shape[1]);
	fb1KaleidoscopeSlice+=lfo(fb1KaleidoscopeSliceC*(gui->fb1Geo1Lfo2[8]),fb1KaleidoscopeSliceTheta,gui->fb1Geo1Lfo2Shape[4]);

	fb1HueAttenuate+=lfo(fb1HueAttenuateC*(gui->fb1Color1Lfo1[0]),fb1HueAttenuateTheta,gui->fb1Color1Lfo1Shape[0]);
	fb1SaturationAttenuate+=lfo(fb1SaturationAttenuateC*(gui->fb1Color1Lfo1[2]),fb1SaturationAttenuateTheta,gui->fb1Color1Lfo1Shape[1]);
	fb1BrightAttenuate+=lfo(fb1BrightAttenuateC*(gui->fb1Color1Lfo1[4]),fb1BrightAttenuateTheta,gui->fb1Color1Lfo1Shape[2]);


	//block2Input
	block2InputXDisplace+=lfo(block2InputXDisplaceC*(gui->block2InputAdjustLfo[0]),block2InputXDisplaceTheta,gui->block2InputAdjustLfoShape[0]);
	block2InputYDisplace+=lfo(block2InputYDisplaceC*(gui->block2InputAdjustLfo[2]),block2InputYDisplaceTheta,gui->block2InputAdjustLfoShape[1]);
	block2InputZDisplace+=lfo(block2InputZDisplaceC*(gui->block2InputAdjustLfo[4]),block2InputZDisplaceTheta,gui->block2InputAdjustLfoShape[2]);
	block2InputRotate+=lfo(block2InputRotateC*(gui->block2InputAdjustLfo[6]),block2InputRotateTheta,gui->block2InputAdjustLfoShape[3]);
	block2InputHueAttenuate+=lfo((gui->block2InputAdjustLfo[8]),block2InputHueAttenuateTheta,gui->block2InputAdjustLfoShape[4]);
	block2InputSaturationAttenuate+=lfo((gui->block2InputAdjustLfo[10]),block2InputSaturationAttenuateTheta,gui->block2InputAdjustLfoShape[5]);
	block2InputBrightAttenuate+=lfo((gui->block2InputAdjustLfo[12]),block2InputBrightAttenuateTheta,gui->block2InputAdjustLfoShape[6]);
	block2InputKaleidoscopeSlice+=lfo( block2InputKaleidoscopeSliceC*(gui->block2InputAdjustLfo[14]),
		block2InputKaleidoscopeSliceTheta ,gui->block2InputAdjustLfoShape[7] );

	//fb2 lfo addon
	fb2MixAmount+=lfo(mixAmountC*(gui->fb2MixAndKeyLfo[0]),fb2MixAmountTheta,gui->fb2MixAndKeyLfoShape[0]);
	fb2KeyThreshold+=lfo(keyThresholdC*(gui->fb2MixAndKeyLfo[2]),fb2KeyThresholdTheta,gui->fb2MixAndKeyLfoShape[1]);
	fb2KeySoft+=lfo((gui->fb2MixAndKeyLfo[4]),fb2KeySoftTheta,gui->fb2MixAndKeyLfoShape[2]);

	fb2XDisplace+=lfo(fb2XDisplaceC*(gui->fb2Geo1Lfo1[0]),fb2XDisplaceTheta,gui->fb2Geo1Lfo1Shape[0]);
	fb2YDisplace+=lfo(fb2YDisplaceC*(gui->fb2Geo1Lfo1[2]),fb2YDisplaceTheta,gui->fb2Geo1Lfo1Shape[1]);
	fb2ZDisplace+=lfo(fb2ZDisplaceC*(gui->fb2Geo1Lfo1[4]),fb2ZDisplaceTheta,gui->fb2Geo1Lfo1Shape[2]);
	fb2Rotate+=lfo(fb2RotateC*(gui->fb2Geo1Lfo1[6]),fb2RotateTheta,gui->fb2Geo1Lfo1Shape[3]);


	fb2ShearMatrix1+=lfo(fb2ShearMatrix1C*(gui->fb2Geo1Lfo2[0]),fb2ShearMatrix1Theta,gui->fb2Geo1Lfo2Shape[0]);
	fb2ShearMatrix2+=lfo(fb2ShearMatrix2C*(gui->fb2Geo1Lfo2[4]),fb2ShearMatrix2Theta,gui->fb2Geo1Lfo2Shape[2]);
	fb2ShearMatrix3+=lfo(fb2ShearMatrix3C*(gui->fb2Geo1Lfo2[6]),fb2ShearMatrix3Theta,gui->fb2Geo1Lfo2Shape[3]);
	fb2ShearMatrix4+=lfo(fb2ShearMatrix4C*(gui->fb2Geo1Lfo2[2]),fb2ShearMatrix4Theta,gui->fb2Geo1Lfo2Shape[1]);
	fb2KaleidoscopeSlice+=lfo(fb2KaleidoscopeSliceC*(gui->fb2Geo1Lfo2[8]),fb2KaleidoscopeSliceTheta,gui->fb2Geo1Lfo2Shape[4]);

	fb2HueAttenuate+=lfo(fb2HueAttenuateC*(gui->fb2Color1Lfo1[0]),fb2HueAttenuateTheta,gui->fb2Color1Lfo1Shape[0]);
	fb2SaturationAttenuate+=lfo(fb2SaturationAttenuateC*(gui->fb2Color1Lfo1[2]),fb2SaturationAttenuateTheta,gui->fb2Color1Lfo1Shape[1]);
	fb2BrightAttenuate+=lfo(fb2BrightAttenuateC*(gui->fb2Color1Lfo1[4]),fb2BrightAttenuateTheta,gui->fb2Color1Lfo1Shape[2]);

	//BLOCK3

	//block1 geo
	block1XDisplace+=lfo(block1XDisplaceC*(gui->block1Geo1Lfo1[0]),block1XDisplaceTheta,gui->block1Geo1Lfo1Shape[0]);
	block1YDisplace+=lfo(block1YDisplaceC*(gui->block1Geo1Lfo1[2]),block1YDisplaceTheta,gui->block1Geo1Lfo1Shape[1]);
	block1ZDisplace+=lfo(block1ZDisplaceC*(gui->block1Geo1Lfo1[4]),block1ZDisplaceTheta,gui->block1Geo1Lfo1Shape[2]);
	block1Rotate+=lfo(block1RotateC*(gui->block1Geo1Lfo1[6]),block1RotateTheta,gui->block1Geo1Lfo1Shape[3]);

	block1ShearMatrix1+=lfo(block1ShearMatrix1C*(gui->block1Geo1Lfo2[0]),block1ShearMatrix1Theta,gui->block1Geo1Lfo2Shape[0]);
	block1ShearMatrix2+=lfo(block1ShearMatrix2C*(gui->block1Geo1Lfo2[4]),block1ShearMatrix2Theta,gui->block1Geo1Lfo2Shape[2]);
	block1ShearMatrix3+=lfo(block1ShearMatrix3C*(gui->block1Geo1Lfo2[6]),block1ShearMatrix3Theta,gui->block1Geo1Lfo2Shape[3]);
	block1ShearMatrix4+=lfo(block1ShearMatrix4C*(gui->block1Geo1Lfo2[2]),block1ShearMatrix4Theta,gui->block1Geo1Lfo2Shape[1]);
	block1KaleidoscopeSlice+=lfo(block1KaleidoscopeSliceC*(gui->block1Geo1Lfo2[8]),block1KaleidoscopeSliceTheta,gui->block1Geo1Lfo2Shape[4]);

	//block1 colorize
	block1ColorizeHueBand1+=lfo( (gui->block1ColorizeLfo1[0]) , block1ColorizeHueBand1Theta , gui->block1ColorizeLfo1Shape[0]  );
	block1ColorizeSaturationBand1+=lfo( (gui->block1ColorizeLfo1[1]) , block1ColorizeSaturationBand1Theta , gui->block1ColorizeLfo1Shape[1]  );
	block1ColorizeBrightBand1+=lfo( (gui->block1ColorizeLfo1[2]) , block1ColorizeBrightBand1Theta , gui->block1ColorizeLfo1Shape[2]  );
	block1ColorizeHueBand2+=lfo( (gui->block1ColorizeLfo1[6]) , block1ColorizeHueBand2Theta , gui->block1ColorizeLfo1Shape[3]  );
	block1ColorizeSaturationBand2+=lfo( (gui->block1ColorizeLfo1[7]) , block1ColorizeSaturationBand2Theta , gui->block1ColorizeLfo1Shape[4]  );
	block1ColorizeBrightBand2+=lfo( (gui->block1ColorizeLfo1[8]) , block1ColorizeBrightBand2Theta , gui->block1ColorizeLfo1Shape[5]  );

	block1ColorizeHueBand3+=lfo( (gui->block1ColorizeLfo2[0]) , block1ColorizeHueBand3Theta , gui->block1ColorizeLfo2Shape[0]  );
	block1ColorizeSaturationBand3+=lfo( (gui->block1ColorizeLfo2[1]) , block1ColorizeSaturationBand3Theta , gui->block1ColorizeLfo2Shape[1]  );
	block1ColorizeBrightBand3+=lfo( (gui->block1ColorizeLfo2[2]) , block1ColorizeBrightBand3Theta , gui->block1ColorizeLfo2Shape[2]  );
	block1ColorizeHueBand4+=lfo( (gui->block1ColorizeLfo2[6]) , block1ColorizeHueBand4Theta , gui->block1ColorizeLfo2Shape[3]  );
	block1ColorizeSaturationBand4+=lfo( (gui->block1ColorizeLfo2[7]) , block1ColorizeSaturationBand4Theta , gui->block1ColorizeLfo2Shape[4]  );
	block1ColorizeBrightBand4+=lfo( (gui->block1ColorizeLfo2[8]) , block1ColorizeBrightBand4Theta , gui->block1ColorizeLfo2Shape[5]  );

	block1ColorizeHueBand5+=lfo( (gui->block1ColorizeLfo3[0]) , block1ColorizeHueBand5Theta , gui->block1ColorizeLfo3Shape[0]  );
	block1ColorizeSaturationBand5+=lfo( (gui->block1ColorizeLfo3[1]) , block1ColorizeSaturationBand5Theta , gui->block1ColorizeLfo3Shape[1]  );
	block1ColorizeBrightBand5+=lfo( (gui->block1ColorizeLfo3[2]) , block1ColorizeBrightBand5Theta , gui->block1ColorizeLfo3Shape[2]  );

	//block2 geo
	block2XDisplace+=lfo(block2XDisplaceC*(gui->block2Geo1Lfo1[0]),block2XDisplaceTheta,gui->block2Geo1Lfo1Shape[0]);
	block2YDisplace+=lfo(block2YDisplaceC*(gui->block2Geo1Lfo1[2]),block2YDisplaceTheta,gui->block2Geo1Lfo1Shape[1]);
	block2ZDisplace+=lfo(block2ZDisplaceC*(gui->block2Geo1Lfo1[4]),block2ZDisplaceTheta,gui->block2Geo1Lfo1Shape[2]);
	block2Rotate+=lfo(block2RotateC*(gui->block2Geo1Lfo1[6]),block2RotateTheta,gui->block2Geo1Lfo1Shape[3]);

	block2ShearMatrix1+=lfo(block2ShearMatrix1C*(gui->block2Geo1Lfo2[0]),block2ShearMatrix1Theta,gui->block2Geo1Lfo2Shape[0]);
	block2ShearMatrix2+=lfo(block2ShearMatrix2C*(gui->block2Geo1Lfo2[4]),block2ShearMatrix2Theta,gui->block2Geo1Lfo2Shape[2]);
	block2ShearMatrix3+=lfo(block2ShearMatrix3C*(gui->block2Geo1Lfo2[6]),block2ShearMatrix3Theta,gui->block2Geo1Lfo2Shape[3]);
	block2ShearMatrix4+=lfo(block2ShearMatrix4C*(gui->block2Geo1Lfo2[2]),block2ShearMatrix4Theta,gui->block2Geo1Lfo2Shape[1]);
	block2KaleidoscopeSlice+=lfo(block2KaleidoscopeSliceC*(gui->block2Geo1Lfo2[8]),block2KaleidoscopeSliceTheta,gui->block2Geo1Lfo2Shape[4]);

	//block2 colorize
	block2ColorizeHueBand1+=lfo( (gui->block2ColorizeLfo1[0]) , block2ColorizeHueBand1Theta , gui->block2ColorizeLfo1Shape[0]  );
	block2ColorizeSaturationBand1+=lfo( (gui->block2ColorizeLfo1[1]) , block2ColorizeSaturationBand1Theta , gui->block2ColorizeLfo1Shape[1]  );
	block2ColorizeBrightBand1+=lfo( (gui->block2ColorizeLfo1[2]) , block2ColorizeBrightBand1Theta , gui->block2ColorizeLfo1Shape[2]  );
	block2ColorizeHueBand2+=lfo( (gui->block2ColorizeLfo1[6]) , block2ColorizeHueBand2Theta , gui->block2ColorizeLfo1Shape[3]  );
	block2ColorizeSaturationBand2+=lfo( (gui->block2ColorizeLfo1[7]) , block2ColorizeSaturationBand2Theta , gui->block2ColorizeLfo1Shape[4]  );
	block2ColorizeBrightBand2+=lfo( (gui->block2ColorizeLfo1[8]) , block2ColorizeBrightBand2Theta , gui->block2ColorizeLfo1Shape[5]  );

	block2ColorizeHueBand3+=lfo( (gui->block2ColorizeLfo2[0]) , block2ColorizeHueBand3Theta , gui->block2ColorizeLfo2Shape[0]  );
	block2ColorizeSaturationBand3+=lfo( (gui->block2ColorizeLfo2[1]) , block2ColorizeSaturationBand3Theta , gui->block2ColorizeLfo2Shape[1]  );
	block2ColorizeBrightBand3+=lfo( (gui->block2ColorizeLfo2[2]) , block2ColorizeBrightBand3Theta , gui->block2ColorizeLfo2Shape[2]  );
	block2ColorizeHueBand4+=lfo( (gui->block2ColorizeLfo2[6]) , block2ColorizeHueBand4Theta , gui->block2ColorizeLfo2Shape[3]  );
	block2ColorizeSaturationBand4+=lfo( (gui->block2ColorizeLfo2[7]) , block2ColorizeSaturationBand4Theta , gui->block2ColorizeLfo2Shape[4]  );
	block2ColorizeBrightBand4+=lfo( (gui->block2ColorizeLfo2[8]) , block2ColorizeBrightBand4Theta , gui->block2ColorizeLfo2Shape[5]  );

	block2ColorizeHueBand5+=lfo( (gui->block2ColorizeLfo3[0]) , block2ColorizeHueBand5Theta , gui->block2ColorizeLfo3Shape[0]  );
	block2ColorizeSaturationBand5+=lfo( (gui->block2ColorizeLfo3[1]) , block2ColorizeSaturationBand5Theta , gui->block2ColorizeLfo3Shape[1]  );
	block2ColorizeBrightBand5+=lfo( (gui->block2ColorizeLfo3[2]) , block2ColorizeBrightBand5Theta , gui->block2ColorizeLfo3Shape[2]  );


	//matrix mixer
	matrixMixBgRedIntoFgRed+=lfo( matrixMixC*(gui->matrixMixLfo1[0]), matrixMixBgRedIntoFgRedTheta , gui->matrixMixLfo1Shape[0] );
	matrixMixBgGreenIntoFgRed+=lfo( matrixMixC*(gui->matrixMixLfo1[1]), matrixMixBgGreenIntoFgRedTheta , gui->matrixMixLfo1Shape[1] );
	matrixMixBgBlueIntoFgRed+=lfo( matrixMixC*(gui->matrixMixLfo1[2]), matrixMixBgBlueIntoFgRedTheta , gui->matrixMixLfo1Shape[2] );

	matrixMixBgRedIntoFgGreen+=lfo( matrixMixC*(gui->matrixMixLfo1[6]), matrixMixBgRedIntoFgGreenTheta , gui->matrixMixLfo1Shape[3] );
	matrixMixBgGreenIntoFgGreen+=lfo( matrixMixC*(gui->matrixMixLfo1[7]), matrixMixBgGreenIntoFgGreenTheta , gui->matrixMixLfo1Shape[4] );
	matrixMixBgBlueIntoFgGreen+=lfo( matrixMixC*(gui->matrixMixLfo1[8]), matrixMixBgBlueIntoFgGreenTheta , gui->matrixMixLfo1Shape[5] );

	matrixMixBgRedIntoFgBlue+=lfo( matrixMixC*(gui->matrixMixLfo2[0]), matrixMixBgRedIntoFgBlueTheta , gui->matrixMixLfo2Shape[0] );
	matrixMixBgGreenIntoFgBlue+=lfo( matrixMixC*(gui->matrixMixLfo2[1]), matrixMixBgGreenIntoFgBlueTheta , gui->matrixMixLfo2Shape[1] );
	matrixMixBgBlueIntoFgBlue+=lfo( matrixMixC*(gui->matrixMixLfo2[2]), matrixMixBgBlueIntoFgBlueTheta , gui->matrixMixLfo2Shape[2] );

	//final lfo addon
	finalMixAmount+=lfo(mixAmountC*(gui->finalMixAndKeyLfo[0]),finalMixAmountTheta,gui->finalMixAndKeyLfoShape[0]);
	finalKeyThreshold+=lfo(keyThresholdC*(gui->finalMixAndKeyLfo[2]),finalKeyThresholdTheta,gui->finalMixAndKeyLfoShape[1]);
	finalKeySoft+=lfo((gui->finalMixAndKeyLfo[4]),finalKeySoftTheta,gui->finalMixAndKeyLfoShape[2]);



	//is this still used...
	float ratio=input1.getWidth()/ofGetWidth();

	framebuffer1.begin();
	// Explicitly set up viewport and projection for current FBO size
	ofViewport(0, 0, framebuffer1.getWidth(), framebuffer1.getHeight());
	ofSetupScreenOrtho(framebuffer1.getWidth(), framebuffer1.getHeight());
	shader1.begin();

	//various test parameters to delete later
	float ch1CribX=0;
	float ch2CribX=0;
	float cribY=60;
	float ch1HdZCrib=0;
	float ch2HdZCrib=0;
	shader1.setUniform1f("cribY",cribY);
	shader1.setUniform1f("width",internalWidth);
	shader1.setUniform1f("height",internalHeight);
	shader1.setUniform1f("inverseWidth",1.0f/internalWidth);
	shader1.setUniform1f("inverseHeight",1.0f/internalHeight);

	// Input resolution uniforms
	shader1.setUniform1f("input1Width", (float)input1Width);
	shader1.setUniform1f("input1Height", (float)input1Height);
	shader1.setUniform1f("inverseWidth1",1.0f/input1Width);
	shader1.setUniform1f("inverseHeight1",1.0f/input1Height);


	int fb1DelayTimeMacroBuffer=int((pastFramesSize-1.0)*(gui->fb1DelayTimeMacroBuffer));
	int fb1DelayTime_d=(gui->fb1DelayTime)+fb1DelayTimeMacroBuffer;

	int pastFrames1Index = (abs(pastFramesOffset - pastFramesSize - (fb1DelayTime_d) + 1) % pastFramesSize);
	int TemporalFilterIndex = (abs(pastFramesOffset - pastFramesSize + 1) % pastFramesSize);
	pastFrames1[pastFrames1Index].draw(0, 0, internalWidth, internalHeight);
	shader1.setUniformTexture("fb1TemporalFilter", pastFrames1[TemporalFilterIndex].getTexture(), 1);

	//channel selection
	if(gui->ch1InputSelect==0){
		// Use input1 (webcam, NDI, or Spout depending on source type)
		if (gui->input1SourceType == 0) {
			if (input1.isInitialized()) {
				shader1.setUniformTexture("ch1Tex",webcamFbo1.getTexture(),2);
			}
		} else if (gui->input1SourceType == 1) {
			shader1.setUniformTexture("ch1Tex",ndiFbo1.getTexture(),2);
		} else {
#if OFAPP_HAS_SPOUT
			shader1.setUniformTexture("ch1Tex",spoutFbo1.getTexture(),2);
#endif
		}

	}
	if(gui->ch1InputSelect==1){
		// Use input2 (webcam, NDI, or Spout depending on source type)
		if (gui->input2SourceType == 0) {
			if (input2.isInitialized()) {
				shader1.setUniformTexture("ch1Tex",webcamFbo2.getTexture(),2);
			}
		} else if (gui->input2SourceType == 1) {
			shader1.setUniformTexture("ch1Tex",ndiFbo2.getTexture(),2);
		} else {
#if OFAPP_HAS_SPOUT
			shader1.setUniformTexture("ch1Tex",spoutFbo2.getTexture(),2);
#endif
		}

	}
	//ch1 parameters

	//we can use this to fix the uncentering of the hd version for now
	shader1.setUniform2f("input1XYFix",ofVec2f(gui->input1XFix,gui->input1YFix));

	//we ADD a logic here for sd pillarbox vs sd fullscreen?
	float ch1ScaleFix=1.0;//fullscreen - inputs now scaled to internal resolution

	//default aspect ratio is 4:3
	//but if we are only sending in 640x480 resolutions anyway
	//we should double check some shit
	float ch1AspectRatio=1.0+gui->sdFixX;
	if(gui->ch1AspectRatioSwitch==0){
		ch1AspectRatio=1.0;  // Inputs are now pre-scaled to internal resolution
		ch1CribX=0;  // No crib offset needed
		ch1HdZCrib=0;  // No zoom crib needed
	}

	bool ch1HdAspectOn=0;


	if(gui->ch1AspectRatioSwitch==1){
		ch1HdAspectOn=1;
		//ch1HdAspectXFix=.5;
		//ch1HdAspectYFix=.66;
	//old
		/*ch1AspectRatio=.83;
		ch1CribX=-109;
		//ch1HdZCrib=-.205;
		ch1HdZCrib=.1;
		*/
	}
	shader1.setUniform1i("ch1HdAspectOn",ch1HdAspectOn);
	shader1.setUniform2f("ch1HdAspectXYFix",ofVec2f(ch1HdAspectXFix,ch1HdAspectYFix));

	shader1.setUniform1f("ch1ScaleFix",ch1ScaleFix);

	shader1.setUniform1f("ch1HdZCrib",ch1HdZCrib);
	shader1.setUniform1f("ch1CribX",ch1CribX);
	shader1.setUniform1f("ch1AspectRatio",ch1AspectRatio);

	shader1.setUniform2f("ch1XYDisplace",ofVec2f(ch1XDisplace,ch1YDisplace));
	//remapping z
	//but maybe not anymore
	/*
	float ch1ZDisplaceMapped=ch1ZDisplace;
	if(ch1ZDisplaceMapped>1.0){
		ch1ZDisplaceMapped=pow(2,(ch1ZDisplaceMapped-1.0f)*8.0f);
		if(ch1ZDisplace>=2.0){ch1ZDisplaceMapped=1000;}
	}
	*/

	shader1.setUniform1f("ch1ZDisplace",ch1ZDisplace);
	shader1.setUniform1f("ch1Rotate",ch1Rotate);
	shader1.setUniform3f("ch1HSBAttenuate",ofVec3f(ch1HueAttenuate,ch1SaturationAttenuate,ch1BrightAttenuate));
	if(gui->ch1Adjust[7]>0){
		ch1PosterizeSwitch=1;
	}
	shader1.setUniform1f("ch1Posterize",ch1Posterize);
	shader1.setUniform1f("ch1PosterizeInvert",ch1PosterizeInvert);
	shader1.setUniform1i("ch1PosterizeSwitch",ch1PosterizeSwitch);
	shader1.setUniform1f("ch1KaleidoscopeAmount",ch1KaleidoscopeAmount);
	shader1.setUniform1f("ch1KaleidoscopeSlice",ch1KaleidoscopeSlice);
	shader1.setUniform1f("ch1BlurAmount",ch1BlurAmount);
	shader1.setUniform1f("ch1BlurRadius",ch1BlurRadius);
	shader1.setUniform1f("ch1SharpenAmount",ch1SharpenAmount);
	shader1.setUniform1f("ch1SharpenRadius",ch1SharpenRadius);
	shader1.setUniform1f("ch1FiltersBoost",ch1FiltersBoost);

	shader1.setUniform1i("ch1GeoOverflow",gui->ch1GeoOverflow);
	shader1.setUniform1i("ch1HMirror",gui->ch1HMirror);
	shader1.setUniform1i("ch1VMirror",gui->ch1VMirror);
	shader1.setUniform1i("ch1HFlip",gui->ch1HFlip);
	shader1.setUniform1i("ch1VFlip",gui->ch1VFlip);
	shader1.setUniform1i("ch1HueInvert",gui->ch1HueInvert);
	shader1.setUniform1i("ch1SaturationInvert",gui->ch1SaturationInvert);
	shader1.setUniform1i("ch1BrightInvert",gui->ch1BrightInvert);
	shader1.setUniform1i("ch1RGBInvert",gui->ch1RGBInvert);
	shader1.setUniform1i("ch1Solarize",gui->ch1Solarize);





	//channel selection
	if(gui->ch2InputSelect==0){
		// Use input1 (webcam, NDI, or Spout depending on source type)
		if (gui->input1SourceType == 0) {
			if (input1.isInitialized()) {
				shader1.setUniformTexture("ch2Tex",webcamFbo1.getTexture(),3);
			}
		} else if (gui->input1SourceType == 1) {
			shader1.setUniformTexture("ch2Tex",ndiFbo1.getTexture(),3);
		} else {
#if OFAPP_HAS_SPOUT
			shader1.setUniformTexture("ch2Tex",spoutFbo1.getTexture(),3);
#endif
		}
	}
	if(gui->ch2InputSelect==1){
		// Use input2 (webcam, NDI, or Spout depending on source type)
		if (gui->input2SourceType == 0) {
			if (input2.isInitialized()) {
				shader1.setUniformTexture("ch2Tex",webcamFbo2.getTexture(),3);
			}
		} else if (gui->input2SourceType == 1) {
			shader1.setUniformTexture("ch2Tex",ndiFbo2.getTexture(),3);
		} else {
#if OFAPP_HAS_SPOUT
			shader1.setUniformTexture("ch2Tex",spoutFbo2.getTexture(),3);
#endif
		}
	}


	//we ADD a logic here for sd pillarbox vs sd fullscreen?
	float ch2ScaleFix=1.0;//fullscreen - inputs now scaled to internal resolution

	//default aspect ratio is 4:3
	//but if we are only sending in 640x480 resolutions anyway
	//we should double check some shit
	float ch2AspectRatio=1.0+gui->sdFixX;
	if(gui->ch2AspectRatioSwitch==0){
		ch2AspectRatio=1.0;  // Inputs are now pre-scaled to internal resolution
		ch2CribX=0;  // No crib offset needed
		ch2HdZCrib=0;  // No zoom crib needed
	}

	bool ch2HdAspectOn=0;
	if(gui->ch2AspectRatioSwitch==1){
		ch2HdAspectOn=1;
	}
	shader1.setUniform1i("ch2HdAspectOn",ch2HdAspectOn);
	shader1.setUniform2f("ch2HdAspectXYFix",ofVec2f(ch2HdAspectXFix,ch2HdAspectYFix));
	shader1.setUniform2f("input2XYFix",ofVec2f(gui->input2XFix,gui->input2YFix));

	shader1.setUniform1f("ch2ScaleFix",ch2ScaleFix);
	shader1.setUniform1f("ch2CribX",ch2CribX);
	shader1.setUniform1f("ch2AspectRatio",ch2AspectRatio);
	shader1.setUniform1f("ch2HdZCrib",ch2HdZCrib);




	shader1.setUniform1f("ratio",ratio);

	//ch2 mix parameters
	shader1.setUniform1f("ch2MixAmount",ch2MixAmount);
	shader1.setUniform3f("ch2KeyValue",ofVec3f(ch2KeyValueRed,ch2KeyValueGreen,ch2KeyValueBlue));
	shader1.setUniform1f("ch2KeyThreshold",ch2KeyThreshold);
	shader1.setUniform1f("ch2KeySoft",ch2KeySoft);
	shader1.setUniform1i("ch2KeyOrder",gui->ch2KeyOrder);
	shader1.setUniform1i("ch2MixType",gui->ch2MixType);
	shader1.setUniform1i("ch2MixOverflow",gui->ch2MixOverflow);

	//ch2 adjust
	shader1.setUniform2f("ch2XYDisplace",ofVec2f(ch2XDisplace,ch2YDisplace));
	//remapping z
	//but maybe not anymore
	/*
	float ch2ZDisplaceMapped=ch2ZDisplace;
	if(ch2ZDisplaceMapped>1.0){
		ch2ZDisplaceMapped=pow(2,(ch2ZDisplaceMapped-1.0f)*8.0f);
		if(ch2ZDisplace>=2.0){ch2ZDisplaceMapped=1000;}
	}
	*/
	shader1.setUniform1f("ch2ZDisplace",ch2ZDisplace);
	shader1.setUniform1f("ch2Rotate",ch2Rotate);
	shader1.setUniform3f("ch2HSBAttenuate",ofVec3f(ch2HueAttenuate,ch2SaturationAttenuate,ch2BrightAttenuate));
	if(gui->ch2Adjust[7]>0){
		ch2PosterizeSwitch=1;
	}
	shader1.setUniform1f("ch2Posterize",ch2Posterize);
	shader1.setUniform1f("ch2PosterizeInvert",ch2PosterizeInvert);
	shader1.setUniform1i("ch2PosterizeSwitch",ch2PosterizeSwitch);
	shader1.setUniform1f("ch2KaleidoscopeAmount",ch2KaleidoscopeAmount);
	shader1.setUniform1f("ch2KaleidoscopeSlice",ch2KaleidoscopeSlice);
	shader1.setUniform1f("ch2BlurAmount",ch2BlurAmount);
	shader1.setUniform1f("ch2BlurRadius",ch2BlurRadius);
	shader1.setUniform1f("ch2SharpenAmount",ch2SharpenAmount);
	shader1.setUniform1f("ch2SharpenRadius",ch2SharpenRadius);
	shader1.setUniform1f("ch2FiltersBoost",ch2FiltersBoost);

	shader1.setUniform1i("ch2GeoOverflow",gui->ch2GeoOverflow);
	shader1.setUniform1i("ch2HMirror",gui->ch2HMirror);
	shader1.setUniform1i("ch2VMirror",gui->ch2VMirror);
	shader1.setUniform1i("ch2HFlip",gui->ch2HFlip);
	shader1.setUniform1i("ch2VFlip",gui->ch2VFlip);
	shader1.setUniform1i("ch2HueInvert",gui->ch2HueInvert);
	shader1.setUniform1i("ch2SaturationInvert",gui->ch2SaturationInvert);
	shader1.setUniform1i("ch2BrightInvert",gui->ch2BrightInvert);
	shader1.setUniform1i("ch2RGBInvert",gui->ch2RGBInvert);
	shader1.setUniform1i("ch2Solarize",gui->ch2Solarize);


	//fb1 parameters
	//fb1mixnkey
	shader1.setUniform1f("fb1MixAmount",fb1MixAmount);
	shader1.setUniform3f("fb1KeyValue",ofVec3f(fb1KeyValueRed,fb1KeyValueGreen,fb1KeyValueBlue));
	shader1.setUniform1f("fb1KeyThreshold",fb1KeyThreshold);
	shader1.setUniform1f("fb1KeySoft",fb1KeySoft);
	shader1.setUniform1i("fb1KeyOrder",gui->fb1KeyOrder);
	shader1.setUniform1i("fb1MixType",gui->fb1MixType);
	shader1.setUniform1i("fb1MixOverflow",gui->fb1MixOverflow);
	//fb1geo1
	shader1.setUniform2f("fb1XYDisplace",ofVec2f(fb1XDisplace,fb1YDisplace));
	shader1.setUniform1f("fb1ZDisplace",fb1ZDisplace);
	shader1.setUniform1f("fb1Rotate",fb1Rotate);
	shader1.setUniform4f("fb1ShearMatrix",ofVec4f(fb1ShearMatrix1, fb1ShearMatrix2, fb1ShearMatrix3, fb1ShearMatrix4) );
	shader1.setUniform1f("fb1KaleidoscopeAmount",fb1KaleidoscopeAmount);
	shader1.setUniform1f("fb1KaleidoscopeSlice",fb1KaleidoscopeSlice);

	shader1.setUniform1i("fb1HMirror",gui->fb1HMirror);
	shader1.setUniform1i("fb1VMirror",gui->fb1VMirror);
	shader1.setUniform1i("fb1HFlip",gui->fb1HFlip);
	shader1.setUniform1i("fb1VFlip",gui->fb1VFlip);
	shader1.setUniform1i("fb1RotateMode",gui->fb1RotateMode);
	shader1.setUniform1i("fb1GeoOverflow",gui->fb1GeoOverflow);

	shader1.setUniform3f("fb1HSBOffset",ofVec3f(fb1HueOffset,fb1SaturationOffset,fb1BrightOffset));
	shader1.setUniform3f("fb1HSBAttenuate",ofVec3f(fb1HueAttenuate,fb1SaturationAttenuate,fb1BrightAttenuate));
	shader1.setUniform3f("fb1HSBPowmap",ofVec3f(fb1HuePowmap,fb1SaturationPowmap,fb1BrightPowmap));
	shader1.setUniform1f("fb1HueShaper",fb1HueShaper);

	if(gui->fb1Color1[10]>0){
		fb1PosterizeSwitch=1;
	}
	shader1.setUniform1f("fb1Posterize",fb1Posterize);
	shader1.setUniform1f("fb1PosterizeInvert",fb1PosterizeInvert);
	shader1.setUniform1i("fb1PosterizeSwitch",fb1PosterizeSwitch);

	shader1.setUniform1i("fb1HueInvert",gui->fb1HueInvert);
	shader1.setUniform1i("fb1SaturationInvert",gui->fb1SaturationInvert);
	shader1.setUniform1i("fb1BrightInvert",gui->fb1BrightInvert);


	//fb1 filters
	shader1.setUniform1f("fb1BlurAmount",fb1BlurAmount);
	shader1.setUniform1f("fb1BlurRadius",fb1BlurRadius);
	shader1.setUniform1f("fb1SharpenAmount",fb1SharpenAmount);
	shader1.setUniform1f("fb1SharpenRadius",fb1SharpenRadius);
	shader1.setUniform1f("fb1TemporalFilter1Amount",fb1TemporalFilter1Amount);
	shader1.setUniform1f("fb1TemporalFilter1Resonance",fb1TemporalFilter1Resonance);
	shader1.setUniform1f("fb1TemporalFilter2Amount",fb1TemporalFilter2Amount);
	shader1.setUniform1f("fb1TemporalFilter2Resonance",fb1TemporalFilter2Resonance);
	shader1.setUniform1f("fb1FiltersBoost",fb1FiltersBoost);


	shader1.end();


    // Switch to perspective for 3D geometry drawing
    if(gui->block1LineSwitch==1 || gui->block1SevenStarSwitch==1 ||
       gui->block1LissaBallSwitch==1 || gui->block1HypercubeSwitch==1 ||
       gui->block1LissajousCurveSwitch==1){
        ofSetupScreenPerspective(framebuffer1.getWidth(), framebuffer1.getHeight());
    }

    if(gui->block1LineSwitch==1){
    	line_draw();
    }
    if(gui->block1SevenStarSwitch==1){
    	sevenStar1Draw();
    }
    if(gui->block1LissaBallSwitch==1){
    	drawSpiralEllipse();
    }
    if(gui->block1HypercubeSwitch==1){
        hypercube_draw();
    }
    if(gui->block1LissajousCurveSwitch==1){
        lissajousCurve1Draw();
    }
	framebuffer1.end();

#if OFAPP_HAS_SPOUT
	// Spout send for Block 1
	if(gui->spoutSendBlock1){
		glFlush();
		// Draw flipped into FBO then send (scale from internal to spout send resolution)
		spoutSendFbo1.begin();
		framebuffer1.getTexture().draw(0, spoutSendHeight, spoutSendWidth, -spoutSendHeight);  // Flip vertically
		spoutSendFbo1.end();
		spoutSenderBlock1.send(spoutSendFbo1.getTexture());
	}
#else
	// Spout not supported on this platform.
#endif

	// NDI send for Block 1
	if(gui->ndiSendBlock1){
		if(!ndiSender1Active) {
			ndiSenderBlock1.CreateSender("GwBlock1", ndiSendWidth, ndiSendHeight);
			ndiSender1Active = true;
		}
		glFlush();
		ndiSendFbo1.begin();
		framebuffer1.getTexture().draw(0, 0, ndiSendWidth, ndiSendHeight);
		ndiSendFbo1.end();
		ndiSenderBlock1.SendImage(ndiSendFbo1);
	} else if(ndiSender1Active) {
		ndiSenderBlock1.ReleaseSender();
		ndiSender1Active = false;
	}












	//BLOCK_2

	framebuffer2.begin();
	// Explicitly set up viewport and projection for current FBO size
	ofViewport(0, 0, framebuffer2.getWidth(), framebuffer2.getHeight());
	ofSetupScreenOrtho(framebuffer2.getWidth(), framebuffer2.getHeight());
	shader2.begin();

	shader2.setUniform1f("width",internalWidth);
	shader2.setUniform1f("height",internalHeight);
	shader2.setUniform1f("inverseWidth",1.0f/internalWidth);
	shader2.setUniform1f("inverseHeight",1.0f/internalHeight);

	// Input resolution uniforms for block2 input processing
	shader2.setUniform1f("input1Width", (float)input1Width);
	shader2.setUniform1f("input1Height", (float)input1Height);
	shader2.setUniform1f("inverseWidth1",1.0f/input1Width);
	shader2.setUniform1f("inverseHeight1",1.0f/input1Height);

	int fb2DelayTimeMacroBuffer=int((pastFramesSize-1.0)*(gui->fb2DelayTimeMacroBuffer));
	int fb2DelayTime_d=(gui->fb2DelayTime)+fb2DelayTimeMacroBuffer;

	//draw pastframes2
	int pastFrames2Index =  (abs(pastFramesOffset - pastFramesSize - (fb2DelayTime_d) + 1) % pastFramesSize);
	pastFrames2[pastFrames2Index].draw(0, 0, internalWidth, internalHeight);
	//send the temporal filter
	int fb2TemporalFilterIndex = (abs(pastFramesOffset - pastFramesSize + 1) % pastFramesSize);
	shader2.setUniformTexture("fb2TemporalFilter", pastFrames2[fb2TemporalFilterIndex].getTexture(), 5);

	bool block2InputMasterSwitch=0;
	float block2InputWidth=internalWidth;
	float block2InputHeight=internalHeight;
	float block2InputWidthHalf=internalWidth/2;
	float block2InputHeightHalf=internalHeight/2;
	//choose block2 input
	float block2AspectRatio=1.0;
	if(gui->block2InputSelect==0){
		ratio=1.0;
		shader2.setUniformTexture("block2InputTex",framebuffer1.getTexture(),6);

	}

	if(gui->block2InputSelect==1){
		ratio=input1.getWidth()/ofGetWidth();
		block2InputMasterSwitch=1;
		// Use input1 (webcam, NDI, or Spout depending on source type)
		if (gui->input1SourceType == 0) {
			if (input1.isInitialized()) {
				shader2.setUniformTexture("block2InputTex",webcamFbo1.getTexture(),6);
			}
		} else if (gui->input1SourceType == 1) {
			shader2.setUniformTexture("block2InputTex",ndiFbo1.getTexture(),6);
		} else {
#if OFAPP_HAS_SPOUT
			shader2.setUniformTexture("block2InputTex",spoutFbo1.getTexture(),6);
#endif
		}
		// Inputs are now pre-scaled to internal resolution

	}

	if(gui->block2InputSelect==2){
		ratio=input2.getWidth()/ofGetWidth();
		block2InputMasterSwitch=1;
		// Use input2 (webcam, NDI, or Spout depending on source type)
		if (gui->input2SourceType == 0) {
			if (input2.isInitialized()) {
				shader2.setUniformTexture("block2InputTex",webcamFbo2.getTexture(),6);
			}
		} else if (gui->input2SourceType == 1) {
			shader2.setUniformTexture("block2InputTex",ndiFbo2.getTexture(),6);
		} else {
#if OFAPP_HAS_SPOUT
			shader2.setUniformTexture("block2InputTex",spoutFbo2.getTexture(),6);
#endif
		}
		// Inputs are now pre-scaled to internal resolution
	}

	shader2.setUniform1f("ratio",ratio);
	shader2.setUniform1i("block2InputMasterSwitch",block2InputMasterSwitch);
	shader2.setUniform1f("block2InputWidth",block2InputWidth);
	shader2.setUniform1f("block2InputHeight",block2InputHeight);
	shader2.setUniform1f("block2InputWidthHalf",block2InputWidthHalf);
	shader2.setUniform1f("block2InputHeightHalf",block2InputHeightHalf);
	//we ADD a logic here for sd pillarbox vs sd fullscreen?
	float block2InputScaleFix=1.0;//fullscreen - inputs now scaled to internal resolution
	float block2InputCribX=0;
	float block2InputHdZCrib=0;
	//default aspect ratio is 4:3
	//but if we are only sending in 640x480 resolutions anyway
	//we should double check some shit
	float block2InputAspectRatio=1.0+gui->sdFixX;
	if(gui->block2InputAspectRatioSwitch==0){
		block2InputAspectRatio=1.0;  // Inputs are now pre-scaled to internal resolution
		block2InputCribX=0;  // No crib offset needed
		block2InputHdZCrib=0;  // No zoom crib needed
	}




	bool block2InputHdAspectOn=0;
	if(gui->block2InputAspectRatioSwitch==1){
		block2InputHdAspectOn=1;
	}
	shader2.setUniform1i("block2InputHdAspectOn",block2InputHdAspectOn);
	shader2.setUniform2f("block2InputHdAspectXYFix",ofVec2f(ch1HdAspectXFix,ch1HdAspectYFix));

	shader2.setUniform1f("block2InputScaleFix",block2InputScaleFix);
	shader2.setUniform1f("block2InputHdZCrib",block2InputHdZCrib);
	shader2.setUniform1f("block2InputCribX",block2InputCribX);
	shader2.setUniform1f("block2InputAspectRatio",block2InputAspectRatio);

	shader2.setUniform2f("block2InputXYDisplace",ofVec2f(block2InputXDisplace,block2InputYDisplace));
	//remapping z
	//but maybe not anymore
	/*
	float block2InputZDisplaceMapped=block2InputZDisplace;
	if(block2InputZDisplaceMapped>1.0){
		block2InputZDisplaceMapped=pow(2,(block2InputZDisplaceMapped-1.0f)*8.0f);
		if(block2InputZDisplace>=2.0){block2InputZDisplaceMapped=1000;}
	}
	*/
	shader2.setUniform1f("block2InputZDisplace",block2InputZDisplace);
	shader2.setUniform1f("block2InputRotate",block2InputRotate);
	shader2.setUniform3f("block2InputHSBAttenuate",ofVec3f(block2InputHueAttenuate,block2InputSaturationAttenuate,block2InputBrightAttenuate));
	if(gui->block2InputAdjust[7]>0){
		block2InputPosterizeSwitch=1;
	}
	shader2.setUniform1f("block2InputPosterize",block2InputPosterize);
	shader2.setUniform1f("block2InputPosterizeInvert",block2InputPosterizeInvert);
	shader2.setUniform1i("block2InputPosterizeSwitch",block2InputPosterizeSwitch);
	shader2.setUniform1f("block2InputKaleidoscopeAmount",block2InputKaleidoscopeAmount);
	shader2.setUniform1f("block2InputKaleidoscopeSlice",block2InputKaleidoscopeSlice);
	shader2.setUniform1f("block2InputBlurAmount",block2InputBlurAmount);
	shader2.setUniform1f("block2InputBlurRadius",block2InputBlurRadius);
	shader2.setUniform1f("block2InputSharpenAmount",block2InputSharpenAmount);
	shader2.setUniform1f("block2InputSharpenRadius",block2InputSharpenRadius);
	shader2.setUniform1f("block2InputFiltersBoost",block2InputFiltersBoost);

	shader2.setUniform1i("block2InputGeoOverflow",gui->block2InputGeoOverflow);
	shader2.setUniform1i("block2InputHMirror",gui->block2InputHMirror);
	shader2.setUniform1i("block2InputVMirror",gui->block2InputVMirror);
	shader2.setUniform1i("block2InputHFlip",gui->block2InputHFlip);
	shader2.setUniform1i("block2InputVFlip",gui->block2InputVFlip);
	shader2.setUniform1i("block2InputHueInvert",gui->block2InputHueInvert);
	shader2.setUniform1i("block2InputSaturationInvert",gui->block2InputSaturationInvert);
	shader2.setUniform1i("block2InputBrightInvert",gui->block2InputBrightInvert);
	shader2.setUniform1i("block2InputRGBInvert",gui->block2InputRGBInvert);
	shader2.setUniform1i("block2InputSolarize",gui->block2InputSolarize);

	//fb2 parameters
	//fb2mixnkey
	shader2.setUniform1f("fb2MixAmount",fb2MixAmount);
	shader2.setUniform3f("fb2KeyValue",ofVec3f(fb2KeyValueRed,fb2KeyValueGreen,fb2KeyValueBlue));
	shader2.setUniform1f("fb2KeyThreshold",fb2KeyThreshold);
	shader2.setUniform1f("fb2KeySoft",fb2KeySoft);
	shader2.setUniform1i("fb2KeyOrder",gui->fb2KeyOrder);
	shader2.setUniform1i("fb2MixType",gui->fb2MixType);
	shader2.setUniform1i("fb2MixOverflow",gui->fb2MixOverflow);
	//fb2geo1
	shader2.setUniform2f("fb2XYDisplace",ofVec2f(fb2XDisplace,fb2YDisplace));
	shader2.setUniform1f("fb2ZDisplace",fb2ZDisplace);
	shader2.setUniform1f("fb2Rotate",fb2Rotate);
	shader2.setUniform4f("fb2ShearMatrix",ofVec4f(fb2ShearMatrix1, fb2ShearMatrix2, fb2ShearMatrix3, fb2ShearMatrix4) );
	shader2.setUniform1f("fb2KaleidoscopeAmount",fb2KaleidoscopeAmount);
	shader2.setUniform1f("fb2KaleidoscopeSlice",fb2KaleidoscopeSlice);

	shader2.setUniform1i("fb2HMirror",gui->fb2HMirror);
	shader2.setUniform1i("fb2VMirror",gui->fb2VMirror);
	shader2.setUniform1i("fb2HFlip",gui->fb2HFlip);
	shader2.setUniform1i("fb2VFlip",gui->fb2VFlip);
	shader2.setUniform1i("fb2RotateMode",gui->fb2RotateMode);
	shader2.setUniform1i("fb2GeoOverflow",gui->fb2GeoOverflow);

	shader2.setUniform3f("fb2HSBOffset",ofVec3f(fb2HueOffset,fb2SaturationOffset,fb2BrightOffset));
	shader2.setUniform3f("fb2HSBAttenuate",ofVec3f(fb2HueAttenuate,fb2SaturationAttenuate,fb2BrightAttenuate));
	shader2.setUniform3f("fb2HSBPowmap",ofVec3f(fb2HuePowmap,fb2SaturationPowmap,fb2BrightPowmap));
	shader2.setUniform1f("fb2HueShaper",fb2HueShaper);

	if(gui->fb2Color1[10]>0){
		fb2PosterizeSwitch=1;
	}
	shader2.setUniform1f("fb2Posterize",fb2Posterize);
	shader2.setUniform1f("fb2PosterizeInvert",fb2PosterizeInvert);
	shader2.setUniform1i("fb2PosterizeSwitch",fb2PosterizeSwitch);

	shader2.setUniform1i("fb2HueInvert",gui->fb2HueInvert);
	shader2.setUniform1i("fb2SaturationInvert",gui->fb2SaturationInvert);
	shader2.setUniform1i("fb2BrightInvert",gui->fb2BrightInvert);


	//fb2 filters
	shader2.setUniform1f("fb2BlurAmount",fb2BlurAmount);
	shader2.setUniform1f("fb2BlurRadius",fb2BlurRadius);
	shader2.setUniform1f("fb2SharpenAmount",fb2SharpenAmount);
	shader2.setUniform1f("fb2SharpenRadius",fb2SharpenRadius);
	shader2.setUniform1f("fb2TemporalFilter1Amount",fb2TemporalFilter1Amount);
	shader2.setUniform1f("fb2TemporalFilter1Resonance",fb2TemporalFilter1Resonance);
	shader2.setUniform1f("fb2TemporalFilter2Amount",fb2TemporalFilter2Amount);
	shader2.setUniform1f("fb2TemporalFilter2Resonance",fb2TemporalFilter2Resonance);
	shader2.setUniform1f("fb2FiltersBoost",fb2FiltersBoost);


	shader2.end();


    // Switch to perspective for 3D geometry drawing
    if(gui->block2LineSwitch==1 || gui->block2SevenStarSwitch==1 ||
       gui->block2LissaBallSwitch==1 || gui->block2HypercubeSwitch==1 ||
       gui->block2LissajousCurveSwitch==1){
        ofSetupScreenPerspective(framebuffer2.getWidth(), framebuffer2.getHeight());
    }

    if(gui->block2LineSwitch==1){
    	line_draw();
    }
    if(gui->block2SevenStarSwitch==1){
    	sevenStar1Draw();
    }
	if(gui->block2LissaBallSwitch==1){
    	drawSpiralEllipse();
    }
	if(gui->block2HypercubeSwitch==1){
        hypercube_draw();
    }
    if(gui->block2LissajousCurveSwitch==1){
        lissajousCurve2Draw();
    }
	framebuffer2.end();

#if OFAPP_HAS_SPOUT
	// Spout send for Block 2
	if(gui->spoutSendBlock2){
		glFlush();
		// Draw flipped into FBO then send (scale from internal to spout send resolution)
		spoutSendFbo2.begin();
		framebuffer2.getTexture().draw(0, spoutSendHeight, spoutSendWidth, -spoutSendHeight);  // Flip vertically
		spoutSendFbo2.end();
		spoutSenderBlock2.send(spoutSendFbo2.getTexture());
	}
#else
	// Spout not supported on this platform.
#endif

	// NDI send for Block 2
	if(gui->ndiSendBlock2){
		if(!ndiSender2Active) {
			ndiSenderBlock2.CreateSender("GwBlock2", ndiSendWidth, ndiSendHeight);
			ndiSender2Active = true;
		}
		glFlush();
		ndiSendFbo2.begin();
		framebuffer2.getTexture().draw(0, 0, ndiSendWidth, ndiSendHeight);
		ndiSendFbo2.end();
		ndiSenderBlock2.SendImage(ndiSendFbo2);
	} else if(ndiSender2Active) {
		ndiSenderBlock2.ReleaseSender();
		ndiSender2Active = false;
	}
















	//FINAL MIX OUT
	framebuffer3.begin();
	// Explicitly set up viewport and projection for current FBO size
	ofViewport(0, 0, framebuffer3.getWidth(), framebuffer3.getHeight());
	ofSetupScreenOrtho(framebuffer3.getWidth(), framebuffer3.getHeight());
	shader3.begin();
	dummyTex.draw(0, 0, framebuffer3.getWidth(), framebuffer3.getHeight());

	shader3.setUniformTexture("block2Output",framebuffer2.getTexture(),8);
	shader3.setUniformTexture("block1Output",framebuffer1.getTexture(),9);

	shader3.setUniform1f("width",internalWidth);
	shader3.setUniform1f("height",internalHeight);
	shader3.setUniform1f("inverseWidth",1.0f/internalWidth);
	shader3.setUniform1f("inverseHeight",1.0f/internalHeight);

	//block1geo1
	shader3.setUniform2f("block1XYDisplace",ofVec2f(block1XDisplace,block1YDisplace));
	//remapping z
	float block1ZDisplaceMapped=block1ZDisplace;
	if(block1ZDisplaceMapped>1.0){
		block1ZDisplaceMapped=pow(2,(block1ZDisplaceMapped-1.0f)*8.0f);
		if(block1ZDisplace>=2.0){block1ZDisplaceMapped=1000;}
	}
	shader3.setUniform1f("block1ZDisplace",block1ZDisplaceMapped);
	shader3.setUniform1f("block1Rotate",block1Rotate);
	shader3.setUniform4f("block1ShearMatrix",ofVec4f(block1ShearMatrix1,
		 block1ShearMatrix2, block1ShearMatrix3, block1ShearMatrix4) );
	shader3.setUniform1f("block1KaleidoscopeAmount",block1KaleidoscopeAmount);
	shader3.setUniform1f("block1KaleidoscopeSlice",block1KaleidoscopeSlice);

	shader3.setUniform1i("block1HMirror",gui->block1HMirror);
	shader3.setUniform1i("block1VMirror",gui->block1VMirror);
	shader3.setUniform1i("block1HFlip",gui->block1HFlip);
	shader3.setUniform1i("block1VFlip",gui->block1VFlip);
	shader3.setUniform1i("block1RotateMode",gui->block1RotateMode);
	shader3.setUniform1i("block1GeoOverflow",gui->block1GeoOverflow);

	//block1 colorize
	shader3.setUniform1i("block1ColorizeSwitch",gui->block1ColorizeSwitch);
	shader3.setUniform1i("block1ColorizeHSB_RGB",gui->block1ColorizeHSB_RGB);

	shader3.setUniform3f("block1ColorizeBand1",ofVec3f(block1ColorizeHueBand1,
				block1ColorizeSaturationBand1,block1ColorizeBrightBand1));
	shader3.setUniform3f("block1ColorizeBand2",ofVec3f(block1ColorizeHueBand2,
				block1ColorizeSaturationBand2,block1ColorizeBrightBand2));
	shader3.setUniform3f("block1ColorizeBand3",ofVec3f(block1ColorizeHueBand3,
				block1ColorizeSaturationBand3,block1ColorizeBrightBand3));
	shader3.setUniform3f("block1ColorizeBand4",ofVec3f(block1ColorizeHueBand4,
				block1ColorizeSaturationBand4,block1ColorizeBrightBand4));
	shader3.setUniform3f("block1ColorizeBand5",ofVec3f(block1ColorizeHueBand5,
				block1ColorizeSaturationBand5,block1ColorizeBrightBand5));

	//block1 filters
	shader3.setUniform1f("block1BlurAmount",block1BlurAmount);
	shader3.setUniform1f("block1BlurRadius",block1BlurRadius);
	shader3.setUniform1f("block1SharpenAmount",block1SharpenAmount);
	shader3.setUniform1f("block1SharpenRadius",block1SharpenRadius);
	shader3.setUniform1f("block1FiltersBoost",block1FiltersBoost);
	shader3.setUniform1f("block1Dither",block1Dither);
	bool block1DitherSwitch=0;
	if(gui->block1Filters[5] >0){block1DitherSwitch=1;}
	shader3.setUniform1i("block1DitherSwitch",block1DitherSwitch);
	shader3.setUniform1i("block1DitherType",gui->block1DitherType);


	//block2geo1
	shader3.setUniform2f("block2XYDisplace",ofVec2f(block2XDisplace,block2YDisplace));
	//remapping z
	float block2ZDisplaceMapped=block2ZDisplace;
	if(block2ZDisplaceMapped>1.0){
		block2ZDisplaceMapped=pow(2,(block2ZDisplaceMapped-1.0f)*8.0f);
		if(block2ZDisplace>=2.0){block2ZDisplaceMapped=1000;}
	}
	shader3.setUniform1f("block2ZDisplace",block2ZDisplaceMapped);
	shader3.setUniform1f("block2Rotate",block2Rotate);
	shader3.setUniform4f("block2ShearMatrix",ofVec4f(block2ShearMatrix1,
		 block2ShearMatrix2, block2ShearMatrix3, block2ShearMatrix4) );
	shader3.setUniform1f("block2KaleidoscopeAmount",block2KaleidoscopeAmount);
	shader3.setUniform1f("block2KaleidoscopeSlice",block2KaleidoscopeSlice);

	shader3.setUniform1i("block2HMirror",gui->block2HMirror);
	shader3.setUniform1i("block2VMirror",gui->block2VMirror);
	shader3.setUniform1i("block2HFlip",gui->block2HFlip);
	shader3.setUniform1i("block2VFlip",gui->block2VFlip);
	shader3.setUniform1i("block2RotateMode",gui->block2RotateMode);
	shader3.setUniform1i("block2GeoOverflow",gui->block2GeoOverflow);

	//block2 colorize
	shader3.setUniform1i("block2ColorizeSwitch",gui->block2ColorizeSwitch);
	shader3.setUniform1i("block2ColorizeHSB_RGB",gui->block2ColorizeHSB_RGB);

	shader3.setUniform3f("block2ColorizeBand1",ofVec3f(block2ColorizeHueBand1,
				block2ColorizeSaturationBand1,block2ColorizeBrightBand1));
	shader3.setUniform3f("block2ColorizeBand2",ofVec3f(block2ColorizeHueBand2,
				block2ColorizeSaturationBand2,block2ColorizeBrightBand2));
	shader3.setUniform3f("block2ColorizeBand3",ofVec3f(block2ColorizeHueBand3,
				block2ColorizeSaturationBand3,block2ColorizeBrightBand3));
	shader3.setUniform3f("block2ColorizeBand4",ofVec3f(block2ColorizeHueBand4,
				block2ColorizeSaturationBand4,block2ColorizeBrightBand4));
	shader3.setUniform3f("block2ColorizeBand5",ofVec3f(block2ColorizeHueBand5,
				block2ColorizeSaturationBand5,block2ColorizeBrightBand5));

	//block2 filters
	shader3.setUniform1f("block2BlurAmount",block2BlurAmount);
	shader3.setUniform1f("block2BlurRadius",block2BlurRadius);
	shader3.setUniform1f("block2SharpenAmount",block2SharpenAmount);
	shader3.setUniform1f("block2SharpenRadius",block2SharpenRadius);
	shader3.setUniform1f("block2FiltersBoost",block2FiltersBoost);
	shader3.setUniform1f("block2Dither",block2Dither);
	bool block2DitherSwitch=0;
	if(gui->block2Filters[5] >0){block2DitherSwitch=1;}
	shader3.setUniform1i("block2DitherSwitch",block2DitherSwitch);
	shader3.setUniform1i("block2DitherType",gui->block2DitherType);



	//final mix parameters
	shader3.setUniform1f("finalMixAmount",finalMixAmount);
	shader3.setUniform3f("finalKeyValue",ofVec3f(finalKeyValueRed,finalKeyValueGreen,finalKeyValueBlue));
	shader3.setUniform1f("finalKeyThreshold",finalKeyThreshold);
	shader3.setUniform1f("finalKeySoft",finalKeySoft);
	shader3.setUniform1i("finalKeyOrder",gui->finalKeyOrder);
	shader3.setUniform1i("finalMixType",gui->finalMixType);
	shader3.setUniform1i("finalMixOverflow",gui->finalMixOverflow);

	//matrixMixer
	shader3.setUniform1i("matrixMixType",gui->matrixMixType);
	shader3.setUniform1i("matrixMixOverflow",gui->matrixMixOverflow);
	shader3.setUniform3f("bgRGBIntoFgRed",ofVec3f(matrixMixBgRedIntoFgRed,
											  matrixMixBgGreenIntoFgRed,
											  matrixMixBgBlueIntoFgRed) );
	shader3.setUniform3f("bgRGBIntoFgGreen",ofVec3f(matrixMixBgRedIntoFgGreen,
											  matrixMixBgGreenIntoFgGreen,
											  matrixMixBgBlueIntoFgGreen) );
	shader3.setUniform3f("bgRGBIntoFgBlue",ofVec3f(matrixMixBgRedIntoFgBlue,
											  matrixMixBgGreenIntoFgBlue,
											  matrixMixBgBlueIntoFgBlue) );


	/*
				//block2 colorize
	shader3.setUniform1i("block2ColorizeSwitch",gui->block2ColorizeSwitch);
	shader3.setUniform1i("block2ColorizeHSB_RGB",gui->block2ColorizeHSB_RGB);
	shader3.setUniform3f("block2ColorizeBand1",ofVec3f(block2ColorizeHueBand1,
				block2ColorizeSaturationBand1,block2ColorizeBrightBand1));
	shader3.setUniform3f("block2ColorizeBand2",ofVec3f(block2ColorizeHueBand2,
				block2ColorizeSaturationBand2,block2ColorizeBrightBand2));
	shader3.setUniform3f("block2ColorizeBand3",ofVec3f(block2ColorizeHueBand3,
				block2ColorizeSaturationBand3,block2ColorizeBrightBand3));
	shader3.setUniform3f("block2ColorizeBand4",ofVec3f(block2ColorizeHueBand4,
				block2ColorizeSaturationBand4,block2ColorizeBrightBand4));
	shader3.setUniform3f("block2ColorizeBand5",ofVec3f(block2ColorizeHueBand5,
				block2ColorizeSaturationBand5,block2ColorizeBrightBand5));

	*/

	shader3.end();
	framebuffer3.end();

#if OFAPP_HAS_SPOUT
	// Spout send for Block 3 (final output)
	if(gui->spoutSendBlock3){
		glFlush();
		// Draw flipped into FBO then send (scale from internal to spout send resolution)
		spoutSendFbo3.begin();
		framebuffer3.getTexture().draw(0, spoutSendHeight, spoutSendWidth, -spoutSendHeight);  // Flip vertically
		spoutSendFbo3.end();
		spoutSenderBlock3.send(spoutSendFbo3.getTexture());
	}
#else
	// Spout not supported on this platform.
#endif

	// NDI send for Block 3 (final output)
	if(gui->ndiSendBlock3){
		if(!ndiSender3Active) {
			ndiSenderBlock3.CreateSender("GwBlock3", ndiSendWidth, ndiSendHeight);
			ndiSender3Active = true;
		}
		glFlush();
		ndiSendFbo3.begin();
		framebuffer3.getTexture().draw(0, 0, ndiSendWidth, ndiSendHeight);
		ndiSendFbo3.end();
		ndiSenderBlock3.SendImage(ndiSendFbo3);
	} else if(ndiSender3Active) {
		ndiSenderBlock3.ReleaseSender();
		ndiSender3Active = false;
	}

	//draw to screen - reset viewport and projection to window size
	ofSetupScreen();

	if(gui->drawMode==0){
		framebuffer1.draw(0, 0, ofGetWidth(), ofGetHeight());
	}
	if(gui->drawMode==1){
		framebuffer2.draw(0, 0, ofGetWidth(), ofGetHeight());
	}
	if(gui->drawMode==2){
		framebuffer3.draw(0, 0, ofGetWidth(), ofGetHeight());
	}
	if(gui->drawMode==3){
		framebuffer1.draw(0, 0, ofGetWidth() / 2, ofGetHeight() / 2);
		framebuffer2.draw(ofGetWidth() / 2, 0, ofGetWidth() / 2, ofGetHeight() / 2);
		framebuffer3.draw(0,ofGetHeight()/2,ofGetWidth()/2, ofGetHeight()/2);
	}


	pastFrames1[abs(pastFramesSize - pastFramesOffset)-1].begin();
	framebuffer1.draw(0, 0, internalWidth, internalHeight);
	pastFrames1[abs(pastFramesSize - pastFramesOffset)-1].end();

	pastFrames2[abs(pastFramesSize-pastFramesOffset)-1].begin();
    framebuffer2.draw(0, 0, internalWidth, internalHeight);
    //ofSetColor(255);
	//ofDrawRectangle(ofGetWidth()/2,ofGetHeight()/2,ofGetWidth()/4,ofGetHeight()/4);
    pastFrames2[abs(pastFramesSize-pastFramesOffset)-1].end();

	pastFramesOffset++;
    pastFramesOffset=pastFramesOffset % pastFramesSize;

	//inputTest();

	//clear the framebuffers
	framebuffer1.begin();
	ofClear(0,0,0,255);
	framebuffer1.end();

	framebuffer2.begin();
	ofClear(0,0,0,255);
	framebuffer2.end();

	framebuffer3.begin();
	ofClear(0,0,0,255);
	framebuffer3.end();


	if(gui->fb1FramebufferClearSwitch==1){
		for(int i=0;i<pastFramesSize;i++){
        	pastFrames1[i].begin();
       		ofClear(0,0,0,255);
        	pastFrames1[i].end();
        }
	}

	if(gui->fb2FramebufferClearSwitch==1){
		for(int i=0;i<pastFramesSize;i++){
        	pastFrames2[i].begin();
       		ofClear(0,0,0,255);
        	pastFrames2[i].end();
        }
	}
}


//--------------------------------------------------------------
void ofApp::inputSetup(){
	// List webcam devices
	input1.listDevices();

	// Allocate webcam FBOs at INTERNAL resolution - GPU-only (no CPU access needed)
	allocateGpuOnlyFbo(webcamFbo1, internalWidth, internalHeight);
	allocateGpuOnlyFbo(webcamFbo2, internalWidth, internalHeight);

	// Allocate NDI input FBOs at INTERNAL resolution - GPU-only
	allocateGpuOnlyFbo(ndiFbo1, internalWidth, internalHeight);
	allocateGpuOnlyFbo(ndiFbo2, internalWidth, internalHeight);

	// Always allocate NDI textures so they're ready when needed
	// (ofxNDI receives at native resolution, we scale into FBO)
	ndiTexture1.allocate(1920, 1080, GL_RGBA);
	ndiTexture2.allocate(1920, 1080, GL_RGBA);

	// Clear them to black
	ofPixels blackPixels;
	blackPixels.allocate(1920, 1080, OF_PIXELS_RGBA);
	blackPixels.setColor(ofColor::black);
	ndiTexture1.loadData(blackPixels);
	ndiTexture2.loadData(blackPixels);

#if OFAPP_HAS_SPOUT
	// Allocate Spout input FBOs at INTERNAL resolution - GPU-only
	allocateGpuOnlyFbo(spoutFbo1, internalWidth, internalHeight);
	allocateGpuOnlyFbo(spoutFbo2, internalWidth, internalHeight);

	// Initialize Spout receivers
	spoutReceiver1.init();
	spoutReceiver2.init();

	// Allocate Spout sender FBOs at spout send resolution - GPU-only (Spout uses texture sharing)
	allocateGpuOnlyFbo(spoutSendFbo1, spoutSendWidth, spoutSendHeight);
	allocateGpuOnlyFbo(spoutSendFbo2, spoutSendWidth, spoutSendHeight);
	allocateGpuOnlyFbo(spoutSendFbo3, spoutSendWidth, spoutSendHeight);

	// Initialize Spout senders at spout send resolution
	spoutSenderBlock1.init("GwBlock1", spoutSendWidth, spoutSendHeight, GL_RGBA);
	spoutSenderBlock2.init("GwBlock2", spoutSendWidth, spoutSendHeight, GL_RGBA);
	spoutSenderBlock3.init("GwBlock3", spoutSendWidth, spoutSendHeight, GL_RGBA);
#endif

	// Allocate NDI sender FBOs at ndi send resolution
	// NOTE: These NEED CPU backing because NDI reads pixels back to CPU for network transmission
	ndiSendWidth = gui->ndiSendWidth;
	ndiSendHeight = gui->ndiSendHeight;
	ndiSendFbo1.allocate(ndiSendWidth, ndiSendHeight, GL_RGBA);
	ndiSendFbo2.allocate(ndiSendWidth, ndiSendHeight, GL_RGBA);
	ndiSendFbo3.allocate(ndiSendWidth, ndiSendHeight, GL_RGBA);

	// NDI senders are created on-demand when enabled in GUI

	// Initialize Input 1 based on source type
	if (gui->input1SourceType == 0) {
		// Webcam
		input1.setVerbose(true);
		input1.setDeviceID(gui->input1DeviceID);
		input1.setDesiredFrameRate(30);
		input1.setup(input1Width, input1Height);
	}

	// Initialize Input 2 based on source type
	if (gui->input2SourceType == 0) {
		// Webcam
		input2.setVerbose(true);
		input2.setDeviceID(gui->input2DeviceID);
		input2.setDesiredFrameRate(30);
		input2.setup(input2Width, input2Height);
	}

	// Initial NDI source scan
	refreshNdiSources();
}

//--------------------------------------------------------------
void ofApp::inputUpdate(){
	// Update Input 1 based on source type
	if (gui->input1SourceType == 0) {
		// Webcam - update and scale into FBO at internal resolution
		if (input1.isInitialized()) {
			input1.update();
			if (input1.isFrameNew()) {
				webcamFbo1.begin();
				ofViewport(0, 0, webcamFbo1.getWidth(), webcamFbo1.getHeight());
				ofSetupScreenOrtho(webcamFbo1.getWidth(), webcamFbo1.getHeight());
				ofClear(0, 0, 0, 255);
				// Scale webcam to fill FBO
				input1.draw(0, 0, webcamFbo1.getWidth(), webcamFbo1.getHeight());
				webcamFbo1.end();
			}
		}
	} else if (gui->input1SourceType == 1) {
		// NDI - receive then scale into FBO
		ndiReceiver1.ReceiveImage(ndiTexture1);
		ndiFbo1.begin();
		ofViewport(0, 0, ndiFbo1.getWidth(), ndiFbo1.getHeight());
		ofSetupScreenOrtho(ndiFbo1.getWidth(), ndiFbo1.getHeight());
		ofClear(0, 0, 0, 255);

		// Simple fill - stretch to fit FBO dimensions
		float srcW = ndiTexture1.getWidth();
		float srcH = ndiTexture1.getHeight();
		if (srcW > 0 && srcH > 0) {
			ndiTexture1.draw(0, 0, ndiFbo1.getWidth(), ndiFbo1.getHeight());
		}
		ndiFbo1.end();
	} else if (gui->input1SourceType == 2) {
#if OFAPP_HAS_SPOUT
		// Spout - receive from active sender and scale into FBO
		if (spoutReceiver1.isInitialized()) {
			spoutReceiver1.receive(spoutTexture1);
		}

		spoutFbo1.begin();
		ofViewport(0, 0, spoutFbo1.getWidth(), spoutFbo1.getHeight());
		ofSetupScreenOrtho(spoutFbo1.getWidth(), spoutFbo1.getHeight());
		ofClear(0, 0, 0, 255);

		// Simple fill - stretch to fit FBO dimensions
		float srcW = spoutTexture1.getWidth();
		float srcH = spoutTexture1.getHeight();
		if (srcW > 0 && srcH > 0) {
			spoutTexture1.draw(0, 0, spoutFbo1.getWidth(), spoutFbo1.getHeight());
		}
		spoutFbo1.end();
#endif
	}

	// Update Input 2 based on source type
	if (gui->input2SourceType == 0) {
		// Webcam - update and scale into FBO at internal resolution
		if (input2.isInitialized()) {
			input2.update();
			if (input2.isFrameNew()) {
				webcamFbo2.begin();
				ofViewport(0, 0, webcamFbo2.getWidth(), webcamFbo2.getHeight());
				ofSetupScreenOrtho(webcamFbo2.getWidth(), webcamFbo2.getHeight());
				ofClear(0, 0, 0, 255);
				// Scale webcam to fill FBO
				input2.draw(0, 0, webcamFbo2.getWidth(), webcamFbo2.getHeight());
				webcamFbo2.end();
			}
		}
	} else if (gui->input2SourceType == 1) {
		// NDI - receive then scale into FBO
		ndiReceiver2.ReceiveImage(ndiTexture2);
		ndiFbo2.begin();
		ofViewport(0, 0, ndiFbo2.getWidth(), ndiFbo2.getHeight());
		ofSetupScreenOrtho(ndiFbo2.getWidth(), ndiFbo2.getHeight());
		ofClear(0, 0, 0, 255);

		// Simple fill - stretch to fit FBO dimensions
		float srcW = ndiTexture2.getWidth();
		float srcH = ndiTexture2.getHeight();
		if (srcW > 0 && srcH > 0) {
			ndiTexture2.draw(0, 0, ndiFbo2.getWidth(), ndiFbo2.getHeight());
		}
		ndiFbo2.end();
	} else if (gui->input2SourceType == 2) {
#if OFAPP_HAS_SPOUT
		// Spout - receive from active sender and scale into FBO
		if (spoutReceiver2.isInitialized()) {
			spoutReceiver2.receive(spoutTexture2);
		}

		spoutFbo2.begin();
		ofViewport(0, 0, spoutFbo2.getWidth(), spoutFbo2.getHeight());
		ofSetupScreenOrtho(spoutFbo2.getWidth(), spoutFbo2.getHeight());
		ofClear(0, 0, 0, 255);

		// Simple fill - stretch to fit FBO dimensions
		float srcW = spoutTexture2.getWidth();
		float srcH = spoutTexture2.getHeight();
		if (srcW > 0 && srcH > 0) {
			spoutTexture2.draw(0, 0, spoutFbo2.getWidth(), spoutFbo2.getHeight());
		}
		spoutFbo2.end();
#endif
	}
}


//--------------------------------------------------------------
void ofApp::inputTest(){

	if(testSwitch1==1){
		if (gui->input1SourceType == 0) {
			input1.draw(0, 0);
		} else if (gui->input1SourceType == 1) {
			ndiFbo1.draw(0, 0);
		} else {
#if OFAPP_HAS_SPOUT
			spoutFbo1.draw(0, 0);
#endif
		}
	}
	if(testSwitch1==2){
		if (gui->input2SourceType == 0) {
			input2.draw(0, 0);
		} else if (gui->input2SourceType == 1) {
			ndiFbo2.draw(0, 0);
		} else {
#if OFAPP_HAS_SPOUT
			spoutFbo2.draw(0, 0);
#endif
		}
	}

}

//--------------------------------------------------------------
void ofApp::reinitializeInputs(){
	ofLogNotice("Video Input") << "Reinitializing video inputs...";

	// Handle Input 1
	if (gui->input1SourceType == 0) {
		// Webcam
		ofLogNotice("Video Input") << "Input 1: Webcam Device " << gui->input1DeviceID;
		ndiReceiver1.ReleaseReceiver();
#if OFAPP_HAS_SPOUT
		spoutReceiver1.release();
#endif
		input1.close();
		input1.setVerbose(true);
		input1.setDeviceID(gui->input1DeviceID);
		input1.setDesiredFrameRate(30);
		input1.setup(input1Width, input1Height);
	} else if (gui->input1SourceType == 1) {
		// NDI
		input1.close();
#if OFAPP_HAS_SPOUT
		spoutReceiver1.release();
#endif
		if (gui->input1NdiSourceIndex < gui->ndiSourceNames.size()) {
			string sourceName = gui->ndiSourceNames[gui->input1NdiSourceIndex];
			ofLogNotice("Video Input") << "Input 1: NDI Source " << sourceName;
			ndiReceiver1.SetSenderName(sourceName);
			ndiReceiver1.CreateReceiver();
			if (!ndiTexture1.isAllocated()) {
				ndiTexture1.allocate(640, 480, GL_RGBA);
			}
		}
	} else if (gui->input1SourceType == 2) {
#if OFAPP_HAS_SPOUT
		// Spout
		input1.close();
		ndiReceiver1.ReleaseReceiver();
		spoutReceiver1.release();
		if (gui->input1SpoutSourceIndex < gui->spoutSourceNames.size()) {
			string sourceName = gui->spoutSourceNames[gui->input1SpoutSourceIndex];
			ofLogNotice("Video Input") << "Input 1: Spout Source " << sourceName;
			spoutReceiver1.init(sourceName);
		} else {
			// No specific sender selected, connect to active sender
			ofLogNotice("Video Input") << "Input 1: Spout (active sender)";
			spoutReceiver1.init();
		}
#endif
	}

	// Handle Input 2
	if (gui->input2SourceType == 0) {
		// Webcam
		ofLogNotice("Video Input") << "Input 2: Webcam Device " << gui->input2DeviceID;
		ndiReceiver2.ReleaseReceiver();
#if OFAPP_HAS_SPOUT
		spoutReceiver2.release();
#endif
		input2.close();
		input2.setVerbose(true);
		input2.setDeviceID(gui->input2DeviceID);
		input2.setDesiredFrameRate(30);
		input2.setup(input2Width, input2Height);
	} else if (gui->input2SourceType == 1) {
		// NDI
		input2.close();
#if OFAPP_HAS_SPOUT
		spoutReceiver2.release();
#endif
		if (gui->input2NdiSourceIndex < gui->ndiSourceNames.size()) {
			string sourceName = gui->ndiSourceNames[gui->input2NdiSourceIndex];
			ofLogNotice("Video Input") << "Input 2: NDI Source " << sourceName;
			ndiReceiver2.SetSenderName(sourceName);
			ndiReceiver2.CreateReceiver();
			if (!ndiTexture2.isAllocated()) {
				ndiTexture2.allocate(640, 480, GL_RGBA);
			}
		}
	} else if (gui->input2SourceType == 2) {
#if OFAPP_HAS_SPOUT
		// Spout
		input2.close();
		ndiReceiver2.ReleaseReceiver();
		spoutReceiver2.release();
		if (gui->input2SpoutSourceIndex < gui->spoutSourceNames.size()) {
			string sourceName = gui->spoutSourceNames[gui->input2SpoutSourceIndex];
			ofLogNotice("Video Input") << "Input 2: Spout Source " << sourceName;
			spoutReceiver2.init(sourceName);
		} else {
			// No specific sender selected, connect to active sender
			ofLogNotice("Video Input") << "Input 2: Spout (active sender)";
			spoutReceiver2.init();
		}
#endif
	}

	ofLogNotice("Video Input") << "Reinitialization complete";
}

//--------------------------------------------------------------
void ofApp::refreshNdiSources(){
	ofLogNotice("NDI") << "Scanning for NDI sources (this may take a moment)...";

	gui->ndiSourceNames.clear();

	// Call FindSenders to discover sources - returns the count found
	int numFound = ndiReceiver1.FindSenders();
	ofLogNotice("NDI") << "FindSenders found: " << numFound;

	// Use the return value from FindSenders as our count
	// (GetSenderCount sometimes returns different values)
	for (int i = 0; i < numFound; i++) {
		std::string name = ndiReceiver1.GetSenderName(i);
		ofLogNotice("NDI") << "  Sender " << i << ": " << name;
		// Skip empty names and our own senders (GwBlock1, GwBlock2, GwBlock3)
		if (!name.empty() && name.rfind("Gw", 0) != 0) {
			gui->ndiSourceNames.push_back(name);
		}
	}

	ofLogNotice("NDI") << "Total NDI sources added: " << gui->ndiSourceNames.size();
}

//---------------------------------------------------------
#if OFAPP_HAS_SPOUT
void ofApp::refreshSpoutSources(){
	ofLogNotice("Spout") << "Scanning for Spout senders...";

	gui->spoutSourceNames.clear();

	// Use a temporary SpoutReceiver to enumerate senders
	SpoutReceiver tempReceiver;
	int numSenders = tempReceiver.GetSenderCount();
	ofLogNotice("Spout") << "Found " << numSenders << " Spout senders";

	char senderName[256];
	for (int i = 0; i < numSenders; i++) {
		if (tempReceiver.GetSender(i, senderName, 256)) {
			ofLogNotice("Spout") << "  Sender " << i << ": " << senderName;
			// Skip our own senders (GwBlock1, GwBlock2, GwBlock3)
			std::string nameStr(senderName);
			if (nameStr.rfind("Gw", 0) != 0) {
				gui->spoutSourceNames.push_back(nameStr);
			}
		}
	}

	ofLogNotice("Spout") << "Total Spout senders added: " << gui->spoutSourceNames.size();
}
#else
void ofApp::refreshSpoutSources(){
	gui->spoutSourceNames.clear();
	ofLogNotice("Spout") << "Spout is not supported on this platform.";
}
#endif

//---------------------------------------------------------
void ofApp::framebufferSetup(){
	// Use internal resolution for all processing buffers
	// These are GPU-only (no CPU backing needed)
	allocateGpuOnlyFbo(framebuffer1, internalWidth, internalHeight);
	allocateGpuOnlyFbo(framebuffer2, internalWidth, internalHeight);
	allocateGpuOnlyFbo(framebuffer3, outputWidth, outputHeight);

	// pastFrames also use internal resolution - GPU-only for major RAM savings
	for(int i=0;i<pastFramesSize;i++){
        allocateGpuOnlyFbo(pastFrames1[i], internalWidth, internalHeight);
        allocateGpuOnlyFbo(pastFrames2[i], internalWidth, internalHeight);
    }

}

//--------------------------------------------------------------
void ofApp::reinitializeResolutions(){
	ofLogNotice("Resolution") << "Reinitializing resolutions:";
	ofLogNotice("Resolution") << "  Input 1: " << input1Width << "x" << input1Height;
	ofLogNotice("Resolution") << "  Input 2: " << input2Width << "x" << input2Height;
	ofLogNotice("Resolution") << "  Internal: " << internalWidth << "x" << internalHeight;
	ofLogNotice("Resolution") << "  Output: " << outputWidth << "x" << outputHeight;
#if OFAPP_HAS_SPOUT
	ofLogNotice("Resolution") << "  Spout Send: " << spoutSendWidth << "x" << spoutSendHeight;
#endif

	// Reinitialize webcams at new resolution if they're the active source
	if (gui->input1SourceType == 0) {
		input1.close();
		input1.setDeviceID(gui->input1DeviceID);
		input1.setDesiredFrameRate(30);
		input1.setup(input1Width, input1Height);
		ofLogNotice("Resolution") << "  Webcam 1 reinitialized at " << input1Width << "x" << input1Height;
	}
	if (gui->input2SourceType == 0) {
		input2.close();
		input2.setDeviceID(gui->input2DeviceID);
		input2.setDesiredFrameRate(30);
		input2.setup(input2Width, input2Height);
		ofLogNotice("Resolution") << "  Webcam 2 reinitialized at " << input2Width << "x" << input2Height;
	}

	// Reallocate webcam FBOs at internal resolution - GPU-only
	allocateGpuOnlyFbo(webcamFbo1, internalWidth, internalHeight);
	allocateGpuOnlyFbo(webcamFbo2, internalWidth, internalHeight);

	// Reallocate main framebuffers - framebuffer3 at output resolution - GPU-only
	allocateGpuOnlyFbo(framebuffer1, internalWidth, internalHeight);
	allocateGpuOnlyFbo(framebuffer2, internalWidth, internalHeight);
	allocateGpuOnlyFbo(framebuffer3, outputWidth, outputHeight);

	// Reallocate dummyTex at internal resolution
	dummyTex.allocate(internalWidth, internalHeight, GL_RGBA);

	// Reallocate pastFrames at internal resolution - GPU-only for major RAM savings
	for(int i=0; i<pastFramesSize; i++){
		allocateGpuOnlyFbo(pastFrames1[i], internalWidth, internalHeight);
		allocateGpuOnlyFbo(pastFrames2[i], internalWidth, internalHeight);
	}

	// Reallocate NDI input FBOs at INTERNAL resolution - GPU-only
	allocateGpuOnlyFbo(ndiFbo1, internalWidth, internalHeight);
	allocateGpuOnlyFbo(ndiFbo2, internalWidth, internalHeight);

#if OFAPP_HAS_SPOUT
	// Reallocate Spout input FBOs at INTERNAL resolution - GPU-only
	allocateGpuOnlyFbo(spoutFbo1, internalWidth, internalHeight);
	allocateGpuOnlyFbo(spoutFbo2, internalWidth, internalHeight);

	// Reallocate Spout send FBOs at spout send resolution - GPU-only (Spout uses texture sharing)
	allocateGpuOnlyFbo(spoutSendFbo1, spoutSendWidth, spoutSendHeight);
	allocateGpuOnlyFbo(spoutSendFbo2, spoutSendWidth, spoutSendHeight);
	allocateGpuOnlyFbo(spoutSendFbo3, spoutSendWidth, spoutSendHeight);
#endif

	// Reallocate NDI send FBOs at ndi send resolution
	// NOTE: These NEED CPU backing because NDI reads pixels back to CPU for network transmission
	ndiSendWidth = gui->ndiSendWidth;
	ndiSendHeight = gui->ndiSendHeight;
	ndiSendFbo1.allocate(ndiSendWidth, ndiSendHeight, GL_RGBA);
	ndiSendFbo2.allocate(ndiSendWidth, ndiSendHeight, GL_RGBA);
	ndiSendFbo3.allocate(ndiSendWidth, ndiSendHeight, GL_RGBA);
	ndiSendFbo1.begin();
	ofClear(0,0,0,255);
	ndiSendFbo1.end();
	ndiSendFbo2.begin();
	ofClear(0,0,0,255);
	ndiSendFbo2.end();
	ndiSendFbo3.begin();
	ofClear(0,0,0,255);
	ndiSendFbo3.end();

	// Update NDI senders with new resolution (only if active)
	if(ndiSender1Active) {
		ndiSenderBlock1.UpdateSender(ndiSendWidth, ndiSendHeight);
	}
	if(ndiSender2Active) {
		ndiSenderBlock2.UpdateSender(ndiSendWidth, ndiSendHeight);
	}
	if(ndiSender3Active) {
		ndiSenderBlock3.UpdateSender(ndiSendWidth, ndiSendHeight);
	}

	ofLogNotice("Resolution") << "Resolution reinitialization complete";
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	// Keyboard shortcuts for output window control
	// F11 - Toggle fullscreen
	// F10 - Toggle window decorations (borders/title bar)

	if (key == OF_KEY_F11 && mainWindow) {
		ofLogNotice("ofApp") << "Toggling fullscreen (F11)";
		mainWindow->toggleFullscreen();
	}

	if (key == OF_KEY_F10 && mainWindow) {
		static bool decorated = true;
		decorated = !decorated;
		ofLogNotice("ofApp") << "Toggling window decorations (F10): " << (decorated ? "ON" : "OFF");

		// Get GLFW window handle
		GLFWwindow* glfwWindow = static_cast<ofAppGLFWWindow*>(mainWindow.get())->getGLFWWindow();
		if (glfwWindow) {
			// Store current position and size
			int xpos, ypos, width, height;
			glfwGetWindowPos(glfwWindow, &xpos, &ypos);
			glfwGetWindowSize(glfwWindow, &width, &height);

			// Set window hint and recreate window attributes
			glfwSetWindowAttrib(glfwWindow, GLFW_DECORATED, decorated ? GLFW_TRUE : GLFW_FALSE);

			// Note: GLFW may require window recreation for decoration changes
			// If the above doesn't work immediately, the window may need to be hidden/shown
			glfwHideWindow(glfwWindow);
			glfwShowWindow(glfwWindow);
		}
	}

	if (key == 'h' || key == 'H') {
		ofLogNotice("ofApp") << "=== Gravity Waaaves Keyboard Shortcuts ===";
		ofLogNotice("ofApp") << "F11 - Toggle fullscreen on output window";
		ofLogNotice("ofApp") << "F10 - Toggle window decorations (borders)";
		ofLogNotice("ofApp") << "H   - Show this help";
	}

	if(key=='1'){testSwitch1=1;}
	if(key=='2'){testSwitch1=2;}

	/*
	if(key=='a'){ch1HdAspectXFix+=.001; cout<<"Xfix: "<< ch1HdAspectXFix<<endl;}
	if(key=='z'){ch1HdAspectXFix-=.001; cout<<"Xfix: "<< ch1HdAspectXFix<<endl;}
	if(key=='s'){ch1HdAspectYFix+=.001; cout<<"Yfix: "<< ch1HdAspectYFix<<endl;}
	if(key=='x'){ch1HdAspectYFix-=.001; cout<<"Yfix: "<< ch1HdAspectYFix<<endl;}
	*/

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}
//--------------------------
void ofApp::line_draw(){
	ofVec3f linePosition1;
	ofVec3f linePosition2;

	line_theta+=.01;
	line_phi+=.013;
	line_eta+=.0079;

	ofPushMatrix();
	ofTranslate(outputWidth/2+outputHeight/8.0f*(cos(line_theta)),outputHeight/2+outputHeight/8.0f*(cos(line_eta)));
	linePosition1.x=outputWidth/16.0f;
	linePosition1.y=outputHeight/16.0f;
	linePosition1.z=0;

	linePosition2.x=-outputWidth/16.0f;
	linePosition2.y=-outputHeight/16.0f;
	linePosition2.z=0;

	ofRotateZRad(line_theta);
	ofRotateXRad(line_eta);
	ofRotateYRad(line_phi);

	ofSetColor( 180.0f+63.0f*( sin(line_eta) ),127.0f+127.0f*( sin(line_theta) ),180.0f+63.0f*( sin(line_phi) ) );

	ofDrawLine(linePosition1,linePosition2);
	ofPopMatrix();
}

//--------------------------
void ofApp::hypercube_draw(){

    int limit=3;
    for(int i=0;i<limit;i++){
        hypercube_theta+=.1*gui->hypercube_theta_rate;

        hypercube_phi+=.1*gui->hypercube_phi_rate;

        hypercube_r=outputWidth/32.0f*(gui->hypercube_size);

        float xr=hypercube_r*(1);
        hypercube_x[0]=xr*(cos(hypercube_theta)-sin(hypercube_theta))*(1-.5*(cos(hypercube_phi)));
        hypercube_x[1]=xr*(cos(hypercube_theta)+sin(hypercube_theta))*(1-.5*(cos(PI/4+hypercube_phi)));
        hypercube_x[2]=xr*(-cos(hypercube_theta)+sin(hypercube_theta))*(1-.5*(cos(PI/2+hypercube_phi)));
        hypercube_x[3]=xr*(-cos(hypercube_theta)-sin(hypercube_theta))*(1-.5*(cos(3*PI/4+hypercube_phi)));
        hypercube_x[4]=xr*(cos(hypercube_theta)-sin(hypercube_theta))*(1-.5*(cos(PI+hypercube_phi)));
        hypercube_x[5]=xr*(cos(hypercube_theta)+sin(hypercube_theta))*(1-.5*(cos(5*PI/4+hypercube_phi)));
        hypercube_x[6]=xr*(-cos(hypercube_theta)+sin(hypercube_theta))*(1-.5*(cos(3*PI/2+hypercube_phi)));
        hypercube_x[7]=xr*(-cos(hypercube_theta)-sin(hypercube_theta))*(1-.5*(cos(7*PI/4+hypercube_phi)));

        float yr=hypercube_r*(1);
        hypercube_y[0]=yr*(sin(hypercube_theta)+cos(hypercube_theta))*(1-.5*(cos(hypercube_phi)));
        hypercube_y[1]=yr*(sin(hypercube_theta)-cos(hypercube_theta))*(1-.5*(cos(PI/4+hypercube_phi)));
        hypercube_y[2]=yr*(-sin(hypercube_theta)-cos(hypercube_theta))*(1-.5*(cos(PI/2+hypercube_phi)));
        hypercube_y[3]=yr*(-sin(hypercube_theta)+cos(hypercube_theta))*(1-.5*(cos(3*PI/4+hypercube_phi)));
        hypercube_y[4]=yr*(sin(hypercube_theta)+cos(hypercube_theta))*(1-.5*(cos(PI+hypercube_phi)));
        hypercube_y[5]=yr*(sin(hypercube_theta)-cos(hypercube_theta))*(1-.5*(cos(5*PI/4+hypercube_phi)));
        hypercube_y[6]=yr*(-sin(hypercube_theta)-cos(hypercube_theta))*(1-.5*(cos(3*PI/2+hypercube_phi)));
        hypercube_y[7]=yr*(-sin(hypercube_theta)+cos(hypercube_theta))*(1-.5*(cos(7*PI/4+hypercube_phi)));

        float zr=hypercube_r*(1);
        hypercube_z[0]=-zr/2*cos(hypercube_phi)+hypercube_r;
        hypercube_z[1]=-zr/2*cos(PI/4+hypercube_phi)+hypercube_r;
        hypercube_z[2]=-zr/2*cos(PI/2+hypercube_phi)+hypercube_r;
        hypercube_z[3]=-zr/2*cos(3*PI/4+hypercube_phi)+hypercube_r;
        hypercube_z[4]=-zr/2*cos(PI+hypercube_phi)+hypercube_r;
        hypercube_z[5]=-zr/2*cos(5*PI/4+hypercube_phi)+hypercube_r;
        hypercube_z[6]=-zr/2*cos(3*PI/2+hypercube_phi)+hypercube_r;
        hypercube_z[7]=-zr/2*cos(7*PI/8+hypercube_phi)+hypercube_r;


        hypercube_color_theta+=.01;
        ofSetColor(127+127*sin(hypercube_color_theta),0+192*abs(cos(hypercube_color_theta*.2)),127+127*cos(hypercube_color_theta/3.0f));
        ofNoFill();
        ofPushMatrix();
        ofTranslate(outputWidth/2,outputHeight/2);
        ofRotateYRad(-PI/2);
        ofRotateZRad(hypercube_phi);
        ofRotateXRad(hypercube_theta/3);
        ofRotateXRad(hypercube_theta/5);

        //list up the vertexes, give them some kind of grouping and
        //set up a set of 3 rotatios for each
        //i think just pick like an inner cube and an outer cube
        //if thats even possible

        ofDrawLine(hypercube_x[0],hypercube_y[0],hypercube_z[0],hypercube_x[1],hypercube_y[1],hypercube_z[1]);
        ofDrawLine(hypercube_x[1],hypercube_y[1],hypercube_z[1],hypercube_x[2],hypercube_y[2],hypercube_z[2]);
        ofDrawLine(hypercube_x[2],hypercube_y[2],hypercube_z[2],hypercube_x[3],hypercube_y[3],hypercube_z[3]);
        ofDrawLine(hypercube_x[3],hypercube_y[3],hypercube_z[3],hypercube_x[4],hypercube_y[4],hypercube_z[4]);
        ofDrawLine(hypercube_x[4],hypercube_y[4],hypercube_z[4],hypercube_x[5],hypercube_y[5],hypercube_z[5]);
        ofDrawLine(hypercube_x[5],hypercube_y[5],hypercube_z[5],hypercube_x[6],hypercube_y[6],hypercube_z[6]);
        ofDrawLine(hypercube_x[6],hypercube_y[6],hypercube_z[6],hypercube_x[7],hypercube_y[7],hypercube_z[7]);
        ofDrawLine(hypercube_x[7],hypercube_y[7],hypercube_z[7],hypercube_x[0],hypercube_y[0],hypercube_z[0]);

        ofDrawLine(hypercube_x[0],hypercube_y[0],-hypercube_z[0],hypercube_x[1],hypercube_y[1],-hypercube_z[1]);
        ofDrawLine(hypercube_x[1],hypercube_y[1],-hypercube_z[1],hypercube_x[2],hypercube_y[2],-hypercube_z[2]);
        ofDrawLine(hypercube_x[2],hypercube_y[2],-hypercube_z[2],hypercube_x[3],hypercube_y[3],-hypercube_z[3]);
        ofDrawLine(hypercube_x[3],hypercube_y[3],-hypercube_z[3],hypercube_x[4],hypercube_y[4],-hypercube_z[4]);
        ofDrawLine(hypercube_x[4],hypercube_y[4],-hypercube_z[4],hypercube_x[5],hypercube_y[5],-hypercube_z[5]);
        ofDrawLine(hypercube_x[5],hypercube_y[5],-hypercube_z[5],hypercube_x[6],hypercube_y[6],-hypercube_z[6]);
        ofDrawLine(hypercube_x[6],hypercube_y[6],-hypercube_z[6],hypercube_x[7],hypercube_y[7],-hypercube_z[7]);
        ofDrawLine(hypercube_x[7],hypercube_y[7],-hypercube_z[7],hypercube_x[0],hypercube_y[0],-hypercube_z[0]);

        ofDrawLine(hypercube_x[0],hypercube_y[0],hypercube_z[0],hypercube_x[0],hypercube_y[0],-hypercube_z[0]);
        ofDrawLine(hypercube_x[1],hypercube_y[1],hypercube_z[1],hypercube_x[1],hypercube_y[1],-hypercube_z[1]);
        ofDrawLine(hypercube_x[2],hypercube_y[2],hypercube_z[2],hypercube_x[2],hypercube_y[2],-hypercube_z[2]);
        ofDrawLine(hypercube_x[3],hypercube_y[3],hypercube_z[3],hypercube_x[3],hypercube_y[3],-hypercube_z[3]);
        ofDrawLine(hypercube_x[4],hypercube_y[4],hypercube_z[4],hypercube_x[4],hypercube_y[4],-hypercube_z[4]);
        ofDrawLine(hypercube_x[5],hypercube_y[5],hypercube_z[5],hypercube_x[5],hypercube_y[5],-hypercube_z[5]);
        ofDrawLine(hypercube_x[6],hypercube_y[6],hypercube_z[6],hypercube_x[6],hypercube_y[6],-hypercube_z[6]);
        ofDrawLine(hypercube_x[7],hypercube_y[7],hypercube_z[7],hypercube_x[7],hypercube_y[7],-hypercube_z[7]);

        ofDrawLine(hypercube_x[0],hypercube_y[0],-hypercube_z[0],hypercube_x[4],hypercube_y[4],-hypercube_z[4]);
        ofDrawLine(hypercube_x[1],hypercube_y[1],-hypercube_z[1],hypercube_x[5],hypercube_y[5],-hypercube_z[5]);
        ofDrawLine(hypercube_x[2],hypercube_y[2],-hypercube_z[2],hypercube_x[6],hypercube_y[6],-hypercube_z[6]);
        ofDrawLine(hypercube_x[3],hypercube_y[3],-hypercube_z[3],hypercube_x[7],hypercube_y[7],-hypercube_z[7]);

        ofDrawLine(hypercube_x[0],hypercube_y[0],hypercube_z[0],hypercube_x[4],hypercube_y[4],hypercube_z[4]);
        ofDrawLine(hypercube_x[1],hypercube_y[1],hypercube_z[1],hypercube_x[5],hypercube_y[5],hypercube_z[5]);
        ofDrawLine(hypercube_x[2],hypercube_y[2],hypercube_z[2],hypercube_x[6],hypercube_y[6],hypercube_z[6]);
        ofDrawLine(hypercube_x[3],hypercube_y[3],hypercube_z[3],hypercube_x[7],hypercube_y[7],hypercube_z[7]);

        ofPopMatrix();

    }//endifor
    ofFill();
}

//---------------------------------------------------------
void ofApp::sevenStar1Setup() {
	//how to auto generate coordinates for n division of circumference of unit circle
	for (int i = 0; i < reps; i++) {
		float theta = float(i)*2.0f*PI / float(reps);
		points[i].set(cos(theta), sin(theta));
	}
	int starIncrementer = 0;
	for (int i = 0; i < reps1; i++) {
		points1[i].set(outputHeight / 4.0f*points[starIncrementer]);
		starIncrementer = (starIncrementer + 3) % reps;
	}
	//second odd star styling
	starIncrementer = 0;
	for (int i = 0; i < reps; i++) {
		points2[i].set(outputHeight / 4.0f*points[starIncrementer]);
		starIncrementer = (starIncrementer + 2) % reps;
	}
	position1 = points1[0];
	position2 = points2[0];
}
//------------------------------------------------------
void ofApp::sevenStar1Draw() {

	thetaHue1 += hueInc1;
	thetaHue2 += hueInc2;
	thetaSaturation1 += saturationInc1;
	thetaChaos += .000125*(thetaHue1*(sin(thetaHue2*.00001)) - thetaHue2 * (sin(thetaHue1*.00001)));

	float squareSize = outputWidth / 64;

	ofPushMatrix();

	ofTranslate(outputWidth / 2, outputHeight / 2);

	position1.x = ofLerp(position1.x, points1[index1].x, increment1);
	position1.y = ofLerp(position1.y, points1[index1].y, increment1);
	//this is kinda neat in that it seems to create a bit of a sense of depth
	//could be pretty cool to have for odd stars 2 or more different paths like this that are moving in different orders over one another??
	float shapedX = position1.x;
	float shapedY = position1.y;
	//probably cool thing to do here with 'shaping' would be to work with beziers, arcs, splines etc
	//and then lerp between the two different paths with some kind of oscillator

	ofColor hsbC1;
	hsbC1.setHsb(127.0f + 63.0f * sin(thetaHue1 + thetaChaos) + 63.0f * cos(thetaHue2 - thetaChaos), 190.0f + 63.0f * cos(thetaSaturation1), 255);
	ofSetColor(hsbC1);
	ofDrawEllipse(shapedX, shapedY, squareSize, squareSize);
	hsbC1.setHsb(255.0f - (127.0f + 63.0f * sin(thetaHue1 + thetaChaos) + 63.0f * cos(thetaHue2 - thetaChaos)), 190.0f + 63.0f * cos(thetaSaturation1), 200);
	ofSetColor(hsbC1);
	ofDrawEllipse(shapedX, shapedY, squareSize - 2, squareSize - 2);

	if (position1 != points1[index1]) {
		increment1 += acceleration1;
	}
	if (position1.distance(points1[index1]) < threshold) {
		index1++;
		index1 = index1 % reps1;
		increment1 = 0;
	}

	position2.x = ofLerp(position2.x, points2[index2].x, increment2);
	position2.y = ofLerp(position2.y, points2[index2].y, increment2);

	hsbC1.setHsb(127.0f + 63.0f * sin(thetaHue2 + thetaChaos) - 63.0f * cos(thetaHue1 - thetaChaos), 190.0f + 63.0f * cos(thetaSaturation1 - thetaHue1), 255);
	ofSetColor(hsbC1);
	ofDrawEllipse(position2.x, position2.y, squareSize, squareSize);
	hsbC1.setHsb(255.0f-(127.0f + 63.0f * sin(thetaHue2 + thetaChaos) - 63.0f * cos(thetaHue1 - thetaChaos)), 190.0f + 63.0f * cos(thetaSaturation1 - thetaHue1), 200);
	ofSetColor(hsbC1);
	ofDrawEllipse(position2.x, position2.y, squareSize - 2, squareSize - 2);

	if (position2 != points2[index2]) {
		increment2 += acceleration2;
	}
	if (position2.distance(points2[index2]) < threshold) {
		index2++;
		index2 = index2 % reps;
		increment2 = 0;
	}

	ofPopMatrix();

}
// -------------------------------------------------------------- -
void ofApp::drawSpiralEllipse() {
	spiralTheta1 += spiralTheta1Inc;
	spiralRadius1 += radius1Inc;

	spiralTheta2 += spiralTheta2Inc;
	spiralRadius2 += radius2Inc;

	spiralTheta3 -= spiralTheta3Inc;
	spiralRadius3 += radius3Inc;


	float x1 = spiralRadius1 * .5*(sin(spiralTheta1 - .001*sin(.01*spiralTheta2)) + cos(spiralTheta3) );
	float y1 = spiralRadius1 * .5*(cos(spiralTheta1-.001*cos(.01*spiralTheta3) + sin(spiralTheta3)) );

	float x2 = spiralRadius2 * sin(spiralTheta2);
	float y2 = spiralRadius2 * cos(spiralTheta2);

	float x3 = spiralRadius3 * sin(spiralTheta3);
	float y3 = spiralRadius3 * cos(spiralTheta3);

	float size = (outputHeight / 64)+22*abs((x1+y1)/((outputWidth/2+outputHeight/2)));

	ofPushMatrix();
	ofTranslate(outputWidth / 2, outputHeight / 2);
	/*
	ofSetColor(190 + 63 * sin(spiralTheta3Inc - .00001*y2), 127 + 127 * cos(spiralTheta2Inc + .00001*x3), 190 + 63 * sin(spiralTheta1Inc - .00001*y1), 255);
	ofDrawEllipse(x1, y1, size, size);
	ofSetColor(190 + 63 * sin(spiralTheta1Inc - .00001*x3), 127 + 127 * cos(spiralTheta3Inc + .00001*x2), 190 + 63 * cos(spiralTheta2Inc - .00001*y1), 255);
	ofDrawEllipse(x2, y2, size, size);
	ofSetColor(190 + 63 * sin(spiralTheta2Inc - .00001*y1), 127 + 127 * cos(spiralTheta2Inc + .00001*x1), 190 + 63 * sin(spiralTheta1Inc - .00001*y3), 255);
	ofDrawEllipse(x3, y3, size, size);
	*/

	ofSetColor(190 + 63 * sin(spiralTheta3 - .00001*y2), 127 + 127 * cos(spiralTheta2 + .00001*x3), 190 + 63 * sin(spiralTheta1 - .00001*y1), 255);
	ofDrawEllipse(x1, y1, size, size);
	ofSetColor(255 - (63 + 190 * sin(spiralTheta3 - .00001*y2)), 255 - (127 + 127 * cos(spiralTheta2 + .00001*x3)), 255 - (63 + 190 * sin(spiralTheta1 - .00001*y1)), 255);
	ofDrawEllipse(x1, y1, size - 2, size - 2);

	/*
	ofSetColor(190 + 63 * sin(.1*spiralTheta1 - .00001*x3), 127 + 127 * cos(.1*spiralTheta3 + .00001*x2), 190 + 63 * cos(.1*spiralTheta2 - .00001*y1), 255);
	ofDrawEllipse(x2, y2, size, size);
	ofSetColor(255 - (190 + 63 * sin(.1*spiralTheta1 - .00001*x3)), 255 - (127 + 127 * cos(.1*spiralTheta3 + .00001*x2)), 255 - (190 + 63 * cos(.1*spiralTheta2 - .00001*y1)), 255);
	ofDrawEllipse(x2, y2, size - 2, size - 2);

	ofSetColor(190 + 63 * sin(.1*spiralTheta2 - .00001*y1), 127 + 127 * cos(.1*spiralTheta2 + .00001*x1), 190 + 63 * sin(.1*spiralTheta1 - .00001*y3), 255);
	ofDrawEllipse(x3, y3, size, size);
	ofSetColor(255 - (190 + 63 * sin(.1*spiralTheta2 - .00001*y1)), 255 - (127 + 127 * cos(.1*spiralTheta2 + .00001*x1)), 255 - (190 + 63 * sin(.1*spiralTheta1 - .00001*y3)), 255);
	ofDrawEllipse(x3, y3, size - 2, size - 2);
	*/
	ofPopMatrix();

	spiralTheta1 = fmod(spiralTheta1, TWO_PI);
	spiralTheta2 = fmod(spiralTheta2, TWO_PI);
	spiralTheta3 = fmod(spiralTheta3, TWO_PI);
	if (x1 + size > ofGetWidth() / 2 || y1 + size > ofGetHeight() / 2) {
		spiralRadius1 = 0;
		radius1Inc = .99*ofRandomf();
		spiralTheta1Inc = .08*ofRandomf();
	}

	if (x2 + size > ofGetWidth() / 2 || y2 + size > ofGetHeight() / 2) {
		spiralRadius2 = 0;
		radius2Inc = .8*ofRandomf();
		spiralTheta2Inc = .09*ofRandomf();
	}

	if (x3 + size > ofGetWidth() / 2 || y3 + size > ofGetHeight() / 2) {
		spiralRadius3 = 0;
		radius3Inc = .9*ofRandomf();
		spiralTheta3Inc = .13*ofRandomf();
	}

}

//--------------------------------------------------------------
// Lissajous Curve Generator - Waveshape function
float ofApp::lissajousWave(float theta, int shape) {
	switch(shape) {
		case 0: return sin(theta);                                         // Sine
		case 1: return (2.0f / PI) * asin(sin(theta));                    // Triangle
		case 2: return (2.0f / TWO_PI) * fmod(theta + PI, TWO_PI) - 1.0f; // Ramp
		case 3: return 1.0f - (2.0f / TWO_PI) * fmod(theta + PI, TWO_PI); // Saw
		case 4: return (sin(theta) >= 0.0f) ? 1.0f : -1.0f;               // Square
		default: return sin(theta);
	}
}

//--------------------------------------------------------------
// Lissajous Curve Generator - Block 1
void ofApp::lissajousCurve1Draw() {
	// Apply LFO modulation to base params
	float xFreqMod = gui->lissajous1XFreq + lfo(gui->lissajous1XFreqLfoAmp, lissajous1XFreqLfoTheta, gui->lissajous1XFreqLfoShape);
	float yFreqMod = gui->lissajous1YFreq + lfo(gui->lissajous1YFreqLfoAmp, lissajous1YFreqLfoTheta, gui->lissajous1YFreqLfoShape);
	float zFreqMod = gui->lissajous1ZFreq + lfo(gui->lissajous1ZFreqLfoAmp, lissajous1ZFreqLfoTheta, gui->lissajous1ZFreqLfoShape);
	float xAmpMod = gui->lissajous1XAmp + lfo(gui->lissajous1XAmpLfoAmp, lissajous1XAmpLfoTheta, gui->lissajous1XAmpLfoShape);
	float yAmpMod = gui->lissajous1YAmp + lfo(gui->lissajous1YAmpLfoAmp, lissajous1YAmpLfoTheta, gui->lissajous1YAmpLfoShape);
	float zAmpMod = gui->lissajous1ZAmp + lfo(gui->lissajous1ZAmpLfoAmp, lissajous1ZAmpLfoTheta, gui->lissajous1ZAmpLfoShape);
	float xPhaseMod = gui->lissajous1XPhase + lfo(gui->lissajous1XPhaseLfoAmp, lissajous1XPhaseLfoTheta, gui->lissajous1XPhaseLfoShape);
	float yPhaseMod = gui->lissajous1YPhase + lfo(gui->lissajous1YPhaseLfoAmp, lissajous1YPhaseLfoTheta, gui->lissajous1YPhaseLfoShape);
	float zPhaseMod = gui->lissajous1ZPhase + lfo(gui->lissajous1ZPhaseLfoAmp, lissajous1ZPhaseLfoTheta, gui->lissajous1ZPhaseLfoShape);
	float xOffsetMod = gui->lissajous1XOffset + lfo(gui->lissajous1XOffsetLfoAmp, lissajous1XOffsetLfoTheta, gui->lissajous1XOffsetLfoShape);
	float yOffsetMod = gui->lissajous1YOffset + lfo(gui->lissajous1YOffsetLfoAmp, lissajous1YOffsetLfoTheta, gui->lissajous1YOffsetLfoShape);
	float speedMod = gui->lissajous1Speed + lfo(gui->lissajous1SpeedLfoAmp, lissajous1SpeedLfoTheta, gui->lissajous1SpeedLfoShape);
	float sizeMod = gui->lissajous1Size + lfo(gui->lissajous1SizeLfoAmp, lissajous1SizeLfoTheta, gui->lissajous1SizeLfoShape);
	float numPointsMod = gui->lissajous1NumPoints + lfo(gui->lissajous1NumPointsLfoAmp, lissajous1NumPointsLfoTheta, gui->lissajous1NumPointsLfoShape);
	float lineWidthMod = gui->lissajous1LineWidth + lfo(gui->lissajous1LineWidthLfoAmp, lissajous1LineWidthLfoTheta, gui->lissajous1LineWidthLfoShape);
	float colorSpeedMod = gui->lissajous1ColorSpeed + lfo(gui->lissajous1ColorSpeedLfoAmp, lissajous1ColorSpeedLfoTheta, gui->lissajous1ColorSpeedLfoShape);
	float hueMod = gui->lissajous1Hue + lfo(gui->lissajous1HueLfoAmp, lissajous1HueLfoTheta, gui->lissajous1HueLfoShape);
	float hueSpreadMod = gui->lissajous1HueSpread + lfo(gui->lissajous1HueSpreadLfoAmp, lissajous1HueSpreadLfoTheta, gui->lissajous1HueSpreadLfoShape);

	// Clamp modulated values to valid ranges
	xFreqMod = ofClamp(xFreqMod, 0.0f, 1.0f);
	yFreqMod = ofClamp(yFreqMod, 0.0f, 1.0f);
	zFreqMod = ofClamp(zFreqMod, 0.0f, 1.0f);
	xAmpMod = ofClamp(xAmpMod, 0.0f, 1.0f);
	yAmpMod = ofClamp(yAmpMod, 0.0f, 1.0f);
	zAmpMod = ofClamp(zAmpMod, 0.0f, 1.0f);
	xOffsetMod = ofClamp(xOffsetMod, 0.0f, 1.0f);
	yOffsetMod = ofClamp(yOffsetMod, 0.0f, 1.0f);
	speedMod = ofClamp(speedMod, 0.0f, 1.0f);
	sizeMod = ofClamp(sizeMod, 0.0f, 1.0f);
	numPointsMod = ofClamp(numPointsMod, 0.0f, 1.0f);
	lineWidthMod = ofClamp(lineWidthMod, 0.0f, 1.0f);
	colorSpeedMod = ofClamp(colorSpeedMod, 0.0f, 1.0f);
	hueMod = ofClamp(hueMod, 0.0f, 1.0f);
	hueSpreadMod = ofClamp(hueSpreadMod, 0.0f, 1.0f);

	// Map to useful ranges
	float xFreq = xFreqMod * 10.0f;
	float yFreq = yFreqMod * 10.0f;
	float zFreq = zFreqMod * 10.0f;
	float xAmp = xAmpMod * outputHeight * 0.35f;
	float yAmp = yAmpMod * outputHeight * 0.35f;
	float zAmp = zAmpMod * outputHeight * 0.25f;
	float xPhase = xPhaseMod * TWO_PI;
	float yPhase = yPhaseMod * TWO_PI;
	float zPhase = zPhaseMod * TWO_PI;
	float xCenter = (xOffsetMod - 0.5f) * outputWidth;
	float yCenter = (yOffsetMod - 0.5f) * outputHeight;
	float sizeScale = 0.5f + sizeMod * 1.5f;
	int numPoints = 50 + int(numPointsMod * 450);
	float lineWidth = 1.0f + lineWidthMod * 9.0f;

	// Update animation thetas
	lissajous1Theta += speedMod * 0.1f;
	lissajous1ColorTheta += colorSpeedMod * 0.05f;

	// Wrap thetas to avoid float overflow
	if (lissajous1Theta > TWO_PI * 1000) lissajous1Theta = fmod(lissajous1Theta, TWO_PI * 100);
	if (lissajous1ColorTheta > TWO_PI * 1000) lissajous1ColorTheta = fmod(lissajous1ColorTheta, TWO_PI * 100);

	ofPushMatrix();
	ofTranslate(outputWidth / 2 + xCenter, outputHeight / 2 + yCenter);
	ofSetLineWidth(lineWidth);

	// Use ofMesh for proper per-vertex colors
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_LINE_STRIP);

	for (int i = 0; i < numPoints; i++) {
		float t = lissajous1Theta + (float(i) / numPoints) * TWO_PI * 4;

		float x = xAmp * sizeScale * lissajousWave(xFreq * t + xPhase, gui->lissajous1XShape);
		float y = yAmp * sizeScale * lissajousWave(yFreq * t + yPhase, gui->lissajous1YShape);
		float z = zAmp * sizeScale * lissajousWave(zFreq * t + zPhase, gui->lissajous1ZShape);

		// Color with base hue and spread along curve
		float positionHue = (float(i) / numPoints) * hueSpreadMod;
		float hue = fmod(hueMod + positionHue + lissajous1ColorTheta, 1.0f);
		ofColor c;
		c.setHsb(hue * 255, 200, 220);

		mesh.addVertex(glm::vec3(x, y, z));
		mesh.addColor(c);
	}

	mesh.draw();

	ofSetLineWidth(1);
	ofPopMatrix();
}

//--------------------------------------------------------------
// Lissajous Curve Generator - Block 2
void ofApp::lissajousCurve2Draw() {
	// Apply LFO modulation to base params
	float xFreqMod = gui->lissajous2XFreq + lfo(gui->lissajous2XFreqLfoAmp, lissajous2XFreqLfoTheta, gui->lissajous2XFreqLfoShape);
	float yFreqMod = gui->lissajous2YFreq + lfo(gui->lissajous2YFreqLfoAmp, lissajous2YFreqLfoTheta, gui->lissajous2YFreqLfoShape);
	float zFreqMod = gui->lissajous2ZFreq + lfo(gui->lissajous2ZFreqLfoAmp, lissajous2ZFreqLfoTheta, gui->lissajous2ZFreqLfoShape);
	float xAmpMod = gui->lissajous2XAmp + lfo(gui->lissajous2XAmpLfoAmp, lissajous2XAmpLfoTheta, gui->lissajous2XAmpLfoShape);
	float yAmpMod = gui->lissajous2YAmp + lfo(gui->lissajous2YAmpLfoAmp, lissajous2YAmpLfoTheta, gui->lissajous2YAmpLfoShape);
	float zAmpMod = gui->lissajous2ZAmp + lfo(gui->lissajous2ZAmpLfoAmp, lissajous2ZAmpLfoTheta, gui->lissajous2ZAmpLfoShape);
	float xPhaseMod = gui->lissajous2XPhase + lfo(gui->lissajous2XPhaseLfoAmp, lissajous2XPhaseLfoTheta, gui->lissajous2XPhaseLfoShape);
	float yPhaseMod = gui->lissajous2YPhase + lfo(gui->lissajous2YPhaseLfoAmp, lissajous2YPhaseLfoTheta, gui->lissajous2YPhaseLfoShape);
	float zPhaseMod = gui->lissajous2ZPhase + lfo(gui->lissajous2ZPhaseLfoAmp, lissajous2ZPhaseLfoTheta, gui->lissajous2ZPhaseLfoShape);
	float xOffsetMod = gui->lissajous2XOffset + lfo(gui->lissajous2XOffsetLfoAmp, lissajous2XOffsetLfoTheta, gui->lissajous2XOffsetLfoShape);
	float yOffsetMod = gui->lissajous2YOffset + lfo(gui->lissajous2YOffsetLfoAmp, lissajous2YOffsetLfoTheta, gui->lissajous2YOffsetLfoShape);
	float speedMod = gui->lissajous2Speed + lfo(gui->lissajous2SpeedLfoAmp, lissajous2SpeedLfoTheta, gui->lissajous2SpeedLfoShape);
	float sizeMod = gui->lissajous2Size + lfo(gui->lissajous2SizeLfoAmp, lissajous2SizeLfoTheta, gui->lissajous2SizeLfoShape);
	float numPointsMod = gui->lissajous2NumPoints + lfo(gui->lissajous2NumPointsLfoAmp, lissajous2NumPointsLfoTheta, gui->lissajous2NumPointsLfoShape);
	float lineWidthMod = gui->lissajous2LineWidth + lfo(gui->lissajous2LineWidthLfoAmp, lissajous2LineWidthLfoTheta, gui->lissajous2LineWidthLfoShape);
	float colorSpeedMod = gui->lissajous2ColorSpeed + lfo(gui->lissajous2ColorSpeedLfoAmp, lissajous2ColorSpeedLfoTheta, gui->lissajous2ColorSpeedLfoShape);
	float hueMod = gui->lissajous2Hue + lfo(gui->lissajous2HueLfoAmp, lissajous2HueLfoTheta, gui->lissajous2HueLfoShape);
	float hueSpreadMod = gui->lissajous2HueSpread + lfo(gui->lissajous2HueSpreadLfoAmp, lissajous2HueSpreadLfoTheta, gui->lissajous2HueSpreadLfoShape);

	// Clamp modulated values to valid ranges
	xFreqMod = ofClamp(xFreqMod, 0.0f, 1.0f);
	yFreqMod = ofClamp(yFreqMod, 0.0f, 1.0f);
	zFreqMod = ofClamp(zFreqMod, 0.0f, 1.0f);
	xAmpMod = ofClamp(xAmpMod, 0.0f, 1.0f);
	yAmpMod = ofClamp(yAmpMod, 0.0f, 1.0f);
	zAmpMod = ofClamp(zAmpMod, 0.0f, 1.0f);
	xOffsetMod = ofClamp(xOffsetMod, 0.0f, 1.0f);
	yOffsetMod = ofClamp(yOffsetMod, 0.0f, 1.0f);
	speedMod = ofClamp(speedMod, 0.0f, 1.0f);
	sizeMod = ofClamp(sizeMod, 0.0f, 1.0f);
	numPointsMod = ofClamp(numPointsMod, 0.0f, 1.0f);
	lineWidthMod = ofClamp(lineWidthMod, 0.0f, 1.0f);
	colorSpeedMod = ofClamp(colorSpeedMod, 0.0f, 1.0f);
	hueMod = ofClamp(hueMod, 0.0f, 1.0f);
	hueSpreadMod = ofClamp(hueSpreadMod, 0.0f, 1.0f);

	// Map to useful ranges
	float xFreq = xFreqMod * 10.0f;
	float yFreq = yFreqMod * 10.0f;
	float zFreq = zFreqMod * 10.0f;
	float xAmp = xAmpMod * outputHeight * 0.35f;
	float yAmp = yAmpMod * outputHeight * 0.35f;
	float zAmp = zAmpMod * outputHeight * 0.25f;
	float xPhase = xPhaseMod * TWO_PI;
	float yPhase = yPhaseMod * TWO_PI;
	float zPhase = zPhaseMod * TWO_PI;
	float xCenter = (xOffsetMod - 0.5f) * outputWidth;
	float yCenter = (yOffsetMod - 0.5f) * outputHeight;
	float sizeScale = 0.5f + sizeMod * 1.5f;
	int numPoints = 50 + int(numPointsMod * 450);
	float lineWidth = 1.0f + lineWidthMod * 9.0f;

	// Update animation thetas
	lissajous2Theta += speedMod * 0.1f;
	lissajous2ColorTheta += colorSpeedMod * 0.05f;

	// Wrap thetas to avoid float overflow
	if (lissajous2Theta > TWO_PI * 1000) lissajous2Theta = fmod(lissajous2Theta, TWO_PI * 100);
	if (lissajous2ColorTheta > TWO_PI * 1000) lissajous2ColorTheta = fmod(lissajous2ColorTheta, TWO_PI * 100);

	ofPushMatrix();
	ofTranslate(outputWidth / 2 + xCenter, outputHeight / 2 + yCenter);
	ofSetLineWidth(lineWidth);

	// Use ofMesh for proper per-vertex colors
	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_LINE_STRIP);

	for (int i = 0; i < numPoints; i++) {
		float t = lissajous2Theta + (float(i) / numPoints) * TWO_PI * 4;

		float x = xAmp * sizeScale * lissajousWave(xFreq * t + xPhase, gui->lissajous2XShape);
		float y = yAmp * sizeScale * lissajousWave(yFreq * t + yPhase, gui->lissajous2YShape);
		float z = zAmp * sizeScale * lissajousWave(zFreq * t + zPhase, gui->lissajous2ZShape);

		// Color with base hue and spread along curve
		float positionHue = (float(i) / numPoints) * hueSpreadMod;
		float hue = fmod(hueMod + positionHue + lissajous2ColorTheta, 1.0f);
		ofColor c;
		c.setHsb(hue * 255, 200, 220);

		mesh.addVertex(glm::vec3(x, y, z));
		mesh.addColor(c);
	}

	mesh.draw();

	ofSetLineWidth(1);
	ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::setupOsc() {
    oscReceiver.setup(gui->oscReceivePort);
    oscSender.setup(gui->oscSendIP, gui->oscSendPort);
    oscEnabled = gui->oscEnabled;

    ofLogNotice("OSC") << "OSC initialized - Enabled: " << oscEnabled;
    ofLogNotice("OSC") << "Receiving on port: " << gui->oscReceivePort;
    ofLogNotice("OSC") << "Sending to: " << gui->oscSendIP << ":" << gui->oscSendPort;
}

//--------------------------------------------------------------
//--------------------------------------------------------------
void ofApp::processOscMessages() {
    if (!oscEnabled || !gui->oscEnabled) return;

    // Skip processing if paused (during sendAll)
    if (gui->oscReceivePaused) return;

    while(oscReceiver.hasWaitingMessages()) {
        ofxOscMessage m;
        oscReceiver.getNextMessage(m);

        string address = m.getAddress();
        float value = m.getArgAsFloat(0);

        ofLogNotice("OSC") << "Received: " << address << " = " << value;

        // Try registry lookup first (handles all registered parameters)
        auto it = gui->oscAddressMap.find(address);
        if (it != gui->oscAddressMap.end() && it->second != nullptr) {
            it->second->setValueFromFloat(value);
            continue;  // Found in registry, skip the helper functions
        }

        // Fallback to helper functions for any unregistered parameters
        if (processOscBlock1(address, value, m)) continue;
        if (processOscBlock2(address, value, m)) continue;
        if (processOscBlock3(address, value, m)) continue;
        if (processOscDiscreteParams(address, m)) continue;
        if (processOscBooleanParams(address, m)) continue;
        if (processOscLfoParamsFb(address, value)) continue;
        if (processOscLfoParamsBlock3B1(address, value)) continue;
        if (processOscLfoParamsBlock3B2AndMatrix(address, value)) continue;
        if (processOscResetCommands(address)) continue;
        if (processOscPresetCommands(address, m)) continue;
    }
}

//--------------------------------------------------------------
// OSC PROCESS HELPER FUNCTIONS - Split to avoid MSVC compiler limits
//--------------------------------------------------------------
bool ofApp::processOscBlock1(const string& address, float value, const ofxOscMessage& m) {
    // BLOCK 1 - Channel 1 Adjust (15 parameters)
    if (address == "/gravity/block1/ch1/xDisplace") gui->ch1Adjust[0] = value;
    else if (address == "/gravity/block1/ch1/yDisplace") gui->ch1Adjust[1] = value;
    else if (address == "/gravity/block1/ch1/zDisplace") gui->ch1Adjust[2] = value;
    else if (address == "/gravity/block1/ch1/rotate") gui->ch1Adjust[3] = value;
    else if (address == "/gravity/block1/ch1/hueOffset") gui->ch1Adjust[4] = value;
    else if (address == "/gravity/block1/ch1/saturationOffset") gui->ch1Adjust[5] = value;
    else if (address == "/gravity/block1/ch1/brightOffset") gui->ch1Adjust[6] = value;
    else if (address == "/gravity/block1/ch1/posterize") gui->ch1Adjust[7] = value;
    else if (address == "/gravity/block1/ch1/kaleidoscopeAmount") gui->ch1Adjust[8] = value;
    else if (address == "/gravity/block1/ch1/kaleidoscopeSlice") gui->ch1Adjust[9] = value;
    else if (address == "/gravity/block1/ch1/blurAmount") gui->ch1Adjust[10] = value;
    else if (address == "/gravity/block1/ch1/blurRadius") gui->ch1Adjust[11] = value;
    else if (address == "/gravity/block1/ch1/sharpenAmount") gui->ch1Adjust[12] = value;
    else if (address == "/gravity/block1/ch1/sharpenRadius") gui->ch1Adjust[13] = value;
    else if (address == "/gravity/block1/ch1/filtersBoost") gui->ch1Adjust[14] = value;

    // BLOCK 1 - Channel 1 Adjust LFO (16 parameters)
    else if (address == "/gravity/block1/ch1/lfo/xDisplaceAmp") gui->ch1AdjustLfo[0] = value;
    else if (address == "/gravity/block1/ch1/lfo/xDisplaceRate") gui->ch1AdjustLfo[1] = value;
    else if (address == "/gravity/block1/ch1/lfo/yDisplaceAmp") gui->ch1AdjustLfo[2] = value;
    else if (address == "/gravity/block1/ch1/lfo/yDisplaceRate") gui->ch1AdjustLfo[3] = value;
    else if (address == "/gravity/block1/ch1/lfo/zDisplaceAmp") gui->ch1AdjustLfo[4] = value;
    else if (address == "/gravity/block1/ch1/lfo/zDisplaceRate") gui->ch1AdjustLfo[5] = value;
    else if (address == "/gravity/block1/ch1/lfo/rotateAmp") gui->ch1AdjustLfo[6] = value;
    else if (address == "/gravity/block1/ch1/lfo/rotateRate") gui->ch1AdjustLfo[7] = value;
    else if (address == "/gravity/block1/ch1/lfo/kaleidoscopeSliceAmp") gui->ch1AdjustLfo[14] = value;
    else if (address == "/gravity/block1/ch1/lfo/kaleidoscopeSliceRate") gui->ch1AdjustLfo[15] = value;
    else if (address == "/gravity/block1/ch1/lfo/hueOffsetAmp") gui->ch1AdjustLfo[8] = value;
    else if (address == "/gravity/block1/ch1/lfo/hueOffsetRate") gui->ch1AdjustLfo[9] = value;
    else if (address == "/gravity/block1/ch1/lfo/saturationOffsetAmp") gui->ch1AdjustLfo[10] = value;
    else if (address == "/gravity/block1/ch1/lfo/saturationOffsetRate") gui->ch1AdjustLfo[11] = value;
    else if (address == "/gravity/block1/ch1/lfo/brightOffsetAmp") gui->ch1AdjustLfo[12] = value;
    else if (address == "/gravity/block1/ch1/lfo/brightOffsetRate") gui->ch1AdjustLfo[13] = value;

    // BLOCK 1 - Channel 2 Mix and Key (7 parameters)
    else if (address == "/gravity/block1/ch2/mixAmount") gui->ch2MixAndKey[0] = value;
    else if (address == "/gravity/block1/ch2/keyThreshold") gui->ch2MixAndKey[4] = value;
    else if (address == "/gravity/block1/ch2/keySoft") gui->ch2MixAndKey[5] = value;
    else if (address == "/gravity/block1/ch2/keyBlue") gui->ch2MixAndKey[3] = value;
    else if (address == "/gravity/block1/ch2/keyRed") gui->ch2MixAndKey[1] = value;
    else if (address == "/gravity/block1/ch2/keyGreen") gui->ch2MixAndKey[2] = value;

    // BLOCK 1 - Channel 2 Mix and Key LFO (6 parameters)
    else if (address == "/gravity/block1/ch2/lfo/mixAmountAmp") gui->ch2MixAndKeyLfo[0] = value;
    else if (address == "/gravity/block1/ch2/lfo/mixAmountRate") gui->ch2MixAndKeyLfo[1] = value;
    else if (address == "/gravity/block1/ch2/lfo/keyThresholdAmp") gui->ch2MixAndKeyLfo[2] = value;
    else if (address == "/gravity/block1/ch2/lfo/keyThresholdRate") gui->ch2MixAndKeyLfo[3] = value;
    else if (address == "/gravity/block1/ch2/lfo/keySoftAmp") gui->ch2MixAndKeyLfo[4] = value;
    else if (address == "/gravity/block1/ch2/lfo/keySoftRate") gui->ch2MixAndKeyLfo[5] = value;

    // BLOCK 1 - Channel 2 Adjust (15 parameters)
    else if (address == "/gravity/block1/ch2/xDisplace") gui->ch2Adjust[0] = value;
    else if (address == "/gravity/block1/ch2/yDisplace") gui->ch2Adjust[1] = value;
    else if (address == "/gravity/block1/ch2/zDisplace") gui->ch2Adjust[2] = value;
    else if (address == "/gravity/block1/ch2/rotate") gui->ch2Adjust[3] = value;
    else if (address == "/gravity/block1/ch2/hueOffset") gui->ch2Adjust[4] = value;
    else if (address == "/gravity/block1/ch2/saturationOffset") gui->ch2Adjust[5] = value;
    else if (address == "/gravity/block1/ch2/brightOffset") gui->ch2Adjust[6] = value;
    else if (address == "/gravity/block1/ch2/posterize") gui->ch2Adjust[7] = value;
    else if (address == "/gravity/block1/ch2/kaleidoscopeAmount") gui->ch2Adjust[8] = value;
    else if (address == "/gravity/block1/ch2/kaleidoscopeSlice") gui->ch2Adjust[9] = value;
    else if (address == "/gravity/block1/ch2/blurAmount") gui->ch2Adjust[10] = value;
    else if (address == "/gravity/block1/ch2/blurRadius") gui->ch2Adjust[11] = value;
    else if (address == "/gravity/block1/ch2/sharpenAmount") gui->ch2Adjust[12] = value;
    else if (address == "/gravity/block1/ch2/sharpenRadius") gui->ch2Adjust[13] = value;
    else if (address == "/gravity/block1/ch2/filtersBoost") gui->ch2Adjust[14] = value;

    // BLOCK 1 - Channel 2 Adjust LFO (16 parameters)
    else if (address == "/gravity/block1/ch2/lfo/xDisplaceAmp") gui->ch2AdjustLfo[0] = value;
    else if (address == "/gravity/block1/ch2/lfo/xDisplaceRate") gui->ch2AdjustLfo[1] = value;
    else if (address == "/gravity/block1/ch2/lfo/yDisplaceAmp") gui->ch2AdjustLfo[2] = value;
    else if (address == "/gravity/block1/ch2/lfo/yDisplaceRate") gui->ch2AdjustLfo[3] = value;
    else if (address == "/gravity/block1/ch2/lfo/zDisplaceAmp") gui->ch2AdjustLfo[4] = value;
    else if (address == "/gravity/block1/ch2/lfo/zDisplaceRate") gui->ch2AdjustLfo[5] = value;
    else if (address == "/gravity/block1/ch2/lfo/rotateAmp") gui->ch2AdjustLfo[6] = value;
    else if (address == "/gravity/block1/ch2/lfo/rotateRate") gui->ch2AdjustLfo[7] = value;
    else if (address == "/gravity/block1/ch2/lfo/kaleidoscopeSliceAmp") gui->ch2AdjustLfo[14] = value;
    else if (address == "/gravity/block1/ch2/lfo/kaleidoscopeSliceRate") gui->ch2AdjustLfo[15] = value;
    else if (address == "/gravity/block1/ch2/lfo/hueOffsetAmp") gui->ch2AdjustLfo[8] = value;
    else if (address == "/gravity/block1/ch2/lfo/hueOffsetRate") gui->ch2AdjustLfo[9] = value;
    else if (address == "/gravity/block1/ch2/lfo/saturationOffsetAmp") gui->ch2AdjustLfo[10] = value;
    else if (address == "/gravity/block1/ch2/lfo/saturationOffsetRate") gui->ch2AdjustLfo[11] = value;
    else if (address == "/gravity/block1/ch2/lfo/brightOffsetAmp") gui->ch2AdjustLfo[12] = value;
    else if (address == "/gravity/block1/ch2/lfo/brightOffsetRate") gui->ch2AdjustLfo[13] = value;

    // BLOCK 1 - FB1 Mix and Key (6 parameters)
    else if (address == "/gravity/block1/fb1/mixAmount") gui->fb1MixAndKey[0] = value;
    else if (address == "/gravity/block1/fb1/keyThreshold") gui->fb1MixAndKey[4] = value;
    else if (address == "/gravity/block1/fb1/keySoft") gui->fb1MixAndKey[5] = value;
    else if (address == "/gravity/block1/fb1/keyBlue") gui->fb1MixAndKey[3] = value;
    else if (address == "/gravity/block1/fb1/keyRed") gui->fb1MixAndKey[1] = value;
    else if (address == "/gravity/block1/fb1/keyGreen") gui->fb1MixAndKey[2] = value;

    // BLOCK 1 - FB1 Geometry (10 parameters)
    else if (address == "/gravity/block1/fb1/xDisplace") gui->fb1Geo1[0] = value;
    else if (address == "/gravity/block1/fb1/yDisplace") gui->fb1Geo1[1] = value;
    else if (address == "/gravity/block1/fb1/zDisplace") gui->fb1Geo1[2] = value;
    else if (address == "/gravity/block1/fb1/rotate") gui->fb1Geo1[3] = value;
    else if (address == "/gravity/block1/fb1/xStretch") gui->fb1Geo1[4] = value;
    else if (address == "/gravity/block1/fb1/yStretch") gui->fb1Geo1[5] = value;
    else if (address == "/gravity/block1/fb1/xShear") gui->fb1Geo1[6] = value;
    else if (address == "/gravity/block1/fb1/yShear") gui->fb1Geo1[7] = value;
    else if (address == "/gravity/block1/fb1/kaleidoscopeAmount") gui->fb1Geo1[8] = value;
    else if (address == "/gravity/block1/fb1/kaleidoscopeSlice") gui->fb1Geo1[9] = value;

    // BLOCK 1 - FB1 Color (11 parameters)
    else if (address == "/gravity/block1/fb1/hueOffset") gui->fb1Color1[0] = value;
    else if (address == "/gravity/block1/fb1/saturationOffset") gui->fb1Color1[1] = value;
    else if (address == "/gravity/block1/fb1/brightOffset") gui->fb1Color1[2] = value;
    else if (address == "/gravity/block1/fb1/hueMultiply") gui->fb1Color1[3] = value;
    else if (address == "/gravity/block1/fb1/saturationMultiply") gui->fb1Color1[4] = value;
    else if (address == "/gravity/block1/fb1/brightMultiply") gui->fb1Color1[5] = value;
    else if (address == "/gravity/block1/fb1/huePowmap") gui->fb1Color1[6] = value;
    else if (address == "/gravity/block1/fb1/saturationPowmap") gui->fb1Color1[7] = value;
    else if (address == "/gravity/block1/fb1/brightPowmap") gui->fb1Color1[8] = value;
    else if (address == "/gravity/block1/fb1/hueShaper") gui->fb1Color1[9] = value;
    else if (address == "/gravity/block1/fb1/posterize") gui->fb1Color1[10] = value;

    // BLOCK 1 - FB1 Filters (9 parameters)
    else if (address == "/gravity/block1/fb1/blurAmount") gui->fb1Filters[0] = value;
    else if (address == "/gravity/block1/fb1/blurRadius") gui->fb1Filters[1] = value;
    else if (address == "/gravity/block1/fb1/sharpenAmount") gui->fb1Filters[2] = value;
    else if (address == "/gravity/block1/fb1/sharpenRadius") gui->fb1Filters[3] = value;
    else if (address == "/gravity/block1/fb1/temp1Amount") gui->fb1Filters[4] = value;
    else if (address == "/gravity/block1/fb1/temp1q") gui->fb1Filters[5] = value;
    else if (address == "/gravity/block1/fb1/temp2Amount") gui->fb1Filters[6] = value;
    else if (address == "/gravity/block1/fb1/temp2q") gui->fb1Filters[7] = value;
    else if (address == "/gravity/block1/fb1/filtersBoost") gui->fb1Filters[8] = value;

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscBlock2(const string& address, float value, const ofxOscMessage& m) {
    // BLOCK 2 - Input Adjust (15 parameters)
    if (address == "/gravity/block2/input/xDisplace") gui->block2InputAdjust[0] = value;
    else if (address == "/gravity/block2/input/yDisplace") gui->block2InputAdjust[1] = value;
    else if (address == "/gravity/block2/input/zDisplace") gui->block2InputAdjust[2] = value;
    else if (address == "/gravity/block2/input/rotate") gui->block2InputAdjust[3] = value;
    else if (address == "/gravity/block2/input/hueOffset") gui->block2InputAdjust[4] = value;
    else if (address == "/gravity/block2/input/saturationOffset") gui->block2InputAdjust[5] = value;
    else if (address == "/gravity/block2/input/brightOffset") gui->block2InputAdjust[6] = value;
    else if (address == "/gravity/block2/input/posterize") gui->block2InputAdjust[7] = value;
    else if (address == "/gravity/block2/input/kaleidoscopeAmount") gui->block2InputAdjust[8] = value;
    else if (address == "/gravity/block2/input/kaleidoscopeSlice") gui->block2InputAdjust[9] = value;
    else if (address == "/gravity/block2/input/blurAmount") gui->block2InputAdjust[10] = value;
    else if (address == "/gravity/block2/input/blurRadius") gui->block2InputAdjust[11] = value;
    else if (address == "/gravity/block2/input/sharpenAmount") gui->block2InputAdjust[12] = value;
    else if (address == "/gravity/block2/input/sharpenRadius") gui->block2InputAdjust[13] = value;
    else if (address == "/gravity/block2/input/filtersBoost") gui->block2InputAdjust[14] = value;

    // BLOCK 2 - FB2 Mix and Key (6 parameters)
    else if (address == "/gravity/block2/fb2/mixAmount") gui->fb2MixAndKey[0] = value;
    else if (address == "/gravity/block2/fb2/keyThreshold") gui->fb2MixAndKey[4] = value;
    else if (address == "/gravity/block2/fb2/keySoft") gui->fb2MixAndKey[5] = value;
    else if (address == "/gravity/block2/fb2/keyBlue") gui->fb2MixAndKey[3] = value;
    else if (address == "/gravity/block2/fb2/keyRed") gui->fb2MixAndKey[1] = value;
    else if (address == "/gravity/block2/fb2/keyGreen") gui->fb2MixAndKey[2] = value;

    // BLOCK 2 - FB2 Geometry (10 parameters)
    else if (address == "/gravity/block2/fb2/xDisplace") gui->fb2Geo1[0] = value;
    else if (address == "/gravity/block2/fb2/yDisplace") gui->fb2Geo1[1] = value;
    else if (address == "/gravity/block2/fb2/zDisplace") gui->fb2Geo1[2] = value;
    else if (address == "/gravity/block2/fb2/rotate") gui->fb2Geo1[3] = value;
    else if (address == "/gravity/block2/fb2/xStretch") gui->fb2Geo1[4] = value;
    else if (address == "/gravity/block2/fb2/yStretch") gui->fb2Geo1[5] = value;
    else if (address == "/gravity/block2/fb2/xShear") gui->fb2Geo1[6] = value;
    else if (address == "/gravity/block2/fb2/yShear") gui->fb2Geo1[7] = value;
    else if (address == "/gravity/block2/fb2/kaleidoscopeAmount") gui->fb2Geo1[8] = value;
    else if (address == "/gravity/block2/fb2/kaleidoscopeSlice") gui->fb2Geo1[9] = value;

    // BLOCK 2 - FB2 Color (11 parameters)
    else if (address == "/gravity/block2/fb2/hueOffset") gui->fb2Color1[0] = value;
    else if (address == "/gravity/block2/fb2/saturationOffset") gui->fb2Color1[1] = value;
    else if (address == "/gravity/block2/fb2/brightOffset") gui->fb2Color1[2] = value;
    else if (address == "/gravity/block2/fb2/hueMultiply") gui->fb2Color1[3] = value;
    else if (address == "/gravity/block2/fb2/saturationMultiply") gui->fb2Color1[4] = value;
    else if (address == "/gravity/block2/fb2/brightMultiply") gui->fb2Color1[5] = value;
    else if (address == "/gravity/block2/fb2/huePowmap") gui->fb2Color1[6] = value;
    else if (address == "/gravity/block2/fb2/saturationPowmap") gui->fb2Color1[7] = value;
    else if (address == "/gravity/block2/fb2/brightPowmap") gui->fb2Color1[8] = value;
    else if (address == "/gravity/block2/fb2/hueShaper") gui->fb2Color1[9] = value;
    else if (address == "/gravity/block2/fb2/posterize") gui->fb2Color1[10] = value;

    // BLOCK 2 - FB2 Filters (9 parameters)
    else if (address == "/gravity/block2/fb2/blurAmount") gui->fb2Filters[0] = value;
    else if (address == "/gravity/block2/fb2/blurRadius") gui->fb2Filters[1] = value;
    else if (address == "/gravity/block2/fb2/sharpenAmount") gui->fb2Filters[2] = value;
    else if (address == "/gravity/block2/fb2/sharpenRadius") gui->fb2Filters[3] = value;
    else if (address == "/gravity/block2/fb2/temp1Amount") gui->fb2Filters[4] = value;
    else if (address == "/gravity/block2/fb2/temp1q") gui->fb2Filters[5] = value;
    else if (address == "/gravity/block2/fb2/temp2Amount") gui->fb2Filters[6] = value;
    else if (address == "/gravity/block2/fb2/temp2q") gui->fb2Filters[7] = value;
    else if (address == "/gravity/block2/fb2/filtersBoost") gui->fb2Filters[8] = value;

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscBlock3(const string& address, float value, const ofxOscMessage& m) {
    // BLOCK 3 - Block 1 Geometry (10 parameters)
    if (address == "/gravity/block3/b1/xDisplace") gui->block1Geo[0] = value;
    else if (address == "/gravity/block3/b1/yDisplace") gui->block1Geo[1] = value;
    else if (address == "/gravity/block3/b1/zDisplace") gui->block1Geo[2] = value;
    else if (address == "/gravity/block3/b1/rotate") gui->block1Geo[3] = value;
    else if (address == "/gravity/block3/b1/xStretch") gui->block1Geo[4] = value;
    else if (address == "/gravity/block3/b1/yStretch") gui->block1Geo[5] = value;
    else if (address == "/gravity/block3/b1/xShear") gui->block1Geo[6] = value;
    else if (address == "/gravity/block3/b1/yShear") gui->block1Geo[7] = value;
    else if (address == "/gravity/block3/b1/kaleidoscopeAmount") gui->block1Geo[8] = value;
    else if (address == "/gravity/block3/b1/kaleidoscopeSlice") gui->block1Geo[9] = value;

    // BLOCK 3 - Block 1 Colorize (15 parameters)
    else if (address == "/gravity/block3/b1/colorize/hueBand1") gui->block1Colorize[0] = value;
    else if (address == "/gravity/block3/b1/colorize/saturationBand1") gui->block1Colorize[1] = value;
    else if (address == "/gravity/block3/b1/colorize/brightBand1") gui->block1Colorize[2] = value;
    else if (address == "/gravity/block3/b1/colorize/hueBand2") gui->block1Colorize[3] = value;
    else if (address == "/gravity/block3/b1/colorize/saturationBand2") gui->block1Colorize[4] = value;
    else if (address == "/gravity/block3/b1/colorize/brightBand2") gui->block1Colorize[5] = value;
    else if (address == "/gravity/block3/b1/colorize/hueBand3") gui->block1Colorize[6] = value;
    else if (address == "/gravity/block3/b1/colorize/saturationBand3") gui->block1Colorize[7] = value;
    else if (address == "/gravity/block3/b1/colorize/brightBand3") gui->block1Colorize[8] = value;
    else if (address == "/gravity/block3/b1/colorize/hueBand4") gui->block1Colorize[9] = value;
    else if (address == "/gravity/block3/b1/colorize/saturationBand4") gui->block1Colorize[10] = value;
    else if (address == "/gravity/block3/b1/colorize/brightBand4") gui->block1Colorize[11] = value;
    else if (address == "/gravity/block3/b1/colorize/hueBand5") gui->block1Colorize[12] = value;
    else if (address == "/gravity/block3/b1/colorize/saturationBand5") gui->block1Colorize[13] = value;
    else if (address == "/gravity/block3/b1/colorize/brightBand5") gui->block1Colorize[14] = value;

    // BLOCK 3 - Block 1 Filters (5 parameters)
    else if (address == "/gravity/block3/b1/blurAmount") gui->block1Filters[0] = value;
    else if (address == "/gravity/block3/b1/blurRadius") gui->block1Filters[1] = value;
    else if (address == "/gravity/block3/b1/sharpenAmount") gui->block1Filters[2] = value;
    else if (address == "/gravity/block3/b1/sharpenRadius") gui->block1Filters[3] = value;
    else if (address == "/gravity/block3/b1/filtersBoost") gui->block1Filters[4] = value;

    // BLOCK 3 - Block 2 Geometry (10 parameters)
    else if (address == "/gravity/block3/b2/xDisplace") gui->block2Geo[0] = value;
    else if (address == "/gravity/block3/b2/yDisplace") gui->block2Geo[1] = value;
    else if (address == "/gravity/block3/b2/zDisplace") gui->block2Geo[2] = value;
    else if (address == "/gravity/block3/b2/rotate") gui->block2Geo[3] = value;
    else if (address == "/gravity/block3/b2/xStretch") gui->block2Geo[4] = value;
    else if (address == "/gravity/block3/b2/yStretch") gui->block2Geo[5] = value;
    else if (address == "/gravity/block3/b2/xShear") gui->block2Geo[6] = value;
    else if (address == "/gravity/block3/b2/yShear") gui->block2Geo[7] = value;
    else if (address == "/gravity/block3/b2/kaleidoscopeAmount") gui->block2Geo[8] = value;
    else if (address == "/gravity/block3/b2/kaleidoscopeSlice") gui->block2Geo[9] = value;

    // BLOCK 3 - Block 2 Colorize (15 parameters)
    else if (address == "/gravity/block3/b2/colorize/hueBand1") gui->block2Colorize[0] = value;
    else if (address == "/gravity/block3/b2/colorize/saturationBand1") gui->block2Colorize[1] = value;
    else if (address == "/gravity/block3/b2/colorize/brightBand1") gui->block2Colorize[2] = value;
    else if (address == "/gravity/block3/b2/colorize/hueBand2") gui->block2Colorize[3] = value;
    else if (address == "/gravity/block3/b2/colorize/saturationBand2") gui->block2Colorize[4] = value;
    else if (address == "/gravity/block3/b2/colorize/brightBand2") gui->block2Colorize[5] = value;
    else if (address == "/gravity/block3/b2/colorize/hueBand3") gui->block2Colorize[6] = value;
    else if (address == "/gravity/block3/b2/colorize/saturationBand3") gui->block2Colorize[7] = value;
    else if (address == "/gravity/block3/b2/colorize/brightBand3") gui->block2Colorize[8] = value;
    else if (address == "/gravity/block3/b2/colorize/hueBand4") gui->block2Colorize[9] = value;
    else if (address == "/gravity/block3/b2/colorize/saturationBand4") gui->block2Colorize[10] = value;
    else if (address == "/gravity/block3/b2/colorize/brightBand4") gui->block2Colorize[11] = value;
    else if (address == "/gravity/block3/b2/colorize/hueBand5") gui->block2Colorize[12] = value;
    else if (address == "/gravity/block3/b2/colorize/saturationBand5") gui->block2Colorize[13] = value;
    else if (address == "/gravity/block3/b2/colorize/brightBand5") gui->block2Colorize[14] = value;

    // BLOCK 3 - Block 2 Filters (5 parameters)
    else if (address == "/gravity/block3/b2/blurAmount") gui->block2Filters[0] = value;
    else if (address == "/gravity/block3/b2/blurRadius") gui->block2Filters[1] = value;
    else if (address == "/gravity/block3/b2/sharpenAmount") gui->block2Filters[2] = value;
    else if (address == "/gravity/block3/b2/sharpenRadius") gui->block2Filters[3] = value;
    else if (address == "/gravity/block3/b2/filtersBoost") gui->block2Filters[4] = value;

    // BLOCK 3 - Matrix Mix (9 parameters)
    else if (address == "/gravity/block3/matrixMix/b1RedToB2Red") gui->matrixMix[0] = value;
    else if (address == "/gravity/block3/matrixMix/b1RedToB2Green") gui->matrixMix[1] = value;
    else if (address == "/gravity/block3/matrixMix/b1RedToB2Blue") gui->matrixMix[2] = value;
    else if (address == "/gravity/block3/matrixMix/b1GreenToB2Red") gui->matrixMix[3] = value;
    else if (address == "/gravity/block3/matrixMix/b1GreenToB2Green") gui->matrixMix[4] = value;
    else if (address == "/gravity/block3/matrixMix/b1GreenToB2Blue") gui->matrixMix[5] = value;
    else if (address == "/gravity/block3/matrixMix/b1BlueToB2Red") gui->matrixMix[6] = value;
    else if (address == "/gravity/block3/matrixMix/b1GreenToB2Blue") gui->matrixMix[7] = value;
    else if (address == "/gravity/block3/matrixMix/b1BlueToB2Blue") gui->matrixMix[8] = value;

    // BLOCK 3 - Final Mix and Key (6 parameters)
    else if (address == "/gravity/block3/final/mixAmount") gui->finalMixAndKey[0] = value;
    else if (address == "/gravity/block3/final/keyThreshold") gui->finalMixAndKey[4] = value;
    else if (address == "/gravity/block3/final/keySoft") gui->finalMixAndKey[5] = value;
    else if (address == "/gravity/block3/final/keyInvert") gui->finalMixAndKey[3] = value;
    else if (address == "/gravity/block3/final/keyRed") gui->finalMixAndKey[1] = value;
    else if (address == "/gravity/block3/final/keyGreen") gui->finalMixAndKey[2] = value;

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscDiscreteParams(const string& address, const ofxOscMessage& m) {
    // BLOCK 1 - Ch1 Discrete Parameters
    if (address == "/gravity/block1/ch1/inputSelect") gui->ch1InputSelect = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/geoOverflow") gui->ch1GeoOverflow = m.getArgAsInt(0);

    // BLOCK 1 - Ch2 Mix and Key Discrete Parameters
    else if (address == "/gravity/block1/ch2/inputSelect") gui->ch2InputSelect = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/keyOrder") gui->ch2KeyOrder = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/mixType") gui->ch2MixType = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/mixOverflow") gui->ch2MixOverflow = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/keyMode") gui->ch2KeyMode = m.getArgAsInt(0);

    // BLOCK 1 - Ch2 Adjust Discrete Parameters
    else if (address == "/gravity/block1/ch2/geoOverflow") gui->ch2GeoOverflow = m.getArgAsInt(0);

    // BLOCK 1 - FB1 Mix and Key Discrete Parameters
    else if (address == "/gravity/block1/fb1/keyOrder") gui->fb1KeyOrder = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/mixType") gui->fb1MixType = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/mixOverflow") gui->fb1MixOverflow = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/keyMode") gui->fb1KeyMode = m.getArgAsInt(0);

    // BLOCK 1 - FB1 Geometry Discrete Parameters
    else if (address == "/gravity/block1/fb1/geoOverflow") gui->fb1GeoOverflow = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/delayTime") {
        gui->fb1DelayTime = m.getArgAsInt(0);
        // Send delay in seconds
        float secDelay = (float)gui->fb1DelayTime / (float)gui->targetFPS;
        sendOscParameter("/gravity/block1/fb1/secDelay", roundf(secDelay * 100.0f) / 100.0f);
    }

    // BLOCK 2 - Input Adjust Discrete Parameters
    else if (address == "/gravity/block2/input/inputSelect") gui->block2InputSelect = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/geoOverflow") gui->block2InputGeoOverflow = m.getArgAsInt(0);

    // BLOCK 2 - FB2 Mix and Key Discrete Parameters
    else if (address == "/gravity/block2/fb2/keyOrder") gui->fb2KeyOrder = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/mixType") gui->fb2MixType = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/mixOverflow") gui->fb2MixOverflow = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/keyMode") gui->fb2KeyMode = m.getArgAsInt(0);

    // BLOCK 2 - FB2 Geometry Discrete Parameters
    else if (address == "/gravity/block2/fb2/geoOverflow") gui->fb2GeoOverflow = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/delayTime") {
        gui->fb2DelayTime = m.getArgAsInt(0);
        // Send delay in seconds
        float secDelay = (float)gui->fb2DelayTime / (float)gui->targetFPS;
        sendOscParameter("/gravity/block2/fb2/secDelay", roundf(secDelay * 100.0f) / 100.0f);
    }

    // BLOCK 3 - Block 1 Geometry Discrete Parameters
    else if (address == "/gravity/block3/b1/geoOverflow") gui->block1GeoOverflow = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b1/rotateMode") gui->block1RotateMode = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b1/colorize/colorspace") gui->block1ColorizeHSB_RGB = m.getArgAsInt(0);

    // BLOCK 3 - Block 2 Geometry Discrete Parameters
    else if (address == "/gravity/block3/b2/geoOverflow") gui->block2GeoOverflow = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b2/rotateMode") gui->block2RotateMode = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b2/colorize/colorspace") gui->block2ColorizeHSB_RGB = m.getArgAsInt(0);

    // BLOCK 3 - Matrix Mix Discrete Parameters
    else if (address == "/gravity/block3/matrixMix/mixType") gui->matrixMixType = m.getArgAsInt(0);
    else if (address == "/gravity/block3/matrixMix/overflow") gui->matrixMixOverflow = m.getArgAsInt(0);

    // BLOCK 3 - Final Mix and Key Discrete Parameters
    else if (address == "/gravity/block3/final/keyOrder") gui->finalKeyOrder = m.getArgAsInt(0);
    else if (address == "/gravity/block3/final/mixType") gui->finalMixType = m.getArgAsInt(0);
    else if (address == "/gravity/block3/final/overflow") gui->finalMixOverflow = m.getArgAsInt(0);
    else if (address == "/gravity/block3/final/keyMode") gui->finalKeyMode = m.getArgAsInt(0);

    // SETTINGS - Performance
    else if (address == "/gravity/settings/fps") {
        int fps = m.getArgAsInt(0);
        if (fps < 1) fps = 1;
        if (fps > 60) fps = 60;
        gui->targetFPS = fps;
        gui->fpsChangeRequested = true;
    }

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscBooleanParams(const string& address, const ofxOscMessage& m) {
    // BLOCK 1 - Ch1 Adjust Boolean Parameters
    if (address == "/gravity/block1/ch1/hMirror") gui->ch1HMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/vMirror") gui->ch1VMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/hFlip") gui->ch1HFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/vFlip") gui->ch1VFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/hueInvert") gui->ch1HueInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/saturationInvert") gui->ch1SaturationInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/brightInvert") gui->ch1BrightInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/rgbInvert") gui->ch1RGBInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch1/solarize") gui->ch1Solarize = m.getArgAsInt(0);

    // BLOCK 1 - Ch2 Adjust Boolean Parameters
    else if (address == "/gravity/block1/ch2/hMirror") gui->ch2HMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/vMirror") gui->ch2VMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/hFlip") gui->ch2HFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/vFlip") gui->ch2VFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/hueInvert") gui->ch2HueInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/saturationInvert") gui->ch2SaturationInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/brightInvert") gui->ch2BrightInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/rgbInvert") gui->ch2RGBInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/ch2/solarize") gui->ch2Solarize = m.getArgAsInt(0);

    // BLOCK 1 - FB1 Geometry Boolean Parameters
    else if (address == "/gravity/block1/fb1/hMirror") gui->fb1HMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/vMirror") gui->fb1VMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/hFlip") gui->fb1HFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/vFlip") gui->fb1VFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/rotateMode") gui->fb1RotateMode = m.getArgAsInt(0);

    // BLOCK 1 - FB1 Geometrical Animations
    else if (address == "/gravity/block1/fb1/hypercube") gui->block1HypercubeSwitch = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/dancingLine") gui->block1LineSwitch = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/septagram") gui->block1SevenStarSwitch = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/lissajousBall") gui->block1LissaBallSwitch = m.getArgAsInt(0);

    // BLOCK 1 - FB1 Color Boolean Parameters
    else if (address == "/gravity/block1/fb1/hueInvert") gui->fb1HueInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/saturationInvert") gui->fb1SaturationInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block1/fb1/brightInvert") gui->fb1BrightInvert = m.getArgAsInt(0);

    // BLOCK 2 - Input Adjust Boolean Parameters
    else if (address == "/gravity/block2/input/hMirror") gui->block2InputHMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/vMirror") gui->block2InputVMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/hFlip") gui->block2InputHFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/vFlip") gui->block2InputVFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/hueInvert") gui->block2InputHueInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/saturationInvert") gui->block2InputSaturationInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/brightInvert") gui->block2InputBrightInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/rgbInvert") gui->block2InputRGBInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block2/input/solarize") gui->block2InputSolarize = m.getArgAsInt(0);

    // BLOCK 2 - FB2 Geometry Boolean Parameters
    else if (address == "/gravity/block2/fb2/hMirror") gui->fb2HMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/vMirror") gui->fb2VMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/hFlip") gui->fb2HFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/vFlip") gui->fb2VFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/rotateMode") gui->fb2RotateMode = m.getArgAsInt(0);

    // BLOCK 2 - FB2 Geometrical Animations
    else if (address == "/gravity/block2/fb2/hypercube") gui->block2HypercubeSwitch = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/dancingLine") gui->block2LineSwitch = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/septagram") gui->block2SevenStarSwitch = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/lissajousBall") gui->block2LissaBallSwitch = m.getArgAsInt(0);

    // BLOCK 2 - FB2 Color Boolean Parameters
    else if (address == "/gravity/block2/fb2/hueInvert") gui->fb2HueInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/saturationInvert") gui->fb2SaturationInvert = m.getArgAsInt(0);
    else if (address == "/gravity/block2/fb2/brightInvert") gui->fb2BrightInvert = m.getArgAsInt(0);

    // BLOCK 3 - Block 1 Geometry Boolean Parameters
    else if (address == "/gravity/block3/b1/hMirror") gui->block1HMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b1/vMirror") gui->block1VMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b1/hFlip") gui->block1HFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b1/vFlip") gui->block1VFlip = m.getArgAsInt(0);

    // BLOCK 3 - Block 2 Geometry Boolean Parameters
    else if (address == "/gravity/block3/b2/hMirror") gui->block2HMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b2/vMirror") gui->block2VMirror = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b2/hFlip") gui->block2HFlip = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b2/vFlip") gui->block2VFlip = m.getArgAsInt(0);

    // BLOCK 3 - Color EQ Boolean Parameters
    else if (address == "/gravity/block3/b1/colorize/active") gui->block1ColorizeSwitch = m.getArgAsInt(0);
    else if (address == "/gravity/block3/b2/colorize/active") gui->block2ColorizeSwitch = m.getArgAsInt(0);

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscLfoParamsFb(const string& address, float value) {
    // FB1 Mix and Key LFO (6 parameters)
    if (address == "/gravity/block1/fb1/lfo/mixAmountAmp") gui->fb1MixAndKeyLfo[0] = value;
    else if (address == "/gravity/block1/fb1/lfo/mixAmountRate") gui->fb1MixAndKeyLfo[1] = value;
    else if (address == "/gravity/block1/fb1/lfo/keyThresholdAmp") gui->fb1MixAndKeyLfo[2] = value;
    else if (address == "/gravity/block1/fb1/lfo/keyThresholdRate") gui->fb1MixAndKeyLfo[3] = value;
    else if (address == "/gravity/block1/fb1/lfo/keySoftAmp") gui->fb1MixAndKeyLfo[4] = value;
    else if (address == "/gravity/block1/fb1/lfo/keySoftRate") gui->fb1MixAndKeyLfo[5] = value;

    // FB1 Geometry LFO 1 (8 parameters)
    else if (address == "/gravity/block1/fb1/lfo/xDisplaceAmp") gui->fb1Geo1Lfo1[0] = value;
    else if (address == "/gravity/block1/fb1/lfo/xDisplaceRate") gui->fb1Geo1Lfo1[1] = value;
    else if (address == "/gravity/block1/fb1/lfo/yDisplaceAmp") gui->fb1Geo1Lfo1[2] = value;
    else if (address == "/gravity/block1/fb1/lfo/yDisplaceRate") gui->fb1Geo1Lfo1[3] = value;
    else if (address == "/gravity/block1/fb1/lfo/zDisplaceAmp") gui->fb1Geo1Lfo1[4] = value;
    else if (address == "/gravity/block1/fb1/lfo/zDisplaceRate") gui->fb1Geo1Lfo1[5] = value;
    else if (address == "/gravity/block1/fb1/lfo/rotateAmp") gui->fb1Geo1Lfo1[6] = value;
    else if (address == "/gravity/block1/fb1/lfo/rotateRate") gui->fb1Geo1Lfo1[7] = value;

    // FB1 Geometry LFO 2 (10 parameters)
    else if (address == "/gravity/block1/fb1/lfo/xStretchAmp") gui->fb1Geo1Lfo2[0] = value;
    else if (address == "/gravity/block1/fb1/lfo/xStretchRate") gui->fb1Geo1Lfo2[1] = value;
    else if (address == "/gravity/block1/fb1/lfo/yStretchAmp") gui->fb1Geo1Lfo2[2] = value;
    else if (address == "/gravity/block1/fb1/lfo/yStretchRate") gui->fb1Geo1Lfo2[3] = value;
    else if (address == "/gravity/block1/fb1/lfo/xShearAmp") gui->fb1Geo1Lfo2[4] = value;
    else if (address == "/gravity/block1/fb1/lfo/xShearRate") gui->fb1Geo1Lfo2[5] = value;
    else if (address == "/gravity/block1/fb1/lfo/yShearAmp") gui->fb1Geo1Lfo2[6] = value;
    else if (address == "/gravity/block1/fb1/lfo/yShearRate") gui->fb1Geo1Lfo2[7] = value;
    else if (address == "/gravity/block1/fb1/lfo/kaleidoscopeSliceAmp") gui->fb1Geo1Lfo2[8] = value;
    else if (address == "/gravity/block1/fb1/lfo/kaleidoscopeSliceRate") gui->fb1Geo1Lfo2[9] = value;

    // FB1 Color LFO (6 parameters)
    else if (address == "/gravity/block1/fb1/lfo/huePowmapAmp") gui->fb1Color1Lfo1[0] = value;
    else if (address == "/gravity/block1/fb1/lfo/huePowmapRate") gui->fb1Color1Lfo1[1] = value;
    else if (address == "/gravity/block1/fb1/lfo/saturationPowmapAmp") gui->fb1Color1Lfo1[2] = value;
    else if (address == "/gravity/block1/fb1/lfo/saturationPowmapRate") gui->fb1Color1Lfo1[3] = value;
    else if (address == "/gravity/block1/fb1/lfo/brightPowmapAmp") gui->fb1Color1Lfo1[4] = value;
    else if (address == "/gravity/block1/fb1/lfo/brightPowmapRate") gui->fb1Color1Lfo1[5] = value;

    // FB2 Mix and Key LFO (6 parameters)
    else if (address == "/gravity/block2/fb2/lfo/mixAmountAmp") gui->fb2MixAndKeyLfo[0] = value;
    else if (address == "/gravity/block2/fb2/lfo/mixAmountRate") gui->fb2MixAndKeyLfo[1] = value;
    else if (address == "/gravity/block2/fb2/lfo/keyThresholdAmp") gui->fb2MixAndKeyLfo[2] = value;
    else if (address == "/gravity/block2/fb2/lfo/keyThresholdRate") gui->fb2MixAndKeyLfo[3] = value;
    else if (address == "/gravity/block2/fb2/lfo/keySoftAmp") gui->fb2MixAndKeyLfo[4] = value;
    else if (address == "/gravity/block2/fb2/lfo/keySoftRate") gui->fb2MixAndKeyLfo[5] = value;

    // FB2 Geometry LFO 1 (8 parameters)
    else if (address == "/gravity/block2/fb2/lfo/xDisplaceAmp") gui->fb2Geo1Lfo1[0] = value;
    else if (address == "/gravity/block2/fb2/lfo/xDisplaceRate") gui->fb2Geo1Lfo1[1] = value;
    else if (address == "/gravity/block2/fb2/lfo/yDisplaceAmp") gui->fb2Geo1Lfo1[2] = value;
    else if (address == "/gravity/block2/fb2/lfo/yDisplaceRate") gui->fb2Geo1Lfo1[3] = value;
    else if (address == "/gravity/block2/fb2/lfo/zDisplaceAmp") gui->fb2Geo1Lfo1[4] = value;
    else if (address == "/gravity/block2/fb2/lfo/zDisplaceRate") gui->fb2Geo1Lfo1[5] = value;
    else if (address == "/gravity/block2/fb2/lfo/rotateAmp") gui->fb2Geo1Lfo1[6] = value;
    else if (address == "/gravity/block2/fb2/lfo/rotateRate") gui->fb2Geo1Lfo1[7] = value;

    // FB2 Geometry LFO 2 (10 parameters)
    else if (address == "/gravity/block2/fb2/lfo/xStretchAmp") gui->fb2Geo1Lfo2[0] = value;
    else if (address == "/gravity/block2/fb2/lfo/xStretchRate") gui->fb2Geo1Lfo2[1] = value;
    else if (address == "/gravity/block2/fb2/lfo/yStretchAmp") gui->fb2Geo1Lfo2[2] = value;
    else if (address == "/gravity/block2/fb2/lfo/yStretchRate") gui->fb2Geo1Lfo2[3] = value;
    else if (address == "/gravity/block2/fb2/lfo/xShearAmp") gui->fb2Geo1Lfo2[4] = value;
    else if (address == "/gravity/block2/fb2/lfo/xShearRate") gui->fb2Geo1Lfo2[5] = value;
    else if (address == "/gravity/block2/fb2/lfo/yShearAmp") gui->fb2Geo1Lfo2[6] = value;
    else if (address == "/gravity/block2/fb2/lfo/yShearRate") gui->fb2Geo1Lfo2[7] = value;
    else if (address == "/gravity/block2/fb2/lfo/kaleidoscopeSliceAmp") gui->fb2Geo1Lfo2[8] = value;
    else if (address == "/gravity/block2/fb2/lfo/kaleidoscopeSliceRate") gui->fb2Geo1Lfo2[9] = value;

    // FB2 Color LFO (6 parameters)
    else if (address == "/gravity/block2/fb2/lfo/huePowmapAmp") gui->fb2Color1Lfo1[0] = value;
    else if (address == "/gravity/block2/fb2/lfo/huePowmapRate") gui->fb2Color1Lfo1[1] = value;
    else if (address == "/gravity/block2/fb2/lfo/saturationPowmapAmp") gui->fb2Color1Lfo1[2] = value;
    else if (address == "/gravity/block2/fb2/lfo/saturationPowmapRate") gui->fb2Color1Lfo1[3] = value;
    else if (address == "/gravity/block2/fb2/lfo/brightPowmapAmp") gui->fb2Color1Lfo1[4] = value;
    else if (address == "/gravity/block2/fb2/lfo/brightPowmapRate") gui->fb2Color1Lfo1[5] = value;

    // Block 2 Input Adjust LFO (16 parameters)
    else if (address == "/gravity/block2/input/lfo/xDisplaceAmp") gui->block2InputAdjustLfo[0] = value;
    else if (address == "/gravity/block2/input/lfo/xDisplaceRate") gui->block2InputAdjustLfo[1] = value;
    else if (address == "/gravity/block2/input/lfo/yDisplaceAmp") gui->block2InputAdjustLfo[2] = value;
    else if (address == "/gravity/block2/input/lfo/yDisplaceRate") gui->block2InputAdjustLfo[3] = value;
    else if (address == "/gravity/block2/input/lfo/zDisplaceAmp") gui->block2InputAdjustLfo[4] = value;
    else if (address == "/gravity/block2/input/lfo/zDisplaceRate") gui->block2InputAdjustLfo[5] = value;
    else if (address == "/gravity/block2/input/lfo/rotateAmp") gui->block2InputAdjustLfo[6] = value;
    else if (address == "/gravity/block2/input/lfo/rotateRate") gui->block2InputAdjustLfo[7] = value;
    else if (address == "/gravity/block2/input/lfo/hueOffsetAmp") gui->block2InputAdjustLfo[8] = value;
    else if (address == "/gravity/block2/input/lfo/hueOffsetRate") gui->block2InputAdjustLfo[9] = value;
    else if (address == "/gravity/block2/input/lfo/saturationOffsetAmp") gui->block2InputAdjustLfo[10] = value;
    else if (address == "/gravity/block2/input/lfo/saturationOffsetRate") gui->block2InputAdjustLfo[11] = value;
    else if (address == "/gravity/block2/input/lfo/brightOffsetAmp") gui->block2InputAdjustLfo[12] = value;
    else if (address == "/gravity/block2/input/lfo/brightOffsetRate") gui->block2InputAdjustLfo[13] = value;
    else if (address == "/gravity/block2/input/lfo/kaleidoscopeSliceAmp") gui->block2InputAdjustLfo[14] = value;
    else if (address == "/gravity/block2/input/lfo/kaleidoscopeSliceRate") gui->block2InputAdjustLfo[15] = value;

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscLfoParamsBlock3B1(const string& address, float value) {
    // Block3 B1 Colorize LFO 1 (12 parameters)
    if (address == "/gravity/block3/lfo/b1/hueBand1Amp") gui->block1ColorizeLfo1[0] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand1Amp") gui->block1ColorizeLfo1[1] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand1Amp") gui->block1ColorizeLfo1[2] = value;
    else if (address == "/gravity/block3/lfo/b1/hueBand1Rate") gui->block1ColorizeLfo1[3] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand1Rate") gui->block1ColorizeLfo1[4] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand1Rate") gui->block1ColorizeLfo1[5] = value;
    else if (address == "/gravity/block3/lfo/b1/hueBand2Amp") gui->block1ColorizeLfo1[6] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand2Amp") gui->block1ColorizeLfo1[7] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand2Amp") gui->block1ColorizeLfo1[8] = value;
    else if (address == "/gravity/block3/lfo/b1/hueBand2Rate") gui->block1ColorizeLfo1[9] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand2Rate") gui->block1ColorizeLfo1[10] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand2Rate") gui->block1ColorizeLfo1[11] = value;

    // Block3 B1 Colorize LFO 2 (12 parameters)
    else if (address == "/gravity/block3/lfo/b1/hueBand3Amp") gui->block1ColorizeLfo2[0] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand3Amp") gui->block1ColorizeLfo2[1] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand3Amp") gui->block1ColorizeLfo2[2] = value;
    else if (address == "/gravity/block3/lfo/b1/hueBand3Rate") gui->block1ColorizeLfo2[3] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand3Rate") gui->block1ColorizeLfo2[4] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand3Rate") gui->block1ColorizeLfo2[5] = value;
    else if (address == "/gravity/block3/lfo/b1/hueBand4Amp") gui->block1ColorizeLfo2[6] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand4Amp") gui->block1ColorizeLfo2[7] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand4Amp") gui->block1ColorizeLfo2[8] = value;
    else if (address == "/gravity/block3/lfo/b1/hueBand4Rate") gui->block1ColorizeLfo2[9] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand4Rate") gui->block1ColorizeLfo2[10] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand4Rate") gui->block1ColorizeLfo2[11] = value;

    // Block3 B1 Colorize LFO 3 (6 parameters)
    else if (address == "/gravity/block3/lfo/b1/hueBand5Amp") gui->block1ColorizeLfo3[0] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand5Amp") gui->block1ColorizeLfo3[1] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand5Amp") gui->block1ColorizeLfo3[2] = value;
    else if (address == "/gravity/block3/lfo/b1/hueBand5Rate") gui->block1ColorizeLfo3[3] = value;
    else if (address == "/gravity/block3/lfo/b1/saturationBand5Rate") gui->block1ColorizeLfo3[4] = value;
    else if (address == "/gravity/block3/lfo/b1/brightBand5Rate") gui->block1ColorizeLfo3[5] = value;

    // Block3 B1 Geo LFO 1 (8 parameters)
    else if (address == "/gravity/block3/lfo/b1/xDisplaceAmp") gui->block1Geo1Lfo1[0] = value;
    else if (address == "/gravity/block3/lfo/b1/xDisplaceRate") gui->block1Geo1Lfo1[1] = value;
    else if (address == "/gravity/block3/lfo/b1/yDisplaceAmp") gui->block1Geo1Lfo1[2] = value;
    else if (address == "/gravity/block3/lfo/b1/yDisplaceRate") gui->block1Geo1Lfo1[3] = value;
    else if (address == "/gravity/block3/lfo/b1/zDisplaceAmp") gui->block1Geo1Lfo1[4] = value;
    else if (address == "/gravity/block3/lfo/b1/zDisplaceRate") gui->block1Geo1Lfo1[5] = value;
    else if (address == "/gravity/block3/lfo/b1/rotateAmp") gui->block1Geo1Lfo1[6] = value;
    else if (address == "/gravity/block3/lfo/b1/rotateRate") gui->block1Geo1Lfo1[7] = value;

    // Block3 B1 Geo LFO 2 (10 parameters)
    else if (address == "/gravity/block3/lfo/b1/xStretchAmp") gui->block1Geo1Lfo2[0] = value;
    else if (address == "/gravity/block3/lfo/b1/xStretchRate") gui->block1Geo1Lfo2[1] = value;
    else if (address == "/gravity/block3/lfo/b1/yStretchAmp") gui->block1Geo1Lfo2[2] = value;
    else if (address == "/gravity/block3/lfo/b1/yStretchRate") gui->block1Geo1Lfo2[3] = value;
    else if (address == "/gravity/block3/lfo/b1/xShearAmp") gui->block1Geo1Lfo2[4] = value;
    else if (address == "/gravity/block3/lfo/b1/xShearRate") gui->block1Geo1Lfo2[5] = value;
    else if (address == "/gravity/block3/lfo/b1/yShearAmp") gui->block1Geo1Lfo2[6] = value;
    else if (address == "/gravity/block3/lfo/b1/yShearRate") gui->block1Geo1Lfo2[7] = value;
    else if (address == "/gravity/block3/lfo/b1/kaleidoscopeSliceAmp") gui->block1Geo1Lfo2[8] = value;
    else if (address == "/gravity/block3/lfo/b1/kaleidoscopeSliceRate") gui->block1Geo1Lfo2[9] = value;

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscLfoParamsBlock3B2AndMatrix(const string& address, float value) {
    // Block3 B2 Colorize LFO 1 (12 parameters)
    if (address == "/gravity/block3/lfo/b2/hueBand1Amp") gui->block2ColorizeLfo1[0] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand1Amp") gui->block2ColorizeLfo1[1] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand1Amp") gui->block2ColorizeLfo1[2] = value;
    else if (address == "/gravity/block3/lfo/b2/hueBand1Rate") gui->block2ColorizeLfo1[3] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand1Rate") gui->block2ColorizeLfo1[4] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand1Rate") gui->block2ColorizeLfo1[5] = value;
    else if (address == "/gravity/block3/lfo/b2/hueBand2Amp") gui->block2ColorizeLfo1[6] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand2Amp") gui->block2ColorizeLfo1[7] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand2Amp") gui->block2ColorizeLfo1[8] = value;
    else if (address == "/gravity/block3/lfo/b2/hueBand2Rate") gui->block2ColorizeLfo1[9] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand2Rate") gui->block2ColorizeLfo1[10] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand2Rate") gui->block2ColorizeLfo1[11] = value;

    // Block3 B2 Colorize LFO 2 (12 parameters)
    else if (address == "/gravity/block3/lfo/b2/hueBand3Amp") gui->block2ColorizeLfo2[0] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand3Amp") gui->block2ColorizeLfo2[1] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand3Amp") gui->block2ColorizeLfo2[2] = value;
    else if (address == "/gravity/block3/lfo/b2/hueBand3Rate") gui->block2ColorizeLfo2[3] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand3Rate") gui->block2ColorizeLfo2[4] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand3Rate") gui->block2ColorizeLfo2[5] = value;
    else if (address == "/gravity/block3/lfo/b2/hueBand4Amp") gui->block2ColorizeLfo2[6] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand4Amp") gui->block2ColorizeLfo2[7] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand4Amp") gui->block2ColorizeLfo2[8] = value;
    else if (address == "/gravity/block3/lfo/b2/hueBand4Rate") gui->block2ColorizeLfo2[9] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand4Rate") gui->block2ColorizeLfo2[10] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand4Rate") gui->block2ColorizeLfo2[11] = value;

    // Block3 B2 Colorize LFO 3 (6 parameters)
    else if (address == "/gravity/block3/lfo/b2/hueBand5Amp") gui->block2ColorizeLfo3[0] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand5Amp") gui->block2ColorizeLfo3[1] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand5Amp") gui->block2ColorizeLfo3[2] = value;
    else if (address == "/gravity/block3/lfo/b2/hueBand5Rate") gui->block2ColorizeLfo3[3] = value;
    else if (address == "/gravity/block3/lfo/b2/saturationBand5Rate") gui->block2ColorizeLfo3[4] = value;
    else if (address == "/gravity/block3/lfo/b2/brightBand5Rate") gui->block2ColorizeLfo3[5] = value;

    // Block3 B2 Geo LFO 1 (8 parameters)
    else if (address == "/gravity/block3/lfo/b2/xDisplaceAmp") gui->block2Geo1Lfo1[0] = value;
    else if (address == "/gravity/block3/lfo/b2/xDisplaceRate") gui->block2Geo1Lfo1[1] = value;
    else if (address == "/gravity/block3/lfo/b2/yDisplaceAmp") gui->block2Geo1Lfo1[2] = value;
    else if (address == "/gravity/block3/lfo/b2/yDisplaceRate") gui->block2Geo1Lfo1[3] = value;
    else if (address == "/gravity/block3/lfo/b2/zDisplaceAmp") gui->block2Geo1Lfo1[4] = value;
    else if (address == "/gravity/block3/lfo/b2/zDisplaceRate") gui->block2Geo1Lfo1[5] = value;
    else if (address == "/gravity/block3/lfo/b2/rotateAmp") gui->block2Geo1Lfo1[6] = value;
    else if (address == "/gravity/block3/lfo/b2/rotateRate") gui->block2Geo1Lfo1[7] = value;

    // Block3 B2 Geo LFO 2 (10 parameters)
    else if (address == "/gravity/block3/lfo/b2/xStretchAmp") gui->block2Geo1Lfo2[0] = value;
    else if (address == "/gravity/block3/lfo/b2/xStretchRate") gui->block2Geo1Lfo2[1] = value;
    else if (address == "/gravity/block3/lfo/b2/yStretchAmp") gui->block2Geo1Lfo2[2] = value;
    else if (address == "/gravity/block3/lfo/b2/yStretchRate") gui->block2Geo1Lfo2[3] = value;
    else if (address == "/gravity/block3/lfo/b2/xShearAmp") gui->block2Geo1Lfo2[4] = value;
    else if (address == "/gravity/block3/lfo/b2/xShearRate") gui->block2Geo1Lfo2[5] = value;
    else if (address == "/gravity/block3/lfo/b2/yShearAmp") gui->block2Geo1Lfo2[6] = value;
    else if (address == "/gravity/block3/lfo/b2/yShearRate") gui->block2Geo1Lfo2[7] = value;
    else if (address == "/gravity/block3/lfo/b2/kaleidoscopeSliceAmp") gui->block2Geo1Lfo2[8] = value;
    else if (address == "/gravity/block3/lfo/b2/kaleidoscopeSliceRate") gui->block2Geo1Lfo2[9] = value;

    // Final Mix and Key LFO (6 parameters)
    else if (address == "/gravity/block3/lfo/final/mixAmountAmp") gui->finalMixAndKeyLfo[0] = value;
    else if (address == "/gravity/block3/lfo/final/mixAmountRate") gui->finalMixAndKeyLfo[1] = value;
    else if (address == "/gravity/block3/lfo/final/keyThresholdAmp") gui->finalMixAndKeyLfo[2] = value;
    else if (address == "/gravity/block3/lfo/final/keyThresholdRate") gui->finalMixAndKeyLfo[3] = value;
    else if (address == "/gravity/block3/lfo/final/keySoftAmp") gui->finalMixAndKeyLfo[4] = value;
    else if (address == "/gravity/block3/lfo/final/keySoftRate") gui->finalMixAndKeyLfo[5] = value;

    // Matrix Mix LFO 1 (12 parameters)
    else if (address == "/gravity/block3/lfo/matrixMix/b1RedToB2RedAmp") gui->matrixMixLfo1[0] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1RedToB2GreenAmp") gui->matrixMixLfo1[1] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1RedToB2BlueAmp") gui->matrixMixLfo1[2] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1RedToB2RedRate") gui->matrixMixLfo1[3] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1RedToB2GreenRate") gui->matrixMixLfo1[4] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1RedToB2BlueRate") gui->matrixMixLfo1[5] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1GreenToB2RedAmp") gui->matrixMixLfo1[6] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1GreenToB2GreenAmp") gui->matrixMixLfo1[7] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1GreenToB2BlueAmp") gui->matrixMixLfo1[8] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1GreenToB2RedRate") gui->matrixMixLfo1[9] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1GreenToB2GreenRate") gui->matrixMixLfo1[10] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1GreenToB2BlueRate") gui->matrixMixLfo1[11] = value;

    // Matrix Mix LFO 2 (6 parameters)
    else if (address == "/gravity/block3/lfo/matrixMix/b1BlueToB2RedAmp") gui->matrixMixLfo2[0] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1BlueToB2GreenAmp") gui->matrixMixLfo2[1] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1BlueToB2BlueAmp") gui->matrixMixLfo2[2] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1BlueToB2RedRate") gui->matrixMixLfo2[3] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1BlueToB2GreenRate") gui->matrixMixLfo2[4] = value;
    else if (address == "/gravity/block3/lfo/matrixMix/b1BlueToB2BlueRate") gui->matrixMixLfo2[5] = value;

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscResetCommands(const string& address) {
    // BLOCK 1 Resets
    if (address == "/gravity/block1/ch1/resetAdjust") gui->ch1AdjustReset = 1;
    else if (address == "/gravity/block1/ch1/lfo/resetAdjust") gui->ch1AdjustLfoReset = 1;
    else if (address == "/gravity/block1/ch2/resetAdjust") gui->ch2AdjustReset = 1;
    else if (address == "/gravity/block1/ch2/lfo/resetAdjust") gui->ch2AdjustLfoReset = 1;
    else if (address == "/gravity/block1/ch2/resetMixAndKey") gui->ch2MixAndKeyReset = 1;
    else if (address == "/gravity/block1/ch2/lfo/resetMixAndKey") gui->ch2MixAndKeyLfoReset = 1;
    else if (address == "/gravity/block1/fb1/resetMixAndKey") gui->fb1MixAndKeyReset = 1;
    else if (address == "/gravity/block1/fb1/lfo/resetMixAndKey") gui->fb1MixAndKeyLfoReset = 1;
    else if (address == "/gravity/block1/fb1/resetGeo") gui->fb1Geo1Reset = 1;
    else if (address == "/gravity/block1/fb1/lfo/resetGeo1") gui->fb1Geo1Lfo1Reset = 1;
    else if (address == "/gravity/block1/fb1/lfo/resetGeo2") gui->fb1Geo1Lfo2Reset = 1;
    else if (address == "/gravity/block1/fb1/resetColor") gui->fb1Color1Reset = 1;
    else if (address == "/gravity/block1/fb1/lfo/resetColor") gui->fb1Color1Lfo1Reset = 1;
    else if (address == "/gravity/block1/fb1/resetFilters") gui->fb1FiltersReset = 1;

    // BLOCK 2 Resets
    else if (address == "/gravity/block2/input/resetAdjust") gui->block2InputAdjustReset = 1;
    else if (address == "/gravity/block2/input/lfo/resetAdjust") gui->block2InputAdjustLfoReset = 1;
    else if (address == "/gravity/block2/fb2/resetMixAndKey") gui->fb2MixAndKeyReset = 1;
    else if (address == "/gravity/block2/fb2/lfo/resetMixAndKey") gui->fb2MixAndKeyLfoReset = 1;
    else if (address == "/gravity/block2/fb2/resetGeo") gui->fb2Geo1Reset = 1;
    else if (address == "/gravity/block2/fb2/lfo/resetGeo1") gui->fb2Geo1Lfo1Reset = 1;
    else if (address == "/gravity/block2/fb2/lfo/resetGeo2") gui->fb2Geo1Lfo2Reset = 1;
    else if (address == "/gravity/block2/fb2/resetColor") gui->fb2Color1Reset = 1;
    else if (address == "/gravity/block2/fb2/lfo/resetColor") gui->fb2Color1Lfo1Reset = 1;
    else if (address == "/gravity/block2/fb2/resetFilters") gui->fb2FiltersReset = 1;

    // BLOCK 3 Resets
    else if (address == "/gravity/block3/b1/resetGeo") gui->block1GeoReset = 1;
    else if (address == "/gravity/block3/b1/resetColorize") gui->block1ColorizeReset = 1;
    else if (address == "/gravity/block3/b1/resetFilters") gui->block1FiltersReset = 1;
    else if (address == "/gravity/block3/b1/lfo/resetGeo1") gui->block1Geo1Lfo1Reset = 1;
    else if (address == "/gravity/block3/b1/lfo/resetGeo2") gui->block1Geo1Lfo2Reset = 1;
    else if (address == "/gravity/block3/b1/lfo/resetColorize1") gui->block1ColorizeLfo1Reset = 1;
    else if (address == "/gravity/block3/b1/lfo/resetColorize2") gui->block1ColorizeLfo2Reset = 1;
    else if (address == "/gravity/block3/b1/lfo/resetColorize3") gui->block1ColorizeLfo3Reset = 1;
    else if (address == "/gravity/block3/b2/resetGeo") gui->block2GeoReset = 1;
    else if (address == "/gravity/block3/b2/resetColorize") gui->block2ColorizeReset = 1;
    else if (address == "/gravity/block3/b2/resetFilters") gui->block2FiltersReset = 1;
    else if (address == "/gravity/block3/b2/lfo/resetGeo1") gui->block2Geo1Lfo1Reset = 1;
    else if (address == "/gravity/block3/b2/lfo/resetGeo2") gui->block2Geo1Lfo2Reset = 1;
    else if (address == "/gravity/block3/b2/lfo/resetColorize1") gui->block2ColorizeLfo1Reset = 1;
    else if (address == "/gravity/block3/b2/lfo/resetColorize2") gui->block2ColorizeLfo2Reset = 1;
    else if (address == "/gravity/block3/b2/lfo/resetColorize3") gui->block2ColorizeLfo3Reset = 1;
    else if (address == "/gravity/block3/matrixMix/reset") gui->matrixMixReset = 1;
    else if (address == "/gravity/block3/matrixMix/lfo/reset1") gui->matrixMixLfo1Reset = 1;
    else if (address == "/gravity/block3/matrixMix/lfo/reset2") gui->matrixMixLfo2Reset = 1;
    else if (address == "/gravity/block3/final/resetMixAndKey") gui->finalMixAndKeyReset = 1;
    else if (address == "/gravity/block3/final/lfo/reset") gui->finalMixAndKeyLfoReset = 1;

    // Macro data resets
    else if (address == "/gravity/macro/reset") gui->macroDataReset = 1;
    else if (address == "/gravity/macro/resetAssignments") gui->macroDataResetAssignments = 1;

    // Framebuffer clears
    else if (address == "/gravity/block1/fb1/clear") gui->fb1FramebufferClearSwitch = 1;
    else if (address == "/gravity/block2/fb2/clear") gui->fb2FramebufferClearSwitch = 1;

    // Send all OSC values
    else if (address == "/gravity/sendAll") gui->sendAllOscValues = 1;

    // Block-level resets
    else if (address == "/gravity/resetAll") gui->resetAllSwitch = 1;
    else if (address == "/gravity/block1/resetAll") gui->block1ResetAllSwitch = 1;
    else if (address == "/gravity/block1/resetInputs") gui->block1InputResetAllSwitch = 1;
    else if (address == "/gravity/block1/fb1/resetAll") gui->fb1ResetAllSwitch = 1;
    else if (address == "/gravity/block2/resetAll") gui->block2ResetAllSwitch = 1;
    else if (address == "/gravity/block2/resetInput") gui->block2InputResetAllSwitch = 1;
    else if (address == "/gravity/block2/fb2/resetAll") gui->fb2ResetAllSwitch = 1;
    else if (address == "/gravity/block3/resetAll") gui->block3ResetAllSwitch = 1;

    else return false;
    return true;
}

//--------------------------------------------------------------
bool ofApp::processOscPresetCommands(const string& address, const ofxOscMessage& m) {
    // Preset selection commands (just change dropdown, don't load/save)
    if (address == "/gravity/preset/selectLoad") {
        if (m.getNumArgs() > 0) {
            int presetIndex = static_cast<int>(m.getArgAsFloat(0));
            gui->loadStateSelectSwitch = presetIndex;
        }
    }
    else if (address == "/gravity/preset/selectSave") {
        if (m.getNumArgs() > 0) {
            int presetIndex = static_cast<int>(m.getArgAsFloat(0));
            gui->saveStateSelectSwitch = presetIndex;
        }
    }
    // Preset action commands (actually load/save)
    else if (address == "/gravity/preset/load") {
        gui->loadALL = 1;
    }
    else if (address == "/gravity/preset/save") {
        gui->saveALL = 1;
    }
    // Bank switching commands - Save Bank
    else if (address == "/gravity/preset/saveBank/index") {
        if (m.getNumArgs() > 0) {
            int bankIndex = static_cast<int>(m.getArgAsFloat(0));
            gui->switchSaveBank(bankIndex);
        }
    }
    else if (address == "/gravity/preset/saveBank/name") {
        if (m.getNumArgs() > 0 && m.getArgType(0) == OFXOSC_TYPE_STRING) {
            std::string bankName = m.getArgAsString(0);
            for (size_t i = 0; i < gui->bankNames.size(); i++) {
                if (gui->bankNames[i] == bankName) {
                    gui->switchSaveBank(i);
                    break;
                }
            }
        }
    }
    // Bank switching commands - Load Bank
    else if (address == "/gravity/preset/loadBank/index") {
        if (m.getNumArgs() > 0) {
            int bankIndex = static_cast<int>(m.getArgAsFloat(0));
            gui->switchLoadBank(bankIndex);
        }
    }
    else if (address == "/gravity/preset/loadBank/name") {
        if (m.getNumArgs() > 0 && m.getArgType(0) == OFXOSC_TYPE_STRING) {
            std::string bankName = m.getArgAsString(0);
            for (size_t i = 0; i < gui->bankNames.size(); i++) {
                if (gui->bankNames[i] == bankName) {
                    gui->switchLoadBank(i);
                    break;
                }
            }
        }
    }
    // Save preset with custom name
    else if (address == "/gravity/preset/saveAs") {
        if (m.getNumArgs() > 0 && m.getArgType(0) == OFXOSC_TYPE_STRING) {
            std::string presetName = m.getArgAsString(0);
            gui->saveALL = 1;  // Populate saveBuffer
            gui->savePresetAs(presetName);
        }
    }
    // UI Scale control
    else if (address == "/gravity/ui/scale") {
        if (m.getNumArgs() > 0) {
            int scaleIndex = static_cast<int>(m.getArgAsFloat(0));
            if (scaleIndex >= 0 && scaleIndex <= 2) {
                gui->uiScaleIndex = scaleIndex;
            }
        }
    }
    else return false;
    return true;
}

//--------------------------------------------------------------
void ofApp::sendOscParameter(string address, float value) {
    if (!oscEnabled || !gui->oscEnabled) return;

    ofxOscMessage m;
    m.setAddress(address);
    m.addFloatArg(value);
    oscSender.sendMessage(m, true);
}
//--------------------------------------------------------------
void ofApp::sendOscString(string address, string value) {
    if (!oscEnabled || !gui->oscEnabled) return;

    ofxOscMessage m;
    m.setAddress(address);
    m.addStringArg(value);
    oscSender.sendMessage(m, true);
}
//--------------------------------------------------------------
void ofApp::reloadOscSettings() {
    oscReceiver.stop();
    oscSender.clear();
    setupOsc();
    ofLogNotice("OSC") << "OSC settings reloaded";
}
//--------------------------------------------------------------
//--------------------------------------------------------------
// OSC SEND HELPER FUNCTIONS - Registry-based
//--------------------------------------------------------------
void ofApp::sendOscParametersByPrefix(const std::string& prefix) {
    for (const auto& param : gui->oscRegistry) {
        if (param.address.find(prefix) == 0) {
            sendOscParameter(param.address, param.getValueAsFloat());
        }
    }
}

//--------------------------------------------------------------
void ofApp::sendOscBlock1Ch1() {
    sendOscParametersByPrefix("/gravity/block1/ch1");
}

//--------------------------------------------------------------
void ofApp::sendOscBlock1Ch2() {
    sendOscParametersByPrefix("/gravity/block1/ch2");
}

//--------------------------------------------------------------
void ofApp::sendOscBlock1Fb1() {
    sendOscParametersByPrefix("/gravity/block1/fb1");
}

//--------------------------------------------------------------
void ofApp::sendOscBlock2Fb2() {
    sendOscParametersByPrefix("/gravity/block2/fb2");
}

//--------------------------------------------------------------
void ofApp::sendOscBlock2Input() {
    sendOscParametersByPrefix("/gravity/block2/input");
}

//--------------------------------------------------------------
void ofApp::sendOscBlock3B1() {
    sendOscParametersByPrefix("/gravity/block3/b1");
    sendOscParametersByPrefix("/gravity/block3/lfo/b1");
}

//--------------------------------------------------------------
void ofApp::sendOscBlock3B2() {
    sendOscParametersByPrefix("/gravity/block3/b2");
    sendOscParametersByPrefix("/gravity/block3/lfo/b2");
}

//--------------------------------------------------------------
void ofApp::sendOscBlock3MatrixAndFinal() {
    sendOscParametersByPrefix("/gravity/block3/final");
    sendOscParametersByPrefix("/gravity/block3/lfo/final");
    sendOscParametersByPrefix("/gravity/block3/matrixMix");
    sendOscParametersByPrefix("/gravity/block3/lfo/matrixMix");
}

//--------------------------------------------------------------
void ofApp::sendAllOscParameters() {
    if (!oscEnabled || !gui->oscEnabled) return;

    // Pause receiving while sending to avoid feedback loops
    gui->oscReceivePaused = true;

    ofLogNotice("OSC") << "Sending all OSC parameters from registry (" << gui->oscRegistry.size() << " total)...";

    // Send all parameters from the registry
    for (const auto& param : gui->oscRegistry) {
        if (!param.address.empty()) {
            sendOscParameter(param.address, param.getValueAsFloat());
        }
    }

    // Resume receiving
    gui->oscReceivePaused = false;

    ofLogNotice("OSC") << "Finished sending all OSC parameters";
}
