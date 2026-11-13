#include "lexer.hpp"
#include "parser.hpp"
#include "type_checker.hpp"
#include "codegen.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <random>
#include <sys/wait.h>

namespace {

#ifndef EXATRON_VERSION
#define EXATRON_VERSION "0.1.0"
#endif

void print_usage(std::ostream& os) {
    os << "Usage:\n";
    os << "  exatron <input.ex> [-o <output_binary>] [--emit-cpp] [--dump-tokens] [--run] [-h|--help] [--version]\n";
}

std::string shell_quote(const std::string& path) {
    std::string res = "'";
    for (char c : path) {
        if (c == '\'') res += "'\\''";
        else res += c;
    }
    res += "'";
    return res;
}

std::string get_cxx_compiler() {
    const char* env_cxx = std::getenv("EXATRON_CXX");
    if (env_cxx && env_cxx[0] != '\0') {
        return env_cxx;
    }
#ifdef EXATRON_DEFAULT_CXX
    return EXATRON_DEFAULT_CXX;
#else
    return "g++";
#endif
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(std::cerr);
        return 1;
    }

    std::string input_file;
    std::string output_binary;
    bool emit_cpp = false;
    bool dump_tokens = false;
    bool run_after = false;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(std::cout);
            return 0;
        } else if (arg == "--version") {
            std::cout << "Exatron compiler v" << EXATRON_VERSION << "\n";
            return 0;
        } else if (arg == "-o" && i + 1 < argc) {
            output_binary = argv[++i];
        } else if (arg == "--emit-cpp") {
            emit_cpp = true;
        } else if (arg == "--dump-tokens") {
            dump_tokens = true;
        } else if (arg == "--run") {
            run_after = true;
        } else if (arg[0] != '-') {
            if (input_file.empty()) {
                input_file = arg;
            } else {
                std::cerr << "Error: Multiple input files specified.\n";
                print_usage(std::cerr);
                return 1;
            }
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            print_usage(std::cerr);
            return 1;
        }
    }

    if (input_file.empty()) {
        std::cerr << "Error: No input file specified.\n";
        print_usage(std::cerr);
        return 1;
    }

    // Read source file
    std::ifstream file(input_file);
    if (!file) {
        std::cerr << "Error: Cannot open file '" << input_file << "'\n";
        return 1;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();
    file.close();

    // Derive stem and binary path
    std::filesystem::path input_path(input_file);
    std::string stem = input_path.stem().string();
    std::filesystem::path bin_path = output_binary.empty() ? std::filesystem::path(stem) : std::filesystem::path(output_binary);

    try {
        // Step 1: Lexing
        exatron::Lexer lexer(source);
        auto tokens = lexer.tokenize();

        if (dump_tokens) {
            for (const auto& tok : tokens) {
                std::cout << "Line " << tok.loc.line << ", Col " << tok.loc.col
                          << ": " << exatron::token_type_to_string(tok.type)
                          << " '" << tok.lexeme << "'\n";
            }
            return 0;
        }

        // Step 2: Parsing
        exatron::Parser parser(tokens);
        auto program = parser.parse_program();

        // Step 3: Type Checking
        exatron::TypeChecker checker;
        checker.check_program(program);

        // Step 4: Code Generation
        exatron::CodeGenerator codegen;
        std::string cpp_code = codegen.generate(program);

        // Determine C++ source path
        std::filesystem::path cpp_path;
        bool is_temp_cpp = false;

        if (emit_cpp) {
            if (!output_binary.empty()) {
                cpp_path = std::filesystem::path(output_binary + ".cpp");
            } else {
                cpp_path = std::filesystem::path(stem + ".cpp");
            }
        } else {
            static std::random_device rd;
            static std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint64_t> dis;
            std::string tmp_name = "exatron_" + std::to_string(dis(gen)) + "_" + stem + ".cpp";
            cpp_path = std::filesystem::temp_directory_path() / tmp_name;
            is_temp_cpp = true;
        }

        // Write generated C++ source
        {
            std::ofstream out(cpp_path);
            if (!out) {
                std::cerr << "Error: Cannot write to '" << cpp_path.string() << "'\n";
                return 1;
            }
            out << cpp_code;
        }

        if (emit_cpp) {
            std::cout << "Generated: " << cpp_path.string() << "\n";
        }

        // Step 5: Native Compilation via g++ / EXATRON_CXX
        std::string cxx = get_cxx_compiler();
        std::string compile_cmd = shell_quote(cxx) + " -std=c++23 -O2 " + shell_quote(cpp_path.string()) + " -o " + shell_quote(bin_path.string()) + " 2>&1";
        int compile_result = std::system(compile_cmd.c_str());

        if (is_temp_cpp) {
            std::error_code ec;
            std::filesystem::remove(cpp_path, ec);
        }

        if (!WIFEXITED(compile_result) || WEXITSTATUS(compile_result) != 0) {
            std::cerr << "Error: Backend C++ compilation failed.\n";
            return 1;
        }

        // Step 6: Optional Immediate Execution
        if (run_after) {
            std::filesystem::path abs_bin = std::filesystem::absolute(bin_path);
            std::string run_cmd = shell_quote(abs_bin.string());
            int run_result = std::system(run_cmd.c_str());
            if (WIFEXITED(run_result)) {
                return WEXITSTATUS(run_result);
            }
            return 1;
        }

    } catch (const exatron::LexerError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const exatron::ParseError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    } catch (const exatron::TypeError& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}
