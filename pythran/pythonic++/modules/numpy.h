#ifndef PYTHONIC_MODULE_NUMPY_H
#define PYTHONIC_MODULE_NUMPY_H

#include <vector>
#include <cmath>

namespace pythonic {
    namespace numpy {

        double const pi = 3.141592653589793238462643383279502884;
        double const e = 2.718281828459045235360287471352662498;


        template<class T>
            struct finalType
            {
                typedef T Type;
            };

        template<class T>
            struct finalType<core::list<T>>
            {
                typedef typename finalType<T>::Type Type;
            };

        template<class type>
            struct depth
            {
                enum { Value = 0 };
            };

        template<class type>
            struct depth<core::list<type>>
            {
                enum { Value = 1 + depth<type>::Value };
            };

        template<class... T, class type>
            core::ndarray<type,sizeof...(T)> build_array(core::list<type> const& prev_l, type const& val, T const& ...l)
            {
                core::ndarray<type, sizeof...(l)> a({l...});
                for(int i=0;i<prev_l.size(); i++)
                    a.data->data[i] = prev_l[i];
                return a;
            }

        template<class... T, class type>
            core::ndarray<typename finalType<type>::Type, depth<core::list<type>>::Value + sizeof...(T)> build_array(core::list<core::list<type> > const &prev_l, core::list<type> const& l, T const& ...s)
            {
                core::list<type> accumul(0);
                for(size_t i=0;i<prev_l.size(); i++)
                    for(size_t j=0; j<l.size(); j++)
                        accumul.push_back(prev_l[i][j]);
                return build_array(accumul, l[0], s..., l.size());
            }

        template<class type>
            core::ndarray<typename finalType<type>::Type, depth<core::list<type>>::Value> array(core::list<type> const& l)
            {
                return build_array(l, l[0], l.size());
            }

        PROXY(pythonic::numpy, array);

        template<class... T>
            core::ndarray<double, sizeof...(T)> build_cst_array(double val, T... t)
            {
                return core::ndarray<double, sizeof...(t)>({t...}, val);
            }

        template<int N>
            struct apply_to_tuple
            {
                template<typename... T, typename... S>
                    static core::ndarray<double, sizeof...(T)> builder(double val, std::tuple<T...> const& t, S... s)
                    {
                        return apply_to_tuple<N-1>::builder(val, t, std::get<N-1>(t), s...);
                    }
            };

        template<>
            struct apply_to_tuple<0>
            {
                template<typename... T, typename... S>
                    static core::ndarray<double, sizeof...(T)> builder(double val, std::tuple<T...> const& t, S... s)
                    {
                        return build_cst_array(val, s...);
                    }
            };

#define NOT_INIT_ARRAY(NAME)\
        template<class... T>\
            core::ndarray<double, sizeof...(T)> NAME(std::tuple<T...> const& t)\
            {\
                return apply_to_tuple<sizeof...(T)-1>::builder(0, t, std::get<sizeof...(T)-1>(t));\
            }\
\
        template<unsigned long N>\
            core::ndarray<double, N> NAME(std::array<long, N> const &t)\
            {\
                return core::ndarray<double, N>(t);\
            }\
\
        core::ndarray<double,1> NAME(size_t size)\
        {\
            return core::ndarray<double, 1>({size});\
        }

NOT_INIT_ARRAY(zeros)

NOT_INIT_ARRAY(empty)


#define CST_ARRAY(NAME, VAL)\
        template<class... T>\
            core::ndarray<double, sizeof...(T)> NAME(std::tuple<T...> const& t)\
            {\
                return apply_to_tuple<sizeof...(T)-1>::builder(VAL, t, std::get<sizeof...(T)-1>(t));\
            }\
\
        template<unsigned long N>\
            core::ndarray<double, N> NAME(std::array<long, N> const &t)\
            {\
                return core::ndarray<double, N>(t);\
            }\
\
        core::ndarray<double,1> NAME(size_t size)\
        {\
            return core::ndarray<double, 1>({size}, VAL);\
        }

        PROXY(pythonic::numpy, zeros);

        CST_ARRAY(ones, 1)

        PROXY(pythonic::numpy, ones);

        PROXY(pythonic::numpy, empty);

        template<class T=long, class U, class V>
        core::ndarray<decltype(std::declval<U>()+std::declval<V>()+std::declval<T>()), 1> arange(U begin, V end, T step=T(1))
        {
            typedef decltype(std::declval<U>()+std::declval<V>()+std::declval<T>()) combined_type;
            size_t num = std::ceil((end - begin)/step);
            core::ndarray<combined_type, 1> a({num});
            if(num>0)
            {
                a[0] = begin;
                std::transform(a.data->data, a.data->data + num - 1, a.data->data + 1, std::bind(std::plus<combined_type>(), step, std::placeholders::_1));
            }
            return a;
        }

        core::ndarray<long, 1> arange(long end)
        {
            xrange xr(end);
            return core::ndarray<long, 1>(xr.begin(), xr.end());
        }

        PROXY(pythonic::numpy, arange);

        core::ndarray<double, 1> linspace(double start, double stop, size_t num=50, bool endpoint = true)
        {
            double step = (stop - start) / (num - endpoint);
            core::ndarray<double, 1> a({num});
            if(num>0)
                a[0] = start;
            std::transform(a.data->data, a.data->data + num - 1, a.data->data + 1, std::bind(std::plus<double>(), step, std::placeholders::_1));
            return a;
        }

        PROXY(pythonic::numpy, linspace);

        template<unsigned long N, class T>
        core::ndarray<double, N> sin(core::ndarray<T, N> const& a)
        {
            return core::ndarray<double,N>(a, (double (*)(double))std::sin);
        }

        using std::sin;
        PROXY(pythonic::numpy, sin);

        template<class... T, class type>
            core::ndarray<type,sizeof...(T)> build_cst_array_from_list(type cst, core::list<type> const& prev_l, type const& val, T const& ...l)
            {
                return core::ndarray<type, sizeof...(l)>({l...}, cst);
            }

        template<class... T, class type>
            core::ndarray<typename finalType<type>::Type, depth<core::list<type>>::Value + sizeof...(T)> build_cst_array_from_list(typename finalType<type>::Type cst, core::list<core::list<type> > const &prev_l, core::list<type> const& l, T const& ...s)
            {
                return build_cst_array_from_list(cst, l, l[0], s..., (size_t)l.size());
            }

        template<class type>
            core::ndarray<typename finalType<type>::Type, depth<core::list<type>>::Value> ones_like(core::list<type> const& l)
            {
                return build_cst_array_from_list(1, l, l[0], (size_t)l.size());
            }

        PROXY(pythonic::numpy, ones_like);

        template<class... T, class type>
            core::ndarray<type,sizeof...(T)> build_not_init_array_from_list(core::list<type> const& prev_l, type const& val, T ...l)
            {
                return core::ndarray<type, sizeof...(l)>({l...});
            }

        template<class... T, class type>
            core::ndarray<typename finalType<type>::Type, depth<core::list<type>>::Value + sizeof...(T)> build_not_init_array_from_list(core::list<core::list<type> > const &prev_l, core::list<type> const& l, T ...s)
            {
                return build_not_init_array_from_list(l, l[0], s..., (size_t)l.size());
            }

#define NOT_INIT_LIKE(NAME)\
        template<class type>\
            core::ndarray<typename finalType<type>::Type, depth<core::list<type>>::Value> NAME(core::list<type> const& l)\
            {\
                return build_not_init_array_from_list(l, l[0], (size_t)l.size());\
            }

        NOT_INIT_LIKE(zeros_like)
        PROXY(pythonic::numpy, zeros_like);

        NOT_INIT_LIKE(empty_like)
        PROXY(pythonic::numpy, empty_like);

        template<class T, unsigned long N, class ...S>
            core::ndarray<T,sizeof...(S)> reshape(core::ndarray<T,N> const& array, S ...s)
            {
                long shp[] = {s...};
                return core::ndarray<T,sizeof...(s)>(array.data, 0, shp);
            }

        PROXY(pythonic::numpy, reshape);
    }
}

#endif