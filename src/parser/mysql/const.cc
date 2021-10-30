#include <fmt/format.h>

#include <string>

namespace mysql {
std::string TiDBReleaseVersion = "None";
std::string ServerVersion = fmt::format("5.7.25-TiDB-%s", TiDBReleaseVersion);

}  // namespace mysql