# Big O Demo: Binary Trees & AVL

**Not graded** — this is a runnable demonstration, not an assignment.

## What This Shows

Benchmarks BST and AVL tree operations at increasing input sizes to demonstrate why balanced trees matter:

| Scenario | Insert | Search | Why |
|----------|--------|--------|-----|
| Balanced BST (random data) | O(log n) avg | O(log n) avg | Random insertion keeps tree roughly balanced |
| Degenerate BST (sorted data) | O(n) | O(n) | Sorted insertion creates a linked list |
| AVL tree (sorted data) | O(log n) | O(log n) | Rotations maintain balance guarantee |

**Key lesson:** O(log n) is the "halving" pattern. Every comparison eliminates half the remaining nodes. A degenerate BST loses this property and degrades to O(n). AVL rotations restore it.

## How to Run

```bash
cmake -B build
cmake --build build
./build/big-o-demo-binary-trees
```

The output shows timing tables for each scenario. Watch the growth column:
- **O(log n):** growth stays small even as n increases dramatically
- **O(n):** growth matches the size multiplier (linear)
