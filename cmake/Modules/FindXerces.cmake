if (WIN32)
   
  if (NOT XERCES_INCLUDE_DIRS)
    message (FATAL_ERROR "Set XERCES_INCLUDE_DIRS manually")
  endif (NOT XERCES_INCLUDE_DIRS)

  if (NOT XERCES_LIBRARIES)
    message (FATAL_ERROR "Set XERCES_LIBRARIES manually")
  endif (NOT XERCES_LIBRARIES)
   
else (WIN32)
  pkg_check_modules (XERCES REQUIRED xerces-c)
endif (WIN32)

