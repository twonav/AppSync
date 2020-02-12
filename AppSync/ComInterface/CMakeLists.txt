if(Linux_ARM)
	set(SOURCES_ComInterface
		${CMAKE_CURRENT_SOURCE_DIR}/AppSyncServerComBtStack.cpp
		${CMAKE_CURRENT_SOURCE_DIR}/AppSyncClientComBtStack.cpp
		PARENT_SCOPE
	)
endif()

