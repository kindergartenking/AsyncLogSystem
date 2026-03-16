#pragma once
#include "Singleton.h"
#include "ConfigMgr.hpp"
#include <boost/asio.hpp>
#include <string>
#include <mutex>
#include <iostream>
#include <atomic>

class TCPMgr : public Singleton<TCPMgr> {
	friend class Singleton<TCPMgr>;
	

private:
	TCPMgr() : socket_(io_context_), is_connected_(false) {
		try {
			std::string host = ConfigMgr::Inst()["CloudServer"].GetValue("Host");
			std::string portStr = ConfigMgr::Inst()["CloudServer"].GetValue("Port");
			int port = std::stoi(portStr);
			host_ = host;
			port_ = port;
			std::cout << host_ << port_ << std::endl;
			connect(host, port);
		}
		catch (const std::exception& e) {
			std::cerr << "TCPMgr initialization failed: " << e.what() << std::endl;
		}
	}

	~TCPMgr() {
		close();
	}
	

	TCPMgr(const TCPMgr&) = delete;
	TCPMgr& operator=(const TCPMgr&) = delete;

public:
	bool connect(const std::string& host, int port) {
		try {
			boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::make_address(host), port);
			boost::system::error_code ec;
			socket_.connect(endpoint, ec);
			if (ec) {
				std::cerr << "Connect failed: " << ec.message() << std::endl;
				is_connected_.store(false);
				return false;
			}

			boost::asio::socket_base::keep_alive keep_alive_option(true);
			socket_.set_option(keep_alive_option, ec);
			if (ec) {
				std::cerr << "Failed to set keep_alive: " << ec.message() << std::endl;
			}
			is_connected_.store(true);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Connect exception: " << e.what() << std::endl;
			is_connected_.store(false);
			return false;
		}
	}

	bool sendBuffers(const std::vector<boost::asio::const_buffer>& buffers) {
		if (buffers.empty()) return true;

		if (!is_connected_.load()) {
			tryReconnect();
			if (!is_connected_.load()) return false;
		}

		std::lock_guard<std::mutex> lock(mutex_);
		if (!is_connected_.load()) return false;

		try {

			boost::asio::write(socket_, buffers);
			return true;
		}
		catch (const std::exception& e) {
			std::cerr << "Send buffers failed: " << e.what() << std::endl;
			close();
			return false;
		}
	}

	

	void close() {
		boost::system::error_code ec;
		socket_.close(ec);
		if (ec) {
			std::cerr << "Close socket failed: " << ec.message() << std::endl;
		}
		is_connected_.store(false);
	}

private:
	bool tryReconnect() {
		std::lock_guard<std::mutex> lock(reconnect_mutex_);
		if (is_connected_.load()) {
			return true;
		}
		close();
		return connect(host_, port_);
	}

	boost::asio::io_context io_context_;
	boost::asio::ip::tcp::socket socket_;
	std::mutex mutex_;
	std::mutex reconnect_mutex_;
	std::string host_;
	int port_;
	std::atomic<bool> is_connected_;
};
