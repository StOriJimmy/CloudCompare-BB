cmake_minimum_required(VERSION 3.0)

option( PLUGIN_STANDARD_QM3C2 "Check to install qM3C2 plugin" OFF )

if (PLUGIN_STANDARD_QM3C2)

	project( QM3C2_PLUGIN )

	include_directories( src )

	add_subdirectory( src )
	add_subdirectory( ui )

	include( ../../../CMakePluginTpl.cmake )
	set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "plugins") 
endif()
