if (WIN32)
   
  if (NOT FREETYPE_INCLUDE_DIRS)
    message (FATAL_ERROR "Set FREETYPE_INCLUDE_DIRS manually")
  endif (NOT FREETYPE_INCLUDE_DIRS)

  if (NOT FREETYPE_LIBRARIES)
    message (FATAL_ERROR "Set FREETYPE_LIBRARIES manually")
  endif (NOT FREETYPE_LIBRARIES)

else (WIN32)
  pkg_check_modules (FREETYPE REQUIRED freetype2)
endif (WIN32)
