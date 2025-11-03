#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {
/**
 * @brief a container like std::priority_queue which is a heap internal.
 * **Exception Safety**: The `Compare` operation might throw exceptions for certain data.
 * In such cases, any ongoing operation should be terminated, and the priority queue should be restored to its original state before the operation began.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
	struct Node {
		T val;
		Node *l, *r;
		int npl; // null-path length (leftist heap)
		Node(const T &v) : val(v), l(nullptr), r(nullptr), npl(1) {}
	};

	Node *root = nullptr;
	size_t cnt = 0;
	Compare comp;

	static int get_npl(Node *x) { return x ? x->npl : 0; }

	// Leftist-heap meld with strong exception-safety:
	// No structural modification to any node happens in a frame
	// until all deeper recursive calls return successfully.
	Node *meld(Node *a, Node *b) {
		if (!a) return b;
		if (!b) return a;
		// Decide the root; if comparator throws here, nothing has been modified yet
		if (comp(a->val, b->val)) {
			Node *tmp = a; a = b; b = tmp;
		}
		// Merge b into a->r; if an exception occurs inside, nothing has been changed in this frame
		Node *newRight = meld(a->r, b);
		a->r = newRight;
		// maintain leftist property
		if (get_npl(a->l) < get_npl(a->r)) {
			Node *tmp = a->l; a->l = a->r; a->r = tmp;
		}
		a->npl = get_npl(a->r) + 1; // since npl(left) >= npl(right)
		return a;
	}

	static Node *clone(Node *x) {
		if (!x) return nullptr;
		Node *t = new Node(x->val);
		t->npl = x->npl;
		t->l = clone(x->l);
		t->r = clone(x->r);
		return t;
	}

	static void clear(Node *x) {
		if (!x) return;
		clear(x->l);
		clear(x->r);
		delete x;
	}

public:
	/**
	 * @brief default constructor
	 */
	priority_queue() : root(nullptr), cnt(0), comp(Compare()) {}

	/**
	 * @brief copy constructor
	 * @param other the priority_queue to be copied
	 */
	priority_queue(const priority_queue &other) : root(clone(other.root)), cnt(other.cnt), comp(other.comp) {}

	/**
	 * @brief deconstructor
	 */
	~priority_queue() { clear(root); root = nullptr; cnt = 0; }

	/**
	 * @brief Assignment operator
	 * @param other the priority_queue to be assigned from
	 * @return a reference to this priority_queue after assignment
	 */
	priority_queue &operator=(const priority_queue &other) {
		if (this == &other) return *this;
		Node *newRoot = clone(other.root);
		// Replace only after clone succeeds (strong exception safety)
		clear(root);
		root = newRoot;
		cnt = other.cnt;
		comp = other.comp;
		return *this;
	}

	/**
	 * @brief get the top element of the priority queue.
	 * @return a reference of the top element.
	 * @throws container_is_empty if empty() returns true
	 */
	const T & top() const {
		if (empty()) throw container_is_empty();
		return root->val;
	}

	/**
	 * @brief push new element to the priority queue.
	 * @param e the element to be pushed
	 */
	void push(const T &e) {
		Node *node = new Node(e);
		try {
			root = meld(root, node);
			++cnt;
		} catch (...) {
			// meld failed (likely comparator threw); restore to original state
			delete node;
			throw runtime_error();
		}
	}

	/**
	 * @brief delete the top element from the priority queue.
	 * @throws container_is_empty if empty() returns true
	 */
	void pop() {
		if (empty()) throw container_is_empty();
		Node *old = root;
		try {
			root = meld(root->l, root->r);
			--cnt;
			delete old;
		} catch (...) {
			// restore (unchanged) state and rethrow as runtime_error
			root = old;
			throw runtime_error();
		}
	}

	/**
	 * @brief return the number of elements in the priority queue.
	 * @return the number of elements.
	 */
	size_t size() const { return cnt; }

	/**
	 * @brief check if the container is empty.
	 * @return true if it is empty, false otherwise.
	 */
	bool empty() const { return cnt == 0; }

	/**
	 * @brief merge another priority_queue into this one.
	 * The other priority_queue will be cleared after merging.
	 * The complexity is at most O(logn).
	 * @param other the priority_queue to be merged.
	 */
	void merge(priority_queue &other) {
		if (this == &other || other.empty()) return;
		try {
			root = meld(root, other.root);
			cnt += other.cnt;
			other.root = nullptr;
			other.cnt = 0;
		} catch (...) {
			// If meld fails, both heaps must remain unchanged.
			// Our meld provides strong exception safety, so state is intact here.
			throw runtime_error();
		}
	}
};

}

#endif