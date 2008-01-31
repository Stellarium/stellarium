/*
 * Stellarium
 * This file Copyright (C) 2005 Robert Spearman
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


#ifndef _IMAGE_MGR_H_
#define _IMAGE_MGR_H_

#include <vector>
#include <string>
#include "StelModule.hpp"
#include "image.h"

using namespace std;

/**
 * Manage images displayed at the request of the command interface (scripts)
 * @author Robert Spearman <rob@digitaliseducation.com>
 */
class ImageMgr : public StelModule
{

public:
    ImageMgr();
    virtual ~ImageMgr();

	///////////////////////////////////////////////////////////////////////////
	// Methods defined in the StelModule class
	virtual void init() {return;}
	virtual double draw(StelCore* core);
	virtual void update(double deltaTime);
	virtual double getCallOrder(StelModuleActionName actionName) const;
	
    int load_image(QString filename, string name, Image::IMAGE_POSITIONING position_type);
    int drop_image(string name);
    int drop_all_images();
    Image *get_image(string name);

private:
    vector<Image*> active_images;

};


#endif // _IMAGE_MGR_H
