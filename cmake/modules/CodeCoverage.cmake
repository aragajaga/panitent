find_program(LCOV_PATH lcov)
find_program(GENHTML_PATH genhtml)

function(setup_target_for_coverage _targetname _testrunner _outputname)
    if(NOT LCOV_PATH)
        message(FATAL_ERROR "lconv not found! Aborting...")
    endif()
    
    if (NOT GENHTML_PATH)
        message(FATAL_ERROR "genhtml not found! Aborting...")
    endif()
    
    add_custom_target(${_targetname}
        ${LCOV_PATH} --directory . --zerocounters
        COMMAND ${_testrunner} ${ARGV3}
        COMMAND ${LCOV_PATH} --directory . --capture --output-file ${_outputname}.info
        COMMAND ${LCOV_PATH} --remove ${_outputname}.info 'build/*' 'tests/*' --output-file ${_outputname}.info.cleaned
        COMMAND ${GENHTML_PATH} -o ${_outputname} ${_outputname}.info.cleaned
        COMMAND ${CMAKE_COMMAND} -E remove ${_outputname}.info ${_outputname}.info.cleaned
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        
    add_custom_command(TARGET ${_targetname} POST_BUILD
        COMMAND ;
        COMMENT "Open ./${_outputname}/index.html in your browser to view coverage report.")
endfunction()
