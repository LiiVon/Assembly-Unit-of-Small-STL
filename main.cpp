#include <iostream>
#include <typeinfo>  // 用于typeid
#include <vector>
#include <list>
#include "alloc.h"  // 包含你的配置器头文件
#include "type_traits.h"
#include "iterator.h"
#include "construct.h"
#include "uninitialized.h"
#include "vector.h"

using namespace std;
using namespace lzstl;

// 测试一级配置器（大内存分配）
void test_level1_alloc() 
{
	cout << "=== 测试一级配置器（大内存） ===" << endl;
	
	// 分配150字节（>128，触发一级配置器）
	void* p1 = alloc::allocate(150);
	cout << "一级配置器分配150字节的地址: " << p1 << endl;
	
	// 释放
	alloc::deallocate(p1, 150);
	cout << "一级配置器释放150字节" << endl;
	
	// 重分配
	void* p2 = alloc::allocate(200);
	cout << "一级配置器分配200字节的地址: " << p2 << endl;
	void* p3 = alloc::reallocate(p2, 200, 300);  // 重分配到300字节
	cout << "一级配置器重分配到300字节的地址: " << p3 << endl;
	alloc::deallocate(p3, 300);
	cout << endl;
}

// 测试二级配置器（小内存分配）
void test_level2_alloc() 
{
	cout << "=== 测试二级配置器（小内存） ===" << endl;
	
	// 分配8字节（触发二级配置器，自由链表索引0）
	void* p4 = alloc::allocate(3);
	cout << "二级配置器分配3字节的地址: " << p4 << endl;
	
	// 分配16字节（索引1）
	void* p5 = alloc::allocate(13);
	cout << "二级配置器分配13字节的地址: " << p5 << endl;
	
	// 释放后再分配，验证自由链表复用
	alloc::deallocate(p4, 8);
	alloc::deallocate(p5, 16);
	void* p6 = alloc::allocate(8);   // 应该复用p4的地址
	void* p7 = alloc::allocate(16);  // 应该复用p5的地址
	cout << "释放后重新分配8字节（复用）: " << p6 << " (原p4: " << p4 << ")" << endl;
	cout << "释放后重新分配16字节（复用）: " << p7 << " (原p5: " << p5 << ")" << endl;
	
	// 测试内存池批量分配（申请20个8字节块）
	void* blocks[20];
	for (int i = 0; i < 20; ++i) 
		blocks[i] = alloc::allocate(8);
	
	cout << "批量分配20个8字节块（内存池）: 首地址" << blocks[0] << ", 尾地址" << blocks[19] << endl;
	
	// 释放批量分配的块
	for (int i = 0; i < 20; ++i) 
		alloc::deallocate(blocks[i], 8);

	cout << "批量释放20个8字节块" << endl;
	cout << endl;
}

// 测试OOM（内存耗尽）处理（需手动触发，谨慎测试）
void test_oom() 
{
	cout << "=== 测试OOM机制 ===" << endl;
	
	// 注册OOM回调函数（打印提示并尝试释放内存）
	auto oom_handler = []() 
	{
		cerr << "触发OOM！尝试释放内存..." << endl;
		// 实际场景中可在此释放缓存等非必要内存
	};
	__malloc_alloc_template<0>::set_malloc_oom_handler(oom_handler);
	
	try 
	{
		// 尝试分配超大内存（可能触发OOM）
		void* p = alloc::allocate(1000000ULL * 1024ULL * 1024ULL * 10ULL);  
		cout << "成功分配超大内存: " << p << endl;
		alloc::deallocate(p, 1000000ULL * 1024ULL * 1024ULL * 10ULL);
	} 
	catch (const bad_alloc& e) 
	{
		cout << "捕获异常: " << e.what() << endl;
	}
}


void test_type_traits() 
{
	cout << "\n=== 测试 type_traits.h ===" << endl;
	
	// 测试基本类型是否为POD
	cout << "int 是否为POD: " << (lzstl::is_pod_type<int>() ? "是" : "否") << endl;
	cout << "double 是否为POD: " << (lzstl::is_pod_type<double>() ? "是" : "否") << endl;
	cout << "char* 是否为POD: " << (lzstl::is_pod_type<char*>() ? "是" : "否") << endl;
	
	// 测试自定义非POD类型
	class NonPOD 
	{
	public:
		NonPOD() {}  // 自定义构造函数，破坏平凡性
		~NonPOD() {} // 自定义析构函数，破坏平凡性
	};
	cout << "NonPOD 是否为POD: " << (lzstl::is_pod_type<NonPOD>() ? "是" : "否") << endl;
	
	// 测试平凡析构
	cout << "int 是否有平凡析构: " << (lzstl::has_trivial_destructor<int>() ? "是" : "否") << endl;
	cout << "NonPOD 是否有平凡析构: " << (lzstl::has_trivial_destructor<NonPOD>() ? "是" : "否") << endl;
	
	// 测试整数/浮点类型判断
	cout << "long 是否为整数类型: " << (lzstl::is_integral_type<long>() ? "是" : "否") << endl;
	cout << "float 是否为浮点类型: " << (lzstl::is_floating_point_type<float>() ? "是" : "否") << endl;
	cout << "int* 是否为整数类型: " << (lzstl::is_integral_type<int*>() ? "是" : "否") << endl;
}

void test_iterator() 
{
	cout << "\n=== 测试 iterator.h ===" << endl;
	
	// 测试原生指针（随机访问迭代器）
	int arr[5] = {1, 2, 3, 4, 5};
	int* ptr = arr;
	lzstl::iterator_traits<int*>::iterator_category tag;
	cout << "int* 迭代器类别（随机访问）: " << typeid(tag).name() << endl;
	
	// 测试distance函数
	auto first = arr;
	auto last = arr + 5;
	cout << "数组元素个数（distance）: " << lzstl::distance(first, last) << endl;
	
	// 测试advance函数（随机访问迭代器）
	lzstl::advance(ptr, 3);  // 移动3步
	cout << "advance后指针指向: " << *ptr << endl;  // 应输出4
	
	// 测试list迭代器（双向迭代器）
	std::list<int> lst = {10, 20, 30, 40};
	auto lit = lst.begin();
	lzstl::advance(lit, 2);  // 双向迭代器只能逐步移动
	cout << "list迭代器advance后指向: " << *lit << endl;  // 应输出30
	
	// 测试迭代器特性萃取
	using list_iter = std::list<int>::iterator;
	cout << "list迭代器值类型: " << typeid(lzstl::iterator_traits<list_iter>::value_type).name() << endl;
	cout << "list迭代器距离类型: " << typeid(lzstl::iterator_traits<list_iter>::difference_type).name() << endl;
}

// 自定义测试类（带构造/析构日志）
class TestObj 
{
public:
	int val;
	TestObj() : val(0) { cout << "TestObj() 无参构造" << endl; }
	TestObj(int v) : val(v) { cout << "TestObj(" << v << ") 有参构造" << endl; }
	TestObj(const TestObj& other) : val(other.val) { cout << "TestObj 拷贝构造" << endl; }
	TestObj(TestObj&& other) : val(other.val) 
	{ 
		other.val = -1;  // 移动后源对象置标记
		cout << "TestObj 移动构造" << endl; 
	}
	~TestObj() { cout << "~TestObj(" << val << ") 析构" << endl; }
};

void test_construct() 
{
	cout << "\n=== 测试 construct.h ===" << endl;
	
	// 分配原始内存（用alloc）
	void* mem = lzstl::alloc::allocate(sizeof(TestObj));
	
	// 测试construct（无参构造）
	TestObj* obj1 = static_cast<TestObj*>(mem);
	lzstl::construct(obj1);  // 调用无参构造
	
	// 测试construct（拷贝构造）
	TestObj temp(100);
	lzstl::construct(obj1, temp);  // 覆盖内存，调用拷贝构造
	
	// 测试construct（移动构造）
	TestObj temp2(200);
	lzstl::construct(obj1, std::move(temp2));  // 调用移动构造
	cout << "移动后源对象值: " << temp2.val << endl;  // 应输出-1
	
	// 测试destroy（单个对象）
	lzstl::destroy(obj1);
	
	// 测试范围destroy（数组）
	size_t n = 3;
	TestObj* arr = static_cast<TestObj*>(lzstl::alloc::allocate(n * sizeof(TestObj)));
	for (size_t i = 0; i < n; ++i) {
		lzstl::construct(arr + i, (int)i);  // 构造3个对象
	}
	lzstl::destroy(arr, arr + n);  // 范围析构
	
	// 释放内存
	lzstl::alloc::deallocate(arr, n * sizeof(TestObj));
	lzstl::alloc::deallocate(mem, sizeof(TestObj));
}

void test_uninitialized() 
{
	cout << "\n=== 测试 uninitialized.h ===" << endl;
	
	// 测试uninitialized_copy（POD类型）
	int src1[] = {1, 2, 3, 4};
	size_t len1 = sizeof(src1) / sizeof(int);
	int* dest1 = static_cast<int*>(lzstl::alloc::allocate(len1 * sizeof(int)));
	auto end1 = lzstl::uninitialized_copy(src1, src1 + len1, dest1);
	cout << "uninitialized_copy POD结果: ";
	for (int* p = dest1; p != end1; ++p) cout << *p << " ";  // 应输出1 2 3 4
	cout << endl;
	lzstl::alloc::deallocate(dest1, len1 * sizeof(int));
	
	// 测试uninitialized_copy（非POD类型）
	TestObj src2[] = {TestObj(10), TestObj(20)};  // 构造源对象
	size_t len2 = sizeof(src2) / sizeof(TestObj);
	TestObj* dest2 = static_cast<TestObj*>(lzstl::alloc::allocate(len2 * sizeof(TestObj)));
	auto end2 = lzstl::uninitialized_copy(src2, src2 + len2, dest2);
	cout << "uninitialized_copy 非POD结果: ";
	for (TestObj* p = dest2; p != end2; ++p) cout << p->val << " ";  // 应输出10 20
	cout << endl;
	lzstl::destroy(dest2, end2);
	lzstl::alloc::deallocate(dest2, len2 * sizeof(TestObj));
	
	// 测试uninitialized_fill_n（填充5个值）
	size_t fill_n = 5;
	int* dest3 = static_cast<int*>(lzstl::alloc::allocate(fill_n * sizeof(int)));
	auto end3 = lzstl::uninitialized_fill_n(dest3, fill_n, 99);
	cout << "uninitialized_fill_n 结果: ";
	for (int* p = dest3; p != end3; ++p) cout << *p << " ";  // 应输出99 99 99 99 99
	cout << endl;
	lzstl::alloc::deallocate(dest3, fill_n * sizeof(int));
	
	// 测试uninitialized_move（移动语义）
	TestObj src3(30);
	TestObj* dest4 = static_cast<TestObj*>(lzstl::alloc::allocate(sizeof(TestObj)));
	auto end4 = lzstl::uninitialized_move(&src3, &src3 + 1, dest4);
	cout << "uninitialized_move 后源对象值: " << src3.val << endl;  // 应输出-1（移动后标记）
	cout << "uninitialized_move 目标对象值: " << dest4->val << endl;  // 应输出30
	lzstl::destroy(dest4, end4);
	lzstl::alloc::deallocate(dest4, sizeof(TestObj));
}

void test_vector() 
{
	cout << "\n=== 测试 vector.h ===" << endl;
	lzstl::vector<int> vec;
	
	// 尾插
	vec.push_back(1);
	vec.push_back(2);
	vec.push_back(3);
	cout << "尾插后：";
	for (auto it = vec.begin(); it != vec.end(); ++it) cout << *it << " "; // 1 2 3
	
	// 插入
	vec.insert(vec.begin() + 1, 10);
	cout << "\n插入后：";
	for (auto& val : vec) cout << val << " "; // 1 10 2 3
	
	// 扩容验证
	cout << "\nsize: " << vec.size() << ", capacity: " << vec.capacity(); // size=4, capacity=4
	
	// 删改
	vec.erase(vec.begin() + 2);
	vec[0] = 100;
	cout << "\n删改后：";
	for (auto it = vec.begin(); it != vec.end(); ++it) cout << *it << " "; // 3 10 100
	
	// 拷贝赋值
	lzstl::vector<int> vec2 = vec;
	cout << "\n拷贝后vec2大小：" << vec2.size(); // 3
}

int main() 
{
	test_level1_alloc();   // 测试一级配置器
	test_level2_alloc();   // 测试二级配置器
//	test_oom();          // 测试OOM（可选，谨慎执行）
	test_type_traits();
	test_iterator();
	test_construct();
	test_uninitialized();
	test_vector();
	return 0;
}
