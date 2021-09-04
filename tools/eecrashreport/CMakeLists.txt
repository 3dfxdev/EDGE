## Copyright (c) 2010 Jamie Jones <jamie_jones_au@yahoo.com.au>
##
## This software is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This software is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor,
## Boston, MA  02110-1301  USA
##
################################################################################
######################### CMake Configuration ##################################
CMAKE_MINIMUM_REQUIRED (VERSION 2.4)
IF (COMMAND cmake_policy)
        CMAKE_POLICY (SET CMP0003 NEW)
ENDIF (COMMAND cmake_policy)

################################################################################
######################### Set Project Source  ##################################
ADD_DEFINITIONS(-D_AFXDLL)
SET(CMAKE_MFC_FLAG 2)
FILE (GLOB EECRASHREPORT_SOURCES *.cc)
INCLUDE_DIRECTORIES (.)

################################################################################
######################### Set Build Targets   ##################################
ADD_EXECUTABLE (eecrashreport WIN32 ${EECRASHREPORT_SOURCES})
TARGET_LINK_LIBRARIES (eecrashreport)
INSTALL (TARGETS eecrashreport
        RUNTIME DESTINATION $<TARGET_FILE_DIR:EDGE>
        LIBRARY DESTINATION $<TARGET_FILE_DIR:EDGE>
        ARCHIVE DESTINATION $<TARGET_FILE_DIR:EDGE>)

add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/eecrashreport.exe
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_BINARY_DIR}/eecrashreport.exe $<TARGET_FILE_DIR:EDGE>/eecrashreport.exe
	DEPENDS eecrashreport)