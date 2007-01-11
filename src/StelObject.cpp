/*
 * Stellarium
 * Copyright (C) 2006 Johannes Gajdosik
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


#include "StelObject.hpp"
#include "StelObjectBase.hpp"

class StelObjectUninitialized : public StelObjectBase {
public:
  StelObjectUninitialized(void) {}
private:
  wstring getInfoString(const Navigator *nav) const {return L"";}
  wstring getShortInfoString(const Navigator *nav) const {return L"";}
  STEL_OBJECT_TYPE get_type(void) const {return STEL_OBJECT_UNINITIALIZED;}
  string getEnglishName(void) const {return "";}
  wstring getNameI18n(void) const {return L"";}
  Vec3d get_earth_equ_pos(const Navigator*) const {return Vec3d(1,0,0);}
  Vec3d getObsJ2000Pos(const Navigator*) const {return Vec3d(1,0,0);}
  float get_mag(const Navigator * nav) const {return -10;}
};

static StelObjectUninitialized uninitialized_object;

StelObject::~StelObject(void) {
  rep->release();
}

StelObject::StelObject(void)
           :rep(&uninitialized_object) {
  rep->retain();
}

StelObject::StelObject(StelObjectBase *r)
           :rep(r?r:&uninitialized_object) {
  rep->retain();
}

StelObject::StelObject(const StelObject &o)
           :rep(o.rep) {
  rep->retain();
}

const StelObject &StelObject::operator=(const StelObject &o) {
  if (this != &o) {
    rep = o.rep;
    rep->retain();
  }
  return *this;
}

StelObject::operator bool(void) const {
  return (rep != &uninitialized_object);
}

bool StelObject::operator==(const StelObject &o) const {
  return (rep == o.rep);
}

void StelObject::update(double deltaTime) {
  rep->update(deltaTime);
}

void StelObject::drawPointer(const Projector *prj,
                              const Navigator *nav) {
  rep->drawPointer(prj,nav);
}

wstring StelObject::getInfoString(const Navigator *nav) const {
  return rep->getInfoString(nav);
}

wstring StelObject::getShortInfoString(const Navigator *nav) const {
  return rep->getShortInfoString(nav);
}

STEL_OBJECT_TYPE StelObject::get_type(void) const {
  return rep->get_type();
}

string StelObject::getEnglishName(void) const {
  return rep->getEnglishName();
}

wstring StelObject::getNameI18n(void) const {
  return rep->getNameI18n();
}

Vec3d StelObject::get_earth_equ_pos(const Navigator *nav) const {
  return rep->get_earth_equ_pos(nav);
}

Vec3d StelObject::getObsJ2000Pos(const Navigator *nav) const {
  return rep->getObsJ2000Pos(nav);
}

float StelObject::get_mag(const Navigator *nav) const {
  return rep->get_mag(nav);
}

float StelObject::getSelectPriority(const Navigator *nav) const {
	return rep->getSelectPriority(nav);
}

Vec3f StelObject::get_RGB(void) const {
  return rep->get_RGB();
}

Vec3f StelObject::getInfoColor(void) const {
	return rep->getInfoColor();
}

double StelObject::get_close_fov(const Navigator *nav) const {
  return rep->get_close_fov(nav);
}

double StelObject::get_satellites_fov(const Navigator *nav) const {
  return rep->get_satellites_fov(nav);
}

double StelObject::get_parent_satellites_fov(const Navigator *nav) const {
  return rep->get_parent_satellites_fov(nav);
}

void StelObject::init_textures(void) {
  StelObjectBase::init_textures();
}

void StelObject::delete_textures(void) {
  StelObjectBase::delete_textures();
}
