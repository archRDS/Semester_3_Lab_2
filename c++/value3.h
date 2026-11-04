#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct AvlNode {
    int key;
    AvlNode* left;
    AvlNode* right;
    unsigned char height;

    // Extra payload for the Huffman tree in Lab 2; BST operations keep using key.
    std::uint64_t weight = 0;
    int symbol = -1;
    int minimum = 0;

    AvlNode(int k);
    AvlNode(std::uint64_t w, int s, int m, AvlNode* l, AvlNode* r)
        : key(s), left(l), right(r), height(1), weight(w), symbol(s), minimum(m) {}
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
