SET(SRCS
	../common/vc_command.c
	../common/vc_config_mgr.c
	../common/vc_config_parser.c
	../common/vc_info_parser.c
	vcd_client_data.c
	vcd_config.c
	vcd_dbus_server.c
	vcd_dbus.c
	vcd_engine_agent.c
	vcd_main.c
	vcd_recorder.c
	vcd_server.c
)

INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/common)
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR}/include)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIE")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -fPIE")
SET(CMAKE_C_FLAGS_RELEASE "-O2 -fPIE")
SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -pie")

## Executable ##
ADD_EXECUTABLE("${PROJECT_NAME}-daemon" ${SRCS})
TARGET_LINK_LIBRARIES("${PROJECT_NAME}-daemon" -ldl -lm ${pkgs_LDFLAGS})

## Install
INSTALL(TARGETS "${PROJECT_NAME}-daemon" DESTINATION bin)
