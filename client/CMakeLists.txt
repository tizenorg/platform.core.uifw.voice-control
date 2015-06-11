SET(SRCS 
	vc.c
	vc_client.c
	vc_dbus.c
	../common/vc_command.c
	../common/vc_config_mgr.c
	../common/vc_config_parser.c
	../common/vc_info_parser.c
)

SET(SETTING_SRCS
	vc_setting.c
	../common/vc_config_mgr.c
	../common/vc_config_parser.c
)

SET(WIDGET_SRCS
	vc_widget.c
	vc_widget_client.c
	vc_widget_dbus.c
	../common/vc_command.c
	../common/vc_config_mgr.c
	../common/vc_config_parser.c
	../common/vc_info_parser.c
)

SET(MANAGER_SRCS
	vc_mgr.c
	vc_mgr_client.c
	vc_mgr_dbus.c
	../common/vc_command.c
	../common/vc_config_mgr.c
	../common/vc_config_parser.c
	../common/vc_info_parser.c
)

#INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
#INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)
#INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/common)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS}")

## voice control library ##
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS})

## voice control setting library ##
ADD_LIBRARY("${PROJECT_NAME}_setting" SHARED ${SETTING_SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}_setting" ${pkgs_LDFLAGS})

## voice control widget library ##
ADD_LIBRARY("${PROJECT_NAME}_widget" SHARED ${WIDGET_SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}_widget" ${pkgs_LDFLAGS})

## voice control manager library ##
ADD_LIBRARY("${PROJECT_NAME}_manager" SHARED ${MANAGER_SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}_manager" ${pkgs_LDFLAGS})

## Install library files ##
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
INSTALL(TARGETS "${PROJECT_NAME}_setting" DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
INSTALL(TARGETS "${PROJECT_NAME}_widget" DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
INSTALL(TARGETS "${PROJECT_NAME}_manager" DESTINATION ${LIBDIR} COMPONENT RuntimeLibraries)
