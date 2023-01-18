
#pragma once

#include <type_traits>
#include <tuple>
#include <functional>
#include <map>
#include <vector>
#include <queue>
#include <map>
#include <set>
#include <stack>
#include <list>
#include <unordered_set>

#define HAS_MEMBER(member)\
	template<typename T, typename... Args>\
struct has_##member\
{\
private:\
	template<typename U> static auto Check(int) -> decltype(std::declval<U>().member(std::declval<Args>()...), std::true_type()); \
	template<typename U> static std::false_type Check(...); \
public:\
enum{ value = std::is_same<decltype(Check<T>(0)), std::true_type>::value }; \
};

//////////////////////////////////////////////////////////////////////////
// template<typename T>
// using decay_t = typename std::decay<T>::type;

template <typename T, template <typename...> class Template>
struct is_specialization_of : std::false_type {};

template <template <typename...> class Template, typename... Args>
struct is_specialization_of<Template<Args...>, Template> : std::true_type{};


//////////////////////////////////////////////////////////////////////////
template <typename... Args, typename F, std::size_t... Idx>
void for_each_l(std::tuple<Args...>& t, F&& f, std::index_sequence<Idx...>)
{
	std::initializer_list<int>{(std::forward<F>(f)(std::get<Idx>(t)), 0)...};
}

template <typename... Args, typename F, std::size_t... Idx>
void for_each_r(std::tuple<Args...>& t, F&& f, std::index_sequence<Idx...>)
{
	constexpr auto size = sizeof...(Idx);
	std::initializer_list<int>{(std::forward<F>(f)(std::get<size - Idx - 1>(t)), 0)...};
}


//////////////////////////////////////////////////////////////////////////
//member function
#define TIMAX_FUNCTION_TRAITS(...)\
	template <typename ReturnType, typename ClassType, typename... Args>\
struct function_traits_impl<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : function_traits_impl<ReturnType(Args...)>{}; \

/*
* 1. function type                                                 ==>     Ret(Args...)
* 2. function pointer                                              ==>     Ret(*)(Args...)
* 3. function reference                                            ==>     Ret(&)(Args...)
* 4. pointer to non-static member function ==> Ret(T::*)(Args...)
* 5. function object and functor                           ==> &T::operator()
* 6. function with generic operator call           ==> template <typeanme ... Args> &T::operator()
*/
template <typename T>
struct function_traits_impl;

template<typename T>
struct function_traits : function_traits_impl<
	std::remove_cv_t<std::remove_reference_t<T>>>
{};

template<typename Ret, typename... Args>
struct function_traits_impl<Ret(Args...)>
{
public:
	enum { arity = sizeof...(Args) };
	typedef Ret function_type(Args...);
	typedef Ret result_type;
	using stl_function_type = std::function<function_type>;
	typedef Ret(*pointer)(Args...);

	template<size_t I>
	struct args
	{
		static_assert(I < arity, "index is out of range, index must less than sizeof Args");
		using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
	};

	typedef std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...> tuple_type;
	using args_type_t = std::tuple<Args...>;
};

template<size_t I, typename Function>
using arg_type = typename function_traits<Function>::template args<I>::type;

// function pointer
template<typename Ret, typename... Args>
struct function_traits_impl<Ret(*)(Args...)> : function_traits<Ret(Args...)>{};

// std::function
template <typename Ret, typename... Args>
struct function_traits_impl<std::function<Ret(Args...)>> : function_traits_impl<Ret(Args...)>{};

// pointer of non-static member function
TIMAX_FUNCTION_TRAITS()
TIMAX_FUNCTION_TRAITS(const)
TIMAX_FUNCTION_TRAITS(volatile)
TIMAX_FUNCTION_TRAITS(const volatile)

// functor
template<typename Callable>
struct function_traits_impl : function_traits_impl<decltype(&Callable::operator())> {};

