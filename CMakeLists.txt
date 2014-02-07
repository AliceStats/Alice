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

# Whether to build alice-addon
OPTION (BUILD_ADDON "Build addon library (optional but useful)" OFF) 

# Build documentation
OPTION (BUILD_DOCUMENTATION "Generate API documentation" OFF) 

# Build examples
OPTION (BUILD_EXAMPLES "Build example code" OFF) 

# Build unit tests
OPTION (BUILD_TEST "Build unit tests, requires Boost.Test library" OFF) 

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
  ADD_DEFINITIONS ( "-O3 --std=c++0x --stdlib=c++ -fPIC" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  ADD_DEFINITIONS ( "-O3 --std=c++0x -fPIC -Wall -Wno-sign-compare -Wno-unused-variable" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  MESSAGE ( STATUS "Intel Compiler not tested yet, falling back to default compiler flags" )
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # use default msvc flags
endif()

#------------------------------------------------------------
# Generate configuration
#------------------------------------------------------------

MESSAGE ( STATUS "Generating src/config.hpp" )
CONFIGURE_FILE (
    ${CMAKE_SOURCE_DIR}/src/config.hpp.in
    ${CMAKE_SOURCE_DIR}/src/config.hpp
)

#------------------------------------------------------------
# Include And Configure Libraries Used
#------------------------------------------------------------

IF ( UNIX )
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
        find_package (Boost COMPONENTS system unit_test_framework REQUIRED)
    ELSE ( BUILD_TEST )
        find_package ( Boost REQUIRED )
    ENDIF ( BUILD_TEST )
    
    include_directories( ${Boost_INCLUDE_DIRS} )
ELSE ( UNIX )
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
    IF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/boost )
        SET ( BOOST_ROOT ${CMAKE_SOURCE_DIR}/deps/boost )
    ENDIF ( EXISTS ${CMAKE_SOURCE_DIR}/deps/boost )
    
    IF ( BUILD_TEST )
        find_package (Boost COMPONENTS system unit_test_framework REQUIRED)
    ELSE ( BUILD_TEST )
        find_package ( Boost REQUIRED )
    ENDIF ( BUILD_TEST )
    
ENDIF ( UNIX )

include_directories( ${CMAKE_SOURCE_DIR}/src )

#------------------------------------------------------------
# Generate protobuffer files and target
#------------------------------------------------------------

FILE( GLOB ProtoFiles ${CMAKE_SOURCE_DIR}/proto/*.proto )
EXECUTE_PROCESS ( COMMAND ${PROTOC} 
    ${ProtoFiles}
    --cpp_out=${CMAKE_SOURCE_DIR}/src
    --proto_path=${CMAKE_SOURCE_DIR}/proto
)

#------------------------------------------------------------
# Build library
#------------------------------------------------------------

SET ( PROTO_SOURCES
    src/ai_activity.pb.cc
    src/demo.pb.cc
    src/dota_commonmessages.pb.cc
    src/dota_modifiers.pb.cc
    src/dota_usermessages.pb.cc
    src/netmessages.pb.cc
    src/networkbasetypes.pb.cc
    src/usermessages.pb.cc
)

SET ( ALICE_CORE_SOURCES 
    src/bitstream.cpp 
    src/entity.cpp 
    src/gamestate.cpp
    src/property.cpp
    src/reader.cpp
    src/stringtable.cpp
)

SET ( ALICE_ADDON_SOURCES
    src/keyvalue.cpp
)

# alice-core
ADD_LIBRARY ( alice-core OBJECT ${ALICE_CORE_SOURCES} ${PROTO_SOURCES} ${SNAPPY_SOURCES} )
ADD_LIBRARY ( alice-core-shared SHARED $<TARGET_OBJECTS:alice-core>)
ADD_LIBRARY ( alice-core-static STATIC $<TARGET_OBJECTS:alice-core>)

TARGET_LINK_LIBRARIES ( alice-core-shared
	${PROTOBUF_LIBRARY}
	${SNAPPY_LIBRARIES}
	${Boost_LIBRARIES}
)

# alice-addon
IF ( BUILD_ADDON )
    ADD_LIBRARY ( alice-addon OBJECT ${ALICE_ADDON_SOURCES} )
    ADD_LIBRARY ( alice-addon-shared SHARED $<TARGET_OBJECTS:alice-addon>)
    ADD_LIBRARY ( alice-addon-static STATIC $<TARGET_OBJECTS:alice-addon>)
ENDIF ( BUILD_ADDON )

#------------------------------------------------------------
# Install headers and lib
#------------------------------------------------------------

SET ( PROTO_HEADERS
    src/ai_activity.pb.h
    src/demo.pb.h
    src/dota_commonmessages.pb.h
    src/dota_modifiers.pb.h
    src/dota_usermessages.pb.h
    src/netmessages.pb.h
    src/networkbasetypes.pb.h
    src/usermessages.pb.h
)

SET ( ALICE_CORE_HEADERS
    src/bitstream.hpp
    src/delegate.hpp
    src/entity.hpp
    src/exception.hpp
    src/gamestate.hpp
    src/handler.hpp
    src/handler_detail.hpp
    src/handler_impl.hpp
    src/multiindex.hpp
    src/property.hpp
    src/reader.hpp
    src/sendprop.hpp
    src/sendtable.hpp
    src/stringtable.hpp
)

SET ( ALICE_ADDON_HEADERS
    src/defs.hpp
    src/keyvalue.hpp
    src/monitor.hpp
    src/queue.hpp
    src/timer.hpp
    src/tree.hpp
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
        alice-core-static
        ${PROTOBUF_LIBRARY}
    	${SNAPPY_LIBRARIES}
    	${Boost_LIBRARIES}
    )
    
    INSTALL( TARGETS alice-example RUNTIME DESTINATION bin )
ENDIF ( BUILD_EXAMPLE )

#------------------------------------------------------------
# Generate API docs
#------------------------------------------------------------

IF ( BUILD_DOCUMENTATION )
    EXECUTE_PROCESS ( COMMAND doxygen Doxyfile )
ENDIF ( BUILD_DOCUMENTATION )