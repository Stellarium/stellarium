//
//  CMDevice.m
//  INDI Server
//
//  Created by Peter Polakovic on 13/01/14.
//  Copyright (c) 2014 CloudMakers, s. r. o. All rights reserved.
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Library General Public
//  License version 2 as published by the Free Software Foundation.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Library General Public License for more details.
//
//  You should have received a copy of the GNU Library General Public License
//  along with this library; see the file COPYING.LIB.  If not, write to
//  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
//  Boston, MA 02110-1301, USA.

#import "CMDevice.h"

@implementation CMDevice

- (id)initWithCoder:(NSCoder *)coder {
  self = [super init];
  if (self != nil) {
    _name = [coder decodeObjectForKey:@"name"];
    _driver = [coder decodeObjectForKey:@"driver"];
    _version = [coder decodeObjectForKey:@"version"];
    _prefix = [coder decodeObjectForKey:@"prefix"];
    _status = IDLE;
  }
  return self;
}

- (void)encodeWithCoder:(NSCoder *)coder {
  [coder encodeObject:_name forKey:@"name"];
  [coder encodeObject:_driver forKey:@"driver"];
  [coder encodeObject:_version forKey:@"version"];
  [coder encodeObject:_prefix forKey:@"prefix"];
}
  
- (NSString *)description {
  return [NSString stringWithFormat:@"%@ (%@)", self.name, self.version];
}

@end
