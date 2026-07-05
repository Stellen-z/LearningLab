#pragma once
// DCList.h
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

typedef struct {
    int x, y;
} Position;

typedef Position DCLDataType;

typedef struct DCListNode {
    DCLDataType data;
    struct DCListNode* prev;
    struct DCListNode* next;
} DCListNode;

// 链表初始化
DCListNode* DCListInit();

// 销毁链表
void DCListDestroy(DCListNode* L);

// 获取链表的下标i的结点
DCListNode* DCListGetElem(DCListNode* L, int i);

// 在pos位置后插入值为x的结点
void DCListInsert(DCListNode* pos, DCLDataType x);

// 删除pos位置的结点
void DCListDelete(DCListNode* pos);

// 头插
void DCListPushFront(DCListNode* L, DCLDataType x);

// 尾插
void DCListPushBack(DCListNode* L, DCLDataType x);

// 头删
void DCListPopFront(DCListNode* L);

// 尾删
void DCListPopBack(DCListNode* L);

// 判空
bool DCListIsEmpty(DCListNode* L);

// 判断链表是否包含指定坐标
bool DCListContains(DCListNode* L, int x, int y);

// 打印链表中的元素
void DCListPrint(DCListNode* L);
