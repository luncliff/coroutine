// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
//  Note
//      Coroutine based network programming
//
//      Asynchronous Socket support with coroutine over Windows API
//       - Overlapped Callback
//       - I/O Completion Port (Proactor pattern)
//
//  To Do
//      More implementation + Test
//      - RFC 3540
//      - RFC 3168
//
// ---------------------------------------------------------------------------
#ifndef _MAGIC_NET_H_
#define _MAGIC_NET_H_

#include <magic/linkable.h>
#include <magic/coroutine.hpp>

#include <cstdint>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h> // System API

#include <WinSock2.h> // Windows Socket API
#include <ws2tcpip.h>
#include <ws2def.h>
#include <in6addr.h>

#pragma comment(lib, "Ws2_32.lib")

namespace net
{
namespace stdex = std::experimental;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using i32 = std::int32_t;
using error = std::uint32_t;

//
//// - Note
////      Well known ports
//enum port : u16
//{
//	zero = 0,
//	tcp_mux = 1, // TCP port service multiplexer
//	echo = IPPORT_ECHO,
//	discard = IPPORT_DISCARD,
//	sys_stat = IPPORT_SYSTAT,
//	daytime = IPPORT_DAYTIME,
//	netstat = IPPORT_NETSTAT,
//	msp = IPPORT_MSP,		  // Message send protocol
//	chargen = IPPORT_CHARGEN, // ttytst source
//	ftp_data = IPPORT_FTP_DATA,
//	ftp = IPPORT_FTP,
//	ssh = 22,
//	telnet = IPPORT_TELNET,
//	//smtp = IPPPORT_SMTP,
//	time = IPPORT_TIMESERVER,
//	name = IPPORT_NAMESERVER,
//	dns = 53, // Domain Name Server
//	whois = IPPORT_WHOIS,
//	http = 80,			  // HTTP
//	rtelnet = 107,		  // Remote telnet
//	ntp = 123,			  // Network Time Protocol
//	netbios_ns = 137,	 // NETBIOS Name Service
//	netbios_dgm = 138,	// NETBIOS Datagram Service
//	netbios_ssn = 139,	// NETBIOS session service
//	https = IPPORT_HTTPS, // Secure HTTP
//	nntps = 563,
//	ldaps = 636,
//	ftps_data = 989,
//	telnets = 992,
//	imaps = 993,
//	ircs = 994,
//	pop3s = IPPORT_POP3,
//};
//
//// - Note
////      Protocol Family
//// - See Also
////      <in.h>
//enum class family : u16
//{
//	inet = PF_INET,   // IPv4
//	inet6 = PF_INET6, // IPv6
//
//	//apple       = PF_APPLETALK,
//	//netbios     = AF_NETBIOS,
//	//irDA        = AF_IRDA,
//	//bluetooth   = AF_BTH,
//};
//
//// - Note
////      Type of socket
//// - See Also
////      <WinSock2.h>
//enum class category
//{
//	zero,
//	stream = SOCK_STREAM,	  // Stream
//	dgram = SOCK_DGRAM,		   // Datagram
//	raw = SOCK_RAW,			   // Raw
//	rdm = SOCK_RDM,			   // Reliable Multicast
//	seqpacket = SOCK_SEQPACKET // Sequenced Packet Stream
//};
//
//// - See Also
////      <in.h> / <Ws2ipdef.h>
//enum class proto
//{
//	tcp = IPPROTO_TCP,
//	udp = IPPROTO_UDP,
//	raw = IPPROTO_RAW,
//	ipv4 = IPPROTO_IP,
//	icmp4 = IPPROTO_ICMP,
//	igmp = IPPROTO_IGMP,
//	ipv6 = IPPROTO_IPV6,
//	icmp6 = IPPROTO_ICMPV6,
//};

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

// - Note
//      Buffer is NOT a resource.
//      It just provide a view, without owning a memory.
struct _INTERFACE_ buffer final : public WSABUF
{
};
static_assert(sizeof(buffer) == sizeof(WSABUF));

// - Note
//      Option setter template
// - See Also
//      `setsockopt()`
template <int LEVEL, int OPTION, typename ArgType>
struct setter
{
  protected:
	ArgType arg{};

  public:
	explicit setter(ArgType opt = ArgType{}) noexcept : arg{opt} {}

	bool set(SOCKET sd) noexcept
	{
		// if error confirmed, store it
		return ::setsockopt(sd, LEVEL, OPTION,
							(const char *)&arg, sizeof(ArgType)) == 0;
	}
};

// - Note
//      Option getter template
// - See Also
//      `getsockopt()`
template <int LEVEL, int OPTION, typename RetType>
struct getter
{
  public:
	auto get(SOCKET sd) noexcept
	{
		RetType opt{};
		int len = sizeof(RetType);

		::getsockopt(sd, LEVEL, OPTION,
					 (char *)&opt, &len);
		return opt;
	}
};

// - Note
//      System API structure for asynchronous I/O + library extension
// - See Also
//      `OVERLAPPED`
struct _INTERFACE_ io_work : public OVERLAPPED
{
	// User's custom data
	LPVOID tag;

  public:
	// Provided at bind operation
	void key(UINT_PTR) noexcept;
	UINT_PTR key() const noexcept;

	// I/O result
	void bytes(uint32_t) noexcept;
	uint32_t bytes() const noexcept;

	// I/O error
	void error(uint32_t) noexcept;
	uint32_t error() const noexcept;
};

/*
// - Note
//      Windows I/O Completion Port
class iocp final
{
    HANDLE cport = INVALID_HANDLE_VALUE;

  public:
    // - Note
    //      Creates an I/O completion port.
    //      - max : Max thread
    // - See Also
    //      `CreateIoCompletionPort`
    explicit iocp(uint32_t max = 1) noexcept;
    ~iocp() noexcept;

  public:
    // - Note
    //      Bind the handle with I/O Completion Port.
    //      Key will be recorded when the handle's operation is done
    // - Return
    //      If fail, Error code
    // - See Also
    //      `CreateIoCompletionPort`
    uint32_t bind(HANDLE h, uint64_t key) noexcept;

    // - Note
    //      Post I/O Result
    // - Return
    //      If fail, Error code
    // - See Also
    //      `PostQueuedCompletionStatus`
    uint32_t post(io_work &cb) noexcept;

    // - Note
    //      Get I/O Result. Return `nullptr` if failed
    // - Return
    //      non-null: pointer to control block
    // - See Also
    //      `GetQueuedCompletionStatus`
    // - Caution
    //      If `GetQueuedCompletionStatus` is failed, return `nullptr`
    //      In the case, `GetLastError` will return its reason
    io_work *get(uint32_t timeout = 0) noexcept;

    // - Note
    //      Check if the handle is valid
    operator bool() const noexcept;
};
*/

namespace ipv6
{
using address = ::in6_addr;

_INTERFACE_ bool is_unspecified(const ipv6::address &) noexcept;
_INTERFACE_ bool is_multicast(const ipv6::address &) noexcept;
_INTERFACE_ bool is_linklocal(const ipv6::address &) noexcept;
_INTERFACE_ bool is_sitelocal(const ipv6::address &) noexcept;
_INTERFACE_ bool is_v4mapped(const ipv6::address &) noexcept;
_INTERFACE_ bool is_v4compatible(const ipv6::address &) noexcept;
_INTERFACE_ bool is_mc_nodelocal(const ipv6::address &) noexcept;
_INTERFACE_ bool is_mc_linklocal(const ipv6::address &) noexcept;
_INTERFACE_ bool is_mc_sitelocal(const ipv6::address &) noexcept;
_INTERFACE_ bool is_mc_orglocal(const ipv6::address &) noexcept;
_INTERFACE_ bool is_mc_global(const ipv6::address &) noexcept;

// - Note
//      IPv6 address-host conversion
//      string >> binary
// - See Also
//      `inet_pton`
// - Return
//      success or fail
_INTERFACE_ bool ston(const char *str, address &node) noexcept;

// - Note
//      IPv6 host-address conversion
//      binary >> string
// - See Also
//      `inet_ntop`
// - Return
//      success or fail
_INTERFACE_ bool ntos64(const address &node, char *str) noexcept;

template <size_t N>
static bool ntos(const address &node, char (&str)[N]) noexcept
{
	static_assert(N >= INET6_ADDRSTRLEN,
				  "Buffer is not enough. It must greater than 64 bytes.");
	return ntos64(node, str);
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//  - IPPROTO_IPV6 level : RFC 3493/3542
//      The values should be consistent with the IPv6 equivalents.
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms738574(v=vs.85).aspx
//
//     g  s   Option Value                Description
//    ------  --------------------------- ---------------------------------
//            IPV6_HOPOPTS                Set/get IPv6 hop-by-hop options.
//     x  x   IPV6_HDRINCL                Header is included with data.
//     x  x   IPV6_UNICAST_HOPS           IP unicast hop limit.
//     x  x   IPV6_UNICAST_IF             IP unicast interface.
//     x  x   IPV6_MULTICAST_IF           IP multicast interface.
//     x  x   IPV6_MULTICAST_HOPS         IP multicast hop limit.
//     x  x   IPV6_MULTICAST_LOOP         IP multicast loopback.
//     -  x   IPV6_ADD_MEMBERSHIP         Add an IP group membership.
//     -  x   IPV6_DROP_MEMBERSHIP        Drop an IP group membership.
//            IPV6_DONTFRAG               Don't fragment IP datagrams.
//            IPV6_PKTINFO                Receive packet information.
//     x  x   IPV6_HOPLIMIT               Receive packet hop limit.
//            IPV6_PROTECTION_LEVEL       Set/get IPv6 protection level.
//     x  x   IPV6_RECVIF                 Receive arrival interface.
//            IPV6_RECVDSTADDR            Receive destination address.
//            IPV6_CHECKSUM               Offset to checksum for
//                                        raw IP socket send.
//            IPV6_V6ONLY                 Treat wildcard bind as
//                                        AF_INET6-only.
//            IPV6_IFLIST                 Enable/Disable an interface list.
//            IPV6_ADD_IFLIST             Add an interface list entry.
//            IPV6_DEL_IFLIST             Delete an interface list entry.
//            IPV6_UNICAST_IF             IP unicast interface.
//            IPV6_RTHDR                  Set/get IPv6 routing header.
//            IPV6_GET_IFLIST             Get an interface list.
//            IPV6_RECVRTHDR              Receive the routing header.
//            IPV6_TCLASS                 Packet traffic class.
//            IPV6_RECVTCLASS             Receive packet traffic class.
//            IPV6_ECN                    Receive ECN codepoints
//                                        in the IP header.
//            IPV6_PKTINFO_EX             Receive extended packet info.
//            IPV6_WFP_REDIRECT_RECORDS   WFP's Connection Redirect Records
//            IPV6_WFP_REDIRECT_CONTEXT   WFP's Connection Redirect Context
//

// - Note
//      IPV6_HDRINCL
struct header_include
	: setter<IPPROTO_IPV6, IPV6_HDRINCL, bool>,
	  getter<IPPROTO_IPV6, IPV6_HDRINCL, bool>
{
	using setter<IPPROTO_IPV6, IPV6_HDRINCL, bool>::setter;
};

// - Note
//      IPV6_HOPLIMIT
struct hop_limit
	: setter<IPPROTO_IPV6, IPV6_HOPLIMIT, bool>,
	  getter<IPPROTO_IPV6, IPV6_HOPLIMIT, bool>
{
	using setter<IPPROTO_IPV6, IPV6_HOPLIMIT, bool>::setter;
};

// - Note
//      IPV6_ADD_MEMBERSHIP
struct join_member
	: setter<IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, ::ipv6_mreq>
{
	join_member(const address &host, u32 _if = 0) noexcept
	{
		const ::in6_addr *paddr = reinterpret_cast<const ::in6_addr *>(&host);
		arg.ipv6mr_multiaddr = *paddr;
		arg.ipv6mr_interface = _if;
	}
};

// - Note
//      IPV6_DROP_MEMBERSHIP
struct leave_member
	: setter<IPPROTO_IPV6, IPV6_DROP_MEMBERSHIP, ::ipv6_mreq>
{
	leave_member(const address &host, u32 _if = 0) noexcept
	{
		const ::in6_addr *paddr = reinterpret_cast<const ::in6_addr *>(&host);
		arg.ipv6mr_multiaddr = *paddr;
		arg.ipv6mr_interface = _if;
	}
};

// - Note
//      IPV6_MULTICAST_HOPS
// - Range
//      0x00 : Host(0)
//      0x01 : Subnet(1)
//      0x20 : Site(32)
//      0x40 : Region(64)
//      0x80 : Continent(128)
//      0xFF : Global(255)
struct multicast_hops
	: setter<IPPROTO_IPV6, IPV6_MULTICAST_HOPS, u32>,
	  getter<IPPROTO_IPV6, IPV6_MULTICAST_HOPS, u32>
{
	explicit multicast_hops(u8 ttl) noexcept : setter{ttl}
	{
	}
};

// - Note
//      IPV6_MULTICAST_IF
struct multicast_if
	: setter<IPPROTO_IPV6, IPV6_MULTICAST_IF, u32>,
	  getter<IPPROTO_IPV6, IPV6_MULTICAST_IF, u32>
{
	using setter<IPPROTO_IPV6, IPV6_MULTICAST_IF, u32>::setter;
};

// - Note
//      IPV6_MULTICAST_LOOP
struct multicast_loop
	: setter<IPPROTO_IPV6, IPV6_MULTICAST_LOOP, bool>,
	  getter<IPPROTO_IPV6, IPV6_MULTICAST_LOOP, bool>
{
	using setter<IPPROTO_IPV6, IPV6_MULTICAST_LOOP, bool>::setter;
};

// - Note
//      IPV6_UNICAST_HOPS
struct unicast_hops
	: setter<IPPROTO_IPV6, IPV6_UNICAST_HOPS, u32>,
	  getter<IPPROTO_IPV6, IPV6_UNICAST_HOPS, u32>
{
	explicit unicast_hops(u8 ttl) noexcept : setter{ttl}
	{
	}
};

// - Note
//      IPV6_UNICAST_IF;
struct unicast_if
	: setter<IPPROTO_IPV6, IPV6_UNICAST_IF, u32>,
	  getter<IPPROTO_IPV6, IPV6_UNICAST_IF, u32>
{
	using setter<IPPROTO_IPV6, IPV6_UNICAST_IF, u32>::setter;
};

} // net::ipv6


_INTERFACE_ bool operator==(const ipv6::address &lhs, const ipv6::address &rhs) noexcept;
_INTERFACE_ bool operator!=(const ipv6::address &lhs, const ipv6::address &rhs) noexcept;

// - Note
//      IP v6 address + port : 28 byte
// - Caution
//      Values are stored in Network Byte order.
//      Member functions support Host byte order
// - Reference
//      https://tools.ietf.org/html/rfc5952
struct endpoint final
{
    u16 family;   // AF_INET6.
    u16 port;	 // Transport level port number.
    u32 flowinfo; // IPv6 flow information.
    ipv6::address addr; // IPv6 address.
    u32 scope_id; // Set of interfaces for a scope.
};
static_assert(sizeof(endpoint) == sizeof(::sockaddr_in6));

// - Note
//      Name of current host
// - See Also
//      `gethostname()`
_INTERFACE_ auto host_name() noexcept -> const char *;

// - Note
//      Socket address >> Host name + Service.
//      NI_MAXHOST : 1025
//      NI_MAXSERV : 32
// - See Also
//      `getnameinfo()`
// - Return
//      Success : zero
//      Failure : non-zero error code
_INTERFACE_ error nameof(const endpoint &ep, char *name) noexcept;
_INTERFACE_ error nameof(const endpoint &ep, char *name, char *serv) noexcept;

namespace tcp
{
// - Note
//      Resolve TCP endpoint for `connect()`
// - See Also
//      `getaddrinfo`
//      `freeaddrinfo`
//      `gai_strerror`
_INTERFACE_ auto resolve(const char *name, const char *serv) noexcept
	-> stdex::generator<endpoint>;

_INTERFACE_ auto resolve(const char *name, u16 port) noexcept
	-> stdex::generator<endpoint>;

// - Note
//      Resolve TCP endpoint for `bind()`
// - See Also
//      `getaddrinfo`
//      `freeaddrinfo`
//      `gai_strerror`
_INTERFACE_ auto resolve(const char *serv) noexcept
	-> stdex::generator<endpoint>;

_INTERFACE_ auto resolve(u16 port) noexcept
	-> stdex::generator<endpoint>;

} // net::tcp

namespace udp
{
// - Note
//      Resolve UDP endpoint
// - See Also
//      `getaddrinfo`
//      `freeaddrinfo`
//      `gai_strerror`
_INTERFACE_ auto resolve(const char *name, const char *serv) noexcept
	-> stdex::generator<endpoint>;

_INTERFACE_ auto resolve(const char *name, u16 port) noexcept
	-> stdex::generator<endpoint>;

// - Note
//      Resolve UDP endpoint
// - See Also
//      `getaddrinfo`
//      `freeaddrinfo`
//      `gai_strerror`
_INTERFACE_ auto resolve(const char *serv) noexcept
	-> stdex::generator<endpoint>;

_INTERFACE_ auto resolve(u16 port) noexcept
	-> stdex::generator<endpoint>;

} // net::udp

#pragma warning(disable : 4505)

// - Note
//      Awaitable sender. Extension of OVERLAPPED
//      To reduce type's size, it uses socket descriptor as its key
struct _INTERFACE_ io_send_to : public io_work
{
	const WSABUF *buffers;
	DWORD count;
	const endpoint *to;

  public:
	io_send_to(SOCKET sock, const buffer *base, unsigned length,
			   const endpoint *remote) noexcept;

    constexpr bool ready() const noexcept { return false; }
	void suspend(stdex::coroutine_handle<> rh) noexcept;
	u32 resume() noexcept;

  private:
	static void CALLBACK onSendTo(DWORD dwError, DWORD dwBytes,
								  LPWSAOVERLAPPED pover, DWORD flags);
};
static_assert(sizeof(io_send_to) <= SYSTEM_CACHE_ALIGNMENT_SIZE);

constexpr bool await_ready(const io_send_to &awaitable) noexcept
{
	return awaitable.ready();
}
static decltype(auto) await_suspend(io_send_to &awaitable,
									stdex::coroutine_handle<> rh) noexcept
{
	return awaitable.suspend(rh);
}
static decltype(auto) await_resume(io_send_to &awaitable) noexcept
{
	return awaitable.resume();
}

// - Note
//      Awaitable receiver. Extension of OVERLAPPED
//      To reduce type's size, it uses socket descriptor as its key
struct _INTERFACE_ io_recv_from : public io_work
{
	WSABUF *buffers;
	DWORD count;
	endpoint *from;

  public:
	io_recv_from(SOCKET sock, buffer *base, unsigned length,
				 endpoint *remote) noexcept;

    constexpr bool ready() const noexcept { return false; }
	void suspend(stdex::coroutine_handle<> rh) noexcept;
	u32 resume() noexcept;

  private:
	static void CALLBACK onRecvFrom(DWORD dwError, DWORD dwBytes,
									LPWSAOVERLAPPED pover, DWORD flags);
};
static_assert(sizeof(io_recv_from) <= SYSTEM_CACHE_ALIGNMENT_SIZE);

struct _INTERFACE_ io_send : public io_work
{
	const WSABUF *buffers;
	DWORD count;

  public:
	io_send(SOCKET _sock, const buffer *_base, unsigned _count) noexcept;

	constexpr bool ready() const noexcept { return false; }
	void suspend(stdex::coroutine_handle<> rh) noexcept;
	u32 resume() noexcept;

  private:
	static void CALLBACK onSend(DWORD dwError, DWORD dwBytes,
								LPWSAOVERLAPPED over, DWORD flags);
};

static_assert(sizeof(io_send) <= SYSTEM_CACHE_ALIGNMENT_SIZE);

struct _INTERFACE_ io_recv : public io_work
{
	WSABUF *buffers;
	DWORD count;

  public:
	io_recv(SOCKET _sock, buffer *_base, unsigned _count) noexcept;

    constexpr bool ready() const noexcept { return false; }
	void suspend(stdex::coroutine_handle<> rh) noexcept;
	u32 resume() noexcept;

  private:
	static void CALLBACK onRecv(DWORD dwError, DWORD dwBytes,
								LPWSAOVERLAPPED over, DWORD flags);
};
static_assert(sizeof(io_recv) <= SYSTEM_CACHE_ALIGNMENT_SIZE);

#pragma warning(default : 4505)

//io_send_to& io_send_to(SOCKET sd, const endpoint &remote, const buffer *buf, uint32_t buflen, io_work& work);

// - Note
//      Non-Copyable IPv6 socket
class _INTERFACE_ socket final
{
	SOCKET sd;
  public:
	auto send_to(const endpoint &remote, const buffer &buf) noexcept
		-> io_send_to;
	auto recv_from(endpoint &remote, buffer &buf) noexcept
		-> io_recv_from;

	template <size_t N>
	decltype(auto) send_to(const endpoint &remote, const buffer (&arr)[N]) noexcept
	{
		return io_send_to{sd, arr, N, &remote};
	}

	template <size_t N>
    decltype(auto) recv_from(endpoint &remote, buffer (&arr)[N]) noexcept
	{
		return io_recv_from{sd, arr, N, &remote};
	}

  public:
	// - Note
	//      Set the socket option's value
	template <typename Option>
	auto set(Option &&_opt) noexcept
	{
		return _opt.set(sd);
	}

	// - Note
	//      Get the socket option's value
	template <typename Option>
	auto get(Option &&_opt) const noexcept
	{
		return _opt.get(sd);
	}
};
static_assert(sizeof(socket) == sizeof(::SOCKET));

// - Note
//      TCP connection
class _INTERFACE_ conn final
{
	SOCKET sd = INVALID_SOCKET;
  public:
	auto send(buffer &buf) noexcept -> io_send;
	auto recv(buffer &buf) noexcept -> io_recv;

	template <size_t N>
    decltype(auto) send(buffer (&buffers)[N]) noexcept
	{
		return io_send{sd, buffers, N};
	}

	template <size_t N>
	decltype(auto) recv(buffer (&buffers)[N]) noexcept
	{
		return io_recv{sd, buffers, N};
	}

  public:
	// - Note
	//      Set the socket option's value
	template <typename Option>
	auto set(Option &&_opt) noexcept
	{
		return _opt.set(sd);
	}

	// - Note
	//      Get the socket option's value
	template <typename Option>
	auto get(Option &&_opt) const noexcept
	{
		return _opt.get(sd);
	}
};
static_assert(sizeof(conn) == sizeof(::SOCKET));

_INTERFACE_ SOCKET dial(const endpoint &remote) noexcept;
_INTERFACE_ SOCKET listen(const endpoint &local, int backlog = 24) noexcept;

namespace tcp
{
struct no_delay
	: setter<IPPROTO_TCP, TCP_NODELAY, bool>,
	  getter<IPPROTO_TCP, TCP_NODELAY, bool>
{
	using setter<IPPROTO_TCP, TCP_NODELAY, bool>::setter;
};

} // net::tcp

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----
//  - SOL_SOCKET level : Basic socket options
//      https://msdn.microsoft.com/en-us/library/windows/desktop/ms740532(v=vs.85).aspx
//
//     g  s   Option Value                Description
//    ------  --------------------------- ---------------------------------
//     x  -   SO_ACCEPTCONN               socket has had listen()
//     x  x   SO_REUSEADDR                allow local address reuse
//     -  -   SO_EXCLUSIVEADDRUSE         disallow local address reuse
//     x  x   SO_KEEPALIVE                keep connections alive
//     x  x   SO_DONTROUTE                just use interface addresses
//     x  x   SO_BROADCAST                permit sending of broadcast msgs
//     -  -   SO_USELOOPBACK              bypass hardware when possible
//     x  x   SO_LINGER                   linger on close if data present
//     -  -   SO_DONTLINGER
//     x  x   SO_OOBINLINE                leave received OOB data in line
//     x  x   SO_SNDBUF                   send    buffer size
//     x  x   SO_RCVBUF                   receive buffer size
//     -  -   SO_RCVLOWAT                 receive at least N bytes
//     x  x   SO_SNDTIMEO                 send    timeout
//     x  x   SO_RCVTIMEO                 receive timeout
//     x  -   SO_ERROR                    get error status and clear
//     x  -   SO_TYPE                     get socket type
//

struct accept_conn
	: getter<SOL_SOCKET, SO_ACCEPTCONN, bool>
{
	using getter<SOL_SOCKET, SO_ACCEPTCONN, bool>::getter;
};

struct reuse_address
	: setter<SOL_SOCKET, SO_REUSEADDR, bool>,
	  getter<SOL_SOCKET, SO_REUSEADDR, bool>
{
	using setter<SOL_SOCKET, SO_REUSEADDR, bool>::setter;
};

struct keep_alive
	: setter<SOL_SOCKET, SO_KEEPALIVE, bool>,
	  getter<SOL_SOCKET, SO_KEEPALIVE, bool>
{
	using setter<SOL_SOCKET, SO_KEEPALIVE, bool>::setter;
};

struct no_route
	: setter<SOL_SOCKET, SO_DONTROUTE, bool>,
	  getter<SOL_SOCKET, SO_DONTROUTE, bool>
{
	using setter<SOL_SOCKET, SO_DONTROUTE, bool>::setter;
};

struct broadcast
	: setter<SOL_SOCKET, SO_BROADCAST, bool>,
	  getter<SOL_SOCKET, SO_BROADCAST, bool>
{
	using setter<SOL_SOCKET, SO_BROADCAST, bool>::setter;
};

struct linger_option
	: setter<SOL_SOCKET, SO_LINGER, ::linger>,
	  getter<SOL_SOCKET, SO_LINGER, ::linger>
{
	explicit linger_option(bool on = false,
						   u16 sec = 0) noexcept
	{
		arg.l_onoff = on;
		arg.l_linger = sec;
	}
};

struct out_of_band
	: setter<SOL_SOCKET, SO_OOBINLINE, bool>,
	  getter<SOL_SOCKET, SO_OOBINLINE, bool>
{
	using setter<SOL_SOCKET, SO_OOBINLINE, bool>::setter;
};

struct send_buffer
	: setter<SOL_SOCKET, SO_SNDBUF, u32>,
	  getter<SOL_SOCKET, SO_SNDBUF, u32>
{
	using setter<SOL_SOCKET, SO_SNDBUF, u32>::setter;
};

struct recv_buffer
	: setter<SOL_SOCKET, SO_RCVBUF, u32>,
	  getter<SOL_SOCKET, SO_RCVBUF, u32>
{
	using setter<SOL_SOCKET, SO_RCVBUF, u32>::setter;
};

struct send_timeout
	: setter<SOL_SOCKET, SO_SNDTIMEO, ::timeval>,
	  getter<SOL_SOCKET, SO_SNDTIMEO, ::timeval>
{
	using setter<SOL_SOCKET, SO_SNDTIMEO, ::timeval>::setter;
};

struct recv_timeout
	: setter<SOL_SOCKET, SO_RCVTIMEO, ::timeval>,
	  getter<SOL_SOCKET, SO_RCVTIMEO, ::timeval>
{
	using setter<SOL_SOCKET, SO_RCVTIMEO, ::timeval>::setter;
};

struct error_status
	: getter<SOL_SOCKET, SO_ERROR, i32>
{
	using getter<SOL_SOCKET, SO_ERROR, i32>::getter;
};

struct sock_type
	: getter<SOL_SOCKET, SO_TYPE, i32>
{
	using getter<SOL_SOCKET, SO_TYPE, i32>::getter;
};

} // net

#endif // _MAGIC_NET_H_