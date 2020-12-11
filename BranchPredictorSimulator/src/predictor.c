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
#include<math.h>

#define MAX 3
#define MIN 0
#define INCREMENT(ct) ({ if((ct)->counter != MAX) (ct)->counter++;})
#define DECREMENT(ct) ({ if((ct)->counter != MIN) (ct)->counter--;})
#define ABS(X) ((X)>=0? (X) : -1*(X))
#define PRED_MASK 2



#define WIDTH 16
#define HEIGHT 146
#define LENGTH 7



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

// 2 bit sarturating counter struct
typedef struct {
  char counter;
}SatCounter;


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

/* branch history table for gshare and 
global predictor in tournament  */
SatCounter* BHTGlobal;
uint32_t globalHistory;



/* local branch pattern history table 
and associate branch history table for 
pattern history. */
uint32_t* PHTLocal;
SatCounter* BHTLocal;

/* meta predictor in tournament predictor*/
SatCounter* META;





void init_global_predictor(int BHTStartValue, SatCounter** BHT){
  //initialize BHT to be Weakly Not Taken
  globalHistory = 0;
  *BHT = (SatCounter*) malloc((1 << ghistoryBits) * sizeof(SatCounter));
  memset (*BHT,BHTStartValue,(1<<ghistoryBits) * sizeof(SatCounter));
}


uint8_t Gshare_predict(uint32_t pc){
  // calculate index into BHT global using XOR
  uint32_t indexedPC = pc % (1<<ghistoryBits);
  uint32_t BHTIndex = indexedPC ^ globalHistory;

  // get prediction bit
  uint8_t counter = BHTGlobal[BHTIndex].counter;
  return (counter & PRED_MASK) >> 1;  
}


void Gshare_train(uint32_t pc, uint8_t outcome){
  //calculating index into BHT global using XOR
  uint32_t indexedPC = pc % (1<<ghistoryBits);
  uint32_t BHTIndex = indexedPC ^ globalHistory;
  uint32_t outcome_padd = outcome;

  // change saturating counter based on branch outcome
  if(outcome == 1) INCREMENT(BHTGlobal+BHTIndex);  
  else DECREMENT(BHTGlobal+BHTIndex);       

  //leftshift glbal history for new branch outcome.
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
  // get global prediction with BHTGlobal
  uint8_t global_prediction = (BHTGlobal[globalHistory].counter & PRED_MASK) >> 1;

  // calculating index into local pattern history table and get local branch prediction rsult
  uint32_t indexedPC = pc % (1<<pcIndexBits);
  uint8_t local_prediction = (BHTLocal[PHTLocal[indexedPC]].counter & PRED_MASK) >> 1;

  //get meta predictor result of which result to use.
  uint8_t chooser_prediction = (META[globalHistory].counter & PRED_MASK) >> 1;

  // decide which branch to take.
  uint8_t result = (chooser_prediction == 0)? global_prediction : local_prediction;
  return result;

}


void Tour_train(uint32_t pc, uint8_t outcome){
  // get global, local prediction from BHT
  uint8_t global_prediction = (BHTGlobal[globalHistory].counter & PRED_MASK) >> 1;
  uint32_t indexedPC = pc % (1<<pcIndexBits);
  uint8_t local_prediction = (BHTLocal[PHTLocal[indexedPC]].counter & PRED_MASK) >> 1;

  // train meta predictor
  if(global_prediction == outcome && local_prediction != outcome) DECREMENT(META+globalHistory);                    
  else if(local_prediction == outcome && global_prediction != outcome) INCREMENT(META+globalHistory);   


  // increase BHT in global predictor, and update global history register
  uint32_t outcome_padd = outcome;
  if(outcome == 1)  INCREMENT(BHTGlobal+globalHistory);                                      
  else DECREMENT(BHTGlobal+globalHistory);                                                       

  globalHistory = globalHistory<<1;
  globalHistory = globalHistory % (1<<ghistoryBits);
  globalHistory = globalHistory | outcome_padd;

  // update branch history table in local predictor
  uint32_t patternHistory = PHTLocal[indexedPC];
  if(outcome == 1)  INCREMENT(BHTLocal+patternHistory);                                  
  else DECREMENT(BHTLocal+patternHistory);                                   

  // update pattern history table with new branch outcome
  patternHistory = patternHistory << 1;
  patternHistory = patternHistory % (1<< lhistoryBits);
  patternHistory = patternHistory | outcome_padd;
  PHTLocal[indexedPC] = patternHistory;
}

//threshold
int8_t theta;
//history register with the same number of weights for each perceptron.
int8_t customHistory[WIDTH];
//table of perceptrons with bias
int16_t weights[HEIGHT][WIDTH+1];


void init_perceptron_predictor(){
  // formula from https://www.cs.utexas.edu/~lin/papers/hpca01.pdf
  theta = (int8_t)(1.93 * WIDTH + 14);
  // initialize bias and weights for each perceptron in table.
  for(int i = 0;i<HEIGHT;i++){
    //bias = 1;
    weights[i][0] = 1;
    for(int j = 1;j<WIDTH+1;j++){
      weights[i][j] = 0;
    }
  }
  // initialize history register with NOTTAKEN
  memset(customHistory,0,WIDTH*sizeof(int16_t));
}


int16_t Custom_predict(uint32_t pc){

  //calculatiing index and get bias from perceptron table
  uint32_t indexedPC = pc % HEIGHT;
  int16_t y = weights[indexedPC][0];
  // return the result of dot product
  for(int i = 1;i<WIDTH+1;i++){
    y += customHistory[i-1] ? weights[indexedPC][i] : (-1)*weights[indexedPC][i];
  }
  return y;
}



void shiftHistory(char outcome){
  //shift in the outcome to LSB of history register 
  for(int i = 1; i < WIDTH; i ++){
    customHistory[i-1] = customHistory[i];
  }
  customHistory[WIDTH-1] = outcome;
}



void Custom_train(uint32_t pc, uint8_t outcome){
  //calculating index 
  uint32_t indexedPC = pc % HEIGHT;

  //calculating branch result
  int16_t y = Custom_predict(pc);
  int result;
  if(y>=0) result = TAKEN;
  else result = NOTTAKEN; 

  // if result is different from label, or result is less than threshold, train the weights.
  if((result != outcome) || ABS(y) <= theta ){
    //train bias
    weights[indexedPC][0] += ((outcome == 1)? 1 : -1) * 1;
    //train weights
    for(int i = 1; i <= WIDTH;i++){
      //wi = wi + t*x
      int16_t predict = (customHistory[i-1] == 0)? -1:1;
      int16_t outcome_signed = (outcome == 0)? -1 : 1;
      if(weights[indexedPC][i] != ((1<<(LENGTH-1))-1) && weights[indexedPC][i] != -(1 <<(LENGTH-1)))
        weights[indexedPC][i] += outcome_signed*predict;

    }
  }
  //shift left with LSB the new outcome
  shiftHistory(outcome);
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
    case CUSTOM:;
      init_perceptron_predictor();
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
    case CUSTOM:;
      int16_t y = Custom_predict(pc);
      return (y>=0) ? TAKEN : NOTTAKEN;
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
    case CUSTOM:
      Custom_train(pc,outcome);
    default:
      break;
  }
}


