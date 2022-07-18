#include "kbd.h"

devcall vgaputc(struct dentry *devptr, char c){
    switch (c)
    {
    case 10: // 回车
        cursor_pos = NEXT_ROW_POS(cursor_pos);
        break;
    
    case 8: // 退格
        if(cursor_pos > 0){
            cursor_pos--;
            SET_CHAR(cursor_pos, 0x20);
        }
        break;
    default:
        SET_CHAR(cursor_pos, c);
        SET_CHAR(cursor_pos + 1, 0x20);
        cursor_pos++;
        break;
    }

    // 处理上卷问题
    if(cursor_pos >= MAX_COL * MAX_ROW){
        cursor_pos -= MAX_COL;
        // 下移
        memcpy(CURSOR_START_ADDR, CURSOR_START_ADDR + MAX_COL * 2, (MAX_ROW - 1) * MAX_COL * 2);
        // 最后一行清空
        for(int i = 0; i < MAX_COL; i++){
            // *(uint32*)(POS2ADDR((MAX_ROW - 1) * MAX_COL + i)) = SET_CHAR(0x20);
            SET_CHAR((MAX_ROW - 1) * MAX_COL + i, 0x20);
        }
    }
    SetCursorPos();
    return (void*)NULL;
}

devcall kbdinit(struct dentry* devptr){
    cursor_pos = 0;
    for(int i = 0; i < MAX_ROW * MAX_COL; i++){
        SET_CHAR(i, 0x20);
    }
    SetCursorPos();
    set_evec(33, (uint32)devptr->dvintr);
    return;
}

void SetCursorPos(){
    outb(0x3d4, 14);
    outb(0x3d5, cursor_pos >> 8);
    outb(0x3d4, 15);
    outb(0x3d5, cursor_pos);
}