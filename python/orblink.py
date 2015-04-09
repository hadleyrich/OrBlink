#!/usr/bin/env python
"""
OrBlink by Hadley Rich <hads@nice.net.nz>

Copyright (c) <2013>, <hads@nice.net.nz>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
"""

import os, sys
try:
  import usb
  import usb.backend.libusb1 as libusb1
except ImportError:
  sys.stderr.write("The pyusb module (version 1.0.0b2 or later) is required.\n")
  sys.exit(2)
else:
  if usb.__version__ < '1.0.0b2':
    sys.stderr.write("The pyusb module (version 1.0.0b2 or later) is required.\n")
    sys.exit(2)

CUSTOM_RQ_SET_RED = 3
CUSTOM_RQ_SET_GREEN = 4
CUSTOM_RQ_SET_BLUE = 5
CUSTOM_RQ_STORE = 6

class OrBlink(object):
  reqtype = usb.util.build_request_type(
    usb.util.CTRL_OUT, usb.util.CTRL_TYPE_VENDOR, usb.util.CTRL_RECIPIENT_DEVICE)

  def __init__(self, vendor_id, product_id):
    if os.path.exists('/etc/openelec-release'):
      # Hack for OpenELEC so it can find the library
      import ctypes
      def loader(find_library):
        return ctypes.CDLL('libusb-1.0.so')
      libusb1._load_library=loader

    self.device = usb.core.find(idVendor=vendor_id, idProduct=product_id)

  @property
  def valid(self):
    if self.device \
    and self.manufacturer_name == 'nicegear.co' \
    and self.product_name == 'OrBlink':
      return True
    return False

  @property
  def manufacturer_name(self):
    return usb.util.get_string(self.device, 1)

  @property
  def product_name(self):
    return usb.util.get_string(self.device, 2)

  def store(self):
    self.device.ctrl_transfer(self.reqtype, CUSTOM_RQ_STORE, 1)

  def set_color(self, r, g, b):
    self.device.ctrl_transfer(self.reqtype, CUSTOM_RQ_SET_RED, r)
    self.device.ctrl_transfer(self.reqtype, CUSTOM_RQ_SET_GREEN, g)
    self.device.ctrl_transfer(self.reqtype, CUSTOM_RQ_SET_BLUE, b)

def hex_to_rgb(value):
    value = value.lstrip('#')
    return tuple(ord(c) for c in value.decode('hex'))

