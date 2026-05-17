#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>

#define HASH_SIZE 8192
#define UDP_PORT 8888
#define DECAY_THRESHOLD 0.02

typedef enum {
    NODE_TYPE_BYTE,
    NODE_TYPE_WORD,
    NODE_TYPE_SYS_CMD
} NodeType;

struct NeuronNode;

typedef struct ConnectionNode {
    struct NeuronNode *target_neuron;
    double weight;
    struct ConnectionNode *next;
} ConnectionNode;

typedef struct NeuronNode {
    NodeType type;
    char *identifier;
    unsigned char byte_val; // Valid if NODE_TYPE_BYTE
    ConnectionNode *connections;
    struct NeuronNode *next_in_bucket;
    
    double correlation_energy;
    int is_exhausted;
} NeuronNode;

typedef struct {
    NeuronNode *buckets[HASH_SIZE];
    NeuronNode *byte_nodes[256]; // Fast lookup array for the raw ASCII table
    int total_nodes;
} SequenceNetwork;

// --- Core Infrastructure ---
SequenceNetwork* create_network();
NeuronNode* init_node(NodeType type, const char *id);
void add_node_to_net(SequenceNetwork *net, NeuronNode *node);
void adjust_connection(NeuronNode *from, NeuronNode *to, double weight_delta);
void free_network(SequenceNetwork *net);

// --- Sequence & Complex Training ---
void inject_ascii_table_matrix(SequenceNetwork *net);
void train_complex_text(SequenceNetwork *net, const char *text);
void process_live_injection(SequenceNetwork *net, const char *payload, int len);

// --- Execution Outputs ---
void execute_bounded_response(SequenceNetwork *net, const char *prompt, int steps);
void execute_infinite_udp_stream(SequenceNetwork *net);

unsigned int hash_string(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = (unsigned char)*str++)) hash = ((hash << 5) + hash) + c;
    return hash % HASH_SIZE;
}

SequenceNetwork* create_network() {
    SequenceNetwork *net = (SequenceNetwork*)calloc(1, sizeof(SequenceNetwork));
    return net;
}

NeuronNode* init_node(NodeType type, const char *id) {
    NeuronNode *node = (NeuronNode*)calloc(1, sizeof(NeuronNode));
    node->type = type;
    node->identifier = strdup(id);
    return node;
}

void add_node_to_net(SequenceNetwork *net, NeuronNode *node) {
    unsigned int idx = hash_string(node->identifier);
    node->next_in_bucket = net->buckets[idx];
    net->buckets[idx] = node;
    net->total_nodes++;
}

void adjust_connection(NeuronNode *from, NeuronNode *to, double weight_delta) {
    if (!from || !to || from == to) return;
    ConnectionNode *curr = from->connections;
    while (curr != NULL) {
        if (curr->target_neuron == to) {
            curr->weight += weight_delta;
            return;
        }
        curr = curr->next;
    }
    ConnectionNode *new_conn = (ConnectionNode*)malloc(sizeof(ConnectionNode));
    new_conn->target_neuron = to;
    new_conn->weight = weight_delta;
    new_conn->next = from->connections;
    from->connections = new_conn;
}

// --- Programmatic ASCII Table Injection ---
void inject_ascii_table_matrix(SequenceNetwork *net) {
    // 1. Allocate and map all 256 individual byte objects
    for (int i = 0; i < 256; i++) {
        char name[32];
        if (isprint(i) && i != ' ') {
            snprintf(name, sizeof(name), "BYTE_'%c'", i);
        } else {
            snprintf(name, sizeof(name), "BYTE_0x%02X", i);
        }
        
        NeuronNode *node = init_node(NODE_TYPE_BYTE, name);
        node->byte_val = (unsigned char)i;
        net->byte_nodes[i] = node;
        add_node_to_net(net, node);
    }

    // 2. Structural Wiring: Teach it the alphabet sequence (a->b->c...) and numerical orders
    // We bind every ASCII byte to its immediate mathematical successor (N -> N+1)
    for (int i = 0; i < 255; i++) {
        // High weights given to traditional alpha-numeric transitions to prioritize readable sequence flow
        double sequence_weight = 1.5;
        if ((i >= 'a' && i < 'z') || (i >= 'A' && i < 'Z') || (i >= '0' && i < '9')) {
            sequence_weight = 4.5; // Strong sequential drive for standard alphabets
        }
        adjust_connection(net->byte_nodes[i], net->byte_nodes[i+1], sequence_weight);
    }
}

NeuronNode* find_word_node(SequenceNetwork *net, const char *word) {
    unsigned int idx = hash_string(word);
    NeuronNode *curr = net->buckets[idx];
    while (curr) {
        if (curr->type == NODE_TYPE_WORD && strcmp(curr->identifier, word) == 0) {
            return curr;
        }
        curr = curr->next_in_bucket;
    }
    return NULL;
}

// Builds macro-text generation loops over the top of the ASCII structural layer
void train_complex_text(SequenceNetwork *net, const char *text) {
    int len = strlen(text);
    if (len == 0) return;

    // Link raw sequence character transitions observed inside the real text
    for (int i = 0; i < len - 1; i++) {
        unsigned char current_byte = text[i];
        unsigned char next_byte = text[i+1];
        adjust_connection(net->byte_nodes[current_byte], net->byte_nodes[next_byte], 2.5);
    }

    // Map macro semantic word abstractions
    char *text_copy = strdup(text);
    char *token = strtok(text_copy, " \n\r\t");
    NeuronNode *prev_word = NULL;

    while (token != NULL) {
        NeuronNode *word_node = find_word_node(net, token);
        if (!word_node) {
            word_node = init_node(NODE_TYPE_WORD, token);
            add_node_to_net(net, word_node);

            // Connect word node bi-directionally to its starting letter byte
            unsigned char first_letter = token[0];
            adjust_connection(word_node, net->byte_nodes[first_letter], 2.0);
            adjust_connection(net->byte_nodes[first_letter], word_node, 2.0);
        }

        if (prev_word) {
            adjust_connection(prev_word, word_node, 3.5);
        }
        prev_word = word_node;
        token = strtok(NULL, " \n\r\t");
    }
    free(text_copy);
}

void reset_runtime_states(SequenceNetwork *net) {
    for (int i = 0; i < HASH_SIZE; i++) {
        NeuronNode *curr = net->buckets[i];
        while (curr != NULL) {
            curr->correlation_energy = 0.0;
            curr->is_exhausted = 0;
            curr = curr->next_in_bucket;
        }
    }
}

void propagate_correlation(SequenceNetwork *net, NeuronNode *source, double energy, int depth) {
    if (!source || energy < 0.05 || depth > 4) return;
    ConnectionNode *conn = source->connections;
    while (conn != NULL) {
        conn->target_neuron->correlation_energy += energy * (conn->weight * 0.40);
        propagate_correlation(net, conn->target_neuron, energy * 0.20, depth + 1);
        conn = conn->next;
    }
}

void process_live_injection(SequenceNetwork *net, const char *payload, int len) {
    if (len == 0) return;

    // 1. Fire activation into the byte network matching the input string
    for (int i = 0; i < len; i++) {
        unsigned char b = payload[i];
        net->byte_nodes[b]->correlation_energy += 12.0;
        propagate_correlation(net, net->byte_nodes[b], 6.0, 0);
    }

    // 2. Fire activation into corresponding text macro tokens
    char *copy = strndup(payload, len);
    char *token = strtok(copy, " \n\r\t");
    while (token != NULL) {
        NeuronNode *w = find_word_node(net, token);
        if (w) {
            w->correlation_energy += 15.0;
            propagate_correlation(net, w, 7.5, 0);
        }
        token = strtok(NULL, " \n\r\t");
    }
    free(copy);
}

// Generates text dynamically by choosing high-probability paths across both micro and macro nodes
void execute_bounded_response(SequenceNetwork *net, const char *prompt, int steps) {
    reset_runtime_states(net);
    process_live_injection(net, prompt, strlen(prompt));

    printf("[Output]: ");
    fflush(stdout);

    // Seed output tracking with the final input byte to check for succession paths
    NeuronNode *curr = net->byte_nodes[(unsigned char)prompt[strlen(prompt) - 1]];
    NeuronNode *candidates[2048];
    double candidate_scores[2048];

    for (int step = 0; step < steps; step++) {
        int candidate_count = 0;
        double total_score_pool = 0.0;

        if (curr == NULL) {
            for (int i = 0; i < HASH_SIZE; i++) {
                NeuronNode *n = net->buckets[i];
                while (n != NULL && candidate_count < 2048) {
                    if (n->correlation_energy > DECAY_THRESHOLD) {
                        candidates[candidate_count] = n;
                        candidate_scores[candidate_count] = n->correlation_energy;
                        total_score_pool += n->correlation_energy;
                        candidate_count++;
                    }
                    n = n->next_in_bucket;
                }
            }
        } else {
            ConnectionNode *conn = curr->connections;
            while (conn != NULL && candidate_count < 2048) {
                NeuronNode *candidate = conn->target_neuron;
                // Accumulate dynamic lane activation weights
                double score = conn->weight * (1.0 + candidate->correlation_energy);
                if (score > DECAY_THRESHOLD) {
                    candidates[candidate_count] = candidate;
                    candidate_scores[candidate_count] = score;
                    total_score_pool += score;
                    candidate_count++;
                }
                conn = conn->next;
            }
        }

        if (candidate_count == 0 || total_score_pool <= 0.0) break;

        // Selection via weighted stochastic wheel distribution
        double random_point = ((double)rand() / (double)RAND_MAX) * total_score_pool;
        double rolling_sum = 0.0;
        NeuronNode *selected = NULL;

        for (int i = 0; i < candidate_count; i++) {
            rolling_sum += candidate_scores[i];
            if (random_point <= rolling_sum) {
                selected = candidates[i];
                break;
            }
        }
        if (!selected) selected = candidates[0];

        // Format and render based on selected type representation
        if (selected->type == NODE_TYPE_BYTE) {
            printf("%c", selected->byte_val);
        } else if (selected->type == NODE_TYPE_WORD) {
            printf(" %s ", selected->identifier);
        }
        fflush(stdout);

        curr = selected;

        // Apply global trace dissipation decay
        for (int i = 0; i < HASH_SIZE; i++) {
            NeuronNode *decay_n = net->buckets[i];
            while (decay_n) {
                decay_n->correlation_energy *= 0.85;
                decay_n = decay_n->next_in_bucket;
            }
        }
        usleep(25000);
    }
    printf("\n");
}

char* read_file_to_string(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return NULL;
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(length + 1);
    if (buffer) {
        size_t r = fread(buffer, 1, length, file);
        buffer[r] = '\0';
    }
    fclose(file);
    return buffer;
}

void free_network(SequenceNetwork *net) {
    for (int i = 0; i < HASH_SIZE; i++) {
        NeuronNode *curr = net->buckets[i];
        while (curr != NULL) {
            NeuronNode *next_node = curr->next_in_bucket;
            ConnectionNode *conn = curr->connections;
            while (conn != NULL) {
                ConnectionNode *next_conn = conn->next;
                free(conn);
                conn = next_conn;
            }
            free(curr->identifier);
            free(curr);
            curr = next_node;
        }
    }
    free(net);
}

int main(int argc, char *argv[]) {
    srand(time(NULL));
    SequenceNetwork *net = create_network();

    // 1. Always inject foundational systemic ASCII relational pathways first
    inject_ascii_table_matrix(net);

    // 2. Layer complex macro texts on top if available
    char *corpus = read_file_to_string("training_corpus.txt");
    if (corpus) {
        train_complex_text(net, corpus);
        free(corpus);
    }

    // 3. Evaluate runtime options
    if (argc > 2 && strcmp(argv[1], "--train") == 0) {
        char *data = read_file_to_string(argv[2]);
        if (data) {
            train_complex_text(net, data);
            printf("[System Log]: Complex phrase structures layered successfully.\n");
            free(data);
        }
    } else if (argc > 1) {
        // If passed a specific query prompt, execute a bounded response text generation step
        execute_bounded_response(net, argv[1], 30);
    } else {
        // Default evaluation test if run without arguments
        printf("[Prompt]: 'm'\n");
        execute_bounded_response(net, "m", 15);
        
        printf("[Prompt]: 'abc'\n");
        execute_bounded_response(net, "abc", 15);
    }

    free_network(net);
    return 1;
}
