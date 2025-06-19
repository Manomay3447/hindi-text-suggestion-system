#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>
#include <locale.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include"ngrams_hi.c"
#include"dict_trie.c"
#include"ngram_trie_hi.c"

#define WORD_MAX_LEN 100
#define MAX_FILES 100
#define WORD_LEN 64

typedef struct TrieManager {
    TrieNode *dictionaryRoot;
    TrieNode *unigramRoot;
    ngramTrieNode *bigramRoot;
    ngramTrieNode *trigramRoot;
    ngramTrieNode *fourgramRoot;
    ngramTrieNode *fivegramRoot;
} TrieManager;

char* to_utf8(const wchar_t* wstr) {
    if (!wstr) return NULL;

    size_t len = wcstombs(NULL, wstr, 0);
    if (len == (size_t)-1) {
        // Conversion failed
        return NULL;
    }

    char *mbstr = (char *)malloc(len + 1); // +1 for null terminator
    if (!mbstr) return NULL;

    wcstombs(mbstr, wstr, len + 1);
    return mbstr;
}

void suggestCompletions(TrieNode *node, wchar_t *buffer, int depth, FILE *out, int *count) {
    if (*count >= 10) return;

    if (node->isWord) {
        buffer[depth] = L'\0';

        char *utf8str = to_utf8(buffer);
        if (utf8str) {
            fprintf(out, "%s\n", utf8str);
            fwprintf(stderr, L"Suggestion[%d]: %ls\n", *count, buffer);
            free(utf8str);
        } else {
            fwprintf(stderr, L"UTF-8 conversion failed for suggestion[%d]: %ls\n", *count, buffer);
        }

        (*count)++;
    }

    for (int i = 0; i < MAX_CHILDREN; ++i) {
        if (node->children[i]) {
            buffer[depth] = (wchar_t)(i + UNICODE_BASE);
            suggestCompletions(node->children[i], buffer, depth + 1, out, count);
        }
    }
}

void fuzzySearchToFile(TrieNode *root, const wchar_t *query, int maxEdits, FILE *out) {
    wchar_t current[WORD_MAX_LEN];
    int foundCount = 0;

    void helper(TrieNode *node, const wchar_t *query, wchar_t *current, int depth, int maxEdits, int *foundCount) {
        if (*foundCount >= 10 || node == NULL) return;

        if (node->isWord && wcslen(query) <= depth + maxEdits) {
            current[depth] = L'\0';

            char *utf8str = to_utf8(current);
            if (utf8str) {
                fprintf(out, "%s\n", utf8str);
                fwprintf(stderr, L"Suggestion[%d] (fuzzy): %ls\n", *foundCount, current);
                free(utf8str);
            } else {
                fwprintf(stderr, L"UTF-8 conversion failed for fuzzy suggestion[%d]: %ls\n", *foundCount, current);
            }

            (*foundCount)++;
        }

        for (int i = 0; i < MAX_CHILDREN; i++) {
            if (node->children[i]) {
                wchar_t ch = (wchar_t)(UNICODE_BASE + i);
                current[depth] = ch;
                current[depth + 1] = L'\0';

                int cost = (depth < wcslen(query)) ? (query[depth] != ch) : 1;

                if (maxEdits - cost >= 0) {
                    helper(node->children[i], query, current, depth + 1, maxEdits - cost, foundCount);
                }
            }
        }
    }

    helper(root, query, current, 0, maxEdits, &foundCount);
}


int getSuggestionsFromTries(const wchar_t *input, TrieManager manager, FILE *out) {
    wchar_t w1[WORD_LEN] = L"", w2[WORD_LEN] = L"", w3[WORD_LEN] = L"", w4[WORD_LEN] = L"";
    wchar_t buffer[256], *tokens[4] = {NULL}, *contextState = NULL;
    int wordCount = 0;

    wcscpy(buffer, input);
    wchar_t *token = wcstok(buffer, L" ", &contextState);
    while (token && wordCount < 4) {
        tokens[wordCount++] = token;
        token = wcstok(NULL, L" ", &contextState);
    }

   if (wordCount > 0) wcscpy(w1, tokens[wordCount - 1]);
   if (wordCount > 1) wcscpy(w2, tokens[wordCount - 2]);
   if (wordCount > 2) wcscpy(w3, tokens[wordCount - 3]);
   if (wordCount > 3) wcscpy(w4, tokens[wordCount - 4]);

   // Reverse order for recent context
   wchar_t **results = NULL;
   int resultCount = 0;

   // Use most specific n-gram trie possible
   if (wordCount >= 4 && manager.fivegramRoot)
       results = searchNgramSuggestions(w4, w3, w2, w1, manager.fivegramRoot, &resultCount);
   else if (wordCount >= 3 && manager.fourgramRoot)
       results = searchNgramSuggestions(w3, w2, w1, NULL, manager.fourgramRoot, &resultCount);
   else if (wordCount >= 2 && manager.trigramRoot)
       results = searchNgramSuggestions(w2, w1, NULL, NULL, manager.trigramRoot, &resultCount);
   else if (wordCount >= 1 && manager.bigramRoot)
       results = searchNgramSuggestions(w1, NULL, NULL, NULL, manager.bigramRoot, &resultCount);
   else if (manager.unigramRoot)
       results = searchUnigramSuggestions(manager.unigramRoot, &resultCount);
    int suggestionexist = 0;
    if (results) {
	suggestionexist = 1;
        for (int i = 0; i < resultCount && i < 10; i++) {
		fwprintf(stderr, L"Suggestion[%d]: %ls\n", i, results[i]);
		char* utf8str = to_utf8(results[i]);
	        if (utf8str) 
		{
    			fprintf(out, "%s\n", utf8str);
    			free(utf8str);
		} 
		else 
		{
    			fwprintf(stderr, L"UTF-8 conversion failed for suggestion[%d]: %ls\n", i, results[i]);
		} 
        }
    }
    return suggestionexist;
}

int collect_files(const char *directory, char **files, const char *filter_keyword) {
    DIR *dir = opendir(directory);
    if (!dir) {
        perror("opendir failed");
        return -1;
    }

    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Build full path
        static char pathbuf[PATH_MAX];
        snprintf(pathbuf, sizeof(pathbuf), "%s/%s", directory, entry->d_name);

        // Ensure it's a regular file
        struct stat st;
        if (stat(pathbuf, &st) == -1 || !S_ISREG(st.st_mode))
            continue;

        // If a filter is specified, match it
        if (filter_keyword && !strstr(entry->d_name, filter_keyword))
            continue;

        files[count++] = strdup(pathbuf);
        if (count >= MAX_FILES) break;
    }

    closedir(dir);
    return count;
}

int main(int argc, char *argv[])
{
   setlocale(LC_ALL,"");
   if (argc != 3) {
        fprintf(stderr, "Usage: %s <dictionary_directory> <input_directory>\n", argv[0]);
        return 1;
    }

    const char *dict_dir = argv[1];
    const char *input_dir = argv[2];

    char *dictFiles[MAX_FILES];
    char *inputFiles[MAX_FILES];
    int dictCount = collect_files(dict_dir, dictFiles, NULL);       
    int inputCount = collect_files(input_dir, inputFiles, "input"); 

    if (dictCount < 0 || inputCount < 0) {
        fprintf(stderr, "Error reading directories.\n");
        return 1;
    }

   TrieManager manager;
   manager.dictionaryRoot = buildUnifiedTrie(inputCount, inputFiles, dictCount, dictFiles);
   manager.unigramRoot = manager.dictionaryRoot;
   generateNgrams(inputCount, inputFiles);
   manager.bigramRoot = buildNgramTrie("2grms.txt");
   manager.trigramRoot = buildNgramTrie("3grms.txt");
   manager.fourgramRoot = buildNgramTrie("4grms.txt");
   manager.fivegramRoot = buildNgramTrie("5grms.txt");
   wprintf(L"All trie Created Successfully!!\n");
   while(1)
   {   
    FILE *in = fopen("/var/www/hindi_suggestions/fifos/c_input_fifo", "r");
    FILE *out = fopen("/var/www/hindi_suggestions/fifos/c_output_fifo", "w");

    if (!in || !out) {
        perror("FIFO open failed");
        sleep(1);
	continue;
    }

    wchar_t input[256];
    while (fgetws(input, sizeof(input), in)) {
        input[wcscspn(input, L"\n")] = 0;

        if (wcslen(input) == 0) {
            fflush(out);
            continue;
        }

	char utf8buf1[512];
	wcstombs(utf8buf1, input, sizeof(utf8buf1));
	fprintf(out, "Suggestions for: %s\n", utf8buf1);
    
	wchar_t lastWord[WORD_LEN] = L"";
    	wchar_t buffer[256];
    	wcscpy(buffer, input);  // Don't destroy original input
    	wchar_t *state;
    	wchar_t *token = wcstok(buffer, L" ", &state);
    	while (token != NULL) {
        	wcscpy(lastWord, token);
        	token = wcstok(NULL, L" ", &state);
    	}

	if (searchDict(manager.dictionaryRoot, lastWord)) {
        // Exact match found in dictionary, use context-aware n-gram suggestions
        	int found = getSuggestionsFromTries(input, manager, out);
        	if (!found) {
            		fuzzySearchToFile(manager.dictionaryRoot, lastWord, 2, out);
        		}
    	} else {
        // No exact match, try prefix match
        	TrieNode *prefixNode = searchPrefix(manager.dictionaryRoot, lastWord);
        	if (prefixNode) {
            		// Suggest completions from prefix
            		fwprintf(out, L"Suggested completions for \"%ls\":\n", lastWord);
            		wchar_t buffer[WORD_MAX_LEN];
            		wcsncpy(buffer, lastWord, WORD_MAX_LEN - 1);
            		int depth = wcslen(lastWord);
            		int count = 0;
            		suggestCompletions(prefixNode, buffer, depth, out, &count);
        	} else {
            		// No prefix match, use fuzzy search
            		fuzzySearchToFile(manager.dictionaryRoot, lastWord, 2, out);
        	}
    	}

        fflush(out);
    }

    fclose(in);
    fclose(out);
   }

   for (int i = 0; i < dictCount; ++i) free(dictFiles[i]);
   for (int i = 0; i < inputCount; ++i) free(inputFiles[i]);
   freeDictTrie(manager.dictionaryRoot);
   freeDictTrie(manager.unigramRoot);
   freeNgramTrie(manager.bigramRoot);
   freeNgramTrie(manager.trigramRoot);
   freeNgramTrie(manager.fourgramRoot);
   freeNgramTrie(manager.fivegramRoot);

   return 0;
}
