/*
DList.cpp

двусвязный список
*/
/*
Copyright (c) 2009 lort
Copyright (c) 2009 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"


#include "DList.hpp"

CDList::CDList():
	count(0)
{
	root.next=&root;
	root.prev=&root;
}

void CDList::Clear()
{
	Node *f=root.next;

	while (f!=&root)
	{
		Node *f1=f;
		f=f->next;
		DeleteNode(f1);
	}

	count=0;
	root.next=&root;
	root.prev=&root;
}
void CDList::CSwap(CDList &l)
{
	const auto c1=count;
	count=l.count;
	l.count=c1;

	const auto r1=root;
	root=l.root;
	l.root=r1;

	if (count==0)
		root.next = root.prev = &root;
	else
		root.next->prev = root.prev->next = &root;

	if (l.count==0)
		l.root.next = l.root.prev = &l.root;
	else
		l.root.next->prev = l.root.prev->next = &l.root;
}
void *CDList::CInsertBefore(void *b, void *item)
{
	Node *Target=b ? (Node*)b-1 : &root;
	Node *node=AllocNode(item);
	node->prev=Target->prev;
	node->next=Target;
	Target->prev->next=node;
	Target->prev=node;
	++count;
	return node+1;
}
void *CDList::CInsertAfter(void *a, void *item)
{
	Node *Target=a ? (Node*)a-1 : &root;
	return CInsertBefore(Target->next + 1, item);
}
void CDList::CMoveBefore(void *b, void *item)
{
	if (!item) return;
	if (item == b) return;
	Node *Target=b ? (Node*)b-1 : &root;
	Node *node=(Node*)item - 1;
	node->prev->next = node->next;
	node->next->prev = node->prev;
	node->prev=Target->prev;
	node->next=Target;
	Target->prev->next=node;
	Target->prev=node;
}
void CDList::CMoveAfter(void *a, void *item)
{
	if (item == a) return;
	Node *Target=a ? (Node*)a-1 : &root;
	CMoveBefore(Target->next + 1, item);
}
void *CDList::CDelete(void *item)
{
	Node *node=(Node*)item-1;
	Node *pr=node->prev;
	Node *nx=node->next;
	pr->next=nx;
	nx->prev=pr;
	--count;
	DeleteNode(node);
	return pr==&root ? nullptr : pr+1;
}
