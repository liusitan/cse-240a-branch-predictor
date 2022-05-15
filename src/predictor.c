//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
// #include <math.h>x/x.
#include "predictor.h"
// #include <stdlib.h>
// tournament choice prediction table
#define SL 0 // predict NT, strong not taken
#define WL 1 // predict NT, weak not taken
#define WG 2 // predict T, weak taken
#define SG 3 // predict T, strong taken
//
// TODO:Student Information
//
const char *studentName = "Sitan Liu";
const char *studentID = "A53306512";
const char *email = "sil017@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 14; // Number of bits used for Global History
int bpType;            // Branch Prediction Type
int verbose;
// tournament predictor

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// tournament
uint64_t zero = 0;
uint64_t local_history_table[2048] = {0};
uint64_t local_prediction[2048] = {WN};
uint64_t global_prediction[4096] = {WN};
uint64_t path = 0;
uint64_t choice_prediction[4096] = {WL};
uint64_t gmask = 0xfff;
uint64_t lmask = 0xfff >> 1;


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//











#define NUMWEIGHT 93
#define NUMTABLE 59
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
    return pc %tablemask;
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


// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}
void init_tournament()
{
  //   for(int i = 0; i < 4096; i++){
  // choice_prediction[i] = WL;
  // gloabl_prediction[i] = WN;
  //   }
}
void init_custom()
{

    }
uint8_t
gshare_predict(uint32_t pc)
{
  // printf("predictor.c 80");
  // fflush(stdout);
  //  get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    // printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

uint8_t
tournament_predict(uint32_t pc)
{
  // printf("predictor.c 106\n");
  // fflush(stdout);
  //  get lower ghistoryBits of pc
  uint32_t pc_lower_bits = pc & lmask;
  path = path & gmask;
  int lhis = 0;
  switch (choice_prediction[path] >> 1)
  {
  case 1:
    return global_prediction[path] >> 1;
  case 0:
    lhis = local_history_table[pc_lower_bits];
    return local_prediction[lhis] >> 1;
  default:
    exit(1);
  }
  return 0;
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  // printf("predictor.c 155\n");
  // fflush(stdout);
  //  get lower ghistoryBits of pc
  uint32_t pc_lower_bits = pc & lmask;
  // printf("predictor.c 159 %llu\n ",path);

  path = path & gmask;

  // lresult
  uint32_t gresult = global_prediction[path];
  // printf("predictor.c 163 %llu\n ",path);
  // fflush(stdout);
  switch (gresult)
  {
  case WN:
    global_prediction[path] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    global_prediction[path] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    global_prediction[path] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    global_prediction[path] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in gresult !\n");
  }
  // printf("predictor.c 180 %llu \n",local_history_table[pc_lower_bits]);
  // fflush(stdout);

  uint32_t lresult = local_prediction[local_history_table[pc_lower_bits]];

  // printf("predictor.c 188\n");
  // fflush(stdout);
  switch (lresult)
  {
  case WN:
    local_prediction[local_history_table[pc_lower_bits]] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    local_prediction[local_history_table[pc_lower_bits]] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    local_prediction[local_history_table[pc_lower_bits]] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    local_prediction[local_history_table[pc_lower_bits]] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in lresult !\n");
    // fflush(stdout);
  }
  // printf("predictor.c 201\n");
  // fflush(stdout);

  lresult = lresult >> 1;
  gresult = gresult >> 1;
  uint32_t choice_outcome = choice_prediction[path];
  if (lresult != gresult)
  {

    switch (choice_outcome)
    {
    case WL:
      choice_prediction[path] = (choice_outcome == 1) ? WG : SL;
      break;
    case SL:
      choice_prediction[path] = (choice_outcome == 1) ? WL : SL;
      break;
    case WG:
      choice_prediction[path] = (choice_outcome == 1) ? SG : WL;
      break;
    case SG:
      choice_prediction[path] = (choice_outcome == 1) ? SG : WG;
      break;
    }
  }

  // else
  // {
  //    switch (choice_outcome)
  //   {
  //   case WL:
  //      choice_prediction[path] =  SL;
  //     break;
  //   case SL:
  //      choice_prediction[path] =  SL;
  //     break;
  //   case WG:
  //      choice_prediction[path] = WL;
  //     break;
  //   case SG:
  //      choice_prediction[path] =  WG;
  //     break;
  // }
  // }
  // printf("predictor.c 244\n");
  // fflush(stdout);

  // gresult

  // update local hissotry table
  //  update local predictions
  path = ((path << 1) | outcome) & gmask;
  local_history_table[pc_lower_bits] = ((local_history_table[pc_lower_bits] << 1) | outcome) & lmask;
}


void cleanup_gshare()
{
  free(bht_gshare);
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
  case CUSTOM:
    init_custom();
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
    // return TAKEN;
  case CUSTOM:
  return perceptron_predict(pc);
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void train_predictor(uint32_t pc, uint8_t outcome)
{

  switch (bpType)
  {
  case STATIC:
  case GSHARE:
    return train_gshare(pc, outcome);
  case TOURNAMENT:
    return train_tournament(pc, outcome);
  case CUSTOM:
  return perceptron_train(pc, outcome);
  default:
    break;
  }
}
