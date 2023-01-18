
#pragma once

//////////////////////////////////////////////////////////////////////////
template<typename T>
T nmax(const T& t) {
    return t;
}

template<typename T1, typename T2>
auto nmax(const T1& t1, const T2& t2) {
    return t1 > t2 ? t1 : t2;
}

template<typename T, typename... ARG>
auto nmax(const T& t, const ARG&... arg) {
    return nmax(t, nmax(arg...));
}


//////////////////////////////////////////////////////////////////////////
template<typename T>
T nmin(const T& t) {
    return t;
}

template<typename T1, typename T2>
auto nmin(const T1& t1, const T2& t2) {
    return t1 < t2 ? t1 : t2;
}

template<typename T, typename... ARG>
auto nmin(const T& t, const ARG&... arg) {
    return nmin(t, nmin(arg...));
}

//////////////////////////////////////////////////////////////////////////
template<typename T>
void _exist_in(bool & b, const T& t) {
    b = false;
}

template<typename T1, typename T2, typename... ARG>
void _exist_in(bool & b, const T1& t1, const T2& t2, const ARG&... arg) {
    if (b) {
        return;
    }
    b = (t1 == t2);
    _exist_in(b, t1, arg...);
}

// 返回t是否存在于后面的几个参数中
template<typename T, typename... ARG>
bool exist_in(const T& t, const ARG&... arg) {
    bool b = false;
    _exist_in(b, t, arg...);
    return b;
}