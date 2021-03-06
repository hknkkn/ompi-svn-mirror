# Copyright (c) 2010-2012 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

# Only one libevent component should be used, selection is done by editing .windows in_use property.

FILE(STRINGS ${CURRENT_PATH}/.windows IN_USE REGEX "^in_use=")
STRING(REPLACE "in_use=" "" IN_USE ${IN_USE})

STRING(REGEX MATCH "libevent[0-9]+" libevent_dir "${CURRENT_PATH}")

IF(${IN_USE} STREQUAL "0")

  SET(RESULT FALSE)

ELSE(${IN_USE} STREQUAL "0")

  SET(LIBEVENT_FOUND TRUE CACHE INTERNAL "allow only one event mca.")

  MESSAGE(STATUS "configure libevent.")

  # set up event include directories.
  INCLUDE_DIRECTORIES ("${CURRENT_PATH}/libevent/compat"
    "${CURRENT_PATH}/libevent/WIN32-Code/"
    "${CURRENT_PATH}/libevent/include/"
    "${CURRENT_PATH}/libevent"
    "${PROJECT_BINARY_DIR}/mca/event/${libevent_dir}")

  SET(LIBEVENT_INCLUDE_DIRS ${CURRENT_PATH}/libevent/compat;${CURRENT_PATH}/libevent/WIN32-Code/;${CURRENT_PATH}/libevent/include/;${CURRENT_PATH}/libevent;${PROJECT_BINARY_DIR}/mca/event/${libevent_dir}/
  CACHE INTERNAL "the libevent dirs that have to be included on the top level.")

  IF(WIN32)

    # generating config.h
    # windows doesn't need this file, just make an empty one
    FILE(WRITE ${PROJECT_BINARY_DIR}/mca/event/${libevent_dir}/libevent/config.h
      " /* config.h.  Generated automatically by CMake. */ ")

    SET(RESULT_COMPONENT_FILES
      ${RESULT_COMPONENT_FILES}
      ${CURRENT_PATH}/${libevent_dir}_component.c
      ${CURRENT_PATH}/${libevent_dir}_module.c
      #system sources
      ${CURRENT_PATH}/libevent/win32select.c
      ${CURRENT_PATH}/libevent/evthread_win32.c
      ${CURRENT_PATH}/libevent/buffer_iocp.c
      ${CURRENT_PATH}/libevent/event_iocp.c
      ${CURRENT_PATH}/libevent/bufferevent_async.c
      #core sources
      ${CURRENT_PATH}/libevent/event.c
      ${CURRENT_PATH}/libevent/evthread.c
      ${CURRENT_PATH}/libevent/buffer.c
      ${CURRENT_PATH}/libevent/bufferevent.c
      ${CURRENT_PATH}/libevent/bufferevent_sock.c
      ${CURRENT_PATH}/libevent/bufferevent_pair.c
      ${CURRENT_PATH}/libevent/listener.c
      ${CURRENT_PATH}/libevent/bufferevent_ratelim.c
      ${CURRENT_PATH}/libevent/evmap.c
      ${CURRENT_PATH}/libevent/log.c
      ${CURRENT_PATH}/libevent/evutil.c
      ${CURRENT_PATH}/libevent/evutil_rand.c
      ${CURRENT_PATH}/libevent/strlcpy.c
      ${CURRENT_PATH}/libevent/signal.c
    )
    IF(WINDOWS_MINGW)
      SET_SOURCE_FILES_PROPERTIES(${CURRENT_PATH}/libevent/win32select.c
                                  ${CURRENT_PATH}/libevent/evthread_win32.c
                                  ${CURRENT_PATH}/libevent/buffer_iocp.c
                                  ${CURRENT_PATH}/libevent/bufferevent_async.c
                                  ${CURRENT_PATH}/libevent/event.c
                                  ${CURRENT_PATH}/libevent/evthread.c
                                  ${CURRENT_PATH}/libevent/bufferevent.c
                                  ${CURRENT_PATH}/libevent/bufferevent_sock.c
                                  ${CURRENT_PATH}/libevent/bufferevent_pair.c
                                  ${CURRENT_PATH}/libevent/bufferevent_ratelim.c
                                  ${CURRENT_PATH}/libevent/evmap.c
                                  ${CURRENT_PATH}/libevent/log.c
                                  ${CURRENT_PATH}/libevent/evutil_rand.c
                                  ${CURRENT_PATH}/libevent/strlcpy.c
                                  ${CURRENT_PATH}/libevent/signal.c
                                  PROPERTIES COMPILE_FLAGS "-D intptr_t=int -D _INTPTR_T_DEFINED")
      SET(OBJ_PATH "${PROJECT_BINARY_DIR}/${CMAKE_FILES_DIRECTORY}/libopen-pal.dir/mca/event/${libevent_dir}/libevent")
      SET(EVENT_OBJ_FILES "${OBJ_PATH}/*.obj" CACHE INTERNAL "event obj files")
    ELSEIF(WINDOWS_VS)
      SET(OBJ_PATH "${PROJECT_BINARY_DIR}/libopen-pal.dir/${CMAKE_CFG_INTDIR}")
        # for generating the static library, as opal will not export event API any more.
    SET(EVENT_OBJ_FILES
      ${OBJ_PATH}/win32select.obj
      ${OBJ_PATH}/evthread_win32.obj
      ${OBJ_PATH}/buffer_iocp.obj
      ${OBJ_PATH}/event_iocp.obj
      ${OBJ_PATH}/bufferevent_async.obj
      ${OBJ_PATH}/event.obj
      ${OBJ_PATH}/evthread.obj
      ${OBJ_PATH}/buffer.obj
      ${OBJ_PATH}/bufferevent.obj
      ${OBJ_PATH}/bufferevent_sock.obj
      ${OBJ_PATH}/bufferevent_pair.obj
      ${OBJ_PATH}/listener.obj
      ${OBJ_PATH}/bufferevent_ratelim.obj
      ${OBJ_PATH}/evmap.obj
      ${OBJ_PATH}/log.obj
      ${OBJ_PATH}/evutil.obj
      ${OBJ_PATH}/evutil_rand.obj
      ${OBJ_PATH}/strlcpy.obj
      ${OBJ_PATH}/signal.obj
      CACHE INTERNAL "event obj files")
    ENDIF(WINDOWS_MINGW)

    OMPI_DEF(OPAL_HAVE_WORKING_EVENTOPS 1 
      "Whether our event component has working event operations or not if not, then assumedly it only has working timers and signals)." 0 1)

    OMPI_DEF(MCA_event_IMPLEMENTATION_HEADER "${CURRENT_PATH}/${libevent_dir}.h"
      "Header to include for event implementation" 1 1)

    SET(LIBEVENT_CONFIG_DONE TRUE CACHE INTERNAL "Libevent config done.")

  ELSE(WIN32)
    SET(RESULT_COMPONENT_FILES
      ${RESULT_COMPONENT_FILES}
    )
  ENDIF(WIN32)

  SET(RESULT TRUE)

ENDIF(${IN_USE} STREQUAL "0")
