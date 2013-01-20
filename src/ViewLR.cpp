
// standard c++ headers
#include <iostream>
#include <stdlib.h>
#include <fstream>
#include <math.h>
#include <string>
#include <vector>
#include <algorithm>
#include <sys/time.h>

// LR spline headers
#include "LRSpline/LRSplineVolume.h"
#include "LRSpline/Element.h"
#include "LRSpline/MeshRectangle.h"
#include "LRSpline/Basisfunction.h"

// ViewLR headers
#include "Camera.h"
#include "Rect.h"

// openGL headers
#include <GL/glut.h>
#include <GL/glu.h>

typedef unsigned int uint;

using namespace std;
using namespace LR;


// window parameters
int window_width  = 1000;
int window_height = 700;

// camera control
double theta      = M_PI_2;    // spin around z-axis
double dt         = 0.08;      // theta-rounds per second
double phi        = 0;         // spin top/bottom
double dp         = 0.8;       // phi-rounds per second
double cam_dist   = 2.0;
Camera cam;


// time iteartion
long frameCount = 0;
int lastFrame   = 0;
struct timeval startTime, lastTime;

// view controls
bool drawElements        = false;
bool drawRectangles      = true;
bool drawFaces           = false;
bool drawBlinkingEl      = true;
bool drawBlinkingRect    = false;
bool drawSolidEdges      = false;
bool doRotation          = true;

// blinking rectangles and elements
vector<Rect> viewEl;
vector<Rect> viewRect;
double lastSpawnTime = 0.0;
double sigma = 0.2;
double startPerSec = 20;
double lifeLength  = 4.0;
double min_alpha   = 0.0;
double max_alpha   = 1.0;

// data buffers
int nRect, nEl;
GLuint *elLines;
GLuint *elFaces;
GLuint *rectLines;
GLuint *rectFaces;
double *rectCoord;
double *rectNormal;
double *rectColor;
double *elCoord;
double *elNormal;
double *elColor;
vector<GLuint> shellEl;
vector<GLuint> sparseEl;
vector<GLuint> sparseRect;
vector<bool> showingElement;
vector<bool> showingRectangle;

// debug stuff
bool printed_err  = false;


void drawScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnableClientState(GL_VERTEX_ARRAY);

	cam.paintBackground();
	cam.setProjection();
	cam.setModelView();

	// draw the axis cross
	glLineWidth(3);
	glBegin(GL_LINES);
		glColor3f(1,0,0);
		glVertex3f(0,0,0); glVertex3f(1,0,0);
		glColor3f(0,1,0);
		glVertex3f(0,0,0); glVertex3f(0,1,0);
		glColor3f(0,0,1);
		glVertex3f(0,0,0); glVertex3f(0,0,1);
	glEnd();

	// draw the ouline of the elements
	if(drawRectangles) {
		glLineWidth(2);
		glColor3d(0.1, 0.1, 0.1);
		glVertexPointer(3, GL_DOUBLE, 0, rectCoord);
		glDrawElements(GL_LINES, nRect*4*2, GL_UNSIGNED_INT, rectLines);
	}
	
	if(drawElements) {
		glLineWidth(1);
		glColor3d(0.0, 0.0, 0.0);
		glVertexPointer(3, GL_DOUBLE, 0, elCoord);
		glDrawElements(GL_LINES, nEl*12*2, GL_UNSIGNED_INT, elLines);
	}

	glClear(GL_DEPTH_BUFFER_BIT);

	// draw surfaces
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	if(drawBlinkingEl    && !drawSolidEdges) {
		// glColor3f(0.6313726, 0.5058824, 0.3137255);
		glVertexPointer(3, GL_DOUBLE, 0, elCoord);
		glColorPointer(4 , GL_DOUBLE, 0, elColor);
		glNormalPointer(   GL_DOUBLE, 0, elNormal);
		glDrawElements(GL_QUADS, sparseEl.size(), GL_UNSIGNED_INT, &sparseEl[0]);
	}

	if(drawBlinkingRect    && !drawSolidEdges) {
		// glColor3f(0.6313726, 0.5058824, 0.3137255);
		glVertexPointer(3, GL_DOUBLE, 0, rectCoord);
		glColorPointer(4 , GL_DOUBLE, 0, rectColor);
		glNormalPointer(   GL_DOUBLE, 0, rectNormal);
		glDrawElements(GL_QUADS, sparseRect.size(), GL_UNSIGNED_INT, &sparseRect[0]);
	}
	glDisableClientState(GL_COLOR_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisable(GL_LIGHTING);

	glEnable(GL_DEPTH_TEST);
	// draw the ouline of the elements
	if(drawRectangles) {
		glLineWidth(2);
		glColor3d(0.1, 0.1, 0.1);
		glVertexPointer(3, GL_DOUBLE, 0, rectCoord);
		glDrawElements(GL_LINES, nRect*4*2, GL_UNSIGNED_INT, rectLines);
	}
	
	if(drawElements) {
		glLineWidth(1);
		glColor3d(0.0, 0.0, 0.0);
		glVertexPointer(3, GL_DOUBLE, 0, elCoord);
		glDrawElements(GL_LINES, nEl*12*2, GL_UNSIGNED_INT, elLines);
	}

	if(drawSolidEdges) {
		glEnable(GL_LIGHTING);
		glEnableClientState(GL_NORMAL_ARRAY);
		glColor3f(0.6313726, 0.5058824, 0.3137255);
		glVertexPointer(3, GL_DOUBLE, 0, elCoord);
		glNormalPointer(   GL_DOUBLE, 0, elNormal);
		glDrawElements(GL_QUADS, shellEl.size(), GL_UNSIGNED_INT, &(shellEl[0]));
		glDisableClientState(GL_NORMAL_ARRAY);
		glDisable(GL_LIGHTING);
	}
	
	// make things appear
	glutSwapBuffers();

	if(!printed_err) {
		cout << "openGL error after first draw call: " << glGetError() << endl;
		printed_err = true;
	}
}

void makeSparseIndices() {
	sparseEl.clear();
	sparseRect.clear();
	for(uint i=0; i<viewEl.size(); i++)
		for(int j=0; j<4; j++) 
			sparseEl.push_back(viewEl[i].i[j]);

	for(uint i=0; i<viewRect.size(); i++)
		for(int j=0; j<4; j++) 
			sparseRect.push_back(viewRect[i].i[j]);
}

void pushRect(int i, double midTime) {
	int ind[4];

	// bottom face
	int k=0;
	ind[k++] = i*4 + 0;
	ind[k++] = i*4 + 1;
	ind[k++] = i*4 + 2;
	ind[k++] = i*4 + 3;
	Rect r(ind, rectCoord, &cam);
	r.midTime = midTime;
	r.initI = i;
	viewRect.push_back(r);
	
	showingRectangle[i] = true;
}

void pushElement(int i, double midTime) {
	int ind[4];
	int elementSetSize = nEl*8;

	// bottom face
	int k=0;
	ind[k++] = i*8 + 0;
	ind[k++] = i*8 + 1;
	ind[k++] = i*8 + 3;
	ind[k++] = i*8 + 2;
	Rect r(ind, elCoord, &cam);
	r.midTime = midTime;
	r.initI = i;
	viewEl.push_back(r);

	// top face
	k=0;
	ind[k++] = i*8 + 4;
	ind[k++] = i*8 + 5;
	ind[k++] = i*8 + 7;
	ind[k++] = i*8 + 6;
	r = Rect(ind, elCoord, &cam);
	r.midTime = midTime;
	r.initI = i;
	viewEl.push_back(r);

	// right face
	k=0;
	ind[k++] = i*8 + 1 + elementSetSize;
	ind[k++] = i*8 + 3 + elementSetSize;
	ind[k++] = i*8 + 7 + elementSetSize;
	ind[k++] = i*8 + 5 + elementSetSize;
	r = Rect(ind, elCoord, &cam);
	r.midTime = midTime;
	r.initI = i;
	viewEl.push_back(r);

	// left face
	k=0;
	ind[k++] = i*8 + 0 + elementSetSize;
	ind[k++] = i*8 + 2 + elementSetSize;
	ind[k++] = i*8 + 6 + elementSetSize;
	ind[k++] = i*8 + 4 + elementSetSize;
	r = Rect(ind, elCoord, &cam);
	r.midTime = midTime;
	r.initI = i;
	viewEl.push_back(r);

	// front face
	k=0;
	ind[k++] = i*8 + 0 + elementSetSize*2;
	ind[k++] = i*8 + 1 + elementSetSize*2;
	ind[k++] = i*8 + 5 + elementSetSize*2;
	ind[k++] = i*8 + 4 + elementSetSize*2;
	r = Rect(ind, elCoord, &cam);
	r.midTime = midTime;
	r.initI = i;
	viewEl.push_back(r);

	// back face
	k=0;
	ind[k++] = i*8 + 2 + elementSetSize*2;
	ind[k++] = i*8 + 3 + elementSetSize*2;
	ind[k++] = i*8 + 7 + elementSetSize*2;
	ind[k++] = i*8 + 6 + elementSetSize*2;
	r = Rect(ind, elCoord, &cam);
	r.midTime = midTime;
	r.initI = i;
	viewEl.push_back(r);

	showingElement[i] = true;
}

void rotateCamera(double mtime) {
	// increase angle for the next frame
	if(doRotation) {
		theta = mtime*dt*2*M_PI;
		phi   = sin(mtime*dp)*M_PI/4.0 + M_PI/2.0;
		cam.setPos(cam_dist, phi, theta);
	}
}

void addNewBlinks(double mtime) {
	int mult = floor((mtime-lastSpawnTime) * startPerSec);
	if(mult > 0) {
		for(int i=0; i<mult; i++) {
			int j = rand() % nEl;
			if(!showingElement[j])
				pushElement(j, mtime + lifeLength/2.0);
		}
		for(int i=0; i<mult; i++) {
			int j = rand() % nRect;
			if(!showingRectangle[j])
				pushRect(j, mtime + lifeLength/2.0);
		}
		lastSpawnTime = mtime;
	} 
}

void updateAlpha(double mtime) {
	// double dt = mtime - lastTime.tv_sec - lastTime.tv_usec*1e-6;

	int n = viewEl.size();
	for(int i=0; i<n; i++) {
		double alpha = 0.0;
		if(fabs(viewEl[i].midTime - mtime) > lifeLength/2.0) {
			showingElement[ viewEl[i].initI ] = false;
			viewEl.erase(viewEl.begin() + i);
			i--;
			n--;
			continue;
		} else {
			double t2 = (mtime-viewEl[i].midTime)*(mtime-viewEl[i].midTime);
			alpha = exp(-t2/sigma) * (max_alpha-min_alpha) + min_alpha;
		}

		for(int j=0; j<4; j++)
			elColor[ 4*viewEl[i].i[j] + 3 ] = alpha;
	}

	n = viewRect.size();
	for(int i=0; i<n; i++) {
		double alpha = 0.0;
		if(fabs(viewRect[i].midTime - mtime) > lifeLength/2.0) {
			showingRectangle[ viewRect[i].initI ] = false;
			viewRect.erase(viewRect.begin() + i);
			i--;
			n--;
			continue;
		} else {
			double t2 = (mtime-viewRect[i].midTime)*(mtime-viewRect[i].midTime);
			alpha = exp(-t2/sigma) * (max_alpha-min_alpha) + min_alpha;
		}

		for(int j=0; j<4; j++)
			rectColor[ 4*viewRect[i].i[j] + 3 ] = alpha;
	}

}


void handleResize(int w, int h) {
	window_width  = w;
	window_height = h;
	glViewport(0,0, window_width, window_height);
	cam.handleResize(0,0,w,h);
}

void handleKeypress(unsigned char key, int x, int y) {
	if(key == 'r') {
		drawRectangles = !drawRectangles;
		cout << "Drawing meshrectangles: " << drawRectangles << endl;
	} else if (key == 'e') {
		drawElements = !drawElements;
		cout << "Drawing elements: " << drawElements << endl;
	} else if (key == 's') {
		doRotation = !doRotation;
		cout << "Auto rotating: " << doRotation << endl;
	} else if (key == '1') {
		drawBlinkingEl    = !drawBlinkingEl   ;
		cout << "Blinking elements: " << drawBlinkingEl    << endl;
	} else if (key == '2') {
		drawBlinkingRect    = !drawBlinkingRect   ;
		cout << "Blinking meshrectangles: " << drawBlinkingRect    << endl;
	} else if (key == '3') {
		drawSolidEdges = !drawSolidEdges;
		cout << "Drawing solid box: " << drawSolidEdges << endl;
	} else if (key == 'q') {
		cout << "Quit" << endl;
		exit(0);
	}
}

void processMouse(int button, int state, int x, int y) {
	cam.processMouse(button, state, x, y);
}

void processMouseActiveMotion(int x, int y) {
	cam.processMouseActiveMotion(x,y);
}

void processMousePassiveMotion(int x, int y) {
	cam.processMousePassiveMotion(x,y);
}

void initRendering() {

	// standard stuff
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glEnable(GL_NORMALIZE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	// glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	// make coloring compatible with lighting
	glColorMaterial ( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE ) ;
	glEnable ( GL_COLOR_MATERIAL ) ;
	// glEnable ( GL_DEPTH_TEST ) ;

	// setup transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// setup camera
	cam.setPos(cam_dist,phi,theta);
	cam.setLookAt(.5, .5, .5);
}

/* executed when program is idle */
void idle() { 
	drawScene();

	// first frame, start timer
	if(frameCount == 0) {
		gettimeofday(&lastTime, NULL);
		startTime = lastTime;
		srand(lastTime.tv_usec);
	// dump frames-per-second every 3 seconds
	} else {
		struct timeval end;
		long mtime, seconds, useconds;    
		gettimeofday(&end, NULL);
	
		seconds  = end.tv_sec  - lastTime.tv_sec;
		useconds = end.tv_usec - lastTime.tv_usec;
	
		mtime = ((seconds) * 1000 + useconds/1000.0) + 0.5;
		if(seconds > 2) {
			printf("Elapsed time: %ld milliseconds\n", mtime);
			printf("Elapsed time since start: %ld s\n", end.tv_sec - startTime.tv_sec);
			double fps = (frameCount - lastFrame) * 1000.0 / mtime;
			printf("%ld frames in %ld milliseconds = %.3f fps\n", frameCount-lastFrame, mtime, fps);

			gettimeofday(&lastTime, NULL);
			lastFrame = frameCount;
		}
	}

	// find seconds since start
	struct timeval end;
	gettimeofday(&end, NULL);
	long seconds  = end.tv_sec  - startTime.tv_sec;
	long useconds = end.tv_usec - startTime.tv_usec;
	double mtime = seconds + useconds*1e-6;

	// update the geometry
	rotateCamera(mtime);
	if(!drawSolidEdges) {
		addNewBlinks(mtime);
		updateAlpha(mtime);
		sort(viewEl.begin(), viewEl.end());
		sort(viewRect.begin(), viewRect.end());
		makeSparseIndices();
	}

	// wait a few moments before continuing
	// usleep(2000);

	/* iterate time step */
	frameCount++;
}


int main(int argc, char **argv) {
	if(argc != 2) {
		cerr << "File usage:\n" << argv[0] << " <filename>" << endl;
		exit(1);
	}
	
	// read geometry
	ifstream inFile;
	inFile.open(argv[1]);
	if(!inFile.good()) {
		cerr << "Error opening \"" << argv[1] << "\"\n";
		exit(2);
	}

	LRSplineVolume lr;
	inFile >> lr;
	inFile.close();

	nRect  = lr.nMeshRectangles();
	rectCoord  = new double[nRect*4*3];
	rectNormal = new double[nRect*4*3];
	rectColor  = new double[nRect*4*4];
	rectLines  = new GLuint[nRect*4*2];
	rectFaces  = new GLuint[nRect*4];

	int i=0;
	int j=0;
	int k=0;
	int l=0;

	for(MeshRectangle* m : lr.getAllMeshRectangles() ) {
		double x1 = m->start_[0];
		double y1 = m->start_[1];
		double z1 = m->start_[2];
		double x2 = m->stop_[0];
		double y2 = m->stop_[1];
		double z2 = m->stop_[2];
		rectCoord[k++] = x1;    rectCoord[k++] = y1;   rectCoord[k++] = z1;
		if(m->constDirection() == 0) {
			rectCoord[k++] = x1;    rectCoord[k++] = y2;   rectCoord[k++] = z1;
			rectCoord[k++] = x1;    rectCoord[k++] = y2;   rectCoord[k++] = z2;
			rectCoord[k++] = x1;    rectCoord[k++] = y1;   rectCoord[k++] = z2;
		} else if(m->constDirection() == 1) {
			rectCoord[k++] = x2;    rectCoord[k++] = y1;   rectCoord[k++] = z1;
			rectCoord[k++] = x2;    rectCoord[k++] = y1;   rectCoord[k++] = z2;
			rectCoord[k++] = x1;    rectCoord[k++] = y1;   rectCoord[k++] = z2;
		} else {
			rectCoord[k++] = x2;    rectCoord[k++] = y1;   rectCoord[k++] = z1;
			rectCoord[k++] = x2;    rectCoord[k++] = y2;   rectCoord[k++] = z1;
			rectCoord[k++] = x1;    rectCoord[k++] = y2;   rectCoord[k++] = z1;
		}
		for(i=0; i<4*3; i++)
			rectNormal[j++] = (i%3==m->constDirection());
		double r = 1.0*rand() / RAND_MAX;
		double g = 1.0*rand() / RAND_MAX;
		double b = 1.0*rand() / RAND_MAX;
		for(int component=0; component<4; component++) {
			rectColor[l++] = r;
			rectColor[l++] = g;
			rectColor[l++] = b;
			rectColor[l++] = min_alpha;
		}

/*
		rectColor[l++] = 0.6313726;
		rectColor[l++] = 0.5058824;
		rectColor[l++] = 0.3137255;
		rectColor[l++] = min_alpha;
*/
	}

	j=0;
	k=0;
	for(int i=0; i<nRect; i++) {
		rectLines[j++] = i*4;
		rectLines[j++] = i*4 + 1;
		rectLines[j++] = i*4 + 1;
		rectLines[j++] = i*4 + 2;
		rectLines[j++] = i*4 + 2;
		rectLines[j++] = i*4 + 3;
		rectLines[j++] = i*4 + 3;
		rectLines[j++] = i*4;

		rectFaces[k++] = i*4  ;
		rectFaces[k++] = i*4+1;
		rectFaces[k++] = i*4+2;
		rectFaces[k++] = i*4+3;
	}

	nEl = lr.nElements();
	elCoord  = new double[nEl*8*3*3]; 
	elNormal = new double[nEl*8*3*3];
	elColor  = new double[nEl*8*4*3];
	elLines  = new GLuint[nEl*12*2];
	elFaces  = new GLuint[nEl*6*4];

	j = 0;
	k = 0;
	l = 0;
	int elementSetSize = nEl*8;
	for(int normalDir=0; normalDir<3; normalDir++) {
		for( Element *el : lr.getAllElements() ) {
			double x1 = el->getParmin(0);
			double y1 = el->getParmin(1);
			double z1 = el->getParmin(2);
			double x2 = el->getParmax(0);
			double y2 = el->getParmax(1);
			double z2 = el->getParmax(2);

			elCoord[k++] = x1;  elCoord[k++] = y1;  elCoord[k++] = z1;
			elCoord[k++] = x2;  elCoord[k++] = y1;  elCoord[k++] = z1;
			elCoord[k++] = x1;  elCoord[k++] = y2;  elCoord[k++] = z1;
			elCoord[k++] = x2;  elCoord[k++] = y2;  elCoord[k++] = z1;
			elCoord[k++] = x1;  elCoord[k++] = y1;  elCoord[k++] = z2;
			elCoord[k++] = x2;  elCoord[k++] = y1;  elCoord[k++] = z2;
			elCoord[k++] = x1;  elCoord[k++] = y2;  elCoord[k++] = z2;
			elCoord[k++] = x2;  elCoord[k++] = y2;  elCoord[k++] = z2;

			// trust me... it's right. Completely unreadable, but right.
			// the idea is to make 3 sets of complete cube coordinates. Corresponding to
			// each set is a normal vector pointing in one of the three cardinal directions
			if(normalDir == 0) // up-down normals
				for(i=0; i<3*8; i++) 
					elNormal[j++] = -(2*(i>11)-1)*(i%3==2) ;
			else if(normalDir == 1) // left-right normals
				for(i=0; i<3*8; i++) 
					elNormal[j++] = -(2*(i%2)-1)*(i%3==0) ;
			else if(normalDir == 2) // front-back normals
				for(i=0; i<3*8; i++) 
					elNormal[j++] = -((i&2)-1)*(i%3==1) ;
			// set color
			double r = 1.0*rand() / RAND_MAX;
			double g = 1.0*rand() / RAND_MAX;
			double b = 1.0*rand() / RAND_MAX;
			for(int component=0; component<8; component++) {
				if(normalDir == 0) {
					elColor[l++] = r;
					elColor[l++] = g;
					elColor[l++] = b;
					elColor[l++] = min_alpha;
				} else {
					for(int component=0; component<4; component++) {
						elColor[l] = elColor[l-elementSetSize*4];
						l++;
					}
				}
/*
				elColor[l++] = 0.6313726;
				elColor[l++] = 0.5058824;
				elColor[l++] = 0.3137255;
				elColor[l++] = min_alpha;
*/
			}

		}
	}

	j = 0;
	k = 0;
	l = 0;
	for(i=0; i<nEl; i++) {

		// bottom lines
		elLines[j++] = i*8    ;
		elLines[j++] = i*8 + 1;
		elLines[j++] = i*8    ;
		elLines[j++] = i*8 + 2;
		elLines[j++] = i*8 + 1;
		elLines[j++] = i*8 + 3;
		elLines[j++] = i*8 + 2;
		elLines[j++] = i*8 + 3;

		// top lines
		elLines[j++] = i*8 + 4;
		elLines[j++] = i*8 + 5;
		elLines[j++] = i*8 + 4;
		elLines[j++] = i*8 + 6;
		elLines[j++] = i*8 + 5;
		elLines[j++] = i*8 + 7;
		elLines[j++] = i*8 + 6;
		elLines[j++] = i*8 + 7;

		// in-between-lines
		elLines[j++] = i*8 + 0;
		elLines[j++] = i*8 + 4;
		elLines[j++] = i*8 + 1;
		elLines[j++] = i*8 + 5;
		elLines[j++] = i*8 + 2;
		elLines[j++] = i*8 + 6;
		elLines[j++] = i*8 + 3;
		elLines[j++] = i*8 + 7;

		// bottom face
		elFaces[k++] = i*8 + 0;
		elFaces[k++] = i*8 + 1;
		elFaces[k++] = i*8 + 3;
		elFaces[k++] = i*8 + 2;

		// top face
		elFaces[k++] = i*8 + 4;
		elFaces[k++] = i*8 + 5;
		elFaces[k++] = i*8 + 7;
		elFaces[k++] = i*8 + 6;

		// right face
		elFaces[k++] = i*8 + 1 + elementSetSize;
		elFaces[k++] = i*8 + 3 + elementSetSize;
		elFaces[k++] = i*8 + 7 + elementSetSize;
		elFaces[k++] = i*8 + 5 + elementSetSize;

		// left face
		elFaces[k++] = i*8 + 0 + elementSetSize;
		elFaces[k++] = i*8 + 2 + elementSetSize;
		elFaces[k++] = i*8 + 6 + elementSetSize;
		elFaces[k++] = i*8 + 4 + elementSetSize;

		// front face
		elFaces[k++] = i*8 + 0 + elementSetSize*2;
		elFaces[k++] = i*8 + 1 + elementSetSize*2;
		elFaces[k++] = i*8 + 5 + elementSetSize*2;
		elFaces[k++] = i*8 + 4 + elementSetSize*2;

		// back face
		elFaces[k++] = i*8 + 2 + elementSetSize*2;
		elFaces[k++] = i*8 + 3 + elementSetSize*2;
		elFaces[k++] = i*8 + 7 + elementSetSize*2;
		elFaces[k++] = i*8 + 6 + elementSetSize*2;

		if(lr.getElement(i)->getParmin(0) == lr.startparam(0)) {
			shellEl.push_back(i*8 + 0 + elementSetSize);
			shellEl.push_back(i*8 + 2 + elementSetSize);
			shellEl.push_back(i*8 + 6 + elementSetSize);
			shellEl.push_back(i*8 + 4 + elementSetSize);
		}
		if(lr.getElement(i)->getParmin(1) == lr.startparam(1)) {
			shellEl.push_back(i*8 + 0 + elementSetSize*2);
			shellEl.push_back(i*8 + 1 + elementSetSize*2);
			shellEl.push_back(i*8 + 5 + elementSetSize*2);
			shellEl.push_back(i*8 + 4 + elementSetSize*2);
		}
		if(lr.getElement(i)->getParmin(2) == lr.startparam(2)) {
			shellEl.push_back(i*8 + 0);
			shellEl.push_back(i*8 + 1);
			shellEl.push_back(i*8 + 3);
			shellEl.push_back(i*8 + 2);
		}
		if(lr.getElement(i)->getParmax(0) == lr.endparam(0)) {
			shellEl.push_back(i*8 + 1 + elementSetSize);
			shellEl.push_back(i*8 + 3 + elementSetSize);
			shellEl.push_back(i*8 + 7 + elementSetSize);
			shellEl.push_back(i*8 + 5 + elementSetSize);
		}
		if(lr.getElement(i)->getParmax(1) == lr.endparam(1)) {
			shellEl.push_back(i*8 + 2 + elementSetSize*2);
			shellEl.push_back(i*8 + 3 + elementSetSize*2);
			shellEl.push_back(i*8 + 7 + elementSetSize*2);
			shellEl.push_back(i*8 + 6 + elementSetSize*2);
		}
		if(lr.getElement(i)->getParmax(2) == lr.endparam(2)) {
			shellEl.push_back(i*8 + 4);
			shellEl.push_back(i*8 + 5);
			shellEl.push_back(i*8 + 7);
			shellEl.push_back(i*8 + 6);
		}
	}

	showingRectangle.resize(nRect, false);
	showingElement.resize(nEl, false);

	// initalize GLUT
	int glArgc = 0;
	glutInit(&glArgc, NULL);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(window_width, window_height);

	
	glutCreateWindow("LR spline volume (parametric space)");
	initRendering();
	
	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress);
	glutMouseFunc(processMouse);
	glutMotionFunc(processMouseActiveMotion);
	glutPassiveMotionFunc(processMousePassiveMotion);
	glutReshapeFunc(handleResize);
	glutIdleFunc(idle);

	glutMainLoop();

}

