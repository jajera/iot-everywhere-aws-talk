# PlatformIO dual framework (arduino + espidf) registers the sketch as an IDF component.
set(app_sources
  "${CMAKE_SOURCE_DIR}/src/main.cpp"
  "${CMAKE_SOURCE_DIR}/src/status_led.cpp"
  "${CMAKE_SOURCE_DIR}/src/wifi_manager.cpp"
  "${CMAKE_SOURCE_DIR}/src/mqtt_manager.cpp"
  "${CMAKE_SOURCE_DIR}/src/ntp_sync.cpp"
  "${CMAKE_SOURCE_DIR}/src/telemetry_publisher.cpp"
  "${CMAKE_SOURCE_DIR}/src/event_publisher.cpp"
  "${CMAKE_SOURCE_DIR}/src/watchdog_supervisor.cpp"
)
idf_component_register(SRCS ${app_sources} INCLUDE_DIRS "." "../include")
