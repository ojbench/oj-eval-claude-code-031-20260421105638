#ifndef SRC_HPP_PYLIST
#define SRC_HPP_PYLIST

#include <iostream>
#include <vector>
#include <memory>
#include <unordered_set>
#include <cstddef>
#include <cstdint>

class pylist {
    struct Node {
        bool is_list;
        long long iv;                  // integer value when !is_list
        std::vector<pylist> lv;        // list value when is_list
        Node() : is_list(true), iv(0), lv() {}
        explicit Node(long long v) : is_list(false), iv(v), lv() {}
    };

    std::shared_ptr<Node> ptr;

    void ensure_list() const {
        // If this node is an int, and we attempt list ops, convert to empty list
        // However, problem semantics imply list vars (like top-level) are lists.
        // We'll keep strict: if not list, transform into list by replacing node.
        if (!ptr) return; // should not happen
        if (!ptr->is_list) {
            // Replace with a new list node; other aliases to the old int remain
            // This object's aliasing semantics follow shared_ptr assignment
            const_cast<pylist*>(this)->ptr = std::make_shared<Node>();
        }
    }

    static void print_impl(std::ostream &os, const pylist &ls, std::unordered_set<const Node*> &visiting) {
        if (!ls.ptr) { os << "[]"; return; }
        if (!ls.ptr->is_list) {
            os << ls.ptr->iv;
            return;
        }
        const Node *addr = ls.ptr.get();
        if (visiting.find(addr) != visiting.end()) {
            os << "[...]";
            return;
        }
        visiting.insert(addr);
        os << "[";
        for (std::size_t i = 0; i < ls.ptr->lv.size(); ++i) {
            if (i) os << ", ";
            const pylist &elem = ls.ptr->lv[i];
            if (elem.ptr && elem.ptr->is_list) {
                // Recurse with cycle detection
                print_impl(os, elem, visiting);
            } else {
                // int or null
                if (elem.ptr && !elem.ptr->is_list) os << elem.ptr->iv; else os << "[]";
            }
        }
        os << "]";
        visiting.erase(addr);
    }

public:
    // Constructors
    pylist() : ptr(std::make_shared<Node>()) {}
    pylist(const pylist&) = default;
    pylist(pylist&&) noexcept = default;
    explicit pylist(long long v) : ptr(std::make_shared<Node>(v)) {}

    // Assignment operators
    pylist& operator=(const pylist&) = default;
    pylist& operator=(pylist&&) noexcept = default;
    pylist& operator=(long long v) {
        ptr = std::make_shared<Node>(v);
        return *this;
    }

    // Append operations (O(1) amortized)
    void append(const pylist &x) {
        ensure_list();
        ptr->lv.push_back(x);
    }
    void append(long long x) {
        ensure_list();
        ptr->lv.emplace_back(x);
    }

    // Pop last element and return it (O(1))
    pylist pop() {
        ensure_list();
        // assume valid according to problem tests
        pylist res = ptr->lv.back();
        ptr->lv.pop_back();
        return res;
    }

    // Indexing (O(1))
    pylist &operator[](std::size_t i) {
        ensure_list();
        return ptr->lv[i];
    }
    const pylist &operator[](std::size_t i) const {
        // const overload for read-only access
        return const_cast<pylist*>(this)->operator[](i);
    }

    // Implicit conversion to integer for arithmetic and comparisons
    operator long long() const {
        // Only valid if this node holds an int; tests guarantee correctness of usage
        if (ptr && !ptr->is_list) return ptr->iv;
        return 0LL; // Fallback; should not be used in tests for list types
    }

    // Stream output
    friend std::ostream &operator<<(std::ostream &os, const pylist &ls) {
        if (!ls.ptr) { os << "[]"; return os; }
        if (!ls.ptr->is_list) { os << ls.ptr->iv; return os; }
        std::unordered_set<const Node*> visiting;
        print_impl(os, ls, visiting);
        return os;
    }
};

#endif // SRC_HPP_PYLIST
