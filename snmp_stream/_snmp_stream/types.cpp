// snmp_stream/_snmp_stream/types.cpp

#include <sstream>

#include <boost/format.hpp>

extern "C" {
#include <debug.h>
}

#include "types.hpp"
#include "utils.hpp"

namespace snmp_stream {

auto ObjectIdentity::repr() const -> std::string {
  return boost::str(boost::format("ObjectIdentity('%1%')") %
                    oid_to_string(*this));
}

ObjectIdentityRange::ObjectIdentityRange(
    std::optional<ObjectIdentity> const &start,
    std::optional<ObjectIdentity> const &stop)
    : start(start.value_or(ObjectIdentity())),
      stop(stop.value_or(ObjectIdentity())) {
  if (!this->start.empty() && !this->stop.empty() && this->start > this->stop) {
    throw std::invalid_argument("'" + oid_to_string(this->start) +
                                "' is not lexicographically less than '" +
                                oid_to_string(this->stop) + "'");
  }
}

auto attr_to_string(ObjectIdentity const &val) -> std::string {
  return attr_to_string(oid_to_string(val));
}

auto ObjectIdentityRange::repr() const -> std::string {
  return boost::str(boost::format("ObjectIdentityRange("
                                  "start=%1%, "
                                  "stop=%2%)") %
                    attr_to_string(start) % attr_to_string(stop));
}

auto operator<(ObjectIdentityRange const &lhs, ObjectIdentityRange const &rhs)
    -> bool {
  DB_TRACELOC(0, "OBJECT_IDENTITY_RANGE_LT: %s, %s\n", lhs.repr().c_str(),
              rhs.repr().c_str());
  if (lhs.get_start().empty()) {
    if (lhs.get_stop().empty()) {
      return !(rhs.get_start().empty() && rhs.get_stop().empty());
    }
    if (rhs.get_start().empty()) {
      if (rhs.get_stop().empty()) {
        return false;
      }
      return lhs.get_stop() > rhs.get_stop();
    }
    return true;
  }
  if (rhs.get_start().empty()) {
    return false;
  }
  if (rhs.get_stop().empty()) {
    return lhs.get_start() < rhs.get_start();
  }
  if (lhs.get_stop().empty()) {
    return lhs.get_start() <= rhs.get_start();
  }
  return lhs.get_start() < rhs.get_start() ||
         (lhs.get_start() == rhs.get_start() &&
          lhs.get_stop() > rhs.get_stop());
}

auto Community::repr() const -> std::string {
  return boost::str(boost::format("Community("
                                  "string=%1%, "
                                  "version=%2%)") %
                    attr_to_string(string) % attr_to_string(version));
}

auto Config::repr() const -> std::string {
  return boost::str(boost::format("Config("
                                  "retries=%1%, "
                                  "timeout=%2%, "
                                  "max_response_var_binds_per_pdu=%3%, "
                                  "max_async_sessions=%4%)") %
                    attr_to_string(get_retries()) %
                    attr_to_string(get_timeout()) %
                    attr_to_string(get_max_response_var_binds_per_pdu()) %
                    attr_to_string(get_max_async_sessions()));
}

auto test_ambiguous_root_oids(std::vector<ObjectIdentity> const &oids)
    -> std::optional<std::tuple<ObjectIdentity, ObjectIdentity>> {
  for (auto it = oids.begin(); it != std::next(oids.end(), -1); ++it) {
    for (auto jt = std::next(it); jt != oids.end(); ++jt) {
      if (snmp_oidtree_compare(it->data(), it->size(), jt->data(),
                               jt->size()) == 0) {
        return std::make_tuple(*it, *jt);
      }
    }
  }
  return std::nullopt;
}

auto SnmpRequest::optimize_ranges(
    SnmpRequestType type,
    std::optional<std::vector<ObjectIdentityRange>> ranges)
    -> std::optional<std::vector<ObjectIdentityRange>> {
  DB_TRACELOC(0, "OPTIMIZE_RANGES: %s\n", attr_to_string(ranges).c_str());
  if (!ranges.has_value() || ranges->empty()) {
    return std::nullopt;
  }
  std::sort(ranges->begin(), ranges->end());
  DB_TRACELOC(0, "OPTIMIZE_RANGES_SORTED: %s\n",
              attr_to_string(ranges).c_str());
  auto it = std::unique(ranges->begin(), ranges->end());
  ranges->resize(std::distance(ranges->begin(), it));
  DB_TRACELOC(0, "OPTIMIZE_RANGES_UNIQUE: %s\n",
              attr_to_string(ranges).c_str());
  std::vector<ObjectIdentityRange> optimized_ranges;
  switch (type) {
  case GET_REQUEST:
    for (auto &&range : *ranges) {
      if (range.get_start() != range.get_stop() || range.get_start().empty()) {
        throw std::invalid_argument("GET_REQUEST only supports point ranges: " +
                                    attr_to_string(range));
      }
    }
    optimized_ranges = *ranges;
    break;
  case WALK_REQUEST:
    for (auto &&range : *ranges) {
      // add the first element
      if (optimized_ranges.empty()) {
        optimized_ranges.push_back(range);
      }
      // ObjectIdentity('') consumes everything
      if (optimized_ranges.back().get_stop().empty()) {
        break;
      }
      // sorting guarantees lhs is bigger
      if (optimized_ranges.back().get_start() == range.get_start()) {
        continue;
      }
      // lhs.stop and rhs.start are guaranteed to have values at this point
      if (optimized_ranges.back().get_stop() >= range.get_start()) {
        optimized_ranges.back() = ObjectIdentityRange(
            optimized_ranges.back().get_start(),
            range.get_stop().empty()
                ? ObjectIdentity()
                : (optimized_ranges.back().get_stop() >= range.get_stop()
                       ? optimized_ranges.back().get_stop()
                       : range.get_stop()));
        continue;
      }
      // no overlap
      optimized_ranges.push_back(range);
    }
    if (optimized_ranges.front().get_start().empty() &&
        optimized_ranges.front().get_stop().empty()) {
      return std::nullopt;
    }
    break;
  }
  return optimized_ranges;
}

SnmpRequest::SnmpRequest(
    SnmpRequestType type, std::string host, Community community,
    std::vector<ObjectIdentity> oids,
    std::optional<std::vector<ObjectIdentityRange>> const &ranges,
    std::optional<std::string> req_id, std::optional<Config> config)
    : type(type), host(std::move(host)), community(std::move(community)),
      oids(std::move(oids)), ranges(optimize_ranges(type, ranges)),
      req_id(std::move(req_id)), config(config) {
  if (this->oids.empty()) {
    throw std::invalid_argument("request missing object identity");
  }
  // test for ambiguous root OIDs
  std::optional<std::tuple<ObjectIdentity, ObjectIdentity>>
      ambiguous_root_oids = test_ambiguous_root_oids(this->oids);
  if (ambiguous_root_oids.has_value()) {
    throw std::invalid_argument(
        "request has ambiguous root OIDs: (" +
        attr_to_string(std::get<0>(*ambiguous_root_oids)) + ", " +
        attr_to_string(std::get<1>(*ambiguous_root_oids)) + ")");
  }
}

auto SnmpRequest::repr() const -> std::string {
  return boost::str(boost::format("SnmpRequest("
                                  "type=%1%, "
                                  "host=%2%, "
                                  "communities=%3%, "
                                  "oids=%4%, "
                                  "ranges=%5%, "
                                  "req_id=%6%, "
                                  "config=%7%)") %
                    attr_to_string(type) % attr_to_string(host) %
                    attr_to_string(community) % attr_to_string(oids) %
                    attr_to_string(ranges) % attr_to_string(req_id) %
                    attr_to_string(config));
}

auto SnmpError::repr() const -> std::string {
  return boost::str(boost::format("SnmpError("
                                  "type=%1%, "
                                  "request=%2%, "
                                  "sys_errno=%3%, "
                                  "snmp_errno=%4%, "
                                  "err_stat=%5%, "
                                  "err_index=%6%, "
                                  "err_oid=%7%, "
                                  "message=%8%)") %
                    attr_to_string(type) % attr_to_string(request) %
                    attr_to_string(sys_errno) % attr_to_string(snmp_errno) %
                    attr_to_string(err_stat) % attr_to_string(err_index) %
                    attr_to_string(err_oid) % attr_to_string(message)

  );
}

auto SnmpResponse::repr() const -> std::string {
  return boost::str(boost::format("SnmpResponse("
                                  "type=%1%, "
                                  "request=%2%, "
                                  "errors=%3%)") %
                    attr_to_string(type) % attr_to_string(request) %
                    attr_to_string(errors));
}

} // namespace snmp_stream
