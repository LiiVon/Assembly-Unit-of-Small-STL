#ifndef LZ_STL_ITERATOR_H
#define LZ_STL_ITERATOR_H

#include <cstddef> //for ptrdiff_t
#include "type_traits.h"

namespace lzstl
{
	// 1. 迭代器标签（迭代器分类）          空类，仅作为标签
	struct input_iterator_tag{};			// 输入迭代器：只读，单遍
	struct output_iterator_tag{};			// 输出迭代器：只写，单遍
	struct forward_iterator_tag :public input_iterator_tag{};   //++
	struct bidirectional_iterator_tag :public forward_iterator_tag{}; //++ -- 
	struct random_access_iterator_tag :public bidirectional_iterator_tag{}; //+=n -=n
	
	// 2. 迭代器基类（供自定义迭代器继承）
	//type int a
	//using a = int
	template <typename Category, 			// 迭代器类别（标签）
			typename T, 
			typename Distance = ptrdiff_t,  // 迭代器距离类型
			typename Pointer = T*,
			typename Reference = T&>
	struct iterator
	{
		typedef Category  iterator_category;
		typedef T         value_type;
		typedef Distance  difference_type;  // 两个迭代器的距离
		typedef Pointer   pointer;
		typedef Reference reference;
	};
	
	// 3. 迭代器特性萃取机（核心）
	template <typename Iterator>
	struct iterator_traits
	{
		typedef typename Iterator::iterator_category	iterator_category;
		typedef typename Iterator::value_type 			value_type;
		typedef typename Iterator::difference_type   difference_type;
		typedef typename Iterator::pointer           pointer;
		typedef typename Iterator::reference         reference;
	};
	
	// 特化：原生指针（T*）视为随机访问迭代器
	template <typename T>
	struct iterator_traits<T*>
	{
		typedef  random_access_iterator_tag		iterator_category;
		typedef  T								value_type;
		typedef  ptrdiff_t   					difference_type;
		typedef  T*         					pointer;
		typedef  T&       						eference;
	};
	
	template <typename T>
	struct iterator_traits<const T*>
	{
		typedef  random_access_iterator_tag		iterator_category;
		typedef  T								value_type;
		typedef  ptrdiff_t   					difference_type;
		typedef  const T*         				pointer;
		typedef  const T&       				eference;
	};
	
	// 4. 迭代器辅助函数（算法中常用）
	// 4.1 获取迭代器类别（用于算法的标签分发）
	template <typename Iterator>
	inline typename iterator_traits<Iterator>::iterator_category
	get_iterator_category(const Iterator&)
	{
		typedef typename iterator_traits<Iterator>::iterator_category Category;
		//() 表示创建一个 Category 类型的临时对象并返回
		//C++ 的函数参数和返回值只能是 “值”（对象），不能直接是 “类型”
		return Category();
	}
	
	// 4.2 获取迭代器距离类型（通常是ptrdiff_t）
	//返回一个 “指向迭代器距离类型的指针”，而不是距离类型的值
	template <typename Iterator>
	inline typename iterator_traits<Iterator>::difference_type
	get_distance_type(const Iterator&)
	{
		//返回 difference_type*（指针类型）
		return static_cast<typename iterator_traits<Iterator>::difference_type*>(nullptr);
	}
	
	// 4.3 获取迭代器值类型（用于创建临时对象）
	template <typename Iterator>
	inline typename iterator_traits<Iterator>::value_type*
	get_value_type(const Iterator&)
	{
		return static_cast<typename iterator_traits<Iterator>::value_type*>(nullptr);
	}
	
	// 5. 计算两个迭代器的距离（根据迭代器类型优化）
	template <typename InputIterator>
	inline typename iterator_traits<InputIterator>::difference_type
	distance(InputIterator first,InputIterator last)
	{
		return __distance(first,last,get_iterator_category(first));
	}
	
	// 辅助函数   InputIterator ++ O(n)
	template <typename InputIterator>
	inline typename iterator_traits<InputIterator>::difference_type
	__distance(InputIterator first,InputIterator last,input_iterator_tag)
	{
		typename iterator_traits<InputIterator>::difference_type n = 0;
		while(first!=last)
		{
			++first;
			++n;
		}
		return n;
	}
	
	// 辅助函数  random_access_Iterator O(1)
	template <typename RandomAccessIterator>
	inline typename iterator_traits<RandomAccessIterator>::difference_type
	__distance(RandomAccessIterator first,RandomAccessIterator last,random_access_iterator_tag)
	{
		return last - first;
	}
	
	// 6. 迭代器前进n步（根据迭代器类型优化）
	//i :需要被移动的迭代器本身（传引用是为了直接修改迭代器的位置）
	//n :需要移动的步数。Distance = ptrdiff_t 比int 更适合表达迭代器的距离，与平台相关
	template <typename InputIterator,typename Distance>
	inline void advance(InputIterator& i,Distance n)
	{
		__advance(i,n,get_iterator_category(i));
	}
	
	// 辅助函数：输入迭代器 - 只能逐个++（n必须为正）
	template <typename InputIterator,typename Distance>
	inline void __advance(InputIterator& i,Distance n,input_iterator_tag)
	{
		while(n-- > 0)
		{
			++i;
		}
	}
	
	
	// 辅助函数：双向迭代器 - 支持--（n可正可负）
	template <typename BidirectionalIterator, typename Distance>
	inline void __advance(BidirectionalIterator& i, Distance n, bidirectional_iterator_tag) 
	{
		if (n >= 0) 
			while (n-- > 0) ++i;
		else
			while (n++ < 0) --i;
	}
	
	// 辅助函数：随机访问迭代器 - 直接+=（O(1)）
	template <typename RandomAccessIterator,typename Distance>
	inline void __advance(RandomAccessIterator& i,Distance n,random_access_iterator_tag)
	{
		i += n;
	}
}
#endif
