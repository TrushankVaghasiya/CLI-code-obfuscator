
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

# Verify outer variable declaration and reference.
if(NOT transformed MATCHES "int obf_var_1 = 10;")
    message(FATAL_ERROR "Outer variable declaration was not renamed")
endif()

if(NOT transformed MATCHES "cout << obf_var_1")
    message(FATAL_ERROR "Outer variable reference was not renamed")
endif()

# Verify lambda-local variable declaration and reference.
if(NOT transformed MATCHES "int obf_var_2 = 20;")
    message(FATAL_ERROR "Lambda-local variable declaration was not renamed")
endif()

if(NOT transformed MATCHES "cout << obf_var_2")
    message(FATAL_ERROR "Lambda-local variable reference was not renamed")
endif()

# Verify original identifier is completely removed.
if(transformed MATCHES "(^|[^A-Za-z0-9_])value([^A-Za-z0-9_]|$)")
    message(FATAL_ERROR "Original identifier 'value' still exists")
endif()

message(STATUS "Lambda shadowing validation passed")
