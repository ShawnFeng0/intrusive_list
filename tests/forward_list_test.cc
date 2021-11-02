//
// Created by fs on 2020-09-21.
//

#include "intrusive_list/forward_list.h"

#include <gtest/gtest.h>

#include <list>

struct list_test_struct {
  int value;

  intrusive_list::forward_list_node node1;
  intrusive_list::forward_list_node node2;

  bool operator==(const list_test_struct& rhs) const {
    return rhs.value == value;
  }
};

TEST(forward_list, push_pop) {
  std::list<list_test_struct> s(10);

  intrusive_list::forward_list<list_test_struct, &list_test_struct::node1> list;
  for (auto& i : s) {
    list.push_front(&i);
  }
  // The above is reverse insertion, in order to keep the two lists consistent,
  // it should be reversed
  s.reverse();

  for (auto& i : s) {
    auto node = list.front();
    list.pop_front();
    ASSERT_EQ(node, &i);
  }
}

TEST(forward_list, empty) {
  std::list<list_test_struct> s(10);
  intrusive_list::forward_list<list_test_struct, &list_test_struct::node1> list;
  ASSERT_TRUE(list.empty());

  for (auto& i : s) {
    list.push_front(&i);
    ASSERT_FALSE(list.empty());
  }

  for (int i = 0; i < 10; ++i) {
    ASSERT_FALSE(list.empty());
    list.pop_front();
  }

  ASSERT_TRUE(list.empty());
}

TEST(forward_list, is_singular) {
  std::array<list_test_struct, 3> s{};
  intrusive_list::forward_list<list_test_struct, &list_test_struct::node1> list;

  ASSERT_FALSE(list.is_singular());

  list.push_front(&s[0]);  // 1
  ASSERT_TRUE(list.is_singular());
  list.push_front(&s[1]);  // 2
  ASSERT_FALSE(list.is_singular());
  list.push_front(&s[2]);  // 3

  ASSERT_FALSE(list.is_singular());

  list.pop_front();  // 2
  ASSERT_FALSE(list.is_singular());
  list.pop_front();  // 1
  ASSERT_TRUE(list.is_singular());
  list.pop_front();  // 0

  ASSERT_FALSE(list.is_singular());
}

TEST(forward_list, iterator) {
  std::list<list_test_struct> s(10);
  intrusive_list::forward_list<list_test_struct, &list_test_struct::node1> list;

  for (auto& i : s) {
    list.push_front(&i);
  }

  auto i = s.rbegin();
  auto j = list.begin();
  for (; i != s.rend() && j != list.end(); ++i, ++j) {
    ASSERT_EQ(&*i, *j);
  }

  ASSERT_EQ(i, s.rend());
  ASSERT_EQ(j, list.end());
}

TEST(forward_list, remove) {
  std::list<list_test_struct> s(10);
  intrusive_list::forward_list<list_test_struct, &list_test_struct::node1> list;

  int num = 0;
  for (auto& i : s) {
    i.value = num++;
    list.push_front(&i);
  }

  ASSERT_EQ(1, list.remove({.value = 5}));
  ASSERT_EQ(0, list.remove({.value = 5}));

  ASSERT_EQ(2, list.remove_if([](const list_test_struct& i) {
    return i.value > 4 && i.value < 8;
  }));
  ASSERT_EQ(0, list.remove_if([](const list_test_struct& i) {
    return i.value > 4 && i.value < 8;
  }));
}
