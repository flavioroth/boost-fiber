//
// echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//               2013      Oliver Kowalke
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/fiber/all.hpp>
#include "round_robin.hpp"
#include "spawn.hpp"
#include "yield.hpp"

using boost::asio::ip::tcp;

const int max_length = 1024;

typedef boost::shared_ptr< tcp::socket > socket_ptr;

void session( socket_ptr sock) {
    try {
        std::cout << "handler request" << std::endl;
        for (;;) {
            char data[max_length];
            boost::system::error_code ec;
            std::size_t length = sock->async_read_some(
                    boost::asio::buffer( data),
                    boost::fibers::asio::yield[ec]);
            if ( ec == boost::asio::error::eof) {
                break; //connection closed cleanly by peer
            } else if ( ec) {
                throw boost::system::system_error( ec); //some other error
            }
            boost::asio::async_write(
                    * sock,
                    boost::asio::buffer( data, length),
                    boost::fibers::asio::yield[ec]);
            if ( ec == boost::asio::error::eof) {
                break; //connection closed cleanly by peer
            } else if ( ec) {
                throw boost::system::system_error( ec); //some other error
            }
        }
    } catch ( std::exception const& e) {
        std::cerr << "Exception in fiber: " << e.what() << "\n";
    }
}

void server( boost::asio::io_service & io_service, unsigned short port) {
    tcp::acceptor a( io_service, tcp::endpoint( tcp::v4(), port) );
    for (;;) {
        socket_ptr socket( new tcp::socket( io_service) );
        boost::system::error_code ec;
        std::cout << "wait for accept" << std::endl;
        a.async_accept(
                * socket,
                boost::fibers::asio::yield[ec]);
        std::cout << "accepted" << std::endl;
        if ( ! ec) {
            boost::fibers::asio::spawn(
                    io_service,
                    boost::bind( session, socket) );
        }
    }
}

int main( int argc, char* argv[]) {
    try {
        if ( 2 != argc) {
            std::cerr << "Usage: echo_server <port>\n";
            return EXIT_FAILURE;
        }
        boost::asio::io_service io_service;
        boost::fibers::use_scheduling_algorithm< boost::fibers::asio::round_robin >( io_service);
        boost::fibers::asio::spawn(
            io_service,
            boost::bind( server, boost::ref( io_service), std::atoi( argv[1]) ) );
        io_service.run();
        return EXIT_SUCCESS;
    } catch ( std::exception const& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return EXIT_FAILURE;
}
