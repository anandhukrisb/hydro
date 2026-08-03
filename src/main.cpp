#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

#include "genration.h"
#include "./parser.h"
#include "./tokenization.h"


int main(int argc, char* argv[])
{

    if (argc != 2)
    {
        std::cout << "Incorrect Usage. Correct usage..." << std::endl;
        std::cout << "hydro <input.hy>" << std::endl;

        return EXIT_FAILURE;
    }

    std::string contents;
    {
        std::stringstream contents_stream;
        std::fstream input(argv[1], std::ios::in);
        contents_stream << input.rdbuf();
        contents = contents_stream.str();
    }

    Tokenizer tokenizer(std::move(contents));

    std::vector<Token> tokens = tokenizer.tokenize();

    // for (const Token& token : tokens) {
    //     if (token.type == TokenType::exit) {
    //         std::cout << "Token: EXIT" << std::endl;
    //     }
    //     else if (token.type == TokenType::int_lit) {
    //         // Remember, value is an optional, so we use .value() to unwrap it!
    //         std::cout << "Token: INT_LIT (Value: " << token.value.value() << ")" << std::endl;
    //     }
    //     else if (token.type == TokenType::semi) {
    //         std::cout << "Token: SEMI (;)" << std::endl;
    //     }
    // }

    Parser parser(std::move(tokens));
    std::optional<node::NodeProg> prog = parser.parse_prog();

    if (!prog.has_value()) {
        std::cerr << "Invalid program!!" << std::endl;
        exit(EXIT_FAILURE);
    }


    {
        Generator generator(prog.value());
        std::fstream file("out.asm", std::ios::out);
        file << generator.gen_prog();
    }



    system("nasm -felf64 out.asm");
    system("ld -o out out.o");

    return EXIT_SUCCESS;
}