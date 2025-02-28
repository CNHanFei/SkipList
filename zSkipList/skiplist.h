#include <iostream>  // 用于输入输出
#include <cstdlib>   // 用于std::rand()
#include <cmath>     // 用于数学函数
#include <cstring>   // 用于memset
#include <mutex>     // 用于互斥锁
#include <fstream>   // 用于文件操作
#include <vector>    // 用于std::vector
#include <string>    // 用于std::string，getline, stoi等
#define STORE_FILE "D:/Code/learn-c++/code/zSkipList/zSkipList/dumpFile"
#include <memory>


std::mutex mtx;     // 互斥锁，用于临界区
std::string delimiter = ":";


#ifndef SKIPLIST_H
#define SKIPLIST_H
template <typename K, typename V>
class Node {
public:

	Node() {}
	Node(K, V, int);
	~Node();
	K get_key()const;
	V get_value()const;
	void set_value(V);

	int node_level;	//节点的层数
	std::vector<Node<K, V>*>forward;//每个点的前向指针，最多有MAX_LEVEL个，
	/*
	forward[0]存的是：在第0层这个点的下一个节点
	forward[1]存的是：在第1层这个点的下一个节点
	*/
private:
	K key;
	V value;
};
template <class K, class V>
Node<K, V>::Node(K k, V v, int level) {
	key = k;
	value = v;
	node_level = level;
	//初始化这个点的forward数组,默认都是nullptr
	forward = std::vector<Node<K, V>*>(level + 1, nullptr);
}
template <class K, class V>
Node<K, V>::~Node() {}

template <class K, class V>
K Node<K, V>::get_key()const { return key; }
template <class K, class V>
V Node<K, V>::get_value()const { return value; }
template<class K, class V>
void Node<K, V>::set_value(V v) { value = v; }


template<class K, class V>
class SkipList {
public:
	SkipList(int);
	~SkipList();
	int get_random_level();
	Node<K, V>* create_node(K, V, int);
	int insert_element(K, V);
	void display_list();
	bool search_element(K);
	void delete_element(K);
	int size();
	void clear(Node<K, V>*);
	void dump_file();
	void load_file();
 private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);

private:
	int _max_level;				//跳表最大层数
	int _skip_list_level;		//跳表当前层数
	Node<K, V>* _header;		//跳表头节点
	int _element_count;			 //跳表元素个数

	//文件io操作
	std::ofstream _file_writer;
	std::ifstream _file_reader;
};

template<class K, class V>
SkipList<K, V>::SkipList(int max_level) {
	this->_max_level = max_level;
	this->_skip_list_level = 0;	//跳表初始化是0层
	this->_header = create_node(K(), V(), _max_level);	//创建头节点
	this->_element_count = 0;	//跳表初始化是0个元素
}

template<class K, class V>
SkipList<K, V>::~SkipList() {}

template<class K, class V>
Node<K, V>* SkipList<K, V>::create_node(K k, V v, int level) {
	Node<K, V>* node = new Node<K, V>(k, v, level);
	return node;
}

template<class K, class V>
void SkipList<K, V>::display_list() {
	std::cout << "\n*****Skip List*****" << "\n";
	for (int i = _skip_list_level; i >= 0; i--) {
		Node<K, V>* node = _header->forward[i];
		std::cout << "Level " << i << ": ";
		while (node != nullptr) {
			std::cout << node->get_key() << " " << node->get_value() << "; ";
			node = node->forward[i];
		}
		std::cout << std::endl;
	}
}

template<class K, class V>
bool SkipList<K, V>::search_element(K key) {
	//std::cout << "search_element-----------------" << std::endl;
	Node<K, V>* current = _header;
	for (int i = _skip_list_level; i >= 0; i--) {
		while (current->forward[i] && current->forward[i]->get_key() < key) {
			current = current->forward[i];
		}
	}
	//std::cout << current->forward[0]->get_key() << std::endl;
	// 到达第0层并向右推进，查找目标节点
	current = current->forward[0];

	// 如果找到目标节点，返回true
	if (current and current->get_key() == key) {
		//std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
		return true;
	}
	//std::cout << "Not Found Key:" << key << std::endl;
	return false;
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level() {

	int k = 1;
	while (rand() % 2) {
		k++;
	}
	k = (k < _max_level) ? k : _max_level;
	return k;
}



/*
						   +------------+
						   |  insert 50 |
						   +------------+
level 4     +-->1+                                                      100
				 |
				 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
											   |    |
											   |    |
level 2         1          10         30       | 50 |          70       100
											   |    |
											   |    |
level 1         1    4     10         30       | 50 |          70       100
											   |    |
											   |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
											   +----+
*/
template<class K, class V>
int SkipList<K, V>::insert_element(K key, V value) {
	std::unique_lock<std::mutex> locker(mtx);
	/*记录每一层中最后一个小于要插入键的节点。
  这样做的目的是为了在插入新节点时能够快速更新前驱节点的指针，
  从而将新节点正确地插入到跳表中*/
	std::vector<Node<K, V>*> update(_max_level + 1, nullptr);
	Node<K, V>* current = _header;
	for (int i = _skip_list_level; i >= 0; i--) {
		while (current->forward[i] && current->forward[i]->get_key() < key) {
			current = current->forward[i];
		}
		update[i] = current;
	}

	//结束之后，update=[40,30,30,10,1,nullptr,nullptr...]
#if 0
	for (auto& x : update) {
		if (x != nullptr)
			std::cout << x->get_key() << ", ";
	}
	std::cout << std::endl;
#endif
	//这个key已经在跳表中了，不用插入，直接返回
	if (current->forward[0] && current->forward[0]->get_key() == key) {
		//std::cout << "key: " << key << ", exists" << std::endl;
		return 1;
	}

	//current->forward[0]==null,说明到了链表结尾了
	if (current->forward[0] == nullptr || current->forward[0]->get_key() != key) {
		//准备插入节点了
		//随机生成这个节点的层数，表示他有几层
		int random_level = get_random_level();

		// 如果生成的随机层数大于当前skip list的层数，更新update
		if (random_level > _skip_list_level) {
			for (int i = _skip_list_level + 1; i < random_level + 1; i++) {
				update[i] = _header;
			}
			_skip_list_level = random_level;
		}
		//假设random_level=6，则更新后的update=[40,30,30,10,1,_header,_header]

		//插入新节点
		//insert_node标注了层数，
		Node<K, V>* insert_node = new Node<K, V>(key, value, random_level);
		for (int i = 0; i <= random_level; i++) {
			insert_node->forward[i] = update[i]->forward[i];
			update[i]->forward[i] = insert_node;
		}
		//std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
		_element_count++;

	}
	return 0;


}

/*
						   +------------+
						   |  Delete 50 |
----------				   +------------+
|level 4 |    +-->1+                                               100
|		 |		 |
|		 |		 |
|level 3 |       1+-------->10+------------------>50+     70       100
|		 |										   |
|		 |										   |
|level 2 |       1          10         30         50|     70       100
|		 |										   |
|		 |										   |
|level 1 |       1    4     10         30         50|     70       100
|		 |										   |
|	     |
|level 0 |       1    4   9 10         30   40    50      70       100
----------

*/
//若是50有第五层的话
//update=[40,30,30,10,1,_header,nullptr...]
template<class K, class V>
void SkipList<K, V>::delete_element(K key) {
	std::unique_lock<std::mutex> locker(mtx);
	std::vector<Node<K, V>*> update(_max_level + 1, nullptr);
	Node<K, V>* current = _header;
	//仍然是从当前层开始往下找
	for (int i = _skip_list_level; i >= 0; i--) {
		while (current->forward[i] and current->forward[i]->get_key() < key) {
			current = current->forward[i];
		}
		update[i] = current;

	}

#if 0
	for (auto& x : update) {
		if (x)
			std::cout << x->get_key() << ", ";
	}
	std::cout << std::endl;
#endif
	current = current->forward[0];		 //current就是要删除的
	if (current != NULL && current->get_key() == key) {
		for (int i = 0; i <= _skip_list_level; i++) {
			// 如果该层的下一节点不是目标节点，跳出循环
			if (update[i]->forward[i] != current)
				break;
			update[i]->forward[i] = current->forward[i];
		}
		// 删除多余的层级
		while (_skip_list_level > 0 && _header->forward[_skip_list_level] == nullptr) {
			_skip_list_level--;
		}

		std::cout << "Successfully deleted key " << key << std::endl;
		delete current;
		_element_count--;
	}
	return;


}

template<class K, class V>
int SkipList<K, V>::size() { return _element_count; }

template <typename K, typename V>
void SkipList<K, V>::clear(Node<K, V>* cur)
{
	if (cur->forward[0] != nullptr) {
		clear(cur->forward[0]);
	}
	delete(cur);
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

	if (!is_valid_string(str)) {
		return;
	}
	*key = str.substr(0, str.find(delimiter));
	*value = str.substr(str.find(delimiter) + 1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {

	if (str.empty()) {
		return false;
	}
	if (str.find(delimiter) == std::string::npos) {
		return false;
	}
	return true;
}

template<typename K, typename V>
// 定义一个模板类 SkipList 的成员函数 dump_file，用于将跳表中的数据导出到文件中
void SkipList<K, V>::dump_file() {

	std::cout << "dump_file-----------------" << std::endl;
	// 打开文件，文件名为 STORE_FILE，用于写入数据
	_file_writer.open(STORE_FILE);
	Node<K, V>* node = this->_header->forward[0];

	// 遍历跳表，直到遇到空节点为止
	while (node != NULL) {
		// 将当前节点的键值对写入文件，格式为 "key:value\n"
		_file_writer << node->get_key() << ":" << node->get_value() << "\n";

		// 同时将当前节点的键值对输出到控制台，格式为 "key:value;\n"
		std::cout << node->get_key() << ":" << node->get_value() << ";\n";

		// 移动到下一个节点，即当前节点第一层前向指针指向的节点
		node = node->forward[0];
	}

	// 刷新文件缓冲区，确保所有数据都被写入文件
	_file_writer.flush();

	// 关闭文件
	_file_writer.close();

	// 函数返回，结束导出操作
	return;
}

// Load data from disk
// 定义模板类SkipList的成员函数load_file
// 该函数用于从文件中加载键值对数据到跳表中
template<typename K, typename V>
void SkipList<K, V>::load_file() {

	_file_reader.open(STORE_FILE);
	std::cout << "load_file-----------------" << std::endl;

	// 定义用于存储文件中每一行的字符串
	std::string line;
	// 定义用于存储键和值的字符串指针
	std::string* key = new std::string();
	std::string* value = new std::string();

	// 逐行读取文件内容
	while (getline(_file_reader, line)) {
		// 从字符串中提取键和值
		get_key_value_from_string(line, key, value);

		// 如果键或值为空，则跳过该行
		if (key->empty() || value->empty()) {
			continue;
		}

		// 将字符串类型的键转换为整数类型
		insert_element(stoi(*key), *value);
		// 输出当前插入的键值对
		std::cout << "key:" << *key << "value:" << *value << std::endl;
	}

	// 释放动态分配的内存
	delete key;
	delete value;

	// 关闭文件
	_file_reader.close();
}



#endif