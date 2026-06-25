//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// UNSUPPORTED: GCC-ALWAYS_INLINE-FIXME

#include <deque>

#include "../../from_range_sequence_containers.h"
#include "test_macros.h"

// template<container-compatible-range<T> R>
//   deque(from_range_t, R&& rg, const Allocator& = Allocator()); // C++23

int main(int, char**) {
  for_all_iterators_and_allocators<int>([]<class Iter, class Sent, class Alloc>() {
    test_sequence_container<std::deque, int, Iter, Sent, Alloc>([]([[maybe_unused]] const auto& c) {
      LIBCPP_ASSERT(c.__invariants());
    });
  });
  test_sequence_container_move_only<std::deque>();

  static_assert(test_constraints<std::deque, int, double>());

#if TEST_STD_VER >= 26
  // these test approximately_sized_range support
  { // correctness tests
    int in[8] = {1, 2, 3, 4, 5, 6, 7, 8};

    // correct hint
    ApproxSizedRange range1(in, in + 8, /*hint=*/8);
    std::deque<int> d1{std::from_range, range1};
    assert(std::ranges::equal(d1, in));
    LIBCPP_ASSERT(d1.__invariants());

    // underestimated hint
    ApproxSizedRange range2(in, in + 8, /*hint=*/2);
    std::deque<int> d2{std::from_range, range2};
    assert(std::ranges::equal(d2, in));
    LIBCPP_ASSERT(d2.__invariants());

    // overestimated hint
    ApproxSizedRange range3(in, in + 8, /*hint=*/100);
    std::deque<int> d3{std::from_range, range3};
    assert(std::ranges::equal(d3, in));
    LIBCPP_ASSERT(d3.__invariants());

    // zero hint
    ApproxSizedRange range4(in, in + 8, /*hint=*/0);
    std::deque<int> d4{std::from_range, range4};
    assert(std::ranges::equal(d4, in));
    LIBCPP_ASSERT(d4.__invariants());

    // overestimated hint, actually 0
    ApproxSizedRange range5(in, in, /*hint=*/64);
    std::deque<int> d5{std::from_range, range5};
    assert(d5.empty());
    LIBCPP_ASSERT(d5.__invariants());
  }

  { // test that extra blocks are allocated for an overestimated reserve hint
    int in[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    test_allocator_statistics big, small;

    ApproxSizedRange range_small(in, in + 8, /*hint=*/8);
    std::deque<int, test_allocator<int>> deque_small(std::from_range, range_small, test_allocator<int>(&small));
    assert(std::ranges::equal(deque_small, in));
    LIBCPP_ASSERT(deque_small.__invariants());

    ApproxSizedRange range_big(in, in + 8, /*hint=*/2048);
    std::deque<int, test_allocator<int>> deque_big(std::from_range, range_big, test_allocator<int>(&big));
    assert(std::ranges::equal(deque_big, in));
    LIBCPP_ASSERT(deque_big.__invariants());

    assert(big.alloc_count > small.alloc_count);
  }
#endif

  // TODO(varconst): `deque`'s constructors currently aren't exception-safe.
  // See https://llvm.org/PR62056.
  //test_exception_safety_throwing_copy<std::deque>();
  //test_exception_safety_throwing_allocator<std::deque, int>();

  return 0;
}
