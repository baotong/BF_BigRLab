SubDirList(SUBDIRS ${CMAKE_CURRENT_SOURCE_DIR})

foreach(SUBDIR ${SUBDIRS})
    # add_subdirectory(${subdir})
    message("Add sub directory ${SUBDIR}")
endforeach()

