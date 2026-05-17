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

// --- Poly-Type Enums ---
typedef enum {
    NODE_TYPE_CHAR,
    NODE_TYPE_UCHAR,
    NODE_TYPE_INT,
    NODE_TYPE_DOUBLE,
    NODE_TYPE_SHORT_TEXT,
    NODE_TYPE_LONG_TEXT,
    NODE_TYPE_FUNCTION,
    NODE_TYPE_SYS_CMD
} NodeType;

struct NeuronNode;

// Callback signature for simulated internal logic functions
typedef void (*NodeFunctionPointer)(struct NeuronNode *self, void *ctx);

typedef struct ConnectionNode {
    struct NeuronNode *target_neuron;
    double weight;
    struct ConnectionNode *next;
} ConnectionNode;

// --- Active Poly-Type Variant Layout ---
typedef struct NeuronNode {
    NodeType type;
    ConnectionNode *connections;
    struct NeuronNode *next_in_bucket;
    
    // Dynamic Activation Accumulators
    double correlation_energy;
    int is_exhausted;

    // Multi-type Storage Union
    union {
        char c_val;
        unsigned char uc_val;
        int i_val;
        double d_val;
        char *text_val; // For short texts, long texts, and system commands
        NodeFunctionPointer func_ptr;
    } value;
} NeuronNode;

typedef struct {
    NeuronNode *buckets[HASH_SIZE];
    int total_nodes;
} VariantNetwork;

// --- Infrastructure Declarations ---
VariantNetwork* create_network();
NeuronNode* init_node(NodeType type);
void add_node_to_net(VariantNetwork *net, NeuronNode *node, const char *hash_key);
void adjust_connection(NeuronNode *from, NeuronNode *to, double weight_delta);
void free_network(VariantNetwork *net);

// --- Real-time Generation, Execution, and Streaming Routing ---
void reset_runtime_states(VariantNetwork *net);
void propagate_correlation(VariantNetwork *net, NeuronNode *source, double energy, int depth);
void process_live_injection(VariantNetwork *net, const char *payload, int len);
void execute_infinite_udp_variant_stream(VariantNetwork *net);

// --- Dummy Callback Example ---
void sample_diagnostic_func(NeuronNode *self, void *ctx) {
    printf(" [FUNC_EXEC: Internal state diagnostic verified] ");
    fflush(stdout);
}

unsigned int hash_raw_bytes(const void *key, int len) {
    unsigned int hash = 5381;
    const unsigned char *p = key;
    for (int i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + p[i];
    }
    return hash % HASH_SIZE;
}

VariantNetwork* create_network() {
    return (VariantNetwork*)calloc(1, sizeof(VariantNetwork));
}

NeuronNode* init_node(NodeType type) {
    NeuronNode *node = (NeuronNode*)calloc(1, sizeof(NeuronNode));
    node->type = type;
    return node;
}

void add_node_to_net(VariantNetwork *net, NeuronNode *node, const char *hash_key) {
    unsigned int idx = hash_raw_bytes(hash_key, strlen(hash_key));
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

// Scans out to find specific textual objects inside the dictionary hashes
NeuronNode* find_text_node(VariantNetwork *net, const char *txt, NodeType expected_type) {
    unsigned int idx = hash_raw_bytes(txt, strlen(txt));
    NeuronNode *curr = net->buckets[idx];
    while (curr) {
        if (curr->type == expected_type && curr->value.text_val && strcmp(curr->value.text_val, txt) == 0) {
            return curr;
        }
        curr = curr->next_in_bucket;
    }
    return NULL;
}

void reset_runtime_states(VariantNetwork *net) {
    for (int i = 0; i < HASH_SIZE; i++) {
        NeuronNode *curr = net->buckets[i];
        while (curr != NULL) {
            curr->correlation_energy = 0.0;
            curr->is_exhausted = 0;
            curr = curr->next_in_bucket;
        }
    }
}

void propagate_correlation(VariantNetwork *net, NeuronNode *source, double energy, int depth) {
    if (!source || energy < 0.01 || depth > 4) return;
    ConnectionNode *conn = source->connections;
    while (conn != NULL) {
        conn->target_neuron->correlation_energy += energy * (conn->weight * 0.35);
        propagate_correlation(net, conn->target_neuron, energy * 0.25, depth + 1);
        conn = conn->next;
    }
}

// Evaluates inbound network lines to shock specific typing matrices
void process_live_injection(VariantNetwork *net, const char *payload, int len) {
    // Check for explicit keyword indicators to excite specialized systems
    if (strstr(payload, "trigger shell") || strstr(payload, "sys")) {
        for (int i = 0; i < HASH_SIZE; i++) {
            NeuronNode *curr = net->buckets[i];
            while (curr) {
                if (curr->type == NODE_TYPE_SYS_CMD) {
                    curr->correlation_energy += 15.0; // Heavily weight system commands
                    propagate_correlation(net, curr, 5.0, 0);
                }
                curr = curr->next_in_bucket;
            }
        }
    }

    // Baseline processing for text inputs
    char *copy = strndup(payload, len);
    char *token = strtok(copy, " \n\r\t");
    while (token != NULL) {
        NeuronNode *n = find_text_node(net, token, NODE_TYPE_SHORT_TEXT);
        if (!n) n = find_text_node(net, token, NODE_TYPE_LONG_TEXT);
        
        if (n) {
            n->correlation_energy += 5.0;
            propagate_correlation(net, n, 2.5, 0);
        }
        token = strtok(NULL, " \n\r\t");
    }
    free(copy);
}

// Executes an operating system command path, returning its output back into the network stream
void handle_system_command_node(VariantNetwork *net, const char *cmd) {
    printf("\n[EXEC SYSTEM COMMAND]: %s\n", cmd);
    fflush(stdout);

    FILE *fp = popen(cmd, "r");
    if (!fp) return;

    char result_buffer[512];
    printf("[SYS OUTPUT]: ");
    // Stream the resulting system command responses directly to the terminal
    while (fgets(result_buffer, sizeof(result_buffer), fp) != NULL) {
        printf("%s", result_buffer);
        fflush(stdout);
        // Feed command results back into the system's memory banks on the fly
        process_live_injection(net, result_buffer, strlen(result_buffer));
    }
    printf("\n[RESUMING STREAM]\n");
    fflush(stdout);
    pclose(fp);
}

// Continuously steps across node relations and processes typed executions
void execute_infinite_udp_variant_stream(VariantNetwork *net) {
    int sockfd;
    char buffer[2048];
    struct sockaddr_in servaddr, cliaddr;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(UDP_PORT);

    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        return;
    }

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    reset_runtime_states(net);

    printf("\n--- Active Poly-Variant Engine Active ---\n");
    printf("[System]: Operational on UDP Port %d\n\n", UDP_PORT);
    
    NeuronNode *curr = NULL;
    NeuronNode *candidates[1024];
    double candidate_scores[1024];
    socklen_t len = sizeof(cliaddr);

    while (1) {
        int n_bytes = recvfrom(sockfd, (char *)buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&cliaddr, &len);
        if (n_bytes > 0) {
            buffer[n_bytes] = '\0';
            process_live_injection(net, buffer, n_bytes);
        }

        int candidate_count = 0;
        double total_score_pool = 0.0;

        if (curr == NULL) {
            for (int i = 0; i < HASH_SIZE; i++) {
                NeuronNode *n = net->buckets[i];
                while (n != NULL && candidate_count < 1024) {
                    double score = n->correlation_energy + 0.01;
                    candidates[candidate_count] = n;
                    candidate_scores[candidate_count] = score;
                    total_score_pool += score;
                    candidate_count++;
                    n = n->next_in_bucket;
                }
            }
        } else {
            ConnectionNode *conn = curr->connections;
            while (conn != NULL && candidate_count < 1024) {
                NeuronNode *candidate = conn->target_neuron;
                double score = conn->weight * (1.0 + candidate->correlation_energy) + 0.01;
                candidates[candidate_count] = candidate;
                candidate_scores[candidate_count] = score;
                total_score_pool += score;
                candidate_count++;
                conn = conn->next;
            }
        }

        if (candidate_count > 0 && total_score_pool > 0.0) {
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

            // --- Execution Logic Based on Node Type ---
            switch (selected->type) {
                case NODE_TYPE_CHAR:
                    printf("%c", selected->value.c_val);
                    break;
                case NODE_TYPE_UCHAR:
                    printf("0x%02X ", selected->value.uc_val);
                    break;
                case NODE_TYPE_INT:
                    printf("%d ", selected->value.i_val);
                    break;
                case NODE_TYPE_DOUBLE:
                    printf("%g ", selected->value.d_val);
                    break;
                case NODE_TYPE_SHORT_TEXT:
                case NODE_TYPE_LONG_TEXT:
                    printf("%s ", selected->value.text_val);
                    break;
                case NODE_TYPE_FUNCTION:
                    if (selected->value.func_ptr) {
                        selected->value.func_ptr(selected, net);
                    }
                    break;
                case NODE_TYPE_SYS_CMD:
                    handle_system_command_node(net, selected->value.text_val);
                    break;
            }
            fflush(stdout);
            curr = selected;
        }

        // Context decay
        for (int i = 0; i < HASH_SIZE; i++) {
            NeuronNode *decay_n = net->buckets[i];
            while (decay_n) { 
                decay_n->correlation_energy *= 0.95; 
                decay_n = decay_n->next_in_bucket; 
            }
        }
        
        usleep(60000); 
    }
    close(sockfd);
}

void free_network(VariantNetwork *net) {
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
            if (curr->type == NODE_TYPE_SHORT_TEXT || curr->type == NODE_TYPE_LONG_TEXT || curr->type == NODE_TYPE_SYS_CMD) {
                free(curr->value.text_val);
            }
            free(curr);
            curr = next_node;
        }
    }
    free(net);
}

// Pre-populates the network structure with mixed nodes for demonstration
void load_demo_fixtures(VariantNetwork *net) {
    // 1. Core String text entities
    NeuronNode *w1 = init_node(NODE_TYPE_SHORT_TEXT); w1->value.text_val = strdup("status"); add_node_to_net(net, w1, "status");
    NeuronNode *w2 = init_node(NODE_TYPE_SHORT_TEXT); w2->value.text_val = strdup("check");  add_node_to_net(net, w2, "check");
    NeuronNode *w3 = init_node(NODE_TYPE_LONG_TEXT);  w3->value.text_val = strdup("system_metrics_report:"); add_node_to_net(net, w3, "system_metrics_report:");

    // 2. Primative numbers
    NeuronNode *n_int = init_node(NODE_TYPE_INT);  n_int->value.i_val = 101; add_node_to_net(net, n_int, "101");
    NeuronNode *n_dbl = init_node(NODE_TYPE_DOUBLE); n_dbl->value.d_val = 98.6; add_node_to_net(net, n_dbl, "98.6");

    // 3. Functional Pointer callback
    NeuronNode *n_func = init_node(NODE_TYPE_FUNCTION); n_func->value.func_ptr = sample_diagnostic_func; add_node_to_net(net, n_func, "diagnostic_callback");

    // 4. System Action Shell scripts
    NeuronNode *n_cmd1 = init_node(NODE_TYPE_SYS_CMD); n_cmd1->value.text_val = strdup("uptime"); add_node_to_net(net, n_cmd1, "uptime");
    NeuronNode *n_cmd2 = init_node(NODE_TYPE_SYS_CMD); n_cmd2->value.text_val = strdup("uname -a"); add_node_to_net(net, n_cmd2, "uname -a");

    // --- Wire up structural lanes ---
    adjust_connection(w1, w2, 2.0);      // status -> check
    adjust_connection(w2, w3, 2.5);      // check -> system_metrics_report:
    adjust_connection(w3, n_cmd1, 3.0);  // system_metrics_report: -> [bash: uptime]
    adjust_connection(n_cmd1, n_int, 1.5); // [bash: uptime] -> 101
    adjust_connection(n_int, n_func, 2.0); // 101 -> [Internal code validation logic execution callback]
    adjust_connection(n_func, n_cmd2, 2.5); // callback -> [bash: uname -a]
}

int main() {
    srand(time(NULL));
    VariantNetwork *net = create_network();
    
    load_demo_fixtures(net);
    execute_infinite_udp_variant_stream(net);

    free_network(net);
    return 0;
}
