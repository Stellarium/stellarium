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

#include "SpoutSender.hpp"
#include "../../external/SpoutLibrary.h"

Q_LOGGING_CATEGORY(spout,"stel.spout")

SpoutSender::SpoutSender(const QString &senderName)
	: valid(false), width(800), height(600), localTexture(0), textureDirty(true)
{
	initializeOpenGLFunctions();

	// Acquire spout object through C API
	spoutLib = GetSpout();
	//we have to copy the name, because the array should be at least 256 bytes
	qstrncpy(name, qPrintable(senderName), 256);

	qCDebug(spout) << "Sender name is:" << name;
	int numAdapters = spoutLib->GetNumAdapters();
	qCDebug(spout) << "Found" << numAdapters << "GPUs";
	for (int i=0; i<numAdapters; i++){
		char gpuName[256]; // 256 chars min required by Spout specs!
		spoutLib->GetAdapterName(i, gpuName, 256);
		qCDebug(spout) << "       GPU" << i << ": " << gpuName;
	}
	qCDebug(spout) << "Currently used: GPU" << spoutLib->GetAdapter();

	//ignore the sizes here, we have to call resize later
	valid = spoutLib->CreateSender(name,width,height);
	if(!valid)
	{
		qCCritical(spout) << "Could not create spout sender";
		qCCritical(spout) << "You may need a better GPU for this function, see Spout docs.";
		qCCritical(spout) << "On a notebook with NVidia Optimus, force running Stellarium on the NVidia GPU.";
		return;
	}

	//create the local GL texture
	glGenTextures(1, &localTexture);
	//setup some texture parameters
	//probably not needed because we are not sampling, but who knows
	glBindTexture(GL_TEXTURE_2D, localTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D, 0);

	qCDebug(spout) << "Sender has been created in" << (spoutLib->GetMemoryShareMode() ? "Memory Share Mode" : "OpenGL/DirectX interop mode");
	qCDebug(spout) << "DX9 mode:" << spoutLib->GetDX9();
}

SpoutSender::~SpoutSender()
{
	//probably ok to release even if invalid?
	spoutLib->ReleaseSender();
	// Release spout object, can't call any methods afterwards
	spoutLib->Release();
	spoutLib = Q_NULLPTR;

	glDeleteTextures(1, &localTexture);
	localTexture = 0;
	qCDebug(spout)<<"Sender"<<name<<"released";
}

void SpoutSender::captureAndSendFrame(GLuint fbo)
{
	if(!valid)
		return;

	if(fbo)
	{
		//if there is a FBO, try to use its texture directly to avoid a copy
		//the Qt GL widget classes hide their FBO internally,
		//so this seems like the only way to find out the texture ID
		//I don't know the performance implications of these queries yet
		GLint objType;
		glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&objType);
		if(objType == GL_TEXTURE) //we can only do this directly if it is a texture, not an RB
		{
			GLint texId;
			glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&texId);
			if(texId)
			{
				//use the texture directly
				spoutLib->SendTexture(texId, GL_TEXTURE_2D, width, height, true, fbo);
				return;
			}
		}
	}

	//if no FBO is bound, or the above method failed, we have
	//to copy from the framebuffer to a local texture first
	glBindTexture(GL_TEXTURE_2D, localTexture);
	if(textureDirty)
	{
		//recreate the texture by using glCopyTexImage2D
		//would GL_BGRA be better here?
		glCopyTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,0,0,width,height,0);

		//manual calling of UpdateSender seems buggy, it creates a new sender name
		//but we don't need to use it
		//SendTexture will update the sender automaticallly if the size is wrong, without these bugs
		//only possible drawback is the sent frame seems to be discarded, only next call will draw properly
		//spoutLib->UpdateSender(name,width,height);
		textureDirty = false;
	}
	else
	{
		//use glCopyTexSubImage2D, should give faster perf (in theory)
		glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,width,height);
	}
	//unbind tex
	glBindTexture(GL_TEXTURE_2D, 0);

	//send to spout
	spoutLib->SendTexture(localTexture, GL_TEXTURE_2D, width, height, true, fbo);
}

void SpoutSender::resize(uint width, uint height)
{
	//qCDebug(spout)<<"Resized output:"<<width<<height;
	this->width = width;
	this->height = height;

	//mark the texture for reinit, we don't do it here because we may not have GL access
	textureDirty = true;
}

