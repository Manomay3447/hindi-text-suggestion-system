#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

#define MAX_CHILDREN 128            
#define UNICODE_BASE 0x0900         

// Trie Node definition
typedef struct DictTrieNode {
    struct DictTrieNode *children[MAX_CHILDREN];
    int isEndOfWord;
} DictTrieNode;

// Function to create a new TrieNode
DictTrieNode* createDictNode() {
    DictTrieNode *node = (DictTrieNode*)malloc(sizeof(DictTrieNode));
    if (!node) {
        perror("Failed to allocate TrieNode");
        exit(1);
    }
    node->isEndOfWord = 0;
    for (int i = 0; i < MAX_CHILDREN; i++)
        node->children[i] = NULL;
    return node;
}

int searchDict(DictTrieNode *root, const wchar_t *word) {
    DictTrieNode *node = root;
    int offset;
    while (*word) {
	offset = *word - UNICODE_BASE;
	if (offset < 0 || offset >= MAX_CHILDREN) {
            word++;  // Skip unsupported characters
            continue;
        }
        if (!node->children[offset]) return 0;
        node = node->children[offset];
        word++;
    }
    return node && node->isEndOfWord;
}

// Fuzzy search and write to output FIFO
void fuzzySearchToFile(DictTrieNode *root, const wchar_t *query, int maxEdits, FILE *out) {
    wchar_t current[100];
    int foundCount = 0;

    void helper(DictTrieNode *node, wchar_t *query, wchar_t *current, int depth, int maxEdits, int *foundCount) {
        if (*foundCount >= 10 || node == NULL) return;

        if (node->isEndOfWord && wcslen(query) <= depth + maxEdits) {
            fwprintf(out, L"%ls\n", current);
            (*foundCount)++;
        }

        for (int i = 0; i < MAX_CHILDREN; i++) {
            if (node->children[i]) {
                wchar_t ch = (i == 0) ? L' ' : (wchar_t)(UNICODE_BASE + i - 1);
                current[depth] = ch;
                current[depth + 1] = L'\0';

                int cost = (depth < wcslen(query)) ? (query[depth] != ch) : 1;

                if (maxEdits - cost >= 0) {
                    helper(node->children[i], query, current, depth + 1, maxEdits - cost, foundCount);
                }
            }
        }
    }

    helper(root, (wchar_t *)query, current, 0, maxEdits, &foundCount);
}

// Insert a word into the Trie
void insertWord(DictTrieNode *root, const wchar_t *word) {
    DictTrieNode *current = root;

    while (*word) {
        int offset = *word - UNICODE_BASE;

        if (offset < 0 || offset >= MAX_CHILDREN) {
            word++;
            continue;  // Skip unsupported characters (punctuation, spaces, etc.)
        }

        if (current->children[offset] == NULL)
            current->children[offset] = createDictNode();

        current = current->children[offset];
        word++;
    }

    current->isEndOfWord = 1;
}

// Recursive function to display all words in Trie
void displayDictTrie(DictTrieNode *root, wchar_t *buffer, int depth) {
    if (root->isEndOfWord) {
        buffer[depth] = L'\0';
        wprintf(L"Word: %ls\n", buffer);
        fflush(stdout);
    }

    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (root->children[i]) {
            buffer[depth] = (wchar_t)(i + UNICODE_BASE);
            displayDictTrie(root->children[i], buffer, depth + 1);
        }
    }
}

// Free the Trie memory
void freeDictTrie(DictTrieNode *root) {
    for (int i = 0; i < MAX_CHILDREN; i++) {
        if (root->children[i])
            freeDictTrie(root->children[i]);
    }
    free(root);
}

DictTrieNode *buildDictTrie(int filecount,char *filepath[]) {
    
    // Enable Unicode support for Hindi
    setlocale(LC_ALL, "");

    // Initialize Trie root
    DictTrieNode *root = createDictNode();

    // Buffer for reading lines from dictionary file
    wchar_t line[256];
    wchar_t buffer[256];

    for(int i=0;i<filecount;i++){
    // Open dictionary file in UTF-8 mode
    FILE *file = fopen(filepath[i], "r, ccs=UTF-8");
    if (file == NULL) {
        perror("Error opening dictionary_file");
        exit(1);
    }

    // Read each word from dictionary file and insert into Trie
    while (fgetws(line, sizeof(line) / sizeof(wchar_t), file)) {
        wchar_t *pos;
        // Remove trailing newline characters
        if ((pos = wcschr(line, L'\n')) != NULL) *pos = L'\0';
        if ((pos = wcschr(line, L'\r')) != NULL) *pos = L'\0';

        if (wcslen(line) > 0) {
            insertWord(root, line);
        }
    }

    fclose(file);
    }
    // Display all words from the Trie
    //wprintf(L"Words stored in Trie:\n");
    //displayDictTrie(root, buffer, 0);

    return(root);
}

