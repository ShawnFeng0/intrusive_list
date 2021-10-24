//
// Created by fs on 2020-09-21.
//

#include "list/list.h"

#include <gtest/gtest.h>

#include "list/list.h"

struct list_test_struct {
  int value;

  list::list_node list1;
  list::list_node list2;
};

TEST(list, test) {
  //  // create list
  //  utos::list::list<list_test_struct, &list_test_struct::list1> list1;
  //
  //  // insert some elements
  //  list_test_struct s1, s2, s3, s4;
  //
  //  list1.insert_head(&s1);
  //  list1.insert_head(&s2);
  //  list1.insert_head(&s3);
  //  list1.insert_head(&s4);
}
