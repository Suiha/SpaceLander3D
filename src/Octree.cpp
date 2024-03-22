
//--------------------------------------------------------------
//
//  Kevin M. Smith
//
//  Simple Octree Implementation 11/10/2020
// 
//  Copyright (c) by Kevin M. Smith
//  Copying or use without permission is prohibited by law. 
//

#include "Octree.h"

//draw a box from a "Box" class  
//
void Octree::drawBox(const Box& box) {
	Vector3 min = box.parameters[0];
	Vector3 max = box.parameters[1];
	Vector3 size = max - min;
	Vector3 center = size / 2 + min;
	ofVec3f p = ofVec3f(center.x(), center.y(), center.z());
	float w = size.x();
	float h = size.y();
	float d = size.z();
	ofDrawBox(p, w, h, d);
}

// return a Mesh Bounding Box for the entire Mesh
//
Box Octree::meshBounds(const ofMesh& mesh) {
	int n = mesh.getNumVertices();
	ofVec3f v = mesh.getVertex(0);
	ofVec3f max = v;
	ofVec3f min = v;
	for (int i = 1; i < n; i++) {
		ofVec3f v = mesh.getVertex(i);

		if (v.x > max.x) max.x = v.x;
		else if (v.x < min.x) min.x = v.x;

		if (v.y > max.y) max.y = v.y;
		else if (v.y < min.y) min.y = v.y;

		if (v.z > max.z) max.z = v.z;
		else if (v.z < min.z) min.z = v.z;
	}
	//cout << "vertices: " << n << endl;
	//cout << "min: " << min << "max: " << max << endl;
	return Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
}

// getMeshPointsInBox:  return an array of indices to points in mesh that are contained 
//                      inside the Box.  Return count of points found;
//
int Octree::getMeshPointsInBox(const ofMesh& mesh, const vector<int>& points,
	Box& box, vector<int>& pointsRtn)
{
	int count = 0;
	for (int i = 0; i < points.size(); i++) {
		ofVec3f v = mesh.getVertex(points[i]);
		if (box.inside(Vector3(v.x, v.y, v.z))) {
			count++;
			pointsRtn.push_back(points[i]);
		}
	}
	return count;
}

// getMeshFacesInBox:  return an array of indices to Faces in mesh that are contained 
//                      inside the Box.  Return count of faces found;
//
int Octree::getMeshFacesInBox(const ofMesh& mesh, const vector<int>& faces,
	Box& box, vector<int>& facesRtn)
{
	int count = 0;
	for (int i = 0; i < faces.size(); i++) {
		ofMeshFace face = mesh.getFace(faces[i]);
		ofVec3f v[3];
		v[0] = face.getVertex(0);
		v[1] = face.getVertex(1);
		v[2] = face.getVertex(2);
		Vector3 p[3];
		p[0] = Vector3(v[0].x, v[0].y, v[0].z);
		p[1] = Vector3(v[1].x, v[1].y, v[1].z);
		p[2] = Vector3(v[2].x, v[2].y, v[2].z);
		if (box.inside(p, 3)) {
			count++;
			facesRtn.push_back(faces[i]);
		}
	}
	return count;
}

//  Subdivide a Box into eight(8) equal size boxes, return them in boxList;
//
void Octree::subDivideBox8(const Box& box, vector<Box>& boxList) {
	Vector3 min = box.parameters[0];
	Vector3 max = box.parameters[1];
	Vector3 size = max - min;
	Vector3 center = size / 2 + min;
	float xdist = (max.x() - min.x()) / 2;
	float ydist = (max.y() - min.y()) / 2;
	float zdist = (max.z() - min.z()) / 2;
	Vector3 h = Vector3(0, ydist, 0);

	//  generate ground floor
	//
	Box b[8];
	b[0] = Box(min, center);
	b[1] = Box(b[0].min() + Vector3(xdist, 0, 0), b[0].max() + Vector3(xdist, 0, 0));
	b[2] = Box(b[1].min() + Vector3(0, 0, zdist), b[1].max() + Vector3(0, 0, zdist));
	b[3] = Box(b[2].min() + Vector3(-xdist, 0, 0), b[2].max() + Vector3(-xdist, 0, 0));

	boxList.clear();
	for (int i = 0; i < 4; i++)
		boxList.push_back(b[i]);

	// generate second story
	//
	for (int i = 4; i < 8; i++) {
		b[i] = Box(b[i - 4].min() + h, b[i - 4].max() + h);
		boxList.push_back(b[i]);
	}
}

void Octree::create(const ofMesh& geo, int numLevels) {
	// Start measuring the time for tree creation
	int startTime = ofGetElapsedTimeMillis();

	// initialize octree structure
	//
	mesh = geo;
	int level = 0;
	root.box = meshBounds(mesh);
	if (!bUseFaces) {
		for (int i = 0; i < mesh.getNumVertices(); i++) {
			root.points.push_back(i);
		}
	}
	else {
		// need to load face vertices here
	}

	width = root.box.max().x() - root.box.min().x();
	height = root.box.max().y() - root.box.min().y();
	length = root.box.max().z() - root.box.min().z();

	float minDimension = (width < length) ? width : length;
	landingWidth = (minDimension < 500) ? 5 : 10;
	fat = (width + length) / (landingWidth * 10); // how fat the map is

	// recursively buid octree
	level++;
	subdivide(mesh, root, numLevels, level);
	generateLandingAreas();
}


// subdivide:  recursive function to perform octree subdivision on a mesh
//
//  subdivide(node) algorithm:
//     1) subdivide box in node into 8 equal side boxes - see helper function subDivideBox8().
//     2) For each child box
//            sort point data into each box  (see helper function getMeshFacesInBox())
//        if a child box contains at list 1 point
//            add child to tree
//            if child is not a leaf node (contains more than 1 point)
//               recursively call subdivide(child)  
//
void Octree::subdivide(const ofMesh& mesh, TreeNode& node, int numLevels, int level) {
	if (level >= numLevels) {
		return;
	}

	// Subdivide the box into 8 equal side boxes
	vector<Box> childBoxes;
	subDivideBox8(node.box, childBoxes);

	// Iterate through each child box
	for (int i = 0; i < 8; i++) {
		TreeNode childNode;
		childNode.box = childBoxes[i];

		// Get points inside the child box
		getMeshPointsInBox(mesh, node.points, childNode.box, childNode.points);

		// If the child node contains at least 1 point, add it to the tree
		if (!childNode.points.empty()) {
			node.children.push_back(childNode);

			// If the child node is not a leaf (contains more than 1 point), recursively subdivide
			if (childNode.points.size() > 1) {
				subdivide(mesh, node.children.back(), numLevels, level + 1);
			}
			else leafNodes.push_back(node);
		}
	}
}

// generate new set of landing areas
void Octree::generateLandingAreas() {
	landingAreas.clear();
	landingPoints.clear();

	// randomly pick leaf node to create landing area from
	for (TreeNode leaf : leafNodes) {
		if (ofRandom(1) < 0.1 && nLandings < maxLandings) {
			glm::vec3 point = mesh.getVertex(leaf.points[0]);
			createLanding(point);
		}
	}
}

// landing area algorithm
// Three things must be true to create new area:
// 1) new landing doesn't overlap with other areas
// 2) new landing is not too close to other areas
// 3) new landing is not on the edge of the map
//
void Octree::createLanding(glm::vec3 point) {

	// create landing area
	Vector3 min = Vector3(point.x - landingWidth, point.y, point.z - landingWidth);
	Vector3 max = Vector3(point.x + landingWidth, point.y + (landingWidth * 2), point.z + landingWidth);
	Box b = Box(min, max);

	// check 1: new landing doesn't overlap with other areas
	bool hit = false;
	for (Box landing : landingAreas) {
		if (landing.overlap(b)) {
			hit = true;
			break;
		}
	}

	// check 2: new landing is not too close to other areas
	if (!hit) {
		for (glm::vec3 p : landingPoints) {
			if (abs(glm::length(point - p)) < landingWidth * fat) {
				hit = true;
				break;
			}
		}
	}
	else return;

	// check 3: new landing is not near edge of map (x, z directions)
	if (!hit) {
		// define bounds
		Vector3 min = root.box.min();
		Vector3 max = root.box.max();
		glm::vec3 bound1 = glm::vec3(min.x(), point.y, min.z());
		glm::vec3 bound2 = glm::vec3(min.x(), point.y, max.z());
		glm::vec3 bound3 = glm::vec3(max.x(), point.y, min.z());
		glm::vec3 bound4 = glm::vec3(max.x(), point.y, max.z());

		// check distance to each bound
		if (abs(glm::length(bound1 - point)) < (landingWidth * fat * 3)) hit = true;
		else if (abs(glm::length(bound2 - point)) < (landingWidth * fat * 3)) hit = true;
		else if (abs(glm::length(bound3 - point)) < (landingWidth * fat * 3)) hit = true;
		else if (abs(glm::length(bound4 - point)) < (landingWidth * fat * 3)) hit = true;
	}
	else return;

	// passes all checks, add landing
	if (!hit) {
		nLandings++;
		landingAreas.push_back(Box(min, max));
		landingPoints.push_back(point);
	}
}

// octree intersect with ray
bool Octree::intersect(const Ray& ray, const TreeNode& node, TreeNode& nodeRtn) {
	bool intersects = false;

	if (node.points.size() == 1 || node.children.empty()) {
		nodeRtn = node;
		return true;
	}

	if (node.box.intersect(ray, 0, 10000.0)) {
		for (int i = 0; i < node.children.size(); i++) {
			if (node.children[i].box.intersect(ray, 0, 10000.0)) {
				intersects = intersect(ray, node.children[i], nodeRtn);
			}
		}
	}

	return intersects;
}


bool Octree::intersect(const Box& box, TreeNode& node, vector<Box>& boxListRtn, vector<int>& pointListRtn) {
	bool intersects = false;

	// termination condition can only be reached after several recursions
	// due to nature of octree
	// box overlaps & contains only 1 point, add to list of intersecting boxes
	if (node.points.size() == 1 || node.children.empty()) {
		boxListRtn.push_back(node.box);
		pointListRtn.push_back(node.points[0]);
		return true;
	}

	if (node.box.overlap(box)) {

		// recurse through children that overlap with lander bounds
		for (int i = 0; i < node.children.size(); i++) {
			if (node.children[i].box.overlap(box)) {
				intersects = intersect(box, node.children[i], boxListRtn, pointListRtn);
			}
		}
	}

	return intersects;
}

void Octree::draw(TreeNode& node, int numLevels, int level) {
	if (level >= numLevels) return;

	// Set color based on the level
	ofColor color = getColor(level);
	ofSetColor(color);

	// Draw the box
	drawBox(node.box);

	// Recursively draw children
	for (int i = 0; i < node.children.size(); i++) {
		draw(node.children[i], numLevels, level + 1);
	}
}