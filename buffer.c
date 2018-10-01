#define buffer_size 4095
CHAR_TYPE buffer[buffer_size];
CHAR_TYPE api_copy[buffer_size+1];   // +1 for nul terminator required by many api calls
unsigned short newline_map[buffer_size];
unsigned short line_widths[buffer_size];    // synchronized with newline_map
//line_widths[0] = 0;

unsigned int text_start = 0;
unsigned int text_length = 0;

unsigned int newline_map_start = 0;
unsigned int newline_amount = 0;

unsigned int *number_of_newlines = &newline_amount; // public use

int euclidean_modulus(int a, int b) {
    int m = a % b;
    if (m < 0) 
        m = m + b;
    return m;
}
int wrap(unsigned short a){
    if(a >= buffer_size)
        return a - buffer_size;
    else
        return a;
}
unsigned short get_line_length(unsigned short line_number){
    if(line_number == 0){
        if(newline_amount == 0)
            return text_length;
        return newline_map[0];
    }
    if(line_number == newline_amount)
        return text_length - 1 - newline_map[newline_amount - 1];

    return newline_map[line_number] - newline_map[line_number - 1];
}

unsigned short get_number_of_columns(){
    int result = 0;
    int previous_index = text_start;
    int line_length;
    for(int i = 0; i < newline_amount; i++){
        line_length = euclidean_modulus(newline_map[i]+1 - previous_index, buffer_size);
        if(line_length > result)
            result = line_length;
        previous_index = newline_map[i]+1;
    }
    line_length = euclidean_modulus(get_end() - previous_index, buffer_size);
    if(line_length > result)
        result = line_length;
    max_scroll_column = result - output_text_dimensions.width;
    if(max_scroll_column < 0)
        max_scroll_column = 0;
    return result;
}

void get_line(unsigned short line_number, CHAR_TYPE **string_pointer, unsigned int *string_length){
    if(line_number <= newline_amount){
        int line_start;
        unsigned short line_end;
        if (line_number == 0)
            line_start = text_start;
        else
            line_start = wrap(newline_map[wrap(newline_map_start + line_number - 1)] + 1); // +1, don't include the newline


        if(line_number == newline_amount)
            line_end = get_end();
        else
            line_end = newline_map[wrap(newline_map_start + line_number)] + 1;

        *string_length = euclidean_modulus(line_end - line_start, buffer_size);
        //DisplayValue(*string_length);
        *string_pointer = get_api_copy(line_start,  *string_length);
        //MessageBox(NULL, *string_pointer, "sh", MB_OK | MB_ICONERROR);

    } else {
        // return zero length string
        api_copy[0] = (TCHAR) 0;
        *string_length = 0;
        *string_pointer = &api_copy[0];
    }
}
 
CHAR_TYPE *get_api_copy(unsigned short start_position, unsigned short length){
    /* Copies the text from the specified position in the circular buffer to a region where the 
     * string is contiguous and has a NUL terminator after it.
     * For use with API calls requiring a single call with a single contiguous string.
     * Returns a pointer to the start of the region. */
    if(length == 0){
        api_copy[0] = (TCHAR) 0;
        return &api_copy[0];
    }
    unsigned int end = (start_position + length) % buffer_size;
    if (start_position < end)
        memcpy(&api_copy[0], &buffer[start_position], sizeof(CHAR_TYPE)*length);
    else {
        unsigned int first_part_length = buffer_size - start_position;
        memcpy(&api_copy[0], &buffer[start_position], sizeof(CHAR_TYPE)*first_part_length);
        memcpy(&api_copy[first_part_length], &buffer[0], sizeof(CHAR_TYPE)*(length - first_part_length));
    }
    api_copy[length] = 0;   // NUL terminator
    return &api_copy[0];
}

CHAR_TYPE *get_end_pointer(){
    return &buffer[get_end()];
}

CHAR_TYPE *get_start_pointer(){
    return &buffer[text_start];
}


unsigned int get_end(){
    return (text_start + text_length) % buffer_size;
}

unsigned int get_newline_map_end(){
    return (newline_map_start + newline_amount) % buffer_size;
}

unsigned int chars_till_edge(){
    return buffer_size - get_end();
}


void advance_start(unsigned short amount){
    if(newline_amount == 0)
        text_start = (text_start + amount) % buffer_size;
    else{
        unsigned short old_start = text_start;
        text_start = (text_start + amount) % buffer_size;
        
        /* Now we have to check if any newlines were overwritten and 
         * change the newline_amount and newline_map_start accordingly. */
        char found_next_newline = 0;

        /* In this case we use (index >= old_start) && (index < text_start)
         * as the condition to check if the character at buffer[index] was overwritten after advancing the start. */
        if(text_start > old_start){
            // relative_index is the index relative to newline_map_start
            for(int relative_index = 0; relative_index < newline_amount; relative_index++){
                int absolute_index = (newline_map_start + relative_index) % buffer_size;
                if( (newline_map[absolute_index] >= old_start) && (newline_map[absolute_index] < text_start) )
                    // newline at absolute_index was overwritten, is out of bounds
                    continue;
                else{
                    // newline at absolute_index is still in the buffer bounds
                    unsigned short newlines_removed = euclidean_modulus(absolute_index - newline_map_start, buffer_size);

                    // adjust it so it doesn't jump around
                    scrollbar_current_line -= newlines_removed;
                    if(scrollbar_current_line < 0)
                        scrollbar_current_line = 0;
                    newline_amount -= newlines_removed;
                    newline_map_start = absolute_index;

                    found_next_newline = 1;
                    break;
                }
            }
        /* Same code as above, just that in this case we use  (index >= old_start) || (index < text_start)*/
        } else {
            for(int relative_index = 0; relative_index < newline_amount; relative_index++){
                int absolute_index = (newline_map_start + relative_index) % buffer_size;
                if( (newline_map[absolute_index] >= old_start) || (newline_map[absolute_index] < text_start) )
                    continue;
                else{
                    unsigned short newlines_removed = euclidean_modulus(absolute_index - newline_map_start, buffer_size);

                    scrollbar_current_line -= newlines_removed;
                    if(scrollbar_current_line < 0)
                        scrollbar_current_line = 0;
                    newline_amount -= newlines_removed;
                    newline_map_start = absolute_index;

                    found_next_newline = 1;
                    break;
                }
            }
        }


        /* All of the newlines must have been overwritten when the start was advanced,
         *  since we never broke from the while loop */
        if(!found_next_newline){
            newline_amount = 0;
        }

    }
}

void advance_newline_map_start(int amount){
    newline_map_start = (newline_map_start + amount) % buffer_size;
}

void increase_newline_map_length(int amount){
    newline_amount += amount;
    if(newline_amount > buffer_size){
        advance_newline_map_start(newline_amount - buffer_size);
        newline_amount = buffer_size;
    }

}

// TODO: Text lengths
void analyze_new_text(unsigned short start, unsigned short end){
    
    //unsigned short previous_index = start;
    for(unsigned short i=start;i<end;i++){
        if(buffer[i] == (TCHAR) '\n'){
            unsigned short newline_map_end = get_newline_map_end();
            //line_widths[newline_map_end] += get_text_width(&buffer[previous_index], i - start);
            newline_map[newline_map_end] = i;
            increase_newline_map_length(1);
        }
    }
}

void increase_length(int amount){
    unsigned short previous_end = get_end();
    text_length += amount;
    if( text_length > buffer_size){
        advance_start(text_length - buffer_size);
        text_length = buffer_size;
    }
    unsigned short new_end = get_end();
    // analyze newly added text
    if(new_end > previous_end){
        analyze_new_text(previous_end, new_end);
    } else {
        analyze_new_text(previous_end, buffer_size);
        analyze_new_text(0, new_end);
    }
}

struct buffer_write_info get_write_info(){
    unsigned int end = get_end();
    unsigned int characters_to_edge = buffer_size - end;
    struct buffer_write_info result;
    result.end_pointer = &buffer[end];
    result.bytes_to_edge = sizeof(CHAR_TYPE)*characters_to_edge;
    result.characters_to_edge = characters_to_edge;
    return result;
};






// ----------------------
CHAR_TYPE temp;
unsigned int tempLocation;
void set_end_to_null(){
    int end = get_end();
    if( (end < text_start) ||  (end == buffer_size)){
        tempLocation = buffer_size - 1;
        temp = buffer[buffer_size - 1];
        buffer[buffer_size - 1] = 0;
    } else {
        tempLocation = end;
        temp = buffer[end];
        buffer[end] = 0;
    }
}
void reset_end(){
    buffer[tempLocation] = temp;
}
// ----------------------