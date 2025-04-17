
#include <frame_types.h>
#include<mutex>
#include<condition_variable>
#include<queue>
class Buffer {
private:
	int size;
	std::mutex frame_mutex;
	std::condition_variable frame_cv;
	std::queue<RawFrame*> frame_queue;

	int next_idx = 0;
	std::condition_variable index_cv;
	int index = 0;
	int session = 0;
public:
	Buffer() :size(8) {
	};
	~Buffer() {};
	void add(RawFrame* frame) {
		frame_mutex.lock();
		if (frame_queue.size() >= size) {
			frame_queue.front()->release();
			frame_queue.pop();
		}
		frame->acquire();
		frame_queue.push(frame);
		frame_mutex.unlock();
		frame_cv.notify_one();
	}
	FaceRoi* process() {
		std::unique_lock<std::mutex> lock(frame_mutex);
		frame_cv.wait(lock, [this] {return !frame_queue.empty(); });
		auto frame = frame_queue.front();
		frame_queue.pop();
		return new FaceRoi{ 0,0,0,0,{},frame,index++,session,TK_UNFINISH };
	}

	bool is_valid(FaceRoi* roi) {
		if (roi != nullptr && roi->session == session)
			return true;
		return false;
	}


	void reset() {
		std::unique_lock<std::mutex> lock(frame_mutex);
		session++;
		while (!frame_queue.empty()) {
			frame_queue.front()->release();
			frame_queue.pop();
		}
	}
};


