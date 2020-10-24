#pragma once

#include <functional>
#include "intrusive_list.h"

namespace signals {

    template<typename T>
    struct signal;

    template<typename... Args>
    struct signal<void(Args...)> {

        struct connection;
        using connections_t = intrusive::list<connection, struct connection_tag>;
        using slot_t = std::function<void(Args...)>;

        struct connection : intrusive::list_element<struct connection_tag> {
            connection() noexcept = default;

            connection(signal *sig, slot_t slot) noexcept: sig(sig), slot(std::move(slot)) {
                sig->connections.push_front(*this);
            }

            connection(connection const &) = delete;

            connection &operator=(connection const &) = delete;

            connection(connection &&other) noexcept: sig(other.sig), slot(std::move(other.slot)) {
                other.disconnect();
                other.sig = nullptr;
                if (sig == nullptr) return;
                sig->connections.push_front(*this);
            }

            connection &operator=(connection &&other)  noexcept {
                disconnect();
                sig = other.sig;
                other.disconnect();
                slot = std::move(other.slot);
                if (sig == nullptr) return *this;
                sig->connections.push_front(*this);
            }

            void disconnect() noexcept {
                if (sig == nullptr) return;
                for (iteration_token *tok = sig->top_token; tok != nullptr; tok = tok->next) {
                    if (&*tok->it == this) { /*tok -> it - iterator on current connection*/
                        ++tok->it;
                    }
                }

                this->unlink(); // we are element of intrustic list
                sig = nullptr;
                // some more todo
                /*
                 iteration_token {sig, it, }
                 emit () signal { tok1 tok2 tok3, 1 2 3 4 }
                 emit 1, tok++
                 emit 2
                    conn2->disconnect() -> it_tokens ++
                 tok->it = emit 3, no tok++
                 emit 3
                 emit 4
                 */
            }

            friend struct signal;

        private:
            signal *sig = nullptr;
            slot_t slot;
        };

        struct iteration_token {
            explicit iteration_token(signal const * sig) noexcept: sig(sig) {
                next = sig->top_token;
                sig->top_token = this;
                it = sig->connections.begin();
            }

            iteration_token(const iteration_token &) = delete;

            iteration_token &operator=(const iteration_token &) = delete;

            ~iteration_token() {
                /* удаляем его в конце нашего эмита
                   когда удаляем сам сигнал, должны ли удалить все итератор токены? Нет.
                    мы должны пройтись по всем итератор токенам и сказать что их сигнал удален.
                    в цикле при вызове каждого токен итератора функции чекаем, удален ли наш сигнал.
                    поэтому что же мы должны сделать здесь?
                   когда мы заходим в эмит, мы добавляем наш итератор на вершину, когда мы удаляем его,  мы удаляем его
                    с вершины. Это своего рода стек.
                    поэтому мы должны просто в сигнале сказать что мы больше не на вершине.
                    */
                sig->top_token = next;
            }

        private:
            signal const *sig;
            typename connections_t::const_iterator it;

            iteration_token *next;

            friend struct signal;
        };


        signal() = default;

        signal(signal const &) = delete;

        signal &operator=(signal const &) = delete;

        ~signal() {
            for (iteration_token *tok = top_token; tok != nullptr; tok = tok->next) {
                tok->sig = nullptr;
            }
        }

        connection connect(std::function<void(Args...)> slot) noexcept {
            return connection(this, std::move(slot));
        }

        void operator()(Args... args) const {
            // 1. create it_token
            // iteration while
            iteration_token current_token(this);
            for (; current_token.sig != nullptr, current_token.it != connections.end(); ++current_token.it) {
                current_token.it->slot(std::forward<Args>(args) ...);
            }

        }


    private:
        connections_t connections;
        mutable iteration_token *top_token = nullptr;
    };

}
