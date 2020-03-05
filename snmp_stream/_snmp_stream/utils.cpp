// snmp_stream/_snmp_stream/utils.cpp

#include "utils.hpp"

namespace snmp_stream {

auto oid_to_string(uint64_t const *oid, size_t oid_size) -> std::string {
  if (oid_size == 0) {
    return "";
  }
  return "." + join(oid, oid_size, ".");
}

auto oid_to_string(std::vector<uint64_t> const &oid) -> std::string {
  if (oid.empty()) {
    return "";
  }
  return "." + join(oid.begin(), oid.end(), ".");
}

} // namespace snmp_stream
