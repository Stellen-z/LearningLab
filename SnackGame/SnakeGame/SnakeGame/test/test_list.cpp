/*
* test_list.cpp — DCList 双向循环链表测试
* 测试带头结点的双向循环链表全部核心 API，
* 验证链表创建、增删、查询、判空、循环性等基础功能。
* 链表以 Position {x, y} 为数据负载类型（DCLDataType）。
*/

#include <gtest/gtest.h>

extern "C" {
#include "DCList.h"
}

/* 创建并销毁链表，验证初始化后为空 */
TEST(ListTest, CreateAndDestroy)
{
    DCListNode *list = DCListInit();
    ASSERT_NE(list, nullptr);
    EXPECT_TRUE(DCListIsEmpty(list));
    DCListDestroy(list);
}

/* 头插 3 个结点，验证链表顺序正确（后插的在前面） */
TEST(ListTest, PushFront)
{
    DCListNode *list = DCListInit();
    Position p1 = { 10, 10 };
    Position p2 = { 11, 10 };
    Position p3 = { 12, 10 };
    DCListPushFront(list, p3);
    DCListPushFront(list, p2);
    DCListPushFront(list, p1);

    EXPECT_FALSE(DCListIsEmpty(list));

    DCListNode *node = list->next;
    EXPECT_EQ(node->data.x, 10);
    EXPECT_EQ(node->data.y, 10);

    node = node->next;
    EXPECT_EQ(node->data.x, 11);
    EXPECT_EQ(node->data.y, 10);

    node = node->next;
    EXPECT_EQ(node->data.x, 12);
    EXPECT_EQ(node->data.y, 10);

    DCListDestroy(list);
}

/* 尾插 3 个结点，验证尾部即为最后插入的结点 */
TEST(ListTest, PushBack)
{
    DCListNode *list = DCListInit();
    Position p1 = { 0, 0 };
    Position p2 = { 1, 0 };
    Position p3 = { 2, 0 };
    DCListPushBack(list, p1);
    DCListPushBack(list, p2);
    DCListPushBack(list, p3);

    DCListNode *node = list->prev;
    EXPECT_EQ(node->data.x, 2);
    EXPECT_EQ(node->data.y, 0);

    node = node->prev;
    EXPECT_EQ(node->data.x, 1);
    EXPECT_EQ(node->data.y, 0);

    DCListDestroy(list);
}

/* 尾插后尾删一次，验证新尾部正确 */
TEST(ListTest, PopBack)
{
    DCListNode *list = DCListInit();
    Position p1 = { 1, 1 };
    Position p2 = { 2, 2 };
    Position p3 = { 3, 3 };
    DCListPushBack(list, p1);
    DCListPushBack(list, p2);
    DCListPushBack(list, p3);

    DCListPopBack(list);
    DCListNode *tail = list->prev;
    EXPECT_EQ(tail->data.x, 2);
    EXPECT_EQ(tail->data.y, 2);

    DCListDestroy(list);
}

/* 尾插后头删一次，验证新头部正确 */
TEST(ListTest, PopFront)
{
    DCListNode *list = DCListInit();
    Position p1 = { 1, 1 };
    Position p2 = { 2, 2 };
    Position p3 = { 3, 3 };
    DCListPushBack(list, p1);
    DCListPushBack(list, p2);
    DCListPushBack(list, p3);

    DCListPopFront(list);
    DCListNode *front = list->next;
    EXPECT_EQ(front->data.x, 2);
    EXPECT_EQ(front->data.y, 2);

    DCListDestroy(list);
}

/* 判空：空链表返回 true，插入后返回 false，删光后恢复 true */
TEST(ListTest, IsEmpty)
{
    DCListNode *list = DCListInit();
    EXPECT_TRUE(DCListIsEmpty(list));

    Position p = { 5, 5 };
    DCListPushFront(list, p);
    EXPECT_FALSE(DCListIsEmpty(list));

    DCListPopBack(list);
    EXPECT_TRUE(DCListIsEmpty(list));

    DCListDestroy(list);
}

/* 包含检测：插入的坐标返回 true，不存在的坐标返回 false */
TEST(ListTest, Contains)
{
    DCListNode *list = DCListInit();
    Position p1 = { 3, 5 };
    Position p2 = { 7, 2 };
    DCListPushBack(list, p1);
    DCListPushBack(list, p2);

    EXPECT_TRUE(DCListContains(list, 3, 5));
    EXPECT_TRUE(DCListContains(list, 7, 2));
    EXPECT_FALSE(DCListContains(list, 0, 0));
    EXPECT_FALSE(DCListContains(list, 3, 6));

    DCListDestroy(list);
}

/* 循环性：哨兵的前驱的后继 = 哨兵，哨兵的后继的前驱 = 哨兵 */
TEST(ListTest, Circular)
{
    DCListNode *list = DCListInit();
    Position p1 = { 1, 0 };
    Position p2 = { 2, 0 };
    Position p3 = { 3, 0 };
    DCListPushBack(list, p1);
    DCListPushBack(list, p2);
    DCListPushBack(list, p3);

    EXPECT_EQ(list->prev->next, list);
    EXPECT_EQ(list->next->prev, list);

    DCListDestroy(list);
}

/* 混合操作：尾插 5 → 尾删 3 → 头插 1，验证最终链表状态 */
TEST(ListTest, MixedPushPop)
{
    DCListNode *list = DCListInit();
    for (int i = 0; i < 5; i++)
    {
        Position p = { i, 0 };
        DCListPushBack(list, p);
    }

    for (int i = 0; i < 3; i++)
        DCListPopBack(list);

    Position newP = { 99, 99 };
    DCListPushFront(list, newP);
    EXPECT_EQ(list->next->data.x, 99);
    EXPECT_EQ(list->next->data.y, 99);

    DCListDestroy(list);
}
