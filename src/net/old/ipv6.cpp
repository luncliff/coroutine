// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
// ---------------------------------------------------------------------------
#include "./net.h"

namespace net
{
namespace ipv6
{
    using address = ::in6_addr;
bool is_unspecified(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_UNSPECIFIED(&a) == TRUE;
}
bool is_multicast(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_MULTICAST(&a) == TRUE;
}
bool is_linklocal(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_LINKLOCAL(&a) == TRUE;
}
bool is_sitelocal(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_SITELOCAL(&a) == TRUE;
}
bool is_v4mapped(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_V4MAPPED(&a) == TRUE;
}
bool is_v4compatible(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_V4COMPAT(&a) == TRUE;
}
bool is_mc_nodelocal(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_MC_NODELOCAL(&a) == TRUE;
}
bool is_mc_linklocal(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_MC_LINKLOCAL(&a) == TRUE;
}
bool is_mc_sitelocal(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_MC_SITELOCAL(&a) == TRUE;
}
bool is_mc_orglocal(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_MC_ORGLOCAL(&a) == TRUE;
}
bool is_mc_global(const ipv6::address &a) noexcept
{
  return ::IN6_IS_ADDR_MC_GLOBAL(&a) == TRUE;
}

bool ston(const char *str, ipv6::address &node) noexcept
{
  int res = ::inet_pton(AF_INET6, str, &node);
  return res > 0;
  //// Not valid string
  //if (res == 0)
  //    return std::make_error_code(std::errc::invalid_argument);
  //// Failed
  //if (res < 0)
}

bool ntos64(const ipv6::address &node, char *str) noexcept
{
  return ::inet_ntop(AF_INET6,
                     &node, str, INET6_ADDRSTRLEN) != nullptr;
}

} // ipv6

bool operator==(const ipv6::address &lhs, const ipv6::address &rhs) noexcept
{
	using namespace std;
	return 0 == memcmp(addressof(lhs), addressof(rhs), sizeof(ipv6::address));
}
bool operator!=(const ipv6::address &lhs, const ipv6::address &rhs) noexcept
{
	return !(lhs == rhs);
}
} // net
