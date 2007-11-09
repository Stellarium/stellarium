/*
    glh - is a platform-indepenedent C++ OpenGL helper library 


    Copyright (c) 2000 Cass Everitt
	Copyright (c) 2000 NVIDIA Corporation
    All rights reserved.

    Redistribution and use in source and binary forms, with or
	without modification, are permitted provided that the following
	conditions are met:

     * Redistributions of source code must retain the above
	   copyright notice, this list of conditions and the following
	   disclaimer.

     * Redistributions in binary form must reproduce the above
	   copyright notice, this list of conditions and the following
	   disclaimer in the documentation and/or other materials
	   provided with the distribution.

     * The names of contributors to this software may not be used
	   to endorse or promote products derived from this software
	   without specific prior written permission. 

       THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
	   POSSIBILITY OF SUCH DAMAGE. 


    Cass Everitt - cass@r3.nu
*/

#ifndef GLH_GLUT_H
#define GLH_GLUT_H

// some helper functions and object to
// make writing simple glut programs even easier! :-)

#ifdef MACOS
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <glh/glh_convenience.h>
#include <algorithm>
#include <list>

namespace glh
{

  class glut_interactor
  {
  public:
	glut_interactor() { enabled = true; }

	virtual void display() {}
	virtual void idle() {}
	virtual void keyboard(unsigned char key, int x, int y) {}
	virtual void menu_status(int status, int x, int y) {}
	virtual void motion(int x, int y) {}
	virtual void mouse(int button, int state, int x, int y) {}
	virtual void passive_motion(int x, int y) {}
	virtual void reshape(int w, int h) {}
	virtual void special(int  key, int x, int y) {}
	virtual void timer(int value) {}
	virtual void visibility(int v) {}

	virtual void enable()  { enabled = true; }
	virtual void disable() { enabled = false; }

	bool enabled;
  };

  std::list<glut_interactor *> interactors;
  bool propagate;

  void glut_display_function()
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
		(*it)->display();
  }

  void glut_idle_function()
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
		(*it)->idle();
  }

  void glut_keyboard_function(unsigned char k, int x, int y)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
	  (*it)->keyboard(k, x, y);
  }

  void glut_menu_status_function(int status, int x, int y)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
		(*it)->menu_status(status, x, y);
  }

  void glut_motion_function(int x, int y)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
		(*it)->motion(x, y);
  }

  void glut_mouse_function(int button, int state, int x, int y)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
	  (*it)->mouse(button, state, x, y);
  }
  
  void glut_passive_motion_function(int x, int y)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
		(*it)->passive_motion(x, y);
  }

  void glut_reshape_function(int w, int h)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
	  (*it)->reshape(w,h);
  }

  void glut_special_function(int k, int x, int y)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
	  (*it)->special(k, x, y);
  }

  void glut_timer_function(int v)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
	  (*it)->timer(v);
  }

  void glut_visibility_function(int v)
  {
	propagate = true;
	for(std::list<glut_interactor *>::iterator it=interactors.begin(); it != interactors.end() && propagate; it++)
	  (*it)->visibility(v);
  }

  // stop processing the current event 
  inline void glut_event_processed()
  {
	  propagate = false;
  }

  inline void glut_helpers_initialize()
  {
	glutDisplayFunc(glut_display_function);
	glutIdleFunc(0);
	glutKeyboardFunc(glut_keyboard_function);
	glutMenuStatusFunc(glut_menu_status_function);
	glutMotionFunc(glut_motion_function);
	glutMouseFunc(glut_mouse_function);
	glutPassiveMotionFunc(glut_passive_motion_function);
	glutReshapeFunc(glut_reshape_function);
	glutSpecialFunc(glut_special_function);
	glutVisibilityFunc(glut_visibility_function);
  }

  inline void glut_remove_interactor(glut_interactor *gi)
  {
	  std::list<glut_interactor *>::iterator it = 
		  std::find(interactors.begin(), interactors.end(), gi);
	if(it != interactors.end())
	  interactors.erase(it);
  }

  inline void glut_add_interactor(glut_interactor *gi, bool append=true)
  {
	glut_remove_interactor(gi);
	if(append)
		interactors.push_back(gi);
	else
		interactors.push_front(gi);
  }

  inline void glut_timer(int msec, int value)
  {
	glutTimerFunc(msec, glut_timer_function, value);
  }

  inline void glut_idle(bool do_idle)
  {
	glutIdleFunc(do_idle ? glut_idle_function : 0);
  }

  class glut_callbacks : public glut_interactor
  {
  public:
	glut_callbacks() :
		display_function(0),
		idle_function(0),
		keyboard_function(0),
		menu_status_function(0),
		motion_function(0),
		mouse_function(0),
		passive_motion_function(0),
		reshape_function(0),
		special_function(0),
		timer_function(0),
		visibility_function()
	{}

	virtual void display()
	{ if(display_function) display_function(); }

	virtual void idle()
	{ if(idle_function) idle_function(); }

	virtual void keyboard(unsigned char k, int x, int y)
	{ if(keyboard_function) keyboard_function(k, x, y); }

	virtual void menu_status(int status, int x, int y)
	{ if(menu_status_function) menu_status_function(status, x, y); }

	virtual void motion(int x, int y)
	{ if(motion_function) motion_function(x,y); }

	virtual void mouse(int button, int state, int x, int y)
	{ if(mouse_function) mouse_function(button, state, x, y); }

	virtual void passive_motion(int x, int y)
	{ if(passive_motion_function) passive_motion_function(x, y); }

	virtual void reshape(int w, int h)
	{ if(reshape_function) reshape_function(w, h); }

	virtual void special(int key, int x, int y)
	{ if(special_function) special_function(key, x, y); }

	virtual void timer(int value)
	{ if(timer_function) timer_function(value); }

	virtual void visibility(int v)
	{ if(visibility_function) visibility_function(v); }

	void (* display_function) ();
	void (* idle_function) ();
	void (* keyboard_function) (unsigned char, int, int);
	void (* menu_status_function) (int, int, int);
	void (* motion_function) (int, int);
	void (* mouse_function) (int, int, int, int);
	void (* passive_motion_function) (int, int);
	void (* reshape_function) (int, int);
	void (* special_function) (int, int, int);
	void (* timer_function) (int);
	void (* visibility_function) (int);

  };



  class glut_perspective_reshaper : public glut_interactor
  {
  public:
	  glut_perspective_reshaper(float infovy = 60.f, float inzNear = .1f, float inzFar = 10.f)
		  : fovy(infovy), zNear(inzNear), zFar(inzFar), aspect_factor(1) {}
	  
	  void reshape(int w, int h)
	  {
		  width = w; height = h;

		  if(enabled) apply();
	  }

	  void apply()
	  {
		  glViewport(0,0,width,height);
		  glMatrixMode(GL_PROJECTION);
		  glLoadIdentity();
		  apply_perspective();
		  glMatrixMode(GL_MODELVIEW);
	  }
	  void apply_perspective()
	  {
		  aspect = aspect_factor * float(width)/float(height);
          if ( aspect < 1 )
		  {
			  // fovy is a misnomer.. we really mean the fov applies to the
			  // smaller dimension
			  float fovx = fovy; 
			  float real_fov = to_degrees(2 * atan(tan(to_radians(fovx/2))/aspect));
              gluPerspective(real_fov, aspect, zNear, zFar);
		  }
          else
		    gluPerspective(fovy, aspect, zNear, zFar);
	  }
	  int width, height;
	  float fovy, aspect, zNear, zFar;
	  float aspect_factor;
  };

  // activates/deactivates on a particular mouse button
  // and calculates deltas while active
  class glut_simple_interactor : public glut_interactor
  {
  public:
	glut_simple_interactor()
	{
	  activate_on = GLUT_LEFT_BUTTON;
	  active = false;
	  use_modifiers = true;
	  modifiers = 0;
	  width = height = 0;
	  x0 = y0 = x = y = dx = dy = 0;
	}

	virtual void mouse(int button, int state, int X, int Y)
	{
	  if(enabled && button == activate_on && state == GLUT_DOWN &&
		 (! use_modifiers || (modifiers == glutGetModifiers())) )
	  {
		  active = true;
		  x = x0 = X;
		  y = y0 = Y;
		  dx = dy = 0;
	  }
	  else if (enabled && button == activate_on && state == GLUT_UP)
	  {
		if(dx == 0 && dy == 0)
			update();
		active = false;
		dx = dy = 0;
	  }
	}

	virtual void motion(int X, int Y)
	{
	  if(enabled && active)
	  {
		dx = X - x;   dy = y - Y;
		x = X;   y = Y;
		update();
	  }
	}

	void reshape(int w, int h)
	{
		  width = w; height = h;
	}

	virtual void apply_transform() = 0; 
	virtual void apply_inverse_transform() = 0;
	virtual matrix4f get_transform() = 0;
	virtual matrix4f get_inverse_transform() = 0;

	virtual void update() {}

	int activate_on;
	bool use_modifiers;
	int modifiers;
	bool active;
	int x0, y0;
	int x, y;
	int dx, dy;
	int width, height;
  };


  class glut_pan : public glut_simple_interactor
  {
  public:
	glut_pan()
	{
	  scale = .01f;
	  invert_increment = false;
	  parent_rotation = 0;
	}
	void update()
	{
	  vec3f v(dx, dy, 0);
	  if(parent_rotation != 0) parent_rotation->mult_vec(v);

	  if(invert_increment)
		  pan -= v * scale;
	  else
		  pan += v * scale;
	  glutPostRedisplay();
	}

	void apply_transform()
	{
	  //cerr << "Applying transform: " << (x - x0) << ", " << (y - y0) << endl;
	  glTranslatef(pan[0], pan[1], pan[2]);
	}
	
	void apply_inverse_transform()
	{
	  //cerr << "Applying transform: " << (x - x0) << ", " << (y - y0) << endl;
	  glTranslatef(-pan[0], -pan[1], -pan[2]);
	}

	matrix4f get_transform()
	{
		matrix4f m;
		m.make_identity();
		m.set_translate(pan);
		return m;
	}	

	matrix4f get_inverse_transform()
	{
		matrix4f m;
		m.make_identity();
		m.set_translate(-pan);
		return m;
	}	


	bool invert_increment;
	const rotationf * parent_rotation;
	vec3f pan;
	float scale;
  };

  
  class glut_dolly : public glut_simple_interactor
  {
  public:
	glut_dolly()
	{
	  scale = .01f;
	  invert_increment = false;
	  parent_rotation = 0;
	}
	void update()
	{
	  vec3f v(0,0,dy);
	  if(parent_rotation != 0) parent_rotation->mult_vec(v); 

	  if(invert_increment)
		  dolly += v * scale;
	  else
		  dolly -= v * scale;
	  glutPostRedisplay();
	}

	void apply_transform()
	{
	  //cerr << "Applying transform: " << (x - x0) << ", " << (y - y0) << endl;
	  glTranslatef(dolly[0], dolly[1], dolly[2]);
	}

	void apply_inverse_transform()
	{
	  //cerr << "Applying transform: " << (x - x0) << ", " << (y - y0) << endl;
	  glTranslatef(-dolly[0], -dolly[1], -dolly[2]);
	}
	
	matrix4f get_transform()
	{
		matrix4f m;
		m.make_identity();
		m.set_translate(dolly);
		return m;
	}	

	matrix4f get_inverse_transform()
	{
		matrix4f m;
		m.make_identity();
		m.set_translate(-dolly);
		return m;
	}	


	bool invert_increment;
	const rotationf * parent_rotation;
	vec3f dolly;
	float scale;
  };


  class glut_trackball : public glut_simple_interactor
  {
  public:
	glut_trackball()
	{
		r = rotationf(vec3f(0, 1, 0), 0);
		centroid = vec3f(0,0,0);
		scale = 1;
		invert_increment = false;
		parent_rotation = 0;
		legacy_mode = false;
	}

	void update()
	{
        if(dx == 0 && dy == 0)
		{
			incr = rotationf();
			return;
		}
		
		if(legacy_mode)
		{
			vec3f v(dy, -dx, 0);
			float len = v.normalize();
			if(parent_rotation != 0) parent_rotation->mult_vec(v);
			//r.mult_dir(vec3f(v), v);
			if(invert_increment)
				incr.set_value(v, -len * scale * -.01);
			else
				incr.set_value(v, len * scale * -.01);
		}
		else
		{
			float min = width < height ? width : height;
			min /= 2.f;
			vec3f offset(width/2.f, height/2.f, 0);
			vec3f a(x-dx, height - (y+dy), 0);
			vec3f b(   x, height -     y , 0);
			a -= offset;
			b -= offset;
			a /= min;
			b /= min;

			a[2] = pow(2.0f, -0.5f * a.length());
			a.normalize();
			b[2] = pow(2.0f, -0.5f * b.length());
			b.normalize();


			vec3f axis = a.cross(b);
			axis.normalize();

			float angle = acos(a.dot(b));

			if(parent_rotation != 0) parent_rotation->mult_vec(axis);

			if(invert_increment)
				incr.set_value(axis, -angle * scale);
			else
				incr.set_value(axis, angle * scale);
				
		}

		// fixme: shouldn't operator*() preserve 'r' in this case? 
		if(incr[3] != 0)
			r = incr * r;
		glutPostRedisplay();
	}

	void increment_rotation()
	{
		if(active) return;
		// fixme: shouldn't operator*() preserve 'r' in this case? 
		if(incr[3] != 0)
			r = incr * r;
		glutPostRedisplay();
	}

	void apply_transform()
	{
		glTranslatef(centroid[0], centroid[1], centroid[2]);
		glh_rotate(r);
		glTranslatef(-centroid[0], -centroid[1], -centroid[2]);
	}

	void apply_inverse_transform()
	{
		glTranslatef(centroid[0], centroid[1], centroid[2]);
		glh_rotate(r.inverse());
		glTranslatef(-centroid[0], -centroid[1], -centroid[2]);
	}


	matrix4f get_transform()
	{
		matrix4f mt, mr, minvt;
		mt.set_translate(centroid);
		r.get_value(mr);
		minvt.set_translate(-centroid);
		return mt * mr * minvt;
	}

	matrix4f get_inverse_transform()
	{
		matrix4f mt, mr, minvt;
		mt.set_translate(centroid);
		r.inverse().get_value(mr);
		minvt.set_translate(-centroid);
		return mt * mr * minvt;
	}

	bool invert_increment;
	const rotationf * parent_rotation;
	rotationf r;
	vec3f centroid;
	float scale;
	bool legacy_mode;
	rotationf incr;
  }; 

  
  class glut_rotate : public glut_simple_interactor
  {
  public:
	glut_rotate()
	{
	  rotate_x = rotate_y = 0;
	  scale = 1;
	}
	void update()
	{
	  rotate_x += dx * scale;
	  rotate_y += dy * scale;
	  glutPostRedisplay();
	}

	void apply_transform()
	{
	  glRotatef(rotate_x, 0, 1, 0);
	  glRotatef(rotate_y, -1, 0, 0);
	}

	void apply_inverse_transform()
	{
	  glRotatef(-rotate_y, -1, 0, 0);
	  glRotatef(-rotate_x, 0, 1, 0);
	}

	matrix4f get_transform()
	{
		rotationf rx(to_radians(rotate_x), 0, 1, 0);
		rotationf ry(to_radians(rotate_y), -1, 0, 0);
		matrix4f mx, my;
		rx.get_value(mx);
		ry.get_value(my);
		return mx * my;
	}	

	matrix4f get_inverse_transform()
	{
		rotationf rx(to_radians(-rotate_x), 0, 1, 0);
		rotationf ry(to_radians(-rotate_y), -1, 0, 0);
		matrix4f mx, my;
		rx.get_value(mx);
		ry.get_value(my);
		return my * mx; 
	}	

	float rotate_x, rotate_y, scale;
  };

  class glut_mouse_to_keyboard : public glut_simple_interactor
  {
  public:
	  glut_mouse_to_keyboard()
	  {
		  keyboard_function = 0;
		  pos_dx_key = neg_dx_key = pos_dy_key = neg_dy_key = 0;
	  }
	  void apply_transform() {}

	  void apply_inverse_transform() {}

	  matrix4f get_transform() {return matrix4f();}

	  matrix4f get_inverse_transform() {return matrix4f();}

	  void update()
	  {
		  if(!keyboard_function) return;
		  if(dx > 0)
			keyboard_function(pos_dx_key, x, y);
		  else if(dx < 0)
			keyboard_function(neg_dx_key, x, y);
		  if(dy > 0)
			keyboard_function(pos_dy_key, x, y);
		  else if(dy < 0)
			keyboard_function(neg_dy_key, x, y);		
	  }
	  unsigned char pos_dx_key, neg_dx_key, pos_dy_key, neg_dy_key;
	  void (*keyboard_function)(unsigned char, int, int);
  };

  inline void glut_exit_on_escape(unsigned char k, int x = 0, int y = 0)
  { if(k==27) exit(0); }

  struct glut_simple_mouse_interactor : public glut_interactor
  {

  public:
	glut_simple_mouse_interactor(int num_buttons_to_use=3)
	{
	  configure_buttons(num_buttons_to_use);
	  camera_mode = false;
	}

	void enable()
	{
		trackball.enable();
		pan.enable();
		dolly.enable();
	}

	void disable()
	{
		trackball.disable();
		pan.disable();
		dolly.disable();
	}

	void set_camera_mode(bool cam)
	{
		camera_mode = cam;
		if(camera_mode)
		{
			trackball.invert_increment = true;
			pan.invert_increment = true;
			dolly.invert_increment = true;
			pan.parent_rotation = & trackball.r;
			dolly.parent_rotation = & trackball.r;
		}
		else
		{
			trackball.invert_increment = false;
			pan.invert_increment = false;
			dolly.invert_increment = false;
			if(pan.parent_rotation == &trackball.r) pan.parent_rotation = 0;
			if(dolly.parent_rotation == &trackball.r) dolly.parent_rotation = 0;
		}
	}
	void configure_buttons(int num_buttons_to_use = 3)
	{
	  switch(num_buttons_to_use)
	  {
	  case 1:
		trackball.activate_on = GLUT_LEFT_BUTTON;
		trackball.modifiers = 0;

		pan.activate_on = GLUT_LEFT_BUTTON;
		pan.modifiers = GLUT_ACTIVE_SHIFT;


		dolly.activate_on = GLUT_LEFT_BUTTON;
		dolly.modifiers = GLUT_ACTIVE_CTRL;

		break;

	  case 2:
		trackball.activate_on = GLUT_LEFT_BUTTON;
		trackball.modifiers = 0;

		pan.activate_on = GLUT_MIDDLE_BUTTON;
		pan.modifiers = 0;


		dolly.activate_on = GLUT_LEFT_BUTTON;
		dolly.modifiers = GLUT_ACTIVE_CTRL;

		break;

	  case 3:
	  default:
		trackball.activate_on = GLUT_LEFT_BUTTON;
		trackball.modifiers = 0;

		pan.activate_on = GLUT_MIDDLE_BUTTON;
		pan.modifiers = 0;

		dolly.activate_on = GLUT_RIGHT_BUTTON;
		dolly.modifiers = 0;

		break;
	  }
	}

	virtual void motion(int x, int y)
	{ 

		trackball.motion(x,y);

		pan.motion(x,y);

		dolly.motion(x,y);
	}
	virtual void mouse(int button, int state, int x, int y)
	{
	  trackball.mouse(button, state, x, y);
	  pan.mouse(button, state, x, y);
	  dolly.mouse(button, state, x, y);
	}

	virtual void reshape(int x, int y)
	{
		trackball.reshape(x,y);
		pan.reshape(x,y);
		dolly.reshape(x,y);
	}

	void apply_transform()
	{
	  pan.apply_transform();
	  dolly.apply_transform();
	  trackball.apply_transform();
	}

	void apply_inverse_transform()
	{
	  trackball.apply_inverse_transform();
	  dolly.apply_inverse_transform();
	  pan.apply_inverse_transform();
	}

	matrix4f get_transform()
	{
	  return ( pan.get_transform()       *
			   dolly.get_transform()     *
			   trackball.get_transform() );
	}

	matrix4f get_inverse_transform()
	{
	  return ( trackball.get_inverse_transform() *
			   dolly.get_inverse_transform()     *
			   pan.get_inverse_transform()       );
	}

	void set_parent_rotation(rotationf *rp)
	{
		trackball.parent_rotation = rp;
		dolly.parent_rotation = rp;
		pan.parent_rotation = rp;
	}

	bool camera_mode;
	glut_trackball trackball;
	glut_pan  pan;
	glut_dolly dolly;
  };

}

#endif
