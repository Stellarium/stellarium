/*
 * Stellarium
 * Copyright (C) 2016 Georg Zotti, Florian Schaukowitsch
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

#include <QOpenGLFunctions>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(spout)

struct SPOUTLIBRARY;

//! Helper class to send rendered frames to Spout (http://spout.zeal.co/)
class SpoutSender : public QObject, protected QOpenGLFunctions
{
	Q_OBJECT
public:
	//! Initializes the Spout library and
	//! creates a sender with the specified name (restricted to 256 chars)
	//! Requires a valid GL context
	SpoutSender(const QString& senderName);
	//! Releases all held resources
	virtual ~SpoutSender() Q_DECL_OVERRIDE;
public slots:
	//! Captures a frame from the currently bound framebuffer,
	//! and sends it to Spout.
	//! Requires a valid GL context.
	//! @param fbo The ID of the current framebuffer, should match with GL state!
	void captureAndSendFrame(GLuint fbo);
	//! Informs the sender about changed buffer dimensions.
	//! Does not need a GL context.
	void resize(uint width, uint height);
	//! True if the sender has been successfuly created
	bool isValid() { return valid; }
private:
	SPOUTLIBRARY* spoutLib;
	bool valid;
	char name[256];
	uint width;
	uint height;
	GLuint localTexture;
	bool textureDirty;
};
