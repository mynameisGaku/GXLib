#pragma once
/// @file Container.h
/// @brief GXLib Container Library アンブレラヘッダー
///
/// EASTL風の高速コンテナライブラリ。
/// デバッグビルド高速化、キャッシュ効率向上、カスタムアロケータ統合、ゼロ例外設計。

// ============================================================================
// Phase 1: インフラ基盤
// ============================================================================
#include "Container/Allocator.h"
#include "Container/TypeTraits.h"
#include "Container/CompressedPair.h"
#include "Container/Iterator.h"
#include "Container/Hash.h"

// ============================================================================
// Phase 2: コアコンテナ
// ============================================================================
#include "Container/Array.h"
#include "Container/Span.h"
#include "Container/Vector.h"
#include "Container/String.h"
#include "Container/WString.h"
#include "Container/StringView.h"

// ============================================================================
// Phase 3: ハッシュコンテナ
// ============================================================================
#include "Container/HashTable.h"
#include "Container/HashMap.h"
#include "Container/HashSet.h"
#include "Container/HashMultiMap.h"
#include "Container/HashMultiSet.h"

// ============================================================================
// Phase 4: 赤黒木コンテナ
// ============================================================================
#include "Container/RBTree.h"
#include "Container/Map.h"
#include "Container/Set.h"
#include "Container/MultiMap.h"
#include "Container/MultiSet.h"

// ============================================================================
// Phase 5: リンクドリスト
// ============================================================================
#include "Container/List.h"
#include "Container/ForwardList.h"
#include "Container/IntrusiveList.h"
#include "Container/IntrusiveForwardList.h"

// ============================================================================
// Phase 6: アダプタ & ユーティリティ
// ============================================================================
#include "Container/Deque.h"
#include "Container/Stack.h"
#include "Container/Queue.h"
#include "Container/PriorityQueue.h"
#include "Container/RingBuffer.h"
#include "Container/Bitset.h"
#include "Container/BitVector.h"

// ============================================================================
// Phase 7: Fixed-Size バリアント
// ============================================================================
#include "Container/FixedVector.h"
#include "Container/FixedList.h"
#include "Container/FixedForwardList.h"
#include "Container/FixedString.h"
#include "Container/FixedHashMap.h"
#include "Container/FixedHashSet.h"
#include "Container/FixedHashMultiMap.h"
#include "Container/FixedHashMultiSet.h"
#include "Container/FixedMap.h"
#include "Container/FixedSet.h"
#include "Container/FixedMultiMap.h"
#include "Container/FixedMultiSet.h"

// ============================================================================
// Phase 8: フラットソートコンテナ
// ============================================================================
#include "Container/VectorMap.h"
#include "Container/VectorSet.h"
#include "Container/VectorMultiMap.h"
#include "Container/VectorMultiSet.h"

// ============================================================================
// Phase 9: 侵入型ハッシュコンテナ
// ============================================================================
#include "Container/IntrusiveHashSet.h"
#include "Container/IntrusiveHashMap.h"
#include "Container/IntrusiveHashMultiSet.h"
#include "Container/IntrusiveHashMultiMap.h"

// ============================================================================
// Phase 10: アルゴリズム
// ============================================================================
#include "Container/Algorithm.h"
