
if(NOT DEFINED OBFUSCATOR OR NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "Missing required test arguments")
endif()

execute_process(
    COMMAND "${OBFUSCATOR}"
            --input "${INPUT}"
            --output "${OUTPUT}"
            --level 1
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator execution failed:\n${stdout}\n${stderr}")
endif()

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "Output file was not generated")
endif()

file(READ "${OUTPUT}" transformed)

# Check that all expected obfuscated identifiers exist.
foreach(identifier obf_var_1 obf_var_2 obf_var_3)
    if(NOT transformed MATCHES "${identifier}")
        message(FATAL_ERROR
            "Expected identifier missing: ${identifier}")
    endif()
endforeach()

# Check that original identifiers have been removed.
foreach(identifier number result)
    if(transformed MATCHES "(^|[^A-Za-z0-9_])${identifier}([^A-Za-z0-9_]|$)")
        message(FATAL_ERROR
            "Original identifier still exists: ${identifier}")
    endif()
endforeach()

message(STATUS "Function parameter transformation test passed")
