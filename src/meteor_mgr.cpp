/*
 * Stellarium
 * This file Copyright (C) 2004 Robert Spearman
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "meteor_mgr.h"

// static variables
Projector* Meteor_mgr::projection = NULL;
navigator* Meteor_mgr::navigation = NULL;
tone_reproductor* Meteor_mgr::eye = NULL;

Meteor_mgr::Meteor_mgr( Projector *proj, navigator *nav, tone_reproductor* tr, int zhr, int maxv ) {

  projection = proj;
  navigation = nav;
  eye = tr;
  ZHR = zhr;
  max_velocity = maxv;

  // calculate factor for whole earth visible meteor rate per second since know visible area ZHR is for
  zhr_to_wsr = 28.44f/3600.f;

}

Meteor_mgr::~Meteor_mgr() {
}

void Meteor_mgr::set_ZHR(int zhr){
  ZHR = zhr;
}

int Meteor_mgr::get_ZHR(){
  return ZHR;
}

void Meteor_mgr::set_max_velocity(int maxv) {
  max_velocity = maxv;
}

void Meteor_mgr::update(int delta_time) {

  // step through and update all active meteors
  int n =0;
  for(vector<Meteor*>::iterator iter = active.begin(); iter != active.end(); ++iter) {
    n++;
    //printf("Meteor %d update\n", ++n);
    if( !( (*iter)->update(delta_time) ) ) {
      // remove dead meteor
      //      printf("Meteor \tdied\n");

      active.erase(iter);
      iter--;  // important!
    }
  }

  // only makes sense given lifetimes of meteors to draw when time_speed is realtime
  // integer speed multiplier would be nicer
  if( abs( navigation->get_time_speed() * 86400.f - 1.0f ) > 0.1f ) {
    // don't start any more meteors
    return;
  }

  /*
  // debug - one at a time
  if(active.begin() == active.end() ) {
    Meteor *m = new Meteor(projection, navigation, max_velocity);
    active.push_back(m);
  }
  */


  // if stellarium has been suspended, don't create huge number of meteors to
  // make up for lost time!
  if( delta_time > 500 ) {
    delta_time = 500;
  }

  // determine average meteors per frame needing to be created
  int mpf = (int)((double)ZHR*zhr_to_wsr*(double)delta_time/1000.0f + 0.5);
  if( mpf < 1 ) mpf = 1;

  int mlaunch = 0;
  for(int i=0; i<mpf; i++) {

    // start new meteor based on ZHR time probability
    double prob = (double)rand()/((double)RAND_MAX+1);
    if( ZHR > 0 && prob < ((double)ZHR*zhr_to_wsr*(double)delta_time/1000.0f/(double)mpf) ) {
      Meteor *m = new Meteor(projection, navigation, eye, max_velocity);
      active.push_back(m);
      mlaunch++;
    }
  }

  //  printf("mpf: %d\tm launched: %d\t(mps: %f)\t%d\n", mpf, mlaunch, ZHR*zhr_to_wsr, delta_time);


}


void Meteor_mgr::draw(void) {

  // step through and draw all active meteors
  for(vector<Meteor*>::iterator iter = active.begin(); iter != active.end(); ++iter) {
    (*iter)->draw();
  }

}

