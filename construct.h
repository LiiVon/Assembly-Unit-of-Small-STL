#ifndef LZ_STL_CONSTRUCT_H
#define LZ_STL_CONSTRUCT_H

#include <new>
#include <utility>
#include "alloc.h"
#include "type_traits.h"
#include "iterator.h"

namespace lzstl
{
	// 1. 构造函数：在已分配的内存上构造对象
	template <typename T,typename Value>
	inline void construct(T* ptr,const Value& value)
	{
		//在 ptr 指向的内存上构造 T 类型对象，拷贝 value 初始化
		new (ptr) T(value);
	}
	
	//对象发生拷贝（如函数返回大对象、容器扩容时），会复制整个对象的资源（如堆内存、文件句柄等）
	//拷贝完成后原对象可能被销毁，导致 “先拷贝再释放” 的冗余操作。
	//移动语义：
	//直接 “掠夺” 其资源，而非拷贝，最后将原对象的资源置空
	//移动构造函数
	//完美转发：保持原参数的 左/右值性质不变
	template <typename T,typename Value>
	inline void construct(T* ptr,Value&& value)
	{
		new(ptr) T(std::forward<Value>(value));
	}
	
	template <typename T>
	inline void construct(T* ptr)
	{
		if(!ptr) return ;
		new(ptr) T();       
	}
	
	// 2. 析构函数：销毁对象（不释放内存）
	// 2.1 销毁单个对象
	template <typename T>
	inline void destroy(T* ptr)
	{
		// 显式调用 析构函数 默认或者 用户自定义都可以
		if(ptr!= nullptr)
			ptr-> ~T();
	}
	
	// 2.2 销毁范围内的对象
	template <typename ForwardIter>
	inline void destroy(ForwardIter first,ForwardIter last)
	{
		typedef typename iterator_traits<ForwardIter>::value_type value_type;
		__destroy_range(first,last,typename type_traits<value_type>::has_trivial_destructor());
	}
	
	// 辅助函数 非平凡 false_type
	template <typename ForwardIter>
	inline void __destroy_range(ForwardIter first,ForwardIter last,false_type)
	{
		// 解引用迭代器 再获取对象地址，调用单个析构
		for(;first != last;++first)
			destroy(&*first) ;
	}
	
	// 辅助函数 平凡 true_type
	template <typename ForwardIter>
	inline void __destroy_range(ForwardIter first,ForwardIter last,true_type)
	{
		//平凡析构  编译器生成的析构函数无实际操作，直接跳过
	}
	
	// 2.3 对原生字符指针的特化（优化常见场景）
	template <>
	inline void destroy(char* first,char* last)
	{
		__destroy_range(first,last,true_type());
	}
	
	template <>
	inline void destroy(const char* first, const char* last) 
	{
		__destroy_range(first, last, true_type());
	}
}

#endif
