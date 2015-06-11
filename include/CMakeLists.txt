## configure pkgconfig files ##
CONFIGURE_FILE(voice-control.pc.in voice-control.pc @ONLY)
CONFIGURE_FILE(voice-control-setting.pc.in voice-control-setting.pc @ONLY)
CONFIGURE_FILE(voice-control-widget.pc.in voice-control-widget.pc @ONLY)
CONFIGURE_FILE(voice-control-engine.pc.in voice-control-engine.pc @ONLY)
CONFIGURE_FILE(voice-control-manager.pc.in voice-control-manager.pc @ONLY)

## Install pc files ##
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice-control.pc DESTINATION ${LIBDIR}/pkgconfig)
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice-control-setting.pc DESTINATION ${LIBDIR}/pkgconfig)
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice-control-widget.pc DESTINATION ${LIBDIR}/pkgconfig)
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice-control-manager.pc DESTINATION ${LIBDIR}/pkgconfig)
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice-control-engine.pc DESTINATION ${LIBDIR}/pkgconfig)

## Install header files ##
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_authority.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_command.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_command_expand.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_common.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_key_defines.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_manager.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_plugin_engine.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_setting.h DESTINATION ${INCLUDEDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/include/voice_control_widget.h DESTINATION ${INCLUDEDIR})
