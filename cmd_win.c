// ID Naming and Numbering Conventions: https://msdn.microsoft.com/en-us/library/t2zechd4.aspx
#define IDC_MAIN_OUTPUT 101
#define IDC_MAIN_RESIZER 102
#define IDC_MAIN_EDIT	103


const TCHAR childStdoutPipeName[] = TEXT("\\\\.\\pipe\\childStdoutPipe");
const TCHAR mainWindowClass[] = TEXT("mainWindow");
const TCHAR submitBoxClass[] = TEXT("submitBox");
const TCHAR resizerClass[] = TEXT("resizer");
const TCHAR outputBoxClass[] = TEXT("outputBox");
const unsigned int RESIZER_HEIGHT = 4;
const unsigned int BUFSIZE = 2048;

int resizerPositionFromBottom = 60;
unsigned short isDragging = 0;
int windowY;

HWND mainWindow;
HWND outputBox;
HWND resizer;
HWND submissionBox;

HHOOK leftMouseUpHook;
HHOOK mouseHook;

HANDLE stdin_read = NULL;
HANDLE stdin_write = NULL;
HANDLE stdout_read = NULL;
HANDLE stdout_write = NULL;

LRESULT CALLBACK (*EditBoxProcedure)(HWND, UINT, WPARAM, LPARAM);


TEXTMETRIC lpTextMetric;
Dimensions OutputBoxDimensions()
{
    
    GetTextMetrics(GetDC(outputBox), &lpTextMetric);

    RECT windowSize;
    GetClientRect(outputBox, &windowSize);

    Dimensions result;    
    result.width = windowSize.right / lpTextMetric.tmAveCharWidth;
    result.height = windowSize.bottom / lpTextMetric.tmHeight;
    return result;
}


unsigned short number_of_columns;

unsigned short number_of_lines_to_draw; // keep this global so we can see if it changes in WM_SIZE event
unsigned short number_of_lines;
unsigned short max_scroll_pos;

void update_number_of_lines(){
    char is_at_end;
    if(scrollbar_current_line == max_scroll_pos)
        is_at_end = 1;
    else    
        is_at_end = 0;

    number_of_lines = *number_of_newlines + 1;
    if(number_of_lines < output_text_dimensions.height)
        number_of_lines_to_draw = number_of_lines;
    else
        number_of_lines_to_draw = output_text_dimensions.height;
    max_scroll_pos = number_of_lines - number_of_lines_to_draw;

    if(is_at_end)
        scrollbar_current_line = max_scroll_pos;
    SCROLLINFO scroll_info;
    scroll_info.cbSize = sizeof(scroll_info);
    scroll_info.fMask = SIF_ALL;
    scroll_info.nMin = 0;
    scroll_info.nMax = number_of_lines - 1;
    scroll_info.nPage = output_text_dimensions.height;
    scroll_info.nPos = scrollbar_current_line;
    SetScrollInfo(outputBox, SB_VERT, &scroll_info, TRUE);
}
void update_horizontal_scroll_info(){
    SCROLLINFO scroll_info;
    scroll_info.cbSize = sizeof(scroll_info);
    scroll_info.fMask = SIF_ALL;
    scroll_info.nMin = 0;
    scroll_info.nMax = number_of_columns - 1;
    scroll_info.nPage = output_text_dimensions.width;
    scroll_info.nPos = scrollbar_current_column;
    SetScrollInfo(outputBox, SB_HORZ, &scroll_info, TRUE);
}

LRESULT CALLBACK OutputBoxProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_ERASEBKGND:
        {
            // DefWindowProc doesn't erase the background. Just use this one instead of writing it from scratch
            return (*EditBoxProcedure)(hwnd, msg, wParam, lParam);
        }
        case WM_VSCROLL:
        {
            int scrollPos, oldScrollPos;
            oldScrollPos = scrollbar_current_line;
            scrollPos = oldScrollPos;
            switch((int) LOWORD(wParam)){
                case SB_BOTTOM:
                    scrollPos = max_scroll_pos;
                    break;
                case SB_ENDSCROLL:
                {
                    SetScrollPos(hwnd, SB_VERT, scrollbar_current_line, TRUE);
                    return 0;
                }
                case SB_LINEDOWN:
                    scrollPos += 1;
                    break;
                case SB_LINEUP:
                    scrollPos -= 1;
                    break;
                case SB_PAGEDOWN:
                {
                    scrollPos += output_text_dimensions.height;
                    break;
                }
                case SB_PAGEUP:
                {
                    scrollPos -= output_text_dimensions.height;
                    break;
                }
                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                    scrollPos = (short) HIWORD(wParam);
                    break;
                case SB_TOP:
                    scrollPos = 0;
                    break;
            }
            if(scrollPos < 0)
                scrollPos = 0;
            else if(scrollPos > max_scroll_pos)
                scrollPos = max_scroll_pos;

            if(scrollPos != oldScrollPos){
                if(!InvalidateRect(hwnd, NULL, TRUE))
                    ErrorCode(TEXT("InvalidateRect failed with code:"), GetLastError());
                scrollbar_current_line = scrollPos;
                SetScrollPos(hwnd, SB_VERT, scrollbar_current_line, TRUE);
            }
            break;
        }
        case WM_HSCROLL:
        {
            int old_scrollbar_column = scrollbar_current_column;
            switch((int) LOWORD(wParam)){
                case SB_BOTTOM:
                    scrollbar_current_column = max_scroll_column;
                    break;
                case SB_ENDSCROLL:
                {
                    SetScrollPos(hwnd, SB_HORZ, scrollbar_current_column, TRUE);
                    return 0;
                }
                case SB_LINERIGHT:
                    scrollbar_current_column += 1;
                    break;
                case SB_LINELEFT:
                    scrollbar_current_column -= 1;
                    break;
                case SB_PAGERIGHT:
                {
                    scrollbar_current_column += output_text_dimensions.width;
                    break;
                }
                case SB_PAGELEFT:
                {
                    scrollbar_current_column -= output_text_dimensions.width;
                    break;
                }
                case SB_THUMBPOSITION:
                case SB_THUMBTRACK:
                    scrollbar_current_column = (short) HIWORD(wParam);
                    break;
                case SB_TOP:
                    scrollbar_current_column = 0;
                    break;
            }
            if(scrollbar_current_column < 0)
                scrollbar_current_column = 0;
            else if(scrollbar_current_column > max_scroll_column)
                scrollbar_current_column = max_scroll_column;

            if(scrollbar_current_column != old_scrollbar_column){
                if(!InvalidateRect(hwnd, NULL, TRUE))
                    ErrorCode(TEXT("InvalidateRect failed with code:"), GetLastError());
                SetScrollPos(hwnd, SB_HORZ, scrollbar_current_column, TRUE);
            }
            break;
        }
        case WM_PAINT:
        {
            DefWindowProc(hwnd, msg, wParam, lParam);

            RECT windowSize;
            GetClientRect(hwnd, &windowSize);

            // set to monospace font
            HFONT hfDefault = GetStockObject(ANSI_FIXED_FONT);
            HDC deviceContext = GetDC(hwnd);
            SelectObject(deviceContext, hfDefault);


            //if(scrollbar_current_line == -1){
            unsigned int line_range_start;
            if(scrollbar_current_line == -1)
                line_range_start = number_of_lines - number_of_lines_to_draw;
            else
                line_range_start = scrollbar_current_line;
            for(unsigned short i=0; i < number_of_lines_to_draw; i++){
                CHAR_TYPE *line;
                unsigned int line_length;
                get_line(line_range_start + i, &line, &line_length);
                if(line_length > scrollbar_current_column){
                    if(!ExtTextOut(deviceContext, 0, i*lpTextMetric.tmHeight, ETO_CLIPPED, &windowSize, line + scrollbar_current_column, line_length - scrollbar_current_column, NULL ))
                        ErrorCode(TEXT("ExtTextOut failed with code:"), GetLastError());
                }
            }
                
            
            break;
        }
        case WM_SIZE:
        {
            // update the global output_text_dimensions variable (from buffer.h)
            HDC deviceContext = GetDC(hwnd);
            HFONT hfDefault = GetStockObject(ANSI_FIXED_FONT);  // set device context to monospace font so we get correct text metrics
            SelectObject(deviceContext, hfDefault);

            GetTextMetrics(deviceContext, &lpTextMetric);

            RECT windowSize;
            GetClientRect(outputBox, &windowSize);

            unsigned int old_width = output_text_dimensions.width;
            output_text_dimensions.width = windowSize.right / lpTextMetric.tmAveCharWidth;
            output_text_dimensions.height = windowSize.bottom / lpTextMetric.tmHeight;

            // update the number_of_lines_to_draw
            // do it here instead of in WM_PAINT because we don't need to redraw if it hasn't changed
            unsigned short old_number_of_lines_to_draw = number_of_lines_to_draw;
            update_number_of_lines();
            update_horizontal_scroll_info();
            if( (number_of_lines_to_draw != old_number_of_lines_to_draw) || (output_text_dimensions.width > old_width) ) {
                //SendMessage(hwnd, WM_ERASEBKGND, (WPARAM) GetDC(hwnd), 0);
                if(!InvalidateRect(hwnd, NULL, TRUE))
                    ErrorCode(TEXT("InvalidateRect failed with code:"), GetLastError());
            }
            break;
        }
        case WM_KEYDOWN:
        {
            int scroll_code = -1;
            switch(wParam){
                case VK_UP:
                {
                    if(GetKeyState(VK_CONTROL) & 0x8000)    // ctrl held down
                        scroll_code = SB_LINEUP; 
                    break; 
                }
                case VK_PRIOR: 
                    scroll_code = SB_PAGEUP; 
                    break; 
     
                case VK_NEXT: 
                    scroll_code = SB_PAGEDOWN; 
                    break; 
     
                case VK_DOWN:
                    if(GetKeyState(VK_CONTROL) & 0x8000)    // ctrl held down
                        scroll_code = SB_LINEDOWN; 
                    break; 
     
                case VK_HOME:
                    if(GetKeyState(VK_CONTROL) & 0x8000)    // ctrl held down
                        scroll_code = SB_TOP; 
                    break; 
     
                case VK_END:
                    if(GetKeyState(VK_CONTROL) & 0x8000)    // ctrl held down
                        scroll_code = SB_BOTTOM; 
                    break; 
            }
            if(scroll_code != -1)
                SendMessage(hwnd, WM_VSCROLL, MAKELONG(scroll_code, 0), 0L);
            break;
        }
        case WM_MOUSEWHEEL:
        {
            int scrollPos, oldScrollPos;
            oldScrollPos = scrollbar_current_line;
            scrollPos = oldScrollPos;

            scrollPos -= ((short) HIWORD(wParam))/WHEEL_DELTA;

            if(scrollPos < 0)
                scrollPos = 0;
            else if(scrollPos > max_scroll_pos)
                scrollPos = max_scroll_pos;

            if(scrollPos != oldScrollPos){
                if(!InvalidateRect(hwnd, NULL, TRUE))
                    ErrorCode(TEXT("InvalidateRect failed with code:"), GetLastError());
                scrollbar_current_line = scrollPos;
                SetScrollPos(hwnd, SB_VERT, scrollbar_current_line, TRUE);
            }
            break;
        }
        case WM_LBUTTONDOWN:
        {
            // translate pixel coordinates to character-based coordinates
            unsigned short pixel_x = (unsigned short) LOWORD(lParam);
            unsigned short pixel_y = (unsigned short) HIWORD(lParam);
            long char_width = lpTextMetric.tmAveCharWidth;
            long char_height = lpTextMetric.tmHeight;

            // this is round(pixel_x/char_width), but without floats
            unsigned short column = scrollbar_current_column + pixel_x/char_width + (pixel_x % char_width > char_width/2);
            unsigned short line = scrollbar_current_line + pixel_y/char_height;
            if(line >= number_of_lines)
                line = number_of_lines - 1;
            unsigned short line_width = get_line_length(line);

            if(column > line_width)
                column = line_width;    // no -1 because you can select the position after the last character
            if(GetFocus() != hwnd){
                SetFocus(hwnd);
                CreateCaret(hwnd, NULL, 0, char_height);
                if(!ShowCaret(hwnd))
                    ErrorCode(TEXT("Could not ShowCaret"), 0);
            }

            if(!SetCaretPos((column - scrollbar_current_column)*char_width, (line - scrollbar_current_line)*char_height))
                ErrorCode(TEXT("SetCaretPos failed with code:"), GetLastError());
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


LRESULT CALLBACK SubmitBoxProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {

        case WM_IME_KEYDOWN:
        case WM_KEYDOWN:
        {
            // support for ctrl+a because it's not supported by default for some reason
            if((wParam == 0x41) && (GetKeyState(VK_CONTROL) & 0x8000)){
                SendMessage(hwnd, EM_SETSEL, 0, -1);
                break;
            }
            // enter key pressed, submit input to process, but only if ctrl is not held down
            else if((wParam == VK_RETURN) && !(GetKeyState(VK_CONTROL) & 0x8000) ){
                //*char16_t submitBuffer;
                //submitBuffer = SendMessage(hwnd, EM_GETHANDLE, 0, 0);

                // copy submit contents to buffer
                // +2 for \r\n
                unsigned long textLength = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0) + 2;
                
                TCHAR submittedText[textLength];
                SendMessage(hwnd, WM_GETTEXT, textLength, (LPARAM) submittedText);


                unsigned long actualLength = STRLEN(submittedText)+2;
                
                // add newline characters after it
                submittedText[actualLength - 2] = (TCHAR) '\r';
                submittedText[actualLength - 1] = (TCHAR) '\n';

                // write buffer to stdin of process
                // have to recalculate the length because microsoft sucks at unicode and can only return an upper bound
                unsigned long bytesWritten;
                if(WriteFile(stdin_write, submittedText, sizeof(TCHAR)*actualLength, &bytesWritten, NULL)){
                    SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) "");    // clear submit box
                }
                break;
            }
        }
        case WM_CHAR:
        case WM_IME_CHAR:
            // prevent this message from creating a newline when enter key is pressed
            if( ((TCHAR) wParam == '\n' || (TCHAR) wParam == '\r') && !(GetKeyState(VK_CONTROL) & 0x8000))
                break;
        default:
            return (*EditBoxProcedure)(hwnd, msg, wParam, lParam);
    }
    return 0;
}



LRESULT CALLBACK ResizerProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_LBUTTONDOWN:
            isDragging = 1;
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_CREATE:
        {
            HFONT hfDefault;

            // -------- output box ---------------
            outputBox = CreateWindowEx(WS_EX_TOPMOST, outputBoxClass, TEXT(""), 
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL, 
                0, 0, 1, 1, hwnd, (HMENU)IDC_MAIN_OUTPUT, GetModuleHandle(NULL), NULL);
            if(outputBox == NULL)
                MessageBox(hwnd, TEXT("Could not create output box."), TEXT("Error"), MB_OK | MB_ICONERROR);


            // -------- resizer ------------------
            resizer = CreateWindowEx(WS_EX_TOPMOST, resizerClass, TEXT(""), WS_CHILD | WS_VISIBLE, 0, 0, 1, 1, hwnd, (HMENU)IDC_MAIN_RESIZER, GetModuleHandle(NULL), NULL);
            if(resizer == NULL)
                MessageBox(hwnd, TEXT("Could not create resizing bar."), TEXT("Error"), MB_OK | MB_ICONERROR);


            // -------- submission box -----------
            submissionBox = CreateWindowEx(WS_EX_CLIENTEDGE, submitBoxClass, TEXT(""), 
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL, 
                0, 0, 100, 100, hwnd, (HMENU)IDC_MAIN_EDIT, GetModuleHandle(NULL), NULL);
            if(submissionBox == NULL)
                MessageBox(hwnd, TEXT("Could not create edit box."), TEXT("Error"), MB_OK | MB_ICONERROR);


            hfDefault = GetStockObject(ANSI_FIXED_FONT);
            SendMessage(submissionBox, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
            //SelectObject(GetDC(outputBox), hfDefault);
            //SendMessage(outputBox, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
        }
        break;
        case WM_SIZE:
        {

            RECT windowSize;
            GetClientRect(hwnd, &windowSize);

            SetWindowPos(outputBox, NULL, 0, 0, windowSize.right, windowSize.bottom - resizerPositionFromBottom, SWP_NOZORDER);
            SetWindowPos(resizer, NULL, 0, windowSize.bottom - resizerPositionFromBottom, windowSize.right, RESIZER_HEIGHT, SWP_NOZORDER);
            SetWindowPos(submissionBox, NULL, 0, windowSize.bottom - resizerPositionFromBottom + RESIZER_HEIGHT, windowSize.right, resizerPositionFromBottom, SWP_NOZORDER);
            break;
        }
        case WM_SETFOCUS:
            SetFocus(submissionBox);
            break;
        case WM_MOVE:
            windowY = HIWORD(lParam);
            break;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
        {
            UnhookWindowsHookEx(leftMouseUpHook);
            UnhookWindowsHookEx(mouseHook);
            PostQuitMessage(0);
            break;
        }

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


LRESULT CALLBACK MouseCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode >= 0 & isDragging){
        MOUSEHOOKSTRUCT mouseInfo = *((MOUSEHOOKSTRUCT*) lParam);
        int mouseY = mouseInfo.pt.y - windowY;

        RECT windowSize;
        GetClientRect(mainWindow, &windowSize);

        resizerPositionFromBottom = windowSize.bottom - mouseY;
        if(resizerPositionFromBottom < 40)
            resizerPositionFromBottom = 40;
        else if(resizerPositionFromBottom > (windowSize.bottom - 40))
            resizerPositionFromBottom = windowSize.bottom - 40;
        SetWindowPos(outputBox, NULL, 0, 0, windowSize.right, windowSize.bottom - resizerPositionFromBottom, SWP_NOZORDER);
        SetWindowPos(resizer, NULL, 0, windowSize.bottom - resizerPositionFromBottom, windowSize.right, RESIZER_HEIGHT, SWP_NOZORDER);
        SetWindowPos(submissionBox, NULL, 0, windowSize.bottom - resizerPositionFromBottom + RESIZER_HEIGHT, windowSize.right, resizerPositionFromBottom - RESIZER_HEIGHT, SWP_NOZORDER);
        
    }
    return CallNextHookEx(mouseHook, nCode, wParam, lParam);


}

LRESULT CALLBACK LeftButtonUpCheck(int nCode, WPARAM wParam, LPARAM lParam)
{
    if(nCode >= 0 && isDragging &&  (wParam == WM_LBUTTONUP) )
        isDragging = 0;

    return CallNextHookEx(leftMouseUpHook, nCode, wParam, lParam);
}

OVERLAPPED overlappedCrap;
void read_child_stdout();

VOID WINAPI StandardOutReadComplete(DWORD errorCode, DWORD numberOfBytesTransfered, LPOVERLAPPED overlapped){
    increase_length(numberOfBytesTransfered);
    /*set_end_to_null();
    SendMessage(outputBox, WM_SETTEXT, 0, (LPARAM) get_start_pointer());
    reset_end();*/
    update_number_of_lines();
    number_of_columns = get_number_of_columns();
    update_horizontal_scroll_info();
    //SendMessage(outputBox, WM_ERASEBKGND, (WPARAM) GetDC(outputBox), 0);
    if(!InvalidateRect(outputBox, NULL, TRUE))
        ErrorCode(TEXT("InvalidateRect failed with code:"), GetLastError());
    read_child_stdout();
}

void read_child_stdout()
{
    ZeroMemory(&overlappedCrap, sizeof(OVERLAPPED));
    struct buffer_write_info writeInfo = get_write_info();
    ReadFileEx(stdout_read, writeInfo.end_pointer, writeInfo.characters_to_edge, &overlappedCrap, 
        &StandardOutReadComplete);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wc;
    MSG messageInfo;

    // ------- Main window --------
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = MainWindowProcedure;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = mainWindowClass;
    wc.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);

    if(!RegisterClassEx(&wc))
        return InfoBox(NULL, TEXT("Main window Registration Failed"), TEXT("Error"));

    
    // ------- Submit box ---------
    GetClassInfoEx(NULL, TEXT("EDIT"), &wc);
    // Save pointer to WndProc for default EDIT box
    EditBoxProcedure = wc.lpfnWndProc;

    wc.lpfnWndProc   = SubmitBoxProcedure;
    wc.hInstance     = hInstance;
    wc.lpszClassName = submitBoxClass;

    if(!RegisterClassEx(&wc))
        return InfoBox(NULL, TEXT("Submit box Registration Failed"), TEXT("Error"));



    // ------- Output box ----------
    wc.lpfnWndProc   = OutputBoxProcedure;
    wc.lpszClassName = outputBoxClass;

    if(!RegisterClassEx(&wc))
        return InfoBox(NULL, TEXT("Output box Registration Failed"), TEXT("Error"));



    // ------- Resizing bar --------
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.style         = 0;
    wc.lpfnWndProc   = ResizerProcedure;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = NULL;
    wc.hCursor       = LoadCursor(NULL, IDC_SIZENS);
    wc.hbrBackground = (HBRUSH)(COLOR_APPWORKSPACE+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = resizerClass;
    wc.hIconSm       = NULL;
   
    if(!RegisterClassEx(&wc))
        return InfoBox(NULL, TEXT("Resizing bar Registration Failed"), TEXT("Error"));



    // ------- Create main window ---
    mainWindow = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        mainWindowClass,
        TEXT("cmdsplit"),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 660, 330,
        NULL, NULL, hInstance, NULL);
    if(mainWindow == NULL)
        return InfoBox(NULL, TEXT("Main window creation failed"), TEXT("Error"));


    // ------- Create mouse hook for detecting dragging -------------------
    mouseHook = SetWindowsHookEx(WH_MOUSE, &MouseCallback, NULL, GetCurrentThreadId());
    if(mouseHook == NULL)
        return InfoBox(NULL, TEXT("Couldn't create mouse hook"), TEXT("Error"));


    // ------- Mouse hook for detecting when left button is released ------
    leftMouseUpHook = SetWindowsHookEx(WH_MOUSE_LL, &LeftButtonUpCheck, hInstance, 0);
    if(leftMouseUpHook == NULL)
        return InfoBox(NULL, TEXT("Couldn't create leftMouseUpHook"), TEXT("Error"));





    // ------- Create the shell process ------------
    // http://archive.today/2018.08.04-062547/https://docs.microsoft.com/en-us/windows/desktop/procthread/creating-a-child-process-with-redirected-input-and-output


    // Set the bInheritHandle flag so pipe handles are inherited.
    SECURITY_ATTRIBUTES securityAttributes;
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES); 
    securityAttributes.bInheritHandle = TRUE; 
    securityAttributes.lpSecurityDescriptor = NULL; 

    // Create a pipe for the child process's STDOUT/STDERR
    // Must use named pipe for this because anonymous pipes don't support asynchronous read using ReadFileEx:
    // http://archive.today/2018.08.10-053406/https://docs.microsoft.com/en-us/windows/desktop/ipc/anonymous-pipe-operations


    stdout_read = CreateNamedPipe(
      childStdoutPipeName,
      PIPE_ACCESS_INBOUND | FILE_FLAG_OVERLAPPED,
      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
      1,
      sizeof(TCHAR) * BUFSIZE,
      sizeof(TCHAR) * BUFSIZE,
      1000,
      &securityAttributes );

    stdout_write = CreateFile(childStdoutPipeName, GENERIC_WRITE, 0, &securityAttributes, OPEN_EXISTING, 0, NULL);
    
    if ( stdout_write == INVALID_HANDLE_VALUE ) 
        return InfoBox(NULL, TEXT("Couldn't create shell stdout write pipe"), TEXT("Error"));
    // Ensure the read handle to the pipe for STDOUT is not inherited.
    if ( ! SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0) )
        return InfoBox(NULL, TEXT("Couldn't set handle info for stdout"), TEXT("Error"));

    // Create a pipe for the child process's STDIN. 
    if (! CreatePipe(&stdin_read, &stdin_write, &securityAttributes, 0))
        return InfoBox(NULL, TEXT("Couldn't create shell stdin pipe"), TEXT("Error"));
    // Ensure the write handle to the pipe for STDIN is not inherited. 
    if ( ! SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0) )
        return InfoBox(NULL, TEXT("Couldn't set handle info for stdin"), TEXT("Error"));




    PROCESS_INFORMATION processInfo; 
    STARTUPINFO startupInfo;
    BOOL success; 
 
    // Set up members of the PROCESS_INFORMATION structure. 
 
    ZeroMemory( &processInfo, sizeof(PROCESS_INFORMATION) );
 
    // Set up members of the STARTUPINFO structure. 
    // This structure specifies the STDIN and STDOUT handles for redirection.
 
    ZeroMemory( &startupInfo, sizeof(STARTUPINFO) );
    startupInfo.cb = sizeof(STARTUPINFO); 
    startupInfo.hStdError = stdout_write;
    startupInfo.hStdOutput = stdout_write;
    startupInfo.hStdInput = stdin_read;
    startupInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    startupInfo.wShowWindow = SW_HIDE;

    // Create the child process. 
     
    success = CreateProcess(NULL, 
        TEXT("cmd.exe"),        // command line 
        NULL,             // process security attributes 
        NULL,             // primary thread security attributes 
        TRUE,                           // handles are inherited 
        CREATE_UNICODE_ENVIRONMENT,     // creation flags 
        NULL,                           // use parent's environment 
        NULL,             // use parent's current directory 
        &startupInfo,     // STARTUPINFO pointer 
        &processInfo);    // receives PROCESS_INFORMATION 
    
    // If an error occurs, exit the application. 
    if ( !success ) 
        return InfoBox(NULL, TEXT("Couldn't create shell process"), TEXT("Error"));
    else 
    {
        // Close handles to the child process and its primary thread.
        // Some applications might keep these handles to monitor the status
        // of the child process, for example. 

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
    }



    // ----- Begin reading stdout -----
    read_child_stdout();

    // ----- Show window --------------
    ShowWindow(mainWindow, nCmdShow);
    UpdateWindow(mainWindow);

    
//GetConsoleMode

    // ------- The Message Loop ------
    // https://weblogs.asp.net/kennykerr/parallel-programming-with-c-part-2-asynchronous-procedure-calls-and-window-messages
    do {
        DWORD result = MsgWaitForMultipleObjectsEx(0, NULL, INFINITE, QS_ALLINPUT, MWMO_ALERTABLE | MWMO_INPUTAVAILABLE);

        /*if (result == WAIT_FAILED)
            continue;*/

        if (result == WAIT_OBJECT_0){
            // process messages until there's no more in queue
            while (PeekMessage(&messageInfo, NULL, 0, 0, PM_REMOVE))
            {
                if (messageInfo.message == WM_QUIT)
                    break;

                // https://stackoverflow.com/questions/20256016/scrolling-the-window-under-the-mouse
                POINT mouse_position;

                if (messageInfo.message == WM_MOUSEWHEEL){
                    GetCursorPos(&mouse_position);
                    messageInfo.hwnd = WindowFromPoint(mouse_position);
                }

                TranslateMessage(&messageInfo);
                DispatchMessage(&messageInfo);
            }            
        }

    } while(messageInfo.message != WM_QUIT);

    return messageInfo.wParam;
}