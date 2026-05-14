#include <iostream>
#include <fstream>
#include <sstream>
#include <optional>
#include <vector>

enum class TokenType
{
    _return,
    int_lit,
    semi
};

struct Token
{
    TokenType type;
    std::optional<std::string> value {};
};

std::vector<Token> tokenize(const std::string& str)
{
    std::vector<Token> tokens;
    std::string buf;
    for (int i = 0; i < str.length(); i++)
    {
        char c = str.at(i);
        if (std::isalpha(c))
        {
            buf.push_back(c);
            i++;
            while (std::isalnum(str.at(i)))
            {
                buf.push_back(str.at(i));
                i++;
            }
            i--;

            if (buf == "return")
            {
                tokens.push_back({.type = TokenType::_return});
                buf.clear();
                continue;
            } else
            {
                std::cerr << "You messed up!" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
    }
}

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

    tokenize(contents);

    return EXIT_SUCCESS;
}