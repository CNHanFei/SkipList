#include <iostream>  // �����������
#include <cstdlib>   // ����std::rand()
#include <cmath>     // ������ѧ����
#include <cstring>   // ����memset
#include <mutex>     // ���ڻ�����
#include <fstream>   // �����ļ�����
#include <vector>    // ����std::vector
#include <string>    // ����std::string��getline, stoi��
#define STORE_FILE "store/dumpFile"

std::mutex mtx;     // �������������ٽ���
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

    // ʹ��std::vector�����涯̬����
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

    // ʹ��vector��ʼ����vector�Ĵ�СΪlevel + 1
    this->forward = std::vector<Node<K, V>*>(level + 1, nullptr);
};

template<typename K, typename V>
Node<K, V>::~Node() {
    // vector���Զ������ڴ棬����Ҫ�ֶ�ɾ��
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
    //�ݹ�ɾ���ڵ�
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
// ��ԭ���������滻Ϊ std::vector

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
    /*��¼ÿһ�������һ��С��Ҫ������Ľڵ㡣
    ��������Ŀ����Ϊ���ڲ����½ڵ�ʱ�ܹ����ٸ���ǰ���ڵ��ָ�룬
    �Ӷ����½ڵ���ȷ�ز��뵽������*/
    std::vector<Node<K, V>*> update;
    Node<K, V>* current = this->_header;

    // ʹ�� std::vector ��������
    update.resize(_max_level + 1, nullptr);  // ���� update �Ĵ�С

    // ����߲㿪ʼ��������²��Ҳ���λ��
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != nullptr && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    // �����0�㲢��ָ��ָ���ұߵĽڵ㣬����Ƿ��Ѿ����ڸ�key
    current = current->forward[0];

    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    // ���currentΪNULL����ʾ�Ѿ����������ĩβ���Ҹ�key������
    if (current == NULL || current->get_key() != key) {

        // ����һ����������Ľڵ�
        int random_level = get_random_level();

        // ������ɵ�����������ڵ�ǰskip list�Ĳ���������update
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level + 1; i < random_level + 1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        // �����½ڵ㲢����
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
// ����һ��ģ���� SkipList �ĳ�Ա���� dump_file�����ڽ������е����ݵ������ļ���
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    // ���ļ����ļ���Ϊ STORE_FILE������д������
    _file_writer.open(STORE_FILE);
    Node<K, V>* node = this->_header->forward[0];

    // ��������ֱ�������սڵ�Ϊֹ
    while (node != NULL) {
        // ����ǰ�ڵ�ļ�ֵ��д���ļ�����ʽΪ "key:value\n"
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        
        // ͬʱ����ǰ�ڵ�ļ�ֵ�����������̨����ʽΪ "key:value;\n"
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        
        // �ƶ�����һ���ڵ㣬����ǰ�ڵ��һ��ǰ��ָ��ָ��Ľڵ�
        node = node->forward[0];
    }

    // ˢ���ļ���������ȷ���������ݶ���д���ļ�
    _file_writer.flush();
    
    // �ر��ļ�
    _file_writer.close();
    
    // �������أ�������������
    return;
}

// Load data from disk
// ����ģ����SkipList�ĳ�Ա����load_file
// �ú������ڴ��ļ��м��ؼ�ֵ�����ݵ�������
template<typename K, typename V>
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    
    // �������ڴ洢�ļ���ÿһ�е��ַ���
    std::string line;
    // �������ڴ洢����ֵ���ַ���ָ��
    std::string* key = new std::string();
    std::string* value = new std::string();
    
    // ���ж�ȡ�ļ�����
    while (getline(_file_reader, line)) {
        // ���ַ�������ȡ����ֵ
        get_key_value_from_string(line, key, value);
        
        // �������ֵΪ�գ�����������
        if (key->empty() || value->empty()) {
            continue;
        }
        
        // ���ַ������͵ļ�ת��Ϊ��������
        insert_element(stoi(*key), *value);
        // �����ǰ����ļ�ֵ��
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    
    // �ͷŶ�̬������ڴ�
    delete key;
    delete value;
    
    // �ر��ļ�
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

    // ����߲㿪ʼ������²���ɾ���ڵ��λ��
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if (current != NULL && current->get_key() == key) {
                        
        // ����Ͳ㿪ʼ���ɾ��Ŀ��ڵ�
        for (int i = 0; i <= _skip_list_level; i++) {
            // ����ò����һ�ڵ㲻��Ŀ��ڵ㣬����ѭ��
            if (update[i]->forward[i] != current)
                break;

            update[i]->forward[i] = current->forward[i];
        }

        // ɾ������Ĳ㼶
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

// ����������header�ڵ㣬û��key��û��value��ֻ��forward���飬forward����ָ��ÿһ��ĵ�һ���ڵ�
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

    // ����߲㿪ʼ������²���Ŀ��ڵ�
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }
    std::cout << current->forward[0]->get_key() << std::endl;
    // �����0�㲢�����ƽ�������Ŀ��ڵ�
    current = current->forward[0];

    // ����ҵ�Ŀ��ڵ㣬����true
    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// ����skip list
template<typename K, typename V>
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

    // ����header�ڵ㲢��ʼ��key��valueΪ��
    K k = K();  // Ĭ�Ϲ���K���͵Ķ���
    V v = V();  // Ĭ�Ϲ���V���͵Ķ���
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

    // �ݹ�ɾ����������
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
