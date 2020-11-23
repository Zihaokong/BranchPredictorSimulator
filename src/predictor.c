//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include<string.h>
#define MAX 3
#define MIN 0
#define INCREMENT(ct) ({ if((ct)->counter != MAX) (ct)->counter++;})
#define DECREMENT(ct) ({ if((ct)->counter != MIN) (ct)->counter--;})
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
}SatCounter;


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//


SatCounter* BHTGlobal;
uint32_t globalHistory;

SatCounter* BHTLocal;
uint32_t* PHTLocal;

SatCounter* META;





void init_global_predictor(int BHTStartValue, SatCounter** BHT){
  //initialize BHT to be Weakly Not Taken
  globalHistory = 0;
  *BHT = (SatCounter*) malloc((1 << ghistoryBits) * sizeof(SatCounter));
  memset (*BHT,BHTStartValue,(1<<ghistoryBits) * sizeof(SatCounter));
}


uint8_t Gshare_predict(uint32_t pc){
  uint32_t indexedPC = pc % (1<<ghistoryBits);
  uint32_t BHTIndex = indexedPC ^ globalHistory;

  uint8_t counter = BHTGlobal[BHTIndex].counter;
  return (counter & PRED_MASK) >> 1;  
}


void Gshare_train(uint32_t pc, uint8_t outcome){
  uint32_t indexedPC = pc % (1<<ghistoryBits);
  uint32_t BHTIndex = indexedPC ^ globalHistory;
  uint32_t outcome_padd = outcome;

  if(outcome == 1) INCREMENT(BHTGlobal+BHTIndex);     //increment(BHT+BHTIndex,3);
  else DECREMENT(BHTGlobal+BHTIndex);                                         //decrement(BHT+BHTIndex,0); 

  globalHistory = globalHistory<<1;
  globalHistory = globalHistory % (1<<ghistoryBits);
  globalHistory = globalHistory | outcome_padd;

}






void init_tour_predictor(int BHTStartValue, SatCounter** BHTGlobal, SatCounter** BHTLocal, uint32_t** PHT, SatCounter** META ){

  // 2**12 entries of global branch history table
  init_global_predictor(BHTStartValue, BHTGlobal);

  // 2**10 entries of local history table
  *PHT = (uint32_t*) calloc((1<<pcIndexBits),sizeof(uint32_t));
  // 2**10 entries of local branch history table
  *BHTLocal = (SatCounter*)calloc((1<<lhistoryBits), sizeof(SatCounter));
  memset(*BHTLocal, BHTStartValue,(1<<lhistoryBits)*sizeof(SatCounter));

  // 2 ** 12 entries of meta predictor history table.
  *META = (SatCounter*) calloc((1<<ghistoryBits), sizeof(SatCounter));
  memset(*META, BHTStartValue, (1<<ghistoryBits)*sizeof(SatCounter));
}

uint8_t Tour_predict(uint32_t pc){
  uint8_t global_prediction = (BHTGlobal[globalHistory].counter & PRED_MASK) >> 1;

  uint32_t indexedPC = pc % (1<<pcIndexBits);
  uint8_t local_prediction = (BHTLocal[PHTLocal[indexedPC]].counter & PRED_MASK) >> 1;

  uint8_t chooser_prediction = (META[globalHistory].counter & PRED_MASK) >> 1;


  uint8_t result = (chooser_prediction == 0)? global_prediction : local_prediction;
  return result;

}


void Tour_train(uint32_t pc, uint8_t outcome){
  uint8_t global_prediction = (BHTGlobal[globalHistory].counter & PRED_MASK) >> 1;
  uint32_t indexedPC = pc % (1<<pcIndexBits);
  uint8_t local_prediction = (BHTLocal[PHTLocal[indexedPC]].counter & PRED_MASK) >> 1;

  // train meta predictor
  if(global_prediction == outcome && local_prediction != outcome) DECREMENT(META+globalHistory);                                        //decrement(META + globalHistory, 0);
  else if(local_prediction == outcome && global_prediction != outcome) INCREMENT(META+globalHistory);   //increment(META + globalHistory, 3);


  // train global predictor
  uint32_t outcome_padd = outcome;
  if(outcome == 1)  INCREMENT(BHTGlobal+globalHistory);                                                       //increment(BHT+globalHistory,3);
  else DECREMENT(BHTGlobal+globalHistory);                                                                      //decrement(BHT+globalHistory,0); 

  globalHistory = globalHistory<<1;
  globalHistory = globalHistory % (1<<ghistoryBits);
  globalHistory = globalHistory | outcome_padd;

  // train local predictor
  uint32_t patternHistory = PHTLocal[indexedPC];
  if(outcome == 1)  INCREMENT(BHTLocal+patternHistory);                                                  //increment(BHTLocal+patternHistory,3);
  else DECREMENT(BHTLocal+patternHistory);                                   //decrement(BHTLocal+patternHistory,0); 

  patternHistory = patternHistory << 1;
  patternHistory = patternHistory % (1<< lhistoryBits);
  patternHistory = patternHistory | outcome_padd;

  PHTLocal[indexedPC] = patternHistory;
}



// Initialize the predictor
void
init_predictor()
{
  switch(bpType){
    case STATIC:
      return;

    case GSHARE:;
      init_global_predictor(WN, &BHTGlobal);
      return;
    case TOURNAMENT:;
      init_tour_predictor(WN,&BHTGlobal,&BHTLocal,&PHTLocal,&META);
      return;
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
      return Gshare_predict(pc);
    case TOURNAMENT:
      //call predictor method
      return Tour_predict(pc);
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
      Gshare_train(pc,outcome);
      return;
    case TOURNAMENT:;
      Tour_train(pc, outcome);
      return;

    default:
      break;
  }
}


