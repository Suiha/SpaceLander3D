//--------------------------------------------------------------
//
//  Kevin M. Smith
//
//  Octree Test - startup scene
// 
//
//  Student Name:   Veronica Hu, Minh Anh Ton
//  Date: 11-26-2023


#include "ofApp.h"
#include "Util.h"


//--------------------------------------------------------------
// setup scene, lighting, state and load geometry
//
void ofApp::setup() {
	bAltKeyDown = false;
	bWireframe = false;
	bLanderLoaded = false;

	ofSetVerticalSync(true);
	ofEnableSmoothing();
	ofEnableDepthTest();

	// setup rudimentary lighting 
	//
	initLightingAndMaterials();

	// Setup 3 - Light System
	// 
	LTerrain.setup();
	LTerrain.enable();
	LTerrain.setAreaLight(3, 3);
	LTerrain.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
	LTerrain.setDiffuseColor(ofFloatColor(1, 1, 1));
	LTerrain.setSpecularColor(ofFloatColor(1, 1, 1));
	LTerrain.rotate(45, ofVec3f(0, 1, 0));
	LTerrain.rotate(-45, ofVec3f(1, 0, 0));
	LTerrain.setPosition(0, 100, 0);

	LLander.setup();
	LLander.disable();
	LLander.setSpotlight();
	LLander.setScale(.03);
	LLander.setAttenuation(2, .01, .01);
	LLander.setAmbientColor(ofColor::yellow);
	LLander.setDiffuseColor(ofFloatColor(1, 1, 1));
	LLander.setSpecularColor(ofFloatColor(1, 1, 1));
	//downward
	LLander.rotate(-90, ofVec3f(1, 0, 0));
	LLander.setPosition(0, 0, 0);

	LLander2.setup();
	LLander2.enable();
	LLander2.setSpotlight();
	LLander2.setScale(.03);
	LLander2.setAttenuation(2, .01, .01);
	LLander2.setAmbientColor(ofColor::lightGray);
	LLander2.setDiffuseColor(ofFloatColor(1, 1, 1));
	LLander2.setSpecularColor(ofFloatColor(1, 1, 1));
	//downward
	LLander2.setPosition(0, 0, 0);
	LLander2.rotate(0, ofVec3f(0, 1, 0));

	// load terrain models
	if (!mars.loadModel("geo/mars-low-5x-v2.obj")) {
		printf("error: mars terrain file not found\n");
	}
	if (!moon.loadModel("geo/moon-houdini.obj")) {
		printf("error: moon terrain file not found\n");
	}
	//mars.loadModel("geo/moon-houdini.obj");
	if (!mud.loadModel("geo/customTerrain/mudLand.obj")) {
		printf("error: mud terrain file not found\n");
	}
	mars.setScaleNormalization(false);
	moon.setScaleNormalization(false);
	mud.setScaleNormalization(false);

	//  Create Octree for terrain
	octreeMars.create(mars.getMesh(0), 20);
	cout << "Mars # of Verts: " << mars.getMesh(0).getNumVertices() << endl;
	octreeMoon.create(moon.getMesh(0), 20);
	cout << "Moon # of Verts: " << moon.getMesh(0).getNumVertices() << endl;
	octreeMud.create(mud.getMesh(0), 20);
	cout << "Mudland # of Verts: " << mud.getMesh(0).getNumVertices() << endl;
	currentNumLevels = 1;  // Set the default number of levels

	// current terrain
	terrain = mud;
	octree = octreeMud;
	gravity = 9.81;
	acceleration = glm::vec3(0, -gravity, 0);


	// lander setup
	if (lander.loadModel("geo/LEM-combined.obj")) {
		bLanderLoaded = true;
		lander.setScaleNormalization(false);
		lander.setPosition(0, 20, 0);

		bboxList.clear();
		for (int i = 0; i < lander.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(lander.getMesh(i)));
		}

		// set up bounding box for lander
		glm::vec3 min = lander.getSceneMin();
		glm::vec3 max = lander.getSceneMax();
		landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
	}

	bounceFactor = 100;

	// sounds
	thrustSound.load("sounds/lander_thrust.mp3");
	thrustSound.setLoop(true);

	// gui
	gui.setup();
	gui.add(numLevels.setup("Number of Octree Levels", 1, 1, 10));

	restartGame.addListener(this, &ofApp::restart);
	gui.add(restartGame.setup("Restart"));

	gui.add(muteSound.set("Mute Sound", false));
	gui.add(displayAltitude.set("Display Altitude Line", false));
	gui.add(fuelUsed.setup("Fuel: " + to_string((fuelTime - (fuelUse / 10)) / 100.0) + " sec"));
	gui.add(hardnessScale.setup("Crashing Threshold", gravity, 0.0, gravity * 2.0));

	marsMap.addListener(this, &ofApp::switchMars);
	moonMap.addListener(this, &ofApp::switchMoon);
	mudMap.addListener(this, &ofApp::switchMud);
	mapOptions.setName("Terrain Maps");
	mapOptions.add(marsMap.set("Mars", false));
	mapOptions.add(moonMap.set("Moon", false));
	mapOptions.add(mudMap.set("Mudland", true));
	gui.add(mapOptions);

	bHide = true;

	// Cameras
	curCam = &fixedCam;
	fixedCam.setPosition(glm::vec3(50, 100, 50));
	fixedCam.setNearClip(.1);
	landerCam.setPosition(lander.getPosition());
	landerCam.setNearClip(.1);
	freeCam.setDistance(20);
	freeCam.setNearClip(.1);

	// Background
	backgroundImage.load("stars.jpg");
}

// listeners for gui
void ofApp::restart() {
	bRunGame = false;
	explode = false;
	startTime = 0.0;
	fuelUse = 0.0;
	score = 0.0;
	correctLanding = 0.0;
	hardLanding = 0.0;
	crashLanding = 0.0;
	angularVelocity = 0.0;
	lander.setPosition(0, startingY, 0);

	velocity = glm::vec3(0, 0, 0);
}

void ofApp::switchMars(bool& val) {
	if (val) {
		bRunGame = false;
		explode = false;

		octree = octreeMars;
		terrain = mars;
		gravity = 3.71;
		acceleration = glm::vec3(0, -gravity, 0);
		velocity = glm::vec3(0, 0, 0);

		startingY = octree.height;
		lander.setPosition(0, startingY, 0);
		landerYOffset = 0;

		moonMap = false;
		mudMap = false;
		hardnessScale = gravity * 0.9;

		explode = false;
		startTime = 0.0;
		fuelUse = 0.0;
		score = 0.0;
		correctLanding = 0.0;
		hardLanding = 0.0;
		crashLanding = 0.0;
		angularVelocity = 0.0;
	}
}

void ofApp::switchMoon(bool& val) {
	if (val) {
		bRunGame = false;
		explode = false;

		octree = octreeMoon;
		terrain = moon;
		gravity = 1.62;
		acceleration = glm::vec3(0, -gravity, 0);
		velocity = glm::vec3(0, 0, 0);

		startingY = octree.height + 100;
		lander.setPosition(0, startingY, 0);
		landerYOffset = 0;

		marsMap = false;
		mudMap = false;
		hardnessScale = gravity * 0.9;

		explode = false;
		startTime = 0.0;
		fuelUse = 0.0;
		score = 0.0;
		correctLanding = 0.0;
		hardLanding = 0.0;
		crashLanding = 0.0;
		angularVelocity = 0.0;
	}
}

void ofApp::switchMud(bool& val) {
	if(val) {
		bRunGame = false;
		explode = false;

		octree = octreeMud;
		terrain = mud;
		gravity = 4.20;
		acceleration = glm::vec3(0, -gravity, 0);
		velocity = glm::vec3(0, 0, 0);

		startingY = octree.height;
		lander.setPosition(0, startingY, 0);
		landerYOffset = 7;

		marsMap = false;
		moonMap = false;
		hardnessScale = gravity * 0.9;

		explode = false;
		startTime = 0.0;
		fuelUse = 0.0;
		score = 0.0;
		correctLanding = 0.0;
		hardLanding = 0.0;
		crashLanding = 0.0;
		angularVelocity = 0.0;
	}
}

//--------------------------------------------------------------
// incrementally update scene (animation)
//
void ofApp::update() {
	currentNumLevels = numLevels;

	if (bLanderLoaded) {
		glm::vec3 landerPos = lander.getPosition(); // current lander position
		ofVec3f min, max;
		Box bounds;

		//2 cams
		fixedCam.lookAt(glm::vec3(landerPos.x, landerPos.y, landerPos.z));
		landerCam.lookAt(glm::vec3(landerPos.x, landerPos.y - 1, landerPos.z));
		landerCam.setPosition(landerPos);
		if (bLookAtLander) {
			freeCam.lookAt(glm::vec3(landerPos.x, landerPos.y, landerPos.z));
		}

		//1 lights
		LLander.setPosition(landerPos);
		LLander2.setPosition(glm::vec3(landerPos.x, landerPos.y + 5.0, landerPos.z));
		LLander2.lookAt(glm::vec3(landerPos.x + heading().x, landerPos.y + 5.0 + heading().y, landerPos.z + heading().z));
		if (bPlayerInput) {
			LLander.enable();
		}
		else {
			LLander.disable();
		}

		//1
		thrustEmitter();
		// Update and remove dead particles
		auto it = particles.begin();
		while (it != particles.end()) {
			it->update();
			if (it->isDead()) {
				it = particles.erase(it);
			}
			else {
				++it;
			}
		}

		// altitude calculation
		// ray from lander straight down
		Vector3 rayOrig = Vector3(landerPos.x, landerPos.y, landerPos.z);
		Ray ray = Ray(rayOrig, Vector3(0, -1, 0));

		// find ground level node
		octree.intersect(ray, octree.root, groundNode);

		// compare lander and ground height
		if (!groundNode.points.empty()) {
			float landerY = landerPos.y + landerYOffset; // offset for certain maps
			float groundY = octree.mesh.getVertex(groundNode.points[0]).y;
			altitude = landerY - groundY;
		}

		// Fuel
		fuelUsed = "Fuel: " + to_string((fuelTime - (fuelUse / 10)) / 100.0) + " sec";

		// collision effect
		if (colBoxList.size() >= 5) {
			if (!bGrounded) {
				// velocity value used later
				float vMagnitude = abs(velocity.x) + abs(velocity.y) + abs(velocity.z);
				float yForce = -velocity.y;

				min = lander.getSceneMin() + landerPos;
				max = lander.getSceneMax() + landerPos;
				bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

				// check if intersect with landing box - otherwise crash land
				bool landed = false;
				for (Box landing : octree.landingAreas) {
					if (landing.overlap(bounds)) {
						landed = true;
						break;
					}
				}

				if (!landed) crashLanding++;
				if (vMagnitude > hardnessScale) {
					// check if velocity of lander surpasses threshold for hard landing - even outside landing area
					// CALL EXPLOSION FUNCTION HERE - tbh still unsure about value for 'hard' so feel free to change it
					if (landed) hardLanding++;
					explode = true;
					acceleration = glm::vec3(ofRandom(-100, 100) * 5.0, ofRandom(100, 200) * 3.0, ofRandom(-100, 100) * 5.0);
					angularVelocity = ofRandom(-1000.0, 1000.0);
				}
				else if (landed) {
					correctLanding++;
				}

				if (!explode) {
					// stop movement on lander so it doesn't move through terrain
					bGrounded = true;
					velocity = glm::vec3(0, 0, 0);
					acceleration = glm::vec3(0, 0, 0);

					// decrease magnitude of bounce if there are multiple
					timeSinceLastBounce = ofGetElapsedTimeMillis() - timeSinceLastBounce;
					if (!bPlayerInput && timeSinceLastBounce < 5000) bounceFactor = (bounceFactor >= 10) ? bounceFactor - 10 : 0;
					else bounceFactor = 100;

					// calculate bounce direction based on collision points
					glm::vec3 bounceVector = glm::vec3(0, 0, 0);
					for (int point : colPoints) {
						// get vector from collision point to lander
						glm::vec3 p = landerPos - octree.mesh.getVertex(point);
						bounceVector += glm::vec3(p.x, -p.y, p.z);
					}
					bounceVector = glm::normalize(bounceVector / colPoints.size()); // average vectors

					bounceVector.y *= yForce;
					force = bounceVector * bounceFactor * (ofGetFrameRate() / 100);
				}
			}
		}
		else {
			bGrounded = false;
			timeSinceLastBounce = ofGetElapsedTimeMillis();
			acceleration = glm::vec3(0, -gravity, 0);
		}

		if (!explode && (correctLanding > 0.0)) {
			score = (correctLanding * 5) + (hardLanding * 1) + (crashLanding * -1);
		}

		// integrate lander
		if (bRunGame) integrate();

		// update collision detection
		min = lander.getSceneMin() + lander.getPosition();
		max = lander.getSceneMax() + lander.getPosition();
		bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

		colBoxList.clear();
		colPoints.clear();
		octree.intersect(bounds, octree.root, colBoxList, colPoints);

		// play sound
		if (!muteSound) {
			if (bPlayerInput && !thrustSound.isPlaying()) thrustSound.play();
			else if (!bPlayerInput && thrustSound.isPlaying()) thrustSound.stop();
		}
	}
}

//1
void ofApp::thrustEmitter() {
	if (!bLanderLoaded) { return; }

	float radius = 0.45; // Adjust the radius as needed
	float lifeSpan = 25.0; // Adjust the lifespan as needed
	float amount = 100; // Adjust the amount as needed
	float evenSpacedAmount = 20;
	float speed = 0.05; // Adjust the speed as needed
	ofVec3f landerPos = lander.getPosition();
	float yOffset = 0.4f;
	ofVec3f center(landerPos.x, landerPos.y + yOffset, landerPos.z);

	if (explode) {
		// Initial Explosion
		if (colBoxList.size() > 2) {
			radius = 20.0;
			yOffset = 1.0;
		}

		//Trail
		radius = 1.5;
		yOffset = 1.8;
		for (int i = 0; i < amount; ++i) {
			float angle = ofDegToRad(ofRandom(0, 360));
			float x, z;
			float y = 0;
			ofVec3f direction;
			float randomAngle;

			if (i < evenSpacedAmount) {
				x = radius * cos(angle);
				z = radius * sin(angle);
				randomAngle = ofRandom(-60, 60);
				lifeSpan += ofRandom(-2.5, 5.0);
			}
			else {
				float distance = ofRandom(0, radius);
				x = distance * cos(angle);
				z = distance * sin(angle);
				randomAngle = ofRandom(-40, 40);
				lifeSpan += ofRandom(-1.0, 5.0);
			}

			direction = ofVec3f(x, y + yOffset + 1, z);
			x += landerPos.x;
			y += landerPos.y + yOffset;
			z += landerPos.z;
			particles.emplace_back(x, y, z, lifeSpan, speed, direction);
		}
	} else if (bPlayerInput) {
		for (int i = 0; i < amount; ++i) {
			float angle = ofDegToRad(ofRandom(0, 360));
			float x, z;
			float y = 0;
			ofVec3f direction;
			float randomAngle;

			if (i < evenSpacedAmount) {
				x = radius * cos(angle);
				z = radius * sin(angle);
				randomAngle = ofRandom(-40, 40);
				lifeSpan += ofRandom(-2.5, 1.0);
			}
			else {
				float distance = ofRandom(0, radius);
				x = distance * cos(angle);
				z = distance * sin(angle);
				randomAngle = ofRandom(-20, 20);
				lifeSpan += ofRandom(-1.0, 1.0);
			}

			direction = (center - ofVec3f(x, y, z)).getNormalized();
			ofVec3f randomOffset(cos(ofDegToRad(randomAngle)), 0, sin(ofDegToRad(randomAngle)));
			direction += randomOffset * 0.2;
			x += landerPos.x;
			y += landerPos.y + yOffset;
			z += landerPos.z;
			particles.emplace_back(x, y, z, lifeSpan, speed, direction);
		}
	}
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(ofColor::black);

	curCam->begin();

	// Draw background around 6 sides
	ofPushMatrix();
	ofSetColor(ofColor::white);
	float offsetZ = 3000.0f;
	float offsetS = offsetZ * 0.002;
	float widthPos = -ofGetWidth() * offsetS / 2;
	float heightPos = -ofGetHeight() * offsetS / 2;
	float width = ofGetWidth() * offsetS;
	float height = ofGetHeight() * offsetS;
	ofRotateY(90.0);
	backgroundImage.draw(widthPos, heightPos, offsetZ, width, height);
	backgroundImage.draw(widthPos, heightPos, -offsetZ, width, height);
	ofPopMatrix();
	ofPushMatrix();
	backgroundImage.draw(widthPos - offsetS, heightPos - offsetS, offsetZ, width, height);
	backgroundImage.draw(widthPos - offsetS, heightPos - offsetS, -offsetZ, width, height);
	ofPopMatrix();
	ofPushMatrix();
	ofRotateX(90.0);
	backgroundImage.draw(widthPos - offsetS, heightPos - offsetS, offsetZ, width, height);
	backgroundImage.draw(widthPos - offsetS, heightPos - offsetS, -offsetZ, width, height);
	ofPopMatrix();


	if (bWireframe) {                    // wireframe mode  (include axis)
		ofDisableLighting();
		ofSetColor(ofColor::slateGray);
		terrain.drawWireframe();
	}
	else {
		ofPushMatrix();
		ofEnableLighting();              // shaded mode
		terrain.drawFaces();
		ofMesh mesh;

		// draw landing areas
		for (Box landing : octree.landingAreas) {
			ofNoFill();
			ofSetColor(ofColor::green);
			Octree::drawBox(landing);
		}

		if (bLanderLoaded) {
			glm::vec3 landerPos = lander.getPosition();

			ofPushMatrix();
			lander.setRotation(1, rotation, 0, 1, 0);
			lander.drawFaces();
			ofPopMatrix();

			if (bDisplayBBoxes) {
				ofNoFill();
				ofSetColor(ofColor::white);

				for (int i = 0; i < lander.getNumMeshes(); i++) {
					ofPushMatrix();
					ofMultMatrix(lander.getModelMatrix());
					ofRotate(-90, 1, 0, 0); // correct model orientation
					Octree::drawBox(bboxList[i]);
					ofPopMatrix();
				}
			}

			if (bLanderSelected) {
				ofVec3f min = lander.getSceneMin() + landerPos;
				ofVec3f max = lander.getSceneMax() + landerPos;
				Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

				ofSetColor(ofColor::white);
				Octree::drawBox(bounds);

				// draw colliding boxes
				ofSetColor(ofColor::lightBlue);
				for (int i = 0; i < colBoxList.size(); i++) {
					Octree::drawBox(colBoxList[i]);
				}
			}

			// physical representation of altitude from ground level
			if (displayAltitude && !groundNode.points.empty()) {
				glm::vec3 groundPos = landerPos;
				groundPos.y = octree.mesh.getVertex(groundNode.points[0]).y - landerYOffset; // offset

				ofSetColor(ofColor::red);
				ofDrawLine(landerPos, groundPos);
			}
		}
	}

	// recursively draw octree
	ofDisableLighting();
	int level = 0;
	
	ofFill();
	//1 Draw particles
	for (auto& particle : particles) {
		particle.draw();
	}
	ofNoFill();

	if (bDisplayOctree) {
		ofNoFill();
		octree.draw(currentNumLevels, 0);
	}
	ofPopMatrix();
	curCam->end();

	if (bRunGame && (explode || (((fuelTime - (fuelUse / 10)) / 100.0) <= 0))) {
		ofSetColor(ofColor::white);
		ofDrawBitmapString("GAME OVER", ofGetWidth() / 2 - 30, ofGetHeight() / 2 - 70);
		if (((fuelTime - (fuelUse / 10)) / 100.0) <= 0) {
			ofDrawBitmapString("No more fuel", ofGetWidth() / 2 - 45, ofGetHeight() / 2 + 50);
		}
		else {
			ofDrawBitmapString("Landed too hard", ofGetWidth() / 2 - 50, ofGetHeight() / 2 + 50);
		}
		ofDrawBitmapString("Score:  " + ofToString(score), ofGetWidth() / 2 - 30, ofGetHeight() / 2 + 80);
		ofDrawBitmapString("h to view settings to restart", ofGetWidth() / 2 - 110, ofGetHeight() / 2 + 110);
	}
	else if (bRunGame && !explode && (correctLanding > 0.0)) {
		ofSetColor(ofColor::white);
		ofDrawBitmapString("MISSION SUCCESS", ofGetWidth() / 2 - 50, ofGetHeight() / 2 - 70);
		ofDrawBitmapString("Score:  " + ofToString(score), ofGetWidth() / 2 - 30, ofGetHeight() / 2 + 80);
		ofDrawBitmapString("h to view settings to restart", ofGetWidth() / 2 - 110, ofGetHeight() / 2 + 110);
	}
	else if (!bRunGame) {
		ofFill();
		ofSetColor(ofColor::black); // Set color to black
		ofDrawRectangle(ofGetWidth() / 4, ofGetHeight() / 4, ofGetWidth() / 2, ofGetHeight() / 2); // Draw a black rectangle
		ofSetColor(ofColor::white);

		float padding = 20.0;
		float xOff = (ofGetWidth() / 4) + padding * 2;
		float yOff = (ofGetHeight() / 4) + padding * 5;

		ofDrawBitmapString("SPACE LANDER 3D", xOff, yOff);
		ofDrawBitmapString("Press G to start game", xOff, yOff + padding * 3);
		ofDrawBitmapString("Successfully land in the green landing pads to win", xOff, yOff + padding * 4);

		ofDrawBitmapString("Setting Controls:", xOff, yOff + padding * 6);
		ofDrawBitmapString("h to toggle settings", xOff, yOff + padding * 7);
		ofDrawBitmapString("Toggle terrain maps to swip maps or reset current map", xOff, yOff + padding * 8);
		ofDrawBitmapString("The lower the crashing threshold the harder the landing", xOff, yOff + padding * 9);

		ofDrawBitmapString("Extra Settings:", xOff, yOff + padding * 11);
		ofDrawBitmapString("F1 for world camera", xOff, yOff + padding * 12);
		ofDrawBitmapString("F2 for lander's camera", xOff, yOff + padding * 13);
		ofDrawBitmapString("F3 for free camera", xOff, yOff + padding * 14);
		ofDrawBitmapString("z to toggle free camera to face lander", xOff, yOff + padding * 15);
		ofDrawBitmapString("c to toggle free camera movement", xOff, yOff + padding * 16);

		ofDrawBitmapString("Lander Controls:", xOff + padding * 17, yOff + padding * 11);
		ofDrawBitmapString("Spacebar to move upward", xOff + padding * 17, yOff + padding * 12);
		ofDrawBitmapString("Up arrow to move forward", xOff + padding * 17, yOff + padding * 13);
		ofDrawBitmapString("Down arrow to move backward", xOff + padding * 17, yOff + padding * 14);
		ofDrawBitmapString("Right arrow to turn right", xOff + padding * 17, yOff + padding * 15);
		ofDrawBitmapString("Left arrow to turn left", xOff + padding * 17, yOff + padding * 16);
	}

	ofSetColor(ofColor::white);
	ofDrawBitmapString("Altitude: " + ofToString(altitude) + " m", 50, ofGetHeight() - 60);
	ofDrawBitmapString("Fuel Left: " + ofToString((fuelTime - (fuelUse / 10)) / 100.0) + " seconds", 50, ofGetHeight() - 30);

	// draw gui
	glDepthMask(false);
	if (!bHide) gui.draw();
	glDepthMask(true);

}


// 
// Draw an XYZ axis in RGB at world (0,0,0) for reference.
//
void ofApp::drawAxis(ofVec3f location) {
	ofPushMatrix();
	ofTranslate(location);

	ofSetLineWidth(1.0);

	// X Axis
	ofSetColor(ofColor(255, 0, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));


	// Y Axis
	ofSetColor(ofColor(0, 255, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

	// Z Axis
	ofSetColor(ofColor(0, 0, 255));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

	ofPopMatrix();
}


void ofApp::keyPressed(int key) {
	switch (key) {
	case 'G':
	case 'g':
		if (bLanderLoaded) {
			bRunGame = true;
			explode = false;
			fuelUse = 0.0;
			score = 0;
		}
		break;
	case 'B':
	case 'b':
		bDisplayBBoxes = !bDisplayBBoxes;
		break;
	case 'C':
	case 'c':
		if (freeCam.getMouseInputEnabled()) freeCam.disableMouseInput();
		else freeCam.enableMouseInput();
		break;
	case 'F':
	case 'f':
		ofToggleFullscreen();
		break;
	case 'H':
	case 'h':
		bHide = !bHide;			 // toggle gui
		break;
	case 'O':
	case 'o':
		bDisplayOctree = !bDisplayOctree;
		break;
	case 'R':
	case 'r':
		freeCam.reset();
		break;
	case 's':
		savePicture();
		break;
	case 'W':
	case 'w':
		toggleWireframeMode();
		break;
	case 'z':
		if (bLookAtLander) bLookAtLander = false;
		else bLookAtLander = true;
		break;
	case ' ': // lander moves upward
		if (bPlayerInput) { fuelUse += ofGetElapsedTimeMillis() - startTime; }
		if (bRunGame && ((fuelTime - (fuelUse / 10)) / 100.0) > 0.0) {
			force = glm::vec3(0, gravity * 10, 0);
			bPlayerInput = true;
			startTime = ofGetElapsedTimeMillis();
		}
		break;
	case OF_KEY_UP: // lander moves forwards
		if (bPlayerInput) { fuelUse += ofGetElapsedTimeMillis() - startTime; }
		if (bRunGame && ((fuelTime - (fuelUse / 10)) / 100.0) > 0.0) moveLander(1); bPlayerInput = true; startTime = ofGetElapsedTimeMillis();
		break;
	case OF_KEY_DOWN: // lander moves backwards
		if (bPlayerInput) { fuelUse += ofGetElapsedTimeMillis() - startTime; }
		if (bRunGame && ((fuelTime - (fuelUse / 10)) / 100.0) > 0.0) moveLander(-1); bPlayerInput = true; startTime = ofGetElapsedTimeMillis();
		break;
	case OF_KEY_LEFT: // lander rotates counter-clockwise
		if (bPlayerInput) { fuelUse += ofGetElapsedTimeMillis() - startTime; }
		if (bRunGame && ((fuelTime - (fuelUse / 10)) / 100.0) > 0.0) rotateLander(1); bPlayerInput = true; startTime = ofGetElapsedTimeMillis();
		break;
	case OF_KEY_RIGHT: // lander rotates clockwise
		if (bPlayerInput) { fuelUse += ofGetElapsedTimeMillis() - startTime; }
		if (bRunGame && ((fuelTime - (fuelUse / 10)) / 100.0) > 0.0) rotateLander(-1); bPlayerInput = true; startTime = ofGetElapsedTimeMillis();
		break;
	case OF_KEY_F1:
		curCam = &fixedCam;
		break;
	case OF_KEY_F2:
		curCam = &landerCam;
		break;
	case OF_KEY_F3:
		curCam = &freeCam;
		break;
	case OF_KEY_ALT:
		freeCam.enableMouseInput();
		bAltKeyDown = true;
		break;
	default:
		break;
	}
}

void ofApp::keyReleased(int key) {
	switch (key) {
	case ' ': // lander moves upward
	case OF_KEY_UP:
	case OF_KEY_DOWN:
	case OF_KEY_LEFT:
	case OF_KEY_RIGHT:
		thrustSound.stop();
		if (bPlayerInput) {
			bPlayerInput = false;
			fuelUse += ofGetElapsedTimeMillis() - startTime;
		}
		break;
	case OF_KEY_ALT:
		freeCam.disableMouseInput();
		bAltKeyDown = false;
		break;
	default:
		break;

	}
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (freeCam.getMouseInputEnabled()) return;

	// if rover is loaded, test for selection
	//
	if (bLanderLoaded) {
		glm::vec3 origin = freeCam.getPosition();
		glm::vec3 mouseWorld = freeCam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);

		ofVec3f min = lander.getSceneMin() + lander.getPosition();
		ofVec3f max = lander.getSceneMax() + lander.getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		bool hit = bounds.intersect(Ray(Vector3(origin.x, origin.y, origin.z), Vector3(mouseDir.x, mouseDir.y, mouseDir.z)), 0, 10000);
		if (hit) {
			bLanderSelected = true;
			mouseDownPos = getMousePointOnPlane(lander.getPosition(), freeCam.getZAxis());
			mouseLastPos = mouseDownPos;
			bInDrag = true;
		}
		else {
			bLanderSelected = false;
		}
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
	// if moving camera, don't allow mouse interaction
	//
	if (freeCam.getMouseInputEnabled()) return;

	if (bInDrag) {
		glm::vec3 landerPos = lander.getPosition();

		glm::vec3 mousePos = getMousePointOnPlane(landerPos, freeCam.getZAxis());
		glm::vec3 delta = mousePos - mouseLastPos;

		landerPos += delta;
		lander.setPosition(landerPos.x, landerPos.y, landerPos.z);
		mouseLastPos = mousePos;

		ofVec3f min = lander.getSceneMin() + lander.getPosition();
		ofVec3f max = lander.getSceneMax() + lander.getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

		colBoxList.clear();
		octree.intersect(bounds, octree.root, colBoxList, colPoints);
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	bInDrag = false;
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {}
//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {}
//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {}
//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {}

//--------------------------------------------------------------
// setup basic ambient lighting in GL  (for now, enable just 1 light)
//
void ofApp::initLightingAndMaterials() {

	static float ambient[] =
	{ .5f, .5f, .5, 1.0f };
	static float diffuse[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float position[] =
	{ 5.0, 5.0, 5.0, 0.0 };

	static float lmodel_ambient[] =
	{ 1.0f, 1.0f, 1.0f, 1.0f };

	static float lmodel_twoside[] =
	{ GL_TRUE };


	glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_POSITION, position);

	glLightfv(GL_LIGHT1, GL_AMBIENT, ambient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, position);


	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
	glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, lmodel_twoside);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//	glEnable(GL_LIGHT1);
	glShadeModel(GL_SMOOTH);
}

void ofApp::savePicture() {
	ofImage picture;
	picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	picture.save("screenshot.png");
	cout << "picture saved" << endl;
}

void ofApp::toggleWireframeMode() {
	bWireframe = !bWireframe;
}

//--------------------------------------------------------------
//
// support drag-and-drop of model (.obj) file loading.  when
// model is dropped in viewport, place origin under cursor
//
void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (lander.loadModel(dragInfo.files[0])) {
		bLanderLoaded = true;
		lander.setScaleNormalization(false);
		lander.setPosition(0, 0, 0);
		cout << "number of meshes: " << lander.getNumMeshes() << endl;
		bboxList.clear();
		for (int i = 0; i < lander.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(lander.getMesh(i)));
		}

		//		lander.setRotation(1, 180, 1, 0, 0);

				// We want to drag and drop a 3D object in space so that the model appears 
				// under the mouse pointer where you drop it !
				//
				// Our strategy: intersect a plane parallel to the camera plane where the mouse drops the model
				// once we find the point of intersection, we can position the lander/lander
				// at that location.
				//

				// Setup our rays
				//
		glm::vec3 origin = freeCam.getPosition();
		glm::vec3 camAxis = freeCam.getZAxis();
		glm::vec3 mouseWorld = freeCam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
		float distance;

		bool hit = glm::intersectRayPlane(origin, mouseDir, glm::vec3(0, 0, 0), camAxis, distance);
		if (hit) {
			// find the point of intersection on the plane using the distance 
			// We use the parameteric line or vector representation of a line to compute
			//
			// p' = p + s * dir;
			//
			glm::vec3 intersectPoint = origin + distance * mouseDir;

			// Now position the lander's origin at that intersection point
			//
			glm::vec3 min = lander.getSceneMin();
			glm::vec3 max = lander.getSceneMax();
			float offset = (max.y - min.y) / 2.0;
			lander.setPosition(intersectPoint.x, intersectPoint.y - offset, intersectPoint.z);

			// set up bounding box for lander while we are at it
			//
			landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		}
	}


}

//  intersect the mouse ray with the plane normal to the camera 
//  return intersection point.   (package code above into function)
//
glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 planePt, glm::vec3 planeNorm) {
	// Setup our rays
	//
	glm::vec3 origin = freeCam.getPosition();
	glm::vec3 camAxis = freeCam.getZAxis();
	glm::vec3 mouseWorld = freeCam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
	float distance;

	bool hit = glm::intersectRayPlane(origin, mouseDir, planePt, planeNorm, distance);

	if (hit) {
		// find the point of intersection on the plane using the distance 
		// We use the parameteric line or vector representation of a line to compute
		//
		// p' = p + s * dir;
		//
		glm::vec3 intersectPoint = origin + distance * mouseDir;

		return intersectPoint;
	}
	else return glm::vec3(0, 0, 0);
}

// return direction lander is facing
glm::vec3 ofApp::heading() {
	if (bLanderLoaded) {
		// get heading of the lander based on current rotation
		glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0), glm::radians(rotation), glm::vec3(0, 1, 0));
		return glm::normalize(rotMatrix * glm::vec4(0, 0, -1, 1));
	}
}

// apply linear force along heading (direction = forward/backward)
void ofApp::moveLander(int dir) {
	force = heading() * 10 * dir;
}

// apply angular force (direction = counter/clockwise)
void ofApp::rotateLander(int dir) {
	angularForce = 50 * dir;
}

void ofApp::integrate() {
	// calculate time interval
	float dt = 1.0 / ofGetFrameRate();

	// update position from velocity & time interval
	ofVec3f pos = lander.getPosition() + (velocity * dt);
	lander.setPosition(pos.x, pos.y, pos.z);

	// update velocity (from acceleration)
	glm::vec3 accel = acceleration;
	accel += (force * 1.0 / mass);
	velocity += (accel * dt);

	// update rotation from angular velocity & time
	rotation += angularVelocity * dt;

	// update angular velocity (from angular acceleration)
	float angAccel = angularAcceleration;
	angAccel += angularForce / mass;
	angularVelocity += angAccel * dt;

	// multiply final result by the damping factor to sim drag
	velocity *= damping;
	angularVelocity *= angularDamping;

	// reset all forces
	force = glm::vec3(0, 0, 0);
	angularForce = 0;
}
