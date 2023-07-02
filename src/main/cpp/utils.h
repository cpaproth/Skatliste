//Copyright (C) 2023 Carsten Paproth - Licensed under MIT License

template<class T> struct Guard {
	T f;
	Guard(T&& f) : f(std::forward<T>(f)) {}
	~Guard() {f();}
};
template<class T> Guard<T> guard(T&& f) {return Guard<T>(std::forward<T>(f));}
