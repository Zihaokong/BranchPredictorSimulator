//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <math.h> 
#include<string.h>


#define PRED_MASK 2

//
// TODO:Student Information
//
const char *studentName = "zihao Kong";
const char *studentID   = "A15502295";
const char *email       = "zikong@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

typedef struct {
  char counter;
}counter;

void increment(counter* ct, int max){
  if(ct->counter!=max) ct->counter++;
}

void decrement(counter* ct,int min){
  if(ct->counter!=min) ct->counter--;
}





//------------------------------------//
//        Predictor Functions         //
//------------------------------------//


counter* BHT;
uint32_t globalHistory;


counter* BHTLocal;
uint32_t* PHT_Local;


//uint32_t BHTIndex;
uint32_t BHTLocalIndex;



counter* init_global_predictor(int globalHistoryBits, int BHTStartValue){
  //initialize BHT to be 0
  unsigned long int row = pow(2,globalHistoryBits);
  counter* BranchHistoryTable = (counter*) calloc(row,sizeof(counter));
  memset (BranchHistoryTable,BHTStartValue,row);
  return BranchHistoryTable;
}

uint8_t global_predict(int pcIndexBits, counter* BHT, uint32_t pc){
  uint32_t indexedPC = pc % (1<<pcIndexBits);

  uint32_t BHTIndex = indexedPC ^ globalHistory;

  uint8_t counter = BHT[BHTIndex].counter;
  return (counter & PRED_MASK) >> 1;  
  }




void train_global(uint32_t pcIndexBits, uint32_t pc, uint8_t outcome, counter* BHT){
  uint32_t indexedPC = pc % (1<<pcIndexBits);
  uint32_t BHTIndex = indexedPC ^ globalHistory;
  uint32_t outcome_padd = outcome;

  if(outcome == 1) increment(BHT+BHTIndex,3);
  else decrement(BHT+BHTIndex,0); 



  globalHistory = globalHistory<<1;
  globalHistory = globalHistory % (1<<pcIndexBits);
  globalHistory = globalHistory | outcome_padd;


}



//void global_train




counter* init_local_predictor(int pcIndexBits, int lhistoryBits, int BHTStartValue, uint32_t ** PatternHistorytable){
  int row_PHT = pow(2,pcIndexBits);
  *PatternHistorytable = (uint32_t*) calloc(row_PHT,sizeof(uint32_t));

  int row_BHT = pow(2,lhistoryBits);
  counter* BranchHistoryTable = (counter*) calloc(row_BHT,sizeof(counter));
  memset (BranchHistoryTable,BHTStartValue,row_BHT);

  return BranchHistoryTable;
}









uint8_t local_predict(int pcIndexBits, uint32_t* PHT_Local, counter* BHTLocal, uint32_t pc){
  uint32_t indexedPC = pc % (1<<pcIndexBits);
  uint8_t counter = BHTLocal[PHT_Local[indexedPC]].counter;

  return (counter & PRED_MASK) >> 1;

}

// Initialize the predictor
//
void
init_predictor()
{
  switch(bpType){
    case STATIC:
      return;

    case GSHARE:;
      BHT = init_global_predictor(ghistoryBits,2);
      return;
    case TOURNAMENT:;
      BHT = init_global_predictor(ghistoryBits,2);
      BHTLocal = init_local_predictor(pcIndexBits, lhistoryBits,2,&PHT_Local);

      //meta predictor
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//


uint8_t make_prediction(uint32_t pc)
{
  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      //call predictor method 
      return global_predict(ghistoryBits,BHT,pc);
    case TOURNAMENT:;
      //call predictor method
      uint8_t result;

      uint8_t lResult = local_predict(pcIndexBits,PHT_Local,BHTLocal,pc);
      uint8_t gResult = global_predict(pcIndexBits,BHT,pc);

      return result;
    case CUSTOM:
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
void
train_predictor(uint32_t pc, uint8_t outcome)
{

  switch(bpType){
    case STATIC:
      return;
    case GSHARE:;
      train_global(ghistoryBits,pc,outcome,BHT);
    
    default:
      break;
  }

}


