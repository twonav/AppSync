add_subdirectory(FTPackets)

if(Linux_ARM)
	set(SOURCES_AppSync
		${CMAKE_CURRENT_SOURCE_DIR}/AppSyncClient.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/AppSyncPushNotification.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/AppSyncFileClient.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/AppSyncFileServer.cpp
	)
endif()

set(SOURCES_AppSync
	${SOURCES_AppSync}
	${SOURCES_FTPackets}
	PARENT_SCOPE
)
