# Copyright (C) 2001-2003 Dirk Huenniger dhun (at) astro (dot) uni-bonn (dot) de
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2.

"""
An INDI Client Library
======================
	It implements the INDI Protocol for Python (see U{http://indi.sourceforge.net/}) \n
	There are two major applications:
		- writing telescope control scripts
		- writing user interfaces for remote telescope control
	Supported platforms are:
		- Linux
		- Windows
		- any other platform supporting Python
		
	The simple way
	--------------
	The most easy way to write a script is demonstrated in the example below (same as file C{test.py})\n
	B{Important:} make sure you got an B{indiserver} running B{tutorial_two} from the indi distribution and that the C{Connection} is set to C{On}.

		>>> from indiclient import *
		>>> indi=indiclient("localhost",7624)
		>>> time.sleep(1)
		>>> indi.tell()
		Telescope Simulator CONNECTION Connection SwitchVector OneOfMany
			CONNECT On Switch On
			DISCONNECT Off Switch Off
		Telescope Simulator EQUATORIAL_COORD Equatorial J2000 NumberVector rw
			RA RA H:M:S Number 3.5
			DEC Dec D:M:S Number 0
		>>> print time.time()
		1126108172.88
		>>> indi.set_and_send_float("Telescope Simulator","EQUATORIAL_COORD","RA",2.0).wait_for_ok_timeout(60.0)
		>>> print time.time()
		1126108211.44
		>>> print indi.get_float("Telescope Simulator","EQUATORIAL_COORD","RA")
		2.0
		>>> print indi.get_text("Telescope Simulator","EQUATORIAL_COORD","RA")
		2:0:0.00
		>>> indi.set_and_send_text("Telescope Simulator","EQUATORIAL_COORD","RA","3:30:00").wait_for_ok_timeout(60.0)
		>>> print indi.get_float("Telescope Simulator","EQUATORIAL_COORD","RA")
		3.5
		>>> indi.quit()
	
	(if you omit the C{.wait_for_ok_timeout(60.0)} the command will still be send, but the function will not wait until the telescope moved to the new position.)
	If this suits you needs you may take a look at: 
		- L{indiclient.get_bool}
		- L{indiclient.get_float}
		- L{indiclient.get_text}
		- L{indiclient.set_and_send_bool}
		- L{indiclient.set_and_send_float}
		- L{indiclient.set_and_send_switchvector_by_elementlabel}
		- L{indiclient.set_and_send_text}
	
	\n

	The object oriented way
	-----------------------
	Otherwise can use a more object oriented approach (same as file C{testobj.py}):

		>>> from indiclient import *
		>>> indi=indiclient("localhost",7624)
		>>> vector=indi.get_vector("Telescope Simulator","CONNECTION")
		>>> vector.set_by_elementname("CONNECT")
		>>> indi.send_vector(vector)
		>>> vector.wait_for_ok()
		>>> vector.tell()
		Telescope Simulator CONNECTION Connection SwitchVector OneOfMany
			CONNECT On Switch On
			DISCONNECT Off Switch Off
		>>> vector.set_by_elementname("DISCONNECT")
		>>> vector.wait_for_ok()
		>>> vector.tell()
		Telescope Simulator CONNECTION Connection SwitchVector OneOfMany
			CONNECT On Switch Off
			DISCONNECT Off Switch On
		>>> element=vector.get_element("CONNECT")
		>>> print element.get_active()
		False
		>>> indi.quit()

	In order to do things like that you should first of all read the documentation of 
	classes in question (you will need roughly 30 minutes to do so) E{:}
		- The INDI object classes:
			- L{indivector}
			- L{indielement}
			- L{indiswitchvector}
			- L{indiblob}
			- L{indinumber}
			- L{indiswitch}
			- L{inditext}
		- Two important L{indiclient} methods:
			- L{indiclient.send_vector}
			- L{indiclient.get_vector}
			
	The event driven way
	--------------------
	Sometimes you want to act in a special way if a special element or a special vector has just been
	received. You can write custom handlers inheriting from the classes:
		- L{indi_custom_element_handler}
		- L{indi_custom_vector_handler}
	And add them with the functions:
		- L{indiclient.add_custom_element_handler}
		- L{indiclient.add_custom_vector_handler}
	A custom hander function for an element is demonstrated in the example below 
	(same as file C{testevent.py}):

		>>> from indiclient import *
		>>> class demohandler(indi_custom_element_handler):
		>>>	def on_indiobject_changed(self,vector,element):
		>>>		print "RA= "+element.get_text()
		>>>		print " has just been received on port "+str(self.indi.port)
		>>> indi=indiclient("localhost",7624)
		>>> print "Installing and calling hander"
		Installing and calling hander
		>>> indi.add_custom_element_handler(demohandler("Telescope Simulator","EQUATORIAL_COORD","RA"))
		RA= 1:0:0.00
		has just been received on port 7624
		>>> print "Done"
		Done
		>>> indi.set_and_send_float("Telescope Simulator","EQUATORIAL_COORD","RA",2.0)
		>>> time.sleep(0.0001)
		>>> indi.set_and_send_float("Telescope Simulator","EQUATORIAL_COORD","RA",1.0)
		>>> print "Staring hander"
		Staring hander
		>>> indi.process_events()
		RA= 1:0:0.00
		has just been received on port 7624
		RA= 1:0:0.00
		has just been received on port 7624
		>>> print "Done"
		Done
		>>> indi.quit()

		
	You have to call the L{indiclient.process_events} method during you main loop. As your handler will only be called
	during the L{indiclient.process_events} method. Your handler will be called once for each time the element was
	received. A main loop could for example look like this:
	
		>>> while True:
		>>>	do_my_stuff()
		>>>	indi.process_events() # here your custom handler is called
		>>>	time.sleep(0.01) # give some time to the operating system.
		RA= 1:0:0.00
		has just been received on port 7624
		RA= 1:0:0.00
		has just been received on port 7624
		
	There is a threaded process that continuesly listens for data from the server and adds it to the list of available indivectors.
	We still use the L{indiclient.process_events} sheme as GTK does not support threading well and L{gtkindiclient} is based on this library. 
	

	Events needed by dynamic clients
	--------------------------------
	If you want to build a dynamic client, you will want to install some more handlers using the methods:
		- L{indiclient.set_timeout_handler}
		- L{indiclient.set_message_handler}
		- L{indiclient.set_def_handlers}
	This is demonstrated in the example below (same as file C{testhandler.py}).
	
	>>> def def_vector(vector,indiclientobject):
	>>>	print "new vector defined by host: "+indiclientobject.host+" : "
	>>>	vector.tell()
	>>> def msg(message,indiclientobject):
	>>>	print "got message by host :"+indiclientobject.host+" : "
	>>>	message.tell()
	>>> indi=indiclient("localhost",7624)
	>>> indi.set_def_handlers(def_vector,def_vector,def_vector,def_vector,def_vector)
	>>> indi.set_message_handler(msg)
	>>> time.sleep(1)
	>>> indi.process_events()
	new vector defined by host: localhost :
	Telescope Simulator CONNECTION Connection SwitchVector OneOfMany
		CONNECT On Switch Off
		DISCONNECT Off Switch On
	new vector defined by host: localhost :
	Telescope Simulator EQUATORIAL_COORD Equatorial J2000 NumberVector rw
		RA RA H:M:S Number 0.180309
		DEC Dec D:M:S Number 0
	got message by host :localhost :
		INDImessage Telescope Simulator Telescope is disconnected
	>>> indi.quit()


@author: Dirk Huenniger
@organization: "Hoher List" observatory Daun (Germany)
@license: GNU General Public License as published by the Free Software Foundation, version 2
@contact: dhun (at) astro (dot) uni-bonn (dot) de
@version: 0.13
"""
# Copyright (C) 2001-2003 Dirk Huenniger dhun (at) astro (dot) uni-bonn (dot) de
#	This program is free software; you can redistribute it and/or
#	modify it under the terms of the GNU General Public License as
#	published by the Free Software Foundation, version 2.

import socket
import xml.parsers.expat
import string
import base64
import sys
import array
import os
import threading
import Queue
import copy
import math
import zlib
import time
import math

def _normalize_whitespace(text):
    """
    Remove redundant whitespace from a string
    @param text: a string containing any unnecessary whitespaces 
    @type text: str
    @return: the input string with exactly one whitespace between each word and no tailing ones.
    @rtype: StringType
    """
    return ' '.join(text.split())


class _indinameconventions:
	"""
	The INDI naming scheme.
	@ivar basenames : The possible "Basenames" of an L{indiobject} ["Text","Switch","Number","BLOB","Light"]
	@type basenames : list of StringType
	"""
	def __init__(self):
		self.basenames=["Text","Switch","Number","BLOB","Light"]
	def _get_defelementtag(self,basename):
		"""
		@param basename : The basename of the tag see (L{basenames})
		@type basename : StringType
		@return:  xml tag of an element that is send by the server when an C{IDDef*} function is called
		on vector which contains the element (like C{defText}).
		@rtype: StringType
		"""
		return "def"+basename
	def _get_defvectortag(self,basename):
		"""
		@param basename : The basename of the tag see (L{basenames})
		@type basename : StringType
		@return:  xml tag of a vector that is send by the server when an C{IDDef*} function is called(like C{defTextVector}).
		@rtype: StringType
		"""
		return "def"+basename+"Vector"
	def _get_setelementtag(self,basename):
		"""
		@param basename : The basename of the tag see (L{basenames})
		@type basename : StringType
		@return:		xml tag of an element that is send by the server when an C{IDSet*} function is called
		on vector which contains the element (like C{oneText}).
		@rtype: StringType
		"""
		return "one"+basename
	def _get_setvectortag(self,basename):
		"""
		@param basename : The basename of the tag see (L{basenames})
		@type basename : StringType
		@return:  xml tag of a vector that is send by the server when am C{IDSet*} function is called (like C{setTextVector}).
		@rtype: StringType
		"""
		return "set"+basename+"Vector"
	def _get_newelementtag(self,basename):
		"""
		@param basename : The basename of the tag see (L{basenames})
		@type basename : StringType
		@return:  xml tag of an element that is send by the client (like C{oneText}).
		@rtype: StringType
		"""
		return "one"+basename
	
	def _get_newvectortag(self,basename):
		"""
		@param basename : The basename of the tag see (L{basenames})
		@type basename : StringType
		@return:  xml tag of a vector that is send by the client (like C{newTextVector}).
		@rtype: StringType
		"""
		return "new"+basename+"Vector"
	def _get_message_tag(self):
		"""
		@return:  xml tag of an INDI message.
		@rtype: StringType
		"""
		return "message"	
	def _get_vector_repr(self,basename):
		"""
		@param basename : The basename of the tag see (L{basenames})
		@type basename : StringType
		@return:  printable representation of the type of the vector .
		@rtype: StringType 
		"""
		return basename+"Vector"
	def _get_element_repr(self,basename):
		"""
		@param basename : The basename of the tag see (L{basenames})
		@type basename : StringType
		@return:  printable representation of the type of the element .
		@rtype: StringType 
		"""
		return basename
		
class _inditagfactory(_indinameconventions):
	""" 
	A Class to create an L{indixmltag} from its XML representation
	@ivar dict : a dictionary mapping XML representations to the corresponding L{indixmltag} objects
	@type dict : DictType
	"""

	def __init__(self):
		""" 
		Constructor
		"""
		_indinameconventions.__init__(self)
		self.dict={}
		for i,basename in enumerate(self.basenames):
			inditag=indixmltag(True,False,False,i,inditransfertypes.idef)
			stringtag=self._get_defvectortag(basename)
			self.dict.update({stringtag: inditag})
			inditag=indixmltag(False,True,False,i,inditransfertypes.idef)
			stringtag=self._get_defelementtag(basename)
			self.dict.update({stringtag: inditag})

			inditag=indixmltag(True,False,False,i,inditransfertypes.iset)
			stringtag=self._get_setvectortag(basename)
			self.dict.update({stringtag: inditag})
			inditag=indixmltag(False,True,False,i,inditransfertypes.iset)
			stringtag=self._get_setelementtag(basename)
			self.dict.update({stringtag: inditag})

			inditag=indixmltag(True,False,False,i,inditransfertypes.inew)
			stringtag=self._get_newvectortag(basename)
			self.dict.update({stringtag: inditag})
			inditag=indixmltag(False,True,False,i,inditransfertypes.inew)
			stringtag=self._get_newelementtag(basename)
			self.dict.update({stringtag: inditag})
				
		inditag=indixmltag(False,False,True,None,inditransfertypes.inew)
		self.dict.update({"message": inditag})


	def create_tag(self,tag):
		"""
		@param tag : the XML tag denoting the vector 
		@type tag : StringType
		@return: An L{indixmltag} created according to the information given in  L{tag} 
		@rtype: L{indixmltag}
		"""
		if self.dict.has_key(tag):
			inditag=self.dict[tag]
			return copy.deepcopy(inditag)
		else:
			return None

class inditransfertype:
	"""
	This is object is used to denote whether the an object was sent from the client to the server or vice versa and
	whether the object is just being defined or if it  was defined earlier.
	"""
	None
	
class inditransfertypes(inditransfertype):
	"""
	A Class containing the different transfer types
	"""
	class inew(inditransfertype):
		"""The object is send from the client to the server"""
		None
	class idef(inditransfertype):
		"""
		The object is send from to the server to the client and the client is not expected to know about it already.
		Thus the server is just defining the object to the client. This corresponds to an C{def*} tag in the XML representation.
		Or  a client calling the C{IDDef*} function.
		"""
		None
	class iset(inditransfertype):
		"""
		The object is send from to the server to the client and the client is expected to know about it already.
		Thus the server is just setting new value for an existing object to the client. This corresponds to an C{set*} tag 
		in the XML representation. Or  a client calling the C{IDSet*}
		"""
		None

class indixmltag(_indinameconventions):
	"""
	Classifys a received INDI object by its tag. Provides functions to generate different versions of the tag for different
	ways of sending it (see L{inditransfertype}).	
	@ivar _is_vector : C{True} if the tag denotes an L{indivector}, C{False} otherwise
	@type _is_vector : BooleanType
	@ivar _is_element : C{True} if the tag denotes an L{indielement}, C{False} otherwise
	@type _is_element : BooleanType
	@ivar _is_message : C{True} if the tag denotes an L{indimessage}, C{False} otherwise
	@type _is_message : BooleanType
	@ivar _transfertype : the way the object has been transferred (see L{inditransfertype}). 
	@type _transfertype : L{inditransfertype}
	@ivar _index : The index of the basename of the object, in the L{basenames} list
	@type _index : IntType	
	"""
	def __init__(self,is_vector,is_element,is_message,index,transfertype):
		"""
		@param is_vector : C{True} if the tag shall denote an L{indivector}, C{False} otherwise
		@type is_vector : BooleanType
		@param is_element : C{True} if the tag shall denote an L{indielement}, C{False} otherwise
		@type is_element : BooleanType
		@param is_message : C{True} if the tag shall denote an L{indimessage}, C{False} otherwise
		@type is_message : BooleanType
		@param transfertype : the way the object has been transferred (see L{inditransfertype}). 
		@type transfertype : L{inditransfertype}
		@param index : The index of the basename of the object, in the L{basenames} list
		@type index : IntType
		"""
		_indinameconventions.__init__(self)
		self._is_vector=is_vector
		self._is_element=is_element
		self._is_message=is_message
		self._transfertype=transfertype
		self._index=index
		if index!=None:
			self._basename=self.basenames[index]
		if self.is_message():
			self._initial_tag=self._get_message_tag()
		else:
			self._initial_tag=self.get_xml(self.get_transfertype())
		
	def get_transfertype(self):
		"""
		@return:  The way the object has been transferred
		@rtype: L{inditransfertype}
		"""
		return self._transfertype
	
	def get_index(self):
		"""
		@return: The index of the basename of the object, in the L{basenames} list
		@rtype: IntType
		"""
		return self._index
		
	def is_vector(self):
		"""
		@return:  C{True}, if it denotes vector, C{False} otherwise
		@rtype: BooleanType
		"""
		return self._is_vector

	def is_element(self):
		"""
		@return:  C{True}, if it denotes an element, C{False} otherwise
		@rtype: BooleanType
		"""
		return self._is_element

	def is_message(self):
		"""
		@return:  C{True}, if it denotes a message, C{False} otherwise
		@rtype: BooleanType
		"""
		return self._is_message
	
	def get_initial_tag(self):
		"""
		@return:  A sting representing the tag specified by the parameters given to the initialiser
		@rtype: StringType
		"""
		return self._initial_tag
	
	def get_xml(self,transfertype):
		"""
		Returns the string the be used in the tags in the XML representation of the object
		(like C{defTextVector} or C{oneSwitch} or C{newLightVector}).
		@param transfertype : An object describing the way the generated XML data is going to be sent (see L{inditransfertype}).
		@type transfertype : L{inditransfertype}
		@return: An XML representation of the object
		@rtype: StringType
		"""
		if transfertype==inditransfertypes.inew:
			if self._is_vector: 
				return self._get_newvectortag(self._basename)
			if self._is_element: 
				return self._get_newelementtag(self._basename)
		if transfertype==inditransfertypes.iset:
			if self._is_vector: 
				return self._get_setvectortag(self._basename)
			if self._is_element: 
				return self._get_setelementtag(self._basename)
		if transfertype==inditransfertypes.idef:
			if self._is_vector: 
				return self._get_defvectortag(self._basename)
			if self._is_element: 
				return self._get_defelementtag(self._basename)
		if self._is_message:
			return self._get_message_tag()
			
	def get_type(self):
		"""
		Returns a string representing the type of the object denoted by this tag (like C{TextVector} or C{Number}). 
		@return: a string representing the type of the object denoted by this tag (like C{TextVector} or C{Number}).
		@rtype: StringType
		"""
		if self._is_vector: 
			return self._get_vector_repr(self._basename)
		if self._is_element: 
			return self._get_element_repr(self._basename)
		if self._is_message:
			return self._get_message_tag()
		
class _indiobjectfactory(_indinameconventions):
	""" 
	A Class to create L{indiobject}s from their XML attributes
	@ivar elementclasses : a list of classes derived from L{indielement} the index has to be synchronous with L{basenames}
	@type elementclasses : a list of L{indielement}
	@ivar vectorclasses : a list of classes derived from L{indielement} the index has to be synchronous with L{basenames}
	@type vectorclasses : a list of L{indivector}
	"""
	def __init__(self):
		""" 
		Constructor
		"""
		_indinameconventions.__init__(self)
		self.tagfactory=_inditagfactory()
		self.elementclasses=[inditext,indiswitch,indinumber,indiblob,indilight]
		self.vectorclasses=[inditextvector,indiswitchvector,indinumbervector,indiblobvector,indilightvector]
		
	def create(self,tag,attrs):
		"""
		@param tag : the XML tag denoting the vector 
		@type tag : StringType
		@param attrs : The XML attributes of the vector
		@type attrs : DictType
		@return: An indiobject created according to the information given in  L{tag}  and L{attrs}
		@rtype: L{indiobject}
		"""
		inditag=self.tagfactory.create_tag(tag)
		if inditag==None:
			return None
		if tag==self._get_message_tag():
			return indimessage(attrs)
		i=inditag.get_index()
		if inditag.is_element():
			vec=self.elementclasses[i](attrs,inditag)
			return self.elementclasses[i](attrs,inditag)
			
		if inditag.is_vector():
			return self.vectorclasses[i](attrs,inditag)			
		
class indipermissions:
	"""
	The indi read/write permissions.
	@ivar perm : The users read/write permissions for the  vector possible values are:
		- C{ro} (Read Only )
		- C{wo} (Write Only)
		- C{rw} (Read/Write)
	@type perm : StringType
	"""
	def __init__(self,perm):
		"""
		@param perm : The users read/write permissions for the  vector possible values are:
			- C{ro} (Read Only )
			- C{wo} (Write Only)
			- C{rw} (Read/Write)
		@type perm : StringType
		"""
		self.perm=perm
		
	def is_readable(self):
		"""
		@return: C{True} is the object is readable, C{False} otherwise
		@rtype: BooleanType
		"""		
		if (self.perm=="ro"  or self.perm=="rw"):
			return True
		else:
			return False
			
	def is_writeable(self):
		"""
		@return: C{True} is the object is writable, C{False} otherwise
		@rtype: BooleanType
		"""
		if (self.perm=="wo"  or self.perm=="rw"):
			return True
		else:
			return False
	
	def get_text(self):
		"""
		@return: a string representing the permissions described by this object
		@rtype: StringType
		"""
		if self.perm=="wo":
			return "write only"
		if self.perm=="rw":
			return "read and write"
		if self.perm=="ro":
			return "read only"
		

class indiobject:
	""" The Base Class for INDI objects (so anything that INDI can send or receive ) 
	@ivar tag: The XML tag of the INDI object (see L{indixmltag}).
	@type tag: L{indixmltag}
	"""
	def __init__(self,attrs,tag):
		"""
		@param tag: The XML tag of the vector (see L{indixmltag}).
		@type tag: L{indixmltag}
		@param attrs: The attributes of the XML version of the INDI object.
		@type attrs: DictType
		"""
		self.tag=tag
		
	def is_valid(self):
		"""
		Checks whether the object is valid. 
		@return:  C{True} if the object is valid, C{False} otherwise.
		@rtype: BooleanType
		"""
		return True
	
	def get_xml(self,transfertype):
		"""
		Returns an XML representation of the object
		@param transfertype : The L{inditransfertype} for which the XML code  shall be generated
		@type transfertype : {inditransfertype}
		@return:  an XML representation of the object
		@rtype: StringType		
		"""
		None

	def _check_writeable(self):
		"""
		Raises an exception if the object is not writable
		@return: B{None}
		@rtype: NoneType
		"""
		return True
		
	def update(self,attrs,tag):
		"""
		Update this element with data received form the XML Parser.
		@param attrs: The attributes of the XML version of the INDI object.
		@type attrs: DictType
		@param tag: The XML tag of the object (see L{indixmltag}).
		@type tag: L{indixmltag}
		@return: B{None}
		@rtype: NoneType
		"""
		self.tag=tag



class indinamedobject(indiobject):
	"""
	An indiobject that has got a name as well as a label
	@ivar name : name of the INDI object as given in the "name" XML attribute 
	@type name : StringType
	@ivar label : label of the INDI object as given in the "label" XML attribute
	@type label : StringType
	"""
	def __init__(self,attrs,tag):
		"""
		@param tag: The XML tag of the object (see L{indixmltag}).
		@type tag: L{indixmltag}
		@param attrs: The attributes of the XML version of the INDI object.
		@type attrs: DictType
		"""
		indiobject.__init__(self,attrs,tag)
		name = _normalize_whitespace(attrs.get('name', ""))
		label = _normalize_whitespace(attrs.get('label', ""))
		self.name=name
		if label=="":
			self.label=name
		else:
			self.label=label
	def getName(self):
		return self.name

class indielement(indinamedobject):
	""" The Base Class of any element of an INDI Vector \n
	@ivar _value : The value of the INDI object. Meaning character contained between the end of the 
		I{StartElement} and the beginning of the I{EndElement} in XML version. This may be coded in another format
		or compressed  and thus require some manipulation before it can be used.
	@type _value : StringType	
	@ivar _old_value : The old value of the object, the value it had when L{_get_changed} was called last time.
	@type _old_value : StringType		
	"""
		
	def __init__(self,attrs,tag):
		indinamedobject.__init__(self,attrs,tag)
		self._set_value('')
		self._old_value=self._value
		
	def _get_changed(self):
		"""
		@return: C{True} if the objects XML data was changed since the last L{_get_changed} was called, 
		C{False} otherwise.
		@rtype: BooleanType
		"""
		if self._old_value==self._value:
			return False
		else:
			self._old_value=self._value
			return True
		
	def set_float(self,num):
		"""
		@param num: The new value to be set.
		@type num: FloatType
		@return: B{None}
		@rtype: NoneType
		"""
		self._set_value(str(num))
	
	def _set_value(self,value):
		"""
		Sets the value variable of this object.
		@param value: A string to be copied into the L{_value}.
		@type value: DictType
		@return: B{None}
		@rtype: NoneType
		"""
		self._check_writeable()
		self._value=value

	def tell(self):
		""" 
		Prints all parameters of the object
		@return: B{None}
		@rtype: NoneType
		"""
		print "   ",self.name,self.label,self.tag.get_type(),self._value
			
	def get_text(self):
		"""
		@return: a string representation of it value
		@rtype: StringType
		"""
		return self._value
		
	def set_text(self,text):
		"""
		@param text: A string representation of the data to be written into the object.
		@type text: StringType
		@return: B{None}
		@rtype: NoneType
		"""
		self._check_writeable()
		self._set_value(str(text))
		
	def get_xml(self,transfertype):
		tag=self.tag.get_xml(transfertype)
		data="<"+tag+' name="'+self.name+'"> '+self._value+"</"+tag+"> "
		return data
	def updateByElement(self,element):
		self._set_value(element._value)
	
	
class indinumber(indielement):
	"""
	A floating point number with a defined format \n
	@ivar format : A format string describing the way the number is displayed 
	@type format : StringType
	@ivar _min : The minimum value of the number 
	@type _min : StringType
	@ivar _max : The maximum value of the number 
	@type _max : StringType
	@ivar _step : The step increment of the number  
	@type _step : StringType	
	"""
	def __init__(self,attrs,tag):
		self._value=""
		indielement.__init__(self,attrs,tag)
		self.format=_normalize_whitespace(attrs.get('format', ""))
		self._min=_normalize_whitespace(attrs.get('min', ""))
		self._max=_normalize_whitespace(attrs.get('max', ""))
		self._step=_normalize_whitespace(attrs.get('step', ""))
	
	def get_min(self):
		"""
		@return: The smallest possible value for this object
		@rtype: FloatType
		"""
		return float(self._min)

	def get_max(self):
		"""
		@return: The highest possible value for this object
		@rtype: FloatType
		"""
		return float(self._max)
	
	def get_step(self):
		"""
		@return: The stepsize
		@rtype: FloatType
		"""
		return float(self._step)
		

	def is_range(self):
		"""
		@return: C{True} if (L{get_step}>0) B{and} (L{get_range}>0)  , C{False} otherwise.
		@rtype: FloatType
		"""
		if self.get_step()<=0 or (self.get_range())<=0:
			return False
		return True
	
	def get_range(self):
		"""
		@return: L{get_max}-L{get_min}
		@rtype: FloatType
		"""
		return self.get_max()-self.get_min()
			
	def get_number_of_steps(self):
		"""
		@return: The number of steps between min and max, 0 if L{is_range}==False
		@rtype: IntType
		"""
		if self.is_range():
			return  int(floor(self.get_range()/self.get_step()))		
		else: 
			return 0
	def _set_value(self,value):
		try:
			a=float(value)
		except:
			return
		indielement._set_value(self,value)
	
	def get_float(self):
		"""
		@return: a float representation of it value
		@rtype: FloatType
		"""
		success=False
		while success==False:
			success=True
			try:
				x=float(self._value)
			except:
				success=False
				time.sleep(1)
				print "INDI Warning: invalid float", self._value
		return x
	
	def get_digits_after_point(self):
		"""
		@return: The number of digits after the decimal point in the string representation of the number.
		@rtype: IntType
		"""
		text=self.get_text()
		i=len(text)-1
		while i>=0:
			if text[i]==".":
				return (len(text)-i-1)
			i=i-1
		return 0
		
	def get_int(self):
		"""
		@return: an integer representation of it value
		@rtype: IntType
		"""
		return int(round(self.get_float()))
		
	def set_float(self,num):
		"""
		@param num: The new value to be set.
		@type num: FloatType
		@return: B{None}
		@rtype: NoneType
		"""
		if self.is_sexagesimal:
			self._set_value(str(num))
		else:
			self._set_value(self.format % num)
	
	def is_sexagesimal(self):
		"""
		@return: C{True} if the format property requires sexagesimal display 
		@rtype: BooleanType
		"""
		return (not (-1==string.find(self.format,"m")))
		
	def get_text(self):
		"""
		@return: a formated string representation of it value
		@rtype:  StringType
		"""
		if (-1==string.find(self.format,"m")):
			return self.format % self.get_float()
		else:
			return _sexagesimal(self.format,self.get_float())
	
	def set_text(self,text):
		"""
		@param text: A string representing the the new value to be set.
			Sexagesimal format is also supported
		@type text: StringType
		@return: B{None}
		@rtype: NoneType
		"""
		sex=[]
		sex.append("")
		sex.append("")
		sex.append("")
		nsex=0
		for i in range(len(text)):
			if text[i]==":":
				nsex=nsex+1
			else:
				sex[nsex]=sex[nsex]+text[i]
		val=0
		error=False
		if nsex>2: error=True
		for i in range(nsex+1):
			factor=1.0/pow(60,i)
			try:
				val=val+float(sex[i])*factor
			except:
				error=True
		if error==False:
			self.set_float(val)

class inditext(indielement):
	"""a (nearly) arbitrary text"""

class indilight(indielement):
	"""
	a status light
	@ivar _value : The overall operating state of the device. possible values are:
		- C{Idle} (device is currently not connected or unreachable)
		- C{Ok} (device is ready to do something) 
		- C{Busy} (device is currently busy doing something, and not ready to do anything else)
		- C{Alert} (device is responding, but at least one function does not work at the moment)
	@type _value : StringType
	"""
	def __init__(self,attrs,tag):
		self._value=""
		indielement.__init__(self,attrs,tag)
		if self.tag.is_vector():
			self._set_value("Alert")
			self._set_value(_normalize_whitespace(attrs.get('state', "")))
			indielement.__init__(self,attrs,tag)
		if self.tag.is_element():
			indielement.__init__(self,attrs,tag)
	
	def is_ok(self):
		"""
		@return: C{True} if the light indicates the C{Ok} state (device is ready to do something), C{False} otherwise
		@rtype:  BooleanType		
		"""
		if self._value=="Ok":
			return True
		else:
			return False
		
	def is_busy(self):
		"""
		@return: C{True} if the light indicates the C{Busy} state (device is currently busy doing something, 
		and not ready to do anything else) , C{False} otherwise
		@rtype:  BooleanType		
		"""
		if self._value=="Busy":
			return True
		else:
			return False
	
	def is_idle(self):
		"""
		@return: C{True} if the light indicates the C{Idle} state (device is currently not connected or unreachable)
		, C{False} otherwise
		@rtype:  BooleanType		
		"""
		if self._value=="Idle":
			return True
		else:
			return False

	def is_alert(self):
		"""
		@return: C{True} if the light indicates the C{Alert}state (device is responding, but at least 
		one function does not work at the moment) C{False} otherwise
		@rtype:  BooleanType		
		"""
		if self._value=="Alert":
			return True
		else:
			return False

	def _set_value(self,value):
		for state in ["Idle","Ok","Busy","Alert"]:
			if value==state:
				indielement._set_value(self,value)

	def set_text(self,text):
		raise Exception, "INDILigths are read only"
		#self._set_value(str(text))
	
	def update(self,attrs,tag):
		if self.tag.is_vectortag(tag):
			self._set_value("Alert")
			self._set_value(_normalize_whitespace(attrs.get('state', "")))
			indielement.update(self,attrs,tag)
		if self.tag.is_elmenttag(tag):
			indielement.update(self,attrs,tag)

class indiswitch(indielement):
	"""a switch that can be either C{On} or C{Off}"""		
	def get_active(self):
		"""
		@return: a Boolean representing the state of the switch:
			- True (I{Python}) = C{"On"} (I{INDI})
			- False (I{Python}) = C{"Off"} (I{INDI})
		@rtype: BooleanType 
		"""
		if self._value=="On":
			return True
		else:	
			return False
	def set_active(self,bool):
		"""
		@param bool: The boolean representation of the new state of the switch.
			- True (I{Python}) = C{"On"} (I{INDI})
			- False (I{Python}) = C{"Off"} (I{INDI})
		@type bool: BooleanType:
		@return: B{None}
		@rtype: NoneType
		"""
		if bool:
			self._set_value("On")
		else:
			self._set_value("Off")

class indiblob(indielement):
	"""
	@ivar format : A string describing the file-format/-extension (e.g C{.fits})
	@type format : StringType
	"""
	def __init__(self,attrs,tag):
		indielement.__init__(self,attrs,tag)
		self.format=_normalize_whitespace(attrs.get('format', ""))

	def _get_decoded_value(self):
		"""
		Decodes the value of the BLOB it does the base64 decoding as well as zlib decompression.
		zlib decompression is done only if the current L{format} string ends with C{.z}.
		base64 decoding is always done.
		@return: the decoded version of value
		@rtype: StringType
		"""
		if len(self.format)>=2:
			if self.format[len(self.format)-2]+self.format[len(self.format)-1]==".z":
				return zlib.decompress( base64.decodestring( self._value ))
			else:
				return base64.decodestring(self._value)
		else:
			return base64.decodestring(self._value)
	
	def _encode_and_set_value(self,value,format):
		"""
		Encodes the value to be written into the BLOB it does the base64 encoding as well as 
		zlib compression. Zlib compression is done only if the current L{format} string ends with C{.z}.
		base64 encoding is always done.
		@param value:  The value to be set, plain binary version.
		@type value: StringType 
		@param format:  The format of the value to be set.
		@type format: StringType 
		@return: B{None}
		@rtype: NoneType
		"""
		self.format=format
		if len(self.format)>=2:
			if self.format[len(self.format)-2]+self.format[len(self.format)-1]==".z":
				self._set_value( base64.encodestring( zlib.compress(value) ))
			else:
				self._set_value(base64.encodestring(value))
		else:
			self._set_value(base64.encodestring(value))
		
	def get_plain_format(self):
		"""
		@return: The format of the BLOB, possible extensions due to compression like C{.z} are removed
		@rtype: StringType
		"""
		if len(self.format)>=2:
			if self.format[len(self.format)-2]+self.format[len(self.format)-1]==".z":
				return self.format.rstrip(".z")
			else: 
				return self.format
		else:
			return self.format
			
	def get_data(self):
		"""
		@return: the plain binary version of its data 
		@rtype: StringType
		"""
		return self._get_decoded_value()

	def get_text(self):
		"""
		@return: the plain binary version of its data
		@rtype: StringType 
		"""
		return self._get_decoded_value()
		
	def set_from_file(self,filename):
		"""
		Loads a BLOB with data from a file. 
		The extension of the file is used as C{format} attribute of the BLOB
		@param filename:  The name of the file to be loaded.
		@type filename: StringType 
		@return: B{None}
		@rtype: NoneType
		"""
		in_file = open(filename,"r")
		text = in_file.read()
		in_file.close()
		(root,ext)=os.path.splitext(filename)
		self._encode_and_set_value(text,ext)
	
	def set_text(self,text):
		self._encode_and_set_value(text,".text")
	
	def get_size(self):
		"""
		@return: size of the xml representation of the data. This is usually not equal to the size of the string object returned by L{get_data}. Because blobs are
		base64 encoded and can be compressed.
		@rtype: StringType 
		"""
		return len(self._value)
	
	def set_from_string(self,text,format):
		"""
		Loads a BLOB with data from a string.
		@param text:  The string to be loaded into the BLOB.
		@type text: StringType 
		@param format:  A string to be used as the format attribute of the BLOB 
		@type format: 	StringType
		@return: B{None}
		@rtype: NoneType
		"""
		self._encode_and_set_value(text,format)
	
	def update(self,attrs,tag):
		self._check_writeable()
		indielement.update(self,attrs,tag)
		self.format=_normalize_whitespace(attrs.get('format', ""))

	def get_xml(self,transfertype):
		tag=self.tag.get_xml(transfertype)
		data="<"+tag+' name="'+self.name+'" size="'+str(self.get_size())+'" format="'+self.format+'"> '
		data=data+self._value+"</"+tag+"> "
		return data
	def updateByElement(self,element):
		self._set_value(element._value)
		self.format=element.format


class indivector(indinamedobject):
	"""
	The base class of all INDI vectors \n
	@ivar host : The hostname of the server that send the vector
	@type host : StringType
	@ivar port : The port on which the server send the vector
	@type port : IntType
	@ivar elements  : The list of L{indielement} objects contained in the vector
	@type elements  : list of L{indielement}
	@ivar _perm : The users read/write permissions for the  vector
	@type _perm : L{indipermissions}
	@ivar group : The INDI group the vector belongs to
	@type group : StringType
	@ivar _light : The StatusLED of the vector
	@type _light : L{indilight} 
	@ivar timeout  : The timeout value. According to the indi white paper it is defined as follows:
	Each Property has a timeout value that specifies the worst-case time it might
	take to change the value to something else The Device may report changes to the timeout
	value depending on current device status. Timeout values give Clients a simple
	ability to detect dysfunctional Devices or broken communication and also gives them 
	a way to predict the duration of an action for scheduling purposes as discussed later
	
	@type timeout  : StringType
	@ivar timestamp : The time when the vector was send out by the INDI server.
	@type timestamp : StringType
	@ivar device  : The INDI device the vector belongs to
	@type device  : StringType
	@ivar _message  : The L{indimessage} associated with the vector or B{None} if not present 
	@type _message  : L{indimessage}
	"""
	def __init__(self,attrs,tag):
		"""
		@param attrs: The attributes of the XML version of the INDI vector.
		@type attrs: DictType
		@param tag: The XML tag of the vector (see L{indixmltag}).
		@type tag: L{indixmltag}
		"""
		indinamedobject.__init__(self,attrs,tag)
		self.device= _normalize_whitespace(attrs.get('device', ""))
		self.timestamp=_normalize_whitespace(attrs.get('timestamp', ""))
		self.timeout=_normalize_whitespace(attrs.get('timeout', ""))
		self._light=indilight(attrs,tag)
		self.group=_normalize_whitespace(attrs.get('group', ""))
		self._perm= indipermissions(_normalize_whitespace(attrs.get('perm', "")))
		if attrs.has_key('message'):
				self._message=indimessage(attrs)
		else:
			self._message=None
		self.elements=[]
		self.port=None
		self.host=None
	
	def get_message(self):
		"""
		@return: The L{indimessage} associated with the vector, if there is any, B{None} otherwise 
		@rtype: L{indimessage}
		"""
		return self._message
	
	def _get_changed(self):
		"""
		@return: C{True} if the objects XML data was changed since the last L{_get_changed} was called, 
		C{False} otherwise.
		@rtype: BooleanType
		"""
		changed=False
		for element in self.elements:
			if element._get_changed():
				changed=True
		return changed

	def tell(self):
		"""" 
		Prints the most important parameters of the vector and its elements.
		@return: B{None}
		@rtype: NoneType
		"""
		print self.device,self.name,self.label,self.tag.get_type(),self._perm.get_text()
		for element in self.elements: 
			element.tell()
		
	def get_light(self):
		"""
		Returns the L{indilight} of the vector
		@return: L{indilight} of the vector
		@rtype: L{indilight}
		"""
		return self._light
		
	def get_permissions(self):
		"""
		Returns the read/write permission of the vector
		@return: the read/write permission of the vector
		@rtype: L{indipermissions}
		"""
		return self._perm
		
	def get_element(self,elementname):
		"""
		Returns an element on this vector matching a given name.
		@param elementname: The name of the element requested
		@type elementname: 	StringType
		@return: The element requested 
		@rtype: L{indielement} 
		"""
		for element in self.elements:
			if elementname==element.name :
				return element
	
	def get_first_element(self):
		"""
		Returns the first element on this vector.
		@return: The first element  
		@rtype: L{indielement} 
		"""
		return self.elements[0]

	def _wait_for_ok_general(self, checkinterval, timeout):
		"""
		Wait until its state is C{Ok}. Usually this means to wait until the server has
		finished the operation requested by sending this vector. 
		@param timeout: An exception will be raised if the no C{Ok} was received for longer than timeout
		since this method was called.
		@type timeout: FloatType
		@param checkinterval: The interval in which this method will check if the state is {Ok}
		@type checkinterval: FloatType		
		@return: B{None}
		@rtype: NoneType
		"""
		t=time.time()
		while not(self._light.is_ok()):
			time.sleep(checkinterval)
			if (time.time()-t)>timeout:
				raise Exception, ("timeout waiting for state to turn Ok "+
					"devicename=" +self.device+" vectorname= " + self.name+
					" "+str(timeout)+ " "+str(time.time()-t)
					)

	def wait_for_ok_timeout(self,timeout):
		"""
		@param timeout: The time after which the L{_light} property of the object has to turn ok .
		@type timeout: FloatType
		@return: B{None}
		@rtype:  NoneType
		"""
		checkinterval=0.1
		if (timeout/1000.0)<checkinterval:
			checkinterval=(timeout/1000.0)
		self._wait_for_ok_general(checkinterval,timeout)

	def wait_for_ok(self):
		"""
		Wait until its state is C{Ok}. Usually this means to wait until the server has
		finished the operation requested by sending this vector. 
		@return: B{None}
		@rtype: NoneType
		"""
		if float(self.timeout)==0.0:
			timeout=0.1
		else:
			timeout=float(self.timeout)
		checkinterval=0.1
		if (timeout/1000.0)<checkinterval:
			checkinterval=(timeout/1000.0)
		self._wait_for_ok_general(checkinterval,timeout)
	
	def update(self,attrs,tag):
		indinamedobject.update(self,attrs,tag)
		self._check_writeable()
		self.timestamp=_normalize_whitespace(attrs.get('timestamp', ""))
		self.timeout=_normalize_whitespace(attrs.get('timeout', ""))
		self._light=indilight(attrs,tag)
	
	def get_xml(self,transfertype):
		tag=self.tag.get_xml(transfertype)
		data="<"+tag+' device="'+self.device+'" name="'+self.name+'"> '
		for element in self.elements:
			data=data+element.get_xml(transfertype)
		data=data+"</"+tag+"> "
		return data
	def updateByVector(self,vector):
		self.timestamp=vector.timestamp
		self.timeout=vector.timeout
		self._light=vector._light
		#print len(vector.elements), len(self.elements)
		for oe in vector.elements:
			for e in self.elements:
				#print e.name,oe.name
				if e.name==oe.name:
					e.updateByElement(oe)
	def getDevice(self):
		return self.device
		

class indiswitchvector(indivector):
	"""
	a vector of switches \n
	@ivar rule: A rule defining which states of switches of the vector are allowed. possible values are:
		- C{OneOfMany} Exactly one of the switches in the vector has to be C{On} all others have to be C{Off}
		- C{AtMostOne}  At most one of the switches in the vector can to be C{On} all others have to be C{Off}
		- C{AnyOfMany} Any switch in the vector may have any state
	@type rule: StringType
	"""
	def __init__(self,attrs,tag):
		indivector.__init__(self,attrs,tag)
		self.rule = _normalize_whitespace(attrs.get('rule', ""))
		
	def tell(self):
		print self.device,self.name,self.label,self.tag.get_type(),self.rule
		for element in self.elements:
			element.tell()
	
	def set_by_elementlabel(self,elementlabel):
		"""
		Sets all L{indiswitch} elements of this vector to C{Off}. And sets the one who's label property matches L{elementlabel}
		to C{On} . If no matching one is found or at least two matching ones are found, nothing is done. 
		@param elementlabel: The INDI Label of the Switch to be set to C{On}
		@type elementlabel: StringType
		@return: B{None}
		@rtype: NoneType
		"""
		found=False
		for element in self.elements:
			if element.label==elementlabel:
				if found:
					return
				found=True
		if not found:
			return
		for element in self.elements:
			element.set_active(False)
			if element.label==elementlabel:
				element.set_active(True)

	def set_by_elementname(self,elementname):
		"""
		Sets all L{indiswitch} elements of this vector to C{Off}. And sets the one who's label property matches L{elementname}
		to C{On}. If no matching one is found or at least two matching ones are found, nothing is done.
		@param elementname: The INDI Name of the Switch to be set to C{On}
		@type elementname: StringType
		@return: B{None}
		@rtype: NoneType
		"""
		found=False
		for element in self.elements:
			if element.name==elementname:
				if found:
					return
				found=True
		if not found:
			return
		for element in self.elements:
			element.set_active(False)
			if element.name==elementname:
				element.set_active(True)
			
	def get_active_element(self):
		"""
		@return: The first active (C{On}) element, B{None} if there is none
		@rtype: L{indiswitch}
		"""
		for element in self.elements:
			if element.get_active():
				return element
		return None

	def set_active_index(self,index):
		"""
		Turns the switch with index L{index} to C{On} and all other switches of this vector to C{Off}.
		@param index: the index of the switch to turned C{On} exclusively
		@type index: IntType
		@return: B{None}
		@rtype: NoneType
		"""
		for i,element in enumerate(self.elements):
			if i==index:
				element.set_active(True)
			else:
				element.set_active(False)

	def get_active_index(self):
		"""
		@return:  the index of the first switch in the Vector that is C{On}
		@rtype: IntType
		"""
		for i,element in enumerate(self.elements):
			if element.get_active():
				return i
		return None
		
class indinumbervector(indivector):
	"""A vector of numbers """

class indiblobvector(indivector):
	"""A vector of BLOBs """	
	
class inditextvector(indivector):
	"""A vector of texts"""

class indilightvector(indivector):
	"""A vector of lights """
	def __init__(self,attrs,tag):
		newattrs=attrs.copy()
		newattrs.update({"perm": 'ro'})
		indivector.__init__(self,newattrs,tag)
	def update(self,attrs):
		newattrs=attrs.copy()
		newattrs.update({"perm": "ro"})
		indivector.update(self,newattrs,tag)
		
class indimessage(indiobject):
	"""
	a text message.
	@ivar device:  The INDI device the message belongs to. 
	@type device: StringType
	@ivar timestamp: The time when the message was send out by the INDI server
	@type timestamp: StringType
	@ivar _value: The INDI message send by the server
	@type _value: StringType
	"""	
	def __init__(self,attrs):
		"""
		@param attrs: The attributes of the XML version of the INDI message.
		@type attrs: DictType
		"""
		tag=indixmltag(False,False,True,None,inditransfertypes.inew)
		indiobject.__init__(self,attrs,tag)
		self.device= _normalize_whitespace(attrs.get('device', ""))
		self.timestamp=_normalize_whitespace(attrs.get('timestamp', ""))
		self._value = _normalize_whitespace(attrs.get('message', ""))
	def tell(self):
		"""
		print the message to the screen
		@return: B{None}
		@rtype: NoneType
		"""
		print "    "+"INDImessage " +self.device+" "+self.get_text()
	
	def get_text(self):
		"""
		@return: A text representing the message received
		@rtype: StringType
		"""
		return self._value
	def is_valid(self):
		"""
		@return: C{True} if the message is valid.
		@rtype: StringType
		"""
		return self._value!=""
		
class _indilist(list):
	"""" A list with a more sophisticated append() function. 
	It checks for the existence an object with the same name overwrites it, if there is one. Will become a dictionary  soon :-)"""
	def __init__(self):
		self.list=[]
	def append(self,element):
		"""
		We check for an element within the list with the same name as the new element to be added 
		an overwrite the old element, if one is found. Otherwise we just add the new element.
		@param element: the element has to be added to the list.
		@type element : any type that has an attribute name
		@return: B{None}
		@rtype: NoneType
		"""
		deleted=1
		while deleted==1:
			deleted=0;
			for i, v in enumerate(self.list):
				if (element.name==v.name):
					del self.list[i]
					deleted=1;
					break
		self.list.append(element)


class _blocking_indi_object_handler:
	"""
	This very abstract class makes sure that something can be blocked while the handler for the 
	indi object L{on_indiobject_changed} is executed. Its does not define what shall be blocked or what blocking actually means.
	So for itself it just does nothing with nothing.  Classes inheriting from this class use it to detect "GUI changed" signals that
	are caused by indiclient derived classes changing the gui, (and could cause client server loopbacks, or data losses if not treated properly).
	@ivar _blocked: A counter incremented each time the L{_block} method is called and decremented by L{_unblock}, >0 means blocked, ==0 mean unblocked 
	@type _blocked: IntType
	"""
	def __init__(self):
		self._blocked=0
	def _block(self):
		"""
		activates the block. 
		@return: B{None}
		@rtype:  NoneType
		"""
		self._blocked=self._blocked+1
	def _unblock(self):
		"""
		releases the block. You have to call it as many times as you called L{_block} in order to release it, otherwise it will stay blocked.
		@return: B{None}
		@rtype:  NoneType
		"""
		if self._blocked>0:
			self._blocked=self._blocked-1
	def _is_blocked(self):
		"""
		@return: C{True} if blocked , C{False} otherwise
		@rtype:  BooleanType
		"""
		return (self._blocked>0)
	def indi_object_change_notify(self,*args):
		"""
		This method activates the block, calls the method  L{on_indiobject_changed}
		(It is called by L{bigindiclient.process_events} each time an INDI object has been received)
		@param args: The L{indiobject} (or indiobjects) that shall be used to update the GUI object
		@type args: list
		@return: B{None}
		@rtype:  NoneType		
		"""
		self._block()
		self.on_indiobject_changed(*args)
		self._unblock()
	def on_indiobject_changed(self,*args):
		"""
		While this function is called the block is active. 
		B{Implement your custom hander here!!} (by overloading this function)
		@param args: The L{indiobject} (or indiobjects) that shall be used to update the GUI object
		@type args: list
		@return: B{None}
		@rtype:  NoneType
		"""
		None
	def configure(self,*args):
		"""
		This method will be called at least once by L{indiclient}. It will be called before L{on_indiobject_changed} has been called for the first time.
		It will be called with the same parameters as L{on_indiobject_changed}. It can be implemented to do some lengthy configuration of some object.
		@param args: The L{indiobject} (or indiobjects) that shall be used to configure the GUI object
		@type args: list
		@return: B{None}
		@rtype:  NoneType		
		"""
		None
		
class gui_indi_object_handler(_blocking_indi_object_handler):
	"""
	This class provides two abstract handler functions. One to handle changes of (not 
	necessary graphical) user interfaces L{on_gui_changed}. And another one  
	L{on_indiobject_changed} to handle changes of INDIobjects received from the INDI server.
	It allows to detect if the method L{on_gui_changed} is in asked to be called,  while the method 
	L{on_indiobject_changed} is running. This situation has to be take special care of, as loopbacks or data losses
	are likely to happen if one does not take care.
	In order to allow this mechanism to work
	every function must B{not} call on_gui_changed directly, but any function may call 
	L{_blocking_on_gui_changed} instead. So if you are writing a custom handler please link the callback of
	your GUI to L{_blocking_on_gui_changed} and implement it in L{on_gui_changed}
	"""
	def _blocking_on_gui_changed(self,*args):
		"""
		Called by the GUI whenever the widget associated with this handler has changed. If the method on_indiobject_changed is currently active,
		the method L{on_blocked} is called otherwise (which is the far more usual case) the method L{on_gui_changed} is called.
		B{Important:} link your GUI callback signal to this function but implement it in L{on_gui_changed} (see L{on_blocked} if you want to know why)
		@param args: The arguments describing the change of the GUI object(s)
		@type args: list
		@return: B{None}
		@rtype:  NoneType		
		"""
		if not self._is_blocked():
			self.on_gui_changed(*args)
		else:
			self.on_blocked(*args)
	def on_blocked(self,*args):
		"""
		The method L{_blocking_on_gui_changed} is called by the GUI, in order to inform us, that a widget has changed.
		If the blocked property is C{True}, when L{_blocking_on_gui_changed} is called by the GUI. This method (L{on_blocked}) is called and nothing else done.
		The blocked property is C{True} while the  L{on_indiobject_changed} function is running, so and indiobject has been received from the server
		and L{on_indiobject_changed} is currently changing the widget associated with it. So there is a question: Was L{_blocking_on_gui_changed} called by the GUI?
		because  L{on_indiobject_changed} changed the widget or was it called because the user changed something? 
		Well, we don't know! And this is bad.
		Because if L{on_indiobject_changed} did it and we send the new state to the server, the server will reply, which will in turn trigger
		L{on_indiobject_changed} which will cause the GUI to emit another L{_blocking_on_gui_changed} signal and finally call L{on_blocked}.
		This way we have generated a nasty loopback. But if we don't send the new state to the server and the change was caused by the user, the user 
		will see the widget in the new state, but the server will not know about it, so the user has got another idea of the status of the system than the server
		and this is also quite dangerous. For the moment we print a warning and send the signal to the server, so we use the solution that might cause a loopback,
		but we make sure that the GUI and the server have got the same information about the status of the devices. One possible solution to this problem that
		causes neither of the problems is implemented in the L{gtkindiclient._comboboxentryhandler} class. We do no send the new state to the server. But we load the latest state
		we received from the server and set the gui to this state. So the user might have clicked at a GUI element, and it might not have changed in the proper
		way. We tested this for the combobox and the togglebutton and it worked.
		@param args: The arguments describing the change of the GUI object(s)
		@type args: list
		@return: B{None}
		@rtype:  NoneType		
		"""
		print "indiclient warning: signal received from GUI while changing gui element"
		print "Danger of loopback!!!!"
		self.on_gui_changed(*args)
	def on_gui_changed(self,*args):
		"""
		B{Important:} Implement your GUI callback here but link you GUI callback signal to {_blocking_on_gui_changed}
		(see L{on_blocked} if you want to know why)
		@param args: The arguments describing the change of the GUI object(s)
		@type args: list
		@return: B{None}
		@rtype:  NoneType
		"""
		None
		
	def set_bidirectional(self):
		"""
		installs callbacks of the GUI that will call the function L{_blocking_on_gui_changed} if the user changes the the GUI object associated 
		with this L{gui_indi_object_handler} . 
		The function L{_blocking_on_gui_changed} will usually call the function L{on_gui_changed}. The function L{on_gui_changed} has to update and send the 
		INDI object associated with this L{gui_indi_object_handler}. 
		@return: B{None}
		@rtype: NoneType
		"""
		return None
	def unset_bidirectional(self):
		"""
		uninstalls the GUI callback installed with L{set_bidirectional} (see L{set_bidirectional} for details)
		@return: B{None}
		@rtype: NoneType
		"""
		return None
		
class indi_vector_identifier:
	"""
	@ivar devicename : The name of the device contaning the vector to be uniquely identifyed by this object 
	@type devicename : StringType
	@ivar vectorname  : The name of the L{indivector} to be uniquely identifyed by this object
	@type vectorname  : StringType
	"""
	def __init__(self,devicename,vectorname):
		"""
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname: The name of the L{indivector}
		@type vectorname: StringType
		"""
		self.devicename=devicename
		self.vectorname=vectorname
	

class indi_element_identifier(indi_vector_identifier):
	"""
	@ivar devicename : The name of the device this handler is associated with
	@type devicename : StringType
	@ivar vectorname  : The name of the indivector this handler is associated with
	@type vectorname  : StringType
	@ivar elementname : The name of the indielement this handler is associated with
	@type elementname : StringType
	"""
	def __init__(self,devicename,vectorname,elementname):
		"""
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname: The name of the Indivector
		@type vectorname: StringType
		@param elementname: name of the INDI element
		@type elementname: StringType
		"""
		indi_vector_identifier.__init__(self,devicename,vectorname)
		self.elementname=elementname

	#def get_element(self):
	#	return self.get_vector().get_element(self.elementname)

class indi_custom_element_handler(gui_indi_object_handler,indi_element_identifier):
	"""
	A base class for a custom handler that will be called each time a specified INDI element is received.
	You have to write a class, inheriting from this one and to overload the L{on_indiobject_changed} method in order to add a custom handler. \n
	@ivar indi : The indiclient instance calling this handler (will be set by indiclient automatically)
	@type indi : L{indiclient}
	@ivar devicename : The name of the device this handler is associated with
	@type devicename : StringType
	@ivar vectorname  : The name of the indivector this handler is associated with
	@type vectorname  : StringType
	@ivar elementname : The name of the indielement this handler is associated with
	@type elementname : StringType
	@ivar type  : The type of the handler
	@type type  : StringType
	"""
	def __init__(self,devicename,vectorname,elementname):
		"""
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname: The name of the Indivector
		@type vectorname: StringType
		@param elementname: name of the INDI element
		@type elementname: StringType
		"""
		gui_indi_object_handler.__init__(self)
		indi_element_identifier.__init__(self,devicename,vectorname,elementname)
		self.type="INDIElementHandler"
	
	def configure(self,vector,element):
		"""
		This hander method will be called only once. It will be called before the L{on_indiobject_changed} method is called for the 
		first time. It will be called as soon as B{vector} has been received for the first time.
		It can be used to do some lengthy calculations in order to set up related data structures that need to be done
		only once and require information from the associated L{indivector} and L{indielement} objects. 
		@param vector:  A copy of the L{indivector} that has been received.  
		@type vector: L{indivector}		
		@param element:  A copy of the L{indielement} that has been received
		@type element: L{indielement}	
		@return: B{None}
		@rtype: NoneType		
		"""
		None

	def on_indiobject_changed(self,vector,element):
		"""
		This hander method will be called each time the specified INDI element/vector is received.
		You have to write a class inheriting from this class and overload this function.
		@param vector:  A copy of the L{indivector} that has been received.  
		@type vector: L{indivector}		
		@param element:  A copy of the L{indielement} that has been received
		@type element: L{indielement}	
		@return: B{None}
		@rtype: NoneType		
		"""
		None
		
	def get_vector(self):
		"""
		Returns the L{indivector} this handler is associated with.
		@return: the L{indivector} this handler is associated with
		@rtype: L{indivector}
		"""
		return self.indi.get_vector(self.devicename,self.vectorname)	

	def get_element(self):
		"""
		Returns the L{indielement} this handler is associated with.
		@return: the L{indielement} this handler is associated with
		@rtype: L{indielement}
		"""
		return self.indi.get_vector(self.devicename,self.vectorname).get_element(self.elementname)	

class mini_element_handler(indi_custom_element_handler):
	"""A class for custom handlers that consists only of a single function, instead of a whole class.
	@ivar handlermethod : The function to be called
	@type handlermethod : function
	"""
	def __init__(self,devicename,vectorname,elementname,handlermethod):
		"""
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname: The name of the vector
		@type vectorname: StringType
		@param elementname : The name of the element 
		@type elementname : StringType 
		@param handlermethod : The function to be called
		@type handlermethod : function
		"""
		indi_custom_element_handler.__init__(self,devicename,vectorname,elementname)
		self.handlermethod=handlermethod
	def on_indiobject_changed(self,vector,element):
		"""
		This method simply calls L{handlermethod} with L{element} as the only parameter.
		@param vector:  A copy of the L{indivector} that has been received.  
		@type vector: L{indivector}		
		@param element:  A copy of the L{indielement} that has been received
		@type element: L{indielement}	
		@return: B{None}
		@rtype: NoneType	
		"""
		self.handlermethod(element)

class indi_custom_vector_handler(gui_indi_object_handler,indi_vector_identifier):
	"""A base class for a custom handler that will be called each time a specified INDI vector is received.
	@ivar indi : The indiclient instance calling this handler (will be set by indiclient automatically)
	@type indi : L{indiclient}
	@ivar devicename : The name of the device this handler is associated with
	@type devicename : StringType
	@ivar vectorname  : The name of the indivector this handler is associated with
	@type vectorname  : StringType
	@ivar type  : The type of the handler
	@type type  : StringType
	"""
	def __init__(self,devicename,vectorname):
		"""
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname: The name of the Indivector
		@type vectorname: StringType
		"""
		gui_indi_object_handler.__init__(self)
		indi_vector_identifier.__init__(self,devicename,vectorname)
		self.devicename=devicename
		self.vectorname=vectorname
		self.type="INDIVectorHandler"

	def configure(self,vector):
		"""
		This hander method will be called only once. It will be called before the L{on_indiobject_changed} method is called for the 
		first time. It will be called as soon as L{vector} has been received for the first time.
		It can be used to do some lengthy calculations in order to set up related data structures that need to be done
		only once and require information from the associated L{indivector} and L{indielement} objects. 
		@param vector:  A copy of the L{indivector} that has been received.  
		@type vector: L{indivector}		
		@return: B{None}
		@rtype: NoneType		
		"""
		None

	def get_vector(self):
		"""
		Returns the L{indivector} this handler is associated with.
		@return: the L{indivector} this handler is associated with
		@rtype: L{indivector}	
		"""
		return self.indi.get_vector(self.devicename,self.vectorname)


	def on_indiobject_changed(self,vector):
		"""
		This hander method will be called each time a specified INDI element/vector is received.
		You have write a class inheriting from this class and overload this function.		
		@param vector:  A copy of the L{indivector} that has been received.  
		@type vector: L{indivector}				
		@return: B{None}
		@rtype: NoneType
		"""
		None
		
	
def _sexagesimal(format,r):
	"""
	@param format: a format string for sexagesimal numbers as defined in the in INDI standard  
	@type format: StringType
	@param r: A floating point number  
	@type r: FloatType
	@return: A sexagesimal representation of the float given on input 
	@rtype: StringType
	"""	
	std = int( math.floor( r ));
	r -= float( math.floor( r ));
	r *= 60.0;
	min = int (math.floor( r ));
	r -= float( min);
	r *= 60.0;
	sec = r;
	output=''+ str(std)+":"+str(min)+":"+"%.2f" % sec
	return output;


class bigindiclient: 
	"""
	@ivar socket  : a TCP/IP socket to communicate with the server 
	@type socket  : socket.socket.socket 
	@ivar expat : an expat XML parser 
	@type expat : xml.parsers.expat 
	@ivar verbose :  If C{True} all XML data will be printed to the screen 
	@type verbose : BooleanType
	@ivar currentMessage : The INDI message currently being processed by the XML parser
	@type currentMessage : L{indimessage}
	@ivar currentElement : The INDI element currently being processed by the XML parser 
	@type currentElement : L{indivector}
	@ivar currentData :  A buffer to accumulate the character data to be written into the value attribute of L{currentElement} 
	@type currentData : StringType
	@ivar currentVector  : The INDI vector currently being processed by the XML parser
	@type currentVector  : L{indivector}
	@ivar defvectorlist : A list of vectors that have been received with C{def*Vector} signal at least one time 
	@type defvectorlist : list of L{indivector}
	@ivar indivectors : The list of all indivectors received so far
	@type indivectors : L{_indilist} of L{indivector} 
	@ivar port : The port address of the INDI server, this instance of L{indiclient} is connected to 
	@type port : IntType
	@ivar host  : The hostname of the INDI server, this instance of L{indiclient} is connected to 
	@type host  : StringType
	@ivar receive_event_queue : A background process (L{_receiver}) is continuesly receiving data and putting them into this queue. 
	This queue  will be read by the L{process_events} method, that the user has to call in order to process any custom handlers.
	@type receive_event_queue : Queue.Queue
	@ivar running_queue : During its destructor indiclient puts signal into this queue in order to stop the background process.
	@type running_queue : Queue.Queue
	@ivar  timeout : A timeout value (see L{timeout_handler})
	@type  timeout : FloatType
	@ivar  timeout_handler : 	This function will be called whenever an indielement has been requested but was not received 
	for a time longer than C{timeout} since the request was issued. (see also L{set_timeout_handler})
	@type  timeout_handler : function 
	@ivar blob_def_handler : Called when a new L{indiblobvector} is defined by the driver (see L{set_def_handlers})
	@type blob_def_handler : function
	@ivar number_def_handler : Called when a new L{indinumbervector} is defined by the driver (see L{set_def_handlers})
	@type number_def_handler : function
	@ivar text_def_handler : Called when a new L{inditextvector} is defined by the driver (see L{set_def_handlers})
	@type text_def_handler : function
	@ivar message_handler : (see L{set_message_handler})
	@type message_handler : function
	@ivar receivetimer : needed to run the _receive thread
	@type receivetimer : threading.Timer
	@ivar _factory : A factory used to create and classify objects during the XML parsing process. 
	@type _factory : _indiobjectfactory()
	"""

	def __init__(self,host,port):
		"""
		@param host:  The hostname or IP address of the server you want to connect to
		@type host: StringType
		@param port:  The port address of the server you want to connect to.
		@type port: IntType
		"""
		self._factory=_indiobjectfactory()
		self.indivectors=_indilist()
		self.custom_element_handler_list=[]
		self.custom_vector_handler_list=[]
		self.defvectorlist=[]
		self.currentVector=None
		self.currentElement=None
		self.currentMessage=None
		self.currentData=None
		self.verbose=False
		self.expat = xml.parsers.expat.ParserCreate()
		self.expat.StartElementHandler = self._start_element
		self.expat.EndElementHandler = self._end_element
		self.expat.CharacterDataHandler = self._char_data
		self.expat.Parse('<?xml version="1.5" encoding="UTF-8"?> <doc>', 0)
		self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		self.socket.settimeout(2)
		self.socket.connect((host, port))
		self.socket.settimeout(0.001)
		self.host=host;
		self.port=port;
		self.socket.send("<getProperties version='1.5'/>")
		self.receive_event_queue=Queue.Queue()
		self.running_queue=Queue.Queue()
		self.receive_vector_queue=Queue.Queue()
		self.timeout=1
		self.blob_def_handler=self._default_def_handler
		self.number_def_handler=self._default_def_handler
		self.switch_def_handler=self._default_def_handler
		self.text_def_handler=self._default_def_handler
		self.blob_def_handler=self._default_def_handler
		self.light_def_handler=self._default_def_handler
		self.message_handler=self._default_message_handler
		self.timeout_handler=self._default_timeout_handler
		self.running_queue.put(True) 
		self.receivetimer = threading.Timer(0.01, self._receiver)
		self.receivetimer.start()
		self.first=True
		
	#def set_verbose(self):
	#	FIXME
	#	Does not work in the treaded version
	#	solution: add queue from indiclient to thread
	
	
	def _element_received(self,vector,element):
		""" Called during the L{process_events} method each time an INDI element has been received
		@param vector: The vector containing the element that has been received  
		@type vector: indivector
		@param element:  The element that has been received
		@type element: indielement
		@return: B{None}
		@rtype: NoneType
		"""
		for handler in self.custom_element_handler_list:
			if ((handler.vectorname==vector.name) and (handler.elementname==element.name) and 
				(handler.devicename==vector.device)):
				handler.indi_object_change_notify(vector,element)

	def _vector_received(self,vector):
		""" Called during the L{process_events} method each time an indivector element has been received
		@param vector: The vector that has been received  
		@type vector: indivector
		@return: B{None}
		@rtype: NoneType
		"""
		for handler in self.custom_vector_handler_list:
			if ((handler.vectorname==vector.name) and (handler.devicename==vector.device)):
				handler.indi_object_change_notify(vector)
		
	def reset_connection(self):
		"""
		Resets the connection to the server
		@return: B{None}
		@rtype: NoneType
		"""
		print "resetting connection port"
		for i in range(10):
			time.sleep(0.1)
			self.receivetimer.cancel()
		time.sleep(0.3)
		self.socket.close()
		time.sleep(3)
		self.expat = xml.parsers.expat.ParserCreate()
		self.expat.StartElementHandler = self._start_element
		self.expat.EndElementHandler = self._end_element
		self.expat.CharacterDataHandler = self._char_data
		self.expat.Parse('<?xml version="1.5" encoding="UTF-8"?> <doc>', 0)
		failed=True
		while failed:
			failed=False
			try:
				self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
				self.socket.settimeout(0.1);
				self.socket.connect((self.host, self.port))
				self.socket.send("<getProperties version='1.5'/>")
				self.socket.settimeout(0.01);
			except:
				time.sleep(1)
				print "Reconnecting"
				failed=True
		print "connection reset successfully"
		self.receivetimer = threading.Timer(0.01, self._receiver)
		self.receivetimer.start()
	

	
	def quit(self):
		"""
		must be called in order to close the indiclient instance
		@return: B{None}
		@rtype: NoneType
		"""
		None
		self.receivetimer.cancel()
		self.socket.close()
		self.running_queue.put(False) 
		
	def tell(self):
		"""
		prints all indivectors and their elements to the screen
		@return: B{None}
		@rtype: NoneType"""
		for indivector in self.indivectors.list: 
			indivector.tell()
		
	def _get_and_update_vector(self,attrs,tag):
		""" 
		Looks for an existing indivector matching the C{device} and C{name} attibutes given in L{attrs}, and updates it
		according to L{attrs}
		@param attrs: The attributes of the XML version of the INDI object.
		@type attrs: DictType
		@param tag : the XML tag denoting the vector 
		@type tag : StringType
		@return: B{None}
		@rtype: NoneType
		"""
		devicename=_normalize_whitespace(attrs.get('device', ""))
		vectorname=_normalize_whitespace(attrs.get('name', ""))
		for i, vector in enumerate(self.indivectors.list):
			if (devicename==vector.device) and (vectorname==vector.name) :
				vector.update(attrs,tag)
				return vector
		return None

	def _get_and_update_element(self,attrs,tag):
		""" 
		Looks in C{self.currentVector} for an existing indivector matching the  C{name} attibute given in L{attrs}, and updates it
		according to L{attrs}.
		@param attrs: The attributes of the XML version of the INDI object.
		@type attrs: DictType
		@param tag : the XML tag denoting the vector 
		@type tag : StringType
		@return: B{None}
		@rtype: NoneType
		"""
		element=self.get_element(self.currentVector.device,self.currentVector.name,
			_normalize_whitespace(attrs.get('name', "")))
		element.update(attrs,tag)
		return element
	
	
	def _receiver(self):
		"""
		A "thread" that receives data from the server
		@return: B{None}
		@rtype: NoneType
		"""
		self.running=True
		while self.running:
			self._receive()
			while self.running_queue.empty()==False:
				self.running=self.running_queue.get()


	def send_vector(self,vector):
		"""
		Sends an INDI vector to the INDI server.
		@param vector:  The INDI vector to be send
		@type vector: indivector
		@return: B{None}
		@rtype: NoneType
		"""
		if not vector.tag.is_vector(): 
			return
		data=vector.get_xml(inditransfertypes.inew)
		self.socket.send(data)
		vector._light._set_value("Busy")
		
	def wait_until_vector_available(self,devicename,vectorname):
		"""
		Looks if the requested vector has already been received and waits until it is received otherwise 
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@return: B{None}
		@rtype: NoneType
		"""
		self.get_vector(devicename,vectorname)
	
	def process_receive_vector_queue(self):
		"""
		Process the entries form the receive_vector_queue created
		by the parsing thread and update the self.indivector.list
		"""
		while self.receive_vector_queue.empty()==False:
			newVector=self.receive_vector_queue.get()			
			devicename=newVector.getDevice()
			vectorname=newVector.getName()
			got=False
			#print vectorname
			for vector in self.indivectors.list:
				if (devicename==vector.device) and (vectorname==vector.name) :
					vector.updateByVector(newVector)
					got=True
			if not got:
				self.indivectors.list.append(newVector)

	
	def _get_vector(self,devicename,vectorname):
		for vector in self.indivectors.list:
			if (devicename==vector.device) and (vectorname==vector.name):
					return vector
		return None

	def get_vector(self,devicename,vectorname):
		"""
		Returns an L{indivector} matching the given L{devicename} and L{vectorname}
		This method will wait until it has been received. In case the vector doesn't exists this
		routine will never return.
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@return: The L{indivector} found
		@rtype: L{indivector}
		"""
		t=time.time()
		while True:
			v=self._get_vector(devicename,vectorname)
			if v==None:
				time.sleep(0.01)
				self.process_receive_vector_queue()
				v=self._get_vector(devicename,vectorname)
				if v!=None:
					return v
			else:
				return v
			if ((time.time()-t)>self.timeout):
				self.timeout_handler(devicename,vectorname,self)
			
	def get_element(self,devicename,vectorname,elementname):
		"""
		Returns an L{indielement} matching the given L{devicename} and L{vectorname}
		This method will wait until it has been received. In case the vector doesn't exists this
		routine will never return.
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementname:  The name of the element
		@type elementname: StringType
		@return: The element found
		@rtype: L{indielement}
		"""
		vector=self.get_vector(devicename,vectorname)
		#print "o",elementname
		#vector.tell()
		for i, element in enumerate(vector.elements):
			if elementname==element.name:
				return element

	def add_mini_element_handler(self,devicename,vectorname,elementname,handlermethod):
		"""
		Adds handler that will be called each time the element is received. 
		Here the handler is a function that takes only one argument, namely the element that was
		received. This function is deprecated. It has been replaced by the L{add_custom_element_handler}
		function.
		@param handlermethod : The function to be called
		@type handlermethod : function
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementname:  The name of the element
		@type elementname: StringType
		@return:  The handler object created
		@rtype: L{mini_element_handler}
		"""
		handler=mini_element_handler(devicename,vectorname,elementname,handlermethod)
		return self.add_custom_element_handler(handler)
		
		
	def add_custom_element_handler(self,handler):
		"""
		Adds a custom handler function for an L{indielement}, the handler will be called each time the L{indielement} is received.
		Furthermore this method will call the hander once. If the element has not been received yet, this function will wait
		until the element is received before calling the handler function. If the L{indielement} does not exist this method will
		B{not} return.
		@param handler:  The handler to be called.
		@type handler: L{indi_custom_element_handler}
		@return: The handler given in the parameter L{handler}
		@rtype: L{indi_custom_element_handler}
		"""
		handler.indi=self
		self.custom_element_handler_list.append(handler)
		vector=self.get_vector(handler.devicename,handler.vectorname)
		element=vector.get_element(handler.elementname)
		handler.configure(vector,element)
		handler.indi_object_change_notify(vector,element)
		return handler

	def add_custom_vector_handler(self,handler):
		"""
		Adds a custom handler function for an L{indivector}, the handler will be called each time the vector is received.
		Furthermore this method will call the hander once. If the vector has not been received yet, this function will wait
		until the vector is received before calling the handler function. If the vector does not exist this method will
		B{not} return.
		@param handler:  The handler to be called.
		@type handler: L{indi_custom_vector_handler}
		@return: The handler given in the parameter L{handler}
		@rtype: L{indi_custom_vector_handler}
		"""
		handler.indi=self
		self.custom_vector_handler_list.append(handler)
		vector=self.get_vector(handler.devicename,handler.vectorname)
		handler.configure(vector)
		handler.indi_object_change_notify(vector)
		return handler
			
	def _default_def_handler(self,vector,indi):
		"""
		Called whenever an indivector was received with an C{def*vector} tag. This
		means that the INDI driver has called an C{IDDef*} function and thus defined a INDI vector.
		It will be called only once for each element, even if more than C{def*vector}
		signals are received. May be replaced by a custom one see L{set_def_handlers}
		@param vector: The vector received.
		@type vector: L{indivector}
		@param indi : The L{indiclient} instance that received the L{indivector} from the server 
		@type indi : L{indiclient}
		@return: B{None}
		@rtype: NoneType
		"""
		None
		
	def _default_timeout_handler(self,devicename,vectorname,indi):
		""" 
		Called whenever an indielement has been requested but was not received for a time longer than 
		C{timeout} since the request was issued. May be replaced by a custom handler see L{set_timeout_handler}
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname: The name of the Indivector
		@type vectorname: StringType
		@param indi : This parameter will be equal to self. It still make sense as external timeout handlers might need it.
		@type indi : indiclient
		@return: B{None}
		@rtype: NoneType
		"""
		print "Timeout",devicename,vectorname
		#raise Exception
		#self._receive()

	def set_timeout_handler(self,handler):
		"""
		Sets a new timeout handler. \n
		@param handler : the new timeout handler (see L{_default_timeout_handler} for an example)
			It will be called whenever an indielement has been requested
			but was not received for a time longer than 
			C{timeout} since the request was issued.
		@type handler : function
		@return: B{None}
		@rtype: NoneType
		"""
		self.timeout_handler=handler
	
	def set_def_handlers(self,blob_def_handler,number_def_handler,
		switch_def_handler,text_def_handler,light_def_handler):
		"""
		Sets new C{def} handlers.
		These will be called whenever an indivector was received with an C{def*Vector} tag. This
		means that the INDI driver has called an C{IDDef*} function and thus defined a new INDI vector.
		These handlers will only be called once for each L{indivector}, even if more than one C{def*Vector}
		signals are received, for the same L{indivector}.\n
		@param blob_def_handler : the new C{defBLOBVector} handler
		@type blob_def_handler : function
		@param number_def_handler : the new C{defNumberVector} handler (see L{_default_def_handler} for example)
		@type number_def_handler  : function
		@param switch_def_handler : the new C{defSwitchVector} handler (see L{_default_def_handler} for example)
		@type switch_def_handler : function
		@param text_def_handler : the new C{defTextVector} handler (see L{_default_def_handler} for example)
		@type text_def_handler : function
		@param light_def_handler : the new C{defLightVector} handler (see L{_default_def_handler} for example)
		@type light_def_handler : function
		@return: B{None}
		@rtype: NoneType
		"""
		self.blob_def_handler=blob_def_handler
		self.number_def_handler=number_def_handler
		self.switch_def_handler=switch_def_handler
		self.text_def_handler=text_def_handler
		self.blob_def_handler=blob_def_handler
		self.light_def_handler=light_def_handler
		
	def set_message_handler(self,handler):
		""" 
		Sets a new message handler.
		This handler will be called whenever an INDI message has been received from the server. \n
		@param handler : the new message handler (see L{_default_message_handler} for an example)
		@type handler : function
		@return: B{None}
		@rtype: NoneType
		"""		
		self.message_handler=handler
	
	def _default_message_handler(self,message,indi):
		"""
		Called whenever an INDI message has been received from the server. 
		C{timeout} since the request was issued. May be replaced by a custom one see L{set_message_handler}
		@param message: The indimessage received
		@type message: L{indimessage}
		@param indi : This parameter will be equal to self. It still make sense as external timeout handlers might need it.
		@type indi : L{indiclient}
		@return: B{None}
		@rtype: NoneType
		"""
		print "got message by host :"+indi.host+" : "
		message.tell()
	
	def process_events(self):
		"""
		Has to be called frequently by any program using this client. All custom handler methods will called by this (and only by this)
		method. furthermore the C{def*handler} and C{massage_handler} methods will be called here.
		See also L{add_custom_element_handler},  L{set_def_handlers}, L{set_message_handler} . If you just don't want to use any 
		custom handlers and you do not use gtkindiclient functions, you do not need to call this method at all.
		@return: B{None}
		@rtype: NoneType
		"""
		self.process_receive_vector_queue()
		try:		
			while self.receive_event_queue.empty()==False:
				vector=self.receive_event_queue.get()
				if not vector.tag.is_message():
					vector=self.get_vector(vector.device,vector.name)
				if vector.is_valid:
					if vector.tag.is_message():
						self.message_handler(vector,self); #vector is in fact an indimessage (historical reasons, marked for change)
					if vector.tag.is_vector():
						self._vector_received(vector)
						for element in vector.elements:
							self._element_received(vector,element)
					if vector.tag.get_transfertype()==inditransfertypes.idef:
						for vec in self.defvectorlist:
							if (vec.name==vector.name) and (vec.device==vector.device):
								self.output_block=False
								return
						if vector.tag.get_type()=="BLOBVector":
							self.blob_def_handler(vector,self)
						if vector.tag.get_type()=="TextVector":
							self.text_def_handler(vector,self)
						if vector.tag.get_type()=="NumberVector":
							self.number_def_handler(vector,self)
						if vector.tag.get_type()=="SwitchVector":
							self.switch_def_handler(vector,self)
						if vector.tag.get_type()=="LightVector":
							self.light_def_handler(vector,self)
						self.defvectorlist.append(vector)
				else:
					print "Received bogus INDIVector"
					try:
						vector.tell()
						raise Exception 
						vector.tell()
					except:
						print "Error printing bogus INDIVector"
						raise Exception 
		except:
			a,b,c =sys.exc_info()
			sys.excepthook(  a,b,c	)
			self.quit()
			raise Exception, "indiclient: Error during process events"

			
	def _receive(self):
		"""receive data from the server
		@return: B{None}
		@rtype: NoneType
		"""
		try:
			self.data = self.socket.recv(1000000)
		except:
			self.data=""
		if self.data!="":
			if self.verbose:
				print self.data
			self.expat.Parse( self.data,0)
		
	def _char_data(self,data):
		"""Char data handler for expat parser. For details (see 
		U{http://www.python.org/doc/current/lib/expat-example.html})
		@param data: The data contained in the INDI element
		@type data: StringType
		@return: B{None}
		@rtype: NoneType
		"""
		try:
			if self.currentElement==None:
				return
			if self.currentVector==None:
				return
			data=string.replace(str(data),'\\n','')
			data = _normalize_whitespace(data)
			if data=="'":
				return
			if data=='':
				return
			self.currentData=self.currentData+data
		except:
			a,b,c =sys.exc_info()
			sys.excepthook(  a,b,c	)
			raise Exception, "indiclient: Error during char data handler"
		
	def _end_element(self,name):
		"""End of XML element handler for expat parser. For details (see 
		U{http://www.python.org/doc/current/lib/expat-example.html})
		@param name : The name of the XML object
		@type name : StringType
		@return: B{None}
		@rtype: NoneType
		"""
		try:
			if self.currentVector==None:
				return
			self.currentVector.host=self.host
			self.currentVector.port=self.port
			if self.currentElement!=None:
				if self.currentElement.tag.get_initial_tag()==name:
					self.currentElement._set_value(self.currentData)
					#if self.currentElement.tag.get_transfertype()==inditransfertypes.idef:
					self.currentVector.elements.append(self.currentElement)
					self.currentElement=None
			if self.currentVector.tag.get_initial_tag()==name:
				#if self.currentVector.is_valid():
				#if self.currentVector.tag.get_transfertype()==inditransfertypes.idef:
				#	self.indivectors.append(self.currentVector)
				self.receive_event_queue.put(copy.deepcopy(self.currentVector));
				self.receive_vector_queue.put(copy.deepcopy(self.currentVector));
				self.currentVector=None
		except:
			a,b,c =sys.exc_info()
			sys.excepthook(  a,b,c	)
			raise Exception, "indiclient: Error during end element handler"
		
	def _start_element(self,name, attrs):
		"""
		Start XML element handler for expat parser. For details (see 
		U{http://www.python.org/doc/current/lib/expat-example.html})
		@param name : The name of the XML object
		@type name : StringType
		@param attrs : The attributes of the XML object
		@type attrs : DictType
		@return: B{None}
		@rtype: NoneType
		"""
		obj=self._factory.create(name,attrs)
		if obj==None:
			return
		if attrs.has_key('message'):
			None
			self.receive_event_queue.put(indimessage(attrs))
		if obj.tag.is_vector():
			if obj.tag.get_transfertype()==inditransfertypes.idef:
				self.currentVector=obj
			if obj.tag.get_transfertype()==inditransfertypes.iset:
				#print name,attrs
				self.currentVector=obj
				#self._get_and_update_vector(attrs,obj.tag)
		if self.currentVector!=None:
			if obj.tag.is_element():
				if self.currentVector.tag.get_transfertype()==inditransfertypes.idef:
					self.currentElement=obj
				if self.currentVector.tag.get_transfertype()==inditransfertypes.iset:
					self.currentElement=obj
					#self._get_and_update_element(attrs,obj.tag)
				if obj.tag.is_element():
					self.currentData=""
		#except:
		#	a,b,c =sys.exc_info()
		#	sys.excepthook(  a,b,c )
		#	raise Exception, "indiclient: Error during start element handler"
	
	def enable_blob(self):
		"""
		Sends a signal to the server that tells it, that this client wants to receive L{indiblob} objects. If this method is not called, the server will not 
		send any L{indiblob}. The DCD clients calls it each time an L{indiblob} is defined.
		@return: B{None}
		@rtype: NoneType
		"""
		data="<enableBLOB>Also</enableBLOB>\n"
		self.socket.send(data)
		
class indiclient(bigindiclient):
	"""providing a simplified interface to L{bigindiclient}"""
	def __init__(self,host,port):
		bigindiclient.__init__(self,host,port)
	
	def set_and_send_text(self,devicename,vectorname,elementname,text):
		"""
		Sets the value of an element by a text, and sends it to the server
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementname:  The name of the element
		@type elementname: StringType
		@param text:  The value to be set.
		@type text: StringType
		@return: The vector containing the element that was just sent.
		@rtype: L{indivector}		
		"""
		vector=self.get_vector(devicename,vectorname)
		vector.get_element(elementname).set_text(text)
		self.send_vector(vector)
		return vector 

	def set_and_send_bool(self,devicename,vectorname,elementname,state):
		"""
		Sets the value of of an indi element by a boolean, and sends it to the server
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementname:  The name of the element
		@type elementname: StringType
		@param state:  The state to be set.
		@type state: BooleanType
		@return: The vector containing the element that was just sent.
		@rtype: L{indivector}
		"""
		vector=self.get_vector(devicename,vectorname)
		vector.get_element(elementname).set_active(state)
		self.send_vector(vector)
		return vector 

	def set_and_send_float(self,devicename,vectorname,elementname,number):
		"""
		Sets the value of an indi element by a floating point number, and sends it to the server
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementname:  The name of the element
		@type elementname: StringType
		@param number: The number to be set.
		@type number: FloatType
		@return: The vector containing the element that was just sent.
		@rtype: L{indivector}
		"""
		vector=self.get_vector(devicename,vectorname)
		vector=self.get_vector(devicename,vectorname)
		vector.get_element(elementname).set_float(number)
		self.send_vector(vector)
		return vector 
	
	def set_and_send_switchvector_by_elementlabel(self,devicename,vectorname,elementlabel):
		"""
		Sets all L{indiswitch} elements in this vector to C{Off}. And sets the one matching the given L{elementlabel}
		to C{On}  
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementlabel: The INDI Label of the Switch to be set to C{On}
		@type elementlabel: StringType
		@return: The vector that that was just sent.
		@rtype: L{indivector}
		"""
		#print elementlabel
		vector=self.get_vector(devicename,vectorname)
		vector.set_by_elementlabel(elementlabel)
		self.send_vector(vector)
		return vector 
	
	def get_float(self,devicename,vectorname,elementname):
		"""
		Returns a floating point number representing the value of the element requested.
		The element must be an L{indinumber}.
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementname:  The name of the element
		@type elementname: StringType
		@return: the value of the element
		@rtype: FloatType
		"""
		vector=self.get_vector(devicename,vectorname)
		return vector.get_element(elementname).get_float()
	
	def get_text(self,devicename,vectorname,elementname):
		"""
		Returns a text representing the value of the element requested.
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementname:  The name of the element
		@type elementname: StringType
		@return: the value of the element
		@rtype: StringType
		"""
		vector=self.get_vector(devicename,vectorname)
		return vector.get_element(elementname).get_text()
	
	def get_bool(self,devicename,vectorname,elementname):
		"""
		Returns Boolean representing the value of the element requested.
		The element must be an L{indiswitch}
		@param devicename:  The name of the device
		@type devicename: StringType
		@param vectorname:  The name of the vector
		@type vectorname: StringType
		@param elementname:  The name of the element
		@type elementname: StringType
		@return: the value of the element
		@rtype: BooleanType
		"""
		vector=self.get_vector(devicename,vectorname)
		return vector.get_element(elementname).get_active()
