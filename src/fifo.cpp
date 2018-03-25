/**
 * Copyright (c) 2014 - 2016 Tolga Cakir <tolga@cevel.net>
 *
 * This source file is part of Sidewinder daemon and is distributed under the
 * MIT License. For more information, see LICENSE file.
 */

#include <csignal>
#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

#include <fifo.hpp>

int Fifo::enable(std::string output) {
	output_ = output;

	// ignore SIGPIPE, else program terminates on unsuccessful write()
	struct sigaction ignore;
	ignore.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &ignore, nullptr);

	if (mkfifo(output_.c_str(), 0644)) {
		std::cerr << "Error creating FIFO." << std::endl;

		return 1;
	}

	std::cerr << "FIFO: " << output << " has been created." << std::endl
		  << "Waiting for user to open it." << std::endl;
	fd_ = open(output_.c_str(), O_WRONLY);

	if (fd_ < 0) {
		if (errno != EINTR) { //EINTR means stopped by abort signal
			std::cerr << "Can't open FIFO for writing." << std::endl;
		}
		return 1;
	}
	return 0;
}

void Fifo::disable() {
	if (fd_) {
		close(fd_);
		fd_ = -1;
		unlink(output_.c_str());
	}
}

int Fifo::reopen() {
    fd_ = open(output_.c_str(), O_WRONLY | O_NONBLOCK);
    if (fd_ != -1) {
        int flags = fcntl(fd_, F_GETFL);
        flags &= ~O_NONBLOCK;
        fcntl(fd_, F_SETFL, flags);
    }
    return fd_;
}

void Fifo::output(std::vector<unsigned char> *buffer) {
	if (fd_ == -1) {
        if (reopen() == -1) {
		    return;
        }
	}

	if (write(fd_, buffer->data(), buffer->size()) == -1) {
        perror("fifo write");
        close(fd_);
        fd_ = -1;
    }
}

Fifo::Fifo() {
	fd_ = -1;
}

Fifo::~Fifo() {
	disable();
}
