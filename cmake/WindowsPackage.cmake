# Executables are mutually exclusive WiX components, selected by explicit consent.
set(GAZEGRID_CERTIFICATE_THUMBPRINT "" CACHE STRING "Public signing certificate thumbprint")
configure_file("${CMAKE_SOURCE_DIR}/platform/windows/Installer.wxs.in"
  "${CMAKE_BINARY_DIR}/Installer.wxs" @ONLY)
set(CPACK_WIX_EXTRA_SOURCES "${CMAKE_BINARY_DIR}/Installer.wxs")
set(CPACK_WIX_PATCH_FILE "${CMAKE_SOURCE_DIR}/platform/windows/InstallerPatch.xml")
set(CPACK_WIX_EXTENSIONS WixIIsExtension)
# ICE30 does not evaluate mutually exclusive component conditions.
set(CPACK_WIX_LIGHT_EXTRA_FLAGS -sice:ICE30)
install(DIRECTORY references/optikey-keyboard-en-us/ DESTINATION keyboards)
install(FILES LICENSE.txt DESTINATION .)
# windeployqt output is populated by Build-Release.ps1 before CPack runs.
set(GAZEGRID_DEPLOY_DIR "${CMAKE_BINARY_DIR}/deploy" CACHE PATH "Qt deployment directory")
install(DIRECTORY "${GAZEGRID_DEPLOY_DIR}/" DESTINATION .)
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
  message(FATAL_ERROR "Windows MSI packaging currently requires x64")
endif()
# Ship the MSVC runtime app-locally so a clean Windows machine can launch.
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)
set(CPACK_GENERATOR WIX)
set(CPACK_WIX_VERSION 3)
set(CPACK_PACKAGE_NAME GazeGrid)
set(CPACK_PACKAGE_VENDOR GazeGrid)
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY GazeGrid)

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")
set(CPACK_WIX_UPGRADE_GUID "7653E284-B290-4D25-B455-32D2CD89E209")
set(CPACK_WIX_UI_REF GazeGridUI)
set(CPACK_WIX_PROPERTY_ARPNOMODIFY 1)
set(CPACK_WIX_ARCHITECTURE x64)
include(CPack)
