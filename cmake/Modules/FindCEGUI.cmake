# Search for CEGUI

if (WIN32)
  # Windows
  message (FATAL_ERROR "Don't know how to find CEGUI on Win32")
else (WIN32)
  # Unix
  find_package (PkgConfig)
  pkg_search_module (CEGUI CEGUI)
  
  set (CEGUI_INCLUDE_DIR ${CEGUI_INCLUDE_DIRS})
  set (CEGUI_LIB_DIR ${CEGUI_LIBDIR})
  set (CEGUI_LIBRARIES ${CEGUI_LIBRARIES})
endif (WIN32)

if (NOT CEGUI_FOUND)
  message (FATAL_ERROR "Could not find CEGUI")
endif (NOT CEGUI_FOUND)
