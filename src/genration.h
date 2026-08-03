#pragma once
#include "parser.h"

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
                gen->m_output << "push rax" << "\n";
            }

            void operator()(const node::NodeExprIdent& expr_ident) {
                // TODO
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
                gen->m_output << "    pop rdi\n";
                gen->m_output << "    syscall\n";
            }

            void operator()(const node::NodeStmtLet& stmt_let) {

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
    const node::NodeProg m_prog;
    std::stringstream m_output;
};
