#pragma once

#include "common.h"

namespace intrusive_list {

struct list_node {
  struct list_node *next, *prev;

  /**
   * Note that the node must already be in a list
   */
  auto remove_self_from_list() {
    next->prev = prev;
    prev->next = next;
    next = nullptr;
    prev = nullptr;
    return *this;
  }

  auto have_container() const { return next && prev; }
};

namespace internal {

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void _list_add(struct list_node *new_, struct list_node *prev,
                             struct list_node *next) {
  next->prev = new_;
  new_->next = next;
  new_->prev = prev;
  prev->next = new_;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list front to add it after
 *
 * Insert a new entry after the specified front.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_node *new_, struct list_node *head) {
  _list_add(new_, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list front to add it before
 *
 * Insert a new entry before the specified front.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_node *new_,
                                 struct list_node *head) {
  _list_add(new_, head->prev, head);
}

/**
 * list_move_tail - delete from one list and add as another's back
 * @list: the entry to move
 * @head: the front that will follow our entry
 */
static inline void list_move_tail(struct list_node *list,
                                  struct list_node *head) {
  list->remove_self_from_list();
  list_add_tail(list, head);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_node *head) {
  return head->next == head;
}

/**
 * list_rotate_left - rotate the list to the left
 * @head: the front of the list
 */
static inline void list_rotate_left(struct list_node *head) {
  struct list_node *first;

  if (!list_empty(head)) {
    first = head->next;
    list_move_tail(first, head);
  }
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int list_is_singular(const struct list_node *head) {
  return !list_empty(head) && (head->next == head->prev);
}

}  // namespace internal

/**
 * list double linked list.
 */
template <typename T, list_node T::*node_field>
class list {
  list_node head_;

 public:
  list() noexcept : head_({&head_, &head_}) {}

  /**
   * insert item at the front of list.
   * @param item item to insert in list.
   */
  void push_front(T &item) { internal::list_add(get_node(&item), &head_); }

  /**
   * insert item at the back of list.
   * @param item item to insert in list.
   */
  void push_back(T &item) { internal::list_add_tail(get_node(&item), &head_); }

  /**
   * Note that the node must already be in a list
   * @param item item to remove
   */
  void remove_if_exists(T &item) {
    decltype(auto) node = get_node(&item);
    if (node->have_container()) {
      node->remove_self_from_list();
    }
  }

  void rotate_left() { internal::list_rotate_left(&head_); }
  bool is_singular() { return internal::list_is_singular(&head_); }

  /**
   * remove the first item in the list.
   */
  void pop_front() { get_node(&front())->remove_self_from_list(); }

  /**
   * remove the last item in the list.
   */
  void pop_back() { get_node(&back())->remove_self_from_list(); }

  /**
   * return first item in list.
   * @return first item in list
   *
   * Note list need not empty.
   */
  T &front() { return *get_owner(head_.next); }

  /**
   * return last item in list.
   * @return last item in list
   *
   * Note list need not empty.
   */
  T &back() { return *get_owner(head_.prev); }

  /**
   * check if the list is empty.
   * @return true if list is empty.
   */
  bool empty() const { return internal::list_empty(&head_); }

  struct Iterator {
    explicit Iterator(list_node *v) : node(v) {}
    explicit operator list_node *() const { return node; }
    inline bool operator!=(const Iterator &rhs) const {
      return node != rhs.node;
    }
    inline bool operator==(const Iterator &rhs) const {
      return node == rhs.node;
    }
    T &operator*() const { return *get_owner(node); }
    T *operator->() const { return get_owner(node); }
    Iterator &operator++() {
      node = node->next;
      return *this;
    }
    list_node *node;
  };

  Iterator begin() { return Iterator{head_.next}; }
  Iterator begin() const { return Iterator{head_.next}; }
  Iterator end() { return Iterator{&head_}; }
  Iterator end() const { return Iterator{&head_}; }

  Iterator erase(Iterator position) {
    Iterator ret = Iterator((position.node->next));
    position.node->remove_self_from_list();
    return ret;
  }

 private:
  static inline constexpr list_node *get_node(T *item) {
    return &(item->*node_field);
  }

  static inline constexpr T *get_owner(list_node *member) {
    return internal::owner_of(member, node_field);
  }
};

}  // namespace intrusive_list
