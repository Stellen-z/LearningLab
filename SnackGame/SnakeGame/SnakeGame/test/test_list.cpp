#include <gtest/gtest.h>

extern "C" {
#include "DCList.h"
}

TEST(ListTest, CreateAndDestroy)
{
    DCListNode *list = DCListInit();
    ASSERT_NE(list, nullptr);
    EXPECT_TRUE(DCListIsEmpty(list));
    DCListDestroy(list);
}

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
