/*
 * Copyright (C) 2009 Fabien Chereau
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
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#ifndef STELSKYLAYER_HPP
#define STELSKYLAYER_HPP

#include <QObject>
#include <QString>
#include <QSharedPointer>
#include "StelCore.hpp"

#include "StelProjectorType.hpp"

class StelCore;

//! Abstract class defining the API to implement for all sky layer.
//! A sky layer is a graphical layer containing image or polygons displayed in the sky.
//! The StelSkyLayerMgr allows to set the display order for layers, as well as opacity.
class StelSkyLayer : public QObject
{
	Q_OBJECT
public:
	StelSkyLayer(QObject* parent=NULL) : QObject(parent), frameType(StelCore::FrameUninitialized) {;}

	//! Draws the content of the layer.
	virtual void draw(StelCore* core, class StelRenderer* renderer, StelProjectorP projector, float opacity=1.)=0;

	//! Return the short name to display in the loading bar.
	virtual QString getShortName() const =0;

	//! Return the short server name to display in the loading bar.
	virtual QString getShortServerCredits() const {return QString();}

	//! Return a hint on which key to use for referencing this layer.
	//! Note that the key effectively used may be different.
	virtual QString getKeyHint() const {return getShortName();}

	//! Return a human readable description of the layer with e.g.
	//! links and copyrights.
	virtual QString getLayerDescriptionHtml() const {return "No description.";}

	//! Set the reference frame type.
	void setFrameType(StelCore::FrameType ft) {frameType = ft;}

	//! Get the reference frame type.
	StelCore::FrameType getFrameType() {return frameType;}

signals:
	//! Emitted when loading of data started or stopped.
	//! @param b true if data loading started, false if finished.
	void loadingStateChanged(bool b);

	//! Emitted when the percentage of loading tiles/tiles to be displayed changed.
	//! @param percentage the percentage of loaded data.
	void percentLoadedChanged(int percentage);
private:
	//! Reference frametype for painter
	StelCore::FrameType frameType;
};

//! @file StelSkyLayerMgr.hpp
//! Define the classes needed for managing layers of sky elements display.

//! @typedef StelSkyLayerP
//! Shared pointer on a StelSkyLayer instance (implement reference counting)
typedef QSharedPointer<StelSkyLayer> StelSkyLayerP;

#endif // STELSKYLAYER_HPP
