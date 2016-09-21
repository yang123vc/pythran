#ifndef PYTHONIC_TYPES_NUMPY_EXPR_HPP
#define PYTHONIC_TYPES_NUMPY_EXPR_HPP

#include "pythonic/include/types/numpy_expr.hpp"

#include "pythonic/utils/meta.hpp"
#include "pythonic/types/nditerator.hpp"

namespace pythonic
{

  namespace types
  {

    namespace details
    {
      template <size_t value, class... Args>
      array<long, value> init_shape(Args const &... args)
      {
        array<long, value> shape;
        for (int i = 0; i < value; ++i) {
          array<long, sizeof...(Args)> IShapes{
              {(value <= Args::value ? args.shape()[i] : 0)...}};
          shape[i] = *std::max_element(IShapes.begin(), IShapes.end());
        }
        return shape;
      }
    }

    template <class Op, class... Args>
    numpy_expr<Op, Args...>::numpy_expr()
    {
    }

    template <class Op, class... Args>
    numpy_expr<Op, Args...>::numpy_expr(Args const &... args)
        : args(args...), _shape(details::init_shape<value>(args...))
    {
    }

    template <class Op, class... Args>
    template <int... I>
    typename numpy_expr<Op, Args...>::const_iterator
        numpy_expr<Op, Args...>::_begin(utils::seq<I...>) const
    {
      return {*this,
              {(size() == std::get<I>(args).shape()[0])...},
              std::get<I>(args).begin()...};
    }

    template <class Op, class... Args>
    typename numpy_expr<Op, Args...>::const_iterator
    numpy_expr<Op, Args...>::begin() const
    {
      return _begin(typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    typename numpy_expr<Op, Args...>::const_iterator
        numpy_expr<Op, Args...>::_end(utils::seq<I...>) const
    {
      return {*this,
              {(size() == std::get<I>(args).shape()[0])...},
              std::get<I>(args).end()...};
    }

    template <class Op, class... Args>
    typename numpy_expr<Op, Args...>::const_iterator
    numpy_expr<Op, Args...>::end() const
    {
      return _end(typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    bool numpy_expr<Op, Args...>::_no_broadcast(utils::seq<I...>) const
    {
      std::initializer_list<bool> nb = {
          ((size() == std::get<I>(args).shape()[0]) ||
           (0 == std::get<I>(args).shape()[0]))...};
      return std::all_of(nb.begin(), nb.end(), [](bool b) { return b; });
    }
    template <class Op, class... Args>
    bool numpy_expr<Op, Args...>::no_broadcast() const
    {
      return _no_broadcast(typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    typename numpy_expr<Op, Args...>::iterator
        numpy_expr<Op, Args...>::_begin(utils::seq<I...>)
    {
      return {*this,
              {(size() == std::get<I>(args).shape()[0])...},
              std::get<I>(args).begin()...};
    }

    template <class Op, class... Args>
    typename numpy_expr<Op, Args...>::iterator numpy_expr<Op, Args...>::begin()
    {
      return _begin(typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    typename numpy_expr<Op, Args...>::iterator
        numpy_expr<Op, Args...>::_end(utils::seq<I...>)
    {
      return {*this,
              {(size() == std::get<I>(args).shape()[0])...},
              std::get<I>(args).end()...};
    }

    template <class Op, class... Args>
    typename numpy_expr<Op, Args...>::iterator numpy_expr<Op, Args...>::end()
    {
      return _end(typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    auto numpy_expr<Op, Args...>::_fast(long i, utils::seq<I...>) const
        -> decltype(Op()(std::get<I>(args).fast(i)...))
    {
      return Op()(std::get<I>(args).fast(i)...);
    }

    template <class Op, class... Args>
    auto numpy_expr<Op, Args...>::fast(long i) const
        -> decltype(this->_fast(i,
                                typename utils::gens<sizeof...(Args)>::type{}))
    {
      return _fast(i, typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    auto numpy_expr<Op, Args...>::_map_fast(
        std::array<long, sizeof...(I)> const &indices, utils::seq<I...>) const
        -> decltype(Op()(std::get<I>(args).fast(std::get<I>(indices))...))
    {
      return Op()(std::get<I>(args).fast(std::get<I>(indices))...);
    }

    template <class Op, class... Args>
    template <class... Indices>
    auto numpy_expr<Op, Args...>::map_fast(Indices... indices) const
        -> decltype(
            this->_map_fast(std::array<long, sizeof...(Indices)>{{indices...}},
                            typename utils::gens<sizeof...(Args)>::type{}))
    {
      static_assert(sizeof...(Indices) == sizeof...(Args), "compatible call");
      return _map_fast(std::array<long, sizeof...(Indices)>{{indices...}},
                       typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    auto numpy_expr<Op, Args...>::operator[](long i) const
        -> decltype(this->fast(i))
    {
      if (i < 0)
        i += _shape[0];
      return fast(i);
    }

    template <class Op, class... Args>
    auto numpy_expr<Op, Args...>::operator()(long i) const
        -> decltype(this->fast(i))
    {
      if (i < 0)
        i += _shape[0];
      return fast(i);
    }

    template <class Op, class... Args>
    array<long, numpy_expr<Op, Args...>::value> const &
    numpy_expr<Op, Args...>::shape() const
    {
      return _shape;
    }

#ifdef USE_BOOST_SIMD
    template <class Op, class... Args>
    template <int... I>
    typename numpy_expr<Op, Args...>::simd_iterator
        numpy_expr<Op, Args...>::_vbegin(vectorize, utils::seq<I...>) const
    {
      return {*this,
              {(size() == std::get<I>(args).shape()[0])...},
              std::make_tuple(std::get<I>(args).begin()...),
              std::get<I>(args).vbegin(vectorize{})...};
    }

    template <class Op, class... Args>
    typename numpy_expr<Op, Args...>::simd_iterator
        numpy_expr<Op, Args...>::vbegin(vectorize) const
    {
      return _vbegin(vectorize{},
                     typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    typename numpy_expr<Op, Args...>::simd_iterator
        numpy_expr<Op, Args...>::_vend(vectorize, utils::seq<I...>) const
    {
      return {*this,
              {(size() == std::get<I>(args).shape()[0])...},
              std::make_tuple(std::get<I>(args).end()...),
              std::get<I>(args).vend(vectorize{})...};
    }

    template <class Op, class... Args>
    typename numpy_expr<Op, Args...>::simd_iterator
        numpy_expr<Op, Args...>::vend(vectorize) const
    {
      return _vend(vectorize{}, typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    typename numpy_expr<Op, Args...>::simd_iterator_nobroadcast
        numpy_expr<Op, Args...>::_vbegin(vectorize_nobroadcast,
                                         utils::seq<I...>) const
    {
      return {*this, std::get<I>(args).vbegin(vectorize_nobroadcast{})...};
    }

    template <class Op, class... Args>
    typename numpy_expr<Op, Args...>::simd_iterator_nobroadcast
        numpy_expr<Op, Args...>::vbegin(vectorize_nobroadcast) const
    {
      return _vbegin(vectorize_nobroadcast{},
                     typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    typename numpy_expr<Op, Args...>::simd_iterator_nobroadcast
        numpy_expr<Op, Args...>::_vend(vectorize_nobroadcast,
                                       utils::seq<I...>) const
    {
      return {*this, std::get<I>(args).vend(vectorize_nobroadcast{})...};
    }

    template <class Op, class... Args>
    typename numpy_expr<Op, Args...>::simd_iterator_nobroadcast
        numpy_expr<Op, Args...>::vend(vectorize_nobroadcast) const
    {
      return _vend(vectorize_nobroadcast{},
                   typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    auto numpy_expr<Op, Args...>::_load(long i, utils::seq<I...>) const
        -> decltype(Op()(std::get<I>(args).load(i)...))
    {
      return Op()(std::get<I>(args).load(i)...);
    }

    template <class Op, class... Args>
    template <class I> // template to prevent automatic instantiation when the
    // type is not vectorizable
    auto numpy_expr<Op, Args...>::load(I i) const
        -> decltype(this->_load(i,
                                typename utils::gens<sizeof...(Args)>::type{}))
    {
      return _load(i, typename utils::gens<sizeof...(Args)>::type{});
    }

    template <class Op, class... Args>
    template <int... I>
    auto numpy_expr<Op, Args...>::_map_load(
        std::array<long, sizeof...(I)> const &indices, utils::seq<I...>) const
        -> decltype(Op()(std::get<I>(args).load(std::get<I>(indices))...))
    {
      return Op()(std::get<I>(args).load(std::get<I>(indices))...);
    }

    template <class Op, class... Args>
    template <class... Indices>
    auto numpy_expr<Op, Args...>::map_load(Indices... indices) const
        -> decltype(
            this->_map_load(std::array<long, sizeof...(Indices)>{{indices...}},
                            typename utils::gens<sizeof...(Args)>::type{}))
    {
      return _map_load(std::array<long, sizeof...(Indices)>{{indices...}},
                       typename utils::gens<sizeof...(Args)>::type{});
    }
#endif

    template <class Op, class... Args>
    template <int... I, class... S>
    auto numpy_expr<Op, Args...>::_get(utils::seq<I...>, S const &... s) const
        -> decltype(Op{}(std::get<I>(args)(s...)...))
    {
      return Op{}(std::get<I>(args)(s...)...);
    }

    template <class Op, class... Args>
    template <class S0, class... S>
    auto numpy_expr<Op, Args...>::operator()(S0 const &s0, S const &... s) const
        -> typename std::enable_if<
            not std::is_scalar<S0>::value,
            decltype(this->_get(typename utils::gens<sizeof...(Args)>::type{},
                                s0, s...))>::type
    {
      return _get(typename utils::gens<sizeof...(Args)>::type{}, s0, s...);
    }

    template <class Op, class... Args>
    template <class F>
    typename std::enable_if<is_numexpr_arg<F>::value,
                            numpy_fexpr<numpy_expr<Op, Args...>, F>>::type
    numpy_expr<Op, Args...>::fast(F const &filter) const
    {
      return numpy_fexpr<numpy_expr, F>(*this, filter);
    }

    template <class Op, class... Args>
    template <class F>
    typename std::enable_if<is_numexpr_arg<F>::value,
                            numpy_fexpr<numpy_expr<Op, Args...>, F>>::type
        numpy_expr<Op, Args...>::
        operator[](F const &filter) const
    {
      return fast(filter);
    }

    template <class Op, class... Args>
    long numpy_expr<Op, Args...>::flat_size() const
    {
      return std::accumulate(_shape.begin() + 1, _shape.end(), _shape.front(),
                             std::multiplies<long>());
    }

    template <class Op, class... Args>
    long numpy_expr<Op, Args...>::size() const
    {
      return _shape[0];
    }
  }
}

#endif
