// Viewer headers
#include "Rect.h"
#include "Camera.h"

// standard c++ headers
#include "stdlib.h"

// GoTools headers
#include <GoTools/utils/Point.h>

Rect::Rect(const Rect &other) {
	coords   = other.coords;
	for(int j=0; j<4; j++)
		i[j] = other.i[j];
	initI    = other.initI;
	midTime  = other.midTime;
	cam      = other.cam;
}

Rect::Rect(int* ind, double *coords, Camera *cam) {
	i[0] = ind[0];
	i[1] = ind[1];
	i[2] = ind[2];
	i[3] = ind[3];
	this->coords = coords;
	this->cam    = cam;
}

bool Rect::operator<(const Rect &other) const {
	Go::Point c(3), o(3);
	Go::Point camPos = cam->getPos();
	double longThis  = -1;
	double longOther = -1;
	double distThis;
	double distOther;
	for(int j=0; j<4; j++) {
		c[0] = coords[3*i[j]  ];
		c[1] = coords[3*i[j]+1];
		c[2] = coords[3*i[j]+2];
		o[0] = other.coords[3*other.i[j]  ];
		o[1] = other.coords[3*other.i[j]+1];
		o[2] = other.coords[3*other.i[j]+2];
		distThis  = camPos.dist2(c);
		distOther = camPos.dist2(o);
		longThis  = (longThis  > distThis ) ? longThis  : distThis;
		longOther = (longOther > distOther) ? longOther : distOther;
	}

	return longThis > longOther;
}

Rect & Rect::operator=(const Rect &other) {
	coords   = other.coords;
	for(int j=0; j<4; j++)
		i[j] = other.i[j];
	initI    = other.initI;
	midTime  = other.midTime;
	cam      = other.cam;
	return *this;
}


