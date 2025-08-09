enum Jimp_Token {
    JIMP_INVALID,
    JIMP_EOF,

    // Puncts
    JIMP_OCURLY,
    JIMP_CCURLY,
    JIMP_OBRACKET,
    JIMP_CBRACKET,
    JIMP_COMMA,
    JIMP_COLON,

    // Symbols
    JIMP_TRUE,
    JIMP_FALSE,
    JIMP_NULL,

    // Values
    JIMP_STRING,
    JIMP_NUMBER,
}

struct Jimp {
    char *file_path;
    char *start;
    char *end;
    char *point;

    Jimp_Token token;
    // TODO: `token_start` is primarily used for diagnostics location. Rename it accordingly.
    char *token_start;

    char *string;
    usz string_count;
    usz string_capacity;
    double number;
    bool boolean;
}

extern fn void begin(Jimp *jimp, char *file_path, char *input, usz input_size) @extern("jimp_begin");
extern fn bool object_begin(Jimp *jimp) @extern("jimp_object_begin");
extern fn bool object_member(Jimp *jimp) @extern("jimp_object_member");
extern fn bool object_end(Jimp *jimp) @extern("jimp_object_end");
extern fn bool number(Jimp *jimp) @extern("jimp_number");
extern fn bool string(Jimp *jimp) @extern("jimp_string");
extern fn bool array_begin(Jimp *jimp) @extern("jimp_array_begin");
extern fn bool array_item(Jimp *jimp) @extern("jimp_array_item");
extern fn bool array_end(Jimp *jimp) @extern("jimp_array_end");
extern fn void unknown_member(Jimp *jimp) @extern("jimp_unknown_member");
