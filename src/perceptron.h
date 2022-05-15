#include <stdio.h>
#include <stdint.h>

#define NUMWEIGHT 128
#define NUMTABLE 64
#define MAXWEIGHT 31
#define MINWEIGHT -32
typedef struct
{
    int8_t weights[NUMWEIGHT];
} perceptron;
perceptron candidate[NUMTABLE];
uint64_t pgh = 0; //
uint64_t pgmask = ((uint64_t)1<<63)-1;
uint64_t threshold = 20;
uint64_t tablemask = NUMTABLE - 1;

// todo verify the table mask value
uint64_t hash(uint32_t pc)
{
    return pc & tablemask;
}
int64_t p_output(uint32_t pc)
{

    uint64_t tpgh = pgh;
    int64_t res = 0;
    perceptron *sp = &candidate[hash(pc)];

    for (int i = 0; i < NUMWEIGHT; i++)
    {
        if(i== 0){
            res += sp->weights[i];
            
            continue;
        }
        int8_t history_bit = (tpgh & 0x1) << 1;
        res += sp->weights[i] * (history_bit - 1);
        tpgh = tpgh >> 1;
    }
    return res;
}
uint8_t perceptron_predict(uint32_t pc)
{
    int64_t y = p_output(pc);
    if (y > 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void perceptron_train(uint32_t pc, uint8_t outcome)
{

    uint8_t pred = perceptron_predict(pc);
    int64_t y = p_output(pc);
    y = y>0? y:-y;
    if (pred != outcome || y<= threshold)
    {
        uint64_t tpgh = pgh;
        perceptron *sp = &candidate[hash(pc)];
        for (int i = 0; i < NUMWEIGHT; i++)
        {
            if(i==0){
            int8_t tmp = sp->weights[i] + outcome;
            sp->weights[i] = tmp;
            continue;
            }
            int8_t history_bit = (tpgh & 0x1);
            int8_t tmp = sp->weights[i] + (outcome == history_bit ? 1 : -1);
            sp->weights[i] = tmp;
            if (tmp > MAXWEIGHT)
            {
                sp->weights[i] = MAXWEIGHT;
            }
            if (tmp < MINWEIGHT)
            {
                sp->weights[i] = MINWEIGHT;
            }

            tpgh = tpgh >> 1;
        }
    }
    pgh = ((pgh << 1) | outcome) & pgmask;
}
