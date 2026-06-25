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
//   constexpr void assign_range(R&& rg); // C++23

#include <sstream>
#include <vector>

#include "../insert_range_sequence_containers.h"
#include "test_macros.h"

// Tested cases:
// - different kinds of assignments (assigning an {empty/one-element/mid-sized/long range} to an
//   {empty/one-element/full} container);
// - an exception is thrown when allocating new elements.
constexpr bool test() {
  static_assert(test_constraints_assign_range<std::vector, bool, char>());

  for_all_iterators_and_allocators<bool, const int*>([]<class Iter, class Sent, class Alloc>() {
    test_sequence_assign_range<std::vector<bool, Alloc>, Iter, Sent>([]([[maybe_unused]] auto&& c) {
      LIBCPP_ASSERT(c.__invariants());
      // `is_contiguous_container_asan_correct` doesn't work on `vector<bool>`.
    });
  });

  {   // Vector may or may not need to reallocate because of the assignment -- make sure to test both cases.
    { // Ensure reallocation happens. Note that `vector<bool>` typically reserves a lot of capacity.
      constexpr int N     = 255;
      bool in[N]          = {};
      std::vector<bool> v = {0, 0, 0, 1, 1, 0, 0, 0};
      assert(v.capacity() < v.size() + std::ranges::size(in));

      v.assign_range(in);
      assert(std::ranges::equal(v, in));
    }

    { // Ensure no reallocation happens.
      bool in[]           = {1, 1, 0, 1, 1};
      std::vector<bool> v = {0, 0, 0, 1, 1, 0, 0, 0};

      v.assign_range(in);
      assert(std::ranges::equal(v, in));
    }

    { // Ensure input-only sized ranges are accepted.
      using input_iter = cpp20_input_iterator<const bool*>;
      const bool in[]{true, true, false, true};
      std::vector<bool> v;
      v.assign_range(std::views::counted(input_iter{std::ranges::begin(in)}, std::ranges::ssize(in)));
      assert(std::ranges::equal(v, std::vector<bool>{true, true, false, true}));
    }
  }

#if TEST_STD_VER >= 26
  { // test assigning an approximately sized range
    constexpr int N1 = 64;
    bool in64[N1]    = {};

    for (int i = 0; i < N1; ++i)
      in64[i] = i % 3 == 0;

    constexpr int N2 = 128;
    bool in128[N2]   = {};
    std::ranges::copy(in64, in128);
    std::ranges::copy(in64, in128 + 64);

    std::vector<bool> v1(64);
    std::vector<bool> v2 = v1;
    std::vector<bool> v3 = v1;
    std::vector<bool> v4 = v1;
    std::vector<bool> v5 = v1;

    ApproxSizedRange range1(in64, in64 + 64, /*hint=*/64);
    assert(v1.capacity() == 64);
    v1.assign_range(range1);
    assert(v1.capacity() == 64);
    assert(std::ranges::equal(v1, in64));

    ApproxSizedRange range2(in128, in128 + 128, /*hint=*/2);
    assert(v2.capacity() == 64);
    v2.assign_range(range2);
    assert(v2.capacity() >= 128);
    assert(std::ranges::equal(v2, in128));

    ApproxSizedRange range3(in64, in64 + 64, /*hint=*/512);
    v3.reserve(128);
    assert(v3.capacity() >= 128);
    assert(v3.capacity() < 512);
    v3.assign_range(range3);
    assert(v3.capacity() >= 512);
    assert(std::ranges::equal(v3, in64));

    ApproxSizedRange range4(in64, in64 + 64, /*hint=*/0);
    assert(v4.capacity() == 64);
    v4.assign_range(range4);
    assert(v4.capacity() == 64);
    assert(std::ranges::equal(v4, in64));

    ApproxSizedRange range5(in64, in64, /*hint=*/0);
    assert(v5.capacity() == 64);
    v5.assign_range(range5);
    assert(v5.capacity() == 64);
    assert(v5.empty());
  }
#endif

  return true;
}

#ifndef TEST_HAS_NO_LOCALIZATION
void test_counted_istream_view() {
  std::istringstream is{"1 1 0 1"};
  auto vals = std::views::istream<bool>(is);
  std::vector<bool> v;
  v.assign_range(std::views::counted(vals.begin(), 3));
  assert(v == (std::vector{true, true, false}));
}
#endif

int main(int, char**) {
  test();
  static_assert(test());

  // Note: `test_assign_range_exception_safety_throwing_copy` doesn't apply because copying booleans cannot throw.
  test_assign_range_exception_safety_throwing_allocator<std::vector, bool>();

#ifndef TEST_HAS_NO_LOCALIZATION
  test_counted_istream_view();
#endif

  return 0;
}
