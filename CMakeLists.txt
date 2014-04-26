# The MIT License (MIT)
# 
# Copyright (c) 2014 Pavel Krajcevski
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

CMAKE_MINIMUM_REQUIRED(VERSION 2.8 FATAL_ERROR)
PROJECT(libhowler)

SET(CMAKE_MODULE_PATH "${libhowler_SOURCE_DIR}" ${CMAKE_MODULE_PATH})

SET(HEADERS
  "howler.h"
)

SET(SOURCES
  "howler.c"
)

FIND_PACKAGE(libusb-1.0 REQUIRED)

INCLUDE_DIRECTORIES(${LIBUSB_1_INCLUDE_DIRS})

ADD_LIBRARY(howler ${HEADERS} ${SOURCES})
TARGET_LINK_LIBRARIES(howler ${LIBUSB_1_LIBRARIES})

ADD_EXECUTABLE(howler-example example.c)
TARGET_LINK_LIBRARIES(howler-example howler)

ADD_EXECUTABLE(howlerctl howlerctl.c)
TARGET_LINK_LIBRARIES(howlerctl howler)
