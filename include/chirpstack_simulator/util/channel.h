//
// Created by chungphb on 10/6/21.
//

#pragma once

#include <list>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace chirpstack_simulator {

template <typename T>
struct channel {
public:
    void close() {
        std::unique_lock<std::mutex> lock{_mutex};
        _closed = true;
        _cond_var.notify_all();
    }

    bool is_closed() {
        std::unique_lock<std::mutex> lock{_mutex};
        return _closed;
    }

    void put(const T& item) {
        std::unique_lock<std::mutex> lock{_mutex};
        if (_closed) {
            throw std::runtime_error("channel: put to closed channel");
        }
        _queue.push_back(item);
        _cond_var.notify_one();
    }

    bool get(T& item, bool wait = true) {
        std::unique_lock<std::mutex> lock{_mutex};
        if (wait) {
            _cond_var.wait(lock, [this] {
                return _closed || !_queue.empty();
            });
        }
        if (_queue.empty()) {
            return false;
        }
        item = _queue.front();
        _queue.pop_front();
        return true;
    }

private:
    std::list<T> _queue;
    std::mutex _mutex;
    std::condition_variable _cond_var;
    bool _closed = false;
};

}