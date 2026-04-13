add_subdirectory(include)
add_subdirectory(src)

set(INSTALL_BUNDLE)

add_executable(LTESniffer LTESniffer.cc)
target_link_libraries(LTESniffer DL_Sniffer_lib falcon_common falcon_phy falcon_util falcon_prof)
target_compile_options(LTESniffer PUBLIC "-std=c++11")
list(APPEND INSTALL_BUNDLE LTESniffer)

add_executable(AlignIQToSf0 AlignIQToSf0.c)
target_link_libraries(AlignIQToSf0 srsran_phy)
list(APPEND INSTALL_BUNDLE AlignIQToSf0)

add_executable(UhdCaptureRobust UhdCaptureRobust.cc)
target_link_libraries(UhdCaptureRobust ${UHD_LIBRARIES} ${Boost_LIBRARIES})
target_compile_options(UhdCaptureRobust PUBLIC "-std=c++11")
list(APPEND INSTALL_BUNDLE UhdCaptureRobust)

# nested install targets are not supported by old Cmake on Ubuntu 18
#list (APPEND INSTALL_BUNDLE
#    falcon_capture_probe
#    falcon_eye)
install(
    TARGETS ${INSTALL_BUNDLE}
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})
message(STATUS "Install Bundle: ${INSTALL_BUNDLE}")

#Copy wrapper scripts into build dir

file(GLOB SCRIPTS "*.sh" "*.py")
message(STATUS "Copy scripts: ${SCRIPTS}")
foreach(SCRIPTFILE ${SCRIPTS})
  get_filename_component(FILENAME ${SCRIPTFILE} NAME)
  message(STATUS "Copy file: ${FILENAME}")
  configure_file("${FILENAME}" "${FILENAME}" COPYONLY)
endforeach(SCRIPTFILE)
