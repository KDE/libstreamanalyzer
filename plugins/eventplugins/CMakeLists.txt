MACRO(ADD_STRIGIEA libname files)
  if(WIN32)
    # this is needed to have mingw, cygwin and msvc libs installed in one directory
    if(MSVC)
      set(prefix msvc_strigiea_)
    elseif(CYGWIN)
      set(prefix cyg_strigiea_)
    elseif(MINGW)
      set(prefix mingw_strigiea_)
    endif()
  else()
    set(prefix strigiea_)
  endif()
  add_library(${libname} MODULE ${files})
  set_target_properties(${libname} PROPERTIES
    PREFIX ${prefix})
  target_link_libraries(${libname} streamanalyzer)
  install(TARGETS ${libname} LIBRARY DESTINATION ${LIB_DESTINATION}/strigi)
ENDMACRO(ADD_STRIGIEA)

include_directories(.)

ADD_STRIGIEA(riff riffeventanalyzer.cpp)
ADD_STRIGIEA(digest digesteventanalyzer.cpp)
