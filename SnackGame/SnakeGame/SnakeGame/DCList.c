#include "DCList.h"

/*
* 内部辅助函数：创建一个新结点
* @param data  要存储的坐标数据（DCLDataType = Position）
* @return      指向新创建的 DCListNode 结点的指针
*               失败时调用 exit(-1) 终止程序
* 说明：从堆上动态申请结点内存，检测是否申请成功，
*       成功后对 data / prev / next 进行初始化。
*/
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

/*
* 链表初始化
* 创建一个带头结点的双向循环链表，哨兵结点的 data 设为 (-1, -1) 作为标记
* 初始状态：哨兵结点的 prev 和 next 都指向自身，表示空链表
* @return  指向哨兵结点的指针（即链表标识）
*/
DCListNode* DCListInit()
{
    Position sentinel = { -1, -1 };
    DCListNode* L = BuyDCListNode(sentinel);

    /* 哨兵自己指自己，形成循环 */
    L->next = L;
    L->prev = L;

    return L;
}

/*
* 销毁链表
* 释放除哨兵外所有数据结点，最后释放哨兵结点
* @param L  哨兵结点指针
*/
void DCListDestroy(DCListNode* L)
{
    assert(L);

    DCListNode* cur = L->next;
    while (cur != L) {
        DCListNode* next = cur->next;  /* 先保存后继，防止 free 后访问野指针 */
        free(cur);
        cur = next;
    }

    free(L);  /* 最后释放哨兵 */
}

/*
* 获取链表中下标为 i 的结点（从哨兵之后的第1个结点开始算 i=0）
* @param L  哨兵结点指针
* @param i  下标（从 0 开始）
* @return   指向第 i 个结点的指针，若 i 越界则触发 assert
*/
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

    assert(j == i);  /* 确保找到了第 i 个结点 */

    return cur;
}

/*
* 打印链表中的全部元素（调试用）
* 从哨兵 next 开始，按前向顺序输出每个结点的 (x, y) 坐标
* @param L  哨兵结点指针
*/
void DCListPrint(DCListNode* L)
{
    DCListNode* cur = L->next;
    while (cur != L) {
        printf("(%d,%d)->", cur->data.x, cur->data.y);
        cur = cur->next;
    }
    printf("\n");
}

/*
* 在 pos 结点之后插入值为 x 的新结点
* @param pos  插 在此结点之后插入）
* @param x    要插入的坐标数据
* 说明：插入操作涉及4个指针的修改——新结点的 prev/next 和
*       相邻结点的 prev/next。需注意赋值顺序，避免断链。
*/
void DCListInsert(DCListNode* pos, DCLDataType x)
{
    assert(pos);

    DCListNode* newNode = BuyDCListNode(x);

    /* 新结点先连接后继结点 */
    newNode->next = pos->next;
    pos->next->prev = newNode;

    /* 再与前驱结点连接 */
    pos->next = newNode;
    newNode->prev = pos;
}

/*
* 头插法：在哨兵结点后（链表头部）插入新结点
* 等价于 DCListInsert(L, x)，即插在哨兵之后
* @param L  哨兵结点指针
* @param x  要插入的坐标数据
*/
void DCListPushFront(DCListNode* L, DCLDataType x)
{
    assert(L);
    DCListInsert(L, x);
}

/*
* 尾插法：在链表末尾插入新结点
* 等价于 DCListInsert(L->prev, x)，即插在最后结点之后、哨兵之前
* @param L  哨兵结点指针
* @param x  要插入的坐标数据
*/
void DCListPushBack(DCListNode* L, DCLDataType x)
{
    assert(L);
    DCListInsert(L->prev, x);
}

/*
* 删除 pos 位置的结点
* 修改前驱和后继的指针，绕过该结点，然后释放内存
* @param pos  要删除的结点指针
*/
void DCListDelete(DCListNode* pos)
{
    assert(pos);

    /* 前驱的 next 指向后继，后继的 prev 指向前驱，绕过 pos */
    pos->next->prev = pos->prev;
    pos->prev->next = pos->next;

    free(pos);
}

/*
* 头删法：删除链表头部第1个有效结点
* 先断言非空（空链表禁止头删），然后删除哨兵的 next
* @param L  哨兵结点指针
*/
void DCListPopFront(DCListNode* L)
{
    assert(L);
    assert(L->next != L);  /* 空链表，不能头删 */
    DCListDelete(L->next);
}

/*
* 尾删法：删除链表末尾最后一个有效结点
* 先断言非空，然后删除哨兵的 prev
* @param L  哨兵结点指针
*/
void DCListPopBack(DCListNode* L)
{
    assert(L);
    assert(L->next != L);  /* 空链表，不能尾删 */
    DCListDelete(L->prev);
}

/*
* 判断链表是否为空
* 在双向循环链表中，空链表的标志是哨兵 next 指向自身
* @param L    哨兵结点指针
* @return     true=空，false=非空
*/
bool DCListIsEmpty(DCListNode* L)
{
    if (L == NULL) return true;
    return L->next == L;
}

/*
* 判断链表中是否包含指定坐标的结点
* 从哨兵 next 开始遍历，逐一比对每个结点的 (x, y)
* @param L    哨兵结点指针
* @param x    目标坐标 x
* @param y    目标坐标 y
* @return     true=存在，false=不存在
*/
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
