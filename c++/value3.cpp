#include "value3.h"

using namespace std;

AvlNode::AvlNode(int k) {
    key = k;
    left = right = nullptr;
    height = 1;
}

static unsigned char getHeight(AvlNode* node) {
    if (node == nullptr) return 0;
    return node->height;
}

static int getBalanceFactor(AvlNode* node) {
    return getHeight(node->left) - getHeight(node->right);
}

static void fixHeight(AvlNode* node) {
    unsigned char left = getHeight(node->left);
    unsigned char right = getHeight(node->right);
    node->height = (left > right ? left : right) + 1;
}

// Повороты меняют связи узлов, когда дерево теряет баланс.
static AvlNode* rotateRight(AvlNode* node) {
    AvlNode* next = node->left;
    node->left = next->right;
    next->right = node;
    fixHeight(node);
    fixHeight(next);
    return next;
}

static AvlNode* rotateLeft(AvlNode* node) {
    AvlNode* next = node->right;
    node->right = next->left;
    next->left = node;
    fixHeight(node);
    fixHeight(next);
    return next;
}

// Восстанавливает баланс после вставки или удаления ключа.
static AvlNode* balance(AvlNode* node) {
    fixHeight(node);
    if (getBalanceFactor(node) == 2) {
        if (getBalanceFactor(node->left) < 0) {
            node->left = rotateLeft(node->left);
        }
        return rotateRight(node);
    }
    if (getBalanceFactor(node) == -2) {
        if (getBalanceFactor(node->right) > 0) {
            node->right = rotateRight(node->right);
        }
        return rotateLeft(node);
    }
    return node;
}

// Рекурсивно ищет место для нового ключа; одинаковые ключи пропускает.
static AvlNode* insert(AvlNode* node, int key) {
    if (node == nullptr) return new AvlNode(key);
    if (key < node->key) node->left = insert(node->left, key);
    else if (key > node->key) node->right = insert(node->right, key);
    else return node;
    return balance(node);
}

static AvlNode* findMin(AvlNode* node) {
    if (node->left == nullptr) return node;
    return findMin(node->left);
}

// Удаляет ключ; если есть два потомка, берет минимальный ключ справа.
static AvlNode* removeKey(AvlNode* node, int key) {
    if (node == nullptr) return nullptr;
    if (key < node->key) node->left = removeKey(node->left, key);
    else if (key > node->key) node->right = removeKey(node->right, key);
    else {
        if (node->left == nullptr) {
            AvlNode* right = node->right;
            delete node;
            return right;
        }
        if (node->right == nullptr) {
            AvlNode* left = node->left;
            delete node;
            return left;
        }
        AvlNode* min = findMin(node->right);
        node->key = min->key;
        node->right = removeKey(node->right, min->key);
    }
    return balance(node);
}

// Обход слева направо возвращает ключи в порядке возрастания.
static void readInOrder(AvlNode* node, vector<int>& result) {
    if (node == nullptr) return;
    readInOrder(node->left, result);
    result.push_back(node->key);
    readInOrder(node->right, result);
}

static void readPreOrder(AvlNode* node, int depth, const string& side, string& result) {
    if (node == nullptr) return;
    result += string(depth * 2, ' ') + side + ": " + to_string(node->key) + '\n';
    readPreOrder(node->left, depth + 1, "L", result);
    readPreOrder(node->right, depth + 1, "R", result);
}

static void deleteNodes(AvlNode* node) {
    if (node == nullptr) return;
    deleteNodes(node->left);
    deleteNodes(node->right);
    delete node;
}

AvlTree* avlCreate() {
    return new AvlTree;
}

void avlDestroy(AvlTree* tree) {
    deleteNodes(tree->root);
    delete tree;
}

void avlInsert(AvlTree* tree, int key) {
    tree->root = insert(tree->root, key);
}

// Ищет ключ, выбирая левое или правое поддерево.
bool avlContains(const AvlTree* tree, int key) {
    AvlNode* node = tree->root;
    while (node != nullptr) {
        if (key == node->key) return true;
        node = key < node->key ? node->left : node->right;
    }
    return false;
}

bool avlDelete(AvlTree* tree, int key) {
    if (!avlContains(tree, key)) return false;
    tree->root = removeKey(tree->root, key);
    return true;
}

vector<int> avlValues(const AvlTree* tree) {
    vector<int> result;
    readInOrder(tree->root, result);
    return result;
}

string avlReadPreOrder(const AvlTree* tree) {
    if (tree->root == nullptr) return "(empty)";
    string result;
    readPreOrder(tree->root, 0, "ROOT", result);
    result.pop_back();
    return result;
}
