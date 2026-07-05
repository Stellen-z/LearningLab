#include "DCList.h"

// 创建一个新结点
static DCListNode* BuyDCListNode(DCLDataType data)
{
    DCListNode* newNode = (DCListNode*)malloc(sizeof(DCListNode));
    if (NULL == newNode) {
        printf("BuyListNode失败!!!\n");
        exit(-1);
    }

    newNode->data = data;
    newNode->next = NULL;
    newNode->prev = NULL;
    return newNode;
}

// 链表初始化
DCListNode* DCListInit()
{
    Position sentinel = { -1, -1 };
    DCListNode* L = BuyDCListNode(sentinel);
    L->next = L;
    L->prev = L;

    return L;
}

// 销毁链表
void DCListDestroy(DCListNode* L)
{
    assert(L);

    DCListNode* cur = L->next;
    while (cur != L) {
        DCListNode* next = cur->next;
        free(cur);
        cur = next;
    }

    free(L);
}

// 获取链表的下标i的结点
DCListNode* DCListGetElem(DCListNode* L, int i)
{
    assert(L);
    assert(i >= 0);

    DCListNode* cur = L->next;
    int j = 0;
    while (cur != L && j < i) {
        cur = cur->next;
        ++j;
    }

    assert(j == i);

    return cur;
}

// 打印链表中的元素
void DCListPrint(DCListNode* L)
{
    DCListNode* cur = L->next;
    while (cur != L) {
        printf("(%d,%d)->", cur->data.x, cur->data.y);
        cur = cur->next;
    }
    printf("\n");
}

// 在pos位置后插入值为x的结点
void DCListInsert(DCListNode* pos, DCLDataType x)
{
    assert(pos);

    DCListNode* newNode = BuyDCListNode(x);

    newNode->next = pos->next;
    pos->next->prev = newNode;

    pos->next = newNode;
    newNode->prev = pos;
}

// 头插
void DCListPushFront(DCListNode* L, DCLDataType x)
{
    assert(L);
    DCListInsert(L, x);
}

// 尾插
void DCListPushBack(DCListNode* L, DCLDataType x)
{
    assert(L);
    DCListInsert(L->prev, x);
}

// 删除pos位置的结点
void DCListDelete(DCListNode* pos)
{
    assert(pos);

    pos->next->prev = pos->prev;
    pos->prev->next = pos->next;

    free(pos);
}

// 头删
void DCListPopFront(DCListNode* L)
{
    assert(L);
    assert(L->next != L);
    DCListDelete(L->next);
}

// 尾删
void DCListPopBack(DCListNode* L)
{
    assert(L);
    assert(L->next != L);
    DCListDelete(L->prev);
}

// 判空
bool DCListIsEmpty(DCListNode* L)
{
    if (L == NULL) return true;
    return L->next == L;
}

// 判断链表是否包含指定坐标
bool DCListContains(DCListNode* L, int x, int y)
{
    if (L == NULL) return false;
    DCListNode* cur = L->next;
    while (cur != L)
    {
        if (cur->data.x == x && cur->data.y == y)
            return true;
        cur = cur->next;
    }
    return false;
}
