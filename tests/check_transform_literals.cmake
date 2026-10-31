
if(NOT DEFINED OBFUSCATOR OR NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "Missing required test arguments")
endif()

execute_process(
    COMMAND "${OBFUSCATOR}"
            --input "${INPUT}"
            --output "${OUTPUT}"
            --level 3
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

# Verify integer literal transformation.
if(NOT transformed MATCHES "obf_var_1 = \\(7 \\+ 0\\);")
    message(FATAL_ERROR "Integer literal 7 was not transformed correctly")
endif()

# Verify return literal transformation.
if(NOT transformed MATCHES "return \\(0 \\+ 0\\);")
    message(FATAL_ERROR "Return literal 0 was not transformed correctly")
endif()

# Verify character literal preservation.
if(NOT transformed MATCHES "obf_var_2 = 'A';")
    message(FATAL_ERROR "Character literal was incorrectly modified")
endif()

# Verify string literal preservation.
if(NOT transformed MATCHES "Hello // world /\\* not a comment \\*/")
    message(FATAL_ERROR "String literal was incorrectly modified")
endif()

# Verify variable references.
if(NOT transformed MATCHES "cout << obf_var_1")
    message(FATAL_ERROR "First variable reference was not renamed")
endif()

if(NOT transformed MATCHES "cout << obf_var_2")
    message(FATAL_ERROR "Second variable reference was not renamed")
endif()

message(STATUS "Integer literal transformation validation passed")
