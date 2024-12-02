#ifndef WINDOW_H
#define WINDOW_H
#include <cstring>
#include <Eigen/Dense>
template<class T> class Window
{
public:
	Window(int, int);
	~Window();
	inline void push_front(const T&);
	inline void shift(const T&);
	inline void reset();
	inline void append(const T*, int);
	inline void append_zero(int);
	T* getHead() const { return frontp; }
	T* getTail() const { return backp; }
	Eigen::Map<Eigen::ArrayX<T>> getArray(int max_size, int tail_ofs = 0) {
		static T ret[1] = { 0 };
		max_size = std::min(window_size, max_size);
		if (size < tail_ofs) {
			return Eigen::Map<Eigen::ArrayX<T>>(ret,1);
		}
		if (size < max_size + tail_ofs) {
			return Eigen::Map<Eigen::ArrayX<T>>(backp -
				size, size - tail_ofs);
		}
		return Eigen::Map<Eigen::ArrayX<T>>(backp -
			max_size - tail_ofs, max_size);
	}

private:
	T* frontp;
	T* backp;
	T* startp;
	T* endp;

	int capicity;
	int window_size;
	bool need_delete;
	unsigned size;
};

template<class T> Window<T>::~Window()
{
	if (need_delete)
		delete[] startp;
}

template<class T> Window<T>::Window(int cap, int window_size)
	: window_size(window_size)
	, need_delete(true)
	, size(0)
{
	if (window_size > cap) {
		capicity = window_size + cap;
	}
	else
		capicity = cap;

	startp = new T[capicity]();
	frontp = startp;

	backp = startp + window_size;
	endp = startp + capicity;

}

template<class T> inline void Window<T>::push_front(const T& t)
{
	if (endp == backp) {
		memmove(startp, frontp, window_size * sizeof(T));
		frontp = startp;
		backp = startp + window_size;
		endp = startp + capicity;
	}
	backp[0] = t;
	backp += 1;
	size++;
}

template<class T> inline void Window<T>::shift(const T& t) {
	push_front(t);
	if (backp - frontp <= window_size)
		return;
	frontp += 1;
}
template<class T> inline void Window<T>::append(const T* t, int size) {
	if (size > capicity)
		return;
	if (backp + size >= endp) {
		memmove(startp, frontp, window_size * sizeof(T));
		frontp = startp;
		backp = startp + window_size;
		endp = startp + capicity;
	}
	memcpy(backp, t, size * sizeof(T));
	backp += size;
	frontp += size;
	size += size;
}
template<class T> inline void Window<T>::append_zero(int size) {
	if (size > capicity)
		return;
	if (backp + size >= endp) {
		memmove(startp, frontp, window_size * sizeof(T));
		frontp = startp;
		backp = startp + window_size;
		endp = startp + capicity;
	}
	memset(backp, 0, size * sizeof(T));
	backp += size;
	frontp += size;

	size += size;
}

template<class T> inline void Window<T>::reset() {
	memset(startp, 0, backp - startp);
	frontp = startp;
	backp = startp + window_size;
	size =0;
}

#endif