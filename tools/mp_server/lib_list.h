//------------------------------------------------------------------------
//  List library
//------------------------------------------------------------------------
//
//  Edge MultiPlayer Server (C) 2005  Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#ifndef __LIB_LIST_H__
#define __LIB_LIST_H__

#include "sys_assert.h"

// (Yes, I'm probably nuts for not using std::list)

class node_c
{
friend class list_c;

private:
	node_c *nd_next;
	node_c *nd_prev;

public:
	node_c() : nd_next(NULL), nd_prev(NULL) { }
	node_c(const node_c& other) : nd_next(other.nd_next),
		nd_prev(other.nd_prev) { }
	~node_c() { }

	inline node_c& operator= (const node_c& rhs)
	{
		if (this != &rhs)
		{
			nd_next = rhs.nd_next;
			nd_prev = rhs.nd_prev;
		}
		return *this;
	}

	inline node_c *NodeNext() const { return nd_next; }
	inline node_c *NodePrev() const { return nd_prev; }
};

class list_c
{
private:
	node_c *head;
	node_c *tail;

public:
	list_c() : head(NULL), tail(NULL) { }
	list_c(const list_c& other) : head(other.head), tail(other.tail) { }
	~list_c() { }

	// FIXME: this may f**k things up (two lists sharing the nodes)
	inline list_c& operator= (const list_c& rhs)
	{
		if (this != &rhs)
		{
			head = rhs.head;
			tail = rhs.tail;
		}
		return *this;
	}

	inline node_c *begin() const { return head; }
	inline node_c *end()   const { return tail; }

	// insert_before
	// insert_after

	void remove(node_c *cur)
	{
		SYS_NULL_CHECK(cur);

		if (cur->nd_next)
			cur->nd_next->nd_prev = cur->nd_prev;
		else
			tail = cur->nd_prev;

		if (cur->nd_prev)
			cur->nd_prev->nd_next = cur->nd_next;
		else
			head = cur->nd_next;
	}

	void push_back(node_c *cur)
	{
		cur->nd_next = NULL;
		cur->nd_prev = tail;

		if (tail)
			tail->nd_next = cur;
		else
			head = cur;

		tail = cur;
	}

	void push_front(node_c *cur)
	{
		cur->nd_next = head;
		cur->nd_prev = NULL;

		if (head)
			head->nd_prev = cur;
		else
			tail = cur;

		head = cur;
	}

	inline node_c *pop_back()
	{
		node_c *cur = tail;

		if (cur)
			remove(cur);

		return cur;
	}

	inline node_c *pop_front()
	{
		node_c *cur = head;

		if (cur)
			remove(cur);

		return cur;
	}
};

#endif /* __LIB_LIST_H__ */
