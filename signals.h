#pragma once

#include <functional>
#include "intrusive_list.h"

namespace signals {
    struct connection_tag;

    template<typename T>
    struct signal;

    template<typename... Args>
    struct signal<void(Args...)> {

        struct connection;
        using connections_t = intrusive::list<connection, connection_tag>;
        using slot_t = std::function<void(Args...)>;

        struct connection : intrusive::list_element<connection_tag> {

            connection() noexcept = default;

            connection(signal *sig, slot_t slot) noexcept: sig(sig), slot(std::move(slot)) {
                sig->connections.push_back(*this);
            }

            virtual ~connection() {
                disconnect();
            }

            connection(connection const &) = delete;

            connection &operator=(connection const &) = delete;

            connection(connection &&other) noexcept: sig(other.sig), slot(std::move(other.slot)) {
                move_constructor(other);
            }

            connection &operator=(connection &&other) noexcept {
                if (this == &other) return *this;
                disconnect();
                sig = other.sig;
                slot = std::move(other.slot);
                move_constructor(other);
                return *this;
            }

            void disconnect() noexcept {
                if (sig == nullptr) return;
                if (sig->top_token != nullptr) {
                    for (iteration_token *tok = sig->top_token; tok != nullptr; tok = tok->next) {
                        if (&*tok->it == this) {
                            --tok->it;
                        }
                    }
                }
                this->unlink();
                sig = nullptr;
                slot = slot_t();
            }

            friend struct signal;

        private:

            void move_constructor(connection &other) {
                if (sig != nullptr) {
                    sig->connections.insert_before_element(other, *this);
                }
                other.disconnect();
                other.sig = nullptr;
            }

            signal *sig = nullptr;
            slot_t slot;
        };

        struct iteration_token {

            explicit iteration_token(signal const *sig) noexcept: sig(sig) {
                it = sig->connections.begin();
                next = sig->top_token;
                sig->top_token = this;
            }

            iteration_token(const iteration_token &) = delete;

            iteration_token &operator=(const iteration_token &) = delete;

            ~iteration_token() {
                if (sig != nullptr) {
                    sig->top_token = next;
                }
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
            while (!connections.empty()){
                connections.begin()->disconnect();
            }
        }

        connection connect(std::function<void(Args...)> slot) noexcept {
            return connection(this, std::move(slot));
        }

        void operator()(Args... args) const {
            for (iteration_token current_token(this); current_token.it != connections.end(); ++current_token.it) {
                current_token.it->slot(std::forward<Args>(args) ...);
                if (current_token.sig == nullptr)
                    return;
            }
        }

    private:
        connections_t connections;
        mutable iteration_token *top_token = nullptr;
    };

}
