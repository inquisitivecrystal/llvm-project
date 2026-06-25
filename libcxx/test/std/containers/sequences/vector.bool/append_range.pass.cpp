//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// ADDITIONAL_COMPILE_FLAGS(has-fconstexpr-steps): -fconstexpr-steps=2000000

// template<container-compatible-range<bool> R>
//   constexpr void append_range(R&& rg); // C++23

#include <vector>

#include "../insert_range_sequence_containers.h"
#include "test_macros.h"

// Tested cases:
// - different kinds of insertions (appending an {empty/one-element/mid-sized/long range} into an
//   {empty/one-element/full} container);
// - an exception is thrown when allocating new elements.
constexpr bool test() {
  static_assert(test_constraints_append_range<std::vector, bool, char>());

  for_all_iterators_and_allocators<bool, const int*>([]<class Iter, class Sent, class Alloc>() {
    test_sequence_append_range<std::vector<bool, Alloc>, Iter, Sent>([]([[maybe_unused]] auto&& c) {
      LIBCPP_ASSERT(c.__invariants());
      // `is_contiguous_container_asan_correct` doesn't work on `vector<bool>`.
    });
  });

  {   // Vector may or may not need to reallocate because of the insertion -- make sure to test both cases.
    { // Ensure reallocation happens.
      constexpr int N     = 255;
      bool in[N]          = {};
      std::vector<bool> v = {0, 0, 0, 1, 1, 0, 0, 0};
      auto initial        = v;
      assert(v.capacity() < v.size() + std::ranges::size(in));

      v.append_range(in);
      // Because `in` is very large (it has to be to exceed the large capacity that `vector<bool>` allocates), it is
      // impractical to have the expected value as a literal.
      assert(v.size() == initial.size() + N);
      assert(std::ranges::equal(v.begin(), v.begin() + initial.size(), initial.begin(), initial.end()));
      assert(std::ranges::equal(v.begin() + initial.size(), v.end(), std::ranges::begin(in), std::ranges::end(in)));
    }

    { // Ensure no reallocation happens.
      bool in[]           = {1, 1, 1, 1, 0, 0, 1, 1, 1, 1};
      std::vector<bool> v = {0, 0, 0, 1, 1, 0, 0, 0};
      v.reserve(v.size() + std::ranges::size(in));
      assert(v.capacity() >= v.size() + std::ranges::size(in));

      v.append_range(in);
      assert(std::ranges::equal(v, std::vector<bool>{0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1}));
    }
  }

#if TEST_STD_VER >= 26
  { // test appending an approximately sized range
    constexpr int N = 255;
    bool in[N]      = {};
    for (int i = 0; i < N; ++i)
      in[i] = i % 3 == 0;

    // range3 is the case that actually checks that the reserve_hint is used; other
    // tests just check capacity and correct copying
    std::vector<bool> v1(std::begin(in), std::end(in));
    v1.shrink_to_fit();
    std::vector<bool> v2 = v1;
    std::vector<bool> v3 = v1;
    std::vector<bool> v4 = v1;
    std::vector<bool> v5 = v1;

    ApproxSizedRange range1(in, in + 8, /*hint=*/8);
    assert(v1.capacity() == 256);
    v1.append_range(range1);
    assert(v1.capacity() >= 263);
    assert(std::ranges::equal(v1.begin(), v1.begin() + N, std::begin(in), std::end(in)));
    assert(std::ranges::equal(v1.begin() + N, v1.end(), range1.begin(), range1.end()));

    ApproxSizedRange range2(in, in + 8, /*hint=*/2);
    assert(v2.capacity() == 256);
    v2.append_range(range2);
    assert(v2.capacity() >= 263);
    assert(std::ranges::equal(v2.begin(), v2.begin() + N, std::begin(in), std::end(in)));
    assert(std::ranges::equal(v2.begin() + N, v2.end(), range2.begin(), range2.end()));

    ApproxSizedRange range3(in, in + 8, /*hint=*/500);
    assert(v3.capacity() == 256);
    v3.append_range(range3);
    assert(v3.capacity() >= N + 500);
    assert(std::ranges::equal(v3.begin(), v3.begin() + N, std::begin(in), std::end(in)));
    assert(std::ranges::equal(v3.begin() + N, v3.end(), range3.begin(), range3.end()));

    ApproxSizedRange range4(in, in + 8, /*hint=*/0);
    assert(v4.capacity() == 256);
    v4.append_range(range4);
    assert(v4.capacity() >= 263);
    assert(std::ranges::equal(v4.begin(), v4.begin() + N, std::begin(in), std::end(in)));
    assert(std::ranges::equal(v4.begin() + N, v4.end(), range4.begin(), range4.end()));

    ApproxSizedRange range5(in, in, /*hint=*/0);
    assert(v5.capacity() == 256);
    v5.append_range(range5);
    assert(v5.capacity() == 256);
    assert(std::ranges::equal(v5.begin(), v5.end(), std::begin(in), std::end(in)));
  }
#endif

  return true;
}

int main(int, char**) {
  test();
  static_assert(test());

  // Note: `test_append_range_exception_safety_throwing_copy` doesn't apply because copying booleans cannot throw.
  test_append_range_exception_safety_throwing_allocator<std::vector, bool>();

  return 0;
}
