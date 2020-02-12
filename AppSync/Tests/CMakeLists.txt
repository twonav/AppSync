add_subdirectory(FTPackets)

set(SOURCES_AppSync	
	${SOURCES_FTPackets}
	${CMAKE_CURRENT_SOURCE_DIR}/AppSyncFileServerUnitTest.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/AppSyncFileClientUnitTest.cpp
	PARENT_SCOPE
)

