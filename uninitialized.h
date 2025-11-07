#ifndef LZ_STL_UNINITIALIZED_H
#define LZ_STL_UNINITIALIZED_H

/*
未初始化内存操作工具 uninitialized_* 系列函数
为什么需要？
通过 alloc 分配空间后，需要construct 构造初始化对象了
该系列函数是这两步的中间
接收为初始化内存的起始地址和 “要构造的元素”，在指定位置构造对象
结合 type_traits 判断元素类型是否为 POD，若为 POD 则直接用 memcpy 等高效内存操作，否则逐个调用构造函数（保证正确性）。
*/

#include "type_traits.h"
#include "alloc.h"
#include "iterator.h"
#include "construct.h"

namespace lzstl
{
	// 提前声明辅助函数（关键：在使用前声明）
	template <typename InputIterator, typename ForwardIterator, typename T>
	ForwardIterator __uninitialized_copy(InputIterator first, InputIterator last, ForwardIterator result, T*);
	
	template <typename ForwardIterator, typename T>
	void __uninitialized_fill(ForwardIterator first, ForwardIterator last, const T& value);
	
	template <typename InputIterator, typename ForwardIterator>
	ForwardIterator __uninitialized_move(InputIterator first, InputIterator last, ForwardIterator result);
	
	template <typename ForwardIterator, typename Size, typename T>
	ForwardIterator __uninitialized_fill_n(ForwardIterator first, Size n, const T& value);
	
	// 1. uninitialized_copy：将[first, last)复制到未初始化内存[result, ...)
	template <typename InputIterator,typename ForwardIterator>
	ForwardIterator uninitialized_copy(InputIterator first,InputIterator last,ForwardIterator result)
	{
		typedef typename iterator_traits<InputIterator>::value_type value_type;
		//POD类型
		if(type_traits<value_type>::is_POD_type::value)
		{
			size_t n = distance(first,last);
			std::memcpy(&*result,&*first,n* sizeof(value_type));
			return result + n;
		}
		else
		{
			return __uninitialized_copy(first,last,result,static_cast<value_type*>(nullptr));
		}
	}
	// 辅助函数 处理 非POD
	template <typename InputIterator,typename ForwardIterator,typename T>
	ForwardIterator __uninitialized_copy(InputIterator first,InputIterator last,ForwardIterator result,T*)
	{
		ForwardIterator cur = result;
		try
		{
			// 逐个构造非 POD 类型
			for(;first!=last;++first,++cur)
				construct(&*cur,*first);
		}
		catch(...)
		{
			destroy(result,cur);
			throw;
		}
		return cur;
	}
	
	// 2. uninitialized_fill：在未初始化内存[first, last)中填充value
	template <typename ForwardIterator,typename T>
	void uninitialized_fill(ForwardIterator first,ForwardIterator last,const T& value)
	{
		typedef typename iterator_traits<ForwardIterator>::value_type value_type;
		if(type_traits<value_type>::is_POD_type::value)
		{
			for(;first!=last;++first)
				*first = value;
		}
		else
		{
			__uninitialized_fill(first,last,value);
		}
	}
	// 辅助函数：非POD类型的uninitialized_fill
	template <typename ForwardIterator,typename T>
	void __uninitialized_fill(ForwardIterator first,ForwardIterator last,const T& value)
	{
		ForwardIterator cur = first;
		try
		{
			for(;cur!=last;++cur)
				construct(&*cur,value);
		}
		catch(...)
		{
			destroy(first,cur);
			throw;
		}
	}
	
	// 3. uninitialized_fill_n：在未初始化内存[first, first + n)中填充n个value
	template <typename ForwardIterator, typename Size, typename T>
	ForwardIterator uninitialized_fill_n(ForwardIterator first, Size n, const T& value) 
	{
		typedef typename iterator_traits<ForwardIterator>::value_type value_type;
		if (type_traits<value_type>::is_POD_type::value) 
		{
			// POD类型：直接循环赋值（无需构造）
			ForwardIterator cur = first;
			for (Size i = 0; i < n; ++i, ++cur) 
				*cur = value;
			return cur;
		} 
		else 
		{
			// 非POD类型：逐个构造n个对象
			return __uninitialized_fill_n(first, n, value);
		}
	}
	
	// 辅助函数：非POD类型的uninitialized_fill_n
	template <typename ForwardIterator, typename Size, typename T>
	ForwardIterator __uninitialized_fill_n(ForwardIterator first, Size n, const T& value) {
		ForwardIterator cur = first;
		try 
		{
			for (Size i = 0; i < n; ++i, ++cur)
				construct(&*cur, value);  // 调用构造函数
		}
		catch (...)
		{
			destroy(first, cur);  // 异常安全：销毁已构造对象
			throw;
		}
		return cur;
	}
	
	// 4. uninitialized_move：将[first, last)移动到未初始化内存[result, ...)
	template <typename InputIterator,typename ForwardIterator>
	ForwardIterator uninitialized_move(InputIterator first,InputIterator last,ForwardIterator result)
	{
		typedef typename iterator_traits<InputIterator>::value_type value_type;
		if(type_traits<value_type>::is_POD_type::value)
		{
			return uninitialized_copy(first,last,result);
		}
		else
		{
			return __uninitialized_move(first,last,result);
		}
	}
	
	// 辅助函数
	template <typename InputIterator,typename ForwardIterator>
	ForwardIterator __uninitialized_move(InputIterator first,InputIterator last,ForwardIterator result)
	{
		ForwardIterator cur = result;
		try
		{
			for(;first!=last;++first,++cur)
				construct(&*cur,std::move(*first));
		}
		catch(...)
		{
			destroy(result,cur);
			throw;
		}
		return cur;
	}
}

#endif 
