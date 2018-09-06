// ---------------------------------------------------------------------------
//
//  Author
//      Park DongHa     | luncliff@gmail.com
//
// ---------------------------------------------------------------------------
#include <cassert>
#include "net.h"

namespace net
{

socket::socket(category type, proto protocol) noexcept
{
	sd = ::WSASocketW(AF_INET6, (int)type, (int)protocol,
					  nullptr, 0, WSA_FLAG_OVERLAPPED);
}
socket::socket(SOCKET _sd) noexcept : sd{_sd}
{
}

socket::~socket() noexcept
{
	if (sd != INVALID_SOCKET)
		::closesocket(sd);
}

socket::socket(socket &&_rhs) noexcept
{
	std::swap(this->sd, _rhs.sd);
}

socket &socket::operator=(socket &&_rhs) noexcept
{
	std::swap(this->sd, _rhs.sd);
	return *this;
}

error socket::bind(const endpoint &local) noexcept
{
	::bind(sd, reinterpret_cast<const sockaddr *>(&local),
		   sizeof(sockaddr_in6));
	return recent();
}

endpoint socket::local() const noexcept
{
	endpoint host{};
	socklen_t len = sizeof(sockaddr_in6);
	::getsockname(sd, reinterpret_cast<sockaddr *>(&host), &len);
	return host;
}

auto socket::send_to(const endpoint &remote, const buffer &buf) noexcept
	-> io_send_to
{
	return io_send_to{sd, &buf, 1, &remote};
}

auto socket::recv_from(endpoint &remote, buffer &buf) noexcept
	-> io_recv_from
{
	return io_recv_from{sd, &buf, 1, &remote};
}

conn::conn(SOCKET _sd) noexcept : sd{_sd}
{
}

conn::~conn() noexcept
{
	this->close();
}

conn::conn(conn &&rhs) noexcept
{
	std::swap(sd, rhs.sd);
}

conn &conn::operator=(conn &&rhs) noexcept
{
	std::swap(sd, rhs.sd);
	return *this;
}

endpoint conn::local() const noexcept
{
	endpoint host{};
	socklen_t len = sizeof(sockaddr_in6);
	::getsockname(sd, reinterpret_cast<sockaddr *>(&host), &len);
	return host;
}

endpoint conn::remote() const noexcept
{
	endpoint peer{};
	socklen_t len = sizeof(sockaddr_in6);
	::getpeername(sd, reinterpret_cast<sockaddr *>(&peer), &len);
	return peer;
}

void conn::shutdown(int option) noexcept
{
	if (sd != INVALID_SOCKET)
		::shutdown(sd, option);
}

void conn::close() noexcept
{
	if (sd != INVALID_SOCKET)
	{
		::shutdown(sd, SD_BOTH);
		::closesocket(sd);
		sd = INVALID_SOCKET;
	}
}

auto conn::send(buffer &buf) noexcept -> io_send
{
	return io_send{sd, &buf, 1};
}

auto conn::recv(buffer &buf) noexcept -> io_recv
{
	return io_recv{sd, &buf, 1};
}

// ---- ---- ---- ---- ---- ---- ---- ---- ---- ----

accepter::accepter(SOCKET _sd) noexcept : sd{_sd}
{
}

accepter::~accepter() noexcept
{
	::shutdown(sd, SD_BOTH);
	::closesocket(sd);
}

accepter::accepter(accepter &&rhs) noexcept
{
	std::swap(sd, rhs.sd);
}

accepter &accepter::operator=(accepter &&rhs) noexcept
{
	std::swap(sd, rhs.sd);
	return *this;
}

SOCKET accepter::accept() noexcept
{
	// client socket. Inherit overlapped properties
	SOCKET cs = WSAAccept(sd, nullptr, nullptr, nullptr, NULL);
	return cs;
}

void accepter::close() noexcept
{
	closesocket(sd);
	sd = INVALID_SOCKET;
}

endpoint accepter::local() const noexcept
{
	endpoint host{};
	socklen_t len = sizeof(sockaddr_in6);
	::getsockname(sd, reinterpret_cast<sockaddr *>(&host), &len);
	return host;
}

SOCKET dial(const endpoint &remote) noexcept
{
	DWORD group = 0;
	WSAPROTOCOL_INFOW *info = nullptr;

	// TCP stream socket
	SOCKET sd = WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
						   info, group, WSA_FLAG_OVERLAPPED);
	if (sd == INVALID_SOCKET)
		goto Error;

	if (WSAConnect(sd,
				   reinterpret_cast<const sockaddr *>(&remote),
				   sizeof(sockaddr_in6),
				   NULL, NULL, NULL, NULL) == SOCKET_ERROR)
	{
		// Check recent error value. this is ASYNC connect
		if (recent() != WSAEWOULDBLOCK)
			goto Error;
	}

	return sd;
Error:
	error ec = recent();
	::closesocket(sd);   // Close anyway
	WSASetLastError(ec); // Recover error code
	return INVALID_SOCKET;
}

SOCKET listen(const endpoint &local, int backlog) noexcept
{
	DWORD group = 0;
	WSAPROTOCOL_INFOW *info = nullptr;

	// TCP listener socket
	SOCKET sd = ::WSASocketW(AF_INET6, SOCK_STREAM, IPPROTO_TCP,
							 info, group, WSA_FLAG_OVERLAPPED); //
	if (sd == INVALID_SOCKET)
		goto Error;
	if (::bind(sd, reinterpret_cast<const sockaddr *>(&local),
			   sizeof(sockaddr_in6)) == SOCKET_ERROR)
		goto Error;
	if (::listen(sd, backlog) == SOCKET_ERROR)
		goto Error;

	// Sucess
	return sd;
Error:
	error ec = recent();
	::closesocket(sd);   // Close anyway
	WSASetLastError(ec); // Recover error code
	return INVALID_SOCKET;
}

} // net
