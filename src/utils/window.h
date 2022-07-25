#ifndef WINDOW_H
#define WINDOW_H
#include <vector>

template<class T> class Window
{
public:
	Window(int, int, bool autoIncrease = false);
	~Window();
	void push_front(const T&);
	T del_back();
	void shift(const T&);
	void reset();
	void append(const T*, int);
	size_t getSize();
	T* frontp;
	T* backp;

private:
	T* startp;
	T* endp;

	int capicity;
	int window_size;
	bool autoIncrease;
	bool increased;

};
template<class T> size_t Window<T>::getSize()
{
	return backp - frontp;
}

template<class T> Window<T>::~Window()
{
	delete[] startp;
}

template<class T> Window<T>::Window(int cap, int window_size, bool autoIncrease)
	: window_size(window_size)
	, autoIncrease(autoIncrease)
	, increased(false)
{
	if (window_size > cap) {
		capicity = window_size + cap;
	}
	else
		capicity = cap;

	startp = new T[capicity]();
	frontp = startp;

	backp = autoIncrease ? startp : startp + window_size;
	endp = startp + capicity;

}
template<class T> void Window<T>::push_front(const T& t)
{
	if (endp == backp) {
		memcpy(startp, frontp, window_size * sizeof(T));
		frontp = startp;
		backp = startp + window_size;
		endp = startp + capicity;
	}
	backp[0] = t;
	backp += 1;
}
template<class T> T Window<T>::del_back() {
	frontp += 1;
	return *(frontp - 1);
}

template<class T> void Window<T>::shift(const T& t) {
	push_front(t);
	if (autoIncrease && !increased)
		if (backp - frontp <= window_size)
			return;
		else
			increased = true;
	frontp += 1;
}

template<class T> void Window<T>::append(const T* t, int size) {
	if (size > capicity)
		return;
	if (backp + size >= endp) {
		memcpy(startp, frontp, window_size);
		frontp = startp;
		backp = startp + window_size;
		endp = startp + capicity;
	}
	memcpy(backp, t, size);
	backp += size;
	frontp += size;
}

template<class T> void Window<T>::reset() {
	memset(startp, 0, backp - startp);
	frontp = startp;
	backp = autoIncrease ? startp : startp + window_size;
	increased = false;
}

#endif