#pragma once
#include <vector>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include <utility>
#include <tuple>
#include <type_traits>

//可平凡复制 
template <typename T, typename std::enable_if<std::is_trivially_copyable<T>::value, int>::type N = 0>
void Serialize(std::ostream & os, const T & val)  { 
    os.write((const char *)&val,sizeof(T)); 
} 

//pair
template <typename K, typename V>
void Serialize(std::ostream & os, const std::pair<K, V> & val) {
	Serialize(os, val.first);
	Serialize(os, val.second);
}

// std::string
void Serialize(std::ostream & os, const std::string &val) {
	size_t size = val.size();
	os.write((const char *)&size, sizeof(size));
	os.write((const char *)val.data(), size * sizeof(typename std::string::value_type));
}

//容器 
template <typename T, typename std::enable_if< std::is_same<typename std::iterator_traits<typename T::iterator>::value_type, typename T::value_type>::value
    , int>::type N = 0> 
void Serialize(std::ostream & os, const T & val) {
	size_t size = val.size();
    os.write((const char *)&size, sizeof(size_t));
    for (auto & v : val) {
        Serialize(os, v); 
    } 
}


//tuple
template<typename T>
int _Serialize(std::ostream & os, T& val) {
	Serialize(os, val);
	return 0;
}

template<typename Tuple, std::size_t... I>
void _Serialize(std::ostream & os, Tuple& tup, std::index_sequence<I...>) {
	std::initializer_list<int>{_Serialize(os, std::get<I>(tup))...};
}


template <typename...Args>
void Serialize(std::ostream & os, const std::tuple<Args...>& val) {
	_Serialize(os, val, std::make_index_sequence<sizeof...(Args)>{});
}


/*************************************** 反序列化 ***************************************/

// 可平凡复制 
template <typename T,typename std::enable_if<std::is_trivially_copyable<T>::value, int>::type N = 0>
void Deserialize(std::istream & is, T & val) { 
     is.read((char *)&val, sizeof(T)); 
}

// std::string
void Deserialize(std::istream & is, std::string &val) {
	size_t size = 0; 
	is.read((char*)&size, sizeof(size_t));
	val.resize(size);
	auto count = size * sizeof(typename std::string::value_type);
	is.read((char *)val.data(), count);
}


//pair
template <typename V>
struct ImpDeserialize {
	template<typename T>
	static void get(std::istream & is, T & val) {
		size_t size = 0;
		is.read((char *)&size, sizeof(size_t));
		for (size_t i = 0; i < size; i++)
		{
			V v;
			Deserialize(is, v);
			val.insert(val.end(), v);
		}
	}
};

template <typename K, typename V>
struct ImpDeserialize<std::pair<K, V>> {
	template<typename T>
	static void get(std::istream & is, T & val) {
		size_t size = 0;
		is.read((char *)&size, sizeof(size_t));
		for (size_t i = 0; i < size; i++) {
			K k;
			V v;
			Deserialize(is, k);
			Deserialize(is, v);
			val.emplace(k, v);
		}
		
	}
};

// 容器 
template <typename T, typename std::enable_if< std::is_same<typename std::iterator_traits<typename T::iterator>::value_type, typename T::value_type>::value
	, int>::type N = 0>
void Deserialize(std::istream & is, T & val) {
	ImpDeserialize<typename  T::value_type>::get(is, val);
}


//tuple
template<typename T>
int _Deserialize(std::istream & is, T& val) {
	Deserialize(is, val);
	return 0;
}

template<typename Tuple, std::size_t... I>
void _Deserialize(std::istream & is, Tuple& tup, std::index_sequence<I...>) {
	std::initializer_list<int>{_Deserialize(is, std::get<I>(tup))...};
}


template <typename...Args>
void Deserialize(std::istream & is, std::tuple<Args...>& val) {
	_Deserialize(is, val, std::make_index_sequence<sizeof...(Args)>{});
}