/*
 * mList.h
 *
 *  Created on: 2018-6-15
 *      Author: tys
 */

#ifndef MLIST_H_
#define MLIST_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 通过结构体成员指针获取结构体指针位置 */
#define containter_of(_ptr, _type, _member) ({	\
	const typeof(((_type *)0)->_member) *__mptr = (_ptr);	\
	(_type *)((char *)__mptr - offsetof(_type, _member));})

/* 链表结构体 */
typedef struct node
{
	struct node* next;
	struct node* prev;
}node_t;

/* 链表初始化 */
static inline void init_list_head(struct node* pHead)
{
	pHead->next = pHead;
	pHead->prev = pHead;
}

static inline void __list_add(struct node* pNew,
		struct node* pPrev, struct node* pNext)
{
	pNext->prev = pNew;
	pNew->next = pNext;
	pNew->prev = pPrev;
	pPrev->next = pNew;
}

/* add data to the list head */
static inline void list_add(struct node* pNew, struct node* pHead)
{
	__list_add(pNew, pHead, pHead->next);
}

/* add data to the list tail */
static inline void list_add_tail(struct node* pNew, struct node* pHead)
{
	__list_add(pNew, pHead->prev, pHead);
}

/**
 * list_empty - check whether a list is empty
 * @head: the list to check.
 *
 * if empty return 1, else 0
 *
 */

static inline int list_empty(const struct node* pHead)
{
	return pHead->next == pHead;
}

/* Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * this is only for internal list manipulation where we know
 *
 * the prev/next entries already!
 */

static inline void __list_del(struct node* pPrev, struct node* pNext)
{
	pNext->prev = pPrev;
	pPrev->next = pNext;
}

/* delete one node from list */
static inline void list_del(struct node* pEntry)
{
	__list_del(pEntry->prev, pEntry->next);
	pEntry->next = NULL;
	pEntry->prev = NULL;
}

/* get one node from head of the list */
static inline struct node* list_take_first(struct node* pHead)
{
	node_t* pNode = NULL;
	do
	{
		if (NULL == pHead || pHead == pHead->next)
			break;

		pNode = pHead->next;
		list_del(pNode);
	}while (0);

	return pNode;
}

/* get the node count in the node list */
static inline unsigned int list_count(struct node* pHead)
{
	int cnt = 0;

	if (NULL == pHead)
		return cnt;

	node_t* pTemp = pHead->next;

	while (pTemp != pHead)
	{
		cnt++;
		pTemp = pTemp->next;
	}

	return cnt;
}

/* get the pointer of the node by member */
#define list_entry(_ptr, _type, _member)	\
	containter_of(_ptr, _type, _member)

#define list_for_each(_pos, _head)	\
	for (_pos = (_head)->next; _pos != (_head); _pos = _pos->next)

/* 遍历过程中如果对链表有删除操作需要使用这个接口 */
#define list_for_each_safe(_pos, _n, _head)	\
	for (_pos = (_head)->next, _n = _pos->next; _pos != (_head);	\
	_pos = _n, _n = _pos->next)

/* scan of all the list */
#define list_for_each_entry(_pos, _head, _member)	\
	for (_pos = list_entry((_head)->next, typeof(*_pos), _member));	\
	&_pos->_member != (_head);	\
	_pos = list_entry(_pos->_member.next, typeof(*_pos), _member))

#define list_for_each_entry_safe(_pos, _n, _head, _member)	\
	for (_pos = list_entry((_head)->next, typeof(*_pos), _member),	\
			_n = list_entry(_pos->_member.next, typeof(*_pos), _member);\
			&_pos->_member != (_head);	\
			_pos = _n, _n = list_entry(_n->_member.next, typeof(*_n), _member))

/* get the first node from the list */
#define list_first_entry(_ptr, _type, _member)	\
	list_entry((_ptr)->next, _type, _member)

#ifdef __cplusplus
}
#endif

#endif /* MLIST_H_ */
