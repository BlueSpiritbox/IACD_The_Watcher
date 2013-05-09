#pragma once

#include "ofMain.h"
#include "ofxCv.h"
#include "ofEvents.h"
#include "ofxUI.h"
#include "ofxTweenzor.h"

class testApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();

		void keyPressed  (int key);
		void keyReleased(int key);

		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		
		ofVideoGrabber cam;
		ofImage gray, graySmall;

		cv::CascadeClassifier classifier;
		vector<cv::Rect> objects;
		float scaleFactor;

		int mainFace;
		bool mirrorActive;

		ofTrueTypeFont font;
		ofTrueTypeFont smallFont;

		ofArduino ard;
		bool bSetupArduino;

		bool idle;
		bool faceTimer;
		float faceTimerStart;

		bool servoTimer;
		float servoTimerStart;
		
		bool mirrorTimer;
		float mirrorTimerStart;

		int pers;
		string personality;
		
		bool perShy;
		bool perFrantic;
		bool perIgnore;

		void onCompleteIdle(float* arg);

		void moveShy();
		void onCompleteShy(float* arg);
		bool moveShyDone;

		void moveFrantic();
		void onCompleteFrantic(float* arg);
		bool moveFranticDone;

		void moveIgnore();
		void onCompleteIgnore(float* arg);
		bool moveIgnoreDone;
		
		ofxUICanvas *gui;
		void exit();
		void guiEvent(ofxUIEventArgs &e);

		float bHor;		//horizontal servo value
		float bVer;		//vertical servo value
		int horMax;
		int verMax;

		int horLeft;
		int horRight;
		int verUp;
		int verDown;


	private:
		void setupArduino(const int & version);
		void digitalPinChanged(const int & pinNum);
		void analogPinChanged(const int & pinNum);
		void updateArduino();
		void resetServo();

};
