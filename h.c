#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>

#define HASH_SIZE 4096
#define MAX_WEIGHT 12.0
#define DB_FILE "matrix.db"

typedef enum { M_BYTE, M_INT, M_WORD, M_PUNCT, M_NL, M_CMD } MType;

struct Node;
typedef struct Conn { struct Node *to; double wt; struct Conn *next; } Conn;
typedef struct Node { 
    MType type; char *id; double nrg; Conn *conns; struct Node *next;
    union { unsigned char bval; int ival; char *sval; } pld;
} Node;

typedef struct { Node *buckets[HASH_SIZE]; Node *bytes[256]; } Net;

char *printed_registry[2048];
int printed_count = 0;

Net* create_net() {
    Net *n = calloc(1, sizeof(Net));
    for (int i = 0; i < 256; i++) {
        n->bytes[i] = calloc(1, sizeof(Node)); n->bytes[i]->type = M_BYTE; 
        n->bytes[i]->pld.bval = i; n->bytes[i]->id = malloc(16); sprintf(n->bytes[i]->id, "B_%02X", i);
        n->buckets[i % HASH_SIZE] = n->bytes[i];
    }
    for (int i = 0; i < 255; i++) {
        Conn *c = calloc(1, sizeof(Conn)); c->to = n->bytes[i+1]; c->wt = 5.0;
        c->next = n->bytes[i]->conns; n->bytes[i]->conns = c;
    }
    return n;
}

Node* find_node(Net *n, const char *id, MType t) {
    Node *c = n->buckets[abs((int)id[0]) % HASH_SIZE];
    while (c) { if (c->type == t && strcmp(c->id, id) == 0) return c; c = c->next; }
    return NULL;
}

void link_nodes(Node *f, Node *t, double base_amt) {
    if (!f || !t || f == t) return;
    for (Conn *c = f->conns; c; c = c->next) {
        if (c->to == t) { c->wt += base_amt; if (c->wt > MAX_WEIGHT) c->wt = MAX_WEIGHT; return; }
    }
    Conn *nc = calloc(1, sizeof(Conn)); nc->to = t; nc->wt = base_amt * 2.0; 
    if (nc->wt > MAX_WEIGHT) nc->wt = MAX_WEIGHT; nc->next = f->conns; f->conns = nc;
}

Node* insert_node(Net *n, const char *id, MType t) {
    Node *w = find_node(n, id, t);
    if (!w) {
        w = calloc(1, sizeof(Node)); w->type = t; w->id = strdup(id); w->pld.sval = w->id;
        int idx = abs((int)id[0]) % HASH_SIZE; w->next = n->buckets[idx]; n->buckets[idx] = w;
    }
    return w;
}

void train(Net *n, const char *txt) {
    int i = 0; Node *curr = NULL, *prev = NULL;
    while (txt[i]) {
        curr = NULL;
        if (txt[i] == '\n') {
            curr = insert_node(n, "\\n", M_NL); i++;
        } else if (ispunct((unsigned char)txt[i])) {
            char p_str[2] = {txt[i], 0}; curr = insert_node(n, p_str, M_PUNCT); i++;
        } else if (isspace((unsigned char)txt[i])) {
            i++; continue;
        } else if (isalnum((unsigned char)txt[i])) {
            int s = i; while (txt[i] && isalnum((unsigned char)txt[i])) i++;
            char *tok = strndup(&txt[s], i - s);
            MType t = (strlen(tok) > 12) ? M_CMD : M_WORD; // Long tokens default as anomalous commands
            curr = insert_node(n, tok, t); free(tok);
        } else { i++; continue; }
        
        if (prev && curr) { link_nodes(prev, curr, 1.5); link_nodes(n->bytes[(unsigned char)txt[i-1]], curr, 1.0); }
        prev = curr;
    }
}

void save_network(Net *n) {
    FILE *f = fopen(DB_FILE, "w"); if (!f) return;
    for (int i = 0; i < HASH_SIZE; i++) {
        for (Node *curr = n->buckets[i]; curr; curr = curr->next) {
            if (curr->type == M_BYTE) continue;
            if (curr->type == M_INT) fprintf(f, "N\tINT\t%s\t%d\n", curr->id, curr->pld.ival);
            else if (curr->type == M_WORD) fprintf(f, "N\tWORD\t%s\t-\n", curr->id);
            else if (curr->type == M_PUNCT) fprintf(f, "N\tPUNCT\t%s\t-\n", curr->id);
            else if (curr->type == M_NL) fprintf(f, "N\tNL\t%s\t-\n", curr->id);
            else if (curr->type == M_CMD) fprintf(f, "N\tCMD\t%s\t%s\n", curr->id, curr->pld.sval);
        }
    }
    for (int i = 0; i < HASH_SIZE; i++) {
        for (Node *curr = n->buckets[i]; curr; curr = curr->next) {
            for (Conn *c = curr->conns; c; c = c->next) {
                fprintf(f, "L\t%d\t%s\t%d\t%s\t%f\n", curr->type, curr->id, c->to->type, c->to->id, c->wt);
            }
        }
    }
    fclose(f);
}

void load_network(Net *n) {
    FILE *f = fopen(DB_FILE, "r"); if (!f) return;
    char line[1024], tag[16], type_str[16], from_id[256], val_str[256], to_id[256];
    int from_type, to_type; double wt;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%15s\t%15s\t%255s\t%255s", tag, type_str, from_id, val_str) != 4) continue;
        if (strcmp(tag, "N") == 0) {
            MType t = M_WORD;
            if (strcmp(type_str, "INT") == 0) t = M_INT;
            else if (strcmp(type_str, "PUNCT") == 0) t = M_PUNCT;
            else if (strcmp(type_str, "NL") == 0) t = M_NL;
            else if (strcmp(type_str, "CMD") == 0) t = M_CMD;
            
            Node *w = insert_node(n, from_id, t);
            if (t == M_INT) w->pld.ival = atoi(val_str);
            else if (t == M_CMD) w->pld.sval = strdup(val_str);
        }
    }
    rewind(f);
    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "L\t%d\t%255s\t%d\t%255s\t%lf", &from_type, from_id, &to_type, to_id, &wt) != 5) continue;
        Node *src = (from_type == M_BYTE) ? n->bytes[strtol(from_id + 2, NULL, 16)] : find_node(n, from_id, from_type);
        Node *dst = (to_type == M_BYTE) ? n->bytes[strtol(to_id + 2, NULL, 16)] : find_node(n, to_id, to_type);
        if (src && dst) link_nodes(src, dst, wt / 2.0);
    }
    fclose(f);
}

// Recursive Multi-Scale Fractal Energy Dispatch Engine
void fractal_push(Net *n, Node *s, double e, int depth) {
    if (!s || e < 0.02 || depth > 4) return;
    for (Conn *c = s->conns; c; c = c->next) {
        double multi_scale_nrg = e * (c->wt * 0.35) * (1.0 / (double)(1 << depth));
        c->to->nrg += multi_scale_nrg;
        fractal_push(n, c->to, multi_scale_nrg, depth + 1);
    }
}

void inject(Net *n, const char *p) {
    for (int i = 0; p[i]; i++) { n->bytes[(unsigned char)p[i]]->nrg += 20.0; fractal_push(n, n->bytes[(unsigned char)p[i]], 10.0, 1); }
    char *cp = strdup(p), *t = strtok(cp, " \t\n\r.,!?");
    while (t) {
        Node *w = find_node(n, t, M_WORD); if (!w) w = find_node(n, t, M_CMD);
        if (w) { w->nrg += 25.0; fractal_push(n, w, 12.0, 1); }
        t = strtok(NULL, " \t\n\r.,!?");
    }
    free(cp);
}

void run_cmd(Net *n, const char *trigger, const char *sys_cmd) {
    char args[256]; printf("\n[EXEC SYSTEM() - '%s']: %s\nArguments: ", trigger, sys_cmd);
    if (!fgets(args, sizeof(args), stdin)) return;
    args[strcspn(args, "\n")] = 0;
    
    char statement[512]; snprintf(statement, sizeof(statement), "%s %s", sys_cmd, args);
    FILE *f = popen(statement, "r"); if (!f) return;
    char b[128]; while (fgets(b, sizeof(b), f)) { printf(" > %s", b); inject(n, b); } pclose(f);
}

int has_been_printed(const char *id) {
    for (int i = 0; i < printed_count; i++) if (strcmp(printed_registry[i], id) == 0) return 1;
    return 0;
}

void respond(Net *n, const char *prompt, int steps) {
    for (int i = 0; i < HASH_SIZE; i++) for (Node *c = n->buckets[i]; c; c = c->next) c->nrg = 0;
    printed_count = 0; inject(n, prompt);
    printf("[Out]: ");
    
    // Chaotic seed derivation prevents deterministic repetition across similar strings
    unsigned int chaos_seed = 0; for (int i = 0; prompt[i]; i++) chaos_seed = (chaos_seed * 31) + prompt[i];
    
    Node *curr = n->bytes[(unsigned char)prompt[strlen(prompt)-1]];
    Node *cand[512]; double sc[512];
    
    for (int s = 0; s < steps; s++) {
        int cc = 0; double tot = 0;
        if (!curr) {
            for (int i = 0; i < HASH_SIZE; i++)
                for (Node *node = n->buckets[i]; node && cc < 512; node = node->next)
                    if (node->nrg > 0.02 && !has_been_printed(node->id)) { cand[cc] = node; sc[cc] = node->nrg; tot += node->nrg; cc++; }
        } else {
            for (Conn *cn = curr->conns; cn && cc < 512; cn = cn->next) {
                double score = cn->wt * (1.0 + cn->to->nrg);
                if (score > 0.02 && !has_been_printed(cn->to->id)) { cand[cc] = cn->to; sc[cc] = score; tot += score; cc++; }
            }
        }
        if (!cc || tot <= 0) break;
        
        chaos_seed = (chaos_seed * 1103515245 + 12345);
        double r = ((double)(chaos_seed & 0x7FFFFFFF) / 2147483647.0) * tot, sum = 0; Node *sel = NULL;
        for (int i = 0; i < cc; i++) { sum += sc[i]; if (r <= sum) { sel = cand[i]; break; } }
        if (!sel) sel = cand[0];

        printed_registry[printed_count++] = sel->id;
        switch (sel->type) {
            case M_BYTE:  printf("%c", sel->pld.bval); break;
            case M_WORD:  printf("%s ", sel->pld.sval); break;
            case M_PUNCT: printf("%s", sel->pld.sval); break;
            case M_NL:    printf("\n"); break;
            case M_CMD:   run_cmd(n, sel->id, sel->pld.sval); break;
            case M_INT:   printf("[%d] ", sel->pld.ival); break;
        }
        fflush(stdout); curr = sel;
        for (int i = 0; i < HASH_SIZE; i++) for (Node *d = n->buckets[i]; d; d = d->next) d->nrg *= 0.65;
    }
    printf("\n");
}

int main(int argc, char **argv) {
    Net *n = create_net();
    load_network(n);

    // Handling customized explicit system command inputs: --cmd "trigger" "command"
    if (argc == 5 && strcmp(argv[1], "--cmd") == 0) {
        char *trigger = argv[2]; char *sys_cmd = argv[3];
        Node *w = insert_node(n, trigger, M_CMD);
        free(w->pld.sval); w->pld.sval = strdup(sys_cmd);
        
        // Tie structural weights back to the leading text character sequence
        link_nodes(n->bytes[(unsigned char)trigger[0]], w, 5.0);
        train(n, argv[4]); // Treat final argument as immediate training reinforcement string
        save_network(n);
        printf("[System]: Custom shell trigger configuration saved to '%s'.\n", DB_FILE);
        return 0;
    }

    if (argc > 2 && strcmp(argv[1], "--train") == 0) {
        FILE *f = fopen(argv[2], "rb");
        if (f) { 
            fseek(f,0,SEEK_END); long l=ftell(f); fseek(f,0,SEEK_SET); char *b=malloc(l+1); fread(b,1,l,f); b[l]='\0'; 
            train(n, b); save_network(n);
            printf("[System]: Matrix updated and written to data file.\n");
            free(b); fclose(f); 
        }
        return 0;
    }

    respond(n, argc > 1 ? argv[1] : "a", 25);
    return 0;
}
