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

//tournament choice prediction table
#define SL  0			// predict NT, strong not taken
#define WL  1			// predict NT, weak not taken
#define WG  2			// predict T, weak taken
#define SG  3			// predict T, strong taken
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


typedef struct{
  uint8_t counter;
  uint8_t tag;
  uint8_t useful;
} bank_entry; 
typedef struct{
  uint8_t counter;
  uint8_t m; 
} bimodal;
// custom taged predictors
typedef unsigned __int128 uint128_t;
uint128_t cgmask = (((uint128_t)1) << 80)- 1;
uint64_t  cimask = (1 << 10) - 1;
uint64_t  cpcmask = (1 << 10)  -1; 
uint64_t ctmask = (1<< 8) -1;
uint64_t cbimodalmask = (1<<11) -1;    
uint128_t cglobal_history = 0; 
bimodal bank0[2048] = {0};
bank_entry bank1[512] = {0};
bank_entry bank2[512] = {0};
bank_entry bank3[512] = {0};
bank_entry bank4[512] = {0};

uint8_t compute_tag(uint64_t bankn, uint64_t pc){
  uint128_t tgh = cglobal_history;
  uint8_t res = pc & ctmask;
  uint8_t csr1 = 0;
  uint8_t csr2= 0;
  for(int i = 0; i < bankn; i++){
  csr1 = tgh & 0xff;
  tgh >>= 8;
  csr2 = tgh & 0xff;
  csr2<<= 1;
  csr2 |= tgh &0x1;
  tgh >>= 8;
  res = res ^ csr1 ^csr2;
  }
  return res; 
}
uint64_t mypow(uint64_t x, uint64_t n){
  uint64_t res = 1;
  while( n >= 1){
    res = x*res;
    n--;
  }
  return res; 
}
uint64_t compute_index(uint64_t bankn, uint64_t pc){
  uint64_t res = pc & cpcmask;
  pc = pc >> 10; 
  res = (res ^ (pc &cpcmask));
  uint64_t his_len = mypow(2,bankn - 1) * 10 - 1;
  uint64_t tgh = cglobal_history; 
  for(int i = his_len; i >= 0; i-= 10){
    res = res ^ (tgh & cimask);
    tgh = tgh >>10;
  }
  return res;
}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

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
void init_custom(){
  // 
}
uint8_t
gshare_predict(uint32_t pc)
{
    //printf("predictor.c 80");
  //fflush(stdout); 
  // get lower ghistoryBits of pc
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
    //printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

uint8_t
tournament_predict(uint32_t pc)
{
      //printf("predictor.c 106\n");
  //fflush(stdout); 
  // get lower ghistoryBits of pc
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
uint64_t compute_bimodel_index(uint32_t pc){
  return pc & cbimodalmask; 
}
uint8_t
custom_predict(uint32_t pc){

bimodal entry0 = bank0[compute_bimodel_index(pc)];
bank_entry entry1 = bank1[compute_index(1,pc)];
bank_entry entry2 = bank2[compute_index(2,pc)];
bank_entry entry3 = bank3[compute_index(3,pc)];
bank_entry entry4 = bank4[compute_index(4,pc)];

  

uint8_t s_tag1 = entry1.tag;
uint8_t s_tag2 = entry2.tag;
uint8_t s_tag3 = entry3.tag;
uint8_t s_tag4 = entry4.tag;

uint8_t c_tag1 = compute_tag(1,pc); 
uint8_t c_tag2 = compute_tag(2,pc);
uint8_t c_tag3 = compute_tag(3,pc);
uint8_t c_tag4 = compute_tag(4,pc);
if(c_tag4 == s_tag4){
  return entry4.counter& 0xf;
}
if(c_tag3 == s_tag3){
    return entry3.counter& 0xf;

}
if(c_tag2 == s_tag2){
    return entry2.counter& 0xf;

}
if(c_tag1 == s_tag1){
    return entry1.counter& 0xf;

}

return entry0.counter & 0xf; 
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
  //printf("predictor.c 155\n");
  //fflush(stdout); 
  // get lower ghistoryBits of pc
  uint32_t pc_lower_bits = pc & lmask;
  //printf("predictor.c 159 %llu\n ",path);

  path = path & gmask;

  // lresult
  uint32_t gresult = global_prediction[path];
  //printf("predictor.c 163 %llu\n ",path);
  //fflush(stdout); 
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
  //printf("predictor.c 180 %llu \n",local_history_table[pc_lower_bits]);
  //fflush(stdout); 

  uint32_t lresult = local_prediction[local_history_table[pc_lower_bits]];

  //printf("predictor.c 188\n");
  //fflush(stdout); 
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
    //fflush(stdout); 

  }
  //printf("predictor.c 201\n");
  //fflush(stdout); 

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
    //printf("predictor.c 244\n");
  //fflush(stdout); 

  // gresult

  // update local hissotry table
  //  update local predictions
  path = ((path <<1) | outcome) & gmask;
  local_history_table[pc_lower_bits] = ((local_history_table[pc_lower_bits] <<1) | outcome) & lmask;
}



void train_custom(uint32_t pc, uint8_t outcome){



}
uint64_t useful_reset_counter = 0; 
#define RESET_PERIOD 262144
void reset_useful_counter(){
useful_reset_counter ++;
  if(useful_reset_counter == RESET_PERIOD){
for(int i = 0; i < 512; i ++){
    bank1[i].useful = 0;
    bank2[i].useful = 0;
    bank3[i].useful = 0;
    bank4[i].useful = 0;
  }
  useful_reset_counter = 0;
  }
  
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
    // return tournament_predict(pc);
    return TAKEN;
  case CUSTOM:
  // return custom_predict(pc);
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
  // return train_custom(pc, outcome);
  default:
    break;
  }
}
