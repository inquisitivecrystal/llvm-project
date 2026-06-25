//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

// template<container-compatible-range<T> R>
//   vector(from_range_t, R&& rg, const Allocator& = Allocator()); // C++23

#include <sstream>
#include <vector>

#include "../../from_range_sequence_containers.h"
#include "asan_testing.h"
#include "test_macros.h"

constexpr bool test() {
  for_all_iterators_and_allocators<int>([]<class Iter, class Sent, class Alloc>() {
    test_sequence_container<std::vector, int, Iter, Sent, Alloc>([]([[maybe_unused]] const auto& c) {
      LIBCPP_ASSERT(c.__invariants());
      LIBCPP_ASSERT(is_contiguous_container_asan_correct(c));
    });
  });
  test_sequence_container_move_only<std::vector>();

  { // Ensure input-only sized ranges are accepted.
    using input_iter = cpp20_input_iterator<const int*>;
    const int in[]{1, 2, 3, 4};
    std::vector v(std::from_range, std::views::counted(input_iter{std::ranges::begin(in)}, std::ranges::ssize(in)));
    assert(std::ranges::equal(v, std::vector<int>{1, 2, 3, 4}));
  }

#if TEST_STD_VER >= 26
  { // test that approximately sized ranges work, even if the reserve_hint is inaccurate
    int in[]{1, 2, 3, 4, 5, 6, 7, 8};

    ApproxSizedRange range1(in, in + 8, /*hint=*/8);
    ApproxSizedRange range2(in, in + 8, /*hint=*/2);
    ApproxSizedRange range3(in, in + 8, /*hint=*/20);
    ApproxSizedRange range4(in, in + 8, /*hint=*/0);
    ApproxSizedRange range5(in, in, /*hint=*/0);

    std::vector v1(std::from_range, range1);
    std::vector v2(std::from_range, range2);
    std::vector v3(std::from_range, range3);
    std::vector v4(std::from_range, range4);
    std::vector<int> correct{1, 2, 3, 4, 5, 6, 7, 8};

    assert(std::ranges::equal(v1, correct));
    assert(std::ranges::equal(v2, correct));
    assert(std::ranges::equal(v3, correct));
    assert(std::ranges::equal(v4, correct));

    std::vector v5(std::from_range, range5);
    assert(std::ranges::equal(v5, std::vector<int>{}));
  }
#endif

  return true;
}

#ifndef TEST_HAS_NO_LOCALIZATION
void test_counted_istream_view() {
  std::istringstream is{"1 2 3 4"};
  auto vals = std::views::istream<int>(is);
  std::vector v(std::from_range, std::views::counted(vals.begin(), 3));
  assert(v == (std::vector{1, 2, 3}));
}
#endif

int main(int, char**) {
  static_assert(test_constraints<std::vector, int, double>());
  test();

  static_assert(test());

  test_exception_safety_throwing_copy<std::vector>();
  test_exception_safety_throwing_allocator<std::vector, int>();

#ifndef TEST_HAS_NO_LOCALIZATION
  test_counted_istream_view();
#endif

  return 0;
}
