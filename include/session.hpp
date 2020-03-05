// snmp_stream/_snmp_stream/session.hpp

#ifndef SESSION_HPP
#define SESSION_HPP

#include <deque>
#include <list>

#include "types.hpp"

#define NO_SUCH_OBJECT 128
#define NO_SUCH_INSTANCE 129
#define END_OF_MIB_VIEW 130

namespace snmp_stream {

/*!
  Collection head.  Range is guaranteed to have a start and stop value
  containing (root_oid + start, root_oid + stop).
*/
class CollectionHead {
private:
  size_t root_oid_index;                 //!< Root OID index.
  ObjectIdentity root_oid;               //!< Root OID.
  ObjectIdentityRange range;             //!< Range to be collected.
  std::optional<ObjectIdentity> req_oid; //!< Request OID.
  std::optional<ObjectIdentity>
      last_resp_oid; //!< Last response OID.  This is only set for WALK_REQUST
                     //!< as it is used to determine if this collection node is
                     //!< complete (req_oid.has_value() &&
                     //!< !last_resp_oid.has_value).
  std::shared_ptr<std::vector<uint8_t>> results; //!< Collected results.

public:
  CollectionHead(
      size_t root_oid_index,   //!< Root OID index.
      ObjectIdentity root_oid, //!< Root OID.
      std::optional<ObjectIdentityRange> const
          &range,                                   //!< Range to be collected.
      std::shared_ptr<std::vector<uint8_t>> results //!< Collected results.
  );

  /*!
    Deactivates collection head.
  */
  inline void reset_req_oid() { req_oid = std::nullopt; }

  /*!
    Collection head is considered active.
  */
  [[nodiscard]] inline auto get_next_req_oid() -> ObjectIdentity const & {
    req_oid = last_resp_oid.value_or(root_oid);
    last_resp_oid = std::nullopt;
    return *req_oid;
  }

  INLINE_CONST_GETTER(CollectionHead, root_oid_index);
  INLINE_CONST_GETTER(CollectionHead, root_oid);
  INLINE_CONST_GETTER(CollectionHead, range);
  INLINE_CONST_GETTER(CollectionHead, req_oid);
  INLINE_CONST_GETTER(CollectionHead, last_resp_oid);
  INLINE_CONST_GETTER(CollectionHead, results);

  INLINE_SETTER(CollectionHead, last_resp_oid);

  /*!
    Append a variable binding to this result.
  */
  void append_result(variable_list const &resp_var_bind);
};

/*!
  SNMP session.
*/
class Session {
public:
  /*!
    Session statuses.
  */
  enum SessionStatus {
    IDLE = 0, //!< Session is idle.
    WAIT,     //!< Session is pending a response.
    CLOSED,   //!< Session is closed.
  };

private:
  SessionStatus status;   //!< Session status.
  SnmpRequest request;    //!< SNMP request used to build this session.
  int pdu_type;           //!< PDU type.
  void *_netsnmp_session; /*!<
Opaque pointer to an opened
<a
href="http://net-snmp.sourceforge.net/dev/agent/structsnmp__session.html">
NET-SNMP session
</a>.
*/
  std::shared_ptr<std::vector<uint8_t>> results; //!< Collected results.
  std::list<std::unique_ptr<CollectionHead>>
      collection_heads; //!< Collection nodes.
  bool err_flag;        //!< Marks this session as hitting a critical error.
  std::vector<SnmpError> errors; //!< Collected errors.

  /*!
    Process a response variable binding.
  */
  static void
  process_var_bind(variable_list const &resp_var_bind, //!< Variable binding.
                   Session &session                    //!< Session.
  );

  /*!
    NET-SNMP callback handler.

    \return `int`: Always returns 1.
  */
  static auto process_pdu(int op,           //!< Response opcode.
                          snmp_session *sp, //!< NET-SNMP Session pointer.
                          int reqid,        //!< Request ID.
                          snmp_pdu *pdu,    //!< Response PDU
                          void *magic       //!< Pointer to this `Session`.
                          ) -> int;

public:
  Session(SnmpRequest request //!< SNMP request used to build this session.
  );

  /*!
    Close the session.
  */
  ~Session();

  INLINE_CONST_GETTER(Session, status);
  INLINE_CONST_GETTER(Session, request);

  /*!
    Send the next request PDU.
  */
  void send();

  /*!
    Read the next response PDU.
  */
  void read();

  /*!
    Get the session response.
  */
  [[nodiscard]] auto get_response() -> SnmpResponse;

  /*!
    Append an error.  TODO timestamp

    \return `SnmpError const &`
  */
  inline auto
  append_error(SnmpError::SnmpErrorType type,    //!< Type of error.
               std::optional<size_t> sys_errno,  //!< System error code.
               std::optional<size_t> snmp_errno, //!< SNMP error code.
               std::optional<size_t> err_stat,   //!< Error status.
               std::optional<size_t> err_index,  //!< Error index.
               std::optional<ObjectIdentity> const &err_oid, //!< Related OID.
               std::optional<std::string> const &message     //!< Error message.
               ) -> SnmpError const & {
    return errors.emplace_back(type, request, sys_errno, snmp_errno, err_stat,
                               err_index, err_oid, message);
  }
};

/*!
  Generate attrs (python module) style attribute values.

  \return `std::string`
*/
[[nodiscard]] inline auto
attr_to_string(Session::SessionStatus const &status //!< SNMP session type.
               ) -> std::string {
  std::string string;
  switch (status) {
  case Session::IDLE:
    string = "IDLE";
    break;
  case Session::WAIT:
    string = "WAIT";
    break;
  case Session::CLOSED:
    string = "CLOSED";
    break;
  }
  return string;
}

/*!
  SNMP session.
*/
class SessionManager {
private:
  std::deque<SnmpRequest> pending_requests; //!< Pending requests.
  std::list<Session> async_sessions;        //!< Active sessions.
  Config config; //!< Default configuration.  Guaranteed to have a
                 //!< value for each configuration item.

  /*!
    Get the default configuration.

    \code{.cpp}
    Config(
      3,   // retries
      3,   // timeout
      10,  // max_response_var_binds_per_pdu
      10   // max_async_sessions
    )
    \endcode

    \return `Config`
  */
  [[nodiscard]] inline static auto get_default_config() -> Config const & {
    static Config const config = Config(3, 3, 10, 10);
    return config;
  }

public:
  SessionManager(
      std::optional<Config> const
          &config //!< Replacement default configuration.  `std::nullopt` values
                  //!< will be replaced by the standard default configuration.
      )
      : config(get_default_config() << config){};

  /*!
    Add a new request.
  */
  void add_request(SnmpRequest const &request //!< SNMP request.
  );

  /*!
    Get the number of pending requests.

    \return `size_t`
  */
  [[nodiscard]] inline auto get_pending_requests_count() -> size_t {
    return pending_requests.size();
  }

  /*!
    Get the number of active async sessions.

    \return `size_t`
  */
  [[nodiscard]] auto get_active_async_sessions_count() -> size_t;

  /*!
    Get the maximum number of active sessions allowed with the current requests
    being processed.  Takes the minimum `Config::max_async_sessions` from each
    request.

    \return `size_t`
  */
  [[nodiscard]] auto get_max_async_sessions() -> size_t;

  /*!
    Run all async sessions until one or more completes.

    \return `std::nullopt`: There are no pending or active sessions to process.
    \return `std::optional<std::vector<SnmpRequest>>`: Sequence of completed
    requests.
  */
  [[nodiscard]] auto run() -> std::optional<std::vector<SnmpResponse>>;
};

} // namespace snmp_stream

#endif
