#ifndef VARIANT_INVOKE_HPP
#define VARIANT_INVOKE_HPP

#include<utility>

template<template<class...>class Template,class Type>
struct is_specialization
    :std::false_type
{};

template<template<class...>class Template,class...Types>
struct is_specialization<Template,Template<Types...>>
    :std::true_type
{};

template<bool,int>
struct Invoke_helper
{};

template<>
struct Invoke_helper<true,0>
{
  template< class F,class Obj, class... Args >
  static constexpr auto
  invoke( F&& f,Obj&&obj, Args&&... args ) noexcept
  {
    return (std::forward<Obj>(obj).*f)(std::forward<Args>(args)...);
  }
};

template<>
struct Invoke_helper<true,2>
{
  template< class F,class Obj, class... Args >
  static constexpr auto
  invoke( F&& f,Obj&&obj, Args&&... args ) noexcept
  {
    return ((*std::forward<Obj>(obj)).*f)(std::forward<Args>(args)...);
  }
};

template<>
struct Invoke_helper<true,1>
{
  template< class F,class Obj, class... Args >
  static constexpr auto
  invoke( F&& f,Obj&&obj, Args&&... args ) noexcept
  {
    return (std::forward<Obj>(obj).get().*f)(std::forward<Args>(args)...);
  }
};

template<>
struct Invoke_helper<false,0>
{
  template< class F,class Obj>
  static constexpr auto
  invoke(F&& f,Obj&&obj) noexcept
  {
    return (std::forward<Obj>(obj).*f);
  }
};

template<>
struct Invoke_helper<false,2>
{
  template< class F,class Obj>
  static constexpr auto
  invoke( F&& f,Obj&&obj) noexcept
  {
    return ((*std::forward<Obj>(obj)).*f);
  }
};

template<>
struct Invoke_helper<false,1>
{
  template< class F,class Obj,class Arg>
  static constexpr auto
  invoke( F&& f,Obj&&obj,Arg&&arg) noexcept
  {
    return (std::forward<Obj>(obj).get().*f)(std::forward<Arg>(arg));
  }
};

template<class >
struct Invoke_traits
{
  using Class_type=void;
  using Other_type=void;
};

template<class Class,class Other>
struct Invoke_traits<Other Class::*>
{
  using Class_type=Class;
  using Other_type=Other;
};

template< class F, class Obj,class... Args,
    std::enable_if_t<std::is_member_pointer<F>::value,bool> =true>
constexpr auto
invoke( F&& f,Obj&&obj,Args&&... args ) noexcept
{
  using Class_type=typename Invoke_traits<F>::Class_type;
  using Obj_type=typename std::remove_cv<typename std::remove_reference<Obj>::type>::type;

  constexpr int choice
      =std::is_same<Class_type,Obj_type>::value || std::is_base_of<Class_type,Obj_type>::value?
      0:is_specialization<std::reference_wrapper,Obj_type>::value?1:2;

  return Invoke_helper<std::is_member_function_pointer<F>::value,choice>
      ::invoke(std::forward<F>(f),std::forward<Obj>(obj),std::forward<Args>(args)...);
}

template<class Callable,class...Args>
constexpr auto invoke(Callable&&callable,Args&&...args)
{
  return std::forward<Callable>(callable)(std::forward<Args>(args)...);
}

template <class,class,class...>
struct invoke_result_helper
{
  using type=void;
  static constexpr bool value=false;
};

template <class Callable,class...Args>
struct invoke_result_helper<std::void_t<decltype(invoke(std::declval<Callable>(), std::declval<Args>()...))>,Callable,Args...>
{
  using type=decltype(invoke(std::declval<Callable>(), std::declval<Args>()...));
  static constexpr bool value=true;
};

template <class Callable,class...Args>
using invoke_result_t = typename invoke_result_helper<Callable, Args...>::type;

template <class Return,class U=typename std::remove_cv<Return>::type>
struct invoke_r_helper
{
public:
  template <class Callable,class... Args>
  static constexpr Return invoke(Callable&&callable,Args&&... args)
  {
    return invoke(std::forward<Callable>(callable),std::forward<Args>(args)...);
  }
};

template <typename R>
struct invoke_r_helper<R, void>
{
public:
  template <class Callable,class... Args>
  static constexpr auto invoke(Callable&&callable,Args&&... args)
  {
    return static_cast<void>(invoke(std::forward<Callable>(callable),std::forward<Args>(args)...));
  }
};

template <class R,class Callable,class... Args>
constexpr R
invoke_r(Callable&&callable,Args&&... args)
{return invoke_r_helper<R>(std::forward<Callable>(callable),std::forward<Args>(args)...);}

template<class Callable,class...Args>
struct is_invocable
    :std::integral_constant<bool,invoke_result_helper<void,Callable,Args...>::value>
{};

template <class,class,class=void>
struct is_invocable_r_helper
    : std::false_type
{};

template <class Callable,class...Args,class Return>
struct is_invocable_r_helper<Callable(Args...),Return,decltype(
    invoke_r<Return>(std::declval<Callable>(), std::declval<Args>()...))>
    : std::true_type
{};

template <class Callable,class...Args>
struct is_invocable_r
    :is_invocable_r_helper<Callable,Args...>
{};

  template <class,class= void>
  struct is_nothrow_invocable_impl
      : std::false_type
  {};

  template <class Callable,class...Args>
  struct is_nothrow_invocable_impl<Callable(Args...), decltype(
      invoke(std::declval<Callable>(), std::declval<Args>()...))>
      : std::integral_constant<bool, noexcept(invoke(std::declval<Callable>(), std::declval<Args>()...))>
  {};


template <class Callable,class...Args>
struct is_nothrow_invocable
    :is_nothrow_invocable_impl<Callable&&(Args&&...)>::type
{};

template <typename, typename, typename= void>
struct is_nothrow_invocable_r_impl
    : std::false_type
{};

template <typename F, typename... Ts, typename R>
struct is_nothrow_invocable_r_impl<F(Ts...), R, decltype((void)
    invoke_r<R>(std::declval<F>(), std::declval<Ts>()...))>
    : std::integral_constant<bool, noexcept(invoke_r<R>(std::declval<F>(), std::declval<Ts>()...))>
{};

template <typename R, typename Fn, typename... ArgTypes>
struct is_nothrow_invocable_r
    : is_nothrow_invocable_r_impl<Fn&&(ArgTypes&&...), R>::type
{};

#endif //VARIANT_INVOKE_HPP
