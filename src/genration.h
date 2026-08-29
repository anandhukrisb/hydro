#pragma once
#include <assert.h>

#include "parser.h"
#include <map>
#include<utility>


class Generator {
public:
    inline Generator(node::NodeProg prog)
        :m_prog(std::move(prog)) {

    }

    void gen_term(const node::NodeTerm* term) {
        struct TermVisitor {
            Generator& gen;

            void operator() (const node::NodeTermParen* term_paren) const {
                gen.gen_expr(term_paren->expr);
            }

            void operator()(const node::NodeTermIntlit* term_int_lit) const {
                gen.m_output << "    mov rax, " << term_int_lit->int_lit.value.value() << "\n";
                gen.push("rax");
            }

            void operator()(const node::NodeTermIdent* term_ident) const {

                auto it = std::ranges::find_if(
                    gen.m_vars.cbegin(),
                    gen.m_vars.cend(),
                    [&](const Var& var) { return var.name == term_ident->ident.value.value(); });

                if (it == gen.m_vars.cend()) {
                    std::cerr << "Undeclared variable: " << term_ident->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                const auto& var = (*it);
                std::stringstream offset;
                offset << "QWORD [rsp + " << (gen.m_stack_size - (*it).stack_loc - 1) * 8 << "]";
                gen.push(offset.str());
            }
        };

        TermVisitor term_visitor { .gen = *this };
        std::visit(term_visitor, term->var);
    }

    void gen_bin_expr(const node::NodeBinExpr* bin_expr) {
        struct BinExprVisitor {
            Generator& gen;

            void operator() (const node::NodeBinExprSub* sub) const {
                gen.gen_expr(sub->rhs);
                gen.gen_expr(sub->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    sub rax, rbx\n";
                gen.push("rax");
            }

            void operator() (const node::NodeBinExprAdd* add) const {
                gen.gen_expr(add->rhs);
                gen.gen_expr(add->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    add rax, rbx\n";
                gen.push("rax");
            }

            void operator() (const node::NodeBinExprMulti* multi) const {
                gen.gen_expr(multi->rhs);
                gen.gen_expr(multi->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    mul rax, rbx\n";
                gen.push("rax");
            }

            void operator() (const node::NodeBinExprDiv* div) const {
                gen.gen_expr(div->rhs);
                gen.gen_expr(div->lhs);
                gen.pop("rax");
                gen.pop("rbx");
                gen.m_output << "    div rbx\n";
                gen.push("rax");
            }
        };

        BinExprVisitor bin_expr_visitor { .gen = *this };
        std::visit(bin_expr_visitor, bin_expr->var);
    }

    void gen_expr(const node::NodeExpr* expr) {

        struct ExprVisitor {

            Generator& gen;

            void operator()(const node::NodeTerm* term) const
            {
                gen.gen_term(term);
            }

            void operator()(const node::NodeBinExpr* bin_expr)
            {
                gen.gen_bin_expr(bin_expr);
            }
        };

        ExprVisitor expr_visitor{ .gen = *this };
        std::visit(expr_visitor, expr->var);
    }

    void gen_scope(const node::NodeScope* scope) {
        begin_scope();
        for (const node::NodeStmt* stmt : scope->stmts) {
            gen_stmt(stmt);
        }
        end_scope();
    }

    void gen_stmt(const node::NodeStmt* stmt) {
        struct StmtVisitor {

            Generator& gen;

            void operator()(const node::NodeStmtExit* stmt_exit) const
            {
                gen.gen_expr(stmt_exit->expr);
                gen.m_output << "    mov rax, 60\n";
                gen.pop("rdi");
                gen.m_output << "    syscall\n";
            }

            void operator()(const node::NodeStmtLet* stmt_let) {
                if (std::ranges::find_if(
                    std::as_const(gen.m_vars),
                    [&](const Var& var) {
                        return var.name == stmt_let->ident.value.value();
                    }) != gen.m_vars.cend() ) {
                    std::cerr << "Identifier already used: " << stmt_let->ident.value.value() << std::endl;
                    exit(EXIT_FAILURE);
                }
                gen.m_vars.push_back(
                    { .name = stmt_let->ident.value.value(), .stack_loc = gen.m_stack_size
                });

                gen.gen_expr(stmt_let->expr);
            }

            void operator() (const node::NodeScope* scope) const {
                gen.gen_scope(scope);
            }

            void operator() (const node::NodeStmtIf* stmt_if) const {

                gen.gen_expr(stmt_if->expr);
                gen.pop("rax");

                const std::string label = gen.create_label();
                gen.m_output << "    test rax, rax\n";
                gen.m_output << "    jz " << label << "\n";
                gen.gen_scope(stmt_if->scope);
                gen.m_output << label << ":\n";
            }

        };

        StmtVisitor visitor{ .gen = *this };
        std::visit(visitor, stmt->var);
    }

    [[nodiscard]] std::string gen_prog() {

        m_output << "global _start\n_start:\n";

        for (const node::NodeStmt* stmt : m_prog.stmts) {
            gen_stmt(stmt);
        }

        m_output << "    mov rax, 60\n";
        m_output << "    mov rdi, 0\n";
        m_output << "    syscall";

        return m_output.str();
    }
private:

    void push(const std::string& reg) {
        m_output << "    push " << reg << "\n";
        m_stack_size++;
    }

    void pop(const std::string& reg) {
        m_output << "    pop " << reg << "\n";
        m_stack_size--;
    }

    void begin_scope() {
        m_scopes.push_back(m_vars.size());
    }

    void end_scope() {
        const size_t pop_count = m_vars.size() - m_scopes.back();
        m_output << "    add rsp, " << pop_count * 8 << "\n";
        m_stack_size -= pop_count;

        for (int i = 0; i < pop_count; i++) {
            m_vars.pop_back();
        }

        m_scopes.pop_back();
    }

    std::string create_label() {
        return "label" + std::to_string(label_count++);
    }

    struct Var {
        std::string name;
        size_t stack_loc;
    };

    const node::NodeProg m_prog;
    std::stringstream m_output;
    size_t m_stack_size = 0;
    std::vector<Var> m_vars {};
    std::vector<size_t> m_scopes {};
    int label_count = 0;
};
