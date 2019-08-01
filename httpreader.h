#ifndef HTTPREADER_H
#define HTTPREADER_H

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <string>

namespace beast = boost::beast;     // from <boost/beast.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;    // from <boost/asio.hpp>
namespace ssl = net::ssl;       // from <boost/asio/ssl.hpp>

using namespace std;
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

class HttpReader
{
private:
    string httpAddress;

    string m_scheme;
    string m_user;
    string m_host;
    string m_port;
    string m_path;
    string m_query;
    string m_fragment;
    bool m_isSSL;

    tcp::socket *m_stream;
    time_t  m_lastConnectionTime;
    beast::flat_buffer m_buffer;
    net::io_context m_ioc;

    bool connectSSL();
    string readSSL();

public:
    HttpReader();
    ~HttpReader();

    void setQuery(string query) { m_query = query; }
    bool parseUri(const string &uri);
    bool connect();
    void close();
    string read();
};


#endif // HTTPREADER_H
