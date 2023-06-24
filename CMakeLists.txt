cmake_minimum_required(VERSION 3.10)

project(statemachine
	VERSION 0.1
    DESCRIPTION "Event Driven State Machine"
)

include(GNUInstallDirs)

find_package(BISON)
find_package(FLEX)

FLEX_TARGET( TCC_Scanner src/lexan.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c )
BISON_TARGET( TCC_Parser src/statemachine.y ${CMAKE_CURRENT_BINARY_DIR}/statemachine.tab.c )
ADD_FLEX_BISON_DEPENDENCY(TCC_Scanner TCC_Parser)

add_executable( ${PROJECT_NAME}
	src/lineno.c
    src/statemachine.c
    src/engine.c

	${FLEX_TCC_Scanner_OUTPUTS}
 	${BISON_TCC_Parser_OUTPUTS}
)

target_link_libraries( ${PROJECT_NAME}
	${CMAKE_THREAD_LIBS_INIT}
	rt
	varserver
    varaction
)

target_include_directories( ${PROJECT_NAME} PRIVATE
	.
	inc
	${CMAKE_BINARY_DIR} )

install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )

file(GLOB test_files "test/*.c")

install( FILES ${test_files}
	DESTINATION /usr/share/statemachine
)
