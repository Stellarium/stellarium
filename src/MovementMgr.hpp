#ifndef MOVEMENTMGR_H_
#define MOVEMENTMGR_H_

#include "StelModule.hpp"
#include "StelCore.hpp"
#include "Projector.hpp"

class MovementMgr : public StelModule
{
public:
	MovementMgr(StelCore* core);
	virtual ~MovementMgr();
	
	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init(const InitParser& conf, LoadingBar& lb);
	virtual void update(double deltaTime) {;}
	virtual string getModuleID() const {return "movements";}
	virtual double draw(Projector *prj, const Navigator *nav, ToneReproducer *eye) {return 0.;}
	virtual bool handleKeys(StelKey key, StelMod mod, Uint16 unicode, Uint8 state);
	virtual bool handleMouseMoves(Uint16 x, Uint16 y, StelMod mod);
	virtual bool handleMouseClicks(Uint16 x, Uint16 y, Uint8 button, Uint8 state, StelMod mod);
	virtual void selectedObjectChangeCallBack();
	
	//! Increment/decrement smoothly the vision field and position
	void updateMotion(double deltaTime);
	
	double getZoomSpeed() {return zoom_speed;}
	
	float getAutoMoveDuration() {return auto_move_duration;}
	//! Set whether auto zoom can go further than normal
	void setFlagManualAutoZoom(bool b) {FlagManualZoom = b;}
	//! Get whether auto zoom can go further than normal
	bool getFlagManualAutoZoom(void) {return FlagManualZoom;}
	
	//! Set object tracking
	void setFlagTracking(bool b);
	//! Get object tracking
	bool getFlagTracking(void) const {return flagTracking;}

	//! Move view in alt/az (or equatorial if in that mode) coordinates
	void panView(double deltaAz, double deltaAlt);

	//! Set automove duration in seconds
	void setAutomoveDuration(float f) {auto_move_duration = f;}
	//! Get automove duration in seconds
	float getAutomoveDuration(void) const {return auto_move_duration;}

	// Move to the given position in equatorial or local coordinate depending on _local_pos value
	void moveTo(const Vec3d& _aim, float move_duration = 1., bool _local_pos = false, int zooming = 0);

	//! Zoom to the given FOV (in degree)
	void zoomTo(double aim_fov, float move_duration = 1.);

	//! Go and zoom temporarily to the selected object.
	void autoZoomIn(float move_duration = 1.f, bool allow_manual_zoom = 1);

	//! Unzoom to the previous position
	void autoZoomOut(float move_duration = 1.f, bool full = 0);
	
	//! Set whether sky position is to be locked
	void setFlagLockEquPos(bool b) {flag_lock_equ_pos=b;}
	//! Set whether sky position is locked
	bool getFlagLockEquPos(void) const {return flag_lock_equ_pos;}

    //! If is currently zooming, return the target FOV, otherwise return current FOV
    double getAimFov(void) const {return (flag_auto_zoom ? zoom_move.aim : core->getProjection()->getFov());}
	
	//! Those are hopefully temporary
	bool getHasDragged() const {return has_dragged;}
	void stopDragging() {is_dragging=false; has_dragged=false;}
private:

	// Viewing direction function : 1 move, 0 stop.
	void turn_right(int);
	void turn_left(int);
	void turn_up(int);
	void turn_down(int);
	void zoom_in(int);
	void zoom_out(int);
	void change_fov(double deltaFov);
	   
	void updateVisionVector(double deltaTime); 
	void update_auto_zoom(double deltaTime); // Update auto_zoom if activated
	
	//! Make the first screen position correspond to the second (useful for mouse dragging)
	void dragView(int x1, int y1, int x2, int y2);
	
	StelCore* core;	// The core on which the movement are applied

	bool flag_lock_equ_pos;			// Define if the equatorial position is locked

	bool flagTracking;				// Define if the selected object is followed
	
	// Flags for mouse movements
	bool is_mouse_moving_horiz;
	bool is_mouse_moving_vert;

	int FlagEnableMoveMouse;  // allow mouse at edge of screen to move view
	int MouseZoom;
	
	int FlagEnableZoomKeys;
	int FlagEnableMoveKeys;

	// Automove
	// Struct used to store data for auto mov
	typedef struct
	{
		Vec3d start;
	    Vec3d aim;
	    float speed;
	    float coef;
		bool local_pos;				// Define if the position are in equatorial or altazimutal
	}auto_move;
	
	auto_move move;					// Current auto movement
    int flag_auto_move;				// Define if automove is on or off
	int zooming_mode;				// 0 : undefined, 1 zooming, -1 unzooming

	double deltaFov,deltaAlt,deltaAz;	// View movement
	double move_speed, zoom_speed;		// Speed of movement and zooming

	int FlagManualZoom;					// Define whether auto zoom can go further
	float auto_move_duration;			// Duration of movement for the auto move to a selected objectin seconds
	
	// Mouse control options
	bool is_dragging, has_dragged;
	int previous_x, previous_y;
	
	// Struct used to store data for auto zoom
	typedef struct
	{
		double start;
	    double aim;
	    float speed;
	    float coef;
	}auto_zoom;
	
	// Automove
	auto_zoom zoom_move;		// Current auto movement
	bool flag_auto_zoom;		// Define if autozoom is on or off
};

#endif /*MOVEMENTMGR_H_*/
