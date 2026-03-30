// ============================================================================
// Big O Demo: Binary Trees & AVL
// ============================================================================
// This demo times BST and AVL tree operations at increasing input sizes so you
// can SEE the difference between O(log n) and O(n) growth patterns.
//
// Three scenarios:
//   1. Balanced BST — random data keeps tree roughly balanced → O(log n)
//   2. Degenerate BST — sorted data creates a linked list → O(n)
//   3. AVL tree — sorted data, but rotations maintain balance → O(log n)
//
// Not graded — run it, read the output, and observe the patterns.
// ============================================================================

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <numeric>
#include <vector>
#include <string>
#include <algorithm>
#include <random>

// ── BST Node ────────────────────────────────────────────────────────────────

struct BSTNode {
    int data;
    BSTNode* left;
    BSTNode* right;
    BSTNode(int value) : data{value}, left{nullptr}, right{nullptr} {}
};

// ── Simple BST (no balancing) ───────────────────────────────────────────────

class BST {
public:
    BST() = default;

    ~BST() { destroy(root_); }

    void insert(int value) {
        root_ = insert(root_, value);
    }

    bool search(int value) const {
        return search(root_, value);
    }

private:
    BSTNode* root_ = nullptr;

    BSTNode* insert(BSTNode* node, int value) {
        if (!node) return new BSTNode(value);
        if (value < node->data)
            node->left = insert(node->left, value);
        else if (value > node->data)
            node->right = insert(node->right, value);
        return node;
    }

    bool search(BSTNode* node, int value) const {
        if (!node) return false;
        if (value == node->data) return true;
        if (value < node->data) return search(node->left, value);
        return search(node->right, value);
    }

    void destroy(BSTNode* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
};

// ── AVL Node ────────────────────────────────────────────────────────────────

struct AVLNode {
    int data;
    AVLNode* left;
    AVLNode* right;
    int height;
    AVLNode(int value)
        : data{value}, left{nullptr}, right{nullptr}, height{1} {}
};

// ── AVL Tree (self-balancing BST) ───────────────────────────────────────────
// Implements single and double rotations to maintain the AVL balance property:
// - For every node, |height(left) - height(right)| <= 1
// - This guarantees O(log n) height, so insert/search are always O(log n)

class AVLTree {
public:
    AVLTree() = default;

    ~AVLTree() { destroy(root_); }

    void insert(int value) {
        root_ = insert(root_, value);
    }

    bool search(int value) const {
        return search(root_, value);
    }

private:
    AVLNode* root_ = nullptr;

    // ── Height helpers ──────────────────────────────────────────────────

    int height(AVLNode* node) const {
        return node ? node->height : 0;
    }

    int balance_factor(AVLNode* node) const {
        return node ? height(node->left) - height(node->right) : 0;
    }

    void update_height(AVLNode* node) {
        node->height = 1 + std::max(height(node->left), height(node->right));
    }

    // ── Single rotations ───────────────────────────────────────────────

    // Right rotate (for left-heavy imbalance)
    //
    //       node              left_child
    //       /  \                /    \
    //  left_child  C   →      A     node
    //    /  \                        /  \
    //   A    B                      B    C
    //
    AVLNode* rotate_right(AVLNode* node) {
        AVLNode* left_child = node->left;
        node->left = left_child->right;
        left_child->right = node;
        update_height(node);
        update_height(left_child);
        return left_child;
    }

    // Left rotate (for right-heavy imbalance)
    //
    //   node                right_child
    //   /  \                  /    \
    //  A  right_child  →   node    C
    //       /  \           /  \
    //      B    C         A    B
    //
    AVLNode* rotate_left(AVLNode* node) {
        AVLNode* right_child = node->right;
        node->right = right_child->left;
        right_child->left = node;
        update_height(node);
        update_height(right_child);
        return right_child;
    }

    // ── Rebalance after insert ──────────────────────────────────────────
    // - balance_factor > 1:  left-heavy
    //     - left child is left-heavy or balanced: single right rotation
    //     - left child is right-heavy: left-right double rotation
    // - balance_factor < -1: right-heavy
    //     - right child is right-heavy or balanced: single left rotation
    //     - right child is left-heavy: right-left double rotation

    AVLNode* rebalance(AVLNode* node) {
        int bf = balance_factor(node);

        // Left-heavy
        if (bf > 1) {
            if (balance_factor(node->left) < 0) {
                // Left-Right case: rotate left child left, then node right
                node->left = rotate_left(node->left);
            }
            return rotate_right(node);
        }

        // Right-heavy
        if (bf < -1) {
            if (balance_factor(node->right) > 0) {
                // Right-Left case: rotate right child right, then node left
                node->right = rotate_right(node->right);
            }
            return rotate_left(node);
        }

        return node;
    }

    // ── Insert ──────────────────────────────────────────────────────────

    AVLNode* insert(AVLNode* node, int value) {
        if (!node) return new AVLNode(value);

        if (value < node->data)
            node->left = insert(node->left, value);
        else if (value > node->data)
            node->right = insert(node->right, value);
        else
            return node;  // no duplicates

        update_height(node);
        return rebalance(node);
    }

    // ── Search ──────────────────────────────────────────────────────────

    bool search(AVLNode* node, int value) const {
        if (!node) return false;
        if (value == node->data) return true;
        if (value < node->data) return search(node->left, value);
        return search(node->right, value);
    }

    // ── Cleanup ─────────────────────────────────────────────────────────

    void destroy(AVLNode* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }
};

// ── Benchmark Utilities ─────────────────────────────────────────────────────

using Clock = std::chrono::high_resolution_clock;

// Returns elapsed time in microseconds
template <typename Func>
double time_us(Func&& func) {
    auto start = Clock::now();
    func();
    auto end = Clock::now();
    return std::chrono::duration<double, std::micro>(end - start).count();
}

void print_header(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
    std::cout << std::setw(12) << "n"
              << std::setw(15) << "time (us)"
              << std::setw(15) << "growth" << "\n";
    std::cout << std::string(42, '-') << "\n";
}

void print_row(int n, double time_us, double prev_time_us) {
    std::cout << std::setw(12) << n
              << std::setw(15) << std::fixed << std::setprecision(1) << time_us;
    if (prev_time_us > 0) {
        std::cout << std::setw(12) << std::setprecision(1) << (time_us / prev_time_us) << "x";
    }
    std::cout << "\n";
}

// ── Result collection ───────────────────────────────────────────────────────

struct BenchResult {
    std::string operation;   // "insert" or "search"
    std::string structure;   // "BST (random)", "BST (sorted)", "AVL (sorted)"
    std::string complexity;  // "O(log n)" or "O(n)"
    int n;
    double time_us;          // per-operation time
};

std::vector<BenchResult> all_results;

// ── Main ────────────────────────────────────────────────────────────────────

int main() {
    std::cout << "============================================================\n";
    std::cout << "  Big O Demo: Binary Trees & AVL\n";
    std::cout << "============================================================\n";
    std::cout << "\nThis demo times BST and AVL tree operations at increasing\n";
    std::cout << "sizes. Watch the 'growth' column:\n";
    std::cout << "  - O(log n): growth stays small (the \"halving\" pattern)\n";
    std::cout << "  - O(n):     growth matches the size multiplier (linear)\n";

    std::vector<int> sizes = {5000, 10000, 20000, 50000};
    // Smaller sizes for degenerate BST (deep recursion causes stack overflow at ~50K)
    std::vector<int> degen_sizes = {1000, 2000, 5000, 10000};

    // Fixed seed for reproducible results
    std::mt19937 rng(42);

    // ── Scenario 1: Balanced BST (random data) ─────────────────────────
    // Random insertion keeps the tree roughly balanced.
    // Total time for n operations: O(n log n) — growth ~2x when n doubles.

    print_header("BST insert (random data) -- O(log n) average");
    double prev = 0;
    for (int n : sizes) {
        // Generate shuffled data
        std::vector<int> data(n);
        std::iota(data.begin(), data.end(), 0);
        std::shuffle(data.begin(), data.end(), rng);

        BST tree;
        double t = time_us([&]() {
            for (int val : data) tree.insert(val);
        });
        print_row(n, t, prev);
        all_results.push_back({"insert", "BST (random)", "O(n log n)", n, t});
        prev = t;
    }

    print_header("BST search (random data) -- O(log n) average");
    prev = 0;
    for (int n : sizes) {
        std::vector<int> data(n);
        std::iota(data.begin(), data.end(), 0);
        std::shuffle(data.begin(), data.end(), rng);

        BST tree;
        for (int val : data) tree.insert(val);

        // Search for every element (volatile prevents optimization)
        volatile int found = 0;
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) found += tree.search(i);
        });
        print_row(n, t, prev);
        all_results.push_back({"search", "BST (random)", "O(n log n)", n, t});
        prev = t;
    }

    // ── Scenario 2: Degenerate BST (sorted data) ───────────────────────
    // Inserting sorted data into a BST creates a linked list.
    // Insert and search degrade to O(n).
    //
    //   0
    //    \
    //     1
    //      \
    //       2
    //        \
    //        ...  ← every node is a right child = linked list!

    print_header("BST insert (sorted data) -- O(n)  [DEGENERATE]");
    prev = 0;
    for (int n : degen_sizes) {
        BST tree;
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) tree.insert(i);
        });
        print_row(n, t, prev);
        all_results.push_back({"insert", "BST (sorted)", "O(n^2)", n, t});
        prev = t;
    }

    print_header("BST search (sorted data) -- O(n)  [DEGENERATE]");
    prev = 0;
    for (int n : degen_sizes) {
        BST tree;
        for (int i = 0; i < n; i++) tree.insert(i);

        // Search for every element (volatile prevents optimization)
        volatile int found = 0;
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) found += tree.search(i);
        });
        print_row(n, t, prev);
        all_results.push_back({"search", "BST (sorted)", "O(n^2)", n, t});
        prev = t;
    }

    // ── Scenario 3: AVL tree (sorted data) ──────────────────────────────
    // Even with sorted data, the AVL tree stays balanced via rotations.
    // Insert and search remain O(log n).

    print_header("AVL insert (sorted data) -- O(log n)  [ROTATIONS FIX IT]");
    prev = 0;
    for (int n : sizes) {
        AVLTree tree;
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) tree.insert(i);
        });
        print_row(n, t, prev);
        all_results.push_back({"insert", "AVL (sorted)", "O(n log n)", n, t});
        prev = t;
    }

    print_header("AVL search (sorted data) -- O(log n)  [ROTATIONS FIX IT]");
    prev = 0;
    for (int n : sizes) {
        AVLTree tree;
        for (int i = 0; i < n; i++) tree.insert(i);

        // Search for every element (volatile prevents optimization)
        volatile int found = 0;
        double t = time_us([&]() {
            for (int i = 0; i < n; i++) found += tree.search(i);
        });
        print_row(n, t, prev);
        all_results.push_back({"search", "AVL (sorted)", "O(n log n)", n, t});
        prev = t;
    }

    // ── Summary ─────────────────────────────────────────────────────────

    std::cout << "\n============================================================\n";
    std::cout << "  Summary: Binary Tree Big O\n";
    std::cout << "============================================================\n";
    std::cout << "\n";
    std::cout << "  Scenario                 | Insert   | Search   | Why\n";
    std::cout << "  -------------------------|----------|----------|--------------------\n";
    std::cout << "  BST (random data)        | O(log n) | O(log n) | Roughly balanced\n";
    std::cout << "  BST (sorted data)        | O(n)     | O(n)     | Linked list!\n";
    std::cout << "  AVL (sorted data)        | O(log n) | O(log n) | Rotations rebalance\n";
    std::cout << "\n";
    std::cout << "  Key takeaway: O(log n) is the \"halving\" pattern.\n";
    std::cout << "  Each comparison eliminates HALF the remaining nodes.\n";
    std::cout << "\n";
    std::cout << "  A degenerate BST loses this property -- it becomes a linked\n";
    std::cout << "  list where every comparison only eliminates ONE node.\n";
    std::cout << "\n";
    std::cout << "  AVL trees guarantee O(log n) by rotating after each insert\n";
    std::cout << "  to keep the tree balanced. The extra cost of rotations is\n";
    std::cout << "  tiny compared to the cost of an unbalanced tree.\n";

    // ── Write CSV and generate charts ─────────────────────────────────
    std::string repo_dir = REPO_DIR;
    std::string csv_path = repo_dir + "/results.csv";
    std::ofstream csv(csv_path);
    csv << "operation,structure,complexity,n,time_us\n";
    for (const auto& r : all_results) {
        csv << r.operation << "," << r.structure << "," << r.complexity
            << "," << r.n << "," << std::fixed << std::setprecision(4)
            << r.time_us << "\n";
    }
    csv.close();
    std::cout << "\n  Results written to CSV -- generating charts...\n";

    std::string cmd = "py -3 \"" + repo_dir + "/graph.py\" --graph-only";
    std::system(cmd.c_str());

    return 0;
}
