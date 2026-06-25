//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// UNSUPPORTED: c++03, c++11, c++14, c++17, c++20

#include <sstream>
#include <vector>

#include "../from_range_sequence_containers.h"
#include "test_macros.h"

// template<container-compatible-range<T> R>
//   vector(from_range_t, R&& rg, const Allocator& = Allocator()); // C++23

constexpr bool test() {
  for_all_iterators_and_allocators<bool>([]<class Iter, class Sent, class Alloc>() {
    test_vector_bool<Iter, Sent, Alloc>([]([[maybe_unused]] const auto& c) {
      LIBCPP_ASSERT(c.__invariants());
      // `is_contiguous_container_asan_correct` doesn't work on `vector<bool>`.
    });
  });

  { // Ensure input-only sized ranges are accepted.
    using input_iter = cpp20_input_iterator<const bool*>;
    const bool in[]{true, true, false, true};
    std::vector v(std::from_range, std::views::counted(input_iter{std::ranges::begin(in)}, std::ranges::ssize(in)));
    assert(std::ranges::equal(v, std::vector<bool>{true, true, false, true}));
  }

#if TEST_STD_VER >= 26
  { // test that approximately sized ranges work, even if the reserve_hint is inaccurate
    bool in[]{true, true, true, true, false, false, false, false};

    ApproxSizedRange range1(in, in + 8, /*hint=*/8);
    ApproxSizedRange range2(in, in + 8, /*hint=*/2);
    ApproxSizedRange range3(in, in + 8, /*hint=*/20);
    ApproxSizedRange range4(in, in + 8, /*hint=*/0);
    ApproxSizedRange range5(in, in, /*hint=*/0);

    std::vector v1(std::from_range, range1);
    std::vector v2(std::from_range, range2);
    std::vector v3(std::from_range, range3);
    std::vector v4(std::from_range, range4);
    std::vector<bool> correct{true, true, true, true, false, false, false, false};

    assert(std::ranges::equal(v1, correct));
    assert(std::ranges::equal(v2, correct));
    assert(std::ranges::equal(v3, correct));
    assert(std::ranges::equal(v4, correct));

    std::vector v5(std::from_range, range5);
    assert(std::ranges::equal(v5, std::vector<bool>{}));
  }
#endif

  return true;
}

#ifndef TEST_HAS_NO_LOCALIZATION
void test_counted_istream_view() {
  std::istringstream is{"1 1 0 1"};
  auto vals = std::views::istream<bool>(is);
  std::vector v(std::from_range, std::views::counted(vals.begin(), 3));
  assert(v == (std::vector{true, true, false}));
}
#endif

int main(int, char**) {
  test();
  static_assert(test());

  static_assert(test_constraints<std::vector, bool, char>());

  // Note: test_exception_safety_throwing_copy doesn't apply because copying a boolean cannot throw.
  test_exception_safety_throwing_allocator<std::vector, bool>();

#ifndef TEST_HAS_NO_LOCALIZATION
  test_counted_istream_view();
#endif

  return 0;
}
