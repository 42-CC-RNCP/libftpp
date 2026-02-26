// network/impl/reactor/epoll_reactor.hpp
#pragma once
#include "network/contracts/reactor.hpp"
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>

class EpollReactor : public IReactor
{
public:
    static constexpr int MAX_EVENTS = 64;

    EpollReactor() { epoll_fd_ = epoll_create1(0); }

    ~EpollReactor() override
    {
        if (epoll_fd_ != -1) {
            ::close(epoll_fd_);
        }
    }

    void add(int fd, IoEvent events, IoCallback cb) override
    {
        callbacks_[fd] = std::move(cb);

        struct epoll_event ev;
        ev.events = _toEpollEvents(events);
        ev.data.fd = fd;
        if (::epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
            throw std::runtime_error("epoll_ctl ADD failed");
        }
    }

    void modify(int fd, IoEvent events) override
    {
        struct epoll_event ev;
        ev.events = _toEpollEvents(events);
        ev.data.fd = fd;
        if (::epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &ev) == -1) {
            throw std::runtime_error("epoll_ctl MOD failed");
        }
    }

    void remove(int fd) override
    {
        if (::epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr) == -1) {
            throw std::runtime_error("epoll_ctl DEL failed");
        }
        callbacks_.erase(fd);
    }

    int poll(int timeout_ms = -1) override
    {
        std::array<struct epoll_event, 64> events;
        int nfds =
            ::epoll_wait(epoll_fd_, events.data(), events.size(), timeout_ms);

        if (nfds == -1) {
            if (errno == EINTR) {
                return 0; // interrupted by signal, not an error
            }
            throw std::runtime_error("epoll_wait failed");
        }

        for (int n = 0; n < nfds; ++n) {
            int fd = events[n].data.fd;
            uint32_t ev = events[n].events;

            auto it = callbacks_.find(fd);
            if (it != callbacks_.end()) {
                IoEvent io_ev = _fromEpollEvents(ev);
                it->second(fd, io_ev);
            }
        }
        return nfds;
    }

private:
    uint32_t _toEpollEvents(IoEvent e)
    {
        uint32_t ev = 0;

        if (static_cast<int>(e) & static_cast<int>(IoEvent::Readable)) {
            ev |= EPOLLIN;
        }
        if (static_cast<int>(e) & static_cast<int>(IoEvent::Writable)) {
            ev |= EPOLLOUT;
        }
        return ev;
    }

    IoEvent _fromEpollEvents(uint32_t e)
    {
        int ev = 0;

        if (e & EPOLLIN) {
            ev |= static_cast<int>(IoEvent::Readable);
        }
        if (e & EPOLLOUT) {
            ev |= static_cast<int>(IoEvent::Writable);
        }
        if (e & EPOLLERR) {
            ev |= static_cast<int>(IoEvent::Error);
        }
        if (e & EPOLLHUP) {
            ev |= static_cast<int>(IoEvent::Closed);
        }
        return static_cast<IoEvent>(ev);
    }

    int epoll_fd_;
    std::unordered_map<int /* fd */, IoCallback> callbacks_;
};
