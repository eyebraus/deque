
typedef struct spec_result_struct {
    unsigned int status_code;
    const char *message;
} spec_result_t;

static inline void init_spec_result(spec_result_t &result) {
    result.status_code = 0;
    result.message = "";
}

static inline void set_spec_result(spec_result_t &result, unsigned int code, const char *msg) {
    result.status_code = code;
    result.message = msg;
}
