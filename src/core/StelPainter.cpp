/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
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

#include "StelPainter.hpp"
#include <QtOpenGL>

#include "StelApp.hpp"
#include "StelUtils.hpp"

#include <QDebug>
#include <QString>
#include <QPainter>
#include <QMutex>
#include <QPaintEngine>

#ifndef NDEBUG
QMutex* StelPainter::globalMutex = new QMutex();
#endif

QPainter* StelPainter::qPainter = NULL;
QGLContext* StelPainter::glContext = NULL;

void StelPainter::setQPainter(QPainter* p)
{
	qPainter=p;
	if (p==NULL)
		return;

	if (p->paintEngine()->type() != QPaintEngine::OpenGL && p->paintEngine()->type() != QPaintEngine::OpenGL2)
	{
		qCritical("StelPainter::setQPainter(): StelPainter needs a QGLWidget to be set as viewport on the graphics view");
		return;
	}
	QGLWidget* glwidget = dynamic_cast<QGLWidget*>(p->device());
	if (glwidget && glwidget->context()!=glContext)
	{
		qCritical("StelPainter::setQPainter(): StelPainter needs to paint on a GLWidget with the same GL context as the one used for initialization.");
		return;
	}
}

void StelPainter::makeMainGLContextCurrent()
{
	Q_ASSERT(glContext!=NULL);
	Q_ASSERT(glContext->isValid());
	glContext->makeCurrent();
}

//GL-REFACTOR: This has been refactored into QGL2Renderer and will be removed after 
//the rest of StelPainter is refactored.
void StelPainter::initSystemGLInfo(QGLContext* ctx)
{
	
	Q_ASSERT(glContext==NULL);
	glContext = ctx;

	makeMainGLContextCurrent();
}
