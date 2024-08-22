
#ifndef VARIANT_VARIANT_HPP
#define VARIANT_VARIANT_HPP

#include<memory>
#include<iostream>
#include"invoke.hpp"

template<class T>
struct type_identity
{
  using type=T;
};

//common_type cxx20 form
template<class...>
struct common_type
{};

template<class T>
struct common_type<T>
    :type_identity<T>
{};

template<class T1,class T2>
struct common_type<T1,T2>
{
private:

  template<class L,class R>
  using condition_type=std::decay_t<decltype(false?std::declval<L>():std::declval<R>())>;

  template<class L,class R,class=void>
  struct Impl_base
  {};

  template<class L,class R>
  struct Impl_base<L,R,std::void_t<std::decay_t<decltype(false?std::declval<L>():std::declval<R>())>>>
      :std::decay<decltype(false?std::declval<L>():std::declval<R>())>
  {};

  template<class L,class R,class=void>
  struct Impl
      :Impl_base<L,R>
  {};

  template<class L,class R>
  struct Impl<L,R,std::void_t<condition_type<const L&,const R&>>>
      :type_identity<condition_type<const L&,const R&>>
  {};
public:
  using type=typename Impl<T1,T2>::type;
};

template<class T1,class T2,class...Rest>
struct common_type<T1,T2,Rest...>
    :common_type<typename common_type<T1,T2>::type,Rest...>
{};

template<class...Types>
using common_type_t=typename common_type<Types...>::type;

template<class T>
struct in_place_type_t
{explicit in_place_type_t()=default;};

template<class T>
constexpr in_place_type_t<T> in_place_type{};

template<size_t>
struct in_place_index_t
{explicit in_place_index_t()=default;};

template<size_t Idx>
constexpr in_place_index_t<Idx> in_place_index{};

template<class T,class...Args>
constexpr auto
construct_at(T* location,Args&&...args)
noexcept(noexcept(::new(nullptr)T(std::declval<Args>()...)))
-> decltype(::new(nullptr)T(std::declval<Args>()...))
{ return ::new((void*)location)T(std::forward<Args>(args)...); }

template<unsigned, unsigned, class>
struct Get_n
    :type_identity<void>
{};

template<unsigned Idx, unsigned Obj,
template<class...>class Template,class Head,class...Rest>
struct Get_n<Idx,Obj,Template<Head,Rest...>>
    :std::conditional_t<Obj==Idx,
        type_identity<Head>,
        Get_n<Idx + 1,Obj,Template<Rest...>>
    >
{};

template<class List, unsigned Idx>
using get_n_t = typename Get_n<0,Idx,List>::type;

template<template<class,class>class Predicate,class Template>
struct max
    :type_identity<Template>
{};

template<template<class,class>class Predicate,
    template<class...>class Template,class T,class U>
struct max<Predicate,Template<T,U>>
    :std::conditional_t<Predicate<T,U>::value,
    type_identity<T>,
    type_identity<U>>
{};

template<template<class,class>class Predicate,
    template<class...>class Template,class T>
struct max<Predicate,Template<T>>
    :type_identity<T>
{};

template<template<class,class>class Predicate,
    template<class...>class Template,
    class T,class U,class O,class...Rest>
struct max<Predicate,Template<T,U,O,Rest...>>
    :std::conditional_t<Predicate<T,U>::value,
        max<Predicate,Template<T,O,Rest...>>,
        max<Predicate,Template<U,O,Rest...>>
        >
{};

template<class Template>
struct max_alignment
{
private:
  template<class L,class R>
  using nn=std::integral_constant<bool,(alignof(L)> alignof(R))>;

public:
  using type=typename max<nn,Template>::type;
};

template<class Template>
struct max_size
{
private:
  template<class L,class R>
  using mm=std::integral_constant<bool,(sizeof(L)>sizeof(R))>;

public:
  using type=typename max<mm,Template>::type;
};

template<class Variant,class Index>
class Variant_unique_index
{
  template<size_t Idx,class>
  struct Impl
      :std::integral_constant<ptrdiff_t,-1>
  {};

  template<template<class...>class Template,size_t Idx,class H,class...R>
  struct Impl<Idx,Template<H,R...>>
      :std::conditional_t<std::is_same<H,Index>{},
          std::integral_constant<size_t,Idx>,
          Impl<Idx+1,Template<R...>>>
  {};

public:
  static constexpr auto value=Impl<0,Variant>::value;
};

template<class T>
using max_alignment_t=typename max_alignment<T>::type;

template<class T>
using max_size_t=typename max_size<T>::type;

template<class T>
struct remove_cvref
{
  using type = std::remove_cv_t<std::remove_reference_t<T>>;
};


static_assert(std::is_same<const int&,std::remove_cv_t<const int&>>{});


template<class T>
using remove_cvref_t=typename remove_cvref<T>::type;

template<class T>
struct pop_front
    :type_identity<T>
{using front=void;};

template<template<class...>class Template,class Head,class...Rest>
struct pop_front<Template<Head,Rest...>>
    :type_identity<Template<Rest...>>
{using front=Head;};

template<class T>
struct variant_size;

template<class T,bool=std::is_array<T>::value>
struct Destroy
{
  static void destroy(T*location)
  {
    location->~T();
  }
};

template<class T>
struct Destroy<T,true>
{
  static void destroy(T*location)
  {
    for(auto &x:*location)
      Destroy<typename std::remove_reference<decltype(x)>::type>::destroy(__builtin_addressof(x));
  }
};

template<class T>
void destroy_at(T*location)
{
  Destroy<T>::destroy(location);
}

class computed_result_type;

template <typename Visitor, typename T>
using visit_element_result =
    decltype(std::declval<Visitor>()(std::declval<T>()));

template <typename R, typename Visitor, typename... Types>
struct visit_result
{
  using type = R;
};

template <typename Visitor, typename... Types>
struct visit_result<computed_result_type, Visitor, Types...> {
  using type = std::common_type_t<visit_element_result<Visitor, Types>...>;
};

template <typename R, typename Visitor, typename... Types>
using visit_result_t = typename visit_result<R, Visitor, Types...>::type;

template<class R,class Variant,class Visitor>
constexpr R visit(Visitor&&visitor,Variant&&var)
{
  if (var.template hold<typename pop_front<Variant>::front>())
  {
    return static_cast<R>(std::forward<Visitor>(visitor)(
        std::forward<Variant>(var).template get<typename pop_front<Variant>::fron>()));
  }
  else if(variant_size<typename pop_front<Variant>::type>::value)
  {
    return visit<R>(std::forward<Variant>(var),std::forward<Visitor>(visitor));
  }
  else
  {
    throw;
  }
}

class bad_variant_access
    :std::exception
{
public:

  explicit bad_variant_access(const char*str)
    :M_str{str}
  {}

  bad_variant_access()=default;

  const char*what()const  noexcept override
  {
    return M_str;
  }
private:

  const char*M_str="bad variant access";
};

[[noreturn]]void throw_bad_variant_access(const char*string="bad variant access")
{throw bad_variant_access{string};}

template<class Variant>
struct Variant_storage
{
public:
  template<class Index>
  static constexpr auto S_index_v=Variant_unique_index<Variant,Index>::value;

public:
  Variant_storage()noexcept=default;

  template<class Type,class...Args>
  explicit Variant_storage(in_place_type_t<Type>,Args&&...args)
  {
    new(M_voidptr())Type{std::forward<Args>(args)...};
    M_index=S_index_v<Type>;
  }

  template<class Type,class U,class...Args>
  explicit Variant_storage(in_place_type_t<Type>,std::initializer_list<U> list,Args&&...args)
  {
    new(M_voidptr())Type{list,std::forward<Args>(args)...};
    M_index=S_index_v<Type>;
  }

  void* M_voidptr()
  {return static_cast<void*>(&M_buffer); }

  const void* M_voidptr()const
  {return static_cast<const void*>(&M_buffer); }

  template <class V>
  V*M_valptr()
  {return static_cast<V*>(static_cast<void*>(&M_buffer));}

  template<class V>
  const V*M_valptr()const
  {return static_cast<const V*>(static_cast<const void*>(&M_buffer));}

  alignas(max_alignment_t<Variant>) unsigned char M_buffer[sizeof(max_size_t<Variant>)]{};

  unsigned char M_index = -1;
};

template<class Type,class Variant>
struct Variant_destroy
{
  bool destroy()
  {
    auto location=static_cast<Variant*>(this);
    constexpr auto index=Variant_unique_index<Variant,Type>::value;
    if(index==location->index()&&location->M_index!=-1)
    {
      //std::cout<<index<<'\n';
      destroy_at(location->template M_valptr<Type>());
    }

    return true;
  }
};

template<class Variant,class Input>
struct Variant_match
{
private:

  template<class Choice>
  static constexpr size_t S_index_v=Variant_unique_index<Variant,Choice>::value;

  template<class Ti>
  struct Array
  {Ti M_array[1]{};};

  template<class T,class Ti,class=void>
  struct Alternative
  {
    static void S_func();
  };

  template<class T,class Ti>
  struct Alternative<T,Ti,std::void_t<decltype(Array<Ti>{{std::declval<T>()}})>>
  {
    static std::integral_constant<size_t ,S_index_v<Ti>> S_func(Ti);
  };

  template<class V,class T>
  struct Impl
  {};

  template<template<class...>class Template,class T,class...Ti>
  struct Impl<Template<Ti...>,T>
      :Alternative<T,Ti>...
  {
    using Alternative<T,Ti>::S_func...;
  };
public:
  using type=get_n_t<Variant,decltype(Impl<Variant,Input>::S_func(std::declval<Input>()))::value>;
};

template<class Variant,class T>
using Variant_match_t=typename Variant_match<remove_cvref_t<Variant>,T>::type;

template<class>
struct make_empty
{};

template<template<class...>class Template, class...Types>
struct make_empty<Template<Types...>>
    :type_identity<Template<>>
{};

template<class Class>
using make_empty_t = typename make_empty<Class>::type;

template<class Class>
class unique
{
private:

  template<class T,class Index>
  static constexpr bool S_contain_v=Variant_unique_index<T,Index>::value!=-1;

  template<bool U,class T,class E=make_empty_t<T>>
  struct Impl
  {
    static constexpr bool S_unique = U;
    using type=E;
  };

  template<bool U,template<class ...>class Template,
      class Head, class...Rest,class...ETypes>
  struct Impl<U,Template<Head,Rest...>,Template<ETypes...>>
      :std::conditional_t<!S_contain_v<Template<ETypes...>,Head>,
        Impl<true,Template<Rest...>,Template<ETypes...,Head>>,
        Impl<false,Template<Rest...>,Template<ETypes...>>
  >
  {};

public:
  using type = typename Impl<true,Class,make_empty_t<Class>>::type;
  static constexpr bool is_unique_v =Impl<true,Class,make_empty_t<Class>>::S_unique;
};

template<class List>
using unique_t = typename unique<List>::type;

template<class List>
constexpr bool is_unique_v = unique<List>::is_unique_v;

template<class Type,class Variant>
struct SMF_control
{
  static bool S_move_from(Variant*src,Variant*other) {
    auto location = static_cast<Variant *>(src);
    constexpr auto index = Variant_unique_index<Variant, Type>::value;
    if (index == location->index()) {
      if (index == other->index())
        *location->template M_valptr<Type>() = std::move(*other->template M_valptr<Type>());
      else {
        location->reset(); // try destroy() for all types
        construct_at(location->template M_valptr<Type>(), std::move(*other->template M_valptr<Type>()));
        location->M_index = index;
      }
    }
    return true;
  }
  static bool S_copy_from(Variant*src,const Variant*other)
  {
    auto location=static_cast<Variant*>(src);
    constexpr auto index=Variant_unique_index<Variant,Type>::value;
    if(index==location->index())
    {
      if (index == other->index())
        *location->template M_valptr<Type>() = *other->template M_valptr<Type>();
      else
      {
        location->reset(); // try destroy() for all types
        construct_at(location->template M_valptr<Type>(),*other->template M_valptr<Type>());
        location->M_index=index;
      }
    }

    return true;
  }
};


template<class...Types>
struct variant
    :private Variant_destroy<Types,variant<Types...>>...,
      private Variant_storage<variant<Types...>>,
      private SMF_control<Types,variant<Types>>...
{
  static_assert(is_unique_v<variant>,"");

  template<class,class>friend class Variant_destroy;
  template<class,class>friend class SMF_control;
private:

  template<class Index>
  static constexpr auto S_index_v=Variant_unique_index<variant,Index>::value;

  template<class Index>
  static constexpr bool S_exist_once_v=S_index_v<Index>!=-1;

  using Storage=Variant_storage<variant>;
  using Storage::Storage;

public:

  constexpr variant()noexcept=default;

  constexpr variant(const variant&other)
  {
    if(other.M_index!=-1)
      bool Func[]={SMF_control<Types,variant<Types>>::S_copy_from(this,&other)...};
  }

  constexpr variant(variant&&other)noexcept
  {
    //::visit<void>([&](auto&&value){*this=std::move(other);},*this);
    if(other.M_index!=-1)
      bool Func[]={SMF_control<Types,variant>::S_move_from(this,&other)...};
  }

  template<class T,class Ti=Variant_match_t<variant,T>,
      std::enable_if_t<!std::is_same<Ti,void>::value,bool> =true,
      std::enable_if_t<std::is_constructible<Ti,T>{},bool> =true,
      std::enable_if_t<!is_specialization<in_place_type_t,T>{},bool> =true
      >
  variant(T&&value)
  noexcept(std::is_nothrow_constructible<Ti,T>{})
  {
    new(this->template M_valptr<Ti>()) Ti{std::forward<Ti>(value)};
    this->M_index=Variant_unique_index<variant,Ti>::value;
  }

  template< class T,class...Args,
      std::enable_if_t<S_exist_once_v<T>,bool> =true,
      std::enable_if_t<std::is_constructible<T,Args...>{},bool> =true>
  explicit variant(in_place_type_t<T>,Args&&... args)
    :Storage(in_place_type<T>,static_cast<Args>(args)...)
  {}

  template< class T,class U,class...Args,
      std::enable_if_t<S_exist_once_v<T>,bool> =true,
      std::enable_if_t<std::is_constructible<T,std::initializer_list<U>&,Args...>{},bool> =true>
  explicit variant(in_place_type_t<T>,std::initializer_list<U> list,Args&&... args)
      :Storage(in_place_type<T>,list,static_cast<Args>(args)...)
  {}

  template<size_t Idx,class...Args,
      class T=get_n_t<variant,Idx>,
      std::enable_if_t<S_exist_once_v<T>,bool> =true,
      std::enable_if_t<std::is_constructible<T,Args...>{},bool> =true>
  explicit variant(in_place_index_t<Idx>,Args&&... args)
      :Storage(in_place_type<T>,static_cast<Args>(args)...)
  {}

  template<size_t Idx,class U,class...Args,
      class T=get_n_t<variant,Idx>,
      std::enable_if_t<S_exist_once_v<T>,bool> =true,
      std::enable_if_t<std::is_constructible<T,std::initializer_list<U>&,Args...>{},bool> =true>
    explicit variant(in_place_index_t<Idx>,std::initializer_list<U> list,Args&&... args)
      :Storage(in_place_type<T>,list,static_cast<Args>(args)...)
  {}

  template<class Type,
      class Ti=Variant_match_t<variant,Type>,
      std::enable_if_t<!std::is_same<Ti,void>::value,bool> =true,
      std::enable_if_t<std::is_constructible<Ti,Type>{},bool> =true>
  variant&operator=(Type&&value)
  {
    if(S_index_v<Ti> ==index())
      *this->template M_valptr<Ti>()=std::forward<Type>(value);
    else
    {
      construct_at(this->template M_valptr<Ti>(),std::forward<Type>(value));
      this->M_index=S_index_v<Ti>;
    }
    return *this;
  }

  variant&operator=(const variant&other)
  {
    if(other.M_index==-1&&this->M_index==-1)
      return *this;
    else if(other.M_index==-1||this->M_index==-1)
    {
      if(other.M_index==-1)
        reset();
      else
        bool Func[]={SMF_control<Types,variant>::S_copy_from(this,&other)...};
      return *this;
    }
    else
    {
      reset();
      bool Func[]={SMF_control<Types,variant>::S_copy_from(this,&other)...};
      return *this;
    }
  }

  variant&operator=(variant&&other)noexcept
  {
    if(other.M_index==-1&&this->M_index==-1)
      return *this;
    else if(other.M_index==-1||this->M_index==-1)
    {
      if(other.M_index==-1)
        reset();
      else
        bool Func[]={SMF_control<Types,variant>::S_move_from(this,&other)...};
      return *this;
    }
    else
    {
      reset();
      bool Func[]={SMF_control<Types,variant>::S_move_from(this,&other)...};
      return *this;
    }
  }

  constexpr size_t index()const noexcept
  {return this->M_index;}

  template<class Type>
  constexpr bool hold()const
  {
    return S_exist_once_v<Type>&&index()==S_index_v<Type>;
  }

  template<class Index,
      std::enable_if_t<S_exist_once_v<Index>>>
  Index&get()
  {
    return *this->template M_valptr<Index>();
  }

  template<class Index,
      std::enable_if_t<S_exist_once_v<Index>,bool>  =true>
  const Index&get()const
  {
    return *this->template M_valptr<Index>();
  }

  template<size_t Index,
      class=std::enable_if_t<(Index<sizeof...(Types))>>
  get_n_t<variant,Index>&get()
  {
    return *this->template M_valptr<get_n_t<variant,Index>>();
  }

  template<size_t Index,
      class=std::enable_if_t<(Index<sizeof...(Types))>>
  const get_n_t<variant,Index>&get()const
  {
    return *this->template M_valptr<get_n_t<variant,Index>>();
  }

  template<class Visitor>
  constexpr decltype(auto) visit(Visitor&& vis )
  {

  }
  template< class R,class Visitor >
  constexpr R visit( Visitor&&vis)
  {

  }

  void reset()
  {
    bool Func[]={Variant_destroy<Types, variant>::destroy()...};
    this->M_index=-1;
  }

  ~variant()
  {
    bool Func[]={Variant_destroy<Types, variant>::destroy()...};
  }

  template<size_t Idx,
      class...Args,
      class=std::enable_if_t<std::is_constructible<get_n_t<variant,Idx>, Args...>::value>>
  decltype(auto)
  emplace(Args&&...args)noexcept
  {
    return construct_at(this->template M_valptr<get_n_t<variant,Idx>>(),std::forward<Args>(args)...);
  }

  template<class T,
      class...Args,
      class=std::enable_if_t<std::is_constructible<T, Args...>::value>>
  decltype(auto)
  emplace(Args&&...args)noexcept
  {
    return construct_at(this->template M_valptr<T>(),std::forward<Args>(args)...);
  }

  template<size_t Idx,class U,
      class...Args,
      class=std::enable_if_t<std::is_constructible<get_n_t<variant,Idx>, std::initializer_list<U>&,Args...>::value>>
  decltype(auto)
  constexpr emplace(std::initializer_list<U> list,Args&&...args)noexcept
  {
    return construct_at(this->template M_valptr<get_n_t<variant,Idx>>(),list,std::forward<Args>(args)...);
  }

  template<class T,class U,
      class...Args,
      class=std::enable_if_t<std::is_constructible<T, std::initializer_list<U>&,Args...>::value>>
  decltype(auto)
  constexpr emplace(std::initializer_list<U> list,Args&&...args)noexcept
  {
    return construct_at(this->template M_valptr<T>(),list,std::forward<Args>(args)...);
  }

  void swap(variant&other)noexcept
  {
    if(other.M_index==-1&&this->M_index==-1)
      return;
    else if(other.M_index==-1||this->M_index==-1)
    {
      if(this->M_index==-1)
        *this=std::move(other);
      else
        other=std::move(*this);
    }
    else
    {
        variant temp;
        temp=std::move(other);
        other=std::move(*this);
        *this=std::move(temp);
    }
  }
};

template<class>
struct variant_size_impl
{};

template<class...Types>
struct variant_size_impl<variant<Types...>>
    :std::integral_constant<size_t,sizeof...(Types)>
{};

template<class Variant>
struct variant_size
    :variant_size_impl<remove_cvref_t<Variant>>
{};

template<class Variant>
constexpr size_t variant_size_v=variant_size<remove_cvref_t<Variant>>::value;
#endif
