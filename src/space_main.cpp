// =============================================================================
// Big O Demo: Binary Trees & AVL -- Space Complexity
// =============================================================================
// Measures two space metrics at increasing input sizes:
//
//   1. Heap bytes -- total memory allocated for tree nodes.
//      All three scenarios are O(n) because you always allocate exactly n nodes.
//      The constant factor differs: BST nodes are 24 bytes, AVL nodes 32 bytes
//      (the extra 8 bytes store the cached height used for rebalancing).
//
//   2. Tree height -- the length of the longest root-to-leaf path.
//      Height == maximum recursion depth for any insert, search, or remove call.
//        - BST (random):  height ≈ 1.4 log₂(n)  →  O(log n) stack frames
//        - BST (sorted):  height = n - 1         →  O(n) stack frames  ← danger
//        - AVL (sorted):  height ≤ 1.44 log₂(n) →  O(log n) stack frames
//
//   The stack-depth story is the key lesson:
//     At n=10,000, a degenerate BST has height 9,999.
//     Each recursive search call adds one stack frame (~64 bytes on x64).
//     That's 9,999 × 64 bytes ≈ 625 KB of stack just for ONE search call.
//     Most systems allow only 1--8 MB of stack, so deep recursion can crash.
//
// Not graded -- run it, read the output, and observe the patterns.
// =============================================================================

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>
#include <cmath>

// -----------------------------------------------------------------------------
// Heap allocation tracking (same technique as module 6 space demo)
// -----------------------------------------------------------------------------
// Overrides global operator new/delete so every heap allocation is counted.
// reset_peak() snapshots the current baseline before building a tree, then
// peak_above_baseline() returns only the memory the tree allocated.

static size_t g_heap_current = 0;
static size_t g_heap_peak    = 0;
static size_t g_baseline     = 0;

void mark_baseline() { g_baseline = g_heap_current; }
size_t peak_above_baseline() { return g_heap_peak - g_baseline; }

void* operator new(size_t size) {
    void* raw = std::malloc(size + sizeof(size_t));
    if (!raw) throw std::bad_alloc();
    std::memcpy(raw, &size, sizeof(size_t));
    g_heap_current += size;
    if (g_heap_current > g_heap_peak) g_heap_peak = g_heap_current;
    return static_cast<char*>(raw) + sizeof(size_t);
}

void operator delete(void* ptr) noexcept {
    if (!ptr) return;
    void* raw = static_cast<char*>(ptr) - sizeof(size_t);
    size_t size;
    std::memcpy(&size, raw, sizeof(size_t));
    g_heap_current -= size;
    std::free(raw);
}

void operator delete(void* ptr, size_t) noexcept { ::operator delete(ptr); }
void* operator new[](size_t size)                 { return ::operator new(size); }
void operator delete[](void* ptr) noexcept        { ::operator delete(ptr); }
void operator delete[](void* ptr, size_t) noexcept{ ::operator delete(ptr); }

// -----------------------------------------------------------------------------
// BST
// -----------------------------------------------------------------------------

struct BSTNode {
    int      data;
    BSTNode* left;
    BSTNode* right;
    BSTNode(int val) : data{val}, left{nullptr}, right{nullptr} {}
};

class BST {
public:
    BST() = default;
    ~BST() { destroy_(root_); }

    void insert(int val) { root_ = insert_(root_, val); }

    int height() const { return height_(root_); }

private:
    BSTNode* root_ = nullptr;

    BSTNode* insert_(BSTNode* node, int val) {
        if (!node) return new BSTNode(val);
        if (val < node->data) node->left  = insert_(node->left,  val);
        else if (val > node->data) node->right = insert_(node->right, val);
        return node;
    }

    int height_(BSTNode* node) const {
        if (!node) return -1;
        return 1 + std::max(height_(node->left), height_(node->right));
    }

    void destroy_(BSTNode* node) {
        if (!node) return;
        destroy_(node->left);
        destroy_(node->right);
        delete node;
    }
};

// -----------------------------------------------------------------------------
// AVL Tree
// -----------------------------------------------------------------------------

struct AVLNode {
    int      data;
    AVLNode* left;
    AVLNode* right;
    int      height;   // ← extra 8 bytes vs BSTNode (4 data + 4 padding)
    AVLNode(int val) : data{val}, left{nullptr}, right{nullptr}, height{1} {}
};

class AVLTree {
public:
    AVLTree() = default;
    ~AVLTree() { destroy_(root_); }

    void insert(int val) { root_ = insert_(root_, val); }

    int height() const { return root_ ? root_->height - 1 : -1; }

private:
    AVLNode* root_ = nullptr;

    int node_height(AVLNode* n) const { return n ? n->height : 0; }

    int balance_factor(AVLNode* n) const {
        return n ? node_height(n->left) - node_height(n->right) : 0;
    }

    void update_height(AVLNode* n) {
        n->height = 1 + std::max(node_height(n->left), node_height(n->right));
    }

    AVLNode* rotate_right(AVLNode* z) {
        AVLNode* y = z->left;
        z->left = y->right;
        y->right = z;
        update_height(z);
        update_height(y);
        return y;
    }

    AVLNode* rotate_left(AVLNode* z) {
        AVLNode* y = z->right;
        z->right = y->left;
        y->left = z;
        update_height(z);
        update_height(y);
        return y;
    }

    AVLNode* rebalance(AVLNode* node) {
        int bf = balance_factor(node);
        if (bf > 1) {
            if (balance_factor(node->left) < 0)
                node->left = rotate_left(node->left);
            return rotate_right(node);
        }
        if (bf < -1) {
            if (balance_factor(node->right) > 0)
                node->right = rotate_right(node->right);
            return rotate_left(node);
        }
        return node;
    }

    AVLNode* insert_(AVLNode* node, int val) {
        if (!node) return new AVLNode(val);
        if (val < node->data)      node->left  = insert_(node->left,  val);
        else if (val > node->data) node->right = insert_(node->right, val);
        else return node;
        update_height(node);
        return rebalance(node);
    }

    void destroy_(AVLNode* node) {
        if (!node) return;
        destroy_(node->left);
        destroy_(node->right);
        delete node;
    }
};

// -----------------------------------------------------------------------------
// Formatting helpers
// -----------------------------------------------------------------------------

std::string fmt_bytes(size_t b) {
    if (b >= 1'000'000) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f MB", b / 1'000'000.0);
        return buf;
    }
    if (b >= 1'000) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.1f KB", b / 1'000.0);
        return buf;
    }
    return std::to_string(b) + " B";
}

void print_header(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
    std::cout << std::setw(10) << "n"
              << std::setw(16) << "heap (nodes)"
              << std::setw(14) << "bytes/node"
              << std::setw(10) << "height"
              << std::setw(16) << "log2(n)"
              << std::setw(16) << "height/log2(n)" << "\n";
    std::cout << std::string(82, '-') << "\n";
}

void print_row(int n, size_t heap_bytes, int height) {
    double log2n = std::log2(n);
    std::cout << std::setw(10) << n
              << std::setw(12) << fmt_bytes(heap_bytes)
              << std::setw(14) << std::fixed << std::setprecision(1)
              << static_cast<double>(heap_bytes) / n
              << std::setw(10) << height
              << std::setw(16) << std::setprecision(1) << log2n
              << std::setw(16) << std::setprecision(2)
              << static_cast<double>(height) / log2n << "\n";
}

// -----------------------------------------------------------------------------
// Result record
// -----------------------------------------------------------------------------

struct SpaceResult {
    std::string scenario;
    std::string complexity;
    int n;
    size_t heap_bytes;
    int height;
};

std::vector<SpaceResult> all_results;

// -----------------------------------------------------------------------------
// Main
// -----------------------------------------------------------------------------

int main() {
    std::cout << "=============================================================\n";
    std::cout << "  Big O Demo: Binary Trees -- Space Complexity\n";
    std::cout << "=============================================================\n";
    std::cout << "\nTwo metrics per scenario:\n";
    std::cout << "  heap (nodes)    -- bytes allocated for all tree nodes\n";
    std::cout << "  height          -- longest root-to-leaf path (= max recursion depth)\n";
    std::cout << "\nKey insight: all three scenarios allocate the SAME number of\n";
    std::cout << "nodes (n), so heap usage is always O(n).  Height is where\n";
    std::cout << "they diverge: balanced trees stay at O(log n), the degenerate\n";
    std::cout << "BST grows to O(n) -- meaning O(n) stack frames per operation.\n";

    std::vector<int> sizes       = {1000, 2000, 5000, 10000, 20000};
    std::vector<int> degen_sizes = {1000, 2000, 5000, 10000};  // cap: deep recursion

    std::mt19937 rng(42);

    // ── Scenario 1: BST, random insertion ─────────────────────────────
    print_header("BST (random data) -- O(n) heap, O(log n) height");
    for (int n : sizes) {
        std::vector<int> data(n);
        std::iota(data.begin(), data.end(), 0);
        std::shuffle(data.begin(), data.end(), rng);

        g_heap_peak = g_heap_current;
        mark_baseline();

        BST tree;
        for (int val : data) tree.insert(val);

        size_t heap = peak_above_baseline();
        int    h    = tree.height();
        print_row(n, heap, h);
        all_results.push_back({"BST (random)", "O(n log n)", n, heap, h});
    }

    // ── Scenario 2: BST, sorted insertion (degenerate) ────────────────
    print_header("BST (sorted data) -- O(n) heap, O(n) height  [DEGENERATE]");
    for (int n : degen_sizes) {
        g_heap_peak = g_heap_current;
        mark_baseline();

        BST tree;
        for (int i = 0; i < n; i++) tree.insert(i);

        size_t heap = peak_above_baseline();
        int    h    = tree.height();
        print_row(n, heap, h);
        all_results.push_back({"BST (sorted)", "O(n^2)", n, heap, h});
    }

    // ── Scenario 3: AVL, sorted insertion ─────────────────────────────
    print_header("AVL (sorted data) -- O(n) heap*, O(log n) height  [ROTATIONS FIX IT]");
    std::cout << "  (* AVL nodes are 32 bytes vs 24 bytes for BST nodes -- height field adds 8 bytes)\n";
    for (int n : sizes) {
        g_heap_peak = g_heap_current;
        mark_baseline();

        AVLTree tree;
        for (int i = 0; i < n; i++) tree.insert(i);

        size_t heap = peak_above_baseline();
        int    h    = tree.height();
        print_row(n, heap, h);
        all_results.push_back({"AVL (sorted)", "O(n log n)", n, heap, h});
    }

    // ── Summary ───────────────────────────────────────────────────────
    std::cout << "\n=============================================================\n";
    std::cout << "  Summary: Space at n=10,000\n";
    std::cout << "=============================================================\n\n";

    std::cout << "  Scenario             | Heap (nodes)  | Height  | Stack risk\n";
    std::cout << "  ---------------------|---------------|---------|------------------\n";

    auto print_summary_row = [&](const std::string& label) {
        for (const auto& r : all_results) {
            if (r.scenario == label && r.n == 10000) {
                int stack_kb = r.height * 64 / 1024;  // 64 bytes/frame estimate
                std::cout << "  " << std::setw(20) << std::left << label
                          << " | " << std::setw(13) << fmt_bytes(r.heap_bytes)
                          << " | " << std::setw(7) << r.height
                          << " | ~" << stack_kb << " KB stack depth\n";
            }
        }
    };

    print_summary_row("BST (random)");
    print_summary_row("BST (sorted)");
    print_summary_row("AVL (sorted)");

    std::cout << "\n  Key takeaway: all three use the same O(n) heap.\n";
    std::cout << "  The degenerate BST at n=10,000 needs ~625 KB of stack per search call.\n";
    std::cout << "  AVL's O(log n) height keeps stack depth under 1 KB -- safe and fast.\n";

    // ── Write CSV ──────────────────────────────────────────────────────
    std::string repo_dir = REPO_DIR;
    std::string csv_path = repo_dir + "/results_space.csv";
    std::ofstream csv(csv_path);
    csv << "scenario,complexity,n,heap_bytes,height\n";
    for (const auto& r : all_results) {
        csv << r.scenario << "," << r.complexity << "," << r.n << ","
            << r.heap_bytes << "," << r.height << "\n";
    }
    csv.close();
    std::cout << "\n  Results written to CSV -- generating charts...\n";

    std::string cmd = "py -3 \"" + repo_dir + "/graph_space.py\" --graph-only";
    std::system(cmd.c_str());

    return 0;
}
