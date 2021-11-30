IF (WIN32)
    set(Boost_USE_STATIC_LIBS       ON)  # only find static libs
    set(Boost_USE_MULTITHREADED     ON)
    set(Boost_USE_STATIC_RUNTIME    OFF)
    set(Boost_ARCHITECTURE          "-x64")
    set(Boost_DEBUG                 ON)
    set(OPENSSL_USE_STATIC_LIBS     TRUE)
ENDIF()

project(dataminer)
cmake_minimum_required(VERSION 3.10)
FIND_PACKAGE( Boost REQUIRED COMPONENTS thread filesystem log)
FIND_PACKAGE( OpenSSL REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
add_compile_options(-std=c++11)

ADD_EXECUTABLE(dataminer dataminer.cpp agenthandler.cpp httpreader.cpp settings.cpp worker.cpp)

IF (WIN32)
    SET(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++ -static")

    TARGET_LINK_LIBRARIES( dataminer LINK_PUBLIC ${Boost_LIBRARIES} Boost::thread Boost::filesystem OpenSSL::SSL ${OPENSSL_CRYPTO_LIBRARY} ws2_32 mswsock advapi32)

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "bin" COMPONENT applications)

    #
    # CPACK macros below here
    #
    set (CPACK_GENERATOR NSIS)
    SET (CPACK_NSIS_MODIFY_PATH ON)

ELSE()

    TARGET_LINK_LIBRARIES( dataminer LINK_PUBLIC ${Boost_LIBRARIES} OpenSSL::SSL)

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
