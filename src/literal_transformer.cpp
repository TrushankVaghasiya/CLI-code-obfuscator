#include "literal_transformer.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <tree_sitter/api.h>

extern "C" const TSLanguage* tree_sitter_cpp(void);

namespace {

// Stores the byte range of an integer literal.
struct LiteralEdit {
    uint32_t start;
    uint32_t end;
    std::string replacement;
};

// Return true if the node is inside a preprocessor directive.
bool isInsidePreprocessor(TSNode node) {
    TSNode parent = ts_node_parent(node);

    while (!ts_node_is_null(parent)) {
        std::string type = ts_node_type(parent);

        if (type.rfind("preproc_", 0) == 0) {
            return true;
        }

        parent = ts_node_parent(parent);
    }

    return false;
}

// Initially support only plain decimal integer literals.
//
// Examples accepted: 0, 5, 42, 1234
// Examples skipped: 0xFF, 012, 5U, 10LL, 3.14
bool isPlainDecimalInteger(const std::string& literal) {
    if (literal.empty()) {
        return false;
    }

    // Avoid transforming octal literals such as 012.
    if (literal.size() > 1 && literal[0] == '0') {
        return false;
    }

    return std::all_of(
        literal.begin(),
        literal.end(),
        [](unsigned char ch) {
            return ch >= '0' && ch <= '9';
        }
    );
}

// Find eligible integer literals in the syntax tree.
void collectIntegerLiterals(
    TSNode node,
    const std::string& source,
    std::vector<LiteralEdit>& edits
) {
    std::string type = ts_node_type(node);

    if (type == "number_literal" &&
        !isInsidePreprocessor(node)) {

        uint32_t start = ts_node_start_byte(node);
        uint32_t end = ts_node_end_byte(node);

        if (start <= end && end <= source.size()) {
            std::string literal =
                source.substr(start, end - start);

            if (isPlainDecimalInteger(literal)) {
                edits.push_back({
                    start,
                    end,
                    "(" + literal + " + 0)"
                });
            }
        }

        return;
    }

    uint32_t childCount = ts_node_child_count(node);

    for (uint32_t i = 0; i < childCount; ++i) {
        collectIntegerLiterals(
            ts_node_child(node, i),
            source,
            edits
        );
    }
}

} // namespace

bool transformIntegerLiterals(
    std::string& source,
    std::string& error
) {
    if (source.size() >
        std::numeric_limits<uint32_t>::max()) {

        error = "Source file is too large for this transformation.";
        return false;
    }

    TSParser* parser = ts_parser_new();

    if (parser == nullptr) {
        error = "Failed to create Tree-sitter parser.";
        return false;
    }

    if (!ts_parser_set_language(parser, tree_sitter_cpp())) {
        ts_parser_delete(parser);
        error = "Failed to initialize the C++ grammar.";
        return false;
    }

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        source.data(),
        static_cast<uint32_t>(source.size())
    );

    if (tree == nullptr) {
        ts_parser_delete(parser);
        error = "Failed to parse source for integer transformation.";
        return false;
    }

    TSNode root = ts_tree_root_node(tree);

    if (ts_node_has_error(root)) {
        ts_tree_delete(tree);
        ts_parser_delete(parser);

        error = "Source contains syntax errors during integer transformation.";
        return false;
    }

    std::vector<LiteralEdit> edits;

    collectIntegerLiterals(root, source, edits);

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    // Apply edits from right to left so byte offsets remain valid.
    std::sort(
        edits.begin(),
        edits.end(),
        [](const LiteralEdit& a, const LiteralEdit& b) {
            return a.start > b.start;
        }
    );

    for (const LiteralEdit& edit : edits) {
        source.replace(
            edit.start,
            edit.end - edit.start,
            edit.replacement
        );
    }

    return true;
}