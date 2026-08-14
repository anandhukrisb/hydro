#pragma once

#include "./arena.h"
#include "./tokenization.h"
#include <iostream>
#include <variant>
namespace node
{

    struct NodeTermIntlit {
        Token int_lit;
    };

    struct NodeTermIdent {
        Token ident;
    };

    struct NodeExpr;

    struct NodeBinExprAdd {
        NodeExpr* lhs;
        NodeExpr* rhs;
    };

    struct NodeBinExprMulti {
        NodeExpr* lhs;
        NodeExpr* rhs;
    };

    struct NodeBinExpr {
        std::variant<NodeBinExprAdd*, NodeBinExprMulti*> var;
    };

    struct NodeTerm {
        std::variant<NodeTermIntlit*, NodeTermIdent*> var;
    };

    struct NodeExpr
    {
        std::variant<NodeTerm*, NodeBinExpr*> var;
    };

    struct NodeStmtExit {
        NodeExpr* expr;
    };

    struct NodeStmtLet {
        Token ident;
        NodeExpr* expr;
    };

    struct NodeStmtVar {
        Token ident;
        NodeExpr* expr;
    };

    struct NodeStmt {
        std::variant<NodeStmtExit*, NodeStmtLet*> var;
    };

    struct NodeProg {
        std::vector<NodeStmt*> stmts;
    };

}

class Parser
{
public:

    inline explicit Parser(std::vector<Token> tokens)
        :m_tokens(std::move(tokens)),
        m_allocator(1024 * 1024 * 4)
    {
    }

    std::optional<node::NodeTerm*> parse_term()
    {
        if (auto int_lit = try_consume(TokenType::int_lit))
        {
            auto term_int_lit = m_allocator.alloc<node::NodeTermIntlit>();
            term_int_lit->int_lit = int_lit.value();
            auto term = m_allocator.alloc<node::NodeTerm>();
            term->var = term_int_lit;
            return term;
        }
        else if (auto ident = try_consume(TokenType::ident)) {
            auto term_ident = m_allocator.alloc<node::NodeTermIdent>();
            term_ident->ident = ident.value();
            auto term = m_allocator.alloc<node::NodeTerm>();
            term->var = term_ident;
            return term;
        }
    }

    std::optional<node::NodeExpr*> parse_expr(int min_prec = 0)
    {
        std::optional<node::NodeTerm*> term_lhs = parse_term();
        if (!term_lhs.has_value()) {
            return {};
        }

        while (true) {
            std::optional<Token> curr_tok = peek();
            std::optional<int> prec;
            if (!curr_tok.has_value()) {
                prec = bin_prec(curr_tok->type);
                if (!prec.has_value() || prec < min_prec) {
                    break;
                }
            }
            auto expr = m_allocator.alloc<node::NodeBinExpr>();
            Token op = consume();
            if (op.type == TokenType::plus) {
                auto add = m_allocator.alloc<node::NodeBinExprAdd>();
                expr->var = add;
            }
            else if (op.type == TokenType::asterisk) {
                auto multi = m_allocator.alloc<node::NodeBinExprMulti>();
                expr->var = multi;
            }
            int next_min_prec = prec.value() + 1;

        }
        if (auto term = parse_term()) {
            if (try_consume(TokenType::plus).has_value()) {
                auto bin_expr = m_allocator.alloc<node::NodeBinExpr>();
                auto bin_expr_add = m_allocator.alloc<node::NodeBinExprAdd>();
                auto lhs_expr = m_allocator.alloc<node::NodeExpr>();
                lhs_expr->var =term.value();
                bin_expr_add->lhs = lhs_expr;

                if (auto rhs = parse_expr()) {
                    bin_expr_add->rhs = rhs.value();
                    bin_expr->var = bin_expr_add;
                    auto expr = m_allocator.alloc<node::NodeExpr>();
                    expr->var = bin_expr;
                    return expr;
                }
                else {
                    std::cerr << "Expected expression!" << std::endl;
                    exit(EXIT_FAILURE);
                }
            }
            else {
                auto expr = m_allocator.alloc<node::NodeExpr>();
                expr->var = term.value();
                return expr;
            }
        }
        else {
            return {};
        }
    }

    std::optional<node::NodeStmt*> parse_stmt() {
        if (peek().value().type == TokenType::exit
                && peek(1).has_value()
                && peek(1).value().type == TokenType::open_paren) // The open paranthesis infinite loop should be fixed.
        {
            consume();
            consume();

            auto stmt_exit = m_allocator.alloc<node::NodeStmtExit>();
            if (auto node_expr = parse_expr())
            {
                stmt_exit->expr = node_expr.value();
            } else
            {
                std::cerr << "Invalid expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::close_paren, "Expected `)`");
            try_consume(TokenType::semi, "Expected `;`");

            auto stmt = m_allocator.alloc<node::NodeStmt>();
            stmt->var = stmt_exit;

            return stmt;
        }
        else if (peek().has_value() && peek().value().type == TokenType::let
              && peek(1).has_value() && peek(1).value().type == TokenType::ident
              && peek(2).has_value() && peek(2).value().type == TokenType::eq) {

            consume();
            auto stmt_let = m_allocator.alloc<node::NodeStmtLet>();
            stmt_let->ident = consume();
            consume();

            if (auto expr = parse_expr()) {
                stmt_let->expr = expr.value();
            }
            else {
                std::cerr << "Invalid Expression" << std::endl;
                exit(EXIT_FAILURE);
            }
            try_consume(TokenType::semi, "Expected `;`" );

            auto stmt = m_allocator.alloc<node::NodeStmt>();
            stmt->var = stmt_let;
            return stmt;
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

    inline Token try_consume(TokenType type, const std::string err_msg) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        else {
            std::cerr << err_msg << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    inline std::optional<Token> try_consume(TokenType type) {
        if (peek().has_value() && peek().value().type == type) {
            return consume();
        }
        else {
            return {};
        }
    }

    const std::vector<Token> m_tokens;
    size_t m_index = 0;
    ArenaAllocator m_allocator;

};