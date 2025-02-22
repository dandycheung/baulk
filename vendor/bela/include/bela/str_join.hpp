// ---------------------------------------------------------------------------
// Copyright © 2025, Bela contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Includes work from abseil-cpp (https://github.com/abseil/abseil-cpp)
// with modifications.
//
// Copyright 2019 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ---------------------------------------------------------------------------
#ifndef BELA_STR_JOIN_HPP
#define BELA_STR_JOIN_HPP
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <string_view>
#include "__strings/str_join_internal.hpp"

namespace bela {
// AlphaNumFormatter()
//
// Default formatter used if none is specified. Uses `absl::AlphaNum` to convert
// numeric arguments to strings.
inline strings_internal::AlphaNumFormatterImpl AlphaNumFormatter() { return strings_internal::AlphaNumFormatterImpl(); }

// Function Template: PairFormatter(Formatter, std::wstring_view, Formatter)
//
// Formats a `std::pair` by putting a given separator between the pair's
// `.first` and `.second` members. This formatter allows you to specify
// custom Formatters for both the first and second member of each pair.
template <typename FirstFormatter, typename SecondFormatter>
inline strings_internal::PairFormatterImpl<FirstFormatter, SecondFormatter>
PairFormatter(FirstFormatter f1, std::wstring_view sep, SecondFormatter f2) {
  return strings_internal::PairFormatterImpl<FirstFormatter, SecondFormatter>(std::move(f1), sep, std::move(f2));
}

// Function overload of PairFormatter() for using a default
// `AlphaNumFormatter()` for each Formatter in the pair.
inline strings_internal::PairFormatterImpl<strings_internal::AlphaNumFormatterImpl,
                                           strings_internal::AlphaNumFormatterImpl>
PairFormatter(std::wstring_view sep) {
  return PairFormatter(AlphaNumFormatter(), sep, AlphaNumFormatter());
}

// Function Template: DereferenceFormatter(Formatter)
//
// Formats its argument by dereferencing it and then applying the given
// formatter. This formatter is useful for formatting a container of
// pointer-to-T. This pattern often shows up when joining repeated fields in
// protocol buffers.
template <typename Formatter>
strings_internal::DereferenceFormatterImpl<Formatter> DereferenceFormatter(Formatter &&f) {
  return strings_internal::DereferenceFormatterImpl<Formatter>(std::forward<Formatter>(f));
}

// Function overload of `DereferenceFormatter()` for using a default
// `AlphaNumFormatter()`.
inline strings_internal::DereferenceFormatterImpl<strings_internal::AlphaNumFormatterImpl> DereferenceFormatter() {
  return strings_internal::DereferenceFormatterImpl<strings_internal::AlphaNumFormatterImpl>(AlphaNumFormatter());
}

// -----------------------------------------------------------------------------
// StrJoin()
// -----------------------------------------------------------------------------
//
// Joins a range of elements and returns the result as a std::wstring.
// `bela::StrJoin()` takes a range, a separator string to use between the
// elements joined, and an optional Formatter responsible for converting each
// argument in the range to a string.
//
// If omitted, the default `AlphaNumFormatter()` is called on the elements to be
// joined.
//
// Example 1:
//   // Joins a collection of strings. This pattern also works with a collection
//   // of `std::wstring_view` or even `const char*`.
//   std::vector<std::wstring> v = {"foo", "bar", "baz"};
//   std::wstring s = bela::StrJoin(v, "-");
//   EXPECT_EQ("foo-bar-baz", s);
//
// Example 2:
//   // Joins the values in the given `std::initializer_list<>` specified using
//   // brace initialization. This pattern also works with an initializer_list
//   // of ints or `std::wstring_view` -- any `AlphaNum`-compatible type.
//   std::wstring s = bela::StrJoin({"foo", "bar", "baz"}, "-");
//   EXPECT_EQ("foo-bar-baz", s);
//
// Example 3:
//   // Joins a collection of ints. This pattern also works with floats,
//   // doubles, int64s -- any `StrCat()`-compatible type.
//   std::vector<int> v = {1, 2, 3, -4};
//   std::wstring s = bela::StrJoin(v, "-");
//   EXPECT_EQ("1-2-3--4", s);
//
// Example 4:
//   // Joins a collection of pointer-to-int. By default, pointers are
//   // dereferenced and the pointee is formatted using the default format for
//   // that type; such dereferencing occurs for all levels of indirection, so
//   // this pattern works just as well for `std::vector<int**>` as for
//   // `std::vector<int*>`.
//   int x = 1, y = 2, z = 3;
//   std::vector<int*> v = {&x, &y, &z};
//   std::wstring s = bela::StrJoin(v, "-");
//   EXPECT_EQ("1-2-3", s);
//
// Example 5:
//   // Dereferencing of `std::unique_ptr<>` is also supported:
//   std::vector<std::unique_ptr<int>> v
//   v.emplace_back(new int(1));
//   v.emplace_back(new int(2));
//   v.emplace_back(new int(3));
//   std::wstring s = bela::StrJoin(v, "-");
//   EXPECT_EQ("1-2-3", s);
//
// Example 6:
//   // Joins a `std::map`, with each key-value pair separated by an equals
//   // sign. This pattern would also work with, say, a
//   // `std::vector<std::pair<>>`.
//   std::map<std::wstring, int> m = {
//       std::make_pair("a", 1),
//       std::make_pair("b", 2),
//       std::make_pair("c", 3)};
//   std::wstring s = bela::StrJoin(m, ",", absl::PairFormatter("="));
//   EXPECT_EQ("a=1,b=2,c=3", s);
//
// Example 7:
//   // These examples show how `bela::StrJoin()` handles a few common edge
//   // cases:
//   std::vector<std::wstring> v_empty;
//   EXPECT_EQ("", bela::StrJoin(v_empty, "-"));
//
//   std::vector<std::wstring> v_one_item = {"foo"};
//   EXPECT_EQ("foo", bela::StrJoin(v_one_item, "-"));
//
//   std::vector<std::wstring> v_empty_string = {""};
//   EXPECT_EQ("", bela::StrJoin(v_empty_string, "-"));
//
//   std::vector<std::wstring> v_one_item_empty_string = {"a", ""};
//   EXPECT_EQ("a-", bela::StrJoin(v_one_item_empty_string, "-"));
//
//   std::vector<std::wstring> v_two_empty_string = {"", ""};
//   EXPECT_EQ("-", bela::StrJoin(v_two_empty_string, "-"));
//
// Example 8:
//   // Joins a `std::tuple<T...>` of heterogeneous types, converting each to
//   // a std::wstring using the `absl::AlphaNum` class.
//   std::wstring s = bela::StrJoin(std::make_tuple(123, "abc", 0.456), "-");
//   EXPECT_EQ("123-abc-0.456", s);

template <typename Iterator, typename Formatter>
std::wstring StrJoin(Iterator start, Iterator end, std::wstring_view sep, Formatter &&fmt) {
  return strings_internal::JoinAlgorithm(start, end, sep, fmt);
}

template <typename Range, typename Formatter>
std::wstring StrJoin(const Range &range, std::wstring_view separator, Formatter &&fmt) {
  return strings_internal::JoinRange(range, separator, fmt);
}

template <typename T, typename Formatter>
std::wstring StrJoin(std::initializer_list<T> il, std::wstring_view separator, Formatter &&fmt) {
  return strings_internal::JoinRange(il, separator, fmt);
}

template <typename... T, typename Formatter>
std::wstring StrJoin(const std::tuple<T...> &value, std::wstring_view separator, Formatter &&fmt) {
  return strings_internal::JoinAlgorithm(value, separator, fmt);
}

template <typename Iterator> std::wstring StrJoin(Iterator start, Iterator end, std::wstring_view separator) {
  return strings_internal::JoinRange(start, end, separator);
}

template <typename Range> std::wstring StrJoin(const Range &range, std::wstring_view separator) {
  return strings_internal::JoinRange(range, separator);
}

template <typename T> std::wstring StrJoin(std::initializer_list<T> il, std::wstring_view separator) {
  return strings_internal::JoinRange(il, separator);
}

template <typename... T> std::wstring StrJoin(const std::tuple<T...> &value, std::wstring_view separator) {
  return strings_internal::JoinAlgorithm(value, separator, AlphaNumFormatter());
}

} // namespace bela

#endif