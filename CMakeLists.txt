IF (WIN32)
    set(Boost_USE_STATIC_LIBS       ON)  # only find static libs
    set(Boost_USE_MULTITHREADED     ON)
    set(Boost_USE_STATIC_RUNTIME    OFF)
    set(Boost_ARCHITECTURE          "-x64")
    set(Boost_DEBUG                 ON)
    set(OPENSSL_USE_STATIC_LIBS     TRUE)
ENDIF()

cmake_minimum_required(VERSION 3.12)
FIND_PACKAGE( Boost REQUIRED COMPONENTS thread filesystem)
FIND_PACKAGE( OpenSSL REQUIRED)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )
add_compile_options(-std=c++11)

ADD_EXECUTABLE(dataminer dataminer.cpp xml2json.cpp httpreader.cpp )

IF (WIN32)
    TARGET_LINK_LIBRARIES( dataminer LINK_PUBLIC ${Boost_LIBRARIES} Boost::thread Boost::filesystem OpenSSL::SSL ${OPENSSL_CRYPTO_LIBRARY} ws2_32 mswsock advapi32)
ELSE()
    TARGET_LINK_LIBRARIES( dataminer LINK_PUBLIC ${Boost_LIBRARIES} OpenSSL::SSL ${OPENSSL_CRYPTO_LIBRARY})
ENDIF()
