#ifndef _CAMERA_H
#define _CAMERA_H

#include <GoTools/utils/Point.h>
#include <GL/glut.h>

class Camera  {

	public:
		// note: (phi, theta) defined as NTNU Matte 2, NOT wikipedia (which swithces the two)
		Camera();
		Camera(int x, int y, int w, int h);
		~Camera();
		void rotate(float d_r, float d_phi, float d_theta);
		void setPos(float r, float phi, float theta);
		void setLookAt(float x, float y, float z);
		void pan(float d_u, float d_v);
		void setModelView();
		void setProjection();
		void handleResize(int x, int y, int w, int h);
		GLfloat getR()        { return r;     };
		GLfloat getPhi()      { return phi;   };
		GLfloat getTheta()    { return theta; };
		Go::Point getLookAt() { return Go::Point(look_at_x, look_at_y, look_at_z); };
		Go::Point getPos()    { return Go::Point(x,y,z); };
		virtual void processMouse(int button, int state, int x, int y);
		virtual void processMouseActiveMotion(int x, int y);
		virtual void processMousePassiveMotion(int x, int y);

		void paintBackground();
		void paintMeta();


	private:
		void recalc_pos();

		GLfloat speed_pan_u;
		GLfloat speed_pan_v;

		GLfloat r;
		GLfloat phi;
		GLfloat theta;

		GLfloat look_at_x;
		GLfloat look_at_y;
		GLfloat look_at_z;
		
		GLfloat x;
		GLfloat y;
		GLfloat z;

		int last_mouse_x;
		int last_mouse_y;
		bool just_warped;

		int vp_width;
		int vp_height;
		
		int specialKey;
		bool right_mouse_button_down;

		double size;
		bool upside_down;
		bool adaptive_tesselation;
};

#endif

