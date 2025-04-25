// // eaf.H  – one‑shot header, no templates, no STL heap use
// #ifndef EAF_H
// #define EAF_H
// struct EAF
// {
//     UINT32 maxLines;       // == #lines in the cache
//     UINT32 counter;        // how many inserts so far
//     UINT32 bits;           // size of bit‑vector (α * maxLines)
//     std::vector<UINT8> bf; // simple bit array

//     // * α = 8 ⇒ ≈ 1.6 % storage overhead for a 16‑way 16 MB cache.*
//     explicit EAF(UINT32 _lines, UINT32 alpha = 8)
//         : maxLines(_lines), counter(0), bits(alpha * _lines),
//           bf((bits + 7) >> 3, 0) {}

//     /*------------ helpers --------------*/
//     inline UINT32 h1(ADDRINT x) const { return (x ^ (x >> 7)) % bits; }
//     inline UINT32 h2(ADDRINT x) const { return (x ^ (x << 11)) % bits; }
//     // b >> 3	integer‑divide the bit index b by 8	converts a bit number into a byte index (byte 0 holds bits 0‑7, byte 1 holds bits 8‑15, …).
//     // b & 7	b mod 8	position of the bit within that byte (0 = LSB, 7 = MSB).
//     inline void setbit(UINT32 b) { bf[b >> 3] |= (1 << (b & 7)); }
//     inline bool testbit(UINT32 b) const { return bf[b >> 3] & (1 << (b & 7)); }

//     /*------------- API -----------------*/
//     bool test(ADDRINT tag) const
//     {
//         return testbit(h1(tag)) && testbit(h2(tag));
//     }
//     void insert(ADDRINT tag)
//     {
//         setbit(h1(tag));
//         setbit(h2(tag));
//         // if (++counter == maxLines)
//         //     clear();
//         // 24th April 2025: halving the counter to avoid FPs
//         // Clear when occupancy reaches ½ of cache lines instead of 100 %
//         // (empirically this keeps Bloom-filter FP-rate ≈ 4 – 6 %)
//         if (++counter == (maxLines >> 1)) //  maxLines / 2
//             clear();
//     }
//     void clear()
//     {
//         std::fill(bf.begin(), bf.end(), 0);
//         counter = 0;
//     }
// };
// #endif




#ifndef EAF_H
#define EAF_H

#include <vector>
#include <array>
#include <algorithm>          // std::fill
#include <random>


#define RAND_SEED 29346
static constexpr int HASH_FUNCS = 3;   // h1, h2, h3

struct EAF
{
    /* -------- member variables ----------------------------------------- */
    UINT32 maxLines;       // == #lines in the cache
    UINT32 counter;        // how many inserts so far
    UINT32 bits;           // size of bit-vector (alpha * maxLines)
    std::vector<UINT8> bf; // simple bit array holding the filter

    UINT32 logSize;        // ⌈log2(bits)⌉
    std::array<std::vector<ADDRINT>, HASH_FUNCS> masks;
                           // masks[i][j] = row-j of matrix for hash i

    /* -------- constructor ---------------------------------------------- */
    explicit EAF(UINT32 _lines, UINT32 alpha = 8)
        : maxLines(_lines),
          counter(0),
          bits(alpha * _lines),
          bf((bits + 7) >> 3, 0)
    {
        /* ----- generate 3-universal (H3) hash matrices ------------------ */
        logSize = 32U - __builtin_clz(bits - 1);          // ceil(log2(bits))

        std::mt19937_64 rng(RAND_SEED);
        std::uniform_int_distribution<ADDRINT> dist(1);   // non-zero rows

        for (auto &vec : masks) {
            vec.resize(logSize);
            for (auto &row : vec)
                row = dist(rng);
        }
    }

    /* -------- parity helper (dot-product mod-2) ------------------------ */
    static inline int parity64(ADDRINT v)
    {
        return __builtin_parityll(v);   // GCC/Clang; MSVC: _mm_popcnt_u64(v)&1
    }

    /* -------- H3 hash functions --------------------------------------- */
    inline UINT32 h(int i, ADDRINT x) const
    {
        UINT32 idx = 0;
        for (UINT32 b = 0; b < logSize; ++b)
            if (parity64(masks[i][b] & x))
                idx |= (1u << b);
        // idx is already < 2^logSize ≤ bits, but mod just in case:
        return (idx >= bits) ? (idx % bits) : idx;
    }

    inline UINT32 h1(ADDRINT x) const { return h(0, x); }
    inline UINT32 h2(ADDRINT x) const { return h(1, x); }
    inline UINT32 h3(ADDRINT x) const { return h(2, x); }

    /* -------- bit helpers --------------------------------------------- */
    inline void setbit(UINT32 b)       { bf[b >> 3] |=  (1u << (b & 7)); }
    inline bool testbit(UINT32 b) const{ return bf[b >> 3] &  (1u << (b & 7)); }

    /* -------- public API ---------------------------------------------- */
    bool test(ADDRINT tag) const
    {
        return testbit(h1(tag)) && testbit(h2(tag)) && testbit(h3(tag));
    }

    void insert(ADDRINT tag)
    {
        setbit(h1(tag));
        setbit(h2(tag));
        setbit(h3(tag));

        if (++counter == (maxLines >> 1))   // clear at ½ occupancy
            clear();
    }

    void clear()
    {
        std::fill(bf.begin(), bf.end(), 0);
        counter = 0;
    }
};

#endif // EAF_H

