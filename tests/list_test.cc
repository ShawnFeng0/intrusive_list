//
// Created by fs on 2020-09-21.
//

#include "intrusive_list/list.h"

#include <gtest/gtest.h>

#include <list>

struct list_test_struct {
  int value;

  intrusive_list::list_node node1;
  intrusive_list::list_node node2;
};

TEST(list, push_front) {
  std::list<list_test_struct> s(10);

  intrusive_list::list<list_test_struct, &list_test_struct::node1> list;
  for (auto& i : s) {
    list.push_front(&i);
  }

  ASSERT_EQ(list.front(), &s.back());
  ASSERT_EQ(list.back(), &s.front());
}

TEST(list, push_back) {
  std::list<list_test_struct> s(10);
  intrusive_list::list<list_test_struct, &list_test_struct::node1> list;
  for (auto& i : s) {
    list.push_back(&i);
  }

  ASSERT_EQ(list.front(), &s.front());
  ASSERT_EQ(list.back(), &s.back());
}

TEST(list, pop) {
  std::list<list_test_struct> s(10);
  intrusive_list::list<list_test_struct, &list_test_struct::node1> list;
  for (auto& i : s) {
    list.push_back(&i);
  }

  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(list.front(), &s.front());
    list.pop_front();
    s.pop_front();
  }

  for (int i = 0; i < 3; ++i) {
    ASSERT_EQ(list.back(), &s.back());
    list.pop_back();
    s.pop_back();
  }
}

TEST(list, empty) {
  std::list<list_test_struct> s(10);
  intrusive_list::list<list_test_struct, &list_test_struct::node1> list;
  ASSERT_TRUE(list.empty());

  for (auto& i : s) {
    list.push_back(&i);
    ASSERT_FALSE(list.empty());
  }

  for (int i = 0; i < 10; ++i) {
    ASSERT_FALSE(list.empty());
    list.pop_front();
  }

  ASSERT_TRUE(list.empty());
}

TEST(list, rotate_left) {
  std::list<list_test_struct> s(10);
  intrusive_list::list<list_test_struct, &list_test_struct::node1> list;
  ASSERT_TRUE(list.empty());
  for (auto& i : s) {
    list.push_back(&i);
  }

  auto front = list.front();
  list.rotate_left();
  ASSERT_NE(front, list.front());
  ASSERT_EQ(front, list.back());
}

TEST(intrusive_list, is_singular) {
  std::array<list_test_struct, 3> s{};
  intrusive_list::list<list_test_struct, &list_test_struct::node1> list;

  ASSERT_FALSE(list.is_singular());

  list.push_front(&s[0]);  // 1
  ASSERT_TRUE(list.is_singular());
  list.push_front(&s[1]);  // 2
  ASSERT_FALSE(list.is_singular());
  list.push_front(&s[2]);  // 3

  ASSERT_FALSE(list.is_singular());

  list.pop_back();  // 2
  ASSERT_FALSE(list.is_singular());
  list.pop_back();  // 1
  ASSERT_TRUE(list.is_singular());
  list.pop_back();  // 0

  ASSERT_FALSE(list.is_singular());
}

TEST(list, iterator) {
  std::list<list_test_struct> s(10);
  intrusive_list::list<list_test_struct, &list_test_struct::node1> list;

  for (auto& i : s) {
    list.push_back(&i);
  }

  auto i = s.begin();
  auto j = list.begin();
  for (; i != s.end() && j != list.end(); ++i, ++j) {
    ASSERT_EQ(&*i, *j);
  }
}
