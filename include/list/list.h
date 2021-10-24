#pragma once

#include <cstddef>
#include <cstdint>

namespace list {

template <class Type, class Member>
static inline constexpr ptrdiff_t offset_of(const Member Type::*member) {
  return reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<Type *>(0)->*member));
}

template <class Type, class Member>
static inline constexpr Type *owner_of(Member *ptr,
                                       const Member Type::*member) {
  return reinterpret_cast<Type *>(reinterpret_cast<intptr_t>(ptr) -
                                  offset_of(member));
}

struct list_node {
  struct list_node *next, *prev;
};

/*
 * Circular doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

#define LIST_HEAD_INIT(name) \
  { &(name), &(name) }

#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

/**
 * INIT_LIST_HEAD - Initialize a list_node structure
 * @list: list_node structure to be initialized.
 *
 * Initializes the list_node to point to itself.  If it is a list header,
 * the result is an empty list.
 */
static inline void INIT_LIST_HEAD(struct list_node *list) {
  list->next = list;
  list->prev = list;
}

#ifdef CONFIG_DEBUG_LIST
extern bool __list_add_valid(struct list_node *new_, struct list_node *prev,
                             struct list_node *next);
extern bool __list_del_entry_valid(struct list_node *entry);
#else
static inline bool __list_add_valid(struct list_node *new_,
                                    struct list_node *prev,
                                    struct list_node *next) {
  return true;
}
static inline bool __list_del_entry_valid(struct list_node *entry) {
  return true;
}
#endif

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_node *new_, struct list_node *prev,
                              struct list_node *next) {
  if (!__list_add_valid(new_, prev, next)) return;

  next->prev = new_;
  new_->next = next;
  new_->prev = prev;
  prev->next = new_;
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_node *new_, struct list_node *head) {
  __list_add(new_, head, head->next);
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_node *new_,
                                 struct list_node *head) {
  __list_add(new_, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_node *prev, struct list_node *next) {
  next->prev = prev;
  prev->next = next;
}

/*
 * Delete a list entry and clear the 'prev' pointer.
 *
 * This is a special-purpose list clearing method used in the networking code
 * for lists allocated as per-cpu, where we don't want to incur the extra
 * WRITE_ONCE() overhead of a regular list_del_init(). The code that uses this
 * needs to check the node 'prev' pointer instead of calling list_empty().
 */
static inline void __list_del_clearprev(struct list_node *entry) {
  __list_del(entry->prev, entry->next);
  entry->prev = NULL;
}

static inline void __list_del_entry(struct list_node *entry) {
  if (!__list_del_entry_valid(entry)) return;

  __list_del(entry->prev, entry->next);
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del(struct list_node *entry) {
  __list_del_entry(entry);
  entry->next = NULL;
  entry->prev = NULL;
}

/**
 * list_replace - replace old entry by new one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace(struct list_node *old, struct list_node *new_) {
  new_->next = old->next;
  new_->next->prev = new_;
  new_->prev = old->prev;
  new_->prev->next = new_;
}

/**
 * list_replace_init - replace old entry by new one and initialize the old one
 * @old : the element to be replaced
 * @new : the new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void list_replace_init(struct list_node *old,
                                     struct list_node *new_) {
  list_replace(old, new_);
  INIT_LIST_HEAD(old);
}

/**
 * list_swap - replace entry1 with entry2 and re-add entry1 at entry2's position
 * @entry1: the location to place entry2
 * @entry2: the location to place entry1
 */
static inline void list_swap(struct list_node *entry1,
                             struct list_node *entry2) {
  struct list_node *pos = entry2->prev;

  list_del(entry2);
  list_replace(entry1, entry2);
  if (pos == entry1) pos = entry2;
  list_add(entry1, pos);
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_node *entry) {
  __list_del_entry(entry);
  INIT_LIST_HEAD(entry);
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_node *list, struct list_node *head) {
  __list_del_entry(list);
  list_add(list, head);
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(struct list_node *list,
                                  struct list_node *head) {
  __list_del_entry(list);
  list_add_tail(list, head);
}

/**
 * list_bulk_move_tail - move a subsection of a list to its tail
 * @head: the head that will follow our entry
 * @first: first entry to move
 * @last: last entry to move, can be the same as first
 *
 * Move all entries between @first and including @last before @head.
 * All three entries must belong to the same linked list.
 */
static inline void list_bulk_move_tail(struct list_node *head,
                                       struct list_node *first,
                                       struct list_node *last) {
  first->prev->next = last->next;
  last->next->prev = first->prev;

  head->prev->next = first;
  first->prev = head->prev;

  last->next = head;
  head->prev = last;
}

/**
 * list_is_first -- tests whether @list is the first entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_first(const struct list_node *list,
                                const struct list_node *head) {
  return list->prev == head;
}

/**
 * list_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int list_is_last(const struct list_node *list,
                               const struct list_node *head) {
  return list->next == head;
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(const struct list_node *head) {
  return head->next == head;
}

/**
 * list_del_init_careful - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 *
 * This is the same as list_del_init(), except designed to be used
 * together with list_empty_careful() in a way to guarantee ordering
 * of other memory operations.
 *
 * Any memory operations done before a list_del_init_careful() are
 * guaranteed to be visible after a list_empty_careful() test.
 */
// static inline void list_del_init_careful(struct list_node *entry) {
//   __list_del_entry(entry);
//   entry->prev = entry;
//   smp_store_release(&entry->next, entry);
// }

/**
 * list_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using list_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is list_del_init(). Eg. it cannot be used
 * if another CPU could re-list_add() it.
 */
// static inline int list_empty_careful(const struct list_node *head) {
//   struct list_node *next = smp_load_acquire(&head->next);
//   return (next == head) && (next == head->prev);
// }

/**
 * list_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void list_rotate_left(struct list_node *head) {
  struct list_node *first;

  if (!list_empty(head)) {
    first = head->next;
    list_move_tail(first, head);
  }
}

/**
 * list_rotate_to_front() - Rotate list to specific item.
 * @list: The desired new front of the list.
 * @head: The head of the list.
 *
 * Rotates list so that @list becomes the new front of the list.
 */
static inline void list_rotate_to_front(struct list_node *list,
                                        struct list_node *head) {
  /*
   * Deletes the list head from the list denoted by @head and
   * places it as the tail of @list, this effectively rotates the
   * list so that @list is at the front.
   */
  list_move_tail(head, list);
}

/**
 * list_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int list_is_singular(const struct list_node *head) {
  return !list_empty(head) && (head->next == head->prev);
}

static inline void __list_cut_position(struct list_node *list,
                                       struct list_node *head,
                                       struct list_node *entry) {
  struct list_node *new_first = entry->next;
  list->next = head->next;
  list->next->prev = list;
  list->prev = entry;
  entry->next = list;
  head->next = new_first;
  new_first->prev = head;
}

/**
 * list_cut_position - cut a list into two
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void list_cut_position(struct list_node *list,
                                     struct list_node *head,
                                     struct list_node *entry) {
  if (list_empty(head)) return;
  if (list_is_singular(head) && (head->next != entry && head != entry)) return;
  if (entry == head)
    INIT_LIST_HEAD(list);
  else
    __list_cut_position(list, head, entry);
}

/**
 * list_cut_before - cut a list into two, before given entry
 * @list: a new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *
 * This helper moves the initial part of @head, up to but
 * excluding @entry, from @head to @list.  You should pass
 * in @entry an element you know is on @head.  @list should
 * be an empty list or a list you do not care about losing
 * its data.
 * If @entry == @head, all entries on @head are moved to
 * @list.
 */
static inline void list_cut_before(struct list_node *list,
                                   struct list_node *head,
                                   struct list_node *entry) {
  if (head->next == entry) {
    INIT_LIST_HEAD(list);
    return;
  }
  list->next = head->next;
  list->next->prev = list;
  list->prev = entry->prev;
  list->prev->next = list;
  head->next = entry;
  entry->prev = head;
}

static inline void __list_splice(const struct list_node *list,
                                 struct list_node *prev,
                                 struct list_node *next) {
  struct list_node *first = list->next;
  struct list_node *last = list->prev;

  first->prev = prev;
  prev->next = first;

  last->next = next;
  next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(const struct list_node *list,
                               struct list_node *head) {
  if (!list_empty(list)) __list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice_tail(struct list_node *list,
                                    struct list_node *head) {
  if (!list_empty(list)) __list_splice(list, head->prev, head);
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void list_splice_init(struct list_node *list,
                                    struct list_node *head) {
  if (!list_empty(list)) {
    __list_splice(list, head, head->next);
    INIT_LIST_HEAD(list);
  }
}

/**
 * list_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void list_splice_tail_init(struct list_node *list,
                                         struct list_node *head) {
  if (!list_empty(list)) {
    __list_splice(list, head->prev, head);
    INIT_LIST_HEAD(list);
  }
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_node pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_node within the struct.
 */
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/**
 * list_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_node within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_first_entry(ptr, type, member) \
  list_entry((ptr)->next, type, member)

/**
 * list_last_entry - get the last element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_node within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)

/**
 * list_first_entry_or_null - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_node within the struct.
 *
 * Note that if the list is empty, it returns NULL.
 */
#define list_first_entry_or_null(ptr, type, member)           \
  ({                                                          \
    struct list_head *head__ = (ptr);                         \
    struct list_head *pos__ = READ_ONCE(head__->next);        \
    pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
  })

/**
 * list_next_entry - get the next element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_node within the struct.
 */
#define list_next_entry(pos, member) \
  list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - get the prev element in list
 * @pos:	the type * to cursor
 * @member:	the name of the list_node within the struct.
 */
#define list_prev_entry(pos, member) \
  list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_node to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_continue - continue iteration over a list
 * @pos:	the &struct list_node to use as a loop cursor.
 * @head:	the head for your list.
 *
 * Continue to iterate over a list, continuing after the current position.
 */
#define list_for_each_continue(pos, head) \
  for (pos = pos->next; pos != (head); pos = pos->next)

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_node to use as a loop cursor.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head) \
  for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
 * list_for_each_safe - iterate over a list safe against removal of list entry
 * @pos:	the &struct list_node to use as a loop cursor.
 * @n:		another &struct list_node to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head) \
  for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = pos->next)

/**
 * list_for_each_prev_safe - iterate over a list backwards safe against removal
 * of list entry
 * @pos:	the &struct list_node to use as a loop cursor.
 * @n:		another &struct list_node to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_prev_safe(pos, n, head) \
  for (pos = (head)->prev, n = pos->prev; pos != (head); pos = n, n = pos->prev)

/**
 * list_entry_is_head - test if the entry points to the head of the list
 * @pos:	the type * to cursor
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 */
#define list_entry_is_head(pos, head, member) (&pos->member == (head))

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 */
#define list_for_each_entry(pos, head, member)             \
  for (pos = list_first_entry(head, typeof(*pos), member); \
       !list_entry_is_head(pos, head, member);             \
       pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 */
#define list_for_each_entry_reverse(pos, head, member)    \
  for (pos = list_last_entry(head, typeof(*pos), member); \
       !list_entry_is_head(pos, head, member);            \
       pos = list_prev_entry(pos, member))

/**
 * list_prepare_entry - prepare a pos entry for use in
 * list_for_each_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_node within the struct.
 *
 * Prepares a pos entry for use as a start point in
 * list_for_each_entry_continue().
 */
#define list_prepare_entry(pos, head, member) \
  ((pos) ?: list_entry(head, typeof(*pos), member))

/**
 * list_for_each_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define list_for_each_entry_continue(pos, head, member) \
  for (pos = list_next_entry(pos, member);              \
       !list_entry_is_head(pos, head, member);          \
       pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define list_for_each_entry_continue_reverse(pos, head, member) \
  for (pos = list_prev_entry(pos, member);                      \
       !list_entry_is_head(pos, head, member);                  \
       pos = list_prev_entry(pos, member))

/**
 * list_for_each_entry_from - iterate over list of given type from the current
 * point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define list_for_each_entry_from(pos, head, member) \
  for (; !list_entry_is_head(pos, head, member);    \
       pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_from_reverse - iterate backwards over list of given type
 *                                    from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 *
 * Iterate backwards over list of given type, continuing from current position.
 */
#define list_for_each_entry_from_reverse(pos, head, member) \
  for (; !list_entry_is_head(pos, head, member);            \
       pos = list_prev_entry(pos, member))

/**
 * list_for_each_entry_safe - iterate over list of given type safe against
 * removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 */
#define list_for_each_entry_safe(pos, n, head, member)     \
  for (pos = list_first_entry(head, typeof(*pos), member), \
      n = list_next_entry(pos, member);                    \
       !list_entry_is_head(pos, head, member);             \
       pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_continue - continue list iteration safe against
 * removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define list_for_each_entry_safe_continue(pos, n, head, member)              \
  for (pos = list_next_entry(pos, member), n = list_next_entry(pos, member); \
       !list_entry_is_head(pos, head, member);                               \
       pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_from - iterate over list from current point safe
 * against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define list_for_each_entry_safe_from(pos, n, head, member) \
  for (n = list_next_entry(pos, member);                    \
       !list_entry_is_head(pos, head, member);              \
       pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_safe_reverse - iterate backwards over list safe against
 * removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_node within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define list_for_each_entry_safe_reverse(pos, n, head, member) \
  for (pos = list_last_entry(head, typeof(*pos), member),      \
      n = list_prev_entry(pos, member);                        \
       !list_entry_is_head(pos, head, member);                 \
       pos = n, n = list_prev_entry(n, member))

/**
 * list_safe_reset_next - reset a stale list_for_each_entry_safe loop
 * @pos:	the loop cursor used in the list_for_each_entry_safe loop
 * @n:		temporary storage used in list_for_each_entry_safe
 * @member:	the name of the list_node within the struct.
 *
 * list_safe_reset_next is not safe to use in general if the list may be
 * modified concurrently (eg. the lock is dropped in the loop body). An
 * exception to this is if the cursor element (pos) is pinned in the list,
 * and list_safe_reset_next is called after re-taking the lock and before
 * completing the current iteration of the loop body.
 */
#define list_safe_reset_next(pos, n, member) n = list_next_entry(pos, member)

/**
 * list double linked list.
 */
template <typename T, list_node T::*node_field>
class list {
  T *head_ptr_;
  T *tail_ptr_;

 public:
  list() : head_ptr_(0x0), tail_ptr_(0x0) {}
  ~list() { clear(); }

  /**
   * insert item at the head of list.
   * @param item item to insert in list.
   */
  void insert_head(T *elem) {}

  /**
   * insert item at the tail of list.
   * @param item item to insert in list.
   */
  void insert_tail(T *item) {}

  /**
   * insert item in list after other list item.
   * @param item item to insert in list.
   * @param in_list item that already is inserted in list.
   */
  void insert_after(T *item, T *in_list) {}

  /**
   * insert item in list before other list item.
   * @param item item to insert in list.
   * @param in_list item that already is inserted in list.
   */
  void insert_before(T *item, T *in_list) {}

  /**
   * remove the first item in the list.
   * @return the removed item.
   */
  T *remove_head() {}

  /**
   * remove the last item in the list.
   * @return the removed item.
   */
  T *remove_tail() {}

  /**
   * remove item from list.
   * @param item the element to remove
   * @note item must exist in list!
   */
  void remove(T *item) {}

  /**
   * return first item in list.
   * @return first item in list or 0x0 if list is empty.
   */
  T *head() { return nullptr; }

  /**
   * return last item in list.
   * @return last item in list or 0x0 if list is empty.
   */
  T *tail() const { return nullptr; }

  /**
   * return next element in list after argument element or 0x0 on end of list.
   * @param item item to get next element in list for.
   * @note item must exist in list!
   * @return element after item in list.
   */
  T *next(T *item) { return nullptr; }

  /**
   * return next element in list after argument element or 0x0 on end of list.
   * @param item item to get next element in list for.
   * @note item must exist in list!
   * @return element after item in list.
   */
  const T *next(const T *item) { return nullptr; }

  /**
   * return previous element in list after argument element or 0x0 on start of
   * list.
   * @param item item to get previous element in list for.
   * @note item must exist in list!
   * @return element before item in list.
   */
  T *prev(T *item) { return nullptr; }

  /**
   * return previous element in list after argument element or 0x0 on start of
   * list.
   * @param item item to get previous element in list for.
   * @note item must exist in list!
   * @return element before item in list.
   */
  const T *prev(const T *item) { return nullptr; }

  /**
   * clear queue.
   */
  void clear() {}

  /**
   * check if the list is empty.
   * @return true if list is empty.
   */
  bool empty() { return false; }

 private:
  static inline constexpr list_node *get_node(const T *item) {
    return &(item->*node_field);
  }

  static inline constexpr T *get_owner(const list_node *member) {
    return owner_of(member, node_field);
  }
};

}  // namespace list
