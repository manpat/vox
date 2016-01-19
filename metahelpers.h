#ifndef METAPROGRAMMINGHELPERS_H
#define METAPROGRAMMINGHELPERS_H

template<size_t C, class F, class... T>
struct ArgumentPackImpl {
	using type = std::tuple<F, T...>;
};
template<class F, class... T>
struct ArgumentPackImpl<1, F, T...> {
	using type = F;
};

template<class F, class... T>
struct CountPack {
	enum{ value =  CountPack<T...>::value+1 };
};
template<class F>
struct CountPack<F> {
	enum{ value = 1 };
};

template<class... T>
using ArgumentPack = typename ArgumentPackImpl<CountPack<T...>::value, T...>::type;

#endif