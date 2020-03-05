// snmp_stream/_snmp_stream/utils.hpp

#ifndef UTILS_HPP
#define UTILS_HPP

#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>

namespace snmp_stream {

/*!
  Join a sequence into a string.  Supports any type `T` that
  `std::ostringstream` supports.

  \return `std::string`
*/
template <typename T>
[[nodiscard]] inline auto join(T *arr,                //!< Pointer to a c-array.
                               size_t size,           //!< Size of the c-array.
                               std::string const &sep //!< Separator.
                               ) -> std::string {
  std::ostringstream oss;
  if (size != 0) {
    oss << arr[0];
  }
  for (size_t i = 1; i < size; ++i) {
    oss << sep << arr[i];
  }
  return oss.str();
}

/*!
  Join a sequence into a string.  Supports any type `T` that
  `std::ostringstream` supports.

  \return `std::string`
*/
template <typename T>
[[nodiscard]] inline auto join(T begin,               //!< Start iterator.
                               T end,                 //!< End iterator.
                               std::string const &sep //!< Separator.
                               ) -> std::string {
  std::ostringstream oss;
  if (begin != end) {
    oss << *begin++;
  }
  while (begin != end) {
    oss << sep << *begin++;
  }
  return oss.str();
}

/*!
  Convert an OID to a string.

  \return `std::string`
*/
[[nodiscard]] auto
oid_to_string(uint64_t const *oid, //!< C-array formatted OID.
              size_t oid_size      //!< Size of the c-array.
              ) -> std::string;

/*!
  Convert an OID to a string.

  \return `std::string`
*/
[[nodiscard]] auto
oid_to_string(std::vector<uint64_t> const &oid //!< Vector formatted OID.
              ) -> std::string;

/*!
  Convert an optional OID to a string.

  \return `std::string`
*/
[[nodiscard]] inline auto
oid_to_string(std::optional<std::vector<uint64_t>> const
                  &oid //!< Optional vector formatted OID.
              ) -> std::string {
  if (oid.has_value()) {
    return oid_to_string(*oid);
  }
  return "None";
}

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`.
*/
[[nodiscard]] inline auto
attr_to_string(std::string const &string //!< String attribute.
               ) -> std::string {
  return "'" + string + "'";
}

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`.
*/
template <typename T, typename std::enable_if<std::is_integral<T>::value,
                                              T>::type * = nullptr>
[[nodiscard]] inline auto attr_to_string(T const &val) -> std::string {
  return std::to_string(val);
}

//! Template for objects with a repr() method.
template <typename, typename = void> struct has_repr : std::false_type {};
//! Template for objects with a repr() method.
template <typename T>
struct has_repr<T, std::void_t<decltype(std::declval<T>().repr())>>
    : std::is_same<decltype(std::declval<T>().repr()), std::string> {};

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`.
*/
template <typename T,
          typename std::enable_if<has_repr<T>::value, T>::type * = nullptr>
[[nodiscard]] inline auto attr_to_string(T const &val) -> std::string {
  return val.repr();
}

//! Template for optional types.
template <typename T> struct is_optional : std::false_type {};
//! Template for optional types.
template <typename T> struct is_optional<std::optional<T>> : std::true_type {};

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`.
*/
template <typename T,
          typename std::enable_if<is_optional<T>::value, T>::type * = nullptr>
[[nodiscard]] inline auto attr_to_string(T const &val) -> std::string {
  if (val.has_value()) {
    return attr_to_string(*val);
  }
  return "None";
}

//! Template for iterable types.
template <typename T, typename = void> struct is_iterable : std::false_type {};
//! Template for iterable types.
template <typename T>
struct is_iterable<T, std::void_t<decltype(std::declval<T>().begin()),
                                  decltype(std::declval<T>().end())>>
    : std::true_type {};

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`.
*/
template <typename T,
          typename std::enable_if<is_iterable<T>::value && !has_repr<T>::value,
                                  T>::type * = nullptr>
[[nodiscard]] inline auto attr_to_string(T const &val) -> std::string {
  std::vector<std::string> strings;
  std::transform(val.begin(), val.end(), std::back_inserter(strings),
                 [](auto i) -> std::string { return attr_to_string(i); });
  return "[" + join(strings.begin(), strings.end(), ", ") + "]";
}

//! Template for smart pointers.
template <typename T, typename = void> struct is_smart_ptr : std::false_type {};
//! Template for smart pointers.
template <typename T>
struct is_smart_ptr<std::shared_ptr<T>> : std::true_type {};
//! Template for smart pointers.
template <typename T>
struct is_smart_ptr<std::unique_ptr<T>> : std::true_type {};
//! Template for smart pointers.
template <typename T> struct is_smart_ptr<std::weak_ptr<T>> : std::true_type {};

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`.
*/
template <typename T,
          typename std::enable_if<is_smart_ptr<T>::value, T>::type * = nullptr>
[[nodiscard]] inline auto attr_to_string(T const &val) -> std::string {
  if (val != nullptr) {
    return attr_to_string(*val);
  }
  return "None";
}

} // namespace snmp_stream

#endif
