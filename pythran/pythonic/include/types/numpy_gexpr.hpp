#ifndef PYTHONIC_INCLUDE_TYPES_NUMPY_GEXPR_HPP
#define PYTHONIC_INCLUDE_TYPES_NUMPY_GEXPR_HPP

#include "pythonic/include/utils/meta.hpp"
#ifdef USE_BOOST_SIMD
#include <boost/simd/pack.hpp>
#include <boost/simd/function/store.hpp>
#endif

namespace pythonic
{

  namespace types
  {

    /* helper to count new axis
     */
    template <class... T>
    struct count_new_axis;

    template <>
    struct count_new_axis<types::none_type> {
      static constexpr size_t value = 1;
    };

    template <class T>
    struct count_new_axis<T> {
      static constexpr size_t value = 0;
    };

    template <class T0, class... T>
    struct count_new_axis<T0, T...> {
      static constexpr size_t value =
          count_new_axis<T0>::value + count_new_axis<T...>::value;
    };

    /* helper to turn a new axis into a slice
     */
    template <class T>
    struct to_slice {
      using type = T;
      static constexpr bool is_new_axis = false;
      T operator()(T value);
    };

    template <>
    struct to_slice<none_type> {
      using type = contiguous_slice;
      static constexpr bool is_new_axis = true;
      contiguous_slice operator()(none_type);
    };

    /* helper to build a new shape out of a shape and a slice with new axis
     */
    template <size_t N, size_t M, size_t C>
    array<long, N> make_reshape(array<long, M> const &shape,
                                array<bool, C> const &is_new_axis);

    /* helper to build an extended slice aka numpy_gexpr out of a subscript
     */
    template <size_t C>
    struct extended_slice {
      template <class T, size_t N, class... S>
      numpy_gexpr<ndarray<T, N + C>, typename to_slice<S>::type...>
      operator()(ndarray<T, N> &&a, S const &... s);

      template <class T, size_t N, class... S>
      numpy_gexpr<ndarray<T, N + C>, typename to_slice<S>::type...>
      operator()(ndarray<T, N> const &a, S const &... s);
    };

    template <>
    struct extended_slice<0> {
      template <class T, size_t N, class... S>
      auto operator()(ndarray<T, N> &&a, long s0, S const &... s)
          -> decltype(std::declval<numpy_iexpr<ndarray<T, N>>>()(s...));

      template <class T, size_t N, class... S>
      auto operator()(ndarray<T, N> const &a, long s0, S const &... s)
          -> decltype(a[s0](s...));

      template <class T, size_t N, class... S>
      numpy_gexpr<ndarray<T, N>, slice, S...>
      operator()(ndarray<T, N> &&a, slice const &s0, S const &... s);

      template <class T, size_t N, class... S>
      numpy_gexpr<ndarray<T, N> const &, slice, S...>
      operator()(ndarray<T, N> const &a, slice const &s0, S const &... s);

      template <class T, size_t N, class... S>
      numpy_gexpr<ndarray<T, N>, contiguous_slice, S...>
      operator()(ndarray<T, N> &&a, contiguous_slice const &s0, S const &... s);

      template <class T, size_t N, class... S>
      numpy_gexpr<ndarray<T, N> const &, contiguous_slice, S...>
      operator()(ndarray<T, N> const &a, contiguous_slice const &s0,
                 S const &... s);
    };

    template <size_t L>
    struct nget;
    /* manually unrolled copy function
     */
    template <size_t I>
    struct flat_copy;

    template <>
    struct flat_copy<0> {
      void operator()(long *, long const *);
    };

    template <size_t I>
    struct flat_copy {
      void operator()(long *to, long const *from);
    };

    /* Meta-Function to count the number of slices in a type list
     */
    template <class... Types>
    struct count_long;

    template <>
    struct count_long<long> {
      static constexpr size_t value = 1;
    };

    template <>
    struct count_long<slice> {
      static constexpr size_t value = 0;
    };

    template <>
    struct count_long<contiguous_slice> {
      static constexpr size_t value = 0;
    };

    template <class T, class... Types>
    struct count_long<T, Types...> {
      static constexpr size_t value =
          count_long<T>::value + count_long<Types...>::value;
    };

    template <>
    struct count_long<> {
      static constexpr size_t value = 0;
    };

    /* helper to get the type of the nth element of an array
     */
    template <class T, size_t N>
    struct nth_value_type {
      using type = typename nth_value_type<typename T::value_type, N - 1>::type;
    };

    template <class T>
    struct nth_value_type<T, 0> {
      using type = T;
    };

    /* helper that yields true if the first slice of a pack is a contiguous
     * slice
     */
    template <class... S>
    struct is_contiguous {
      static const bool value = false;
    };

    template <class... S>
    struct is_contiguous<contiguous_slice, S...> {
      static const bool value = true;
    };

    /* numpy_gexpr factory
     *
     * replaces the constructor, in order to properly merge gexpr composition
     *into a single gexpr
     */
    namespace details
    {

      // this struct is specialized for every type combination and takes care of
      // the slice merge
      template <class T, class Tp>
      struct merge_gexpr;

      template <>
      struct merge_gexpr<std::tuple<>, std::tuple<>> {
        std::tuple<> const &operator()(std::tuple<> const &t0,
                                       std::tuple<> const &);
      };

      template <class... T0>
      struct merge_gexpr<std::tuple<T0...>, std::tuple<>> {
        std::tuple<T0...> const &operator()(std::tuple<T0...> const &t0,
                                            std::tuple<>);
      };

      template <class... T1>
      struct merge_gexpr<std::tuple<>, std::tuple<T1...>> {
        std::tuple<T1...> const &operator()(std::tuple<>,
                                            std::tuple<T1...> const &t1);
      };

      template <class S0, class... T0, class S1, class... T1>
      struct merge_gexpr<std::tuple<S0, T0...>, std::tuple<S1, T1...>> {
        auto operator()(std::tuple<S0, T0...> const &t0,
                        std::tuple<S1, T1...> const &t1)
            -> decltype(std::tuple_cat(
                std::make_tuple(std::get<0>(t0) * std::get<0>(t1)),
                merge_gexpr<std::tuple<T0...>, std::tuple<T1...>>{}(
                    tuple_tail(t0), tuple_tail(t1))));
      };

      template <class... T0, class S1, class... T1>
      struct merge_gexpr<std::tuple<long, T0...>, std::tuple<S1, T1...>> {
        auto operator()(std::tuple<long, T0...> const &t0,
                        std::tuple<S1, T1...> const &t1)
            -> decltype(std::tuple_cat(
                std::make_tuple(std::get<0>(t0)),
                merge_gexpr<std::tuple<T0...>, std::tuple<S1, T1...>>{}(
                    tuple_tail(t0), t1)));
      };

      template <class S0, class... T0, class... T1>
      struct merge_gexpr<std::tuple<S0, T0...>, std::tuple<long, T1...>> {
        auto operator()(std::tuple<S0, T0...> const &t0,
                        std::tuple<long, T1...> const &t1)
            -> decltype(std::tuple_cat(
                std::make_tuple(std::get<0>(t1) * std::get<0>(t0).step +
                                std::get<0>(t0).lower),
                merge_gexpr<std::tuple<T0...>, std::tuple<T1...>>{}(
                    tuple_tail(t0), tuple_tail(t1))));
      };

      template <class... T0, class... T1>
      struct merge_gexpr<std::tuple<long, T0...>, std::tuple<long, T1...>> {
        auto operator()(std::tuple<long, T0...> const &t0,
                        std::tuple<long, T1...> const &t1)
            -> decltype(std::tuple_cat(
                std::make_tuple(std::get<0>(t0)),
                merge_gexpr<std::tuple<T0...>, std::tuple<long, T1...>>{}(
                    tuple_tail(t0), t1)));
      };

      template <class Arg, class... Sp>
      numpy_gexpr<Arg, Sp...> _make_gexpr(Arg arg, std::tuple<Sp...> const &t);

      template <class Arg, class... S>
      struct make_gexpr {
        numpy_gexpr<Arg, S...> operator()(Arg arg, S const &... s);
      };

      // this specialization is in charge of merging gexpr
      template <class Arg, class... S, class... Sp>
      struct make_gexpr<numpy_gexpr<Arg, S...> const &, Sp...> {
        auto operator()(numpy_gexpr<Arg, S...> const &arg, Sp const &... s)
            -> decltype(
                _make_gexpr(std::declval<Arg>(),
                            merge_gexpr<std::tuple<S...>, std::tuple<Sp...>>{}(
                                std::tuple<S...>(), std::tuple<Sp...>())));
      };
    }

    template <class Arg, class... S>
    auto make_gexpr(Arg &&arg, S const &... s)
        -> decltype(details::make_gexpr<Arg, S...>{}(std::forward<Arg>(arg),
                                                     s...));

    /* type-based compile time overlapping detection: detect if a type may
     *overlap with another
     * the goal is to detect whether the following operation
     *
     * a[...] = b
     *
     * requires a copy.
     *
     * It requires a copy if b = a[...], as in
     *
     * a[1:] = a[:-1]
     *
     * because this is *not* equivalent to for i in range(0, n-1): a[i+1] = a[i]
     *
     * to avoid the copy, we rely on the lhs type
     */

    template <class E>
    struct may_overlap_gexpr
        : std::integral_constant<bool, not is_dtype<E>::value> {
    };

    template <class E>
    struct may_overlap_gexpr<E &> : may_overlap_gexpr<E> {
    };

    template <class E>
    struct may_overlap_gexpr<E const &> : may_overlap_gexpr<E> {
    };

    template <class T, size_t N>
    struct may_overlap_gexpr<ndarray<T, N>> : std::false_type {
    };

    template <class E>
    struct may_overlap_gexpr<numpy_iexpr<E>> : may_overlap_gexpr<E> {
    };

    template <class E>
    struct may_overlap_gexpr<numpy_texpr<E>> : may_overlap_gexpr<E> {
    };

    template <class E>
    struct may_overlap_gexpr<list<E>>
        : std::integral_constant<bool, not is_dtype<E>::value> {
    };

    template <class E, size_t N>
    struct may_overlap_gexpr<array<E, N>> : may_overlap_gexpr<E> {
    };

    template <class Op, class... Args>
    struct may_overlap_gexpr<numpy_expr<Op, Args...>>
        : utils::any_of<may_overlap_gexpr<Args>::value...> {
    };

    template <class Arg, class... S>
    struct numpy_gexpr_helper;

    /* Expression template for numpy expressions - extended slicing operators
     */
    template <class Arg, class... S>
    struct numpy_gexpr {
      // numpy_gexpr is a wrapper for extended sliced array around a numpy
      // expression.
      // It contains compacted sorted slices value in lower, step and upper is
      // the same as shape.
      // indices for long index are store in the indices array.
      // position for slice and long value in the extended slice can be found
      // through the S... template
      // and compacted values as we know that first S is a slice.

      using dtype = typename std::remove_reference<Arg>::type::dtype;
      static constexpr size_t value =
          std::remove_reference<Arg>::type::value - count_long<S...>::value;

      // It is not possible to vectorize everything. We only vectorize if the
      // last dimension is contiguous, which happens if
      // 1. Arg is an ndarray (this is too strict)
      // 2. the size of the gexpr is lower than the dim of arg, or it's the
      // same, but the last slice is contiguous
      static const bool is_vectorizable =
          std::is_same<ndarray<typename std::remove_reference<Arg>::type::dtype,
                               std::remove_reference<Arg>::type::value>,
                       typename std::remove_cv<typename std::remove_reference<
                           Arg>::type>::type>::value and
          (sizeof...(S) < std::remove_reference<Arg>::type::value or
           std::is_same<contiguous_slice,
                        typename std::tuple_element<
                            sizeof...(S)-1, std::tuple<S...>>::type>::value);
      static const bool is_strided =
          std::remove_reference<Arg>::type::is_strided or
          (((sizeof...(S)-count_long<S...>::value) == value) and
           not std::is_same<
               contiguous_slice,
               typename std::tuple_element<sizeof...(S)-1,
                                           std::tuple<S...>>::type>::value);

      using value_type = typename std::remove_reference<decltype(
          numpy_gexpr_helper<Arg, S...>::get(std::declval<numpy_gexpr>(),
                                             0L))>::type;

      using iterator =
          typename std::conditional<is_strided or value != 1,
                                    nditerator<numpy_gexpr>, dtype *>::type;
      using const_iterator =
          typename std::conditional<is_strided or value != 1,
                                    const_nditerator<numpy_gexpr>,
                                    dtype const *>::type;

      Arg arg;
      std::tuple<S...> slices;
      dtype *buffer;
      array<long, value> _shape;
      array<long, value> const &shape() const;
      array<long, value> lower;
      array<long, value> step;
      array<long, std::remove_reference<Arg>::type::value - value> indices;

      numpy_gexpr();
      numpy_gexpr(numpy_gexpr const &) = default;
      numpy_gexpr(numpy_gexpr &&) = default;

      template <class Argp> // not using the default one, to make it possible to
      // accept reference and non reference version of
      // Argp
      numpy_gexpr(numpy_gexpr<Argp, S...> const &other);

      template <size_t J, class Slice>
      typename std::enable_if<std::is_same<Slice, slice>::value or
                                  std::is_same<Slice, contiguous_slice>::value,
                              void>::type
      init_shape(std::tuple<S const &...> const &values, Slice const &s,
                 utils::int_<1>, utils::int_<J>);

      template <size_t I, size_t J, class Slice>
      typename std::enable_if<std::is_same<Slice, slice>::value or
                                  std::is_same<Slice, contiguous_slice>::value,
                              void>::type
      init_shape(std::tuple<S const &...> const &values, Slice const &s,
                 utils::int_<I>, utils::int_<J>);

      template <size_t J>
      void init_shape(std::tuple<S const &...> const &values, long cs,
                      utils::int_<1>, utils::int_<J>);

      template <size_t I, size_t J>
      void init_shape(std::tuple<S const &...> const &values, long cs,
                      utils::int_<I>, utils::int_<J>);

      // private because we must use the make_gexpr factory to create a gexpr
    private:
      template <class _Arg, class... _S>
      friend struct details::make_gexpr;

      template <size_t C>
      friend struct extended_slice;

      template <typename T>
      friend struct ::pythonic::from_python;

      // When we create a new numpy_gexpr, we deduce step, lower and shape from
      // slices
      // and indices from long value.
      // Also, last shape information are set from origin array like in :
      // >>> a = numpy.arange(2*3*4).reshape(2,3,4)
      // >>> a[:, 1]
      // the last dimension (4) is missing from slice information
      // Finally, if origin expression was already sliced, lower bound and step
      // have to
      // be increased
      numpy_gexpr(Arg const &arg, std::tuple<S const &...> const &values);
      numpy_gexpr(Arg const &arg, S const &... s);

    public:
      template <class Argp, class... Sp>
      numpy_gexpr(numpy_gexpr<Argp, Sp...> const &expr, Arg arg);

      template <class G>
      numpy_gexpr(G const &expr, Arg &&arg);

      template <class E>
      typename std::enable_if<may_overlap_gexpr<E>::value, numpy_gexpr &>::type
      _copy(E const &expr);

      template <class E>
      typename std::enable_if<not may_overlap_gexpr<E>::value,
                              numpy_gexpr &>::type
      _copy(E const &expr);

      template <class E>
      numpy_gexpr &operator=(E const &expr);

      numpy_gexpr &operator=(numpy_gexpr const &expr);

      template <class Op, class E>
      typename std::enable_if<may_overlap_gexpr<E>::value, numpy_gexpr &>::type
      update_(E const &expr);

      template <class Op, class E>
      typename std::enable_if<not may_overlap_gexpr<E>::value,
                              numpy_gexpr &>::type
      update_(E const &expr);

      template <class E>
      numpy_gexpr &operator+=(E const &expr);

      numpy_gexpr &operator+=(numpy_gexpr const &expr);

      template <class E>
      numpy_gexpr &operator-=(E const &expr);

      numpy_gexpr &operator-=(numpy_gexpr const &expr);

      template <class E>
      numpy_gexpr &operator*=(E const &expr);

      numpy_gexpr &operator*=(numpy_gexpr const &expr);

      template <class E>
      numpy_gexpr &operator/=(E const &expr);

      numpy_gexpr &operator/=(numpy_gexpr const &expr);

      const_iterator begin() const;
      const_iterator end() const;

      iterator begin();
      iterator end();

      auto fast(long i) const
          -> decltype(numpy_gexpr_helper<Arg, S...>::get(*this, i));

      auto fast(long i)
          -> decltype(numpy_gexpr_helper<Arg, S...>::get(*this, i));

#ifdef USE_BOOST_SIMD
      using simd_iterator = const_simd_nditerator<numpy_gexpr>;
      simd_iterator vbegin() const;
      simd_iterator vend() const;
      template <class I>
      auto load(I i) const
          -> decltype(boost::simd::load<boost::simd::pack<dtype>>(this->buffer,
                                                                  i));

      template <class V>
      void store(V &&v, long i);

#endif

      auto operator[](long i) const -> decltype(this->fast(i));
      auto operator[](long i) -> decltype(this->fast(i));
      auto operator()(long i) const -> decltype((*this)[i]);
      auto operator()(long i) -> decltype((*this)[i]);

      template <class... Sp>
      auto operator()(contiguous_slice const &s0, Sp const &... s) const
          -> decltype(make_gexpr(*this, s0, s...));

      auto operator[](contiguous_slice const &s0) const
          -> decltype(make_gexpr(*this, s0));

      template <class... Sp>
      auto operator()(slice const &s0, Sp const &... s) const
          -> decltype(make_gexpr(*this, s0, s...));

      auto operator[](slice const &s0) const -> decltype(make_gexpr(*this, s0));

      template <class... Sp>
      auto operator()(long i, Sp const &... s) const
          -> decltype(std::declval<value_type>()(s...));

      template <size_t M>
      auto operator[](array<long, M> const &indices) const
          & -> decltype(nget<M - 1>()(*this, indices));

      template <size_t M>
          auto operator[](array<long, M> const &indices) &&
          -> decltype(nget<M - 1>()(std::move(*this), indices));

      template <class F>
      typename std::enable_if<is_numexpr_arg<F>::value,
                              numpy_fexpr<numpy_gexpr, F>>::type
      fast(F const &filter) const;

      template <class F>
      typename std::enable_if<is_numexpr_arg<F>::value,
                              numpy_fexpr<numpy_gexpr, F>>::type
      operator[](F const &filter) const;

      long flat_size() const;
    };

    // As gexpr has to begin with a slice. When we access it, we need to forward
    // the firsts accessed information
    // to an iexpr until the value in S... is a slice again.
    template <class Arg, class S0, class S1, class... S>
    struct numpy_gexpr_helper<Arg, S0, S1, S...> {
      using type = numpy_gexpr<numpy_iexpr<Arg const &>, S1, S...>;
      static type get(numpy_gexpr<Arg, S0, S1, S...> const &e, long i);
      static type get(numpy_gexpr<Arg, S0, S1, S...> &e, long i);
    };

    // Compute forwarding of "long" index to iexpr until we reach a new slice in
    // S... parameters
    namespace
    {

      template <size_t N, class Arg, class... S>
      struct finalize_numpy_gexpr_helper;

      // We reach a new slice so we have a new gexpr
      template <size_t N, class Arg, class... S>
      struct finalize_numpy_gexpr_helper<N, Arg, contiguous_slice, S...> {
        using type = numpy_gexpr<Arg, contiguous_slice, S...>;
        template <class E, class F>
        static type get(E const &e, F &&f);
      };

      // We reach a new slice so we have a new gexpr
      template <size_t N, class Arg, class... S>
      struct finalize_numpy_gexpr_helper<N, Arg, slice, S...> {

        using type = numpy_gexpr<Arg, slice, S...>;
        template <class E, class F>
        static type get(E const &e, F &&f);
      };

      // We have a long index so it is not a gexpr but a iexpr.
      // We declare a new iexpr and we continue looking for a new slice.
      template <size_t N, class Arg, class... S>
      struct finalize_numpy_gexpr_helper<N, Arg, long, S...> {
        template <class E, class F>
        static auto get(E const &e, F &&f)
            -> decltype(finalize_numpy_gexpr_helper<
                N + 1, numpy_iexpr<Arg const &>,
                S...>::get(e, std::declval<numpy_iexpr<Arg const &>>()));

        template <class E, class F>
        static auto get(E &e, F &&f) -> decltype(finalize_numpy_gexpr_helper<
            N + 1, numpy_iexpr<Arg const &>,
            S...>::get(e, std::declval<numpy_iexpr<Arg const &> &>()));
      };

      // If it was a single sliced array, we can return the matching iexpr.
      template <size_t N, class Arg>
      struct finalize_numpy_gexpr_helper<N, Arg, long> {
        template <class E, class F>
        static auto get(E const &e, F const &f)
            -> decltype(numpy_iexpr_helper<F, Arg::value>::get(f, 0));

        template <class E, class F>
        static auto get(E const &e, F &f)
            -> decltype(numpy_iexpr_helper<F, Arg::value>::get(f, 0));
      };
    }

    // forward new combination of gexpr/iexpr computation to
    // finalize_numpy_gexpr_helper
    template <class Arg, class S0, class... S>
    struct numpy_gexpr_helper<Arg, S0, long, S...> {
      static auto get(numpy_gexpr<Arg, S0, long, S...> const &e, long i)
          -> decltype(finalize_numpy_gexpr_helper<
              0, numpy_iexpr<Arg const &>, long,
              S...>::get(e, std::declval<numpy_iexpr<Arg const &>>()));

      static auto get(numpy_gexpr<Arg, S0, long, S...> &e, long i)
          -> decltype(finalize_numpy_gexpr_helper<
              0, numpy_iexpr<Arg const &>, long,
              S...>::get(e, std::declval<numpy_iexpr<Arg const &> &>()));
    };

    // If we have no more slice later, we can say it is an iexpr (We look only
    // for one last long? Not many last long?)
    template <class Arg, class S>
    struct numpy_gexpr_helper<Arg, S, long> {
      static auto get(numpy_gexpr<Arg, S, long> const &e, long i)
          -> decltype(numpy_iexpr_helper<numpy_iexpr<Arg const &>,
                                         numpy_iexpr<Arg const &>::value>::
                          get(std::declval<numpy_iexpr<Arg const &>>(), 0));

      static auto get(numpy_gexpr<Arg, S, long> &e, long i)
          -> decltype(numpy_iexpr_helper<numpy_iexpr<Arg const &>,
                                         numpy_iexpr<Arg const &>::value>::
                          get(std::declval<numpy_iexpr<Arg const &> &>(), 0));
    };

    template <class Arg, class S>
    struct numpy_gexpr_helper<Arg, S>
        : numpy_iexpr_helper<numpy_gexpr<Arg, S>, numpy_gexpr<Arg, S>::value> {
    };
  }

  template <class Arg, class... S>
  struct returnable<types::numpy_gexpr<Arg, S...>> {
    using type = types::numpy_gexpr<typename returnable<Arg>::type, S...>;
  };

  template <class T, size_t N, class... S>
  struct assignable<types::numpy_gexpr<types::ndarray<T, N> const &, S...>> {
    using type = types::numpy_gexpr<types::ndarray<T, N> const &, S...>;
  };

  template <class T, size_t N, class... S>
  struct assignable<types::numpy_gexpr<types::ndarray<T, N> &, S...>> {
    using type = types::numpy_gexpr<types::ndarray<T, N> &, S...>;
  };

  template <class Arg, class... S>
  struct assignable<types::numpy_gexpr<Arg, S...>> {
    using type = types::numpy_gexpr<typename assignable<Arg>::type, S...>;
  };

  template <class Arg, class... S>
  struct lazy<types::numpy_gexpr<Arg, S...>> {
    using type =
        types::numpy_gexpr<typename lazy<Arg>::type, typename lazy<S>::type...>;
  };

  template <class Arg, class... S>
  struct lazy<types::numpy_gexpr<Arg const &, S...>> {
    using type = types::numpy_gexpr<typename lazy<Arg>::type const &,
                                    typename lazy<S>::type...>;
  };
}
/* type inference stuff  {*/
#include "pythonic/include/types/combined.hpp"

template <class Arg, class... S>
struct __combined<pythonic::types::numpy_gexpr<Arg, S...>,
                  pythonic::types::numpy_gexpr<Arg, S...>> {
  using type = pythonic::types::numpy_gexpr<Arg, S...>;
};

template <class Arg, class... S, class Argp, class... Sp>
struct __combined<pythonic::types::numpy_gexpr<Arg, S...>,
                  pythonic::types::numpy_gexpr<Argp, Sp...>> {
  using type = pythonic::types::numpy_gexpr<Arg, S...>;
};

template <class Arg, class... S, class O>
struct __combined<pythonic::types::numpy_gexpr<Arg, S...>, O> {
  using type = pythonic::types::numpy_gexpr<Arg, S...>;
};

template <class Arg, class... S, class O>
struct __combined<O, pythonic::types::numpy_gexpr<Arg, S...>> {
  using type = pythonic::types::numpy_gexpr<Arg, S...>;
};

/* combined are sorted such that the assigned type comes first */
template <class Arg, class... S, class T, size_t N>
struct __combined<pythonic::types::numpy_gexpr<Arg, S...>,
                  pythonic::types::ndarray<T, N>> {
  using type = pythonic::types::numpy_gexpr<Arg, S...>;
};

template <class Arg, class... S, class T, size_t N>
struct __combined<pythonic::types::ndarray<T, N>,
                  pythonic::types::numpy_gexpr<Arg, S...>> {
  using type = pythonic::types::ndarray<T, N>;
};

#endif
