include(FetchContent)
# Pinned upstream client; the separately installed bus driver is a runtime prerequisite.
# SOURCE_SUBDIR avoids upstream's SDK/development-file install rules in our MSI.
FetchContent_Declare(vigemclient
  GIT_REPOSITORY https://github.com/nefarius/ViGEmClient.git
  GIT_TAG b66d02d57e32cc8595369c53418b843e958649b4
  SOURCE_SUBDIR src
)
FetchContent_MakeAvailable(vigemclient)
add_library(gazegrid_vigem STATIC
  "${vigemclient_SOURCE_DIR}/src/ViGEmClient.cpp")
set_target_properties(gazegrid_vigem PROPERTIES AUTOMOC OFF)
target_include_directories(gazegrid_vigem PUBLIC "${vigemclient_SOURCE_DIR}/include")
target_link_libraries(gazegrid_vigem PRIVATE setupapi)
install(FILES "${vigemclient_SOURCE_DIR}/LICENSE" DESTINATION licenses RENAME ViGEmClient-LICENSE.txt)
