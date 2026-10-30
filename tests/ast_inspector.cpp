
#include <tree_sitter/api.h>

#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <cstdint>

extern "C" const TSLanguage* tree_sitter_cpp(void);

void printAST(TSNode node, const std::string& source, int depth = 0) {
    for (int i = 0; i < depth; ++i) {
        std::cout << "  ";
    }

    const std::uint32_t start = ts_node_start_byte(node);
    const std::uint32_t end = ts_node_end_byte(node);

    std::string text = source.substr(start, end - start);

    if (text.size() > 60) {
        text = text.substr(0, 57) + "...";
    }

    std::cout << ts_node_type(node) << " : " << text << '\n';

    const std::uint32_t count = ts_node_child_count(node);

    for (std::uint32_t i = 0; i < count; ++i) {
        printAST(ts_node_child(node, i), source, depth + 1);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: ast_inspector <input.cpp>\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);

    if (!input) {
        std::cerr << "Could not open input file\n";
        return 1;
    }

    std::string source{
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>()
    };

    TSParser* parser = ts_parser_new();

    if (!parser) {
        std::cerr << "Could not create parser\n";
        return 1;
    }

    if (!ts_parser_set_language(parser, tree_sitter_cpp())) {
        std::cerr << "Could not load C++ grammar\n";
        ts_parser_delete(parser);
        return 1;
    }

    TSTree* tree = ts_parser_parse_string(
        parser,
        nullptr,
        source.data(),
        static_cast<std::uint32_t>(source.size())
    );

    if (!tree) {
        std::cerr << "Parsing failed\n";
        ts_parser_delete(parser);
        return 1;
    }

    TSNode root = ts_tree_root_node(tree);
    printAST(root, source);

    ts_tree_delete(tree);
    ts_parser_delete(parser);

    return 0;
}
