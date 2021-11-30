project(dataminer)
cmake_minimum_required(VERSION 3.10)

find_program(conan conan)
if(NOT EXISTS ${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
    message(STATUS "Generating conan config file in ${CMAKE_BINARY_DIR}.")
    execute_process(
        COMMAND ${conan} install ${CMAKE_SOURCE_DIR} -if ${CMAKE_BINARY_DIR}  --build=missing -s build_type=${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
endif()

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

IF(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden -fvisibility=hidden")
ENDIF()

ADD_EXECUTABLE(dataminer dataminer.cpp agenthandler.cpp httpreader.cpp settings.cpp worker.cpp)

target_include_directories(
  dataminer
  PUBLIC
    "${CONAN_INCLUDE_DIRS}"
    )

target_link_libraries(
  dataminer
  PUBLIC
  CONAN_PKG::boost CONAN_PKG::openssl
  $<$<PLATFORM_ID:Windows>:shlwapi>
)

IF (WIN32)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin" COMPONENT applications)

    #
    # CPACK macros below here
    #
    set (CPACK_GENERATOR NSIS)
    SET (CPACK_NSIS_MODIFY_PATH ON)

ELSE()

    install(TARGETS ${PROJECT_NAME} DESTINATION ".")

    set (CPACK_GENERATOR ZIP)
ENDIF()

set (CPACK_PACKAGE_NAME "dataminer")
set (CPACK_PACKAGE_VENDOR "MTConnect")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "dataminer - a data collector for MTConnect agents")
set (CPACK_PACKAGE_VERSION "1.1.2")
set (CPACK_PACKAGE_VERSION_MAJOR "1")
set (CPACK_PACKAGE_VERSION_MINOR "1")
set (CPACK_PACKAGE_VERSION_PATCH "2")
set (CPACK_PACKAGE_INSTALL_DIRECTORY "MTConnect dataminer")


# Must be after the last CPACK macros
include(CPack)
