/*
 GigE interface wrapper on araviss
 Copyright (C) 2016 Hendrik Beijeman (hbeyeman@gmail.com)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#ifndef CPP_ARV_BLACKFLY_H
#define CPP_ARV_BLACKFLY_H

class BlackFly : public ArvGeneric
{
  public:
    BlackFly(void *camera_device);
    bool connect();
    void exposure_start(void);

  protected:
    bool _configure(void);
    bool _get_initial_config();
    bool _set_initial_config();

  private:
    bool _custom_settings();
    void _fixup();
};

#endif
