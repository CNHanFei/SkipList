#include <iostream>  // 用于输入输出
#include <cstdlib>   // 用于std::rand()
#include <cmath>     // 用于数学函数
#include <cstring>   // 用于memset
#include <mutex>     // 用于互斥锁
#include <fstream>   // 用于文件操作
#include <vector>    // 用于std::vector
#include <string>    // 用于std::string，getline, stoi等
#define STORE_FILE "store/dumpFile"

std::mutex mtx;     // 互斥锁，用于临界区
std::string delimiter = ":";

// Class template to implement node
template<typename K, typename V>
class Node {

public:

    Node() {}

    Node(K k, V v, int);

    ~Node();

    K get_key() const;

    V get_value() const;

    void set_value(V);

    // 使用std::vector来代替动态数组
    std::vector<Node<K, V>*> forward;

    int node_level;

private:
    K key;
    V value;
};

template<typename K, typename V>
Node<K, V>::Node(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level;

    // 使用vector初始化，vector的大小为level + 1
    this->forward = std::vector<Node<K, V>*>(level + 1, nullptr);
};

template<typename K, typename V>
Node<K, V>::~Node() {
    // vector会自动管理内存，不需要手动删除
};

template<typename K, typename V>
K Node<K, V>::get_key() const {
    return key;
};

template<typename K, typename V>
V Node<K, V>::get_value() const {
    return value;
};

template<typename K, typename V>
void Node<K, V>::set_value(V value) {
    this->value = value;
};

// Class template for Skip list
template <typename K, typename V>
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
    void dump_file();
    void load_file();
    //递归删除节点
    void clear(Node<K, V>*);
    int size();

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_valid_string(const std::string& str);

private:
    // Maximum level of the skip list 
    int _max_level;

    // current level of skip list 
    int _skip_list_level;

    // pointer to header node 
    Node<K, V>* _header;

    // file operator
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // skiplist current element count
    int _element_count;
};

// create new node 
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V>* n = new Node<K, V>(k, v, level);
    return n;
}

// Insert given key and value in skip list 
// return 1 means element exists  
// return 0 means insert successfully
// 将原来的数组替换为 std::vector

 // Insert given key and value in skip list 
// return 1 means element exists  
// return 0 means insert successfully
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
template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {

    mtx.lock();
    /*记录每一层中最后一个小于要插入键的节点。
    这样做的目的是为了在插入新节点时能够快速更新前驱节点的指针，
    从而将新节点正确地插入到跳表中*/
    std::vector<Node<K, V>*> update;
    Node<K, V>* current = this->_header;

    // 使用 std::vector 代替数组
    update.resize(_max_level + 1, nullptr);  // 调整 update 的大小

    // 从最高层开始，逐层向下查找插入位置
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // 到达第0层并将指针指向右边的节点，检查是否已经存在该key
    current = current->forward[0];

    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    // 如果current为NULL，表示已经到达链表的末尾，且该key不存在
    if (current == NULL || current->get_key() != key) {

        // 生成一个随机层数的节点
        int random_level = get_random_level();

        // 如果生成的随机层数大于当前skip list的层数，更新update
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level + 1; i < random_level + 1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        // 创建新节点并插入
        Node<K, V>* inserted_node = create_node(key, value, random_level);

        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count++;
    }
    mtx.unlock();
    return 0;
}


// Display skip list 
template<typename K, typename V>
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****" << "\n";
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V>* node = this->_header->forward[i];
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// Dump data in memory to file 
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

// Get current SkipList size
template<typename K, typename V>
int SkipList<K, V>::size() {
    return _element_count;
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

// Delete element from skip list 
template<typename K, typename V>
void SkipList<K, V>::delete_element(K key) {

    mtx.lock();
    Node<K, V>* current = this->_header;
    Node<K, V>* update[_max_level + 1];
    memset(update, 0, sizeof(Node<K, V>*) * (_max_level + 1));

    // 从最高层开始逐层向下查找删除节点的位置
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if (current != NULL && current->get_key() == key) {
                        
        // 从最低层开始逐层删除目标节点
        for (int i = 0; i <= _skip_list_level; i++) {
            // 如果该层的下一节点不是目标节点，跳出循环
            if (update[i]->forward[i] != current)
                break;

            update[i]->forward[i] = current->forward[i];
        }

        // 删除多余的层级
        while (_skip_list_level > 0 && _header->forwfard[_skip_list_level] == nullptr) {
            _skip_list_level--;
        }

        std::cout << "Successfully deleted key " << key << std::endl;
        delete current;
        _element_count--;
    }
    mtx.unlock();
    return;
}

// Search for element in skip list 

// 这个最左侧是header节点，没有key，没有value，只有forward数组，forward数组指向每一层的第一个节点
/*
                           +------------+
                           |  select 60 |
----------				   +------------+
|level 4 |    +-->1+                                                     100
|		 |		 |
|		 |		 |
|level 3 |       1+-------->10+------------------>50+           70       100
|		 |										   |
|		 |										   |
|level 2 |       1          10         30         50|           70       100
|		 |										   |
|		 |										   |
|level 1 |       1    4     10         30         50|           70       100
|		 |										   |
|	     |
|level 0 |       1    4   9 10         30   40    50+-->60      70       100
----------

*/
template<typename K, typename V>
bool SkipList<K, V>::search_element(K key) {

    std::cout << "search_element-----------------" << std::endl;
    Node<K, V>* current = _header;

    // 从最高层开始逐层向下查找目标节点
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }
    std::cout << current->forward[0]->get_key() << std::endl;
    // 到达第0层并向右推进，查找目标节点
    current = current->forward[0];

    // 如果找到目标节点，返回true
    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// 构造skip list
template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // 创建header节点并初始化key和value为空
    K k = K();  // 默认构造K类型的对象
    V v = V();  // 默认构造V类型的对象
    this->_header = new Node<K, V>(k, v, _max_level);
};

template<typename K, typename V>
SkipList<K, V>::~SkipList() {

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }

    // 递归删除跳表链条
    if (_header->forward[0] != nullptr) {
        clear(_header->forward[0]);
    }
    delete(_header);
}

template <typename K, typename V>
void SkipList<K, V>::clear(Node<K, V>* cur)
{
    if (cur->forward[0] != nullptr) {
        clear(cur->forward[0]);
    }
    delete(cur);
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level() {

    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};
