# ggalloc

A custom dynamic memory allocator written in C++ — built from scratch without `malloc` or `free`. Interfaces directly with the OS kernel via POSIX system calls and includes a **Mark-and-Sweep Garbage Collector**.

---

## What Does It Do?

`ggalloc` replaces the standard heap allocator. It requests raw memory from the OS, manages it via a linked free-list, splits and coalesces blocks to fight fragmentation, and reclaims unreachable memory automatically through a GC cycle.

---

## Architecture

```
Heap Layout:
[META|PAYLOAD] -> [META|PAYLOAD] -> .... -> [META|PAYLOAD] -> NULL
   Block 1            Block 2                  Block N
```

Every allocated block has a **hidden metadata header** placed directly before the user payload:

```cpp
struct meta {
    size_t size;       // Payload size
    int free;          // 1 = free, 0 = in use
    int magic;         // 0xDEADBEEF — corruption sentinel
    int is_reachable;  // GC mark bit
    struct meta* next; // Next block in list
};
```

**Key macros:**

```cpp
#define PAYLOAD(block)  ((void*)((char*)(block) + SIZEOFMETA))  // metadata → user pointer
#define META(block)     ((struct meta*)((char*)(block) - SIZEOFMETA))  // user pointer → metadata
#define ALIGN(size)     ((size + 15) & ~15)  // 16-byte alignment (ARM64/x86_64)
```

---

## Core Components

### `ggalloc(size_t size)` — Allocator

1. Aligns requested size to 16 bytes.
2. If heap is empty → calls `space_request()` to bootstrap.
3. Otherwise → runs `free_block()` (First-Fit search).
4. If a large enough free block is found → splits it via `block_split()` if there's room for a new header + minimum payload.
5. If no free block found → requests more memory from OS.
6. Returns `PAYLOAD(block)` — the user-facing pointer.

### `ggfree(void* ptr)` — Manual Free

1. Steps backward via `META(ptr)` to find the hidden header.
2. Validates `magic == 0xDEADBEEF` — ignores invalid/double-free attempts.
3. Marks block as `free = 1`, `is_reachable = 0`.
4. Calls `coalesce()` to merge adjacent free blocks.

### `coalesce()` — Defragmentation

Traverses the full heap. When two physically adjacent blocks are both free, merges them into one larger block. Runs after every `ggfree()` and every GC sweep.

### `mark_phase(void** stack_b, void** stack_t)` — GC Mark

Scans the program stack from bottom to top. For each word-sized value on the stack, checks if it falls within heap bounds (`valid_heap()`). If it does, steps backward to the metadata header, validates the magic number, and marks `is_reachable = 1`.

### `sweep_phase()` — GC Sweep

Iterates every block in the heap. Any `free == 0` block with `is_reachable == 0` is reclaimed (`free = 1`). Reachable blocks have their mark reset to `0` for the next GC cycle. Calls `coalesce()` after each reclaimed block.

### `space_request(struct meta* ptr, size_t size)` — OS Interface

Uses `sbrk(0)` to get the current program break (= start of new block), then `sbrk(total)` to push the heap boundary up by `size + sizeof(meta)`. Links the new block to the end of the list via `ptr->next`.

---

## File Structure

```
ggalloc/
  ├── LICENSE
  ├── allocator.h       # Metadata struct, alignment macros, PAYLOAD/META macros
  └── allocator.cpp     # All allocator + GC logic; main() stub
```

---

## Build & Run

Requires a C++17 compiler and a POSIX-compatible OS (Linux or macOS).

```bash
# Compile the allocator
g++ -std=c++17 -o allocator ggalloc/allocator.cpp

# Run
./allocator
```

> **macOS ARM64 note:** 16-byte alignment is enforced by default, matching the M1/M2 ABI requirement.

---

## Current Status

| Step | Feature                    | Status |
| ---- | -------------------------- | ------ |
| 1    | Block metadata struct      | ✅ Done |
| 2    | Alignment macros           | ✅ Done |
| 3    | First-Fit search algorithm | ✅ Done |
| 4    | OS memory request (`sbrk`) | ✅ Done |
| 5    | Block splitting            | ✅ Done |
| 6    | Core allocator (`ggalloc`) | ✅ Done |
| 7    | GC Mark Phase              | ✅ Done |
| 8    | GC Sweep Phase             | ✅ Done |
| 9    | Manual free (`ggfree`)     | ✅ Done |
| 10   | Coalescing (defrag)        | ✅ Done |

#### GC - Garbage Collector

---

## Concepts Demonstrated

- POSIX memory management (using `sbrk`)
- Pointer arithmetic and unsafe casting in C++
- Linked-list heap management
- 16-byte memory alignment via bitwise operations
- First-Fit allocation strategy
- Block splitting and coalescing
- Mark-and-Sweep garbage collection
- Magic number corruption detection (`0xDEADBEEF`)
- Stack scanning for GC root set

---

## Real-World Context

Used in
- **Game engines**
- **Embedded systems**
- **High-frequency trading** 
- **Language runtimes**
