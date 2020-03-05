// snmp_stream/_snmp_stream/module.cpp

#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

extern "C" {
#include <debug.h>
}

#include "session.hpp"
#include "utils.hpp"

#define READONLY_PROPERTY(T, field)                                            \
#field, \
[](T const &obj) { \
  return obj.get_ ## field(); \
}, \
[](T const &obj, \
    decltype(std::declval<T>().get_ ## field()) &val) { \
  throw std::invalid_argument(#T " is read-only: " \
                              "failed to assign " #field "=" + \
                              attr_to_string(val) + " to " + \
                              obj.repr()); \
}

namespace py = pybind11;

namespace snmp_stream {

[[nodiscard]] auto as_ndarray(std::shared_ptr<std::vector<uint8_t>> const &seq)
    -> py::array_t<uint8_t> {
  DB_TRACELOC(0, "ORIG_NDARRAY_USE_COUNT: %d\n", seq.use_count());

  auto *shared_ptr = new std::shared_ptr<std::vector<uint8_t>>(seq);

  auto capsule = py::capsule((void *)shared_ptr, [](void *ptr) {
    auto *shared_ptr =
        reinterpret_cast<std::shared_ptr<std::vector<uint8_t>> *>(ptr);

    DB_TRACELOC(0, "DESTORY_NDARRAY_USE_COUNT: %d\n", shared_ptr->use_count());
    shared_ptr->~shared_ptr();
  });

  DB_TRACELOC(0, "NEW_NDARRAY_USE_COUNT: %d\n", seq.use_count());

  return py::array(seq->size(), seq->data(), capsule);
}

PYBIND11_MODULE(_snmp_stream, m) { // NOLINT

  m.doc() = "Snmp-stream C++ extension.";
  py::class_<ObjectIdentity>(m, "ObjectIdentity", "Object identity.")
      .def(py::init<std::optional<std::vector<oid_t>> const &>(),
           py::arg("oid") = std::nullopt,
           "Initialize an :class:`ObjectIdentity`.")
      .def(
          "is_root_of",
          [](ObjectIdentity *obj, ObjectIdentity const &other) {
            return obj->is_root_of(other);
          },
          py::is_operator(), py::arg("other"),
          "Test if this :class:`ObjectIdentity` is a root of another "
          ":class:`ObjectIdentity`.")
      .def(
          "__add__",
          [](ObjectIdentity *obj, ObjectIdentity const &other) {
            return *obj + other;
          },
          py::is_operator(), py::arg("other"),
          "Add two :class:`ObjectIdentity` together returning a new "
          ":class:`ObjectIdentity`.")
      .def(
          "__eq__",
          [](ObjectIdentity *obj, ObjectIdentity const &other) {
            return *obj == other;
          },
          py::is_operator(), py::arg("other"),
          "Lexicographically compare two :class:`ObjectIdentity`.")
      .def(
          "__lt__",
          [](ObjectIdentity *obj, ObjectIdentity const &other) {
            return *obj < other;
          },
          py::is_operator(), py::arg("other"),
          "Lexicographically compare two :class:`ObjectIdentity`.")
      .def(
          "__le__",
          [](ObjectIdentity *obj, ObjectIdentity const &other) {
            return *obj <= other;
          },
          py::is_operator(), py::arg("other"),
          "Lexicographically compare two :class:`ObjectIdentity`.")
      .def(
          "__gt__",
          [](ObjectIdentity *obj, ObjectIdentity const &other) {
            return *obj > other;
          },
          py::is_operator(), py::arg("other"),
          "Lexicographically compare two :class:`ObjectIdentity`.")
      .def(
          "__ge__",
          [](ObjectIdentity *obj, ObjectIdentity const &other) {
            return *obj >= other;
          },
          py::is_operator(), py::arg("other"),
          "Lexicographically compare two :class:`ObjectIdentity`.")
      .def(
          "__iter__",
          [](ObjectIdentity const &oid) {
            return py::make_iterator(oid.begin(), oid.end());
          },
          py::keep_alive<0, 1>(), "Return an iterator of sub-OIDs (int).")
      .def(
          "__getitem__",
          [](ObjectIdentity const &oid, ssize_t index) { return oid[index]; },
          "Return the number of sub-OIDs.")
      .def(
          "__len__", [](ObjectIdentity const &oid) { return oid.size(); },
          "Return the number of sub-OIDs.")
      .def("__str__",
           [](ObjectIdentity const &oid) { return oid_to_string(oid); })
      .def("__repr__", [](ObjectIdentity const &oid) { return oid.repr(); })
      .def(py::pickle(
          [](ObjectIdentity const &oid) {
            return *static_cast<std::vector<oid_t> const *>(&oid);
          },
          [](std::vector<oid_t> const &v) { return (ObjectIdentity){v}; }));

  py::class_<ObjectIdentityRange>(m, "ObjectIdentityRange",
                                  "Object identity range.")
      .def(py::init<std::optional<ObjectIdentity> const &,
                    std::optional<ObjectIdentity> const &>(),
           py::arg("start"), py::arg("stop"))
      .def(py::init<ObjectIdentity const &>(), py::arg("oid"))
      .def_property(READONLY_PROPERTY(ObjectIdentityRange, start))
      .def_property(READONLY_PROPERTY(ObjectIdentityRange, stop))
      .def(
          "__eq__",
          [](ObjectIdentityRange const &a, ObjectIdentityRange const &b) {
            return a == b;
          },
          py::is_operator())
      .def(
          "__lt__",
          [](ObjectIdentityRange const &a, ObjectIdentityRange const &b) {
            return a < b;
          },
          py::is_operator(), "Compare two `ObjectIdentityRange`.")
      .def(
          "__le__",
          [](ObjectIdentityRange const &a, ObjectIdentityRange const &b) {
            return a <= b;
          },
          py::is_operator(), "Compare two `ObjectIdentityRange`.")
      .def(
          "__gt__",
          [](ObjectIdentityRange const &a, ObjectIdentityRange const &b) {
            return a > b;
          },
          py::is_operator(), "Compare two `ObjectIdentityRange`.")
      .def(
          "__ge__",
          [](ObjectIdentityRange const &a, ObjectIdentityRange const &b) {
            return a >= b;
          },
          py::is_operator(), "Compare two `ObjectIdentityRange`.")
      .def("__str__",
           [](ObjectIdentityRange const &range) { return range.repr(); })
      .def("__repr__",
           [](ObjectIdentityRange const &range) { return range.repr(); })
      .def(py::pickle(
          [](ObjectIdentityRange const &range) {
            // WARNING: make_tuple doesn't place nicely with
            // std::optional<ObjectIdentity>.  It erases the original vector,
            // hence the copy constructor.  pybind11#2111
            return py::make_tuple(
                std::optional<ObjectIdentity>(range.get_start()),
                std::optional<ObjectIdentity>(range.get_stop()));
          },
          [](py::tuple const &t) {
            return (ObjectIdentityRange){
                t[0].cast<std::optional<ObjectIdentity>>(),
                t[1].cast<std::optional<ObjectIdentity>>()};
          }));

  py::class_<Community> community(m, "Community",
                                  "SNMP community string and protocol version");

  community
      .def(py::init<std::string const &, Community::Version>(),
           py::arg("string"), py::arg("version"))
      .def_property(READONLY_PROPERTY(Community, string))
      .def_property(READONLY_PROPERTY(Community, version))
      .def(
          "__eq__",
          [](Community const &a, const Community &b) { return a == b; },
          py::is_operator())
      .def("__str__",
           [](Community const &community) { return community.repr(); })
      .def("__repr__",
           [](Community const &community) { return community.repr(); })
      .def(py::pickle(
          [](Community const &community) {
            return py::make_tuple(community.get_version(),
                                  community.get_string());
          },
          [](py::tuple const &t) {
            return (Community){t[1].cast<std::string>(),
                               t[0].cast<Community::Version>()};
          }));

  py::enum_<Community::Version>(community, "Version", "SNMP protocol versions.")
      .value("V1", Community::V1)
      .value("V2C", Community::V2C)
      .export_values();

  py::class_<Config>(m, "Config", "SNMP configuration.")
      .def(py::init<
               std::optional<ssize_t> const &, std::optional<ssize_t> const &,
               std::optional<size_t> const &, std::optional<size_t> const &>(),
           py::arg("retries") = std::nullopt, py::arg("timeout") = std::nullopt,
           py::arg("max_response_var_binds_per_pdu") = std::nullopt,
           py::arg("max_async_sessions") = std::nullopt)
      .def_property(READONLY_PROPERTY(Config, retries))
      .def_property(READONLY_PROPERTY(Config, timeout))
      .def_property(READONLY_PROPERTY(Config, max_response_var_binds_per_pdu))
      .def_property(READONLY_PROPERTY(Config, max_async_sessions))
      .def(
          "__eq__", [](Config const &a, Config const &b) { return a == b; },
          py::is_operator())
      .def("__str__", [](Config const &config) { return config.repr(); })
      .def("__repr__", [](Config const &config) { return config.repr(); })
      .def(py::pickle(
          [](Config const &config) {
            return py::make_tuple(config.get_retries(), config.get_timeout(),
                                  config.get_max_response_var_binds_per_pdu(),
                                  config.get_max_async_sessions());
          },
          [](py::tuple const &t) {
            return (Config){t[0].cast<std::optional<ssize_t>>(),
                            t[1].cast<std::optional<ssize_t>>(),
                            t[2].cast<std::optional<size_t>>(),
                            t[3].cast<std::optional<size_t>>()};
          }));

  m.def("test_ambiguous_root_oids", &test_ambiguous_root_oids, py::arg("oids"));

  py::class_<SnmpRequest> snmp_request(m, "SnmpRequest", "SNMP request.");

  snmp_request
      .def(py::init<SnmpRequest::SnmpRequestType, std::string &,
                    Community const &, std::vector<ObjectIdentity> const &,
                    std::optional<std::vector<ObjectIdentityRange>> const &,
                    std::optional<std::string> const &,
                    std::optional<Config> const &>(),
           py::arg("type"), py::arg("host"), py::arg("community"),
           py::arg("oids"), py::arg("ranges") = std::nullopt,
           py::arg("req_id") = std::nullopt, py::arg("config") = std::nullopt)
      .def_property(READONLY_PROPERTY(SnmpRequest, type))
      .def_property(READONLY_PROPERTY(SnmpRequest, host))
      .def_property(READONLY_PROPERTY(SnmpRequest, community))
      .def_property(READONLY_PROPERTY(SnmpRequest, oids))
      .def_property(READONLY_PROPERTY(SnmpRequest, ranges))
      .def_property(READONLY_PROPERTY(SnmpRequest, req_id))
      .def_property(READONLY_PROPERTY(SnmpRequest, config))
      .def(
          "__eq__",
          [](SnmpRequest const &a, SnmpRequest const &b) { return a == b; },
          py::is_operator())
      .def("__str__", [](SnmpRequest const &error) { return error.repr(); })
      .def("__repr__", [](SnmpRequest const &error) { return error.repr(); })
      .def(py::pickle(
          [](SnmpRequest const &request) {
            return py::make_tuple(request.get_type(), request.get_host(),
                                  request.get_community(), request.get_oids(),
                                  request.get_ranges(), request.get_req_id(),
                                  request.get_config());
          },
          [](py::tuple const &t) {
            return (SnmpRequest){
                t[0].cast<SnmpRequest::SnmpRequestType>(),
                t[1].cast<std::string>(),
                t[2].cast<Community>(),
                t[3].cast<std::vector<ObjectIdentity>>(),
                t[4].cast<std::optional<std::vector<ObjectIdentityRange>>>(),
                t[5].cast< // NOLINT(readability-magic-numbers)
                    std::optional<std::string>>(),
                t[6].cast< // NOLINT(readability-magic-numbers)
                    std::optional<Config>>()};
          }));

  py::enum_<SnmpRequest::SnmpRequestType>(snmp_request, "SnmpRequestType",
                                          "SNMP request types.")
      .value("GET_REQUEST", SnmpRequest::SnmpRequestType::GET_REQUEST,
             "Doctest")
      .value("WALK_REQUEST", SnmpRequest::SnmpRequestType::WALK_REQUEST)
      .export_values();

  py::class_<SnmpError> snmp_error(m, "SnmpError", "SNMP error.");

  snmp_error
      .def(py::init<
               SnmpError::SnmpErrorType, SnmpRequest const &,
               std::optional<int64_t> const &, std::optional<int64_t> const &,
               std::optional<int64_t> const &, std::optional<int64_t> const &,
               std::optional<ObjectIdentity> const &,
               std::optional<std::string> const &>(),
           py::arg("type"), py::arg("request"),
           py::arg("sys_errno") = std::nullopt,
           py::arg("snmp_errno") = std::nullopt,
           py::arg("err_stat") = std::nullopt,
           py::arg("err_index") = std::nullopt,
           py::arg("err_oid") = std::nullopt, py::arg("message") = std::nullopt)
      .def_property(READONLY_PROPERTY(SnmpError, type))
      .def_property(READONLY_PROPERTY(SnmpError, request))
      .def_property(READONLY_PROPERTY(SnmpError, sys_errno))
      .def_property(READONLY_PROPERTY(SnmpError, snmp_errno))
      .def_property(READONLY_PROPERTY(SnmpError, err_stat))
      .def_property(READONLY_PROPERTY(SnmpError, err_index))
      .def_property(READONLY_PROPERTY(SnmpError, err_oid))
      .def_property(READONLY_PROPERTY(SnmpError, message))
      .def(
          "__eq__",
          [](SnmpError const &a, SnmpError const &b) { return a == b; },
          py::is_operator())
      .def("__str__", [](SnmpError const &error) { return error.repr(); })
      .def("__repr__", [](SnmpError const &error) { return error.repr(); })
      .def(py::pickle(
          [](SnmpError const &error) {
            return py::make_tuple(error.get_type(), error.get_request(),
                                  error.get_sys_errno(), error.get_snmp_errno(),
                                  error.get_err_stat(), error.get_err_index(),
                                  error.get_err_oid(), error.get_message());
          },
          [](py::tuple const &t) {
            return (SnmpError){t[0].cast<SnmpError::SnmpErrorType>(),
                               t[1].cast<SnmpRequest>(),
                               t[2].cast<std::optional<int64_t>>(),
                               t[3].cast<std::optional<int64_t>>(),
                               t[4].cast<std::optional<int64_t>>(),
                               t[5].cast< // NOLINT(readability-magic-numbers)
                                   std::optional<int64_t>>(),
                               t[6].cast< // NOLINT(readability-magic-numbers)
                                   std::optional<ObjectIdentity>>(),
                               t[7].cast< // NOLINT(readability-magic-numbers)
                                   std::optional<std::string>>()};
          }));

  py::enum_<SnmpError::SnmpErrorType>(snmp_error, "SnmpErrorType",
                                      "SNMP error types.")
      .value("SESSION_ERROR", SnmpError::SnmpErrorType::SESSION_ERROR)
      .value("CREATE_REQUEST_PDU_ERROR",
             SnmpError::SnmpErrorType::CREATE_REQUEST_PDU_ERROR)
      .value("SEND_ERROR", SnmpError::SnmpErrorType::SEND_ERROR)
      .value("BAD_RESPONSE_PDU_ERROR",
             SnmpError::SnmpErrorType::BAD_RESPONSE_PDU_ERROR)
      .value("TIMEOUT_ERROR", SnmpError::SnmpErrorType::TIMEOUT_ERROR)
      .value("ASYNC_PROBE_ERROR", SnmpError::SnmpErrorType::ASYNC_PROBE_ERROR)
      .value("TRANSPORT_DISCONNECT_ERROR",
             SnmpError::SnmpErrorType::TRANSPORT_DISCONNECT_ERROR)
      .value("CREATE_RESPONSE_PDU_ERROR",
             SnmpError::SnmpErrorType::CREATE_RESPONSE_PDU_ERROR)
      .value("VALUE_WARNING", SnmpError::SnmpErrorType::VALUE_WARNING)
      .export_values();

  py::class_<SnmpResponse> snmp_response(m, "SnmpResponse", "SNMP response.");

  snmp_response
      .def(py::init<SnmpResponse::SnmpResponseType, SnmpRequest const &,
                    std::vector<uint8_t>, std::vector<SnmpError>>(),
           py::arg("type"), py::arg("request"), py::arg("results"),
           py::arg("errors"))
      .def_property(READONLY_PROPERTY(SnmpResponse, type))
      .def_property(READONLY_PROPERTY(SnmpResponse, request))
      .def_property(
          "results",
          [](SnmpResponse const &obj) { return as_ndarray(obj.get_results()); },
          [](SnmpResponse const &obj, py::array_t<uint8_t> &val) {
            throw std::invalid_argument("SnmpResponse is read-only: "
                                        "failed to assign results=" +
                                        py::repr(val).cast<std::string>() +
                                        " to " + obj.repr());
          })
      .def_property(READONLY_PROPERTY(SnmpResponse, errors))
      .def(
          "__eq__",
          [](SnmpResponse const &a, SnmpResponse const &b) { return a == b; },
          py::is_operator())
      .def("__str__",
           [](SnmpResponse const &response) { return response.repr(); })
      .def("__repr__",
           [](SnmpResponse const &response) { return response.repr(); })
      .def(py::pickle(
          [](SnmpResponse const &response) {
            return py::make_tuple(response.get_type(), response.get_request(),
                                  *response.get_results(),
                                  response.get_errors());
          },
          [](py::tuple const &t) {
            return (SnmpResponse){t[0].cast<SnmpResponse::SnmpResponseType>(),
                                  t[1].cast<SnmpRequest>(),
                                  std::make_shared<std::vector<uint8_t>>(
                                      t[2].cast<std::vector<uint8_t>>()),
                                  t[3].cast<std::vector<SnmpError>>()};
          }));

  py::enum_<SnmpResponse::SnmpResponseType>(snmp_response, "SnmpResponseType",
                                            "SNMP response types.")
      .value("SUCCESSFUL", SnmpResponse::SnmpResponseType::SUCCESSFUL)
      .value("DONE_WITH_ERRORS",
             SnmpResponse::SnmpResponseType::DONE_WITH_ERRORS)
      .value("FAILED", SnmpResponse::SnmpResponseType::FAILED)
      .export_values();

  py::class_<SessionManager>(m, "SessionManager", "SNMP session manager")
      .def(py::init<std::optional<Config> const &>(),
           py::arg("config") = std::nullopt)
      .def("add_request", &SessionManager::add_request)
      .def("run", &SessionManager::run);
}

} // namespace snmp_stream
