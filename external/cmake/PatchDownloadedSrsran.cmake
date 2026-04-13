if(NOT DEFINED SRSRAN_SRC_DIR)
  message(FATAL_ERROR "SRSRAN_SRC_DIR is required")
endif()

function(patch_file_contains path needle replacement description)
  file(READ "${path}" content)
  string(FIND "${content}" "${replacement}" replacement_pos)
  if(NOT replacement_pos EQUAL -1)
    message(STATUS "${description}: already applied")
    return()
  endif()

  string(FIND "${content}" "${needle}" needle_pos)
  if(needle_pos EQUAL -1)
    message(FATAL_ERROR "${description}: expected snippet not found in ${path}")
  endif()

  string(REPLACE "${needle}" "${replacement}" patched "${content}")
  file(WRITE "${path}" "${patched}")
  message(STATUS "${description}: applied")
endfunction()

function(ensure_file_contains path marker description)
  file(READ "${path}" content)
  string(FIND "${content}" "${marker}" marker_pos)
  if(marker_pos EQUAL -1)
    message(FATAL_ERROR "${description}: expected marker not found in ${path}")
  endif()
  message(STATUS "${description}: already applied")
endfunction()

set(fmt_core "${SRSRAN_SRC_DIR}/lib/include/srsran/srslog/bundled/fmt/core.h")
patch_file_contains(
  "${fmt_core}"
  [=[#include <cstring>
#include <functional>
#include <iterator>]=]
  [=[#include <cstring>
#include <functional>
#include <array>
#include <iterator>]=]
  "fmt <array> include fix")

set(srsran_cmake "${SRSRAN_SRC_DIR}/CMakeLists.txt")
patch_file_contains(
  "${srsran_cmake}"
  [=[# Add -Werror to C/C++ flags for newer compilers
if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
endif()]=]
  [=[# Add -Werror to C/C++ flags for newer compilers
# Disabled for local GCC 12/aarch64 builds where vendored srsRAN emits warnings
# that are non-fatal in practice but stop the full LTESniffer build.
# if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
#   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror")
# endif()]=]
  "srsRAN Werror disable")

set(rf_uhd_generic "${SRSRAN_SRC_DIR}/lib/src/phy/rf/rf_uhd_generic.h")
patch_file_contains(
  "${rf_uhd_generic}"
  [=[    // Set receiver subdevice spec string
    std::string rx_subdev;
    if (dev_addr.has_key("rx_subdev_spec")) {
      rx_subdev = dev_addr.pop("rx_subdev_spec");
    }

    // Set over the wire format]=]
  [=[    // Set receiver subdevice spec string
    std::string rx_subdev;
    if (dev_addr.has_key("rx_subdev_spec")) {
      rx_subdev = dev_addr.pop("rx_subdev_spec");
    }

    // Optional Rx antenna selection, e.g. rxant=RX2 or rxant=TX/RX
    std::string rx_ant;
    if (dev_addr.has_key("rxant")) {
      rx_ant = dev_addr.pop("rxant");
    }

    // Set over the wire format]=]
  "srsRAN UHD rx antenna argument parsing")

patch_file_contains(
  "${rf_uhd_generic}"
  [=[    stream_args.channels.resize(nof_channels);
    for (size_t i = 0; i < (size_t)nof_channels; i++) {
      stream_args.channels[i] = i;
    }
]=]
  [=[    stream_args.channels.resize(nof_channels);
    for (size_t i = 0; i < (size_t)nof_channels; i++) {
      stream_args.channels[i] = i;
    }

    if (not rx_ant.empty()) {
      for (size_t ch = 0; ch < (size_t)nof_channels; ch++) {
        Debug("Setting Rx antenna on channel " << ch << " to " << rx_ant);
        try {
          usrp->set_rx_antenna(rx_ant, ch);
        } catch (const uhd::exception& e) {
          Error(e.what());
          return error_from_uhd_exception(&e);
        } catch (const boost::exception& e) {
          Error(boost::diagnostic_information(e));
          return UHD_ERROR_BOOSTEXCEPT;
        } catch (const std::exception& e) {
          Error(e.what());
          return UHD_ERROR_STDEXCEPT;
        } catch (...) {
          Error("Unrecognized exception caught.");
          return UHD_ERROR_UNKNOWN;
        }
      }
    }
]=]
  "srsRAN UHD rx antenna selection")
