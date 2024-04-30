#include<disassem_la.h>
extern const char *regs[];
static uint32_t GetInst(char *inst, uint32_t code){
    uint32_t opcode_31_25 = BITS(code, 31, 25);

    if (opcode_31_25 == PCADDU12I) {strcpy(inst, "pcaddu12i");return TYPE_1RI20;}
    else { strcpy(inst,"\0");return TYPE_N;}
}
void disassem_la(char *str, uint32_t code){
        char inst[30];
        uint32_t type =  GetInst(inst,code);
        Assert(inst!=NULL,"GetInst false.");

        char p[20];
        int rd = BITS(code, 4, 0);

        switch (type)
        {
        case TYPE_1RI20:
            int imm = SEXT(BITS(code, 24, 5), 20) << 12;
            sprintf(p,"  %s, %d",regs[rd],imm);
            break;
        default:
            p[0] = '\0';
            break;
        }
        strcat(inst,p);
        strcpy(str,inst);
}