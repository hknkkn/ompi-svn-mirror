#
# Copyright (c) 2007-2012 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

FILE(STRINGS ${OpenMPI_SOURCE_DIR}/VERSION VALUE REGEX "^major=")
STRING(REPLACE "major=" "" MAJOR_VERSION ${VALUE})

FILE(STRINGS ${OpenMPI_SOURCE_DIR}/VERSION VALUE REGEX "^minor=")
STRING(REPLACE "minor=" "" MINOR_VERSION ${VALUE})

FILE(STRINGS ${OpenMPI_SOURCE_DIR}/VERSION VALUE REGEX "^release=")
STRING(REPLACE "release=" "" RELEASE_VERSION ${VALUE})

FILE(STRINGS ${OpenMPI_SOURCE_DIR}/VERSION VALUE REGEX "^greek=")
STRING(REPLACE "greek=" "" GREEK_VERSION ${VALUE})

FILE(STRINGS ${OpenMPI_SOURCE_DIR}/VERSION VALUE REGEX "^want_repo_rev=")
STRING(REPLACE "want_repo_rev=" "" WANT_REPO_REV ${VALUE})

FILE(STRINGS ${OpenMPI_SOURCE_DIR}/VERSION VALUE REGEX "^repo_rev=")
STRING(REPLACE "repo_rev=" "" REPO_REV ${VALUE})

FILE(STRINGS ${OpenMPI_SOURCE_DIR}/VERSION VALUE REGEX "^date=")
STRING(REPLACE "date=" "" RELEASE_DATE ${VALUE})
STRING(REPLACE "\"" "" RELEASE_DATE ${RELEASE_DATE})

IF(${RELEASE_VERSION} EQUAL 0)
  SET(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}${GREEK_VERSION})
ELSE(${RELEASE_VERSION} EQUAL 0)
  SET(VERSION_STRING ${MAJOR_VERSION}.${MINOR_VERSION}.${RELEASE_VERSION}${GREEK_VERSION})
ENDIF(${RELEASE_VERSION} EQUAL 0)

# Get the svn version
IF("${WANT_REPO_REV}" STREQUAL "1")

  IF("${REPO_REV}" STREQUAL "-1")
    # for VS.NET2003 users working with web projects, we should also check "_svn" 
    IF(EXISTS ${OpenMPI_SOURCE_DIR}/.svn OR EXISTS ${OpenMPI_SOURCE_DIR}/_svn)
      EXECUTE_PROCESS (COMMAND svnversion -n
                       WORKING_DIRECTORY  ${OpenMPI_SOURCE_DIR}
                       OUTPUT_VARIABLE    OUTPUT
                       RESULT_VARIABLE    RESULT
                       ERROR_VARIABLE     ERROR)

      IF(NOT RESULT STREQUAL "0")
        # subversion might be not installed, let's try TortoiseSVN.
        EXECUTE_PROCESS (COMMAND SubWCRev 
                                 . contrib/platform/win32/ConfigFiles/revision.in
                                 ${OpenMPI_BINARY_DIR}/revision
                         WORKING_DIRECTORY  ${OpenMPI_SOURCE_DIR}
                         OUTPUT_VARIABLE    OUTPUT
                         RESULT_VARIABLE    RESULT
                         ERROR_VARIABLE     ERROR)
        IF(RESULT STREQUAL "0")
          FILE(STRINGS ${OpenMPI_BINARY_DIR}/revision OUTPUT REGEX "[0-9]*")
        ENDIF(RESULT STREQUAL "0")
      ENDIF(NOT RESULT STREQUAL "0")
    ELSEIF(EXISTS ${OpenMPI_SOURCE_DIR}/.hg)
      EXECUTE_PROCESS (COMMAND hg -v -R . tip
                       WORKING_DIRECTORY  ${OpenMPI_SOURCE_DIR}
                       OUTPUT_VARIABLE    OUTPUT
                       RESULT_VARIABLE    RESULT
                       ERROR_VARIABLE     ERROR)
      STRING(REGEX REPLACE "changeset:[^:]*:" "" OUTPUT ${OUTPUT})
      STRING(REGEX REPLACE "tag.*" "" OUTPUT ${OUTPUT})
      STRING(REPLACE "\n" "" OUTPUT ${OUTPUT})
    ENDIF(EXISTS ${OpenMPI_SOURCE_DIR}/.svn OR EXISTS ${OpenMPI_SOURCE_DIR}/_svn)

    IF(NOT RESULT STREQUAL "0")
      MESSAGE(STATUS "SVN ERROR:${RESULT} ${ERROR}")
    ELSE(NOT RESULT STREQUAL "0")
      IF(${RESULT} EQUAL 0 AND NOT "${OUTPUT}" STREQUAL "exported" )
        SET(SVN_VERSION "r${OUTPUT}")
      ENDIF(${RESULT} EQUAL 0 AND NOT "${OUTPUT}" STREQUAL "exported")
    ENDIF(NOT RESULT STREQUAL "0")

  ENDIF("${REPO_REV}" STREQUAL "-1")

  SET(VERSION_STRING ${VERSION_STRING}${SVN_VERSION})

ELSE("${WANT_REPO_REV}" STREQUAL "1")
    SET(SVN_VERSION ${REPO_REV})
ENDIF("${WANT_REPO_REV}" STREQUAL "1")

# Set OPAL versions
OMPI_DEF(OPAL_WANT_REPO_REV ${WANT_REPO_REV} "SVN verstion of OPAL" 1 1)
OMPI_DEF(OPAL_REPO_REV "${SVN_VERSION}" "SVN verstion of OPAL" 1 1)
OMPI_DEF(OPAL_GREEK_VERSION "${GREEK_VERSION}" "Greek - alpha, beta, etc - release number of Open Portable Access Layer." 1 1)
OMPI_DEF(OPAL_MAJOR_VERSION ${MAJOR_VERSION} "Major release number of Open Portable Access Layer." 0 1)
OMPI_DEF(OPAL_MINOR_VERSION ${MINOR_VERSION} "Minor release number of Open Portable Access Layer." 0 1)
OMPI_DEF(OPAL_RELEASE_VERSION ${RELEASE_VERSION} "Release number of Open Portable Access Layer." 0 1)
OMPI_DEF(OPAL_VERSION ${VERSION_STRING} "Complete release number of Open Portable Access Layer." 1 1)
OMPI_DEF(OPAL_IDENT_STRING ${VERSION_STRING} "ident string for Open MPI." 1 1)

# Set OMPI versions
OMPI_DEF(OMPI_WANT_REPO_REV ${WANT_REPO_REV} "SVN verstion of OMPI" 1 1)
OMPI_DEF(OMPI_REPO_REV "${SVN_VERSION}" "SVN verstion of OMPI" 1 1)
OMPI_DEF(OMPI_GREEK_VERSION "${GREEK_VERSION}" "Greek - alpha, beta, etc - release number of Open Portable Access Layer." 1 1)
OMPI_DEF(OMPI_MAJOR_VERSION ${MAJOR_VERSION} "Major release number of Open MPI." 0 1)
OMPI_DEF(OMPI_MINOR_VERSION ${MINOR_VERSION} "Minor release number of Open MPI." 0 1)
OMPI_DEF(OMPI_RELEASE_VERSION ${RELEASE_VERSION} "Release number of Open MPI." 0 1)
OMPI_DEF(OMPI_VERSION ${VERSION_STRING} "Complete release number of Open MPI." 1 1)

# Set ORTE versions
OMPI_DEF(ORTE_WANT_REPO_REV ${WANT_REPO_REV} "SVN verstion of ORTE" 1 1)
OMPI_DEF(ORTE_REPO_REV "${SVN_VERSION}" "SVN verstion of ORTE" 1 1)
OMPI_DEF(ORTE_GREEK_VERSION "${GREEK_VERSION}" "Greek - alpha, beta, etc - release number of Open Run-Time Environment." 1 1)
OMPI_DEF(ORTE_MAJOR_VERSION ${MAJOR_VERSION} "Major release number of Open Run-Time Environment." 0 1)
OMPI_DEF(ORTE_MINOR_VERSION ${MINOR_VERSION} "Minor release number of Open Run-Time Environment." 0 1)
OMPI_DEF(ORTE_RELEASE_VERSION ${RELEASE_VERSION} "Release number of Open Run-Time Environment." 0 1)
OMPI_DEF(ORTE_VERSION ${VERSION_STRING} "Complete release number of Open Run-Time Environment." 1 1)
