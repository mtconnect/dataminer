#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <iostream>
#include <cctype>
#include <regex>

#include "httpreader.h"

namespace beast = boost::beast; // from <boost/beast.hpp>
namespace http = beast::http;   // from <boost/beast/http.hpp>

HttpReader::HttpReader()
{
    m_stream = nullptr;
    m_isSSL = false;
    m_lastConnectionTime = 0;
}

HttpReader::~HttpReader()
{
    close();
}

bool HttpReader::connect()
{
    try {
        if (m_scheme.compare("https") == 0)
            connectSSL();
        else {

            // These objects perform our I/O
            tcp::resolver resolver(m_ioc);
            m_stream = new tcp::socket(m_ioc);

            // Look up the domain name
            auto const results = resolver.resolve(m_host, m_port);

            boost::asio::connect(*m_stream, results.begin(), results.end());
        }
    }
    catch (exception &e)
    {
        std::cerr << "[" << m_host << ":" << m_port << "] " << e.what() << endl;
        return false;
    }

    boost::system::error_code ec;
    boost::asio::socket_base::keep_alive option(true);
    m_stream->set_option(option, ec);

    m_lastConnectionTime = time(nullptr);
    return true;
}


bool HttpReader::connectSSL()
{
    // The SSL context is required, and holds certificates
    ssl::context ctx(ssl::context::tlsv12_client);

    // Verify the remote server's certificate
    ctx.set_verify_mode(ssl::verify_none);

    // These objects perform our I/O
    tcp::resolver resolver(m_ioc);
    ssl::stream<tcp::socket> *stream = new ssl::stream<tcp::socket>(m_ioc, ctx);

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if(!SSL_set_tlsext_host_name(stream->native_handle(), m_host.c_str()))
    {
        beast::error_code ec{static_cast<int>(::ERR_get_error()), net::error::get_ssl_category()};
        throw beast::system_error{ec};
    }

    // Look up the domain name
    auto const results = resolver.resolve(m_host, m_port);

    // Make the connection on the IP address we get from a lookup
    boost::asio::connect(stream->next_layer(), results.begin(), results.end());

    // Perform the SSL handshake
    stream->handshake(ssl::stream_base::client);

    m_stream = (tcp::socket *)stream;
    return true;
}

void HttpReader::close()
{
    if (m_stream != nullptr)
    {
        try {
            beast::error_code ec;
            m_stream->shutdown(tcp::socket::shutdown_both, ec);
        }
        catch (std::exception& e)
        {
            cerr << e.what() << endl;
        }

        if (m_isSSL)
            delete (ssl::stream<tcp::socket> *)m_stream;
        else
            delete m_stream;

        m_stream = nullptr;

    }
    m_lastConnectionTime = 0;
}

string HttpReader::read()
{
    if (m_stream == nullptr)
    {
        // try to reconnect
        if (!connect())
            return "";
    }

    // Declare a container to hold the response
    http::response<http::string_body> res;

    try {

        // Set up an HTTP GET request message
        http::request<http::string_body> req{http::verb::get, "/"+m_path + m_query, 11};
        req.set(http::field::host, m_host);
        req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        if (m_isSSL)
            http::write(*(ssl::stream<tcp::socket> *)m_stream, req);
        else
            http::write(*m_stream, req);

        // Receive the HTTP response
        if (m_isSSL)
            http::read(*(ssl::stream<tcp::socket> *)m_stream, m_buffer, res);
        else
            http::read(*m_stream, m_buffer, res);

        return res.body().data();
    }
    catch(std::exception const& e)
    {
        std::cerr << "[" << m_host << ":" << m_port << "] Error: " << e.what() << std::endl;

        string respdata = res.body().data();

        // try reconnect
        close();

        time_t last = m_lastConnectionTime;
        if (!connect())
            std::cerr <<
                "[" << m_host << ":" << m_port << "] Fail to reconnect to MT agent!" << std::endl;
        else {
            std::cerr <<
                "[" << m_host << ":" << m_port << "] Reconnect to MT agent!"  << std::endl;

            // avoid repeat read errors if reconnect happens within 5 seconds
            if (m_lastConnectionTime - last > 5)
                return read();
        }
    }
    return "";
}

static const char* SCHEME_REGEX   = "((http[s]?)://)?";  // match http or https before the ://
static const char* USER_REGEX     = "(([^@/:\\s]+)@)?";  // match anything other than @ / : or whitespace before the ending @
static const char* HOST_REGEX     = "([^@/:\\s]+)";      // mandatory. match anything other than @ / : or whitespace
static const char* PORT_REGEX     = "(:([0-9]{1,5}))?";  // after the : match 1 to 5 digits
static const char* PATH_REGEX     = "(/[^:#?\\s]*)?";    // after the / match anything other than : # ? or whitespace
static const char* QUERY_REGEX    = "(\\?(([^?;&#=]+=[^?;&#=]+)([;|&]([^?;&#=]+=[^?;&#=]+))*))?"; // after the ? match any number of x=y pairs, seperated by & or ;
static const char* FRAGMENT_REGEX = "(#([^#\\s]*))?";    // after the # match anything other than # or whitespace

static const std::regex regExpr(std::string("^")
    + SCHEME_REGEX + USER_REGEX
    + HOST_REGEX + PORT_REGEX
    + PATH_REGEX + QUERY_REGEX
    + FRAGMENT_REGEX + "$");

bool HttpReader::parseUri(const string &uri)
{
    std::smatch matchResults;
    if (std::regex_match(uri.cbegin(), uri.cend(), matchResults, regExpr))
    {
        m_scheme.assign(matchResults[2].first, matchResults[2].second);
        boost::algorithm::to_lower(m_scheme);

        m_user.assign(matchResults[4].first, matchResults[4].second);
        m_host.assign(matchResults[5].first, matchResults[5].second);
        m_port.assign(matchResults[7].first, matchResults[7].second);
        m_path.assign(matchResults[8].first, matchResults[8].second);
        m_query.assign(matchResults[10].first, matchResults[10].second);
        m_fragment.assign(matchResults[15].first, matchResults[15].second);

        if (m_port.length() == 0)
        {
            if (m_scheme.compare("https") == 0)
            {
                m_port = "443";
                m_isSSL = true;
            }
            else
                m_port = "80";
        }
        return true;
    }

    return false;
}
