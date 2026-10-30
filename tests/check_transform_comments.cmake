if(NOT DEFINED OBFUSCATOR OR
   NOT DEFINED INPUT OR
   NOT DEFINED OUTPUT)
    message(FATAL_ERROR
        "OBFUSCATOR, INPUT, and OUTPUT are required")
endif()

execute_process(
    COMMAND "${OBFUSCATOR}"
        -i "${INPUT}"
        -o "${OUTPUT}"
        --level 2
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)

if(NOT result EQUAL 0)
    message(FATAL_ERROR
        "Obfuscator failed.\nSTDOUT:\n${stdout}\nSTDERR:\n${stderr}")
endif()

if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "Transformed output was not created")
endif()

file(READ "${OUTPUT}" transformed)

# The line comment must be removed.
string(FIND "${transformed}"
    "This line comment should be removed."
    line_comment_pos)

if(NOT line_comment_pos EQUAL -1)
    message(FATAL_ERROR "Line comment was not removed")
endif()

# The block comment must be removed.
string(FIND "${transformed}"
    "This block comment should be removed."
    block_comment_pos)

if(NOT block_comment_pos EQUAL -1)
    message(FATAL_ERROR "Block comment was not removed")
endif()

# Comment-like text inside the string must remain.
string(FIND "${transformed}"
    "This // is not a comment"
    string_pos)

if(string_pos EQUAL -1)
    message(FATAL_ERROR
        "String literal was incorrectly modified")
endif()

# Variable renaming must still work.
string(FIND "${transformed}" "obf_var_1" renamed_pos)

if(renamed_pos EQUAL -1)
    message(FATAL_ERROR
        "Expected variable renaming at Level 2")
endif()

message(STATUS
    "Level 2 comment-removal test passed")