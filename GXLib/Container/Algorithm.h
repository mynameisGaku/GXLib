#pragma once
/// @file Algorithm.h
/// @brief ソートアルゴリズム・探索・ヒープ操作・ユーティリティ

#include "pch_common.h"
#include "Container/Allocator.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <utility>

namespace gx::container {

// ============================================================================
// Min / Max / Clamp
// ============================================================================

/// @brief 二値の最小値
template <typename T>
__forceinline constexpr const T& Min(const T& a, const T& b) noexcept
{
    return (a < b) ? a : b;
}

/// @brief 二値の最大値
template <typename T>
__forceinline constexpr const T& Max(const T& a, const T& b) noexcept
{
    return (a > b) ? a : b;
}

/// @brief 値を [lo, hi] にクランプ
template <typename T>
__forceinline constexpr const T& Clamp(const T& v, const T& lo, const T& hi) noexcept
{
    return Min(Max(v, lo), hi);
}

// ============================================================================
// Median
// ============================================================================

/// @brief 三値の中央値
template <typename T>
__forceinline constexpr const T& Median(const T& a, const T& b, const T& c) noexcept
{
    if (a < b)
    {
        if (b < c) return b;       // a < b < c
        if (a < c) return c;       // a < c <= b
        return a;                   // c <= a < b
    }
    else
    {
        if (a < c) return a;       // b <= a < c
        if (b < c) return c;       // b < c <= a
        return b;                   // c <= b <= a
    }
}

/// @brief カスタム比較子付き三値の中央値
template <typename T, typename Compare>
__forceinline constexpr const T& Median(const T& a, const T& b, const T& c, Compare comp) noexcept
{
    if (comp(a, b))
    {
        if (comp(b, c)) return b;
        if (comp(a, c)) return c;
        return a;
    }
    else
    {
        if (comp(a, c)) return a;
        if (comp(b, c)) return c;
        return b;
    }
}

// ============================================================================
// Swap
// ============================================================================

/// @brief 二要素のスワップ
template <typename T>
__forceinline void Swap(T& a, T& b) noexcept
{
    T tmp = std::move(a);
    a = std::move(b);
    b = std::move(tmp);
}

// ============================================================================
// Insertion Sort
// ============================================================================

/// @brief 挿入ソート — 小範囲に最適 (O(n^2))
template <typename Iter, typename Compare>
void InsertionSort(Iter first, Iter last, Compare comp)
{
    if (first == last) return;
    for (Iter i = first + 1; i != last; ++i)
    {
        auto key = std::move(*i);
        Iter j = i;
        while (j != first && comp(key, *(j - 1)))
        {
            *j = std::move(*(j - 1));
            --j;
        }
        *j = std::move(key);
    }
}

/// @brief 挿入ソート (デフォルト比較: operator<)
template <typename Iter>
void InsertionSort(Iter first, Iter last)
{
    InsertionSort(first, last, [](const auto& a, const auto& b) { return a < b; });
}

// ============================================================================
// Shell Sort
// ============================================================================

/// @brief シェルソート — 中規模配列向け (O(n^1.25) 平均)
template <typename Iter, typename Compare>
void ShellSort(Iter first, Iter last, Compare comp)
{
    auto n = last - first;
    if (n <= 1) return;

    // Ciura gap sequence (extended)
    static constexpr ptrdiff_t gaps[] = {
        701, 301, 132, 57, 23, 10, 4, 1
    };

    for (auto gap : gaps)
    {
        if (gap >= n) continue;
        for (auto i = first + gap; i < last; ++i)
        {
            auto temp = std::move(*i);
            auto j = i;
            while (j >= first + gap && comp(temp, *(j - gap)))
            {
                *j = std::move(*(j - gap));
                j -= gap;
            }
            *j = std::move(temp);
        }
    }
}

/// @brief シェルソート (デフォルト比較)
template <typename Iter>
void ShellSort(Iter first, Iter last)
{
    ShellSort(first, last, [](const auto& a, const auto& b) { return a < b; });
}

// ============================================================================
// Quick Sort (median-of-3, insertion sort fallback)
// ============================================================================

namespace detail {

/// @brief QuickSort 内部実装
template <typename Iter, typename Compare>
void QuickSortImpl(Iter first, Iter last, Compare& comp, int depthLimit)
{
    while (last - first > 16)
    {
        if (depthLimit == 0)
        {
            // Fall back to shell sort to avoid worst-case O(n^2)
            ShellSort(first, last, comp);
            return;
        }
        --depthLimit;

        // Median-of-3 pivot selection
        Iter mid = first + (last - first) / 2;
        Iter pivotIter;

        if (comp(*first, *mid))
        {
            if (comp(*mid, *(last - 1)))
                pivotIter = mid;
            else if (comp(*first, *(last - 1)))
                pivotIter = last - 1;
            else
                pivotIter = first;
        }
        else
        {
            if (comp(*first, *(last - 1)))
                pivotIter = first;
            else if (comp(*mid, *(last - 1)))
                pivotIter = last - 1;
            else
                pivotIter = mid;
        }

        // Move pivot to last-1
        Swap(*pivotIter, *(last - 1));
        auto& pivot = *(last - 1);

        // Partition (Hoare-style)
        Iter i = first;
        Iter j = last - 2;

        for (;;)
        {
            while (i <= j && comp(*i, pivot)) ++i;
            while (i <= j && comp(pivot, *j)) --j;
            if (i >= j) break;
            Swap(*i, *j);
            ++i;
            --j;
        }

        // Restore pivot
        Swap(*i, *(last - 1));

        // Recurse on smaller partition, iterate on larger
        if (i - first < last - (i + 1))
        {
            QuickSortImpl(first, i, comp, depthLimit);
            first = i + 1;
        }
        else
        {
            QuickSortImpl(i + 1, last, comp, depthLimit);
            last = i;
        }
    }
    // Small range: insertion sort
    InsertionSort(first, last, comp);
}

/// @brief 再帰深さ上限を計算 (2 * log2(n))
inline int DepthLimit(ptrdiff_t n) noexcept
{
    int depth = 0;
    while (n > 1) { n >>= 1; ++depth; }
    return depth * 2;
}

} // namespace detail

/// @brief クイックソート — median-of-3ピボット、小範囲は挿入ソートに退避
template <typename Iter, typename Compare>
void QuickSort(Iter first, Iter last, Compare comp)
{
    auto n = last - first;
    if (n <= 1) return;
    detail::QuickSortImpl(first, last, comp, detail::DepthLimit(n));
}

/// @brief クイックソート (デフォルト比較)
template <typename Iter>
void QuickSort(Iter first, Iter last)
{
    QuickSort(first, last, [](const auto& a, const auto& b) { return a < b; });
}

// ============================================================================
// Merge Sort (with buffer)
// ============================================================================

namespace detail {

template <typename Iter, typename Compare>
void MergeSortBufferImpl(Iter first, Iter last, typename std::iterator_traits<Iter>::value_type* buffer, Compare& comp)
{
    auto n = last - first;
    if (n <= 16)
    {
        InsertionSort(first, last, comp);
        return;
    }

    Iter mid = first + n / 2;
    MergeSortBufferImpl(first, mid, buffer, comp);
    MergeSortBufferImpl(mid, last, buffer, comp);

    // If already sorted, skip merge
    if (!comp(*mid, *(mid - 1)))
        return;

    // Merge into buffer
    using ValueType = typename std::iterator_traits<Iter>::value_type;
    Iter left = first;
    Iter right = mid;
    ValueType* out = buffer;

    while (left != mid && right != last)
    {
        if (comp(*right, *left))
            *out++ = std::move(*right++);
        else
            *out++ = std::move(*left++);
    }
    while (left != mid)
        *out++ = std::move(*left++);
    while (right != last)
        *out++ = std::move(*right++);

    // Copy back
    out = buffer;
    for (Iter it = first; it != last; ++it)
        *it = std::move(*out++);
}

} // namespace detail

/// @brief マージソート — 一時バッファ使用、安定ソート
template <typename Iter, typename Compare>
void MergeSortBuffer(Iter first, Iter last, Compare comp)
{
    auto n = last - first;
    if (n <= 1) return;

    using ValueType = typename std::iterator_traits<Iter>::value_type;
    Allocator alloc;
    auto* buffer = static_cast<ValueType*>(alloc.allocate(
        static_cast<size_t>(n) * sizeof(ValueType), alignof(ValueType), 0));

    detail::MergeSortBufferImpl(first, last, buffer, comp);

    alloc.deallocate(buffer, static_cast<size_t>(n) * sizeof(ValueType));
}

/// @brief マージソート (デフォルト比較)
template <typename Iter>
void MergeSortBuffer(Iter first, Iter last)
{
    MergeSortBuffer(first, last, [](const auto& a, const auto& b) { return a < b; });
}

// ============================================================================
// Radix Sort (LSD, 8-bit passes, unsigned integer types)
// ============================================================================

/// @brief LSD基数ソート — 符号なし整数型専用、8ビットパス
template <typename T>
void RadixSort(T* first, T* last)
{
    static_assert(std::is_unsigned_v<T>, "RadixSort requires unsigned integer types");

    auto n = last - first;
    if (n <= 1) return;

    constexpr int kBitsPerPass = 8;
    constexpr size_t kBucketCount = 1 << kBitsPerPass;
    constexpr T kMask = static_cast<T>(kBucketCount - 1);
    constexpr int kPasses = (sizeof(T) * 8 + kBitsPerPass - 1) / kBitsPerPass;

    Allocator alloc;
    T* buffer = static_cast<T*>(alloc.allocate(
        static_cast<size_t>(n) * sizeof(T), alignof(T), 0));

    T* src = first;
    T* dst = buffer;

    for (int pass = 0; pass < kPasses; ++pass)
    {
        int shift = pass * kBitsPerPass;

        // Count
        size_t counts[kBucketCount] = {};
        for (ptrdiff_t i = 0; i < n; ++i)
        {
            size_t bucket = static_cast<size_t>((src[i] >> shift) & kMask);
            ++counts[bucket];
        }

        // Prefix sum
        size_t offsets[kBucketCount];
        offsets[0] = 0;
        for (size_t i = 1; i < kBucketCount; ++i)
            offsets[i] = offsets[i - 1] + counts[i - 1];

        // Scatter
        for (ptrdiff_t i = 0; i < n; ++i)
        {
            size_t bucket = static_cast<size_t>((src[i] >> shift) & kMask);
            dst[offsets[bucket]++] = src[i];
        }

        // Swap src/dst
        T* tmp = src;
        src = dst;
        dst = tmp;
    }

    // If odd number of passes, result is in buffer — copy back
    if constexpr (kPasses % 2 != 0)
    {
        // src is buffer, need to copy to first
        std::memcpy(first, buffer, static_cast<size_t>(n) * sizeof(T));
    }

    alloc.deallocate(buffer, static_cast<size_t>(n) * sizeof(T));
}

/// @brief 符号付き整数用基数ソート — ビット反転で符号なしに変換して実行
template <typename T>
void RadixSortSigned(T* first, T* last)
{
    static_assert(std::is_signed_v<T> && std::is_integral_v<T>,
                  "RadixSortSigned requires signed integer types");

    using U = std::make_unsigned_t<T>;
    constexpr U signBit = static_cast<U>(1) << (sizeof(U) * 8 - 1);

    auto n = last - first;

    // Flip sign bit to make unsigned ordering match signed ordering
    U* uFirst = reinterpret_cast<U*>(first);
    for (ptrdiff_t i = 0; i < n; ++i)
        uFirst[i] ^= signBit;

    RadixSort(uFirst, uFirst + n);

    // Flip back
    for (ptrdiff_t i = 0; i < n; ++i)
        uFirst[i] ^= signBit;
}

// ============================================================================
// Heap extensions (EASTL-style)
// ============================================================================

namespace detail {

/// @brief ヒープ上方修復 (sift up)
template <typename Iter, typename Compare>
void HeapSiftUp(Iter first, size_t index, Compare& comp)
{
    while (index > 0)
    {
        size_t parent = (index - 1) / 2;
        if (comp(*(first + parent), *(first + index)))
        {
            Swap(*(first + parent), *(first + index));
            index = parent;
        }
        else
        {
            break;
        }
    }
}

/// @brief ヒープ下方修復 (sift down)
template <typename Iter, typename Compare>
void HeapSiftDown(Iter first, size_t heapSize, size_t index, Compare& comp)
{
    for (;;)
    {
        size_t left = 2 * index + 1;
        size_t right = 2 * index + 2;
        size_t largest = index;

        if (left < heapSize && comp(*(first + largest), *(first + left)))
            largest = left;
        if (right < heapSize && comp(*(first + largest), *(first + right)))
            largest = right;

        if (largest != index)
        {
            Swap(*(first + index), *(first + largest));
            index = largest;
        }
        else
        {
            break;
        }
    }
}

} // namespace detail

/// @brief ヒープ要素変更後の修復 — index位置の要素を変更した後にヒープ性質を回復
template <typename Iter, typename Compare>
void ChangeHeap(Iter first, size_t heapSize, size_t index, Compare comp)
{
    assert(index < heapSize);

    // Try sifting up first, then down
    size_t parent = (index > 0) ? (index - 1) / 2 : 0;
    if (index > 0 && comp(*(first + parent), *(first + index)))
    {
        detail::HeapSiftUp(first, index, comp);
    }
    else
    {
        detail::HeapSiftDown(first, heapSize, index, comp);
    }
}

/// @brief ヒープ任意位置の要素を削除
template <typename Iter, typename Compare>
void RemoveHeap(Iter first, size_t heapSize, size_t index, Compare comp)
{
    assert(heapSize > 0 && index < heapSize);

    size_t last = heapSize - 1;
    if (index != last)
    {
        Swap(*(first + index), *(first + last));
        // Restore heap on the swapped-in element
        ChangeHeap(first, last, index, comp);
    }
    // Element is now at position 'last', caller is responsible for removing it
}

/// @brief ヒープ構築 (Floyd's sift-down)
template <typename Iter, typename Compare>
void MakeHeap(Iter first, size_t heapSize, Compare comp)
{
    if (heapSize <= 1) return;
    for (size_t i = heapSize / 2; i > 0; --i)
        detail::HeapSiftDown(first, heapSize, i - 1, comp);
}

/// @brief ヒープに要素をプッシュ (最後に追加済みの要素をsift up)
template <typename Iter, typename Compare>
void PushHeap(Iter first, size_t heapSize, Compare comp)
{
    if (heapSize <= 1) return;
    detail::HeapSiftUp(first, heapSize - 1, comp);
}

/// @brief ヒープから先頭を取り出し (last-1に移動)
template <typename Iter, typename Compare>
void PopHeap(Iter first, size_t heapSize, Compare comp)
{
    assert(heapSize > 0);
    if (heapSize <= 1) return;
    Swap(*first, *(first + heapSize - 1));
    detail::HeapSiftDown(first, heapSize - 1, 0, comp);
}

/// @brief ヒープソート
template <typename Iter, typename Compare>
void HeapSort(Iter first, Iter last, Compare comp)
{
    size_t n = static_cast<size_t>(last - first);
    if (n <= 1) return;
    MakeHeap(first, n, comp);
    for (size_t i = n; i > 1; --i)
    {
        PopHeap(first, i, comp);
    }
}

/// @brief ヒープソート (デフォルト比較)
template <typename Iter>
void HeapSort(Iter first, Iter last)
{
    HeapSort(first, last, [](const auto& a, const auto& b) { return a < b; });
}

// ============================================================================
// Binary Search
// ============================================================================

/// @brief 二分探索 — 見つかった要素のイテレータを返す (見つからない場合は last)
template <typename Iter, typename T, typename Compare>
Iter BinarySearchI(Iter first, Iter last, const T& value, Compare comp)
{
    Iter orig_last = last;
    Iter lo = first;
    Iter hi = last;

    while (lo < hi)
    {
        Iter mid = lo + (hi - lo) / 2;
        if (comp(*mid, value))
            lo = mid + 1;
        else
            hi = mid;
    }

    // lo is lower_bound position — check if it points to value
    if (lo != orig_last && !comp(value, *lo))
        return lo;
    return orig_last;
}

/// @brief 二分探索 (デフォルト比較)
template <typename Iter, typename T>
Iter BinarySearchI(Iter first, Iter last, const T& value)
{
    return BinarySearchI(first, last, value, [](const auto& a, const auto& b) { return a < b; });
}

/// @brief lower_bound — value 以上の最初のイテレータ
template <typename Iter, typename T, typename Compare>
Iter LowerBound(Iter first, Iter last, const T& value, Compare comp)
{
    while (first < last)
    {
        Iter mid = first + (last - first) / 2;
        if (comp(*mid, value))
            first = mid + 1;
        else
            last = mid;
    }
    return first;
}

/// @brief lower_bound (デフォルト比較)
template <typename Iter, typename T>
Iter LowerBound(Iter first, Iter last, const T& value)
{
    return LowerBound(first, last, value, [](const auto& a, const auto& b) { return a < b; });
}

/// @brief upper_bound — value より大きい最初のイテレータ
template <typename Iter, typename T, typename Compare>
Iter UpperBound(Iter first, Iter last, const T& value, Compare comp)
{
    while (first < last)
    {
        Iter mid = first + (last - first) / 2;
        if (comp(value, *mid))
            last = mid;
        else
            first = mid + 1;
    }
    return first;
}

/// @brief upper_bound (デフォルト比較)
template <typename Iter, typename T>
Iter UpperBound(Iter first, Iter last, const T& value)
{
    return UpperBound(first, last, value, [](const auto& a, const auto& b) { return a < b; });
}

// ============================================================================
// Partitioning
// ============================================================================

/// @brief パーティション — pred(elem) が true の要素を前に集める
/// @return パーティション境界 (false 要素の先頭)
template <typename Iter, typename Predicate>
Iter Partition(Iter first, Iter last, Predicate pred)
{
    while (first != last)
    {
        while (first != last && pred(*first))
            ++first;
        if (first == last) break;
        --last;
        while (first != last && !pred(*last))
            --last;
        if (first == last) break;
        Swap(*first, *last);
        ++first;
    }
    return first;
}

// ============================================================================
// Nth Element (introselect)
// ============================================================================

/// @brief nth_element — n番目に小さい要素を正しい位置に配置
template <typename Iter, typename Compare>
void NthElement(Iter first, Iter nth, Iter last, Compare comp)
{
    if (first == last || nth == last) return;

    while (last - first > 16)
    {
        // Median-of-3 pivot
        Iter mid = first + (last - first) / 2;
        const auto& pivotVal = Median(*first, *mid, *(last - 1), comp);
        // Find pivot position
        Iter pivotIter;
        if (!comp(pivotVal, *first) && !comp(*first, pivotVal))
            pivotIter = first;
        else if (!comp(pivotVal, *mid) && !comp(*mid, pivotVal))
            pivotIter = mid;
        else
            pivotIter = last - 1;

        Swap(*pivotIter, *(last - 1));

        Iter i = first;
        Iter j = last - 2;
        for (;;)
        {
            while (i <= j && comp(*i, *(last - 1))) ++i;
            while (i <= j && comp(*(last - 1), *j)) --j;
            if (i >= j) break;
            Swap(*i, *j);
            ++i;
            --j;
        }
        Swap(*i, *(last - 1));

        if (i == nth) return;
        if (nth < i)
            last = i;
        else
            first = i + 1;
    }
    InsertionSort(first, last, comp);
}

/// @brief nth_element (デフォルト比較)
template <typename Iter>
void NthElement(Iter first, Iter nth, Iter last)
{
    NthElement(first, nth, last, [](const auto& a, const auto& b) { return a < b; });
}

// ============================================================================
// IsSorted / IsSortedUntil
// ============================================================================

/// @brief 範囲がソート済みかチェック
template <typename Iter, typename Compare>
bool IsSorted(Iter first, Iter last, Compare comp)
{
    if (first == last) return true;
    Iter next = first;
    ++next;
    while (next != last)
    {
        if (comp(*next, *first)) return false;
        first = next;
        ++next;
    }
    return true;
}

/// @brief IsSorted (デフォルト比較)
template <typename Iter>
bool IsSorted(Iter first, Iter last)
{
    return IsSorted(first, last, [](const auto& a, const auto& b) { return a < b; });
}

/// @brief ソート順が崩れる最初の位置を返す
template <typename Iter, typename Compare>
Iter IsSortedUntil(Iter first, Iter last, Compare comp)
{
    if (first == last) return last;
    Iter next = first;
    ++next;
    while (next != last)
    {
        if (comp(*next, *first)) return next;
        first = next;
        ++next;
    }
    return last;
}

// ============================================================================
// Rotate
// ============================================================================

/// @brief 範囲を左回転 — [first, middle) と [middle, last) を入れ替え
template <typename Iter>
Iter Rotate(Iter first, Iter middle, Iter last)
{
    if (first == middle) return last;
    if (middle == last) return first;

    Iter next = middle;
    Iter ret = last - (middle - first);

    while (first != next)
    {
        Swap(*first, *next);
        ++first;
        ++next;
        if (next == last)
            next = middle;
        else if (first == middle)
            middle = next;
    }
    return ret;
}

// ============================================================================
// Copy / Move / Fill / Generate
// ============================================================================

/// @brief 範囲コピー
template <typename InputIter, typename OutputIter>
OutputIter Copy(InputIter first, InputIter last, OutputIter dest)
{
    while (first != last)
    {
        *dest = *first;
        ++first;
        ++dest;
    }
    return dest;
}

/// @brief 範囲ムーブ
template <typename InputIter, typename OutputIter>
OutputIter Move(InputIter first, InputIter last, OutputIter dest)
{
    while (first != last)
    {
        *dest = std::move(*first);
        ++first;
        ++dest;
    }
    return dest;
}

/// @brief 範囲を値で埋める
template <typename Iter, typename T>
void Fill(Iter first, Iter last, const T& value)
{
    while (first != last)
    {
        *first = value;
        ++first;
    }
}

/// @brief ジェネレータで範囲を埋める
template <typename Iter, typename Generator>
void Generate(Iter first, Iter last, Generator gen)
{
    while (first != last)
    {
        *first = gen();
        ++first;
    }
}

// ============================================================================
// Find / Count / ForEach
// ============================================================================

/// @brief 線形探索 — 値に一致する最初のイテレータ
template <typename Iter, typename T>
Iter Find(Iter first, Iter last, const T& value)
{
    while (first != last)
    {
        if (*first == value) return first;
        ++first;
    }
    return last;
}

/// @brief 述語に一致する最初のイテレータ
template <typename Iter, typename Predicate>
Iter FindIf(Iter first, Iter last, Predicate pred)
{
    while (first != last)
    {
        if (pred(*first)) return first;
        ++first;
    }
    return last;
}

/// @brief 値の出現回数
template <typename Iter, typename T>
size_t Count(Iter first, Iter last, const T& value)
{
    size_t count = 0;
    while (first != last)
    {
        if (*first == value) ++count;
        ++first;
    }
    return count;
}

/// @brief 述語に一致する出現回数
template <typename Iter, typename Predicate>
size_t CountIf(Iter first, Iter last, Predicate pred)
{
    size_t count = 0;
    while (first != last)
    {
        if (pred(*first)) ++count;
        ++first;
    }
    return count;
}

/// @brief 範囲の各要素に関数を適用
template <typename Iter, typename Func>
Func ForEach(Iter first, Iter last, Func func)
{
    while (first != last)
    {
        func(*first);
        ++first;
    }
    return func;
}

// ============================================================================
// Accumulate / Transform
// ============================================================================

/// @brief 畳み込み (左)
template <typename Iter, typename T>
T Accumulate(Iter first, Iter last, T init)
{
    while (first != last)
    {
        init = init + *first;
        ++first;
    }
    return init;
}

/// @brief 畳み込み (カスタム二項演算)
template <typename Iter, typename T, typename BinaryOp>
T Accumulate(Iter first, Iter last, T init, BinaryOp op)
{
    while (first != last)
    {
        init = op(init, *first);
        ++first;
    }
    return init;
}

/// @brief 変換 — 各要素に関数を適用して出力先に書き込み
template <typename InputIter, typename OutputIter, typename UnaryOp>
OutputIter Transform(InputIter first, InputIter last, OutputIter dest, UnaryOp op)
{
    while (first != last)
    {
        *dest = op(*first);
        ++first;
        ++dest;
    }
    return dest;
}

// ============================================================================
// Remove / Unique
// ============================================================================

/// @brief 値に一致する要素を論理削除 (後方に移動)
/// @return 有効範囲の新しいend
template <typename Iter, typename T>
Iter Remove(Iter first, Iter last, const T& value)
{
    Iter result = first;
    while (first != last)
    {
        if (!(*first == value))
        {
            if (result != first)
                *result = std::move(*first);
            ++result;
        }
        ++first;
    }
    return result;
}

/// @brief 述語に一致する要素を論理削除
template <typename Iter, typename Predicate>
Iter RemoveIf(Iter first, Iter last, Predicate pred)
{
    Iter result = first;
    while (first != last)
    {
        if (!pred(*first))
        {
            if (result != first)
                *result = std::move(*first);
            ++result;
        }
        ++first;
    }
    return result;
}

/// @brief ソート済み範囲の連続重複を論理削除
template <typename Iter>
Iter Unique(Iter first, Iter last)
{
    if (first == last) return last;
    Iter result = first;
    ++first;
    while (first != last)
    {
        if (!(*result == *first))
        {
            ++result;
            if (result != first)
                *result = std::move(*first);
        }
        ++first;
    }
    return ++result;
}

/// @brief カスタム等価比較付き Unique
template <typename Iter, typename BinaryPred>
Iter Unique(Iter first, Iter last, BinaryPred pred)
{
    if (first == last) return last;
    Iter result = first;
    ++first;
    while (first != last)
    {
        if (!pred(*result, *first))
        {
            ++result;
            if (result != first)
                *result = std::move(*first);
        }
        ++first;
    }
    return ++result;
}

} // namespace gx::container
