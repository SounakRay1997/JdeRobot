/*
*  Copyright (C) 1997-2010 JDERobot Developers Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *   Authors : Eduardo Perdices <eperdices@gsyc.es>,
 *             Jose María Cañas Plaza <jmplaza@gsyc.es>
 *
 */

#include "drawarea.h"

namespace teleoperator {

	const float DrawArea::MAXWORLD = 30.;
	const float DrawArea::PI = 3.141592654;

	DrawArea::DrawArea(BaseObjectType* cobject, const Glib::RefPtr<Gnome::Glade::Xml>& builder)
	: Gtk::DrawingArea(cobject), Gtk::GL::Widget<DrawArea>() {

		this->refresh_time = 100; //ms

		/*Resize drawing area*/
		this->set_size_request(640,480);

		Glib::RefPtr<Gdk::GL::Config> glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB | Gdk::GL::MODE_DEPTH | Gdk::GL::MODE_DOUBLE);
		if (!glconfig) {
			std::cerr << "*** Cannot find the double-buffered visual.\n" << "*** Trying single-buffered visual.\n";

			// Try single-buffered visual
			glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB | Gdk::GL::MODE_DEPTH);
			if (!glconfig) {
				std::cerr << "*** Cannot find any OpenGL-capable visual.\n";
				std::exit(1);
			}
		}

		/*Set OpenGL-capability to the widget.*/
		this->unrealize();
		if(!this->set_gl_capability(glconfig) || !this->is_gl_capable()) {
			std::cerr << "No Gl capability\n";
			std::exit(1);
		}
		this->realize();

		/*Add events*/
		this->add_events(	Gdk::BUTTON1_MOTION_MASK    |
											Gdk::BUTTON2_MOTION_MASK    |
											Gdk::BUTTON3_MOTION_MASK    |
											Gdk::BUTTON_PRESS_MASK      |
											Gdk::BUTTON_RELEASE_MASK    |
											Gdk::VISIBILITY_NOTIFY_MASK);

		this->signal_motion_notify_event().connect(sigc::mem_fun(this,&DrawArea::on_motion_notify));
		this->signal_scroll_event().connect(sigc::mem_fun(this,&DrawArea::on_drawarea_scroll));

		/*Call to expose_event*/
		Glib::signal_timeout().connect( sigc::mem_fun(*this, &DrawArea::on_timeout), this->refresh_time);

		/*Init Glut*/
		int val_init = 0;
		glutInit(&val_init, NULL);
 		glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);

		/*GL Camera Position and FOA*/
		this->glcam_pos.X=-100.;
		this->glcam_pos.Y=-100.;
		this->glcam_pos.Z=70.;
		this->glcam_foa.X=0.;
		this->glcam_foa.Y=0.;
		this->glcam_foa.Z=0.;
/*
		this->glcam_pos.X = 1.5;
		this->glcam_pos.Y = -8.0;
		this->glcam_pos.Z = 2.5;	

		this->glcam_foa.X = -3.0;
		this->glcam_foa.Y = 0.5;
		this->glcam_foa.Z = 0.0;	
	*/	
		/*Initial Position*/
		/*this->cam_pos.X = -5.5;
		this->cam_pos.Y = 0.0;
		this->cam_pos.Z = 3.0;	*/

		/*Init camera*/
		this->camera.position.X = 0.;
		this->camera.position.Y = 0.;
		this->camera.position.Z = 0.;
		this->camera.position.H = 1.;
		this->camera.foa.X = 0.;
		this->camera.foa.Y = 0.;
		this->camera.foa.Z = 0.;
		this->camera.foa.H = 1.;
		this->camera.fdistx = 300.0;
		this->camera.fdisty = 300.0;
		this->camera.v0 = IMAGE_WIDTH/2;
		this->camera.u0 = IMAGE_HEIGHT/2;
		this->camera.roll = 0.0;
		this->camera.columns = IMAGE_WIDTH;
		this->camera.rows = IMAGE_HEIGHT;
		this->camera.skew = 0.0;

    this->scale = 0.1;
    this->radius = 20.0;
    this->lati = 0.2;
    this->longi = -1.0;
    this->old_x = 0.0;
    this->old_y = 0.0; 

		init_pioneer(); // IMPORTANTE: para cargar los vectores de sonares y lasers
	}

	DrawArea::~DrawArea() {	}

	void DrawArea::InitOGL (int w, int h) {
		GLfloat ambient[] = {1.0, 1.0, 1.0, 1.0};
		GLfloat diffuse[] = {1.0, 1.0, 1.0, 1.0};
		GLfloat position[] = {0.0, 3.0, 3.0, 0.0};
		GLfloat lmodel_ambient[] = {0.2, 0.2, 0.2, 1.0};
		GLfloat local_view[] = {0.0};

		glViewport(0,0,(GLint)w,(GLint)h);  
		glDrawBuffer(GL_BACK);
		glClearColor(0.6f, 0.8f, 1.0f, 0.0f);
		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* With this, the pioneer appears correctly, but the cubes don't */
		glLightfv (GL_LIGHT0, GL_AMBIENT, ambient);
		glLightfv (GL_LIGHT0, GL_DIFFUSE, diffuse);
		glLightfv (GL_LIGHT0, GL_POSITION, position);
		glLightModelfv (GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
		glLightModelfv (GL_LIGHT_MODEL_LOCAL_VIEWER, local_view);
		glEnable (GL_LIGHT0);
		// glEnable (GL_LIGHTING);

		glEnable(GL_TEXTURE_2D);     // Enable Texture Mapping
		glEnable (GL_AUTO_NORMAL);
		glEnable (GL_NORMALIZE);  
		glEnable(GL_DEPTH_TEST);     // Enables Depth Testing
		glDepthFunc(GL_LESS);  
		glShadeModel(GL_SMOOTH);     // Enables Smooth Color Shading
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	bool DrawArea::on_expose_event(GdkEventExpose* event) {
		Gtk::Allocation allocation = get_allocation();
		GLfloat width, height;

		Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();
		glwindow->gl_begin(get_gl_context());

		glDrawBuffer(GL_BACK);
		glClearColor(0.6f, 0.8f, 1.0f, 0.0f);

		glClearDepth(1.0);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);
		
		width = allocation.get_width();
		height = allocation.get_height();

		//glViewport(0,0,width,height);
		this->InitOGL (width, height);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		/*Angulo	ratio		znear, zfar*/
		gluPerspective(50.0, 	width/height, 	1.0, 50000.0);	

		glMatrixMode (GL_MODELVIEW);
		glLoadIdentity();

		/*pos cam		pto central	vector up*/
		gluLookAt(this->glcam_pos.X, this->glcam_pos.Y, this->glcam_pos.Z,
							this->glcam_foa.X, this->glcam_foa.Y, this->glcam_foa.Z,
							0., 0., 1.);

		/*Draw world*/
		this->drawScene();

		/*Swap buffers*/
		if (glwindow->is_double_buffered())
			glwindow->swap_buffers();
		else
			glFlush();

		glwindow->gl_end();

		return true;
	}

	bool DrawArea::on_timeout() {
		/*Force our program to redraw*/
		Glib::RefPtr<Gdk::Window> win = get_window();
		if (win) {
			Gdk::Rectangle r(0, 0, get_allocation().get_width(), get_allocation().get_height());
			win->invalidate_rect(r, false);
		}
		return true;
	}

	void DrawArea::drawScene() {
		int i,c,row,j,k;
		Tvoxel start,end;
		float r,lati,longi,dx,dy,dz;
		float matColors[4];
		float  Xp_sensor, Yp_sensor;
		float dpan=0.5,dtilt=0.5;

		// Absolute Frame of Reference
		// floor
		glColor3f( 0.4, 0.4, 0.4 );
		glBegin(GL_LINES);
		for(i=0;i<((int)MAXWORLD+1);i++) {
			v3f(-(int)MAXWORLD*10/2.+(float)i*10,-(int)MAXWORLD*10/2.,0.);
			v3f(-(int)MAXWORLD*10/2.+(float)i*10,(int)MAXWORLD*10/2.,0.);
			v3f(-(int)MAXWORLD*10/2.,-(int)MAXWORLD*10/2.+(float)i*10,0.);
			v3f((int)MAXWORLD*10/2.,-(int)MAXWORLD*10/2.+(float)i*10,0.);
		}
		glEnd();

		// absolute axis
		glLineWidth(3.0f);
		glColor3f( 0.7, 0., 0. );
		glBegin(GL_LINES);
		v3f( 0.0, 0.0, 0.0 );   
		v3f( 10.0, 0.0, 0.0 );
		glEnd();
		glColor3f( 0.,0.7,0. );
		glBegin(GL_LINES);
		v3f( 0.0, 0.0, 0.0 );   
		v3f( 0.0, 10.0, 0.0 );
		glEnd();
		glColor3f( 0.,0.,0.7 );
		glBegin(GL_LINES);
		v3f( 0.0, 0.0, 0.0 );   
		v3f( 0.0, 0.0, 10.0 );
		glEnd();
		glLineWidth(1.0f);

		// Robot Frame of Reference
		mypioneer.posx=this->robotx/100.;
		mypioneer.posy=this->roboty/100.;
		mypioneer.posz=0.;
		mypioneer.foax=this->robotx/100.;
		mypioneer.foay=this->roboty/100.;
		mypioneer.foaz=10.;
		mypioneer.roll=this->robottheta*(180/PI);

		glTranslatef(mypioneer.posx,mypioneer.posy,mypioneer.posz);
		dx=(mypioneer.foax-mypioneer.posx);
		dy=(mypioneer.foay-mypioneer.posy);
		dz=(mypioneer.foaz-mypioneer.posz);
		longi=(float)atan2(dy,dx)*360./(2.*PI);
		glRotatef(longi,0.,0.,1.);
		r=sqrt(dx*dx+dy*dy+dz*dz);
		if (r<0.00001) lati=0.;
		else lati=acos(dz/r)*360./(2.*PI);
		glRotatef(lati,0.,1.,0.);
		glRotatef(mypioneer.roll,0.,0.,1.);

		// X axis
		glColor3f( 1., 0., 0. );
		glBegin(GL_LINES);
		v3f( 0.0, 0.0, 0.0 );   
		v3f( 5.0, 0.0, 0.0 );
		glEnd();
		// Y axis
		glColor3f( 0., 1., 0. );  
		glBegin(GL_LINES);
		v3f( 0.0, 0.0, 0.0 );   
		v3f( 0.0, 5.0, 0.0 );
		glEnd();
		// Z axis
		glColor3f( 0., 0., 1.);
		glBegin(GL_LINES);
		v3f( 0.0, 0.0, 0.0 );   
		v3f( 0.0, 0.0, 5.0 );
		glEnd();

		// robot body
		glEnable (GL_LIGHTING);
		glPushMatrix();
		glTranslatef(1.,0.,0.);
		// the body it is not centered. With this translation we center it
		loadModel(); // CARGAMOS EL MODELO DEL PIONEER
		glPopMatrix();
		glDisable (GL_LIGHTING);

		// sonars
		glColor3f( 0., 0.8, 0. );
		glLineWidth(2.0f);

		for (k = 0; k < this->numSonars; k++) {
			start.x=us_coord[k][0];
			start.y=us_coord[k][1];

			Xp_sensor = this->us[k];
			Yp_sensor = 0.;

			// Coordenadas del punto detectado por el US con respecto al sistema del sensor, eje x+ normal al sensor
			end.x = us_coord[k][0] + Xp_sensor*us_coord[k][3] - Yp_sensor*us_coord[k][4];
			end.y = us_coord[k][1] + Yp_sensor*us_coord[k][3] + Xp_sensor*us_coord[k][4];

			glBegin(GL_LINES);
				glVertex3f (start.x/100., start.y/100., 2.0f);
				glVertex3f (end.x/100., end.y/100., 2.0f);
			glEnd();
		}
		glLineWidth(1.0f);

		// laser
		glEnable (GL_LIGHTING);
		matColors[0] = 1.0;
		matColors[1] = 0.0;
		matColors[2] = 0.0;
		matColors[3] = 0.5;
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE,matColors);

		start.x=laser_coord[0]*10.;
		start.y=laser_coord[1];
		for(k=0;k<this->numLasers;k++) {
			Xp_sensor = this->distanceData[k]*cos(((float)k-90.)*DEGTORAD);
			Yp_sensor = this->distanceData[k]*sin(((float)k-90.)*DEGTORAD);

			// Coordenadas del punto detectado por el US con respecto al sistema del sensor, eje x+ normal al sensor
			end.x = laser_coord[0]*10. + Xp_sensor*laser_coord[3] - Yp_sensor*laser_coord[4];
			end.y = laser_coord[1] + Yp_sensor*laser_coord[3] + Xp_sensor*laser_coord[4];

			glBegin(GL_POLYGON); 
				glVertex3f (laser_coord[0]*10./100., laser_coord[1]/100., 3.2);
				glVertex3f (start.x/100., start.y/100., 3.2);
				glVertex3f (end.x/100., end.y/100., 3.2);
			glEnd();
			start.x=end.x;
			start.y=end.y;
		}
		glDisable (GL_LIGHTING);
	}

	void DrawArea::draw_world() {
		/* Ground  */ 
		glColor3f( 0.4, 0.4, 0.4 );
		glBegin(GL_LINES);
		int i;
		for(i=0;i<61;i++) {
			v3f(-10.+(float)i,-30,0);
			v3f(-10.+(float)i,30.,0.);
			v3f(-10.,-30.+(float)i,0);
			v3f(50.,-30.+(float)i,0);
		}
		glEnd();	

		/* Axis */
		glColor3f( 1., 0., 0.);
		glBegin(GL_LINES);
			v3f( 0.0, 0.0, 0.0 );
			v3f( 10.0, 0.0, 0.0 );
		glEnd();
		glColor3f(0.,1.,0.);
		glBegin(GL_LINES);
			v3f( 0.0, 0.0, 0.0 );
			v3f( 0.0, 10.0, 0.0 );
		glEnd();
		glColor3f(0.,0.,1.);
		glBegin(GL_LINES);
			v3f( 0.0, 0.0, 0.0 );   
			v3f( 0.0, 0.0, 10.0 );
		glEnd();
	}

	bool DrawArea::on_motion_notify(GdkEventMotion* event) {
		float desp = 0.01;
		float x=event->x;
		float y=event->y;

		/* if left mouse button is toggled */
		if (event->state & GDK_BUTTON1_MASK) {
			if ((x - old_x) > 0.0) longi -= desp;
			else if ((x - old_x) < 0.0) longi += desp;

			if ((y - old_y) > 0.0) lati += desp;
			else if ((y - old_y) < 0.0) lati -= desp;

			this->glcam_pos.X=radius*cosf(lati)*cosf(longi) + this->glcam_foa.X;
			this->glcam_pos.Y=radius*cosf(lati)*sinf(longi) + this->glcam_foa.Y;
			this->glcam_pos.Z=radius*sinf(lati) + this->glcam_foa.Z;
		}
		/* if right mouse button is toggled */
		if (event->state & GDK_BUTTON3_MASK) {
			if ((x - old_x) > 0.0) longi -= desp;
			else if ((x - old_x) < 0.0) longi += desp;

			if ((y - old_y) > 0.0) lati += desp;
			else if ((y - old_y) < 0.0) lati -= desp;

			this->glcam_foa.X=-radius*cosf(lati)*cosf(longi) + this->glcam_pos.X;
			this->glcam_foa.Y=-radius*cosf(lati)*sinf(longi) + this->glcam_pos.Y;
			this->glcam_foa.Z=-radius*sinf(lati) + this->glcam_pos.Z;
		}

		old_x=x;
		old_y=y;
	}

	bool DrawArea::on_drawarea_scroll(GdkEventScroll * event) {
		float vx, vy, vz;

		vx = (this->glcam_foa.X - this->glcam_pos.X)/radius;
		vy = (this->glcam_foa.Y - this->glcam_pos.Y)/radius;
		vz = (this->glcam_foa.Z - this->glcam_pos.Z)/radius;

		if (event->direction == GDK_SCROLL_UP) {
			this->glcam_foa.X = this->glcam_foa.X + vx;
			this->glcam_foa.Y = this->glcam_foa.Y + vy;
			this->glcam_foa.Z = this->glcam_foa.Z + vz;

			this->glcam_pos.X = this->glcam_pos.X + vx;
			this->glcam_pos.Y = this->glcam_pos.Y + vy;
			this->glcam_pos.Z = this->glcam_pos.Z + vz;
		}

		if (event->direction == GDK_SCROLL_DOWN) {
			this->glcam_foa.X = this->glcam_foa.X - vx;
			this->glcam_foa.Y = this->glcam_foa.Y - vy;
			this->glcam_foa.Z = this->glcam_foa.Z - vz;

			this->glcam_pos.X = this->glcam_pos.X - vx;
			this->glcam_pos.Y = this->glcam_pos.Y - vy;
			this->glcam_pos.Z = this->glcam_pos.Z - vz;
		}
	}
} /*namespace*/

