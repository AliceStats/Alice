#------------------------------------------------------------
# Standard CMake Stuff
#------------------------------------------------------------

CMAKE_MINIMUM_REQUIRED ( VERSION 2.8 )
PROJECT ( Alice )

SET ( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMake )

MESSAGE ( STATUS "Source DIR is ${PROJECT_SOURCE_DIR}" )
MESSAGE ( STATUS "Binary DIR is ${PROJECT_BINARY_DIR}" )
MESSAGE ( STATUS "Build type is ${CMAKE_BUILD_TYPE}" )
MESSAGE ( STATUS "Libs extension is '${SUFFIXLIB}'. Bins extension is '${SUFFIXBIN}'." )
MESSAGE ( STATUS "Installation prefix directory is " ${CMAKE_INSTALL_PREFIX} )
MESSAGE ( STATUS "Host system is " ${CMAKE_HOST_SYSTEM} " with processor " ${CMAKE_HOST_SYSTEM_PROCESSOR} )
MESSAGE ( STATUS "Target system is " ${CMAKE_SYSTEM} " with processor " ${CMAKE_SYSTEM_PROCESSOR} )
MESSAGE ( STATUS "Compiler identified as " ${CMAKE_CXX_COMPILER_ID} )

# Whether to build alice-addon
OPTION (BUILD_ADDON "Build addon library (optional but useful)" OFF)

# Build documentation
OPTION (BUILD_DOCUMENTATION "Generate API documentation" OFF)

# Build examples
OPTION (BUILD_EXAMPLE "Build example code" OFF)

# Build unit tests
OPTION (BUILD_TEST "Build unit tests, requires Boost.Test library" OFF)

# Enable debug output
OPTION (DEBUG "Print useful information to cout, slows parser down a lot" 0)

# Enable bzip2 decompression via boost.iostreams
OPTION (BZIP2 "Enables bzip2 decompression via boost.iostream" 0)

# Enable emscripten support
OPTION (BUILD_EMSCRIPTEN "Enables emscripten support" 0)

#------------------------------------------------------------
# Version Info
#------------------------------------------------------------

IF ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    # Getting the correct SHA-1 only works on windows
    INCLUDE ( GetGitRevisionDescription )
    GET_GIT_HEAD_REVISION ( GIT_REFSPEC GIT_SHA1 )
    MESSAGE ( STATUS "Git Revision is " ${GIT_SHA1}  )
ELSE ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )
    SET ( GIT_SHA1 "" )
ENDIF ( CMAKE_SYSTEM_NAME STREQUAL "Linux" )

#------------------------------------------------------------
# Compiler Setup
#------------------------------------------------------------

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  SET ( CMAKE_CXX_FLAGS "-O3 --std=c++0x -fPIC -Wall -Wno-sign-compare -Wno-unused-variable --stdlib=libc++" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  SET ( CMAKE_CXX_FLAGS "-O3 --std=c++0x -fPIC -fno-pie -Wall -Wno-sign-compare -Wno-unused-variable" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  MESSAGE ( STATUS "Intel Compiler not tested yet, falling back to default compiler flags" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # use default msvc flags
endif()

# Check for 64 bit, ignore when using emscripten
SET ( DOTA_64BIT "0" )
IF ( NOT BUILD_EMSCRIPTEN )
    IF ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
        SET (DOTA_64BIT "1")
    ENDIF ( CMAKE_SIZEOF_VOID_P EQUAL 8 )
ENDIF ( NOT BUILD_EMSCRIPTEN )

#------------------------------------------------------------
# Generate configuration
#------------------------------------------------------------

MESSAGE ( STATUS "Generating src/config.hpp" )
CONFIGURE_FILE (
    ${CMAKE_SOURCE_DIR}/src/alice/config.hpp.in
    ${CMAKE_SOURCE_DIR}/src/alice/config.hpp
)

#------------------------------------------------------------
# Include And Configure Libraries Used
#------------------------------------------------------------

IF ( BUILD_EMSCRIPTEN )
    MESSAGE ( STATUS "Building Alice using emscripten, please see the build-emscripten.md for further information" )

    # Some compiler options
    SET ( CMAKE_CXX_FLAGS "-O2 -std=c++0x -Wno-warn-absolute-paths -Wall -Wno-sign-compare -Wno-unused-variable -s TOTAL_MEMORY=100000000" )
    SET ( CMAKE_EXECUTABLE_SUFFIX ".html" )

    # Include Protobuf
    IF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/src/.libs/libprotobuf.so )
        SET ( PROTOBUF_LIBRARY ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/src/.libs/libprotobuf.so )
    ELSE ( )
        MESSAGE ( error "Can't find protobuf" )
    ENDIF ( )

    MESSAGE ( STATUS "Treating libprotobuf.so under deps/ as LLVM-Bytecode" )

    # Try to find protoc
    IF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/src/.libs/protoc )
        SET ( PROTOC ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/src/.libs/protoc )
        MESSAGE ( STATUS "Using protoc under deps/" )
    ELSEIF ( EXISTS /usr/bin/protoc )
        SET ( PROTOC /usr/bin/protoc )
        MESSAGE ( STATUS "Using protoc under /usr/bin/" )
    ELSE ( )
        MESSAGE ( error "Can't find protoc" )
    ENDIF ( )

    # Include Snappy
    IF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1/.libs/libsnappy.so )
        SET ( SNAPPY_LIBRARIES ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1/.libs/libsnappy.so )
    ELSE ( )
        MESSAGE ( error "Can't find snappy" )
    ENDIF ( )

    MESSAGE ( STATUS "Treating libsnappy.so under deps/ as LLVM-Bytecode" )

    # Include dirs under /deps/, just assume boost is there
    include_directories ( ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/src )
    include_directories ( ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1 )
    include_directories ( ${CMAKE_SOURCE_DIR}/deps/boost-1.55.0 )
ELSEIF ( UNIX )
    # Protocol Buffers
    find_package( Protobuf REQUIRED )
    include_directories( ${PROTOBUF_INCLUDE_DIR} )
    SET ( PROTOC "protoc" )

    # Snappy
    include ( FindSnappy )
    find_package ( Snappy REQUIRED )
    include_directories( ${SNAPPY_INCLUDE_DIR} )
    SET ( SNAPPY_SOURCES "" )

    # Boost headers / librarys
    IF ( BUILD_TEST )
        SET ( BOOST_COMPONENTS system unit_test_framework )
    ENDIF ( )

    IF ( BZIP2 )
        SET ( BOOST_COMPONENTS ${BOOST_COMPONENTS} iostreams )
        find_package ( BZip2 )
    ENDIF ( )

    find_package (Boost COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
    include_directories( ${Boost_INCLUDE_DIRS} )
ELSE ( )
    # Protobuf
    IF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/vsprojects/Release )
        SET ( CMAKE_INCLUDE_PATH ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/src/ )
        SET ( CMAKE_LIBRARY_PATH ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/vsprojects/Release )
        SET ( PROTOC ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/vsprojects/Release/protoc.exe )
    ENDIF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/protobuf-2.5.0/vsprojects/Release )

    find_package( Protobuf REQUIRED )
    include_directories( ${PROTOBUF_INCLUDE_DIR} )

    # Snappy
    IF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1/ )
		# just build snappy with the project on windows
		SET ( SNAPPY_SOURCES ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1/snappy.cc ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1/snappy-sinksource.cc )
		SET ( SNAPPY_LIBRARIES "" )
		include_directories( ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1/ )
	ELSE ( EXISTS ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1/ )
		MESSAGE ( error "Can't find snappy" )
    ENDIF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/snappy-1.1.1/ )

    # Boost headers / librarys
    IF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/boost-1.55.0 )
        SET ( BOOST_ROOT ${CMAKE_SOURCE_DIR}/deps/boost-1.55.0 )
    ENDIF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/boost-1.55.0 )

    IF ( BUILD_TEST )
        find_package (Boost COMPONENTS system unit_test_framework REQUIRED)
    ELSE ( BUILD_TEST )
        find_package ( Boost REQUIRED )
    ENDIF ( BUILD_TEST )

    include_directories( ${Boost_INCLUDE_DIRS} )
ENDIF ( )

include_directories( ${CMAKE_SOURCE_DIR}/src )

#------------------------------------------------------------
# Generate protobuffer files and target
#------------------------------------------------------------

FILE( GLOB ProtoFiles ${CMAKE_SOURCE_DIR}/proto/*.proto )
EXECUTE_PROCESS ( COMMAND ${PROTOC}
    ${ProtoFiles}
    --cpp_out=${CMAKE_SOURCE_DIR}/src/alice
    --proto_path=${CMAKE_SOURCE_DIR}/proto
)

#------------------------------------------------------------
# Build library
#------------------------------------------------------------

SET ( PROTO_SOURCES
    src/alice/ai_activity.pb.cc
    src/alice/demo.pb.cc
    src/alice/dota_commonmessages.pb.cc
    src/alice/dota_gcmessages_common.pb.cc
    src/alice/dota_modifiers.pb.cc
    src/alice/dota_usermessages.pb.cc
    src/alice/gcsdk_gcmessages.pb.cc
    src/alice/netmessages.pb.cc
    src/alice/networkbasetypes.pb.cc
    src/alice/network_connection.pb.cc
    src/alice/steammessages.pb.cc
    src/alice/usermessages.pb.cc
)

SET ( ALICE_CORE_SOURCES
    src/alice/bitstream.cpp
    src/alice/entity.cpp
    src/alice/parser.cpp
    src/alice/property.cpp
    src/alice/stringtable.cpp
    src/alice/dem_stream_bzip2.cpp
    src/alice/dem_stream_file.cpp
    src/alice/dem_stream_memory.cpp
)

SET ( ALICE_ADDON_SOURCES
    src/alice/keyvalue.cpp
)

# alice-core
ADD_LIBRARY ( alice-core OBJECT ${ALICE_CORE_SOURCES} ${PROTO_SOURCES} ${SNAPPY_SOURCES} )
ADD_LIBRARY ( alice-core-shared SHARED $<TARGET_OBJECTS:alice-core>)
ADD_LIBRARY ( alice-core-static STATIC $<TARGET_OBJECTS:alice-core>)

# default library to link against
IF ( BUILD_EMSCRIPTEN )
    SET ( ALICE_LIB "alice-core-shared" )
ELSE ( )
    SET ( ALICE_LIB "alice-core-static" )
ENDIF ( )

TARGET_LINK_LIBRARIES ( alice-core-shared
    ${PROTOBUF_LIBRARY}
    ${SNAPPY_LIBRARIES}
    ${Boost_LIBRARIES}
    ${BZIP2_LIBRARIES}
)

# alice-addon
IF ( BUILD_ADDON )
    ADD_LIBRARY ( alice-addon OBJECT ${ALICE_ADDON_SOURCES} )
    ADD_LIBRARY ( alice-addon-shared SHARED $<TARGET_OBJECTS:alice-addon>)
    ADD_LIBRARY ( alice-addon-static STATIC $<TARGET_OBJECTS:alice-addon>)
ENDIF ( BUILD_ADDON )

#------------------------------------------------------------
# Build unittests
#------------------------------------------------------------

IF ( BUILD_TEST )
    ADD_SUBDIRECTORY( test )
ENDIF ( BUILD_TEST )

#------------------------------------------------------------
# Install headers and lib
#------------------------------------------------------------

SET ( PROTO_HEADERS
    src/alice/ai_activity.pb.h
    src/alice/demo.pb.h
    src/alice/dota_commonmessages.pb.h
    src/alice/dota_gcmessages_common.pb.h
    src/alice/dota_modifiers.pb.h
    src/alice/dota_usermessages.pb.h
    src/alice/gcsdk_gcmessages.pb.h
    src/alice/netmessages.pb.h
    src/alice/networkbasetypes.pb.h
    src/alice/network_connection.pb.h
    src/alice/usermessages.pb.h
    src/alice/steammessages.pb.h
)

SET ( ALICE_CORE_HEADERS
    src/alice/alice.hpp
    src/alice/bitstream.hpp
    src/alice/config.hpp
    src/alice/dem.hpp
    src/alice/dem_stream_bzip2.hpp
    src/alice/dem_stream_file.hpp
    src/alice/dem_stream_memory.hpp
    src/alice/delegate.hpp
    src/alice/entity.hpp
    src/alice/event.hpp
    src/alice/exception.hpp
    src/alice/handler.hpp
    src/alice/handler_detail.hpp
    src/alice/handler_impl.hpp
    src/alice/multiindex.hpp
    src/alice/parser.hpp
    src/alice/property.hpp
    src/alice/sendprop.hpp
    src/alice/sendtable.hpp
    src/alice/settings.hpp
    src/alice/stringtable.hpp
)

SET ( ALICE_ADDON_HEADERS
    src/alice/defs.hpp
    src/alice/keyvalue.hpp
    src/alice/monitor.hpp
    src/alice/queue.hpp
    src/alice/timer.hpp
    src/alice/tree.hpp
)

INSTALL( FILES ${PROTO_HEADERS} ${ALICE_CORE_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/alice/ )

INSTALL( TARGETS alice-core-shared alice-core-static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

IF ( BUILD_ADDON )
    INSTALL( FILES ${ALICE_ADDON_HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/alice/ )
    INSTALL( TARGETS alice-addon-shared alice-addon-static
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
ENDIF ( BUILD_ADDON )

#------------------------------------------------------------
# Build example
#------------------------------------------------------------

IF ( BUILD_EXAMPLE )
    ADD_EXECUTABLE ( alice-example
        example/example.cpp
    )

    TARGET_LINK_LIBRARIES ( alice-example
        ${ALICE_LIB}
        ${PROTOBUF_LIBRARY}
    	${SNAPPY_LIBRARIES}
    	${Boost_LIBRARIES}
    )

    INSTALL( TARGETS alice-example RUNTIME DESTINATION bin )

    ADD_EXECUTABLE ( alice-performance
        example/performance.cpp
    )

    TARGET_LINK_LIBRARIES ( alice-performance
        ${ALICE_LIB}
        ${PROTOBUF_LIBRARY}
    	${SNAPPY_LIBRARIES}
    	${Boost_LIBRARIES}
    )

    INSTALL( TARGETS alice-performance RUNTIME DESTINATION bin )

    ADD_EXECUTABLE ( alice-visualize
        example/visualize.cpp
    )

    TARGET_LINK_LIBRARIES ( alice-visualize
        ${ALICE_LIB}
        ${PROTOBUF_LIBRARY}
    	${SNAPPY_LIBRARIES}
    	${Boost_LIBRARIES}
    )

    INSTALL( TARGETS alice-visualize RUNTIME DESTINATION bin )

    ADD_EXECUTABLE ( alice-chat
        example/chat.cpp
    )

    TARGET_LINK_LIBRARIES ( alice-chat
        ${ALICE_LIB}
        ${PROTOBUF_LIBRARY}
    	${SNAPPY_LIBRARIES}
    	${Boost_LIBRARIES}
    )

    INSTALL( TARGETS alice-chat RUNTIME DESTINATION bin )

    ADD_EXECUTABLE ( alice-verify
        example/verify.cpp
    )

    TARGET_LINK_LIBRARIES ( alice-verify
        ${ALICE_LIB}
        ${PROTOBUF_LIBRARY}
    	${SNAPPY_LIBRARIES}
    	${Boost_LIBRARIES}
    	${BZIP2_LIBRARIES}
    )

    INSTALL( TARGETS alice-verify RUNTIME DESTINATION bin )
ENDIF ( BUILD_EXAMPLE )

#------------------------------------------------------------
# Generate API docs
#------------------------------------------------------------

IF ( BUILD_DOCUMENTATION )
    MESSAGE ( STATUS "Generating Doxyfile" )
    CONFIGURE_FILE (
        ${CMAKE_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_SOURCE_DIR}/Doxyfile
    )

    EXECUTE_PROCESS ( COMMAND doxygen ${CMAKE_SOURCE_DIR}/Doxyfile )
    INSTALL ( DIRECTORY doc DESTINATION ${CMAKE_INSTALL_PREFIX}/share/alice/ )
ENDIF ( BUILD_DOCUMENTATION )
