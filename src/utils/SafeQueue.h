#ifndef _SAFE_QUEUE_H_
#define _SAFE_QUEUE_H_

#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>

template <typename T>
class MPSCQueue {
public:
	MPSCQueue(size_t capacity) : capacity_(capacity) {}

	// Non-blocking write: always succeeds, discards the oldest element if the queue is full.
	void write(const T& item) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (queue_.size() == capacity_) {
			queue_.pop_front();  // Discard the oldest element
		}
		queue_.push_back(item);
		cv_.notify_one();  // Notify consumer if it's waiting
	}

	// Blocking read: waits for an item if the queue is empty.
	T read(int ms = -1) {
		std::unique_lock<std::mutex> lock(mutex_);
		if (wait_for_valid_with_lock(ms)) {
			T item = queue_.front();
			queue_.pop_front();
			return item;
		}
		else {
			return T();
		}
	}

	bool wait_for_valid(int ms = -1) {
		std::lock_guard<std::mutex> lock(mutex_);
		return wait_for_valid_with_lock(ms);
	}


	size_t size() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return queue_.size();
	}

private:
	bool wait_for_valid_with_lock(int ms = 0) {
		if (ms == 0) {
			if (queue_.empty()) {
				return false;
			}
			return true;
		}
		else {
			if (ms < 0) {
				std::unique_lock<std::mutex> lock(mutex_);
				cv_.wait(lock, [this]() { return !queue_.empty(); });
				return true;
			}
			else {
				std::unique_lock<std::mutex> lock(mutex_);
				return cv_.wait_for(lock, std::chrono::milliseconds(ms), [this]() { return !queue_.empty(); });
			}
		}
	}
private:
	size_t capacity_;
	std::deque<T> queue_;
	mutable std::mutex mutex_;
	std::condition_variable cv_;
};

template<class T>
class SafeQueue {

	std::queue<T*> q;

	std::mutex mtx;
	std::condition_variable cv;
	std::condition_variable cv_producer;

	std::condition_variable sync_wait;
	bool finish_processing = false;
	int sync_counter = 0;
	std::function<void(T*)> delete_function;
	int max_size;


	void DecreaseSyncCounter() {
		if (--sync_counter == 0) {
			sync_wait.notify_one();
		}
	}

public:

	typedef typename std::queue<T>::size_type size_type;

	SafeQueue(int max_size = -1, std::function<void(T*)> delete_function = (T * t){}) :delete_function(delete_function), max_size(max_size) {}

	~SafeQueue() {
		Finish();
	}

	bool Produce(T* item) {

		std::lock_guard<std::mutex> lock(mtx);
		if (finish_processing)
			return false;
		if (max_size > 0 && q.size() >= max_size) {
			delete_function(q.front());
			q.pop();
		}
		q.push(item);
		cv.notify_one();
		return true;
	}
	bool ProduceWait(T* item) {

		std::lock_guard<std::mutex> lock(mtx);


		sync_counter++;
		cv_producer.wait(lock, [&] {
			return !(q.size() >= max_size || finish_processing);
			});
		if (q.size() >= max_size) {
			DecreaseSyncCounter();
			return false;
		}

		q.push(std::move(item));
		cv.notify_one();

		DecreaseSyncCounter();
		return true;
	}

	size_type Size() {

		std::lock_guard<std::mutex> lock(mtx);

		return q.size();

	}

	[[nodiscard]]
	T* Consume() {

		std::lock_guard<std::mutex> lock(mtx);

		if (q.empty()) {
			return NULL;
		}

		item = q.front();
		q.pop();
		cv_producer.notify_one();

		return item;

	}

	[[nodiscard]]
	T* ConsumeWait() {

		std::unique_lock<std::mutex> lock(mtx);

		sync_counter++;

		cv.wait(lock, [&] {
			return !q.empty() || finish_processing;
			});

		if (q.empty()) {
			DecreaseSyncCounter();
			return NULL;
		}

		item = q.front();
		q.pop();
		cv_producer.notify_one();

		DecreaseSyncCounter();
		return item;

	}

	void Finish() {

		std::unique_lock<std::mutex> lock(mtx);

		finish_processing = true;
		cv.notify_all();
		cv_producer.notify_all();

		sync_wait.wait(lock, [&]() {
			return sync_counter == 0;
			});

		finish_processing = false;

	}

};
#endif