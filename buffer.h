
#ifndef char16_t
#define char16_t wchar_t
#endif

#define CHAR_TYPE char

int max_scroll_column = 0;
unsigned short get_number_of_columns();
unsigned short get_text_width(TCHAR *text_start, unsigned short text_length);
unsigned int *number_of_newlines;
int scrollbar_current_line = 0;
int scrollbar_current_column = 0;

/*typedef struct circularBuffer{
    char16_t buffer[buffer_size];
    unsigned int start;
    unsigned int length;
} circularBuffer;*/

typedef struct Dimensions {
    unsigned int width;
    unsigned int height;
} Dimensions;

Dimensions output_text_dimensions;

struct buffer_write_info{
    CHAR_TYPE *end_pointer;
    unsigned int bytes_to_edge;
    unsigned int characters_to_edge;
};
struct buffer_write_info get_write_info();

CHAR_TYPE *get_api_copy(start_position, length);
CHAR_TYPE *get_start_pointer();
CHAR_TYPE *get_end_pointer();
unsigned int get_end();
unsigned int chars_till_edge();
void advance_start(unsigned short amount);
void increase_length(int amount);
void set_end_to_null();
void reset_end();
void get_line(unsigned short line_number, CHAR_TYPE **string_pointer, unsigned int *string_length);
unsigned short get_line_length(unsigned short line_number);