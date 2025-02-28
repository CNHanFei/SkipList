#include <iostream>  // �����������
#include <cstdlib>   // ����std::rand()
#include <cmath>     // ������ѧ����
#include <cstring>   // ����memset
#include <mutex>     // ���ڻ�����
#include <fstream>   // �����ļ�����
#include <vector>    // ����std::vector
#include <string>    // ����std::string��getline, stoi��
#define STORE_FILE "D:/Code/learn-c++/code/zSkipList/zSkipList/dumpFile"
#include <memory>


std::mutex mtx;     // �������������ٽ���
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

	int node_level;	//�ڵ�Ĳ���
	std::vector<Node<K, V>*>forward;//ÿ�����ǰ��ָ�룬�����MAX_LEVEL����
	/*
	forward[0]����ǣ��ڵ�0����������һ���ڵ�
	forward[1]����ǣ��ڵ�1����������һ���ڵ�
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
	//��ʼ��������forward����,Ĭ�϶���nullptr
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
	int _max_level;				//����������
	int _skip_list_level;		//����ǰ����
	Node<K, V>* _header;		//����ͷ�ڵ�
	int _element_count;			 //����Ԫ�ظ���

	//�ļ�io����
	std::ofstream _file_writer;
	std::ifstream _file_reader;
};

template<class K, class V>
SkipList<K, V>::SkipList(int max_level) {
	this->_max_level = max_level;
	this->_skip_list_level = 0;	//�����ʼ����0��
	this->_header = create_node(K(), V(), _max_level);	//����ͷ�ڵ�
	this->_element_count = 0;	//�����ʼ����0��Ԫ��
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
	// �����0�㲢�����ƽ�������Ŀ��ڵ�
	current = current->forward[0];

	// ����ҵ�Ŀ��ڵ㣬����true
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
	/*��¼ÿһ�������һ��С��Ҫ������Ľڵ㡣
  ��������Ŀ����Ϊ���ڲ����½ڵ�ʱ�ܹ����ٸ���ǰ���ڵ��ָ�룬
  �Ӷ����½ڵ���ȷ�ز��뵽������*/
	std::vector<Node<K, V>*> update(_max_level + 1, nullptr);
	Node<K, V>* current = _header;
	for (int i = _skip_list_level; i >= 0; i--) {
		while (current->forward[i] && current->forward[i]->get_key() < key) {
			current = current->forward[i];
		}
		update[i] = current;
	}

	//����֮��update=[40,30,30,10,1,nullptr,nullptr...]
#if 0
	for (auto& x : update) {
		if (x != nullptr)
			std::cout << x->get_key() << ", ";
	}
	std::cout << std::endl;
#endif
	//���key�Ѿ����������ˣ����ò��룬ֱ�ӷ���
	if (current->forward[0] && current->forward[0]->get_key() == key) {
		//std::cout << "key: " << key << ", exists" << std::endl;
		return 1;
	}

	//current->forward[0]==null,˵�����������β��
	if (current->forward[0] == nullptr || current->forward[0]->get_key() != key) {
		//׼������ڵ���
		//�����������ڵ�Ĳ�������ʾ���м���
		int random_level = get_random_level();

		// ������ɵ�����������ڵ�ǰskip list�Ĳ���������update
		if (random_level > _skip_list_level) {
			for (int i = _skip_list_level + 1; i < random_level + 1; i++) {
				update[i] = _header;
			}
			_skip_list_level = random_level;
		}
		//����random_level=6������º��update=[40,30,30,10,1,_header,_header]

		//�����½ڵ�
		//insert_node��ע�˲�����
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
//����50�е����Ļ�
//update=[40,30,30,10,1,_header,nullptr...]
template<class K, class V>
void SkipList<K, V>::delete_element(K key) {
	std::unique_lock<std::mutex> locker(mtx);
	std::vector<Node<K, V>*> update(_max_level + 1, nullptr);
	Node<K, V>* current = _header;
	//��Ȼ�Ǵӵ�ǰ�㿪ʼ������
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
	current = current->forward[0];		 //current����Ҫɾ����
	if (current != NULL && current->get_key() == key) {
		for (int i = 0; i <= _skip_list_level; i++) {
			// ����ò����һ�ڵ㲻��Ŀ��ڵ㣬����ѭ��
			if (update[i]->forward[i] != current)
				break;
			update[i]->forward[i] = current->forward[i];
		}
		// ɾ������Ĳ㼶
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



#endif