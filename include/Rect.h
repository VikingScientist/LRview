#ifndef _RECT_H
#define _RECT_H

class Camera;

class Rect {
	public:
		double* coords;
		int i[4];
		int initI;
		double midTime;
		Camera *cam;
		
		Rect(const Rect &other);
		Rect(int *ind, double *coords, Camera *cam);

		bool operator<(const Rect &other) const;
		Rect & operator=(const Rect &other) ;
};

#endif
