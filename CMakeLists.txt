project(CFS_BC42_LIB C)

include_directories(fsw/src)
include_directories(fsw/app_inc)

# The shared OSAL and cFE include directories should always be used
# Note that this intentionally does NOT include PSP-specific includes, just the generic
include_directories(${CFECORE_SOURCE_DIR}/src/inc)
include_directories(${CFEPSP_SOURCE_DIR}/fsw/inc)

aux_source_directory(fsw/src LIB_SRC_FILES)

# Create the app module
add_cfe_app(bc42_lib ${LIB_SRC_FILES})
target_link_libraries (bc42_lib m)

