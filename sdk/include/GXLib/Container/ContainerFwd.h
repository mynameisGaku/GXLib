#pragma once
/// @file ContainerFwd.h
/// @brief コンテナ型の gx:: 名前空間エイリアス
///
/// pch_common.h から include され、全モジュールで gx::Vector, gx::String 等が利用可能になる。

#include "Container/Container.h"

namespace gx {

// Core containers
using container::Vector;
using container::String;
using container::WString;
using container::Array;
using container::Span;

// Hash containers
using container::HashMap;
using container::HashSet;
using container::HashMultiMap;
using container::HashMultiSet;

// Red-Black tree containers
using container::Map;
using container::Set;
using container::MultiMap;
using container::MultiSet;

// Linked lists
using container::List;
using container::ForwardList;

// Adapters
using container::Deque;
using container::Stack;
using container::Queue;
using container::PriorityQueue;

// Flat containers
using container::VectorMap;
using container::VectorSet;
using container::VectorMultiMap;
using container::VectorMultiSet;

// Utilities
using container::RingBuffer;
using container::Bitset;
using container::BitVector;
using container::StringView;

// Fixed-size variants
using container::FixedVector;
using container::FixedString;
using container::FixedList;
using container::FixedForwardList;
using container::FixedHashMap;
using container::FixedHashSet;
using container::FixedMap;
using container::FixedSet;

// Hash function
using container::Hash;

} // namespace gx
