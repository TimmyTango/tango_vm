#include <stdint.h>
#include <stdbool.h>

enum TinyVarType {
    TINY_VAR_INT = 0,
    TINY_VAR_FLOAT,
    TINY_VAR_BOOL,
};

typedef struct {
    enum TinyVarType type;
    union {
        int64_t i64;
        double f64;
        bool b;
    };
} TinyVar;

void addTinyVars(TinyVar* left, TinyVar* right, TinyVar* out) {
    if (left->type == TINY_VAR_INT && right->type == TINY_VAR_INT) {
        out->type = TINY_VAR_INT;
        out->i64 = left->i64 + right->i64;
    }
}

// int main() {
//     TinyVar v = {
//         .type = TINY_VAR_INT,
//         {.i64 = 342}
//     };

//     printf("%d\n", v.i64);
//     return 0;
// }