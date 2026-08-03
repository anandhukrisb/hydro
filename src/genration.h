#pragma once
#include "parser.h"
#include <unordered_map>

class Generator {
public:
    inline Generator(node::NodeProg prog)
        :m_prog(std::move(prog)) {

    }

    void gen_expr(const node::NodeExpr expr) {

        struct ExprVisitor {

            Generator* gen;

            void operator()(const node::NodeExprIntLit& expr_int_lit) const
            {
                gen->m_output << "mov rax, " << expr_int_lit.int_lit.value.value() << "\n";
                gen->push("rax");
            }

            void operator()(const node::NodeExprIdent& expr_ident) {

            }
        };

        ExprVisitor expr_visitor{ .gen = this };
        std::visit(expr_visitor, expr.var);
    }

    void gen_stmt(const node::NodeStmt& stmt) {
        struct StmtVisitor {

            Generator* gen;

            void operator()(const node::NodeStmtExit& stmt_exit) const
            {

                gen->gen_expr(stmt_exit.expr);
                gen->m_output << "    mov rax, 60\n";
                gen->pop("rdi");
                gen->m_output << "    syscall\n";
            }

            void operator()(const node::NodeStmtLet& stmt_let) {
                if (gen->m_vars.contains(stmt_let.ident.value.value())) {
                    std::cerr << "Identifier already used: " << stmt_let.ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                else {
                    gen->m_vars.insert(
                        { stmt_let.ident.value.value(), Var { .stack_loc = gen->m_stack_size }
                    });

                    gen->gen_expr(stmt_let.expr);
                }
            }
        };

        StmtVisitor visitor{ .gen = this };
        std::visit(visitor, stmt.var);
    }

    [[nodiscard]] std::string gen_prog() {

        m_output << "global _start\n_start:\n";

        for (const node::NodeStmt& stmt : m_prog.stmts) {
            gen_stmt(stmt);
        }

        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall";

        return m_output.str();
    }
private:

    void push(const std::string& reg) {
        m_output << "push " << reg << "\n";
        m_stack_size++;
    }

    void pop(const std::string& reg) {
        m_output << "pop " << reg << "\n";
        m_stack_size--;
    }

    struct Var {
        size_t stack_loc;
    };

    const node::NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_size = 0;
    std::unordered_map<std::string, Var> m_vars {};
};
