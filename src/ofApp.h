#pragma once

#include "ofMain.h"
#include "GuiApp.h"
#include "ofxOsc.h"
#include "ofxNDIreceiver.h"
#include "ofxNDIsender.h"

#if defined(TARGET_WIN32)
#include "ofxSpout.h"
#define OFAPP_HAS_SPOUT 1
#else
#define OFAPP_HAS_SPOUT 0
#endif

#define ROOT_THREE 1.73205080757

class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed(int key);
		void keyReleased(int key);


		shared_ptr<GuiApp> gui;
		shared_ptr<ofAppBaseWindow> mainWindow;  // Reference to output window

		// OSC Communication
		ofxOscReceiver oscReceiver;
		ofxOscSender oscSender;
		void setupOsc();
		void processOscMessages();
		void sendOscParameter(string address, float value);
		void sendOscString(string address, string value);
		void sendAllOscParameters();
		void reloadOscSettings();
		bool oscEnabled;

		// OSC Send Helper Functions (registry-based)
		void sendOscParametersByPrefix(const std::string& prefix);
		void sendOscBlock1Ch1();
		void sendOscBlock1Ch2();
		void sendOscBlock1Fb1();
		void sendOscBlock2Fb2();
		void sendOscBlock2Input();
		void sendOscBlock3B1();
		void sendOscBlock3B2();
		void sendOscBlock3MatrixAndFinal();

		// OSC Process Helper Functions (split to avoid compiler limits)
		bool processOscBlock1(const string& address, float value, const ofxOscMessage& m);
		bool processOscBlock2(const string& address, float value, const ofxOscMessage& m);
		bool processOscBlock3(const string& address, float value, const ofxOscMessage& m);
		bool processOscDiscreteParams(const string& address, const ofxOscMessage& m);
		bool processOscBooleanParams(const string& address, const ofxOscMessage& m);
		bool processOscLfoParamsFb(const string& address, float value);
		bool processOscLfoParamsBlock3B1(const string& address, float value);
		bool processOscLfoParamsBlock3B2AndMatrix(const string& address, float value);
		bool processOscResetCommands(const string& address);
		bool processOscPresetCommands(const string& address, const ofxOscMessage& m);

	//globals
	// Input resolutions
	int input1Width = 640;
	int input1Height = 480;
	int input2Width = 640;
	int input2Height = 480;

	// Legacy inputWidth/inputHeight for compatibility
	int inputWidth=640;
	int inputHeight=480;

	// Output and internal resolutions
	int outputWidth;
	int outputHeight;
	int internalWidth;
	int internalHeight;

#if OFAPP_HAS_SPOUT
	// Spout send resolution
	int spoutSendWidth = 1280;
	int spoutSendHeight = 720;
#endif

	//video inputs
	void inputSetup();
	void inputUpdate();
	void inputTest();
	void reinitializeInputs();
	ofVideoGrabber input1;
	ofVideoGrabber input2;
	ofFbo webcamFbo1;  // FBO for scaling webcam 1 to internal resolution
	ofFbo webcamFbo2;  // FBO for scaling webcam 2 to internal resolution

	// NDI receivers
	ofxNDIreceiver ndiReceiver1;
	ofxNDIreceiver ndiReceiver2;
	ofTexture ndiTexture1;
	ofTexture ndiTexture2;
	ofFbo ndiFbo1;  // FBO for scaling NDI input 1 to 640x480
	ofFbo ndiFbo2;  // FBO for scaling NDI input 2 to 640x480
	void refreshNdiSources();

#if OFAPP_HAS_SPOUT
	// Spout receivers
	ofxSpout::Receiver spoutReceiver1;
	ofxSpout::Receiver spoutReceiver2;
	ofTexture spoutTexture1;  // Texture to receive into
	ofTexture spoutTexture2;  // Texture to receive into
	ofFbo spoutFbo1;  // FBO for scaling Spout input 1 to 640x480
	ofFbo spoutFbo2;  // FBO for scaling Spout input 2 to 640x480

	// Spout senders (one per output channel)
	ofxSpout::Sender spoutSenderBlock1;
	ofxSpout::Sender spoutSenderBlock2;
	ofxSpout::Sender spoutSenderBlock3;
	ofFbo spoutSendFbo1;  // FBO for flipping Block 1 output
	ofFbo spoutSendFbo2;  // FBO for flipping Block 2 output
	ofFbo spoutSendFbo3;  // FBO for flipping Block 3 output
#endif

	// NDI senders (one per output channel)
	ofxNDIsender ndiSenderBlock1;
	ofxNDIsender ndiSenderBlock2;
	ofxNDIsender ndiSenderBlock3;
	bool ndiSender1Active = false;  // Track if sender is created
	bool ndiSender2Active = false;
	bool ndiSender3Active = false;
	ofFbo ndiSendFbo1;  // FBO for Block 1 output
	ofFbo ndiSendFbo2;  // FBO for Block 2 output
	ofFbo ndiSendFbo3;  // FBO for Block 3 output

	// NDI send resolution
	int ndiSendWidth = 1280;
	int ndiSendHeight = 720;

	// Spout source management (stub implementation exists on non-Windows)
	void refreshSpoutSources();

	//framebuffers
	void framebufferSetup();
	void reinitializeResolutions();
	ofFbo framebuffer1;
	ofFbo framebuffer2;
	ofFbo framebuffer3;

	//shaders
	ofShader shader1;
	ofShader shader2;
	ofShader shader3;


	//COEFFICIENTS
	//mix and key coefficients
	float mixAmountC=2.0;
	float keyC=1.0;//use for r g b value
	float keyThresholdC=ROOT_THREE+.001;

	//BLOCK 1

	//ch1 adjust coefficients
	float ch1XDisplaceC=-640.0;//make x negative for better intuitive controls
	float ch1YDisplaceC=480.0;
	float ch1ZDisplaceC=1.0;
	float ch1RotateC=PI;
	float ch1PosterizeC=15.0;
	float ch1KaleidoscopeAmountC=21;
	float ch1KaleidoscopeSliceC=PI;

	float ch1FilterRadiusC=9.0;
	float ch1SharpenAmountC=1.0;

	//ch2 adjust coefficients
	float ch2XDisplaceC=-640.0;//make x negative for better intuitive controls
	float ch2YDisplaceC=480.0;
	float ch2ZDisplaceC=1.0;
	float ch2RotateC=PI;
	float ch2PosterizeC=15.0;
	float ch2KaleidoscopeAmountC=21;
	float ch2KaleidoscopeSliceC=PI;
	float ch2FilterRadiusC=9.0;
	float ch2SharpenAmountC=1.0;

	//fb1 geo1 coefficiens
	float fb1XDisplaceC=-80;
	float fb1YDisplaceC=80;
	float fb1ZDisplaceC=.5;
	float fb1RotateC=PI;
	float fb1ShearMatrix1C=.25;
	float fb1ShearMatrix2C=-.25;
	float fb1ShearMatrix3C=.25;
	float fb1ShearMatrix4C=.25;
	float fb1KaleidoscopeAmountC=21;
	float fb1KaleidoscopeSliceC=PI;

	//fb1 color coefficients
	float fb1HueOffsetC=.25;
	float fb1SaturationOffsetC=.25;
	float fb1BrightOffsetC=.25;
	float fb1HueAttenuateC=.25;
	float fb1SaturationAttenuateC=.25;
	float fb1BrightAttenuateC=.25;
	float fb1HuePowmapC=.1;
	float fb1SaturationPowmapC=.1;
	float fb1BrightPowmapC=.1;
	float fb1HueShaperC=1.0;
	float fb1PosterizeC=15.0;

	//fb1 filters coefficients
	float fb1FilterRadiusC=9.0;
	float fb1SharpenAmountC=.6;
	float fb1TemporalFilterAmountC=2.0;


	//BLOCK 2

	//block2Input adjust coefficients
	float block2InputXDisplaceC=-640.0;//make x negative for better intuitive controls
	float block2InputYDisplaceC=480.0;
	float block2InputZDisplaceC=1.0;
	float block2InputRotateC=PI;
	float block2InputPosterizeC=15.0;
	float block2InputKaleidoscopeAmountC=21;
	float block2InputKaleidoscopeSliceC=PI;
	float block2InputFilterRadiusC=9.0;
	float block2InputSharpenAmountC=1.0;

	//fb2 geo1 coefficiens
	float fb2XDisplaceC=-80;
	float fb2YDisplaceC=80;
	float fb2ZDisplaceC=.5;
	float fb2RotateC=PI;
	float fb2ShearMatrix1C=.25;
	float fb2ShearMatrix2C=-.25;
	float fb2ShearMatrix3C=.25;
	float fb2ShearMatrix4C=.25;
	float fb2KaleidoscopeAmountC=21;
	float fb2KaleidoscopeSliceC=PI;

	//fb2 color coefficients
	float fb2HueOffsetC=.25;
	float fb2SaturationOffsetC=.25;
	float fb2BrightOffsetC=.25;
	float fb2HueAttenuateC=.25;
	float fb2SaturationAttenuateC=.25;
	float fb2BrightAttenuateC=.25;
	float fb2HuePowmapC=.1;
	float fb2SaturationPowmapC=.1;
	float fb2BrightPowmapC=.1;
	float fb2HueShaperC=1.0;
	float fb2PosterizeC=15.0;

	//fb2 filters coefficients
	float fb2FilterRadiusC=9.0;
	float fb2SharpenAmountC=.6;
	float fb2TemporalFilterAmountC=2.0;

	//BLOCK3

	//block1 geo1 coefficiens
	float block1XDisplaceC=-1280;
	float block1YDisplaceC=720;
	float block1ZDisplaceC=1.0;
	float block1RotateC=PI;
	float block1ShearMatrix1C=1;
	float block1ShearMatrix2C=-1;
	float block1ShearMatrix3C=1;
	float block1ShearMatrix4C=1;
	float block1KaleidoscopeAmountC=21;
	float block1KaleidoscopeSliceC=PI;

	//block1 filters coefficients
	float block1FilterRadiusC=9.0;
	float block1SharpenAmountC=1.0;
	float block1TemporalFilterAmountC=2.0;
	float block1DitherC=15.0;

	//block2 geo1 coefficiens
	float block2XDisplaceC=-1280;
	float block2YDisplaceC=720;
	float block2ZDisplaceC=1.0;
	float block2RotateC=PI;
	float block2ShearMatrix1C=1;
	float block2ShearMatrix2C=-1;
	float block2ShearMatrix3C=1;
	float block2ShearMatrix4C=1;
	float block2KaleidoscopeAmountC=21;
	float block2KaleidoscopeSliceC=PI;

	//block2 filters coefficients
	float block2FilterRadiusC=9.0;
	float block2SharpenAmountC=1.0;
	float block2TemporalFilterAmountC=2.0;
	float block2DitherC=15.0;

	//mix coefficients
	float matrixMixC=6.0;



	//lfos
	float lfo(float amp, float rate,int shape);
	void lfoUpdate();
	float lfoRateC=.15;

	//BLOCK 1
	float ch1XDisplaceTheta=0;
	float ch1YDisplaceTheta=0;
	float ch1ZDisplaceTheta=0;
	float ch1RotateTheta=0;
	float ch1HueAttenuateTheta=0;
	float ch1SaturationAttenuateTheta=0;
	float ch1BrightAttenuateTheta=0;
	float ch1KaleidoscopeSliceTheta=0;

	float ch2MixAmountTheta=0;
	float ch2KeyThresholdTheta=0;
	float ch2KeySoftTheta=0;

	float ch2XDisplaceTheta=0;
	float ch2YDisplaceTheta=0;
	float ch2ZDisplaceTheta=0;
	float ch2RotateTheta=0;
	float ch2HueAttenuateTheta=0;
	float ch2SaturationAttenuateTheta=0;
	float ch2BrightAttenuateTheta=0;
	float ch2KaleidoscopeSliceTheta=0;

	float fb1MixAmountTheta=0;
	float fb1KeyThresholdTheta=0;
	float fb1KeySoftTheta=0;

	float fb1XDisplaceTheta=0;
	float fb1YDisplaceTheta=0;
	float fb1ZDisplaceTheta=0;
	float fb1RotateTheta=0;

	float fb1ShearMatrix1Theta=0;
	float fb1ShearMatrix2Theta=0;
	float fb1ShearMatrix3Theta=0;
	float fb1ShearMatrix4Theta=0;
	float fb1KaleidoscopeSliceTheta=0;

	float fb1HueAttenuateTheta=0;
	float fb1SaturationAttenuateTheta=0;
	float fb1BrightAttenuateTheta=0;


	//BLOCK 2
	float block2InputXDisplaceTheta=0;
	float block2InputYDisplaceTheta=0;
	float block2InputZDisplaceTheta=0;
	float block2InputRotateTheta=0;
	float block2InputHueAttenuateTheta=0;
	float block2InputSaturationAttenuateTheta=0;
	float block2InputBrightAttenuateTheta=0;
	float block2InputKaleidoscopeSliceTheta=0;

	float fb2MixAmountTheta=0;
	float fb2KeyThresholdTheta=0;
	float fb2KeySoftTheta=0;

	float fb2XDisplaceTheta=0;
	float fb2YDisplaceTheta=0;
	float fb2ZDisplaceTheta=0;
	float fb2RotateTheta=0;

	float fb2ShearMatrix1Theta=0;
	float fb2ShearMatrix2Theta=0;
	float fb2ShearMatrix3Theta=0;
	float fb2ShearMatrix4Theta=0;
	float fb2KaleidoscopeSliceTheta=0;

	float fb2HueAttenuateTheta=0;
	float fb2SaturationAttenuateTheta=0;
	float fb2BrightAttenuateTheta=0;

    //BLOCK 3

	//block1 geo
	float block1XDisplaceTheta=0;
	float block1YDisplaceTheta=0;
	float block1ZDisplaceTheta=0;
	float block1RotateTheta=0;

	float block1ShearMatrix1Theta=0;
	float block1ShearMatrix2Theta=0;
	float block1ShearMatrix3Theta=0;
	float block1ShearMatrix4Theta=0;
	float block1KaleidoscopeSliceTheta=0;

	//block1 colorize
	float block1ColorizeHueBand1Theta=0;
	float block1ColorizeSaturationBand1Theta=0;
	float block1ColorizeBrightBand1Theta=0;
	float block1ColorizeHueBand2Theta=0;
	float block1ColorizeSaturationBand2Theta=0;
	float block1ColorizeBrightBand2Theta=0;
	float block1ColorizeHueBand3Theta=0;
	float block1ColorizeSaturationBand3Theta=0;
	float block1ColorizeBrightBand3Theta=0;
	float block1ColorizeHueBand4Theta=0;
	float block1ColorizeSaturationBand4Theta=0;
	float block1ColorizeBrightBand4Theta=0;
	float block1ColorizeHueBand5Theta=0;
	float block1ColorizeSaturationBand5Theta=0;
	float block1ColorizeBrightBand5Theta=0;

	//block2 geo
	float block2XDisplaceTheta=0;
	float block2YDisplaceTheta=0;
	float block2ZDisplaceTheta=0;
	float block2RotateTheta=0;

	float block2ShearMatrix1Theta=0;
	float block2ShearMatrix2Theta=0;
	float block2ShearMatrix3Theta=0;
	float block2ShearMatrix4Theta=0;
	float block2KaleidoscopeSliceTheta=0;

	//block2 colorize
	float block2ColorizeHueBand1Theta=0;
	float block2ColorizeSaturationBand1Theta=0;
	float block2ColorizeBrightBand1Theta=0;
	float block2ColorizeHueBand2Theta=0;
	float block2ColorizeSaturationBand2Theta=0;
	float block2ColorizeBrightBand2Theta=0;
	float block2ColorizeHueBand3Theta=0;
	float block2ColorizeSaturationBand3Theta=0;
	float block2ColorizeBrightBand3Theta=0;
	float block2ColorizeHueBand4Theta=0;
	float block2ColorizeSaturationBand4Theta=0;
	float block2ColorizeBrightBand4Theta=0;
	float block2ColorizeHueBand5Theta=0;
	float block2ColorizeSaturationBand5Theta=0;
	float block2ColorizeBrightBand5Theta=0;


	//matrix mix
	float matrixMixBgRedIntoFgRedTheta=0;
	float matrixMixBgGreenIntoFgRedTheta=0;
	float matrixMixBgBlueIntoFgRedTheta=0;

	float matrixMixBgRedIntoFgGreenTheta=0;
	float matrixMixBgGreenIntoFgGreenTheta=0;
	float matrixMixBgBlueIntoFgGreenTheta=0;

	float matrixMixBgRedIntoFgBlueTheta=0;
	float matrixMixBgGreenIntoFgBlueTheta=0;
	float matrixMixBgBlueIntoFgBlueTheta=0;

	//final mix
	float finalMixAmountTheta=0;
	float finalKeyThresholdTheta=0;
	float finalKeySoftTheta=0;

	//hypercube
	void hypercube_draw();

    float hypercube_theta=0;
    float hypercube_phi=0;
    float hypercube_r=0.0;

    float hypercube_x[8];
    float hypercube_y[8];
    float hypercube_z[8];

    float hypercube_color_theta=0;

    //line
    void line_draw();
    float line_theta=0;
    float line_phi=0;
    float line_eta=0;

	//sevenStar
	void sevenStar1Setup();
	void sevenStar1Draw();

	//try to add this to .h
	//sevenstar1 biz
	static const int reps = 7;
	ofVec2f points[reps];
	//i don't think this would be every time? but should double check
	//this one increments 3 = floor[7/2] to 8 reps
	static const int reps1 = reps + 1;
	ofVec2f points1[reps1];

	ofVec2f position1;
	float increment1 = 0;
	int index1 = 0;

	float acceleration1 = .002;
	float threshold = .125;

	//this one increments 2, will work for any odd star
	ofVec2f points2[reps];
	ofVec2f position2;
	float increment2 = 0;
	int index2 = 0;

	float acceleration2 = .00125;

	float thetaHue1;
	float thetaHue2;
	float thetaSaturation1;
	float thetaChaos;

	float hueInc1 = .021257;
	float hueInc2 = .083713;
	float saturationInc1 = .00612374;
	float chaosInc = .0001;

	//lissaBall
	void drawSpiralEllipse();

	float spiralTheta1 = 0;
	float spiralRadius1 = 0;

	float radius1Inc = .75;
	float spiralTheta1Inc = .07;

	float spiralTheta2 = 0;
	float spiralRadius2 = 0;

	float radius2Inc = .55;
	float spiralTheta2Inc = .08;

	float spiralTheta3 = 0;
	float spiralRadius3 = 0;

	float radius3Inc = .65;
	float spiralTheta3Inc = .05;

	// ============== LISSAJOUS CURVE GENERATOR ==============
	void lissajousCurve1Draw();
	void lissajousCurve2Draw();
	float lissajousWave(float theta, int shape);

	// Animation thetas
	float lissajous1Theta = 0;
	float lissajous2Theta = 0;
	float lissajous1ColorTheta = 0;
	float lissajous2ColorTheta = 0;

	// Block 1 LFO thetas (16)
	float lissajous1XFreqLfoTheta = 0;
	float lissajous1YFreqLfoTheta = 0;
	float lissajous1ZFreqLfoTheta = 0;
	float lissajous1XAmpLfoTheta = 0;
	float lissajous1YAmpLfoTheta = 0;
	float lissajous1ZAmpLfoTheta = 0;
	float lissajous1XPhaseLfoTheta = 0;
	float lissajous1YPhaseLfoTheta = 0;
	float lissajous1ZPhaseLfoTheta = 0;
	float lissajous1XOffsetLfoTheta = 0;
	float lissajous1YOffsetLfoTheta = 0;
	float lissajous1SpeedLfoTheta = 0;
	float lissajous1SizeLfoTheta = 0;
	float lissajous1NumPointsLfoTheta = 0;
	float lissajous1LineWidthLfoTheta = 0;
	float lissajous1ColorSpeedLfoTheta = 0;
	float lissajous1HueLfoTheta = 0;
	float lissajous1HueSpreadLfoTheta = 0;

	// Block 2 LFO thetas (16)
	float lissajous2XFreqLfoTheta = 0;
	float lissajous2YFreqLfoTheta = 0;
	float lissajous2ZFreqLfoTheta = 0;
	float lissajous2XAmpLfoTheta = 0;
	float lissajous2YAmpLfoTheta = 0;
	float lissajous2ZAmpLfoTheta = 0;
	float lissajous2XPhaseLfoTheta = 0;
	float lissajous2YPhaseLfoTheta = 0;
	float lissajous2ZPhaseLfoTheta = 0;
	float lissajous2XOffsetLfoTheta = 0;
	float lissajous2YOffsetLfoTheta = 0;
	float lissajous2SpeedLfoTheta = 0;
	float lissajous2SizeLfoTheta = 0;
	float lissajous2NumPointsLfoTheta = 0;
	float lissajous2LineWidthLfoTheta = 0;
	float lissajous2ColorSpeedLfoTheta = 0;
	float lissajous2HueLfoTheta = 0;
	float lissajous2HueSpreadLfoTheta = 0;
};
