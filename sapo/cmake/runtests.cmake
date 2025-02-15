macro(SAPO_TEST SAPO_EXEC TEST_NAME TEST_DIR TEST_RESULTS_DIR)
    message(${CMAKE_CURRENT_SOURCE_DIR})

    execute_process(COMMAND ${SAPO_EXEC} -t ${TEST_DIR}/${TEST_NAME}.sil
                    RESULT_VARIABLE CMD_RESULT
                    OUTPUT_FILE ${TEST_NAME}.out)

    if(CMD_RESULT)
        message(FATAL_ERROR "Error on test ${TEST_NAME}")
    else()
        execute_process(COMMAND ${CMAKE_COMMAND} -E compare_files ${TEST_RESULTS_DIR}/${TEST_NAME}.out ${TEST_NAME}.out
                            RESULT_VARIABLE CMD_RESULT)
        if(CMD_RESULT)
            message(FATAL_ERROR "Error on test ${TEST_NAME}")
        else()
            file(REMOVE ${TEST_NAME}.out)
        endif()
    endif()
endmacro()

sapo_test(${SAPO_EXEC} ${TEST_NAME} ${TEST_DIR} ${TEST_RESULTS_DIR})
