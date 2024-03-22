#pragma once

class Particle {
public:
    ofVec3f position;
    float lifespan;
    float speed;
    ofVec3f direction;

    Particle(float x, float y, float z, float _lifespan, float _speed, ofVec3f _direction) {
        position = ofVec3f(x, y, z);
        lifespan = _lifespan;
        speed = _speed;
        direction = _direction;
    }

    void update() {
        position += direction * speed / 10;
        position.y -= speed;
        lifespan -= 1.0;
    }

    void draw() {
        ofSetColor(ofColor::yellow);
        ofDrawSphere(position, 0.01); // Adjust the sphere radius as needed
    }

    bool isDead() {
        return lifespan < 0;
    }
};
