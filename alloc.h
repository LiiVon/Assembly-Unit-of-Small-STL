#ifndef LZ_STL_ALLOC_H
#define LZ_STL_ALLOC_H

#include <new>
#include <cstdlib> // for exit()
#include <cstddef> // for ptrdiff_t,size_t
#include <climits>
#include <cstring>
#include "type_traits.h"

/*
用户申请内存：
大于128字节 一级配置器 __malloc_alloc_template
	内存足够 直接用std::malloc/free/remalloc

	内存不足 函数指针__malloc_alloc_oom_handler 简称(handler) 接受OOM用户回调函数
			 函数 接受函数指针handler 为参数 ，并返回指向函数的指针(该指针指向的就是OOM用户回调函数)
             调用OOM_alloc,OOM_remalloc

小于128字节 二级配置器 __default_alloc_template
	内存足够 判断所需节点大小，大于128，直接调用一级配置器
             否则从自由链表申请所需空间，申请成功则取出该链表节点

	内存不足 自由链表向内存池批量申请一批内存
			 如果内存池空间不足，只返回了一块，则直接传给用户
             如果真的返回了一批，则从第二块开始，将这一批块，用头插法挂到自由链表合适的位置上。

             开始判断内存池含量与被自由链表申请的空间的大小
             1.内存池剩余空间大于等于所需申请空间，则直接分配
			 2.剩余空间满足一个块，也直接分配
			 3.不满足一个块，但是还有一丢丢，也不能浪费，将其作为新的节点，作为链表的新的头节点
               然后计算内存池需要向其他地方申请的内存数
               先向更高级更大的链表节点申请试试看，如果没有，就像系统申请，如果还是不行就向一级配置器申请

*/
/*
1.为什么空间分配和函数构造要分开？
A:分配后，是否构造可以根据用户需求自行选择，若系统分配空间后默认构造但用户又将构造区域的内容覆盖，就会浪费默认构造的开销

2.为什么要分两级配置器？
A:解决内存碎片问题，小内存和大内存的碎片风险不同
  大内存直接用malloc/free，小碎片对于大内存来言，影响较小
  小内存频繁的分配和释放空间会产生大量内存碎片，小碎片堆小内存的占比就大了，影响也就大了

3.什么是内存碎片问题？
A:总的空闲内存够，但是这些小内存碎片分散在不同位置，不连续，无法被有效分配。
  外部碎片：频繁分配 / 释放不同大小的内存块，导致原本连续的内存被 “拆分”
  内部碎片：内存分配时的 “对齐要求” 或 “固定块尺寸分配”。

4.二级配置器是怎么解决内存碎片问题的？
A:内存池+固定尺寸自由链表，根源上避免外部碎片的产生，并高效复用内存
  1）固定尺寸划分，将小内存按8字节整数倍划分为16个规格，覆盖所有<=128字节的需求
      避免“大小不一的小块内存混杂在堆中”，从源头减少零散空闲块的产生
  2）自由链表，为每个规格维护一条自由链表
     分配/释放空间时，取出/退回到原来的链表，而非直接还给系统堆，
     使得同尺寸的空闲块被集中管理，不会分散在堆中，避免外部碎片
  3) 内存池一次性调用大量内存，减少系统调用malloc/free的次数，也就减少了内存碎片


*/
namespace lzstl
{
	//第一级配置器  __malloc_alloc_template
	template <int inst>
	//inst 是一个标记值，用于区分一个模板类的不同实例
	class __malloc_alloc_template
	{
	private:
		//oom out of memory
		static void* oom_malloc(size_t n);
		static void* oom_realloc(void*p ,size_t n);
		//回调函数，用户自定义的处理oom的函数
		// 指针__malloc_alloc_oom_handler 指向一个无返回值和参数的 用户设置的 回调函数
		//存储 用户OOM函数 的 地址
		static void (* __malloc_alloc_oom_handler)(); 
	public:
		// 通用指针 void* 返回的肯定是指针，因为开辟了空间
		static void* allocate(size_t n)
		{
			void* result = malloc(n);
			if(!result)
				result = oom_malloc(n);
			return result;
		}
		
		static void deallocate(void* p,size_t /*n*/)
		{
			//size_t /*n*/ 表示接收一个 size_t 类型的参数，但在函数内部不使用这个参数
			//二级要指定指针p与大小
			//一级与二级要统一接口
			return free(p);
		}
		
		static void* realloc(void* p,size_t /*old_sz*/,size_t new_sz)
		{
			void* result = std::realloc(p,new_sz);
			if(!result)
				result = oom_realloc(p,new_sz);
			return result;
		}
		
		//设置一个接受 用户回调函数 的函数
		//参数是 该回调函数
		//返回类型 是 “指向无参数、无返回值的函数的指针”
		static void (*set_malloc_oom_handler(void (*f)()))()
		{
			//old也是指针，存储旧的 OOM函数 地址
			void (*old)() = __malloc_alloc_oom_handler;
			//将用户的f OOM函数 更新给 全局变量
			__malloc_alloc_oom_handler = f;
			//返回旧的回调函数（方便用户后续恢复）
			return old;
		}
	};
	template <int inst>
	void (*__malloc_alloc_template<inst>::__malloc_alloc_oom_handler)() = nullptr;
	
	template <int inst>
	void* __malloc_alloc_template<inst>::oom_malloc(size_t n)
	{
		//临时函数指针。指向 OOM函数
		void (*my_malloc_oom_handler)();
		//结果 指针  返回值是开辟的地址
		void* result;
		
		for(;;)
		{
			//临时函数指针 存储  oom函数
			//因为__malloc_alloc_oom_handler 也是 函数指针，  并提前指向了 OOM回调函数
			my_malloc_oom_handler = __malloc_alloc_oom_handler;
			//用户没有设置OOM，直接异常
			if(!my_malloc_oom_handler)
				throw std::bad_alloc();
			//设置了OOM，调用OOM处理空间
			(*my_malloc_oom_handler)();
			//OOM函数处理完内存不足后，并再次尝试开辟空间
			result = std::malloc(n);
			//开辟成功，就返回开辟的地址
			if(result)
				return result;
		}
	}
	
	template <int inst>
	void* __malloc_alloc_template<inst>::oom_realloc(void* p,size_t n)
	{
		void (*my_alloc_oom_handler)();
		void* result;
		
		for(;;)
		{
			my_alloc_oom_handler = __malloc_alloc_oom_handler;
			if(!my_alloc_oom_handler)
				throw std::bad_alloc();
			(*my_alloc_oom_handler)();
			result  = std::realloc(p,n);
			if(result)
				return result;
		}
	}
	
	//二级配置器 __default_alloc_template
	template <bool is_thread_safe,int inst>
	//线程安全，为什么？
	//全局的自由链表和内存池，在多线程并发访问时会产生 数据竞争
	class __default_alloc_template
	{
	private:
		enum {__ALIGN = 8}; //块大小 8字节对齐
		enum {__MAX_BYTES = 128}; //最大块 128字节
		enum {__NFREELISTS = __MAX_BYTES / __ALIGN}; // numbers freelists  16
		
		//向上对其到8的倍数
		static size_t ROUND_UP(size_t bytes)
		{
			return (bytes + __ALIGN -1)& ~(__ALIGN - 1);
		}
		//自由链表节点结构
		//union 的特性是所有成员共享同一块内存空间
		//内存块空闲时，它是自由链表的一个节点，使用free_list_link指针，链接下一个空闲块
		//内存块被用户使用时，它是用户数据的起始地址client_data[1]
		union obj
		{
			union obj* free_list_link;
			char client_data[1];
		};
		
		//自由链表数组
		//每个元素都是 指向obj类型的指针
		//存储每一条自由链表的头节点地址
		static obj* volatile free_list[__NFREELISTS];
		
		//根据字节数找到对应的自由链表的索引
		static size_t FREELIST_INDEX(size_t bytes)
		{
			return ((bytes + __ALIGN -1)/ __ALIGN -1);
		}
		
		//自由链表空间不足时，向内存池一次性申请大量空间
		//nobjs :希望一次性分配的块数量
		static void* chunk_alloc(size_t size,int& nobjs);
		
		//内存池地址
		static char* start_free;
		static char* end_free;
		//内存池总大小
		static size_t heap_size;
		
	public:
		static void* allocate(size_t n) //n个字节大小
		{
			if(n==0) return nullptr;
			void* ret;
			
			if(n > (size_t)__MAX_BYTES)
				ret = __malloc_alloc_template<inst>::allocate(n);
			//二级配置器 分配空间  将对应的自由链表的节点块取走
			else
			{
				//my_free_list 是指向 “该索引对应的自由链表头指针” 的指针（即 free_list[i] 的地址）
				obj* volatile* my_free_list = free_list + FREELIST_INDEX(n);
				//取出该自由链表的第一个空闲块
				obj* result = *my_free_list;
				if(!result)
					//如果链表为空（result 是 nullptr），说明没有现成的空闲块
					//即，自由链表内存不足，需要向内存池补充一批同大小的块
					//refill 申请方式 是 调用chunk_alloc 函数
					ret = refill(ROUND_UP(n));
				else
				{
					//如果链表不为空，从链表头部取下一个空闲块（result）
					//更新链表头指针：（让链表头指向原第二个块）
					//将取下的块（result）返回给用户
					*my_free_list = result->free_list_link;
					ret = result;
				}
			}
			return ret;
		}
		
		static void deallocate(void*p,size_t n)
		{
			if(n==0) return;
			if(n > (size_t)__MAX_BYTES)
				__malloc_alloc_template<inst>::deallocate(p,n);
			//二级配置器 释放空间 将对应的自由链表的节点块p归还插入到合适的位置
			else
			{
				obj* volatile* my_free_list = free_list + FREELIST_INDEX(n);
				obj*q = (obj*)p;
				//头插法归还
				q->free_list_link = *my_free_list;
				*my_free_list = (obj*)p;
			}
		}
		
		static void* reallocate(void*p,size_t old_sz,size_t new_sz)
		{
			deallocate(p,old_sz);
			return allocate(new_sz);
		}
		
		
	private:
		//自由链表空间不足，free_list[i] == 0
		//向内存池 批量申请一批内存块 
		//n 是已经对齐后的内存块大小
		static void* refill(size_t n)
		{
			//一次默认分配20个块 8字节大小
			int nobjs = 20; 
			// 从内存池申请 nobjs 个大小为 n的块
			//返回的 chunk 是这一批块的起始地址。
			char* chunk = (char*)chunk_alloc(n,nobjs);
			obj* volatile* my_free_list;
			obj* result;
			obj* current_obj,*next_obj;
			
			if(nobjs == 1) return chunk;//只分配到了一个块，直接返回给用户
			
			//找到对应的自由链表，确定将这批块要挂到哪个自由链表
			my_free_list = free_list + FREELIST_INDEX(n);
			//第一个块（chunk 到 chunk+n）给用户，剩下的块从第二块开始挂上链表
			result = (obj*)chunk;
			*my_free_list = next_obj = (obj*)(chunk + n);
			
			//此时next_obj已经指向了第二个块
			//将剩余块链接成自由链表
			for(int i =1;;++i)
			{
				current_obj = next_obj; //cur 是 第二个块
				next_obj = (obj*)((char*)next_obj+n); //第三个
				if(nobjs-1 == i)
				// 最后一个块
				{
					// 链表结尾设为nullptr
					current_obj->free_list_link = nullptr;
					break;
				}
				else
					current_obj->free_list_link = next_obj;
			}
			return result;
		}
	};
	// 初始化二级配置器静态成员
	template <bool is_thread_safe,int inst>
	char* __default_alloc_template<is_thread_safe,inst>::start_free = nullptr;
	
	template <bool is_thread_safe,int inst>
	char* __default_alloc_template<is_thread_safe,inst>::end_free = nullptr;
	
	template <bool is_thread_safe,int inst>
	size_t __default_alloc_template<is_thread_safe,inst>::heap_size = 0;
	
	template <bool is_thread_safe,int inst>
	typename __default_alloc_template<is_thread_safe,inst>::obj* volatile
	__default_alloc_template<is_thread_safe,inst>::free_list[__NFREELISTS] = {nullptr};
	
	
	//内存池 与 chunk_alloc
	template <bool is_thread_safe ,int inst>
	void* __default_alloc_template<is_thread_safe,inst>::chunk_alloc(size_t size,int& nobjs)
	{
		//被自由链表申请的总空间
		size_t total_bytes = size* nobjs;
		//内存池剩余空间
		size_t bytes_left = end_free - start_free;
		//同上，返回的是第一个块的地址
		char* result;
		
		//场景 1：内存池剩余空间足够分配（最理想情况）
		if(bytes_left >= total_bytes)
		{
			result = start_free;
			start_free += total_bytes;
			return result;
		}
		
		//场景 2：内存池空间不足，但至少能分 1 个块
		else if(bytes_left >= size)
		{
			//实际能分的块数
			nobjs = bytes_left / size;
			//实际能分配的空间
			total_bytes = size* nobjs;
			result = start_free;
			start_free += total_bytes;
			return result;
		}
		
		//场景 3：内存池空间连 1 个块都不够（最复杂的情况）
		
		//步骤 1：计算向系统申请的内存大小
		//申请 2 * total_bytes（原计划的 2 倍，多囤点减少下次申请）。
		//额外加 ROUND_UP(heap_size >> 4)（随内存池总大小 heap_size 增长，动态扩容）
		size_t bytes_to_get = 2* total_bytes + ROUND_UP(heap_size >>4);
		
		//步骤 2：利用内存池剩余的 “零头空间”
		if(bytes_left > 0)
		{
			//找到剩余空间所在数组的位置 假设是i
			obj* volatile* my_free_list = free_list + FREELIST_INDEX(bytes_left);
			//star_free 到 bytes_left 这段内存是 剩余空间的 地址
			//将这段内存 强制 转为 一个 链表的节点
			//头插法，将 该节点 插到 前面假设的位置i的前面
			((obj*)start_free)->free_list_link = *my_free_list;
			//更新链表头，也就是把剩余的小空间 也挂到了该自由链表上，不浪费它，而且是头节点
			*my_free_list = (obj*)start_free;
		}
		
		//步骤 3：向系统（堆）申请内存
		start_free = (char*)std::malloc(bytes_to_get);
		
		//步骤 4：系统内存也不足（malloc 失败）
		if(!start_free)
		{
			obj* volatile* my_free_list,*p;
			 // 遍历更大的自由链表，尝试“借”一块
			for(size_t i = size;i<=__MAX_BYTES;i += __ALIGN)
			{
				my_free_list = free_list + FREELIST_INDEX(i);
				p = *my_free_list;
				if(p)
				{
					//更新表头
					*my_free_list = p->free_list_link;
					start_free = (char*)p;
					end_free = start_free + i;
					return chunk_alloc(size,nobjs);
				}
			}
			//如果所有链表节点都没有，就找一级配置器
			end_free = nullptr;
			start_free = (char*)__malloc_alloc_template<inst>::allocate(bytes_to_get);
		}
		
		heap_size += bytes_to_get;
		end_free = start_free + bytes_to_get;
		return chunk_alloc(size,nobjs);
	}
	typedef __default_alloc_template<false,0> alloc;
}

#endif //LZ_STL_ALLOC_H
