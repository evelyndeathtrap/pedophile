#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_TOKENS 500
#define TOKEN_LEN 64
#define CMD_LEN 256
#define THRESHOLD 3

typedef struct {
    char token[TOKEN_LEN];
    int weight;
    char command[CMD_LEN];
} Node;

typedef struct {
    int from_idx;
    int to_idx;
    int weight;
} Transition;

Node network[MAX_TOKENS];
int network_size = 0;

Transition transitions[MAX_TOKENS * 2];
int transition_count = 0;

// Helper to look up or insert a token into the network
int get_or_create_token(const char *token_str) {
    for (int i = 0; i < network_size; i++) {
        if (strcmp(network[i].token, token_str) == 0) {
            return i;
        }
    }
    if (network_size >= MAX_TOKENS) return -1;
    
    strncpy(network[network_size].token, token_str, TOKEN_LEN);
    network[network_size].weight = 0;
    memset(network[network_size].command, 0, CMD_LEN);
    network_size++;
    return network_size - 1;
}

// Track transition patterns for the generator
void record_transition(int from_idx, int to_idx) {
    for (int i = 0; i < transition_count; i++) {
        if (transitions[i].from_idx == from_idx && transitions[i].to_idx == to_idx) {
            transitions[i].weight++;
            return;
        }
    }
    if (transition_count < (MAX_TOKENS * 2)) {
        transitions[transition_count].from_idx = from_idx;
        transitions[transition_count].to_idx = to_idx;
        transitions[transition_count].weight = 1;
        transition_count++;
    }
}

void save_brain() {
    FILE *f = fopen("brain.dat", "wb");
    if (!f) return;
    fwrite(&network_size, sizeof(int), 1, f);
    fwrite(network, sizeof(Node), network_size, f);
    fwrite(&transition_count, sizeof(int), 1, f);
    fwrite(transitions, sizeof(Transition), transition_count, f);
    fclose(f);
}

void load_brain() {
    FILE *f = fopen("brain.dat", "rb");
    if (!f) return;
    if (fread(&network_size, sizeof(int), 1, f) == 1) {
        fread(network, sizeof(Node), network_size, f);
    }
    if (fread(&transition_count, sizeof(int), 1, f) == 1) {
        fread(transitions, sizeof(Transition), transition_count, f);
    }
    fclose(f);
}

// Custom stream parser to divide text into words, symbols, or newlines
int tokenize_input(const char *input, char tokens[][TOKEN_LEN]) {
    int count = 0;
    int i = 0;
    int len = strlen(input);

    while (i < len && count < 100) {
        if (input[i] == '\n') {
            strcpy(tokens[count++], "\n");
            i++;
        } else if (isspace(input[i])) {
            // Group spaces together
            int t = 0;
            while (i < len && isspace(input[i]) && input[i] != '\n' && t < TOKEN_LEN - 1) {
                tokens[count][t++] = input[i++];
            }
            tokens[count][t] = '\0';
            count++;
        } else if (isalnum(input[i])) {
            // Group alphanumeric characters into a word
            int t = 0;
            while (i < len && isalnum(input[i]) && t < TOKEN_LEN - 1) {
                tokens[count][t++] = input[i++];
            }
            tokens[count][t] = '\0';
            count++;
        } else {
            // Punctuation / standalone symbol
            tokens[count][0] = input[i];
            tokens[count][1] = '\0';
            count++;
            i++;
        }
    }
    return count;
}

void train_mode() {
    printf("=== HEBBIAN TRAINING MODE ===\n");
    printf("Provide text inputs to train the system. Type 'exit' to stop.\n");
    
    char buffer[1024];
    char tokens[100][TOKEN_LEN];

    while (1) {
        printf("\nTrain Input > ");
        if (!fgets(buffer, sizeof(buffer), stdin)) break;
        
        // Strip out early trailing newline from fgets to handle uniform exit string
        if (strncmp(buffer, "exit", 4) == 0) break;

        int total_tokens = tokenize_input(buffer, tokens);
        int last_idx = -1;

        printf("[Processing %d tokens...]\n", total_tokens);

        for (int i = 0; i < total_tokens; i++) {
            int idx = get_or_create_token(tokens[i]);
            if (idx == -1) continue;

            network[idx].weight++;

            // Handle transitions
            if (last_idx != -1) {
                record_transition(last_idx, idx);
            }
            last_idx = idx;

            // Check if connection is strong enough
            if (network[idx].weight < THRESHOLD) {
                char printable[TOKEN_LEN];
                strcpy(printable, network[idx].token);
                if (strcmp(printable, "\n") == 0) strcpy(printable, "\\n");

                printf("  ⚠️ [Anomaly]: '%s' is unfamiliar (Weight %d/%d)\n", printable, network[idx].weight, THRESHOLD);
                printf("   ? Map a system() command to this trigger? (y/n): ");
                
                char choice = getchar();
                while (getchar() != '\n'); // clear stdin buffer

                if (choice == 'y' || choice == 'Y') {
                    printf("   ? Enter system command: ");
                    if (fgets(network[idx].command, CMD_LEN, stdin)) {
                        // Remove trailing newline from command string
                        network[idx].command[strcspn(network[idx].command, "\n")] = 0;
                        printf("   ✅ Mapped successfully.\n");
                    }
                } else {
                    printf("   Skipped mapping.\n");
                }
            } else {
                // Fully wired node trigger execution
                if (strlen(network[idx].command) > 0) {
                    char printable[TOKEN_LEN];
                    strcpy(printable, network[idx].token);
                    if (strcmp(printable, "\n") == 0) strcpy(printable, "\\n");

                    printf("  🚀 [Wired Trigger '%s']: Executing bound system command\n", printable);
                    system(network[idx].command);
                }
            }
        }
        save_brain();
    }
}

void generate_mode() {
    if (network_size == 0) {
        printf("The network is empty. Train it first using --train.\n");
        return;
    }

    // Pick a random starting point token
    int current_idx = rand() % network_size;
    int max_tokens = 40;

    for (int t = 0; t < max_tokens; t++) {
        printf("%s", network[current_idx].token);

        // Find candidate matches in structural transitions
        int candidates[MAX_TOKENS];
        int candidate_weights[MAX_TOKENS];
        int total_weight = 0;
        int match_count = 0;

        for (int i = 0; i < transition_count; i++) {
            if (transitions[i].from_idx == current_idx) {
                candidates[match_count] = transitions[i].to_idx;
                candidate_weights[match_count] = transitions[i].weight;
                total_weight += transitions[i].weight;
                match_count++;
            }
        }

        if (match_count > 0 && total_weight > 0) {
            // Select via weighted probability roll
            int roll = rand() % total_weight;
            int current_sum = 0;
            for (int i = 0; i < match_count; i++) {
                current_sum += candidate_weights[i];
                if (roll < current_sum) {
                    current_idx = candidates[i];
                    break;
                }
            }
        } else {
            // Hit a dead end sequence; choose a random node to continue
            current_idx = rand() % network_size;
        }
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    load_brain();

    if (argc > 1 && strcmp(argv[1], "--train") == 0) {
        train_mode();
    } else {
        generate_mode();
    }

    return 0;
}
