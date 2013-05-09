#include "testApp.h"

using namespace ofxCv;
using namespace cv;

//--------------------------------------------------------------
void testApp::setup(){

	ofSetVerticalSync(true);
	ofSetFrameRate(630);

	classifier.load(ofToDataPath("haarcascade_frontalface_alt2.xml"));
	
	scaleFactor = .125;
	cam.initGrabber(640, 480);

	// shouldn't need to allocate, resize should do this for us
	graySmall.allocate(cam.getWidth() * scaleFactor, cam.getHeight() * scaleFactor, OF_IMAGE_GRAYSCALE);

	ofBackground(28, 46, 52);

	ard.connect("COM2", 57600);
	ofAddListener(ard.EInitialized, this, &testApp::setupArduino);
	bSetupArduino	= false;

	mirrorActive = false;
	idle = true;
	faceTimer = false;
	servoTimer = false;
	mirrorTimer = false;

	personality = "";
	perShy = false;
	perFrantic = false;
	perIgnore = false;

	moveShyDone = false;

	bHor = 90;		//horizontal servo
	bVer = 90;		//vertical servo
	horMax = 50;	//+ - horizontal range
	verMax = 50;	//+ - vertical range 

	horLeft = 90 + horMax;		//hor. maximum rotational degree
	horRight = 90 - horMax;		//hor minimum rotationdal degree
	verUp = 90 + verMax;		//ver. maximum rotational degree
	verDown = 90 - verMax;		//ver. minimum rotational degree


	// must call this before adding any tweens //
	Tweenzor::init();
	Tweenzor::add(&bHor, 90, 130, 1.f, 2.f);
	
	//Tweenzor::getTween( &bHor )->setRepeat( 1, true );	// the second argument (true) is a ping pong parameter
	// let's add a listener so we know when this tween is done //
	Tweenzor::addCompleteListener( Tweenzor::getTween(&bHor), this, &testApp::onCompleteIdle);


	font.loadFont("franklinGothic.otf", 20);
    smallFont.loadFont("franklinGothic.otf", 12);

		gui = new ofxUICanvas(640, 0, 200, ofGetHeight());

    gui->addLabel("THE WATCHER");
    gui->addSpacer(250, 1); 
	gui->addLabel("INDICATORS");
	gui->addToggle("Arduino Connected", &bSetupArduino, 16, 16);
	//gui->addToggle("Mirror Active", &mirrorActive, 16, 16);	
	//gui->addToggle("Face Detected", &faceFound, 16, 16);
	gui->addSpacer(250, 1);
	gui->addLabel("PERSONALITY");
	gui->addToggle("Idle", &idle, 16, 16);
	gui->addToggle("Shy", &perShy, 16, 16);
	gui->addToggle("Frantic", &perFrantic, 16, 16);
	gui->addToggle("Ignoring", &perIgnore, 16, 16);
	gui->addSpacer(250, 1); 
	//gui->addLabel("SETTINGS");
	//gui->addLabelButton("Cam Settings", false);		//cam.videoSettings();
	//gui->addSpacer(250, 1); 
	//gui->addSpacer(250, 1); 
	//gui->addLabel("DATA");
	gui->addSlider("HORIZONTAL SERVO", horLeft, horRight, &bHor);
	gui->addSlider("VERTICAL SERVO", verDown, verUp, &bVer);
	gui->addSpacer(250, 1); 
    gui->autoSizeToFitWidgets(); 
	ofAddListener(gui->newGUIEvent, this, &testApp::guiEvent); 
	gui->loadSettings("GUI/guiSettings.xml");			//load xml settings for gui
}

//--------------------------------------------------------------
void testApp::update(){
	cam.update();
	if(cam.isFrameNew()) {
		convertColor(cam, gray, CV_RGB2GRAY);
		resize(gray, graySmall);
		Mat graySmallMat = toCv(graySmall);
		
		graySmall.update();

		classifier.detectMultiScale(graySmallMat, objects, 1.06, 1,
			//CascadeClassifier::DO_CANNY_PRUNING |
			//CascadeClassifier::FIND_BIGGEST_OBJECT |
			//CascadeClassifier::DO_ROUGH_SEARCH |
			0);
	}

	mainFace = 0;
	int mainFaceArea = 0;
	for(int i = 0; i < objects.size(); i++) {
		if( objects[i].area() > mainFaceArea ) {
			mainFaceArea = objects[i].area();
			mainFace = i;
		}
	}
	
	if( !mirrorActive ) {
		if( objects.size() > 0 && mainFaceArea > 750 ) {
			//timer
			if(!faceTimer) {
				faceTimer = true;
				faceTimerStart = ofGetElapsedTimeMillis();
			} else {
				if(  (ofGetElapsedTimeMillis() - faceTimerStart) >= 500 ) {
					cout << "face found!" << endl;
					faceTimer = false;
					idle = false;
					mirrorActive = true;
					Tweenzor::removeAllTweens();

					pers = rand() % 3;
					//pers = 2;
					if(pers == 0) {
						personality = "shy";
						moveShyDone = false;
						perShy = true;

						moveShy();

					} else if(pers == 1) {
						personality = "frantic";
						moveFranticDone = false;
						perFrantic = true;

						moveFrantic();

					} else if(pers == 2) {
						personality = "ignore";		
						moveIgnoreDone = false;
						perIgnore = true;

						moveIgnore();
					}
					//Tweenzor::add(&bVer, 90, 95, 0.f, 0.5f);
					//Tweenzor::add(&bVer, 95, 90, 0.5f, 0.5f);
				}
			}
		} else if( objects.size() == 0) {
			faceTimer = false;
		}
	} else {
		if(objects.size() > 0) {
			if(personality == "shy" && moveShyDone) {
				moveShy();
			} else if(personality == "frantic" && moveFranticDone) {
				moveFrantic();
			} else if(personality == "ignore" && moveIgnoreDone) {
				moveIgnore();
			}
		}

		if( objects.size() == 0 || mainFaceArea < 430 ) {
			if( !mirrorTimer ){
				mirrorTimerStart = ofGetElapsedTimeMillis();	
				mirrorTimer = true;
			} else  {
				if( (ofGetElapsedTimeMillis() - mirrorTimerStart) >= 10000 ){		//buffer time exceeded

					cout << "Mirror deactivated"<<endl;
					mirrorTimer = false;
					mirrorActive = false;
					idle = true;
					perShy = false;
					perFrantic = false;
					perIgnore = false;

					resetServo();

					Tweenzor::add(&bHor, 90, 130, 1.f, 2.f);
					//Tweenzor::getTween( &bHor )->setRepeat( 1, true );	// the second argument (true) is a ping pong parameter
					Tweenzor::addCompleteListener( Tweenzor::getTween(&bHor), this, &testApp::onCompleteIdle); // let's add a listener so we know when this tween is done //
				}
			}
		} else {
			mirrorTimer = false;
		}
	}

	Tweenzor::update( ofGetElapsedTimeMillis() );

	updateArduino();
}

//--------------------------------------------------------------
void testApp::draw(){

	ofSetColor(255);
	cam.draw(0, 0);

	ofNoFill();

	ofScale(1 / scaleFactor, 1 / scaleFactor);	//scale cam image up
	if( objects.size() > 0 ) {
		ofRect(toOf(objects[mainFace]));
		ofDrawBitmapString(ofToString(objects[mainFace].area()), objects[mainFace].x, objects[mainFace].y);
	}
	ofDrawBitmapString(ofToString(objects.size()), 10, 20);

	ofScale(scaleFactor, scaleFactor);	//scale everything back

	/**Data**/
	float paddingLeft = 645.0;
	float paddingTop = 310;
	
	ofFill();
	ofSetColor(255,255,255);
	//if( mirrorTimer ){
	//	smallFont.drawString("Buffer timer: " + ofToString(ofGetElapsedTimeMillis() - mirrorTimerStart) + " out of "+ ofToString(mirrorDelay), paddingLeft, paddingTop); 
	//} else {
	//	smallFont.drawString("Buffer timer: 0 out of "+ ofToString(mirrorDelay), paddingLeft,paddingTop); 
	//}
	//smallFont.drawString("Head scale: " + ofToString(headScale), paddingLeft, paddingTop+20);
	//smallFont.drawString("X angle: " + ofToString(anglePosX), paddingLeft, paddingTop+40);
	//smallFont.drawString("Y angle: " + ofToString(anglePosY), paddingLeft+120, paddingTop+40);
	//smallFont.drawString("H angle: " + ofToString(bHor), paddingLeft, paddingTop+60);
	//smallFont.drawString("V angle: " + ofToString(bVer), paddingLeft+120, paddingTop+60);
}

//--------------------------------------------------------------
// this function is called on when the tween is complete //
void testApp::onCompleteIdle(float* arg) {

	float _tarX = 0.f;
	float _begin = 0.f;
	
	// the arg argument is a pointer to the variable passed in when calling Tweenzor::add();
	// if you want to check its value, you must de-reference it by calling *arg
	if (*arg > 100) {
		_tarX = 50;
		_begin = 130;
	} else {
		_tarX = 130;
		_begin = 50;
	}
	
	
	Tweenzor::add( &bHor, _begin, _tarX, 0.f, 4.f );
	
	// add the complete listener again so that it will fire again, creating a loop //
	Tweenzor::addCompleteListener( Tweenzor::getTween(&bHor), this, &testApp::onCompleteIdle);

	
}

//--------------------------------------------------------------
void testApp::moveShy(){
	cout << "The mirror is shy!" << endl;
	moveShyDone = false;

	int _bHorNew;
	int _bVerNew;
	if( bHor >= 90 ){
		_bHorNew = rand() % 21 + 40;		//40-60
			
	} else if( bHor < 90 ){
		_bHorNew = rand() % 21 + 120;		//120-140; it's reversed for the horizontal servo
	} 
	_bVerNew = rand() % 21 + 85;			//90-110

	Tweenzor::add(&bHor, bHor, _bHorNew, 1.f, 2.5f, EASE_IN_CUBIC);
	Tweenzor::add(&bVer, bVer, _bVerNew, 1.f, 2.5f, EASE_IN_CUBIC);
	Tweenzor::addCompleteListener( Tweenzor::getTween(&bHor), this, &testApp::onCompleteShy);
}

//--------------------------------------------------------------
void testApp::onCompleteShy(float* arg){
	cout << "Shy: Move Done!" << endl;
	moveShyDone = true;
}

//--------------------------------------------------------------
void testApp::moveFrantic(){
	cout << "The mirror is frantic!" << endl;
	moveFranticDone = false;

		int _rand = rand() % 2;
	int _bHorNew;
	//int _bVerNew;
	if( bHor >= 140 ){
		_bHorNew = bHor - 30;
	} else if( bHor <= 50 ){
		_bHorNew = bHor + 30;
	} else {
		if(_rand == 0) {
			_bHorNew = bHor + 10;
		} else {
			_bHorNew = bHor - 10;
		}
	}
	
	int _bVerNew = rand() % 21 + 90;			//50-80

	Tweenzor::add(&bHor, bHor, _bHorNew, 0.f, .1f);
	Tweenzor::add(&bVer, bVer, _bVerNew, 0.5f, 0.25f);
	Tweenzor::addCompleteListener( Tweenzor::getTween(&bHor), this, &testApp::onCompleteFrantic);
}

//--------------------------------------------------------------
void testApp::onCompleteFrantic(float* arg){
	cout << "Frantic: Move Done!" << endl;
	moveFranticDone = true;
}

//--------------------------------------------------------------
void testApp::moveIgnore(){
	cout << "The mirror is ignoring you!" << endl;
	moveIgnoreDone = false;

	int _bHorNew;
	int _bVerNew;
	if( bHor >= 90 ){
		_bHorNew = rand() % 21 + 40;		//40-60
			
	} else if( bHor < 90 ){
		_bHorNew = rand() % 21 + 120;		//120-140; it's reversed for the horizontal servo
	} 
	_bVerNew = rand() % 36 + 90;			//90-125

	Tweenzor::add(&bHor, bHor, _bHorNew, 0.5f, 0.5f);	//tweens won't be noticable
	Tweenzor::add(&bVer, bVer, _bVerNew, 0.5f, 0.5f);
	Tweenzor::addCompleteListener( Tweenzor::getTween(&bHor), this, &testApp::onCompleteIgnore);
}

//--------------------------------------------------------------
void testApp::onCompleteIgnore(float* arg){
	cout << "Ignore: Move Done!" << endl;
	moveIgnoreDone = true;
}

//--------------------------------------------------------------
void testApp::setupArduino(const int & version){

	// remove listener because we don't need it anymore
	ofRemoveListener(ard.EInitialized, this, &testApp::setupArduino);
    
    // it is now safe to send commands to the Arduino
    bSetupArduino = true;

	//print firmware name and version to console
	cout << ard.getFirmwareName() << endl; 
    cout << "firmata v" << ard.getMajorFirmwareVersion() << "." << ard.getMinorFirmwareVersion() << endl;

	//Note: pin A0~A5 = pin 14~19
	ard.sendDigitalPinMode(13, ARD_OUTPUT);		//D13 as digital output

	ard.sendServoAttach(9);		//D9 as servo pin
	ard.sendServoAttach(10);	//D10 as servo pin
	resetServo();

}

//--------------------------------------------------------------
void testApp::updateArduino(){
	ard.update();

	if(bSetupArduino) {
		//if(idle) { 
		//	ard.sendServo(9, bHor);
		//}
		
		//if(mirrorActive){
			ard.sendServo(9, bHor);
			ard.sendServo(10, bVer);
		//}
	}
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){

}

//--------------------------------------------------------------
void testApp::keyReleased(int key){

}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}

//--------------------------------------------------------------
void testApp::resetServo(){

	//int _bHorNew = 90;
	//int _bVerNew = 98;
	bHor = 90;
	bVer = 98;
	//Tweenzor::add(&bHor, bHor, _bHorNew, 0.5f, 0.25f);	//tweens won't be noticable
	//Tweenzor::add(&bVer, bVer, _bVerNew, 0.5f, 0.25f);
	ard.sendServo(9, bHor);		//reset to default 90 degrees on horizontal servo
	ard.sendServo(10, bVer);	//reset to default 90 degrees on vertical servo

	//servoTimer = false;

	cout << "Servos reset to default" << endl;
}

//--------------------------------------------------------------
void testApp::exit(){
   // gui->saveSettings("GUI/guiSettings.xml");
	Tweenzor::destroy();
	resetServo();
    delete gui; 
}

//--------------------------------------------------------------
void testApp::guiEvent(ofxUIEventArgs &e){

	string name = e.widget->getName();

	if(name == "HORIZONTAL SERVO"){
		ofxUISlider *slider = (ofxUISlider *) e.widget;   
		ard.sendServo(9, (int)slider->getScaledValue(), false);
	} else if(name == "VERTICAL SERVO"){
		ofxUISlider *slider = (ofxUISlider *) e.widget;   
		ard.sendServo(10, (int)slider->getScaledValue(), false);
	} 
}
//--------------------------------------------------------------