#ifndef LZ_STL_TYPE_TRAITS_H
#define LZ_STL_TYPE_TRAITS_H

/*
什么是POD类型？ 为了与c兼容的数据类型
1.平凡性（Trivial）
	有平凡的默认构造函数（即不自定义构造函数，或使用 = default 显式生成）。
	有平凡的拷贝 / 移动构造函数和拷贝 / 移动赋值运算符（同样不自定义，或显式默认）。
	有平凡的析构函数（不自定义析构函数，或显式默认）。
2.标准布局（Standard Layout）
	所有非静态成员有相同的访问控制（如全为 public）。
	没有虚函数或虚基类。
	所有基类也必须是标准布局类型。
	非静态成员的类型及其基类的类型不能有相同的嵌套类型。

常见的 POD 类型
基本数据类型（如 int、char、double 等）。
符合上述条件的结构体 / 联合体（如仅包含基本类型成员，无自定义特殊成员函数）。
指针类型、数组（元素为 POD 类型时）。

为什么要POD？
内存操作安全，可以用c的 memcpy 来复制POD对象
与C 兼容
*/

/*
为什么要设计type_traits?
traits之前 不得不对 不同类型数据 编写大量的特化代码
	这种方式需要为每种 POD 类型手动特化，代码量爆炸，且无法处理用户自定义的 POD 类型（如 struct { int a; char b; }）。

type_traits:在编译期萃取类型特性，实现基于类型的条件优化”
类型特性（如 “是否为 POD”）是类型的固有属性，可以通过模板特化在编译期 “标记” 这些属性

核心作用：
1. 判断类型是否具有特定特性（如是否为POD、是否有平凡构造/析构等）
2. 对类型进行转换（如移除const/volatile修饰符）
3. 分类基础类型（整数、浮点等）
这些信息用于泛型代码的条件优化，体现STL"零成本抽象"哲学
*/

namespace lzstl
{
	// 1. 基础标签类型（用于编译期分支判断）
	struct true_type {static const bool value = true;};
	struct false_type {static const bool value = false;};
	
	// 2.类型特性主模板 默认设为 false 更安全
	template <typename T>
	struct type_traits
	{
		//memcpy 的安全性取决于 “拷贝行为是否平凡”
		//平凡性  与 POD（平凡性且标准布局）
		typedef false_type		has_trivial_default_constructor;
		typedef false_type		has_trivial_copy_constructor;
		typedef false_type      has_trivial_assignment_operator;   // 平凡赋值运算符
		typedef false_type  	has_trivial_destructor;
		typedef false_type      is_POD_type;
	};
	
	// 3.对基本类型的特化  C++98风格，需逐个特化
	// 3.1 bool
	template <>
	struct type_traits<bool>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	// 3.2 char
	template <>
	struct type_traits<char>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <>
	struct type_traits<signed char>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <>
	struct type_traits<unsigned char>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	// 3.3 short
	template <>
	struct type_traits<short>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <>
	struct type_traits<unsigned short>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	// 3.4 int
	template <>
	struct type_traits<int>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <>
	struct type_traits<unsigned int>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	// 3.5 long
	template <>
	struct type_traits<long>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <>
	struct type_traits<unsigned long>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	// 3.6 float
	template <>
	struct type_traits<float>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <>
	struct type_traits<double>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <>
	struct type_traits<long double>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	// 3.7 T*
	template <typename T>
	struct type_traits<T*>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <typename T>
	struct type_traits<const T*>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	template <typename T>
	struct type_traits<volatile T*>
	{
		typedef true_type      has_trivial_default_constructor;
		typedef true_type      has_trivial_copy_constructor;
		typedef true_type      has_trivial_assignment_operator;
		typedef true_type      has_trivial_destructor;
		typedef true_type      is_POD_type;
	};
	
	// 4. 类型分类：整数/浮点判断（算法优化常用）
	template <typename T>
	struct is_integral : public false_type {};
	
	template <>struct is_integral<bool>: public true_type{};
	template <>struct is_integral<char>: public true_type{};
	template <>struct is_integral<signed char>: public true_type{};
	template <>struct is_integral<unsigned char>: public true_type{};
	template <>struct is_integral<short>: public true_type{};
	template <>struct is_integral<unsigned short>: public true_type{};
	template <>struct is_integral<int>: public true_type{};
	template <>struct is_integral<unsigned int>: public true_type{};
	template <>struct is_integral<long>: public true_type{};
	template <>struct is_integral<unsigned long>: public true_type{};
	
	template <typename T>
	struct is_floating_point : public false_type{};
	
	template <>struct is_floating_point<float> : public true_type{};
	template <>struct is_floating_point<double> : public true_type{};
	template <>struct is_floating_point<long double> : public true_type{};
	
	// 5. 类型转换：移除const/volatile修饰符（泛型编程必备）
	template <typename T>
	struct remove_const {typedef T type;};
	
	template <typename T>
	struct remove_const<const T> {typedef T type;};
	
	template <typename T>
	struct remove_volatile {typedef T type;};
	
	template <typename T>
	struct remove_volatile<volatile T> {typedef T type;};
	
	template <typename T>
	struct remove_cv
	{
		typedef typename remove_volatile<typename remove_const<T>::type>::type type;
	};
	
	// 6. 便捷接口：简化特性萃取调用
	/*
	先通过 type_traits<T> 萃取 T 的 is_POD_type 特性（得到 true_type 或 false_type）；
	再访问这个特性类型中定义的静态常量 value，得到 true 或 false。
	*/
	template <typename T>
	inline bool is_pod_type() 
	{
		return type_traits<T>::is_POD_type::value;
	}
	
	template <typename T>
	inline bool has_trivial_default_constructor() 
	{
		return type_traits<T>::has_trivial_default_constructor::value;
	}
	
	template <typename T>
	inline bool has_trivial_copy_constructor() 
	{
		return type_traits<T>::has_trivial_copy_constructor::value;
	}
	
	template <typename T>
	inline bool has_trivial_assignment_operator() 
	{
		return type_traits<T>::has_trivial_assignment_operator::value;
	}

	template <typename T>
	inline bool has_trivial_destructor() 
	{
		return type_traits<T>::has_trivial_destructor::value;
	}
	
	template <typename T>
	inline bool is_integral_type()
	{
		return is_integral<T>::value;
	}
	
	template <typename T>
	inline bool is_floating_point_type()
	{
		return is_floating_point<T>::value;
	}
}
#endif 
