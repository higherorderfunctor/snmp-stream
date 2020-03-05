// snmp_stream/_snmp_stream/types.hpp

#ifndef TYPES_HPP
#define TYPES_HPP

#include <memory>
#include <optional>
#include <vector>

extern "C" {
#include <net-snmp/net-snmp-config.h>
#include <net-snmp/net-snmp-includes.h>
}

#define INLINE_CONST_GETTER(T, field)                                          \
  /*! Auto-generated getter. */                                                \
  [[nodiscard]] inline auto get_##field() const->decltype(T::field) const & {  \
    return this->field;                                                        \
  }                                                                            \
  /*! Auto-generated getter. */                                                \
  [[nodiscard]] inline auto get_##field()->decltype(T::field) const & {        \
    return std::as_const(*this).get_##field();                                 \
  }

#define INLINE_SETTER(T, field)                                                \
  /*! Auto-generated setter. */                                                \
  inline void set_##field(decltype(T::field) val) { (field) = std::move(val); }

#define REPR(T)                                                                \
  /*! Auto-generated repr() const method.  \return `std::string` */            \
  [[nodiscard]] auto repr() const->std::string;                                \
  /*! Auto-generated repr() method.  \return `std::string` */                  \
  [[nodiscard]] inline auto repr()->std::string {                              \
    return std::as_const(*this).repr();                                        \
  };

namespace snmp_stream {

//! Single OID octet.  Octect size is determined by NET-SNMP.
using oid_t = oid;

/*!
  Object identity.
*/
class ObjectIdentity : public std::vector<oid_t> {
public:
  /*!
    Default constructor (zero length OID).

    \code{.cpp}
    ObjectIdentity();
    \endcode
  */
  ObjectIdentity() = default;

  /*!
    C-array pointer constructor.

    \code{.cpp}
    ObjectIdentity(ptr, ptr + size);
    \endcode
  */
  ObjectIdentity(oid_t const *begin, //!< Pointer to start of c-array.
                 oid_t const *end    //!< Pointer to end of c-array.
                 )
      : std::vector<oid_t>(begin, end) {}

  /*!
    Vector constructor.

    \code{.cpp}
    ObjectIdentity({1, 2, 3});
    ObjectIdentity(ObjectIdentity({1, 2, 3}));
    \endcode
  */
  ObjectIdentity(std::vector<oid_t> const &oid //!< Vector OID.
                 )
      : std::vector<oid_t>(oid) {}

  /*!
    Optional constructor.

    \code{.cpp}
    ObjectIdentity(std::nullopt); // same as default constructor
    \endcode
  */
  template <typename T>
  ObjectIdentity(std::optional<T> const &oid //!< Optional OID.
                 )
      : std::vector<oid_t>(oid.has_value() ? *oid : ObjectIdentity()) {}

  /*!
    Test if this `ObjectIdentity` is a root of another `ObjectIdentity`.

    \return `bool`.
  */
  [[nodiscard]] inline auto
  is_root_of(ObjectIdentity const &other //!< Other `ObjectIdentity` to compare.
  ) const -> bool {
    return netsnmp_oid_is_subtree(this->data(), this->size(), other.data(),
                                  other.size()) == 0;
  }

  [[nodiscard]] inline auto
  is_root_of(ObjectIdentity const &other //!< Other `ObjectIdentity` to compare.
             ) -> bool {
    return std::as_const(*this).is_root_of(other);
  };

  /*!
    Concatenate two `ObjectIdentity` together returning a new `ObjectIdentity`.

    \return `ObjectIdentity`
  */
  [[nodiscard]] inline auto operator+(
      ObjectIdentity const &other //!< Other `ObjectIdentity` to concatenate.
      ) -> ObjectIdentity {
    ObjectIdentity oid;
    oid.reserve(this->size() + other.size());
    oid.insert(oid.end(), this->begin(), this->end());
    oid.insert(oid.end(), other.begin(), other.end());
    return oid;
  }

  /*!
    Lexicographically compare two `ObjectIdentity`.

    \return `bool`
  */
  [[nodiscard]] inline auto
  operator<(ObjectIdentity const &other //!< Other `ObjectIdentity` to compare.
            ) -> bool {
    return snmp_oid_compare(this->data(), this->size(), other.data(),
                            other.size()) == -1;
  }

  /*!
    Lexicographically compare two `ObjectIdentity`.

    \return `bool`
  */
  [[nodiscard]] inline auto
  operator<=(ObjectIdentity const &other //!< Other `ObjectIdentity` to compare.

             ) -> bool {
    return snmp_oid_compare(this->data(), this->size(), other.data(),
                            other.size()) <= 0;
  }

  /*!
    Lexicographically compare two `ObjectIdentity`.

    \return `bool`
  */
  [[nodiscard]] inline auto
  operator>(ObjectIdentity const &other //!< Other `ObjectIdentity` to compare.

            ) -> bool {
    return snmp_oid_compare(this->data(), this->size(), other.data(),
                            other.size()) == 1;
  }

  /*!
    Lexicographically compare two `ObjectIdentity`.

    \return `bool`
  */
  [[nodiscard]] inline auto
  operator>=(ObjectIdentity const &other //!< Other `ObjectIdentity` to compare.
             ) -> bool {
    return snmp_oid_compare(this->data(), this->size(), other.data(),
                            other.size()) >= 0;
  }

  REPR(ObjectIdentityRange);
};

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`
*/
[[nodiscard]] auto attr_to_string(ObjectIdentity const &val) -> std::string;

/*!
  OID range.  This class is for masking OIDs to see if they fall within a range.
*/
class ObjectIdentityRange {
private:
  ObjectIdentity start; //!< Start value.
  ObjectIdentity stop;  //!< Stop value.

public:
  /*!
    Default constructor.

    \code{.cpp}
    ObjectRange(); // ObjectIdentityRange(ObjectIdentity(), ObjectIdentity())
    \endcode
  */
  ObjectIdentityRange() = default;

  /*!
    Point constructor.
  */
  ObjectIdentityRange(ObjectIdentity const &oid //!< Single OID.
                      )
      : start(oid), stop(oid){};

  /*!
    Range constructor.

    \exception std::invalid_argument `start` is not lexicographically less
    than `stop`.
  */
  ObjectIdentityRange(
      std::optional<ObjectIdentity> const &start, //!< Optional start OID.
      std::optional<ObjectIdentity> const &stop   //!< Optional stop OID.
  );

  INLINE_CONST_GETTER(ObjectIdentityRange, start);
  INLINE_CONST_GETTER(ObjectIdentityRange, stop);
  REPR(ObjectIdentityRange);
};

/*!
  Compare two `ObjectIdentityRange`.

  \return `bool`
*/
[[nodiscard]] inline auto operator==(
    ObjectIdentityRange const &lhs, //!< Left-hand side object to compare.
    ObjectIdentityRange const &rhs  //!< Right-hand side object to compare.
    ) -> bool {
  return (lhs.get_start() == rhs.get_start()) &&
         (lhs.get_stop() == rhs.get_stop());
}

/*!
  Compare two `ObjectIdentityRange`.  Lexicographically compare
  `ObjectIdentityRange.start`.  If they are equal, the largest range comes
  first.  The following truth table implements this comparison.

  \return `bool`
*/
// clang-format off
/*!
  \rst
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | lhs.start | lhs.stop | < | rhs.start | rhs.stop |                                               |
  +===========+==========+===+===========+==========+===============================================+
  | none      | none     |   | none      | none     | false                                         |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | none      | none     |   | none      | (value)  | true                                          |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | none      | none     |   | (value)   | none     | true                                          |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | none      | none     |   | (value)   | (value)  | true                                          |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | none      | (value)  |   | none      | none     | false                                         |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | none      | (value)  |   | none      | (value)  | lhs.stop > rhs.stop                           |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | none      | (value)  |   | (value)   | none     | true                                          |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | none      | (value)  |   | (value)   | (value)  | true                                          |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | (value)   | none     |   | none      | none     | false                                         |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | (value)   | none     |   | none      | (value)  | false                                         |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | (value)   | none     |   | (value)   | none     | lhs.start < rhs.start                         |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | (value)   | none     |   | (value)   | (value)  | lhs.start <= rhs.start                        |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | (value)   | (value)  |   | none      | none     | false                                         |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | (value)   | (value)  |   | none      | (value)  | false                                         |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | (value)   | (value)  |   | (value)   | none     | lhs.start < rhs.start                         |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  | (value)   | (value)  |   | (value)   | (value)  | lhs.start < rhs.start ||                      |
  |           |          |   |           |          | lhs.start == rhs.start &&                     |
  |           |          |   |           |          | lhs.stop > rhs.stop                           |
  +-----------+----------+---+-----------+----------+-----------------------------------------------+
  \endrst
*/
// clang-format on
[[nodiscard]] auto
operator<(ObjectIdentityRange const &lhs, //!< Left-hand side object to compare.
          ObjectIdentityRange const &rhs //!< Right-hand side object to compare.
          ) -> bool;

/*!
  Compare two `ObjectIdentityRange`.

  \return `bool`
*/
// clang-format off
/*!
  \sa Implemented in terms of `operator<(ObjectIdentityRange const &, ObjectIdentityRange const &)`.
*/
// clang-format on
[[nodiscard]] inline auto operator<=(
    ObjectIdentityRange const &lhs, //!< Left-hand side object to compare.
    ObjectIdentityRange const &rhs  //!< Right-hand side object to compare.
    ) -> bool {
  return lhs == rhs || lhs < rhs;
}

/*!
  Compare two `ObjectIdentityRange`.

  \return `bool`
*/
// clang-format off
/*!
  \sa Implemented in terms of `operator<(ObjectIdentityRange const &, ObjectIdentityRange const &)`.
*/
// clang-format on
[[nodiscard]] inline auto
operator>(ObjectIdentityRange const &lhs, //!< Left-hand side object to compare.
          ObjectIdentityRange const &rhs //!< Right-hand side object to compare.
          ) -> bool {
  return rhs < lhs;
}

/*!
  Compare two `ObjectIdentityRange`.

  \return `bool`
*/
// clang-format off
/*!
  \sa Implemented in terms of `operator<(ObjectIdentityRange const &, ObjectIdentityRange const &)`.
*/
// clang-format on
[[nodiscard]] inline auto operator>=(
    ObjectIdentityRange const &lhs, //!< Left-hand side object to compare.
    ObjectIdentityRange const &rhs  //!< Right-hand side object to compare.
    ) -> bool {
  return lhs == rhs || lhs > rhs;
}

/*!
  SNMP community string and protocol version.
*/
class Community {
public:
  /*!
    SNMP protocol versions.
  */
  enum Version {
    V1 = SNMP_VERSION_1,  //!< SNMP version 1.
    V2C = SNMP_VERSION_2c //!< SNMP version 2c.
  };

private:
  std::string string; //!< Community string.
  Version version;    //!< SNMP protocol version.

public:
  Community(std::string string, //!< Community string.
            Version version     //!< SNMP protocol version.
            )
      : string(std::move(string)), version(version) {}

  INLINE_CONST_GETTER(Community, string);
  INLINE_CONST_GETTER(Community, version);
  REPR(Community);
};

/*!
  Compare two `Community`.

  \return `bool`
*/
[[nodiscard]] inline auto
operator==(Community const &lhs, //!< Left-hand side object to compare.
           Community const &rhs  //!< Right-hand side object to compare.

           ) -> bool {
  return (lhs.get_string() == rhs.get_string()) &&
         (lhs.get_version() == rhs.get_version());
}

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`
*/
[[nodiscard]] inline auto
attr_to_string(Community::Version const &version //!< SNMP protocol version.
               ) -> std::string {
  std::string string;
  switch (version) {
  case Community::Version::V1:
    string = "V1";
    break;
  case Community::Version::V2C:
    string = "V2C";
    break;
  }
  return string;
}

/*!
  SNMP configuration.
*/
class Config {
private:
  std::optional<ssize_t> retries; //!< Number of retries.
  std::optional<ssize_t> timeout; //!< Timeout in seconds.
  std::optional<size_t>
      max_response_var_binds_per_pdu;       //!< Maximum number of variable
                                            //!< bindings per PDU.
  std::optional<size_t> max_async_sessions; //!< Number of concurrent sessions.

public:
  /*!
    \exception std::invalid_argument `retries` is not greater than or equal to
    0. \exception std::invalid_argument `timeout` is not greater than or equal
    to 0. \exception std::invalid_argument `max_async_sessions` is not greater
    than 0.
  */
  Config(std::optional<ssize_t> const retries, //!< Number of retries.
         std::optional<ssize_t> const timeout, //!< Timeout in seconds.
         std::optional<size_t> const
             max_response_var_binds_per_pdu, //!< Maximum number of variable
                                             //!< bindings per PDU.
         std::optional<size_t> const
             max_async_sessions //!< Number of concurrent sessions.
         )
      : retries(retries), timeout(timeout),
        max_response_var_binds_per_pdu(max_response_var_binds_per_pdu),
        max_async_sessions(max_async_sessions) {
    if (this->retries.has_value() && *this->retries < 0) {
      throw std::invalid_argument("retries must be greater than or equal to 0");
    }
    if (this->timeout.has_value() && *this->timeout < 0) {
      throw std::invalid_argument("timeout must be greater than or equal to 0");
    }
    if (this->max_response_var_binds_per_pdu.has_value() &&
        *this->max_response_var_binds_per_pdu < 0) {
      throw std::invalid_argument(
          "max_response_var_binds_per_pdu must be greater than 0");
    }
    if (this->max_async_sessions.has_value() && *this->max_async_sessions < 1) {
      throw std::invalid_argument("max_async_sessions must be greater than 0");
    }
  }

  INLINE_CONST_GETTER(Config, retries);
  INLINE_CONST_GETTER(Config, timeout);
  INLINE_CONST_GETTER(Config, max_response_var_binds_per_pdu);
  INLINE_CONST_GETTER(Config, max_async_sessions);
  REPR(Config);
};

/*!
  Compare two `Config`.

  \return `bool`
*/
[[nodiscard]] inline auto
operator==(Config const &lhs, //!< Left-hand side object to compare.
           Config const &rhs  //!< Right-hand side object to compare.
           ) -> bool {
  return (lhs.get_retries() == rhs.get_retries()) &&
         (lhs.get_timeout() == rhs.get_timeout()) &&
         (lhs.get_max_response_var_binds_per_pdu() ==
          rhs.get_max_response_var_binds_per_pdu()) &&
         (lhs.get_max_async_sessions() == rhs.get_max_async_sessions());
}

/*!
  Merge two `Config` by taking values from the `rhs` unless a value is
  `std::nullopt` where the `lhs` value is taken instead.

  \return `Config`
*/
[[nodiscard]] inline auto
operator<<(Config const &lhs, //!< Left-hand side `Config` to merge.
           Config const &rhs  //!< Right-hand side `Config` to merge.
           ) -> Config {
  return (Config){
      rhs.get_retries().has_value() ? rhs.get_retries() : lhs.get_retries(),
      rhs.get_timeout().has_value() ? rhs.get_timeout() : lhs.get_timeout(),
      rhs.get_max_response_var_binds_per_pdu().has_value()
          ? rhs.get_max_response_var_binds_per_pdu()
          : lhs.get_max_response_var_binds_per_pdu(),
      rhs.get_max_async_sessions().has_value() ? rhs.get_max_async_sessions()
                                               : lhs.get_max_async_sessions()};
}

/*!
  Merge two `Config` by taking values from the `rhs` unless a value is
  `std::nullopt` where the `lhs` value is taken instead.

  \return `Config`
*/
[[nodiscard]] inline auto operator<<(
    std::optional<Config> const &lhs, //!< Left-hand side `Config` to merge.
    Config const &rhs                 //!< Right-hand side `Config` to merge.
    ) -> class Config {
  return lhs.has_value() ? *lhs << rhs : rhs;
}

/*!
  Merge two `Config` by taking values from the `rhs` unless a value is
 ` std::nullopt` where the `lhs` value is taken instead.

  \return `Config`
*/
[[nodiscard]] inline auto
operator<<(
    Config const &lhs,               //!< Left-hand side `Config` to merge.
    std::optional<Config> const &rhs //!< Right-hand side `Config` to merge.
    ) -> Config {
  return rhs.has_value() ? lhs << *rhs : lhs;
}

/*!
  Merge two `Config` by taking values from the `rhs` unless a value is
  `std::nullopt` where the `lhs` value is taken instead.

  \return `Config`
*/
[[nodiscard]] inline auto operator<<(
    std::optional<Config> const &lhs, //!< Left-hand side `Config` to merge.
    std::optional<Config> const &rhs  //!< Right-hand side `Config` to merge.
    ) -> std::optional<Config> {
  if (lhs.has_value()) {
    return {*lhs << rhs};
  }
  return rhs;
}

/*!
  Test that no OID in a sequence is a root of another in that sequence.

  \return `std::optional<std::tuple<ObjectIdentity, ObjectIdentity>>`: Returns
  the pair that failed the test or `std::nullopt` if the test passed.
*/
[[nodiscard]] auto test_ambiguous_root_oids(
    std::vector<ObjectIdentity> const &oids // Sequence of OIDs.
    ) -> std::optional<std::tuple<ObjectIdentity, ObjectIdentity>>;

/*!
  SNMP request.
*/
class SnmpRequest {
public:
  /*!
    SNMP request types.
  */
  enum SnmpRequestType {
    GET_REQUEST = 0, //!< Get request.
    WALK_REQUEST     //!< Walk request.
  };

private:
  SnmpRequestType type;             //!< SNMP request type.
  std::string host;                 //!< Target host.
  Community community;              //!< SNMP community.
  std::vector<ObjectIdentity> oids; //!< Sequence of OIDs to collect.
  std::optional<std::vector<ObjectIdentityRange>>
      ranges; //!< Optional sequence of OID ranges to restrict collection on
              //!< (appended to each OID).
  std::optional<std::string> req_id; //!< Optional request ID.
  std::optional<Config> config;      //!< SNMP configuration.

  /*!
    Optimize `ObjectIdentityRange` by collapsing overlapping segments.  A
    GET_REQUEST only supports point ranges.

    \exception std::invalid_argument `GET_REQUEST` contains non-point
    `ObjectIdentityRange`.
    \return `std::optional<ObjectIdentityRange>`
  */
  [[nodiscard]] static auto
  optimize_ranges(SnmpRequestType type, //!< SNMP request type.
                  std::optional<std::vector<ObjectIdentityRange>>
                      ranges //!< Parameters to optimize.
                  ) -> std::optional<std::vector<ObjectIdentityRange>>;

public:
  /*!
    \exception std::invalid_argument `GET_REQUEST` contains non-point
    `ObjectIdentityRange`.
    \exception std::invalid_argument `oids` is empty.
    \exception std::invalid_argument `oids` contains an OID that is a root of
    another (ambiguous root OIDs).
   */
  SnmpRequest(
      SnmpRequestType type,             //!< SNMP request type.
      std::string host,                 //!< Target host.
      Community community,              //!< SNMP community.
      std::vector<ObjectIdentity> oids, //!< Sequence of OIDs to collect.
      std::optional<std::vector<ObjectIdentityRange>> const
          &ranges, //!< Optional sequence of OID ranges to restrict collection
                   //!< on (appended to each OID).
      std::optional<std::string>,  //!< Optional request ID.
      std::optional<Config> config //!< SNMP configuration.
  );

  INLINE_CONST_GETTER(SnmpRequest, type);
  INLINE_CONST_GETTER(SnmpRequest, host);
  INLINE_CONST_GETTER(SnmpRequest, community);
  INLINE_CONST_GETTER(SnmpRequest, oids);
  INLINE_CONST_GETTER(SnmpRequest, ranges);
  INLINE_CONST_GETTER(SnmpRequest, req_id);
  INLINE_CONST_GETTER(SnmpRequest, config);
  REPR(SnmpRequest);
};

/*!
  Compare two `SnmpRequest`.

  \return `bool`
*/
[[nodiscard]] inline auto
operator==(SnmpRequest const &lhs, //!< Left-hand side object to compare.
           SnmpRequest const &rhs  //!< Right-hand side object to compare.
           ) -> bool {
  return (lhs.get_type() == rhs.get_type()) &&
         (lhs.get_host() == rhs.get_host()) &&
         (lhs.get_community() == rhs.get_community()) &&
         (lhs.get_oids() == rhs.get_oids()) &&
         (lhs.get_ranges() == rhs.get_ranges()) &&
         (lhs.get_req_id() == rhs.get_req_id()) &&
         (lhs.get_config() == rhs.get_config());
}

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`
*/
[[nodiscard]] inline auto
attr_to_string(SnmpRequest::SnmpRequestType const &type //!< SNMP request type.
               ) -> std::string {
  std::string string;
  switch (type) {
  case SnmpRequest::GET_REQUEST:
    string = "GET_REQUEST";
    break;
  case SnmpRequest::WALK_REQUEST:
    string = "WALK_REQUEST";
    break;
  }
  return string;
}

/*!
  SNMP error.
*/
class SnmpError {
public:
  /*!
    SNMP error types.
  */
  enum SnmpErrorType {
    SESSION_ERROR = 0,          //!< Session creation error.
    CREATE_REQUEST_PDU_ERROR,   //!< Allocation error creating request PDU.
    SEND_ERROR,                 //!< Request send error.
    BAD_RESPONSE_PDU_ERROR,     //!< Bad PDU error.
    TIMEOUT_ERROR,              //!< Request timed out error.
    ASYNC_PROBE_ERROR,          //!< Async Probe error.
    TRANSPORT_DISCONNECT_ERROR, //!< Transport disconnect error.
    CREATE_RESPONSE_PDU_ERROR,  //!< Allocation error creating response PDU.
    VALUE_WARNING //!< Response variable binding contains an END_OF_MIB_VIEW,
                  //!< NO_SUCH_INSTANCE, or NO_SUCH_OBJECT value.  The values
                  //!< are discarded from the results and only recorded as an
                  //!< error.
  };

private:
  SnmpErrorType type;                    //!< Type of error.
  SnmpRequest request;                   //!< Request instance for this error.
  std::optional<ssize_t> sys_errno;      //!< System error code.
  std::optional<ssize_t> snmp_errno;     //!< SNMP error code.
  std::optional<ssize_t> err_stat;       //!< Error status.
  std::optional<ssize_t> err_index;      //!< Error index.
  std::optional<ObjectIdentity> err_oid; //!< Related OID.
  std::optional<std::string> message;    //!< Error message.

public:
  SnmpError(SnmpErrorType const type, //!< Type of error.
            SnmpRequest request,      //!< Request instance for this error.
            std::optional<ssize_t> const sys_errno,  //!< System error code.
            std::optional<ssize_t> const snmp_errno, //!< SNMP error code.
            std::optional<ssize_t> const err_stat,   //!< Error status.
            std::optional<ssize_t> const err_index,  //!< Error index.
            std::optional<ObjectIdentity> err_oid,   //!< Related OID.
            std::optional<std::string> message       //!< Error message.
            )
      : type(type), request(std::move(request)), sys_errno(sys_errno),
        snmp_errno(snmp_errno), err_stat(err_stat), err_index(err_index),
        err_oid(std::move(err_oid)), message(std::move(message)) {}

  INLINE_CONST_GETTER(SnmpError, type);
  INLINE_CONST_GETTER(SnmpError, request);
  INLINE_CONST_GETTER(SnmpError, sys_errno);
  INLINE_CONST_GETTER(SnmpError, snmp_errno);
  INLINE_CONST_GETTER(SnmpError, err_stat);
  INLINE_CONST_GETTER(SnmpError, err_index);
  INLINE_CONST_GETTER(SnmpError, err_oid);
  INLINE_CONST_GETTER(SnmpError, message);
  REPR(SnmpError);
};

/*!
  Compare two `SnmpError`.

  \return `bool`
*/
[[nodiscard]] inline auto
operator==(SnmpError const &lhs, //!< Left-hand side object to compare
           SnmpError const &rhs  //!< Right-hand side object to compare
           ) -> bool {
  return (lhs.get_type() == rhs.get_type()) &&
         (lhs.get_request() == rhs.get_request()) &&
         (lhs.get_sys_errno() == rhs.get_sys_errno()) &&
         (lhs.get_snmp_errno() == rhs.get_snmp_errno()) &&
         (lhs.get_err_stat() == rhs.get_err_stat()) &&
         (lhs.get_err_index() == rhs.get_err_index()) &&
         (lhs.get_err_oid() == rhs.get_err_oid()) &&
         (lhs.get_message() == rhs.get_message());
}

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`
*/
[[nodiscard]] inline auto
attr_to_string(SnmpError::SnmpErrorType const &type //!< SNMP error type.
               ) -> std::string {
  std::string string;
  switch (type) {
  case SnmpError::SESSION_ERROR:
    string = "SESSION_ERROR";
    break;
  case SnmpError::CREATE_REQUEST_PDU_ERROR:
    string = "CREATE_REQUEST_PDU_ERROR";
    break;
  case SnmpError::SEND_ERROR:
    string = "SEND_ERROR";
    break;
  case SnmpError::BAD_RESPONSE_PDU_ERROR:
    string = "BAD_RESPONSE_PDU_ERROR";
    break;
  case SnmpError::TIMEOUT_ERROR:
    string = "TIMEOUT_ERROR";
    break;
  case SnmpError::ASYNC_PROBE_ERROR:
    string = "ASYNC_PROBE_ERROR";
    break;
  case SnmpError::TRANSPORT_DISCONNECT_ERROR:
    string = "TRANSPORT_DISCONNECT_ERROR";
    break;
  case SnmpError::CREATE_RESPONSE_PDU_ERROR:
    string = "CREATE_RESPONSE_PDU_ERROR";
    break;
  case SnmpError::VALUE_WARNING:
    string = "VALUE_WARNING";
    break;
  }
  return string;
}

/*!
  SNMP response.
*/
class SnmpResponse {
public:
  /*!
    SNMP response types.
  */
  enum SnmpResponseType {
    SUCCESSFUL = 0,   //!< Request was successful.
    DONE_WITH_ERRORS, //!< Request was successful with errors.
    FAILED,           //!< Request failed.
  };

private:
  SnmpResponseType type;                         //!< SNMP response type.
  SnmpRequest request;                           //!< SNMP request.
  std::shared_ptr<std::vector<uint8_t>> results; //!< SNMP results.
  std::vector<SnmpError> errors;                 //!< Collected errors.

public:
  /*!
    Copy argument constructor (for python).
  */
  SnmpResponse(SnmpResponseType type,               //!< SNMP response type.
               SnmpRequest request,                 //!< SNMP request.
               std::vector<uint8_t> const &results, //!< Raw SNMP results.
               std::vector<SnmpError> const &errors //!< Collected errors.
               )
      : type(type), request(std::move(request)),
        results(std::make_shared<std::vector<uint8_t>>(results)),
        errors(std::vector<SnmpError>(errors)) {}

  /*!
    Shared argument constructor (for internal use).
  */
  SnmpResponse(
      SnmpResponseType type,                         //!< SNMP response type.
      SnmpRequest request,                           //!< SNMP request.
      std::shared_ptr<std::vector<uint8_t>> results, //!< Raw SNMP results.
      std::vector<SnmpError> errors                  //!< Collected errors.
      )
      : type(type), request(std::move(request)), results(std::move(results)),
        errors(std::move(errors)) {}

  INLINE_CONST_GETTER(SnmpResponse, type);
  INLINE_CONST_GETTER(SnmpResponse, request);
  INLINE_CONST_GETTER(SnmpResponse, results);
  INLINE_CONST_GETTER(SnmpResponse, errors);
  REPR(SnmpResponse);
};

/*!
  Compare two `SnmpResponse`.

  \return `bool`
*/
[[nodiscard]] inline auto
operator==(SnmpResponse const &lhs, //!< Left-hand side object to compare
           SnmpResponse const &rhs  //!< Right-hand side object to compare
           ) -> bool {
  return (lhs.get_type() == rhs.get_type()) &&
         (lhs.get_request() == rhs.get_request()) &&
         (*(lhs.get_results()) == *(rhs.get_results())) &&
         (lhs.get_errors() == rhs.get_errors());
}

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`
*/
[[nodiscard]] inline auto attr_to_string(
    SnmpResponse::SnmpResponseType const &type //!< SNMP response type.
    ) -> std::string {
  std::string string;
  switch (type) {
  case SnmpResponse::SUCCESSFUL:
    string = "SUCCESSFUL";
    break;
  case SnmpResponse::DONE_WITH_ERRORS:
    string = "DONE_WITH_ERRORS";
    break;
  case SnmpResponse::FAILED:
    string = "FAILED";
    break;
  }
  return string;
}

} // namespace snmp_stream

#endif
