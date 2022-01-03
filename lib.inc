section .text

; Принимает код возврата и завершает текущий процесс
; Params:
;   rdi: Return status
; Out:
;   !exit
exit:
  mov   rax, 60        
  syscall


; Принимает указатель на нуль-терминированную строку, возвращает её длину
; Params:
;   rdi: String link
; Out:
;   rax: String length
string_length:
  xor   rax, rax
  .loop:
    cmp   byte [rdi+rax], 0
    je    .end
    inc   rax
    jmp   .loop
  .end:
    ret


; Принимает указатель на нуль-терминированную строку, выводит её в stdout
; Params:
;  rdi: String link
; Out:
;  !stdout
print_string:
  call  string_length
  mov   rdx, rax
  mov   rax, 1
  mov   rsi, rdi
  mov   rdi, 1
  syscall
  ret

; Переводит строку (выводит символ с кодом 0xA)
; Params:
;  -
; Out:
;  !stdout
print_newline:
  mov  rdi, `\n`
  ; WARN. Next there should be an label print_char


; Принимает код символа и выводит его в stdout
; Params:
;  rdi: Code of char
; Out:
;  !stdout
print_char:
  push  rdi
  mov   rdi, 1
  mov   rdx, 1
  mov   rax, 1
  mov   rsi, rsp
  syscall
  pop   rdi
  ret


; Выводит знаковое 8-байтовое число в десятичном формате 
; Params:
;  rdi: Unsigned 8-byte decimal number
; Out:
;  !stdout
print_int:
  cmp   rdi, 0
  jge   print_uint

  push  rdi
  mov   rdi, '-'
  call  print_char
  pop   rdi
  
  neg   rdi
  ; WARN. Next there should be an label print_uint


; Выводит беззнаковое 8-байтовое число в десятичном формате 
; Совет: выделите место в стеке и храните там результаты деления
; Не забудьте перевести цифры в их ASCII коды.
; Params:
;  rdi: Unsigned 8-byte decimal number
; Out:
;  !stdout
print_uint:
  mov   r10, 10
  mov   rax, rdi
  mov   rdi, rsp
  sub   rsp, 21
  dec   rdi
  mov   byte[rdi], 0
  .div_loop:
    xor   rdx, rdx
    div   r10
    add   dl, '0'
    dec   rdi
    mov   [rdi], dl
    test  rax, rax
    jne   .div_loop

  call  print_string
  add   rsp, 21
  ret


; Принимает два указателя на нуль-терминированные строки, возвращает 1 если они равны, 0 иначе
; Params:
;  rdi: First string link
;  rsi: Second string link
; Out:
;  rax: Result
string_equals:
  .loop:
    mov   al, [rdi]
    cmp   al, [rsi]
    jne   .false
    test  al, al
    je    .true
    inc   rdi
    inc   rsi
    jmp   .loop
  .false: 
    xor   rax, rax 
    ret
  .true:
    mov   rax, 1
    ret


; Читает один символ из stdin и возвращает его. Возвращает 0 если достигнут конец потока
; Params:
;  !stdin
; Out:
;  rax: Char from stdin
;       0 if the end of the stream is reached
read_char:
  push  0 
  xor   rax, rax
  xor   rdi, rdi
  mov   rdx, 1
  mov   rsi, rsp
  syscall
  pop   rax 
  ret 

; Принимает: адрес начала буфера, размер буфера
; Читает в буфер слово из stdin, пропуская пробельные символы в начале, .
; Пробельные символы это пробел 0x20, табуляция 0x9 и перевод строки 0xA.
; Останавливается и возвращает 0 если слово слишком большое для буфера
; При успехе возвращает адрес буфера в rax, длину слова в rdx.
; При неудаче возвращает 0 в rax
; Эта функция должна дописывать к слову нуль-терминатор
; Params:
;  rdi: Buffer string link
;  rsi: Buffer string size   
; Out:
;  rax: Filled buffer link
;       0 if operation failed
;  rdx: Filled buffer size 
read_word:
  mov   r8, rsi 
  mov   r9, rdi 
  xor   r10, r10 
  xor   rdi, rdi 
  mov   rdx, 1 
  .skip:
    xor   rax, rax 
    mov   rsi, r9 
    syscall
    test  rax, rax
    je    .term_zero 
    cmp   byte[r9], ' ' 
    je    .skip  
    cmp   byte[r9], `\t`
    je    .skip
    cmp   byte[r9], `\n` 
    je    .skip
    inc   r10 
  
  .read:
    xor   rax, rax 
    lea   rsi, [r9+r10] 
    syscall
    mov   cl, byte[r9+r10]
    test  rax, rax
    jz    .term_zero 
    cmp   cl, ' ' 
    je    .term_zero 
    cmp   cl, `\t`
    je    .term_zero 
    cmp   cl, `\n` 
    je    .term_zero 
    cmp   r8, r10 
    jl    .overflow 
    inc   r10 
    jmp   .read 

  .term_zero:
    mov   byte[r9+r10], 0
    mov   rdx, r10 
    mov   rax, r9
    ret
  .overflow:
    xor   rdx, r8 
    xor   rax, rax
    ret


; Принимает указатель на строку, пытается
; прочитать из её начала беззнаковое число.
; Возвращает в rax: число, rdx : его длину в символах
; rdx = 0 если число прочитать не удалось
; Params:
;  rdi: String link
; Out:
;  rax: Decimal number
;  rdx: Decimal number length
;       0 if operation failed
parse_uint:
  xor   r8, r8
  xor   rax, rax
  xor   rdx, rdx

  .loop:
    cmp   byte[rdi+rdx], '0'
    jl    .exit
    cmp   byte[rdi+rdx], '9'
    jg    .exit

    mov   r8b, [rdi+rdx] 
    sub   r8b, '0' 
    imul  rax, 10 
    add   rax, r8 
    inc   rdx 
    jmp   .loop

  .exit:
    ret


; Принимает указатель на строку, пытается
; прочитать из её начала знаковое число.
; Если есть знак, пробелы между ним и числом не разрешены.
; Возвращает в rax: число, rdx : его длину в символах (включая знак, если он был) 
; rdx = 0 если число прочитать не удалось
; Params:
;  rdi: String link
; Out:
;  rax: Decimal number
;  rdx: Decimal number length
;       0 if operation failed
parse_int:
  xor   rax, rax
  cmp   byte[rdi], '-'
  jne   parse_uint
  inc   rdi
  call  parse_uint
  neg   rax
  inc   rdx
  ret 


; Принимает указатель на строку, указатель на буфер и длину буфера
; Копирует строку в буфер
; Возвращает длину строки если она умещается в буфер, иначе 0
; Params:
;  rdi: String link
;  rsi: Buffer string link
;  rdx: Buffer string size 
; Out:
;  rax: String length
;       0 if buffer end reached
string_copy:
  xor   rcx, rcx
  .loop:
    cmp   rcx, rdx
    jge   .overflow
    mov   al, [rdi+rcx]
    mov   [rsi+rcx], al
    inc   rcx
    test  al, al
    je    .end
    jmp   .loop
  .overflow:
    xor   rax, rax
    ret
  .end:
    mov   rax, rcx
    ret


; Thanks
; https://github.com/Nessphoro/sublimeassembly
