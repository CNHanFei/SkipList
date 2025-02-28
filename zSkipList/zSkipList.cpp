// zSkipList.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "test.h"
using namespace std;
int main()
{
	SkipList<int,string> sl(5);
	sl.insert_element(1, "one");
	sl.insert_element(2, "two");
	sl.insert_element(3, "three");
	sl.insert_element(4, "four");
	sl.insert_element(5, "five");
	sl.insert_element(6, "six");
	sl.insert_element(7, "seven");
	sl.insert_element(8, "eight");
	sl.insert_element(9, "nine");
	sl.display_list();
	sl.search_element(6);
}