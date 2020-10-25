#pragma once

#include <iterator>

namespace intrusive {

    struct default_tag;

    template<typename Tag = default_tag>
    struct list_element {

        list_element<Tag> *next = nullptr;
        list_element<Tag> *prev = nullptr;

        list_element<Tag>() = default;

        virtual ~list_element() {
            unlink();
        }

        list_element(list_element &&other) {
            next = other.next;
            prev = other.prev;
            other.next = other.prev = nullptr;
        }

        list_element &operator=(list_element &&other) {
            next = other.next;
            prev = other.prev;
            other.next = other.prev = nullptr;
            return *this;
        }

        void unlink() {
            if (prev != nullptr && next != nullptr) {
                prev->next = next;
                next->prev = prev;
            }
            prev = next = nullptr;
        }

    };

    template<typename T, typename Tag>
    struct list_iterator {

        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = std::remove_const_t<T>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type *;
        using reference = value_type &;
        using base = std::conditional_t<std::is_const<T>::value,
                list_element<Tag> const, list_element<Tag> >;

        static_assert(std::is_convertible_v<value_type &, list_element<Tag> &>,
                      "value type is not convertible to list_element");

        list_iterator() = default;

        list_iterator(base &other) noexcept: current(&other) {}

        template<typename V>
        list_iterator(list_iterator<V, Tag> other, std::enable_if_t<
                std::is_same_v<V, std::remove_const_t<T>> && std::is_const_v<T>> * = nullptr) noexcept : current(
                &(*other)) {}


        T &operator*() const noexcept {
            return static_cast<T &>(*current);
        }

        T *operator->() const noexcept {
            return static_cast<T *>(current);
        }

        list_iterator &operator++() & noexcept {
            current = current->next;
            return *this;
        }

        list_iterator &operator--() & noexcept {
            current = current->prev;
            return *this;
        }

        list_iterator operator++(int) & noexcept {
            list_iterator temp(current);
            current = current->next;
            return temp;
        }

        list_iterator operator--(int) & noexcept {
            list_iterator temp(current);
            current = current->prev;
            return temp;
        }

        bool operator==(list_iterator const &rhs) const & noexcept {
            return (current == rhs.current);
        }

        bool operator!=(list_iterator const &rhs) const & noexcept {
            return (current != rhs.current);
        }

    private:
        explicit list_iterator(list_element<Tag> *current) noexcept: current(current) {}

        base *current;
    };

    template<typename T, typename Tag = default_tag>
    struct list {
        typedef list_iterator<T, Tag> iterator;
        typedef list_iterator<const T, Tag> const_iterator;

        static_assert(std::is_convertible_v<T &, list_element<Tag> &>,
                      "value type is not convertible to list_element");

        list() noexcept {
            make_empty_head();
        };

        list(list const &) = delete;

        list(list &&other) noexcept {
            if (this != &other) {
                if (!other.empty()) {
                    take_head_from(other);
                } else {
                    make_empty_head();
                }
            }
        };

        ~list() {
            clear();
        }

        list &operator=(list const &) = delete;

        list &operator=(list &&other) noexcept {
            if (this != &other) {
                clear();//free all previous list nodes;
                if (!other.empty()) {
                    take_head_from(other);
                }
            }
            return *this;
        };

        void clear() noexcept {
            while (!this->empty()) {
                erase(begin());
            }
        }

        void push_back(T &el) noexcept {
            insert(end(), el);
        }

        void pop_back() noexcept {
            head.prev->unlink();
        }

        T &back() noexcept {
            iterator temp = end();
            --temp;
            return *temp;
        }

        T const &back() const noexcept {
            const_iterator temp = end();
            --temp;
            return *temp;
        }

        void push_front(T &el) noexcept {
            insert(begin(), el);
        }

        void pop_front() noexcept {
            erase(begin());
        }

        T &front() noexcept {
            iterator temp = end();
            ++temp;
            return *temp;
        }

        T const &front() const noexcept {
            const_iterator temp = end();
            ++temp;
            return *temp;
        }

        bool empty() const noexcept {
            return (head.next == &head);
        }

        iterator begin() noexcept {
            return iterator(*head.next);
        }

        const_iterator begin() const noexcept {
            return const_iterator(*head.next);
        }

        iterator end() noexcept {
            return iterator(head);
        }

        const_iterator end() const noexcept {
            return const_iterator(head);
        }

        iterator insert(const_iterator itr, T &el) noexcept {
            list_element<Tag> const &ll = *itr;
            auto &next_element = const_cast<list_element<Tag> &>(ll);
            list_element<Tag> &element = el;
            return insert_before(next_element, element);
//            element.prev = next_element.prev;
//            element.prev->next = next_element.prev = &element;
//            element.next = &next_element;
//            return iterator(element);
        }

        iterator erase(const_iterator itr) noexcept {
            list_element<Tag> const &ll = *itr;
            auto &list = const_cast<list_element<Tag> &>(ll);
            list_element<Tag> &list1 = *list.next;
            list.unlink();
            return list1;
        }

        void splice(const_iterator pos, list &other, const_iterator first, const_iterator last) noexcept {
            while (first != last) {
                T &cur_element = const_cast<T &>(*first);
                first = erase(first);
                other.insert(pos, cur_element);
            }
        }

        void insert_before_element(T &element_after_inserted, T &element) {
            list_element<Tag> &el = element_after_inserted;
            insert_before(el, element);
        }

        iterator insert_before(list_element<Tag>  & next_element, list_element<Tag> & element) {
            element.prev = next_element.prev;
            element.prev->next = next_element.prev = &element;
            element.next = &next_element;
            return iterator(element);
        }

    private :

        void take_head_from(list &other) {
            head.next = other.head.next;
            head.prev = other.head.prev;
            head.next->prev = head.prev->next = &head;
            other.make_empty_head();
        }

        void make_empty_head() {
            head.next = head.prev = &head;
        }

        list_element<Tag> head;
    };
}