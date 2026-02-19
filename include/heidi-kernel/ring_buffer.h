#pragma once

#include <vector>
#include <mutex>

namespace heidi {

template <typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) : capacity_(capacity), buffer_(capacity) {}

    void push(const T& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_[head_] = item;
        head_ = (head_ + 1) % capacity_;
        if (full_) {
            tail_ = (tail_ + 1) % capacity_;
        }
        if (head_ == tail_) {
            full_ = true;
        }
    }

    std::vector<T> last_n(size_t n) const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<T> result;
        size_t count = size_unlocked();
        size_t to_fetch = std::min(n, count);

        result.reserve(to_fetch);
        for (size_t i = 0; i < to_fetch; ++i) {
            size_t idx = (head_ + capacity_ - to_fetch + i) % capacity_;
            result.push_back(buffer_[idx]);
        }
        return result;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_unlocked();
    }

private:
    size_t size_unlocked() const {
        if (full_) return capacity_;
        if (head_ >= tail_) return head_ - tail_;
        return capacity_ + head_ - tail_;
    }

    size_t capacity_;
    std::vector<T> buffer_;
    size_t head_ = 0;
    size_t tail_ = 0;
    bool full_ = false;
    mutable std::mutex mutex_;
};

} // namespace heidi
