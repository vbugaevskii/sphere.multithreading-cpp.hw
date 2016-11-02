#include "proxy.h"

void Connection::start(const std::string& server_ip, unsigned short server_port)
{
    std::cout << "Connection is established!" << std::endl;

    is_active = true;
    server_socket.async_connect(
            ip::tcp::endpoint(ip::address::from_string(server_ip), server_port),
            boost::bind(&Connection::handle_connect_server, shared_from_this(), _1)
    );
}

void Connection::stop(const error_code& error)
{
    if (error)
    {
        std::cout << "Error " << error.value() << ": " << error.message() << std::endl;
        if (error.value() == 2)
        {
            if (server_buffer[0])
                do_write_to_client(strlen(server_buffer));
            if (client_buffer[0])
                do_write_to_server(strlen(client_buffer));
        }
    }

    if (is_active)
    {
        server_socket.close();
        client_socket.close();
        is_active = false;
    }
}

Connection::~Connection()
{
    std::cout << "Connection is terminated!" << std::endl;
    stop(error_code());
}

void Connection::handle_connect_server(const error_code& error)
{
    if (error)
        stop(error);
    else
    {
        do_read_from_client();
        do_read_from_server();
    }
}

void Connection::handle_read_from_server(const error_code& error, size_t size)
{
    if (error)
        stop(error);
    else
    {
        if (size)
            do_write_to_client(size);
        else
            do_read_from_client();
    }
}

void Connection::handle_write_to_server(const error_code& error)
{
    if (error)
        stop(error);
    else
    {
        memset(client_buffer, 0, BUF_SIZE);
        do_read_from_client();
    }
}

void Connection::handle_read_from_client(const error_code& error, size_t size)
{
    if (error)
        stop(error);
    else
    {
        if (size)
            do_write_to_server(size);
        else
            do_read_from_server();
    }
}

void Connection::handle_write_to_client(const error_code& error)
{
    if (error)
        stop(error);
    else
    {
        memset(server_buffer, 0, BUF_SIZE);
        do_read_from_server();
    }
}

void Connection::do_read_from_server()
{
    server_socket.async_receive(
            buffer(server_buffer, BUF_SIZE),
            boost::bind(&Connection::handle_read_from_server, shared_from_this(), _1, _2)
    );
}

void Connection::do_read_from_client()
{
    client_socket.async_receive(
            buffer(client_buffer, BUF_SIZE),
            boost::bind(&Connection::handle_read_from_client, shared_from_this(), _1, _2)
    );
}

void Connection::do_write_to_server(size_t size)
{
    server_socket.async_send(
            buffer(client_buffer, size),
            boost::bind(&Connection::handle_write_to_server, shared_from_this(), _1)
    );
}

void Connection::do_write_to_client(size_t size)
{
    client_socket.async_send(
            buffer(server_buffer, size),
            boost::bind(&Connection::handle_write_to_client, shared_from_this(), _1)
    );
}

ProxyPortListener::ProxyPortListener(
        io_service& ios_, unsigned short port, std::vector<ParserConfigFile::pair_addr>& servers_
) : ios(ios_), acceptor_(ios_, ip::tcp::endpoint(ip::tcp::v4(), port)), servers(servers_)
{
    do_accept_connection(Connection::instance(ios_));
}

void ProxyPortListener::handle_accept(boost::shared_ptr<Connection> connection, const error_code & err)
{
    auto addr = servers[std::rand() % servers.size()];

    connection->start(addr.first, addr.second);
    do_accept_connection(Connection::instance(ios));
}

void ProxyPortListener::do_accept_connection(boost::shared_ptr<Connection> connection)
{
    acceptor_.async_accept(
            connection->get_client_socket(),
            boost::bind(&ProxyPortListener::handle_accept, this, connection, _1)
    );
}

Proxy::Proxy(const std::string& config_) : config(config_)
{
    srand(time(0));

    for (auto port : config.get_ports())
        ports.push_back(ProxyPortListener::instance(ios, port, config[port]));
}