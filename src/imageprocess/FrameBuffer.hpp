
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

	std::mutex output_mutex;
	std::condition_variable output_cv;
	int next_idx = 0;
	std::condition_variable index_cv;
	std::queue<FaceRoi*> output_queue;
	int index = 0;
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
		return new FaceRoi{ 0,0,0,0,{},frame,index++,TRACKING_UNF };
	}

	void put(FaceRoi* face_roi) {
		std::unique_lock<std::mutex> lock(output_mutex);
		while (output_queue.size() >= size || next_idx != face_roi->index) {
			if (output_queue.size() >= size) {
				output_cv.wait(lock);
			}
			else {
				index_cv.wait(lock);
			}
		}
		output_queue.push(face_roi);
		next_idx++;
		lock.unlock();
		index_cv.notify_all();
	}

	std::vector<FaceRoi*> get() { //return list
		std::unique_lock<std::mutex> lock(output_mutex);
		std::vector<FaceRoi*> output_vec;
		while (!output_queue.empty()) {
			output_vec.push_back(output_queue.front());
			output_queue.pop();
		}
		lock.unlock();
		if (output_vec.size() != 0)
			output_cv.notify_all();
		return output_vec;
	}
};


