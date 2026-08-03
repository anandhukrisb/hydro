#pragma once

#include "./tokenization.h"
#include <iostream>
#include <variant>
namespace node
{

    struct NodeExprIntLit {
        Token int_lit;
    };

    struct NodeExprIdent {
        Token ident;
    };

    struct NodeExpr
    {
        std::variant<NodeExprIntLit, NodeExprIdent> var;
    };

    struct NodeStmtExit {
        NodeExpr expr;
    };

    struct NodeStmtLet {
        Token ident;
        NodeExpr expr;
    };

    struct NodeStmtVar {
        Token ident;
        NodeExpr expr;
    };

    struct NodeStmt {
        std::variant<NodeStmtExit, NodeStmtLet> var;
    };

    struct NodeProg {
        std::vector<NodeStmt> stmts;
    };

}

class Parser
{
public:

    inline explicit Parser(std::vector<Token> tokens)
        :m_tokens(std::move(tokens))
    {
    }

    std::optional<node::NodeExpr> parse_expr()
    {
        if (peek().has_value() && peek().value().type == TokenType::int_lit)
        {
            return node::NodeExpr { .var = node::NodeExprIntLit { .int_lit = consume() } };
        }
        else if (peek().has_value() && peek().value().type == TokenType::ident) {
            return node::NodeExpr { .var = node::NodeExprIdent { .ident = consume() } };
        }
        else
        {
            return {};
        }

    }

    std::optional<node::NodeStmt> parse_stmt() {
        if (peek().value().type == TokenType::exit
                && peek(1).has_value()
                && peek(1).value().type == TokenType::open_paren) // The open paranthesis infinite loop should be fixed.
        {
            consume();
            consume();

            node::NodeStmtExit stmt_exit;
            if (auto node_expr = parse_expr())
            {
                stmt_exit = {.expr = node_expr.value()};
            } else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (peek().has_value() && peek().value().type == TokenType::close_paren) {
                consume();
            }
            else {
                std::cerr << "Expected `)`" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (peek().has_value() && peek().value().type == TokenType::semi) {
                consume();
            }
            else {
                std::cerr << "Expected `;`" << std::endl;
                exit(EXIT_FAILURE);
            }

            return node::NodeStmt{.var = stmt_exit};
        }
        else if (peek().has_value() && peek().value().type == TokenType::let
              && peek(1).has_value() && peek(1).value().type == TokenType::ident
              && peek(2).has_value() && peek(2).value().type == TokenType::eq) {

            consume();
            auto stmt_let = node::NodeStmtLet {.ident = consume()};
            consume();

            if (auto expr = parse_expr()) {
                stmt_let.expr = expr.value();
            }
            else {
                std::cerr << "Invalid Expression" << std::endl;
                exit(EXIT_FAILURE);
            }

            if (peek().has_value() && peek().value().type == TokenType::semi) {
                consume();
            }
            else {
                std::cerr << "Expected `;`" << std::endl;
                exit(EXIT_FAILURE);
            }
            return node::NodeStmt { .var = stmt_let };
        }
        else {
            return {};
        }
    }

    std::optional<node::NodeProg> parse_prog() {
        node::NodeProg prog;
        while (peek().has_value()) {
            if (auto stmt = parse_stmt()) {
                prog.stmts.push_back(stmt.value());
            }
            else {
                std::cerr << "Invalid statement" << std::endl;
                exit(EXIT_FAILURE);
            }
        }

        return prog;
    }

private:

    [[nodiscard]] inline std::optional<Token> peek(int offset = 0) const
    {
        if (m_index + offset >= m_tokens.size())
        {
            return {};
        } else
        {
            return m_tokens.at(m_index + offset);
        }
    }

    inline Token consume()
    {
        return m_tokens.at(m_index++);
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;

};