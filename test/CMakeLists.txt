IF ( BUILD_ADDON )
    ADD_EXECUTABLE ( alice-test-tree
        alice/tree.cpp
    )

    TARGET_LINK_LIBRARIES ( alice-test-tree
        alice-core-static
        ${PROTOBUF_LIBRARY}
    	${SNAPPY_LIBRARIES}
    	${Boost_LIBRARIES}
    )
ENDIF ( BUILD_ADDON )