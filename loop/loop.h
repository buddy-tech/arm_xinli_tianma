#ifndef _LOOP_H
#define _LOOP_H

#include <pthread.h>

typedef int (*loop_search_func)(const void *, const void *);
typedef void (*loop_delete_func)(const void *);

typedef struct tag_loop_list
{
	pthread_mutex_t	mutex;
	int					size;
	int					head;
	int					tail;
	void**				items;
	loop_delete_func	del_func;	//溢出时，是否删除
}loop_list_t;

int loop_create( loop_list_t* l, int size, loop_delete_func del );
void* loop_pop_from_tail( loop_list_t* l );
void* loop_pop_from_head( loop_list_t* l );
int loop_push_to_head( loop_list_t* l, void* data );
int loop_push_to_tail( loop_list_t* l, void* data );
void* loop_get_from_head( loop_list_t* l ,int offset);
void* loop_search( loop_list_t* l, void*, loop_search_func search );
void loop_cleanup( loop_list_t* l );
void loop_clear( loop_list_t* l );
int  loop_is_empty( loop_list_t* l );
int  loop_is_full( loop_list_t* l );
void loop_remove( loop_list_t* l, void* data );

#endif
