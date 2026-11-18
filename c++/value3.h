#pragma once

#include <string>
#include <vector>

struct AvlNode {
    int key;
    AvlNode* left;
    AvlNode* right;
    unsigned char height;

    AvlNode(int k);
};

struct AvlTree {
    AvlNode* root = nullptr;
};

AvlTree* avlCreate();
void avlDestroy(AvlTree* tree);
void avlInsert(AvlTree* tree, int key);
bool avlContains(const AvlTree* tree, int key);
bool avlDelete(AvlTree* tree, int key);
std::vector<int> avlValues(const AvlTree* tree);
std::string avlReadPreOrder(const AvlTree* tree);
