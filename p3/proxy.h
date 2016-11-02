#ifndef P3_PROXY_H_H
#define P3_PROXY_H_H

#include <cstdlib>
#include <iostream>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "config_parser.h"

#include <ctime>
#include <random>

using namespace boost::asio;
typedef boost::system::error_code error_code;

const size_t BUF_SIZE = 1024;


class Connection : public boost::enable_shared_from_this<Connection>, boost::noncopyable
{
    Connection(io_service& ios) : server_socket(ios), client_socket(ios), is_active(false) {}

public:
    static boost::shared_ptr<Connection> instance(io_service& ios)
    {
        boost::shared_ptr<Connection> inst(new Connection(ios));
        return inst;
    }

    ip::tcp::socket& get_server_socket() const { return server_socket; }
    ip::tcp::socket& get_client_socket() const { return client_socket; }

    void start(const std::string& server_ip, unsigned short server_port);
    void stop(const error_code& error);

    ~Connection();

private:
    void handle_connect_server(const error_code& error);

    void handle_read_from_server(const error_code& error, size_t size);
    void handle_write_to_server(const error_code& error);

    void handle_read_from_client(const error_code& error, size_t size);
    void handle_write_to_client(const error_code& error);

    void do_write_to_server(size_t size);
    void do_read_from_server();

    void do_write_to_client(size_t size);
    void do_read_from_client();

private:
    ip::tcp::socket server_socket;
    ip::tcp::socket client_socket;

    char server_buffer[BUF_SIZE];
    char client_buffer[BUF_SIZE];

    bool is_active;
};


class ProxyPortListener
{
    ProxyPortListener(io_service& ios_, unsigned short port, std::vector<ParserConfigFile::pair_addr>& servers_);

public:
    static std::shared_ptr<ProxyPortListener> instance(
            io_service& ios_, unsigned short port, std::vector<ParserConfigFile::pair_addr>& servers_)
    {
        std::shared_ptr<ProxyPortListener> inst(new ProxyPortListener(ios_, port, servers_));
        return inst;
    }

private:
    void handle_accept(boost::shared_ptr<Connection> connection, const error_code & err);

    void do_accept_connection(boost::shared_ptr<Connection> connection);

private:
    io_service& ios;
    ip::tcp::acceptor acceptor_;
    std::vector<ParserConfigFile::pair_addr>& servers;
};


class Proxy
{
    typedef std::shared_ptr<ProxyPortListener> port_ptr;
public:
    Proxy(const std::string& config_);
    void run() { ios.run(); }

private:
    io_service ios;
    std::vector<port_ptr> ports;
    ParserConfigFile config;
};

#endif //P3_PROXY_H_H
