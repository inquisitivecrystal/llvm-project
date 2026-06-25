//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20
// ADDITIONAL_COMPILE_FLAGS(has-fconstexpr-steps): -fconstexpr-steps=2000000

// template<container-compatible-range<T> R>
//   constexpr void assign_range(R&& rg); // C++23

#include <sstream>
#include <vector>

#include "../../insert_range_sequence_containers.h"
#include "asan_testing.h"
#include "test_macros.h"

// Tested cases:
// - different kinds of assignments (assigning an {empty/one-element/mid-sized/long range} to an
//   {empty/one-element/full} container);
// - assigning move-only elements;
// - an exception is thrown when copying the elements or when allocating new elements.
constexpr bool test() {
  static_assert(test_constraints_assign_range<std::vector, int, double>());

  for_all_iterators_and_allocators<int, const int*>([]<class Iter, class Sent, class Alloc>() {
    test_sequence_assign_range<std::vector<int, Alloc>, Iter, Sent>([]([[maybe_unused]] auto&& c) {
      LIBCPP_ASSERT(c.__invariants());
      LIBCPP_ASSERT(is_contiguous_container_asan_correct(c));
    });
  });
  test_sequence_assign_range_move_only<std::vector>();

  {   // Vector may or may not need to reallocate because of the assignment -- make sure to test both cases.
    { // Ensure reallocation happens.
      int in[]           = {-1, -2, -3, -4, -5, -6, -7, -8, -9, -10};
      std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};
      v.shrink_to_fit();
      assert(v.capacity() < v.size() + std::ranges::size(in));

      v.assign_range(in);
      assert(std::ranges::equal(v, in));
    }

    { // Ensure no reallocation happens -- the input range is shorter than the vector.
      int in[]           = {-1, -2, -3, -4, -5};
      std::vector<int> v = {1, 2, 3, 4, 5, 6, 7, 8};

      v.assign_range(in);
      assert(std::ranges::equal(v, in));
    }

    { // Ensure no reallocation happens -- the input range is longer than the vector but within capacity.
      int in[]           = {-1, -2, -3, -4, -5, -6, -7, -8};
      std::vector<int> v = {1, 2, 3, 4, 5};
      v.reserve(std::ranges::size(in));
      assert(v.capacity() >= std::ranges::size(in));

      v.assign_range(in);
      assert(std::ranges::equal(v, in));
    }

    { // Ensure input-only sized ranges are accepted.
      using input_iter = cpp20_input_iterator<const int*>;
      const int in[]{1, 2, 3, 4};
      std::vector<int> v;
      v.assign_range(std::views::counted(input_iter{std::ranges::begin(in)}, std::ranges::ssize(in)));
      assert(std::ranges::equal(v, std::vector<int>{1, 2, 3, 4}));
    }
  }

#if TEST_STD_VER >= 26
  { // test assigning an approximately sized range
    int in3[]{1, 2, 3};
    int in8[]{1, 2, 3, 4, 5, 6, 7, 8};
    int in12[]{1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    std::vector<int> v1 = {0, 1, 2, 3, 4, 5, 6, 7};
    v1.shrink_to_fit();
    std::vector<int> v2 = v1;
    std::vector<int> v3 = v1;
    std::vector<int> v4 = v1;
    std::vector<int> v5 = v1;
    std::vector<int> v6 = v1;

    ApproxSizedRange range1(in8, in8 + 8, /*hint=*/8);
    assert(v1.capacity() == 8);
    v1.assign_range(range1);
    assert(v1.capacity() == 8);
    assert(std::ranges::equal(v1, in8));

    ApproxSizedRange range2(in12, in12 + 12, /*hint=*/2);
    assert(v2.capacity() == 8);
    v2.assign_range(range2);
    assert(v2.capacity() >= 12);
    assert(std::ranges::equal(v2, in12));

    ApproxSizedRange range3(in8, in8 + 8, /*hint=*/100);
    v3.reserve(16);
    assert(v3.capacity() >= 16);
    assert(v3.capacity() < 100);
    v3.assign_range(range3);
    assert(v3.capacity() >= 100);
    assert(std::ranges::equal(v3, in8));

    ApproxSizedRange range4(in8, in8 + 8, /*hint=*/0);
    assert(v4.capacity() == 8);
    v4.assign_range(range4);
    assert(v4.capacity() == 8);
    assert(std::ranges::equal(v4, in8));

    ApproxSizedRange range5(in8, in8, /*hint=*/0);
    assert(v5.capacity() == 8);
    v5.assign_range(range5);
    assert(v5.capacity() == 8);
    assert(v5.empty());

    ApproxSizedRange range6(in3, in3 + 3, /*hint=*/3);
    assert(v6.capacity() == 8);
    v6.assign_range(range6);
    assert(v6.capacity() == 8);
    assert(std::ranges::equal(v6, in3));
  }
#endif

  return true;
}

#ifndef TEST_HAS_NO_LOCALIZATION
void test_counted_istream_view() {
  std::istringstream is{"1 2 3 4"};
  auto vals = std::views::istream<int>(is);
  std::vector<int> v;
  v.assign_range(std::views::counted(vals.begin(), 3));
  assert(v == (std::vector{1, 2, 3}));
}
#endif

int main(int, char**) {
  test();
  static_assert(test());

  test_assign_range_exception_safety_throwing_copy<std::vector>();
  test_assign_range_exception_safety_throwing_allocator<std::vector, int>();

#ifndef TEST_HAS_NO_LOCALIZATION
  test_counted_istream_view();
#endif

  return 0;
}
