# Copyright (c) 2007-2012 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

#PROJECT(CHELLO C CXX)

SET(CHELLO_FILES
  chello.c
)

ADD_EXECUTABLE (chello ${CHELLO_FILES})

TARGET_LINK_LIBRARIES (chello libmpi libopen-rte libopen-pal Ws2_32.lib shlwapi.lib)