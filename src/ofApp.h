#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include  "ofxAssimpModelLoader.h"
#include "Octree.h"
#include "ParticleCustom.h"
#include <glm/gtx/intersect.hpp>
#include <glm/glm.hpp>

class ofApp : public ofBaseApp {

public:
	void setup();
	void update();
	void draw();

	void keyPressed(int key);
	void keyReleased(int key);
	void mouseMoved(int x, int y);
	void mouseDragged(int x, int y, int button);
	void mousePressed(int x, int y, int button);
	void mouseReleased(int x, int y, int button);
	void mouseEntered(int x, int y);
	void mouseExited(int x, int y);
	void windowResized(int w, int h);
	void dragEvent(ofDragInfo dragInfo);
	void gotMessage(ofMessage msg);
	void drawAxis(ofVec3f);
	void initLightingAndMaterials();
	void savePicture();
	void toggleWireframeMode();
	bool mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f& point);
	glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 p, glm::vec3 n);


	ofxAssimpModelLoader mars, moon, mud;
	ofxAssimpModelLoader terrain; // current terrain
	Octree octreeMars, octreeMoon, octreeMud;
	Octree octree; // current terrain's octree
	int currentNumLevels;
	vector<Box> bboxList;

	// lander

	// Background image
	ofImage backgroundImage;

	// landing areas & score count
	int correctLanding, hardLanding, crashLanding; // # of each landings done
	int score;

	// gui
	ofxPanel gui;
	ofxIntSlider numLevels;
	ofxButton restartGame;
	ofParameter<bool> muteSound;
	ofParameter<bool> displayAltitude;
	ofxLabel fuelUsed;
	ofxFloatSlider hardnessScale;
	ofParameterGroup mapOptions;
	ofParameter<bool> marsMap, moonMap, mudMap;

	void restart();
	void switchMars(bool& val);
	void switchMoon(bool& val);
	void switchMud(bool& val);

	bool bRunGame;
	bool bPlayerInput;
	bool bAltKeyDown;
	bool bHide;
	bool bWireframe;
	bool bDisplayOctree = false;
	bool bDisplayBBoxes = false;

	ofVec3f intersectPoint;
	glm::vec3 mouseDownPos, mouseLastPos;
	bool bInDrag = false;


	// LANDER FUNCTIONS/OBJECTS
	//

	glm::vec3 heading();
	void integrate();
	void moveLander(int dir);
	void rotateLander(int dir);

	ofxAssimpModelLoader lander;
	Box boundingBox, landerBounds;
	ofLight LTerrain, LLander, LLander2;
	vector<Box> colBoxList;
	vector<int> colPoints;
	ofSoundPlayer thrustSound;
	bool bLanderLoaded;
	float startingY, gravity, bounceFactor, timeSinceLastBounce;
	bool bLanderSelected = false;
	bool bGrounded = false;
	bool explode = false;

	// altitude sensor
	TreeNode groundNode;
	float landerYOffset = 0;
	float altitude = 0;

	// parameters for movement
	glm::vec3 velocity = glm::vec3(0, 0, 0);
	glm::vec3 acceleration = glm::vec3(0, 0, 0);
	glm::vec3 force = glm::vec3(0, 0, 0);
	float mass = 1.0;
	float damping = .99;

	// parameters for angular (rotation) motion
	float rotation = 0.0; // degrees
	float angularVelocity = 0;
	float angularAcceleration = 0;
	float angularForce = 0;
	float angularDamping = .99;

	// Emitter
	void thrustEmitter();
	vector<Particle> particles;

	// Cameras
	ofCamera  fixedCam;
	ofEasyCam landerCam;
	ofEasyCam freeCam;
	ofCamera* curCam;
	bool bLookAtLander = false;

	// Fuel
	float fuelUse = 0.0;
	float startTime = 0.0;
	float fuelTime = 2.0 * 6.0 * 1000.0;

};

