/*******************************************************************************
 Copyright(c) 2016 CloudMakers, s. r. o.. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 *******************************************************************************/

#pragma once

#include "inditelescope.h"

class ScopeScript : public INDI::Telescope
{
  public:
    ScopeScript();
    virtual ~ScopeScript() = default;

    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
    virtual bool saveConfigItems(FILE *fp) override;

    void ISGetProperties(const char *dev) override;
    bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n) override;
    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual bool Handshake() override;

  protected:
    virtual bool MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command) override;
    virtual bool MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command) override;
    virtual bool Abort() override;

    virtual bool ReadScopeStatus() override;
    virtual bool Goto(double, double) override;
    virtual bool Sync(double ra, double dec) override;
    virtual bool Park() override;
    virtual bool UnPark() override;

  private:
    bool RunScript(int script, ...);

    ITextVectorProperty ScriptsTP;
    IText ScriptsT[15];
};
