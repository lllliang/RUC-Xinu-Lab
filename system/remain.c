#include<xinu.h>

uint16 init_remain_time(pri16 priority){
    if(priority <= 10) return T3;
    else if (priority <= 20) return T2;
    else return T1;
}

uint16 retime(pri16 priority){
    if(priority <= 10) return T3;
    else if (priority <= 20) return T2;
    else return T1;
}