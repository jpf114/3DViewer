include_guard()

option(THREEDVIEWER_RUNTIME_POSTBUILD "Copy runtime DLLs and data after build" ON)
set(THREEDVIEWER_RUNTIME_ROOT "" CACHE PATH "vcpkg installed/<triplet> prefix for runtime data (share/gdal, share/proj, osgPlugins)")
set(THREEDVIEWER_OSG_PLUGINS_DIR "" CACHE PATH "Directory containing osgPlugins-*; auto-detected from THREEDVIEWER_RUNTIME_ROOT if empty")

function(_threeviewer_detect_runtime_root out_var)
  if(NOT THREEDVIEWER_RUNTIME_ROOT STREQUAL "")
    set(${out_var} "${THREEDVIEWER_RUNTIME_ROOT}" PARENT_SCOPE)
    return()
  endif()
  if(DEFINED ENV{VCPKG_ROOT})
    file(TO_CMAKE_PATH "$ENV{VCPKG_ROOT}" _vcpkg_root)
    if(EXISTS "${_vcpkg_root}/installed/x64-windows")
      set(${out_var} "${_vcpkg_root}/installed/x64-windows" PARENT_SCOPE)
      return()
    endif()
  endif()
  set(${out_var} "" PARENT_SCOPE)
endfunction()

function(threeviewer_runtime_post_build target)
  if(NOT THREEDVIEWER_RUNTIME_POSTBUILD)
    return()
  endif()

  _threeviewer_detect_runtime_root(_runtime_root)
  if(_runtime_root STREQUAL "")
    return()
  endif()

  if(EXISTS "${_runtime_root}/share/gdal")
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${_runtime_root}/share/gdal"
        "$<TARGET_FILE_DIR:${target}>/share/gdal"
      COMMENT "Copying GDAL data"
    )
  endif()

  if(EXISTS "${_runtime_root}/share/proj")
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${_runtime_root}/share/proj"
        "$<TARGET_FILE_DIR:${target}>/share/proj"
      COMMENT "Copying PROJ data"
    )
  endif()

  set(_osg_plugins_dir "${THREEDVIEWER_OSG_PLUGINS_DIR}")
  if(_osg_plugins_dir STREQUAL "")
    file(GLOB _osg_plugins_dirs "${_runtime_root}/plugins/osgPlugins-*")
    if(_osg_plugins_dirs)
      list(GET _osg_plugins_dirs 0 _osg_plugins_dir)
    endif()
  endif()

  if(_osg_plugins_dir AND EXISTS "${_osg_plugins_dir}")
    get_filename_component(_osg_plugins_name "${_osg_plugins_dir}" NAME)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${_osg_plugins_dir}"
        "$<TARGET_FILE_DIR:${target}>/${_osg_plugins_name}"
      COMMENT "Copying OSG plugins"
    )
  endif()

  if(WIN32)
    add_custom_command(TARGET ${target} POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${target}>
        $<TARGET_FILE_DIR:${target}>
      COMMAND_EXPAND_LISTS
      COMMENT "Copying runtime DLLs via TARGET_RUNTIME_DLLS"
    )

    set(_qt_plugin_root
      "$<IF:$<CONFIG:Debug>,${_runtime_root}/debug/Qt6/plugins,${_runtime_root}/Qt6/plugins>"
    )
    foreach(_plugin_dir platforms styles imageformats)
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
          "$<TARGET_FILE_DIR:${target}>/${_plugin_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${_qt_plugin_root}/${_plugin_dir}"
          "$<TARGET_FILE_DIR:${target}>/${_plugin_dir}"
        COMMENT "Copying Qt ${_plugin_dir} plugins"
      )
    endforeach()
  endif()
endfunction()

function(threeviewer_install_target_release_only target)
  install(TARGETS ${target}
    RUNTIME DESTINATION .
    CONFIGURATIONS Release
  )
endfunction()

function(threeviewer_install_runtime_data_release)
  _threeviewer_detect_runtime_root(_runtime_root)
  if(_runtime_root STREQUAL "")
    return()
  endif()

  if(EXISTS "${_runtime_root}/share/gdal")
    install(DIRECTORY "${_runtime_root}/share/gdal"
      DESTINATION share
      CONFIGURATIONS Release
    )
  endif()

  if(EXISTS "${_runtime_root}/share/proj")
    install(DIRECTORY "${_runtime_root}/share/proj"
      DESTINATION share
      CONFIGURATIONS Release
    )
  endif()

  set(_osg_plugins_dir "${THREEDVIEWER_OSG_PLUGINS_DIR}")
  if(_osg_plugins_dir STREQUAL "")
    file(GLOB _osg_plugins_dirs "${_runtime_root}/plugins/osgPlugins-*")
    if(_osg_plugins_dirs)
      list(GET _osg_plugins_dirs 0 _osg_plugins_dir)
    endif()
  endif()

  if(_osg_plugins_dir AND EXISTS "${_osg_plugins_dir}")
    get_filename_component(_osg_plugins_name "${_osg_plugins_dir}" NAME)
    install(DIRECTORY "${_osg_plugins_dir}/"
      DESTINATION "${_osg_plugins_name}"
      CONFIGURATIONS Release
    )
  endif()
endfunction()

function(threeviewer_install_windows_packaging_extras)
  if(NOT WIN32)
    return()
  endif()

  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/fonts.conf")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/packaging/fonts.conf"
      DESTINATION .
      CONFIGURATIONS Release
    )
  endif()
endfunction()

function(threeviewer_install_runtime_dlls target)
  if(NOT WIN32)
    return()
  endif()

  install(FILES $<TARGET_RUNTIME_DLLS:${target}>
    DESTINATION .
    CONFIGURATIONS Release
  )

  install(CODE "
    file(GLOB _build_dlls \"${CMAKE_BINARY_DIR}/Release/*.dll\")
    if(_build_dlls)
      file(INSTALL DESTINATION \"\${CMAKE_INSTALL_PREFIX}\"
        TYPE SHARED_LIBRARY
        FILES \${_build_dlls})
    endif()
  " CONFIGURATIONS Release)

  _threeviewer_detect_runtime_root(_runtime_root)
  if(_runtime_root STREQUAL "")
    return()
  endif()

  set(_qt_plugin_root "${_runtime_root}/Qt6/plugins")
  foreach(_plugin_dir platforms styles imageformats)
    if(EXISTS "${_qt_plugin_root}/${_plugin_dir}")
      file(GLOB _plugin_dlls "${_qt_plugin_root}/${_plugin_dir}/*.dll")
      if(_plugin_dlls)
        install(FILES ${_plugin_dlls}
          DESTINATION "${_plugin_dir}"
          CONFIGURATIONS Release
        )
      endif()
    endif()
  endforeach()
endfunction()

function(threeviewer_install_windeployqt_at_install)
  if(NOT WIN32)
    return()
  endif()

  find_program(_windeployqt NAMES windeployqt6 windeployqt
    HINTS "${Qt6_DIR}/../../../bin"
  )

  if(_windeployqt)
    install(CODE "
      execute_process(
        COMMAND \"${_windeployqt}\" \"\${CMAKE_INSTALL_PREFIX}/3DViewer.exe\"
        RESULT_VARIABLE _wdq_result
      )
      if(NOT _wdq_result EQUAL 0)
        message(WARNING \"windeployqt returned \${_wdq_result}\")
      endif()
    "
    CONFIGURATIONS Release
    )
  endif()
endfunction()
