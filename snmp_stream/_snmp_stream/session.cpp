// snmp_stream/_snmp_stream/session.cpp

#include <pybind11/pybind11.h>

#include <algorithm>
#include <cinttypes>
#include <cmath>
#include <cstring>
#include <map>

extern "C" {
#include <debug.h>
}

#include "session.hpp"
#include "utils.hpp"

// macro to align to system
#define SYS_ALIGN(x) (((x) + (sizeof(size_t) - 1)) & ~(sizeof(size_t) - 1))

#define HEADER_BYTES 16

namespace py = pybind11;

namespace snmp_stream {

enum class endian {
#ifdef _WIN32
  little = 0,
  big = 1,
  native = little
#else
  little = __ORDER_LITTLE_ENDIAN__,
  big = __ORDER_BIG_ENDIAN__,
  native = __BYTE_ORDER__
#endif
};

CollectionHead::CollectionHead(size_t root_oid_index, ObjectIdentity root_oid,
                               std::optional<ObjectIdentityRange> const &range,
                               std::shared_ptr<std::vector<uint8_t>> results)
    : root_oid_index(root_oid_index), root_oid(std::move(root_oid)),
      req_oid(std::nullopt), last_resp_oid(std::nullopt),
      results(std::move(results)) {
  ObjectIdentity start = this->root_oid;
  ObjectIdentity stop = this->root_oid;
  if (range.has_value()) {
    start.insert(start.end(), range->get_start().begin(),
                 range->get_start().end());
    stop.insert(stop.end(), range->get_stop().begin(), range->get_stop().end());
  }
  this->range = ObjectIdentityRange(start, stop);
  DB_TRACELOC(0, "COLLECTION_HEAD_CREATE: (%s, %s)\n",
              oid_to_string(this->root_oid).c_str(),
              attr_to_string(this->range).c_str());
}

void CollectionHead::append_result(variable_list const &resp_var_bind) {
  // get a timestamp for the response
  time_t timestamp;
  time(&timestamp);

  size_t index_size = resp_var_bind.name_length - root_oid.size();
  size_t resp_var_bind_size = (
      // timestamp
      SYS_ALIGN(sizeof(timestamp)) +
      // root oid index
      SYS_ALIGN(sizeof(root_oid_index)) +
      // value type
      SYS_ALIGN(sizeof(resp_var_bind.type)) +
      // index size
      SYS_ALIGN(sizeof(index_size)) +
      // index
      SYS_ALIGN(index_size * sizeof(oid_t)) +
      // value size
      SYS_ALIGN(sizeof(resp_var_bind.val_len)) +
      // value
      SYS_ALIGN(resp_var_bind.val_len));

  // resize the array
  size_t pos = results->size();
  results->resize(results->size() + SYS_ALIGN(sizeof(resp_var_bind_size)) +
                  resp_var_bind_size);

  // copy the variable binding size
  std::memcpy(&(*results)[pos], &resp_var_bind_size,
              sizeof(resp_var_bind_size));

  // copy the timestamp
  std::memcpy(&(*results)[pos += SYS_ALIGN(sizeof(resp_var_bind_size))],
              &timestamp, sizeof(timestamp));

  // copy the root oid index
  std::memcpy(&(*results)[pos += SYS_ALIGN(sizeof(timestamp))], &root_oid_index,
              sizeof(root_oid_index));

  // copy the value type
  std::memcpy(&(*results)[pos += SYS_ALIGN(sizeof(root_oid_index))],
              &resp_var_bind.type, sizeof(resp_var_bind.type));

  // copy the index size
  std::memcpy(&(*results)[pos += SYS_ALIGN(sizeof(resp_var_bind.type))],
              &index_size, sizeof(index_size));

  // copy the index
  std::memcpy(&(*results)[pos += SYS_ALIGN(sizeof(index_size))],
              resp_var_bind.name + root_oid.size(), index_size * sizeof(oid_t));

  // copy the value_size
  std::memcpy(&(*results)[pos += SYS_ALIGN(index_size * sizeof(oid_t))],
              &resp_var_bind.val_len, sizeof(resp_var_bind.val_len));

  // copy the value
  std::memcpy(&(*results)[pos += SYS_ALIGN(sizeof(resp_var_bind.val_len))],
              resp_var_bind.val.bitstring, resp_var_bind.val_len);
}

void Session::process_var_bind(variable_list const &resp_var_bind,
                               Session &session) {
  static std::map<uint8_t, std::string> WARNING_VALUE_TYPES = {
      {NO_SUCH_OBJECT, "NO_SUCH_OBJECT"},
      {NO_SUCH_INSTANCE, "NO_SUCH_INSTANCE"},
      {END_OF_MIB_VIEW, "END_OF_MIB_VIEW"}};

  auto resp_oid = ObjectIdentity(
      resp_var_bind.name, resp_var_bind.name + resp_var_bind.name_length);

  DB_TRACELOC(0, "SESSION_PROCESS_VAR_BIND_RESPONSE_OID: %d: %s\n",
              resp_var_bind.type, oid_to_string(resp_oid).c_str());

  // Find the collection node for this variable binding.  Note: This is the
  // reason why a request OID cannot be a root of another request OID (ambiguous
  // root OID).
  auto request_type = session.request.get_type();
  auto it = std::find_if(
      session.collection_heads.begin(), session.collection_heads.end(),
      [&request_type,
       &resp_oid](std::unique_ptr<CollectionHead> const &collection_head) {
        // skip if collection head was not active for this PDU
        if (!collection_head->get_req_oid().has_value()) {
          return false;
        }
        if (request_type == SnmpRequest::GET_REQUEST) {
          // for a GET_REQUEST, OID must match the range point
          return collection_head->get_range().get_start() == resp_oid;
        }
        // for a WALK_REQUEST, OID must be between the range inclusive or stop
        // must be a root of the OID
        if (resp_oid >= collection_head->get_range().get_start() &&
            resp_oid <= collection_head->get_range().get_stop()) {
          return true;
        }
        return collection_head->get_range().get_stop().is_root_of(resp_oid);
      });

  switch (session.request.get_type()) {
  case SnmpRequest::GET_REQUEST:
    // If no root OID is found, discard the variable binding and generate an
    // error. This should never happen in a GET request.
    if (it == session.collection_heads.end()) {
      session.append_error(SnmpError::VALUE_WARNING, {}, {}, {},
                           resp_var_bind.index, resp_oid, "root OID not found");
      DB_TRACELOC(0, "SESSION_PROCESS_VAR_BIND_ROOT_OID_NOT_FOUND: %s\n",
                  session.errors.back().repr().c_str());
      session.err_flag = true;
      return;
    }

    // If the response OID does not match the request OID, discard the
    // variable binding and generate an error. This should never happen in a
    // GET request.
    if (resp_oid != *(*it)->get_req_oid()) {
      session.append_error(SnmpError::VALUE_WARNING, {}, {}, {},
                           resp_var_bind.index, resp_oid,
                           "request OID does not match response OID: " +
                               oid_to_string(*(*it)->get_req_oid()));
      DB_TRACELOC(0, "SESSION_PROCESS_VAR_BIND_GET_RESP_NOT_MATCH_REQ: %s\n",
                  session.errors.back().repr().c_str());
      session.err_flag = true;
      return;
    }

    // Test for non-value types and generate an error if matched.
    if (WARNING_VALUE_TYPES.find(resp_var_bind.type) !=
        WARNING_VALUE_TYPES.end()) {
      session.append_error(SnmpError::VALUE_WARNING, {}, {}, {},
                           resp_var_bind.index, resp_oid,
                           WARNING_VALUE_TYPES[resp_var_bind.type]);
      DB_TRACELOC(0, "SESSION_PROCESS_VAR_BIND_VALUE_WARNING: %s\n",
                  session.errors.back().repr().c_str());
      session.err_flag = true;
      return;
    }

    break;
  case SnmpRequest::WALK_REQUEST:
    // If no root OID is found, silently discard the variable binding.  Likely
    // cause for collecting this response is an overrun on a walk from another
    // root OID.
    if (it == session.collection_heads.end()) {
      return;
    }

    if ((*it)->get_last_resp_oid().has_value()) {
      // If the response OID is lexicographically less than or equal to the
      // last response OID, discard the response.  Likely cause for collecting
      // this response is an overrun on a walk from another root OID.
      if (resp_oid <= *(*it)->get_last_resp_oid()) {
        return;
      }
    } else {
      // If the response OID is lexicographically less than the request OID,
      // discard the response.  Likely cause for collecting this response is
      // an overrun on a walk from another root OID.
      if (resp_oid <= *(*it)->get_req_oid()) {
        return;
      }
    }

    DB_TRACELOC(
        0, "SESSION_PROCESS_VAR_BIND_SET_LAST_RESPONSE_OID: %s: %s -> %s\n",
        (*it)->get_range().repr().c_str(),
        oid_to_string((*it)->get_last_resp_oid()).c_str(),
        oid_to_string(resp_oid).c_str());
    (*it)->set_last_resp_oid(resp_oid);
    break;
  }
  DB_TRACELOC(0, "SESSION_PROCESS_VAR_BIND_APPEND_RESULT: %s\n",
              oid_to_string(resp_oid).c_str());
  (*it)->append_result(resp_var_bind);
}

auto Session::process_pdu(int op,
                          snmp_session *sp, // NOLINT(misc-unused-parameters)
                          int reqid,        // NOLINT(misc-unused-parameters)
                          snmp_pdu *pdu, void *magic) -> int {
  auto *session = (Session *)magic;
  DB_TRACELOC(0, "SESSION_PROCESS_PDU: %s\n", session->request.repr().c_str());

  switch (op) {
  case NETSNMP_CALLBACK_OP_RECEIVED_MESSAGE:
    session->status = IDLE;
    DB_TRACELOC(0, "SESSION_PROCESS_PDU_OP_RECEIVED_MESSAGE\n");
    // check that we got a PDU
    if (pdu != nullptr) {
      // check that we got a valid PDU
      if (pdu->command == SNMP_MSG_RESPONSE) {
        // check the PDU doesn't have an error status
        if (pdu->errstat == SNMP_ERR_NOERROR) {
          // add each response variable binding to the results
          for (variable_list *var_bind = pdu->variables; var_bind != nullptr;
               var_bind = var_bind->next_variable) {
            process_var_bind(*var_bind, *session);
          }
        } else {
          // find the variable binding with an error
          variable_list *err_var_bind;
          for (err_var_bind = pdu->variables;
               err_var_bind != nullptr && err_var_bind->index != pdu->errindex;
               err_var_bind = err_var_bind->next_variable) {
          }
          session->append_error(
              SnmpError::BAD_RESPONSE_PDU_ERROR, {}, {}, pdu->errstat,
              pdu->errindex,
              (err_var_bind == nullptr)
                  ? (std::optional<ObjectIdentity>)std::nullopt
                  : (std::optional<ObjectIdentity>){ObjectIdentity(
                        err_var_bind->name,
                        err_var_bind->name + err_var_bind->name_length)},
              std::string(snmp_errstring((int)pdu->errstat)));
          DB_TRACELOC(0, "SESSION_PROCESS_PDU_BAD_RESPONSE_PDU_ERROR: %s\n",
                      session->errors.back().repr().c_str());
          session->status = CLOSED;
          session->err_flag = true;
        }
      } else {
        session->append_error(
            SnmpError::BAD_RESPONSE_PDU_ERROR, {}, SNMPERR_PROTOCOL, {}, {}, {},
            "expected RESPONSE-PDU, got " +
                std::string(snmp_pdu_type(pdu->command)) + "-PDU");
        DB_TRACELOC(0, "SESSION_PROCESS_PDU_BAD_RESPONSE_PDU_ERROR: %s\n",
                    session->errors.back().repr().c_str());
        session->status = CLOSED;
        session->err_flag = true;
      }
    } else {
      session->append_error(SnmpError::CREATE_RESPONSE_PDU_ERROR, {}, {}, {},
                            {}, {},
                            "failed to allocate memory for the response PDU");
      DB_TRACELOC(0, "SESSION_PROCESS_PDU_CREATE_RESPONSE_PDU_ERROR: %s\n",
                  session->errors.back().repr().c_str());
      session->status = CLOSED;
      session->err_flag = true;
    }
    break;
  case NETSNMP_CALLBACK_OP_TIMED_OUT:
    session->append_error(SnmpError::TIMEOUT_ERROR, {}, SNMPERR_TIMEOUT, {}, {},
                          {}, "timeout error");
    DB_TRACELOC(0, "SESSION_PROCESS_PDU_OP_TIMED_OUT: %s\n",
                session->errors.back().repr().c_str());
    session->status = CLOSED;
    session->err_flag = true;
    break;
  case NETSNMP_CALLBACK_OP_SEND_FAILED:
    session->append_error(SnmpError::ASYNC_PROBE_ERROR, {}, {}, {}, {}, {},
                          "async probe error");
    DB_TRACELOC(0, "SESSION_PROCESS_PDU_OP_SEND_FAILED: %s\n",
                session->errors.back().repr().c_str());
    session->status = CLOSED;
    session->err_flag = true;
    break;
  case NETSNMP_CALLBACK_OP_DISCONNECT:
    session->append_error(SnmpError::TRANSPORT_DISCONNECT_ERROR, {},
                          SNMPERR_ABORT, {}, {}, {},
                          "transport disconnect error");
    DB_TRACELOC(0, "SESSION_PROCESS_PDU_OP_DISCONNECT\n: %s\n",
                session->errors.back().repr().c_str());
    session->status = CLOSED;
    session->err_flag = true;
    break;
  case NETSNMP_CALLBACK_OP_RESEND:
    DB_TRACELOC(0, "SESSION_PROCESS_PDU_OP_RESEND\n");
    break;
  }

  // remove collection nodes that had a request but no response
  for (auto it = session->collection_heads.begin();
       it != session->collection_heads.end();) {
    DB_TRACELOC(0, "SESSION_PROCESS_PDU_FINAL_COLLECTION_HEAD: (%s, %s)\n",
                attr_to_string((*it)->get_req_oid()).c_str(),
                attr_to_string((*it)->get_last_resp_oid()).c_str());
    if ((*it)->get_req_oid().has_value()) {
      if (!(*it)->get_last_resp_oid().has_value()) {
        DB_TRACELOC(0, "SESSION_PROCESS_PDU_REMOVE_COLLECTION_HEAD: %s\n",
                    (*it)->get_range().repr().c_str());
        it = session->collection_heads.erase(it);
        continue;
      }
      (*it)->reset_req_oid();
    }
    ++it;
  }

  // close the session once all collection nodes have completed
  if (session->collection_heads.empty()) {
    session->status = CLOSED;
  }

  return 1;
}

Session::Session(SnmpRequest request)
    : request(std::move(request)),
      results(std::make_shared<std::vector<uint8_t>>()) {
  DB_TRACELOC(0, "SESSION_CREATE: %s\n", this->request.repr().c_str());

  switch (this->request.get_type()) {
  case SnmpRequest::GET_REQUEST:
    pdu_type = SNMP_MSG_GET;
    break;
  case SnmpRequest::WALK_REQUEST:
    switch (this->request.get_community().get_version()) {
    case Community::V1:
      pdu_type = SNMP_MSG_GETNEXT;
      break;
    case Community::V2C:
      pdu_type = SNMP_MSG_GETBULK;
      break;
    }
    break;
  }

  // init a NET-SNMP session template
  netsnmp_session session;
  snmp_sess_init(&session);

  // configure the NET-SNMP session template
  session.peername = strdup(this->request.get_host().c_str());
  session.retries = *this->request.get_config()->get_retries();
  session.timeout = *this->request.get_config()->get_timeout() * ONE_SEC;
  session.community =
      (u_char *)strdup(this->request.get_community().get_string().c_str());
  session.community_len = strlen((char *)session.community);
  session.version = this->request.get_community().get_version();

  // open the session
  _netsnmp_session = snmp_sess_open(&session);

  DB_TRACELOC(0, "SESSION_ADDRESS: 0x%zx\n", _netsnmp_session);

  if (_netsnmp_session == nullptr) {
    char *message;
    int sys_errno;
    int snmp_errno;
    snmp_error(&session, &errno, &snmp_errno, &message);
    auto error = append_error(SnmpError::SESSION_ERROR, sys_errno, snmp_errno,
                              {}, {}, {}, std::string(message));
    DB_TRACELOC(0, "SNMP_ERROR: %s\n", error.repr().c_str());
    SNMP_FREE(message);
    SNMP_FREE(session.peername);
    SNMP_FREE(session.community);
    status = CLOSED;
    err_flag = true;
    return;
  }

#ifdef DEBUG
  // DEBUG check not strictly necessary, but linters will complain the ptr is
  // never used
  auto *session_ptr = snmp_sess_session(_netsnmp_session);
  DB_TRACELOC(0, "SESSION_HOSTNAME: %s\n", session_ptr->peername);
  DB_TRACELOC(0, "SESSION_VERSION: %u\n", session_ptr->version);
  DB_TRACELOC(0, "SESSION_COMMUNITY: %s\n", session_ptr->community);
  DB_TRACELOC(0, "SESSION_COMMUNITY_LENGTH: %u\n", session_ptr->community_len);
  DB_TRACELOC(0, "SESSION_RETRIES: %d\n", session_ptr->retries);
  DB_TRACELOC(0, "SESSION_TIMEOUT: %d\n", session_ptr->timeout);
#endif

  status = IDLE;

  // fill the results header
  size_t pos = 0;
  results->resize(HEADER_BYTES);
  // endianess
  if constexpr (endian::native == endian::little) {
    (*results)[0] = 0;
  } else if constexpr (endian::native == endian::big) {
    (*results)[0] = 1;
  } else {
    throw std::runtime_error("endianness could not be detected");
  }
  (*results)[1] = SYS_ALIGN(sizeof(size_t)); // word size
  (*results)[2] = sizeof(oid_t);             // octet size

  // add metadata to the results header
  pos = results->size();
  size_t tmp = this->request.get_req_id().has_value()
                   ? this->request.get_req_id()->size()
                   : 0;
  results->resize(pos + SYS_ALIGN(sizeof(tmp)) + SYS_ALIGN(tmp));
  std::memcpy(&(*results)[pos], &tmp, sizeof(tmp));
  if (this->request.get_req_id().has_value()) {
    std::memcpy(&(*results)[pos + SYS_ALIGN(sizeof(tmp))],
                this->request.get_req_id()->c_str(), tmp);
  }

  // append the number of root OIDs to the results header
  pos = results->size();
  tmp = this->request.get_oids().size();
  results->resize(pos + SYS_ALIGN(sizeof(tmp)));
  std::memcpy(&(*results)[pos], &tmp, sizeof(tmp));

  // create each collection head
  size_t root_oid_index = 0;
  for (auto &&oid : this->request.get_oids()) {
    if (this->request.get_ranges().has_value() &&
        !this->request.get_ranges()->empty()) {
      for (auto &&range : *this->request.get_ranges()) {
        collection_heads.push_back(std::make_unique<CollectionHead>(
            root_oid_index++, oid, range, results));
      }
    } else {
      collection_heads.push_back(std::make_unique<CollectionHead>(
          root_oid_index++, oid, std::nullopt, results));
    }
    // append the root OID to the results header
    pos = results->size();
    tmp = oid.size();
    results->resize(pos + SYS_ALIGN(sizeof(tmp)) +
                    SYS_ALIGN(tmp * sizeof(oid_t)));
    std::memcpy(&(*results)[pos], &tmp, sizeof(tmp));
    std::memcpy(&(*results)[pos + SYS_ALIGN(sizeof(tmp))], oid.data(),
                tmp * sizeof(oid_t));
  }
}

Session::~Session() {
  DB_TRACELOC(0, "SESSION_DESTORY: %s\n", request.repr().c_str());
  if (_netsnmp_session == nullptr) {
    return;
  }
  netsnmp_session *session = snmp_sess_session(_netsnmp_session);
  SNMP_FREE(session->peername);
  SNMP_FREE(session->community);
  snmp_sess_close(_netsnmp_session);
  DB_TRACELOC(0, "SESSION_DESTORY_NETSNMP_SESSION_DELETED: %s\n",
              request.repr().c_str());
}

void Session::send() {
  DB_TRACELOC(0, "SESSION_SEND: %s: %s\n", attr_to_string(status).c_str(),
              request.repr().c_str());

  if (status != Session::IDLE) {
    return;
  }

  // create the request PDU
  netsnmp_pdu *pdu = snmp_pdu_create(pdu_type);

  if (pdu == nullptr) {
    auto error =
        append_error(SnmpError::CREATE_REQUEST_PDU_ERROR, {}, {}, {}, {}, {},
                     "failed to allocate memory for the request PDU");
    DB_TRACELOC(0, "SESSION_SEND_PDU_CREATION_ERROR: %s\n",
                error.repr().c_str());
    status = CLOSED;
    err_flag = true;
    return;
  }

  /*
    Iterate through each of the collection heads filling the PDU with variable
    bindings.  For BULK requests, insert sqrt(max_response_var_binds_per_pdu)
    variable binding into the PDU to collect
    (n^2 < max_response_var_binds_per_pdu) variable bindings. For GET  requests,
    insert max_response_var_binds_per_pdu variable bindings.  Additionally, only
    insert up to the number of collection heads.
  */
  size_t var_bind_count = 0;
  size_t max_response_var_binds_per_pdu =
      *request.get_config()->get_max_response_var_binds_per_pdu();
  netsnmp_variable_list *var_bind;
  while (var_bind_count < (pdu_type == SNMP_MSG_GETBULK
                               ? sqrt(max_response_var_binds_per_pdu)
                               : max_response_var_binds_per_pdu) &&
         var_bind_count < collection_heads.size()) {
    ObjectIdentity const &req_oid =
        collection_heads.front()->get_next_req_oid();
    DB_TRACELOC(0, "SESSION_SEND_ADD_VAR_BIND: '%s'\n",
                oid_to_string(req_oid).c_str());
    var_bind = snmp_add_null_var(pdu, req_oid.data(), req_oid.size());
    if (var_bind == nullptr) {
      auto error = append_error(SnmpError::CREATE_REQUEST_PDU_ERROR, {}, {}, {},
                                {}, req_oid, "failed to add OID to PDU");
      DB_TRACELOC(0, "SESSION_SEND_ADD_VAR_BIND_ERROR: %s\n",
                  error.repr().c_str());
      err_flag = true; // not fatal, but OID will no longer be attempted
      collection_heads.pop_front(); // remove collection head on failure
    } else {
      // rotate collection heads
      collection_heads.splice(collection_heads.end(), collection_heads,
                              collection_heads.begin());
      var_bind_count++;
    }
  }

  if (pdu_type == SNMP_MSG_GETBULK) {
    pdu->non_repeaters = 0;
    pdu->max_repetitions = // should be n^2 < max_response_var_binds_per_pdu
                           // unless there are fewer collection heads
        (ssize_t)(max_response_var_binds_per_pdu / var_bind_count);
  }

  // dispatch the PDU and log on error
  if (snmp_sess_async_send(_netsnmp_session, pdu, process_pdu, this) == 0) {
    char *message;
    int sys_errno;
    int snmp_errno;
    snmp_sess_error(_netsnmp_session, &sys_errno, &snmp_errno, &message);
    auto error = append_error(SnmpError::SEND_ERROR, sys_errno, snmp_errno, {},
                              {}, {}, std::string(message));
    DB_TRACELOC(0, "SESSION_SEND_ASYNC_SEND_ERROR: %s\n", error.repr().c_str());
    snmp_free_pdu(pdu);
    SNMP_FREE(message);
    status = CLOSED;
    err_flag = true;
    return;
  }

  // set the state to waiting
  status = WAIT;
}

void Session::read() {
  DB_TRACELOC(0, "SESSION_READ: %s\n", request.repr().c_str());
  DB_TRACELOC(0, "SESSION_READ_STATUS: %s\n", attr_to_string(status).c_str());

  // check that the session is waiting for data
  if (status != WAIT) {
    return;
  }

  // init a socket set
  fd_set fdset;
  FD_ZERO(&fdset);

  // init the highest numbered socket id + 1 in the set to 0
  int nfds = 0;

  // init a timeout parameter
  struct timeval timeout;

  // init socket reads to be blocking
  int block = NETSNMP_SNMPBLOCK;

  // let NET-SNMP fill all the parameters above for select
  snmp_sess_select_info(_netsnmp_session, &nfds, &fdset, &timeout, &block);

  // make the syscall to select to read the session socket
  int count =
      select(nfds, &fdset, nullptr, nullptr, block != 0 ? nullptr : &timeout);

  // check if the socket is ready to read
  if (count != 0) {
    // read the socket data; this triggers the callback function
    DB_TRACELOC(0, "SESSION_READ_SOCKET_HAS_DATA\n");
    snmp_sess_read(_netsnmp_session, &fdset);
  } else {
    // retry or timeout otherwise
    DB_TRACELOC(0, "SESSION_READ_TIMEOUT_OR_RETRY_SOCKET\n");
    snmp_sess_timeout(_netsnmp_session);
  }
}

auto Session::get_response() -> SnmpResponse {
  return (SnmpResponse){SnmpResponse::SUCCESSFUL, request, results, errors};
};

void SessionManager::add_request(SnmpRequest const &request) {
  DB_TRACELOC(0, "SESSION_MANAGER_ADD_REQUEST: %s\n", request.repr().c_str());
  pending_requests.emplace_back(request.get_type(), request.get_host(),
                                request.get_community(), request.get_oids(),
                                request.get_ranges(), request.get_req_id(),
                                config << request.get_config());
}

auto SessionManager::get_active_async_sessions_count() -> size_t {
  return std::count_if(async_sessions.begin(), async_sessions.end(),
                       [](auto const &session) {
                         return session.get_status() != Session::CLOSED;
                       });
}

auto SessionManager::get_max_async_sessions() -> size_t {
  auto max_async_sessions = std::numeric_limits<size_t>::max();
  for (auto &&session : async_sessions) {
    if (session.get_status() != Session::CLOSED) {
      max_async_sessions = std::min(
          max_async_sessions,
          *session.get_request().get_config()->get_max_async_sessions());
    }
  }
  return max_async_sessions;
}

auto SessionManager::run() -> std::optional<std::vector<SnmpResponse>> {
  DB_TRACELOC(0, "SESSION_MANAGER_RUN: %s\n", config.repr().c_str());
  DB_TRACELOC(0, "SESSION_MANAGER_PRE_PENDING_REQUESTS: %zu\n",
              get_pending_requests_count());
  DB_TRACELOC(0, "SESSION_MANAGER_PRE_ASYNC_SESSIONS: %zu\n",
              get_active_async_sessions_count());

  py::gil_scoped_release release;

  // move pending requests to active until max async sessions is met
  while (
      // check that there are pending requests
      !pending_requests.empty() &&
      // check that adding another session will not exceed the maximum
      // number of async sessions for those already active
      get_active_async_sessions_count() + 1 <=
          std::min(get_max_async_sessions(), *pending_requests.front()
                                                  .get_config()
                                                  ->get_max_async_sessions())) {
    async_sessions.emplace_back(pending_requests.front());
    pending_requests.pop_front();
  }

  DB_TRACELOC(0, "SESSION_MANAGER_POST_PENDING_REQUESTS: %zu\n",
              get_pending_requests_count());
  DB_TRACELOC(0, "SESSION_MANAGER_POST_ASYNC_SESSIONS: %zu\n",
              get_active_async_sessions_count());

  std::vector<SnmpResponse> responses;

  if (!async_sessions.empty()) {
    // perform IO until at least one session has completed
    while (async_sessions.size() == get_active_async_sessions_count()) {
      for (auto &&session : async_sessions) {
        session.send();
      }
      for (auto &&session : async_sessions) {
        session.read();
      }
    }

    // collect results from completed sessions
    for (auto it = async_sessions.begin(); it != async_sessions.end();) {
      if (it->get_status() == Session::CLOSED) {
        responses.push_back(it->get_response());
        it = async_sessions.erase(it);
        --it;
      }
    }
  }

  py::gil_scoped_acquire acquire;

  return responses.empty()
             ? (std::optional<std::vector<SnmpResponse>>)std::nullopt
             : responses;
}

} // namespace snmp_stream
