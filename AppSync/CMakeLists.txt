add_subdirectory(ComInterface)
add_subdirectory(FTPackets)

if(Linux)
	set(SOURCES_AppSync
		${CMAKE_CURRENT_SOURCE_DIR}/AppSyncFileServer.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/AppSyncFileClient.cpp
	)

	if(Linux_ARM)
		set(SOURCES_AppSync
			${SOURCES_AppSync}
			${CMAKE_CURRENT_SOURCE_DIR}/AppSyncClient.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/AppSyncPushNotification.cpp
		)
	endif()
endif()

set(SOURCES_AppSync
	${SOURCES_AppSync}
	${SOURCES_ComInterface}
	${SOURCES_FTPackets}
	PARENT_SCOPE
)
