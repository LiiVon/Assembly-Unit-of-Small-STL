#ifndef LZ_STL_VECTOR_H
#define LZ_STL_VECTOR_H

#include "type_traits.h"
#include "alloc.h"
#include "iterator.h"
#include "construct.h"
#include "uninitialized.h"
#include <cstddef>

namespace lzstl
{
	template <typename T,typename Alloc = alloc>
	class vector
	{
	public:
		typedef T 			value_type;
		typedef T* 			iterator;
		typedef const T*	const_iterator;
		typedef T&			reference;
		typedef const T&	const_reference;
		typedef size_t		size_type;
		typedef ptrdiff_t 	difference_type;
		typedef Alloc 		allocator_type;		// 分配器类型
	private:
		//容器均为 前闭后开
		iterator _start;			 // 数据区起始地址
		iterator _finish;            // 数据区末尾地址的下一个地址
		iterator _end_of_storage;
		allocator_type _alloc;		// 分配器对象（负责内存分配/释放）
		
		// -------------------------- 内部辅助函数 --------------------------
		// 分配内存并构造n个元素（值为value）
		iterator _allocate_and_construct(size_type n,const value_type& value)
		{
			iterator res = _alloc.allocate(n*sizeof(value_type));
			try
			{
				uninitialized_fill(res,res+n,value);
				return res;
			}
			catch(...)
			{
				_alloc.deallocate(res,n*sizeof(value_type));
				throw;
			}
		}
		
		// 销毁[first, last)元素并释放内存
		void _destroy_and_deallocate(iterator first,iterator last)
		{
			if(first!=last)
			{
				destroy(first,last);
				_alloc.deallocate(first,(last-first)*sizeof(value_type));
			}
		}
		
		// 扩容逻辑：至少扩容到new_capacity
		void _reallocate(size_type new_capacity)
		{
			if(new_capacity <= capacity()) return;
			
			// 1. 分配新内存
			iterator new_start = static_cast<iterator>(_alloc.allocate(new_capacity*sizeof(value_type)));
			iterator new_finish = new_start;
			
			try
			{
				// 2. 复制旧元素到新内存
				new_finish = uninitialized_copy(_start,_finish,new_start);
			}
			catch(...)
			{
				destroy(new_start,new_finish);
				_alloc.deallocate(new_start,new_capacity*sizeof(value_type));
				throw;
			}
			
			// 3. 销毁旧元素并释放旧内存
			_destroy_and_deallocate(_start,_finish);
			// 4. 更新指针
			_start = new_start;
			_finish = new_finish;
			_end_of_storage = new_start + new_capacity;
		}
		
		// 确保容量至少为n，不足则扩容（默认2倍扩容，最小1）
		void _ensure_capacity(size_type n)
		{
			if(n>capacity())
			{
				size_type new_cap = (capacity() == 0) ? 1 : capacity() * 2;
				if (new_cap < n) 
					new_cap = n;  // 确保能容纳n个元素
				_reallocate(new_cap);
			}
		}
		
	public:
		// -------------------------- 构造函数/析构函数/赋值运算符 --------------------------
		// 默认构造：空vector
		vector():_start(nullptr),_finish(nullptr),_end_of_storage(nullptr){}
		
		// 构造n个值为value的元素
		explicit vector(size_type n,const value_type& value = value_type())
		{
			_ensure_capacity(n);
			_finish = uninitialized_fill(_start,_start+n,value);
		}
		
		// 拷贝构造
		vector(const vector& rhs)
		{
			_ensure_capacity(rhs.size());
			_finish = uninitialized_copy(rhs._start,rhs._finish,_start);
		}
		
		// 迭代器范围构造
		template <typename InputIterator>
		vector(InputIterator first,InputIterator last)
		{
			size_type n =0;
			InputIterator tmp = first;
			while(tmp!=last)
			{
				++n;
				++tmp;
			}
			_ensure_capacity(n);
			_finish = uninitialized_copy(first,last,_start);
		}
		
		// 析构函数
		~vector() 
		{
			_destroy_and_deallocate(_start, _finish);
		}
		
		//拷贝赋值
		vector& operator=(const vector& rhs)
		{
			// 1. 处理自赋值
			if(this == &rhs)
				return *this;
			
			// 2. 若当前容量足够，直接销毁旧元素（无需重新分配内存）
			if(rhs.size() <= capacity())
			{
				destroy(_start,_finish);
				_finish = _start;
				_finish = uninitialized_copy(rhs._start,rhs._finish,_start);
			}
			else
			{
				// 3. 容量不足：销毁旧内存，分配新内存并复制
				// 3.1 销毁当前元素并释放旧内存
				_destroy_and_dealloctae(_start,_finish);
				
				// 3.2 分配与 rhs 相同大小的内存
				_start = _alloc.allocate(rhs.size()*sizeof(value_type));
				_end_of_storage = _start + rhs.size();
				
				// 3.3 复制 rhs 的元素到新内存
				_finish = uninitialized_copy(rhs._start, rhs._finish, _start);
			}
			return *this;
		}
		
		// -------------------------- 迭代器接口（STL标准）--------------------------
		iterator begin() {return _start;}
		const_iterator begin()const {return _start;}
		iterator end() {return _finish;}
		const_iterator end()const {return _finish;}
		
		// -------------------------- 容量与大小操作 --------------------------
		size_type size() const {return _finish - _start;}
		size_type capacity() const {return _end_of_storage - _start;}
		bool empty() const {return begin() == end();}
		
		// 预留容量（仅分配内存，不构造元素）
		void reserve(size_type n )
		{
			if(n>capacity())
				_reallocate(n);
		}
		
		// 调整大小（构造/析构元素）
		void resize(size_type n,const value_type& value = value_type())
		{
			if(n < size())
			{
				// 缩小：析构多余元素
				destroy(_start+n,_finish);
				_finish = _start + n;
			}
			else if(n > size())
			{
				// 扩大：先确保容量，再构造新元素
				_ensure_capacity(n);
				_finish = uninitialized_fill(_finish,_start+n,value);
			}
		}
		
		// 清空vector（析构所有元素，容量不变）
		void clear() 
		{
			destroy(_start, _finish);
			_finish = _start;
		}
		
		// -------------------------- 元素访问 --------------------------
		reference operator[](size_type idx) {return _start[idx];}
		const_reference operator[] (size_type idx) const {return _start[idx];}
		
		//   [begin,end)  end（）-1
		reference front() {return *begin();}
		const_reference front() const {return *begin();}
		
		reference back() {return *(end()-1);}
		const_reference back() const {return *(end()-1);}
		
		//兼容C
		value_type* data() {return _start;}
		const value_type* data() const {return _start;}
		
		// -------------------------- 元素插入/删除 --------------------------
		void push_back(const value_type& value)
		{
			if(_finish == _end_of_storage)
				_ensure_capacity(size() + 1);
			construct(_finish,value);
			++_finish;
		}
		
		void pop_back()
		{
			if(!empty())
			{
				--_finish;
				destroy(_finish);
			}
		}
		
		//pos 插入单个
		iterator insert(iterator pos,const value_type& value)
		{
			size_type idx = pos-_start; // 记录索引 ---- 偏移量
			if(_finish == _end_of_storage)
			{
				_ensure_capacity(size()+1);
				pos = _start + idx;     // 扩容后重新计算pos（旧pos已失效）
			}
			// start,start+1, ... ,pos,pos+1........finish-2,finish-1,finish
			// 移动[pos, _finish)区间的元素向后一位（空出pos位置）
			if(_finish!=_start)
				construct(_finish,*(_finish-1));
			
			//从后往前移动元素,完事插入更新
			iterator cur = _finish-1;
			while(cur>pos)
			{
				*cur = *(cur-1);
				--cur;
			}
			*pos = value;
			++_finish;
			return pos;
		}
		
		//pos 插入n个
		iterator insert(iterator pos,size_type n,const value_type& value)
		{
			if(n==0) return pos;
			
			size_type idx = pos-_start;
			_ensure_capacity(size()+n);
			pos = _start + idx;      //扩容后重新定位pos
			
			iterator new_finish = _finish + n;
			
			//从后往前
			iterator cur = _finish-1;
			iterator new_cur = new_finish-1;
			while(cur >= pos)
			{
				*new_cur = *cur;
				--cur;
				--new_cur;
			}
			//在空出的[pos, pos + n)位置构造n个value元素
			uninitialized_fill(pos,pos+n,value);
			//更新
			_finish = new_finish;
			return pos;
		}
		
		//迭代器范围插入  将 [first, last) 范围内的已有元素复制到 pos 位置
		template <typename InputIterator>
		iterator insert(iterator pos,InputIterator first,InputIterator last)
		{
			size_type n = 0;
			InputIterator tmp = first;
			while(tmp !=last)
			{
				++n;
				++tmp;
			}
			if(n==0) return pos;
			 
			size_type idx = pos-_start;
			_ensure_capacity(size()+n);
			pos = _start+idx;
			
			iterator new_finish = _finish + n;
			iterator cur = _finish-1;
			iterator new_cur = new_finish-1;
			while(cur >= pos)
			{
				*new_cur = *cur;
				--cur;
				--new_cur;
			}
			
			uninitialized_copy(first,last,pos);
			_finish = new_finish;
			return pos;
		}
		
		//pos 删除单个  pos到finish-1  前移
		iterator erase(iterator pos)
		{
			if(pos+1 != _finish)
			{
				iterator cur = pos;
				while(cur+1 !=_finish)
				{
					*cur = *(cur+1);
					++cur;
				}
			}
			--_finish;
			destroy(_finish);
			return pos;
		}
		
		//迭代器范围删除
		// st,st+1,st+1,...first,first+1....last-1,last,......fin-1,fin
		iterator erase(iterator first,iterator last)
		{
			if(first == last) return last;
			
			size_type n = last-first;
			iterator cur = first;
			iterator src = last;
			while(src != _finish)
			{
				*cur = *src;
				++cur;
				++src;
			}
			
			iterator new_finish = _finish-n;
			destroy(new_finish,_finish);
			
			_finish = new_finish;
			return first;
		}
		
		// -------------------------- 分配器相关 --------------------------
		allocator_type get_allocator() const {return _alloc;}
	};
	
}

#endif
