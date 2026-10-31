
if(NOT DEFINED OBFUSCATOR OR NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "Missing required test arguments")
endif()

execute_process(
    COMMAND "${OBFUSCATOR}"
            --input "${INPUT}"
            --output "${OUTPUT}"
            --level 2
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

file(READ "${INPUT}" original)
file(READ "${OUTPUT}" transformed)

# Verify line comment removal.
if(transformed MATCHES "This line comment should be removed")
    message(FATAL_ERROR "Line comment was not removed")
endif()

# Verify block comment removal.
if(transformed MATCHES "This block comment should be removed")
    message(FATAL_ERROR "Block comment was not removed")
endif()

# Verify that comment-like text inside strings is preserved.
if(NOT transformed MATCHES "This // is not a comment")
    message(FATAL_ERROR "String literal was incorrectly modified")
endif()

# Verify variable renaming.
if(NOT transformed MATCHES "int obf_var_1 = 5;")
    message(FATAL_ERROR "Variable transformation failed")
endif()

# Verify newline preservation.
string(REGEX MATCHALL "\n" original_newlines "${original}")
string(REGEX MATCHALL "\n" transformed_newlines "${transformed}")

list(LENGTH original_newlines original_count)
list(LENGTH transformed_newlines transformed_count)

if(NOT original_count EQUAL transformed_count)
    message(FATAL_ERROR
        "Newline count changed: ${original_count} -> ${transformed_count}")
endif()

message(STATUS "Comment removal validation passed")
