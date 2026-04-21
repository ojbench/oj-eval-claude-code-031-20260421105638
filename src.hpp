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
    bool self_proxy = false;                    // represents a self-reference to parent list
    std::weak_ptr<Node> self_parent;

    // Helper to check if this represents a list (normal or self-proxy)
    bool is_list_like() const {
        if (self_proxy) return true;
        return ptr && ptr->is_list;
    }
    // Resolve list node pointer (nullptr if not a list)
    std::shared_ptr<Node> get_list_ptr() const {
        if (self_proxy) return self_parent.lock();
        if (ptr && ptr->is_list) return ptr;
        return nullptr;
    }

    void ensure_list() {
        if (self_proxy) {
            // self-proxy always refers to an existing list; nothing to change
            return;
        }
        if (!ptr) return; // should not happen
        if (!ptr->is_list) {
            ptr = std::make_shared<Node>();
        }
    }

    static void print_impl(std::ostream &os, const pylist &ls, std::unordered_set<const Node*> &visiting) {
        // Print int
        if (!ls.self_proxy && ls.ptr && !ls.ptr->is_list) {
            os << ls.ptr->iv;
            return;
        }
        // Determine node for list
        std::shared_ptr<Node> lsp = ls.get_list_ptr();
        if (!lsp) { os << "[]"; return; }
        const Node *addr = lsp.get();
        if (visiting.find(addr) != visiting.end()) {
            os << "[...]";
            return;
        }
        visiting.insert(addr);
        os << "[";
        for (std::size_t i = 0; i < lsp->lv.size(); ++i) {
            if (i) os << ", ";
            const pylist &elem = lsp->lv[i];
            if (elem.is_list_like()) {
                print_impl(os, elem, visiting);
            } else {
                // integer element
                if (elem.ptr) os << elem.ptr->iv; else os << "[]";
            }
        }
        os << "]";
        visiting.erase(addr);
    }

    // Factory for self-proxy element referencing parent list
    static pylist make_self_proxy(const std::shared_ptr<Node>& parent) {
        pylist p;
        p.self_proxy = true;
        p.self_parent = parent;
        p.ptr.reset();
        return p;
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
        self_proxy = false;
        self_parent.reset();
        ptr = std::make_shared<Node>(v);
        return *this;
    }

    // Append operations (O(1) amortized)
    void append(const pylist &x) {
        std::shared_ptr<Node> lsp = get_list_ptr();
        if (!lsp) {
            // convert int to list then append
            ensure_list();
            lsp = get_list_ptr();
        }
        // Detect self-reference to avoid shared_ptr cycles
        if (x.is_list_like()) {
            std::shared_ptr<Node> child = x.get_list_ptr();
            if (child && child.get() == lsp.get()) {
                lsp->lv.push_back(make_self_proxy(lsp));
                return;
            }
        }
        lsp->lv.push_back(x);
    }
    void append(long long x) {
        std::shared_ptr<Node> lsp = get_list_ptr();
        if (!lsp) { ensure_list(); lsp = get_list_ptr(); }
        lsp->lv.emplace_back(x);
    }

    // Pop last element and return it (O(1))
    pylist pop() {
        std::shared_ptr<Node> lsp = get_list_ptr();
        // assume valid according to problem tests
        pylist res = lsp->lv.back();
        lsp->lv.pop_back();
        return res;
    }

    // Indexing (O(1))
    pylist &operator[](std::size_t i) {
        std::shared_ptr<Node> lsp = get_list_ptr();
        return lsp->lv[i];
    }
    const pylist &operator[](std::size_t i) const {
        return const_cast<pylist*>(this)->operator[](i);
    }

    // Implicit conversion to integer for arithmetic and comparisons
    operator long long() const {
        if (!self_proxy && ptr && !ptr->is_list) return ptr->iv;
        return 0LL;
    }

    // Stream output
    friend std::ostream &operator<<(std::ostream &os, const pylist &ls) {
        if (!ls.self_proxy && ls.ptr && !ls.ptr->is_list) { os << ls.ptr->iv; return os; }
        std::unordered_set<const Node*> visiting;
        print_impl(os, ls, visiting);
        return os;
    }
};

#endif // SRC_HPP_PYLIST
