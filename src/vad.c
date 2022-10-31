#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "vad.h"
#include "pav_analysis.h"


const float FRAME_TIME = 10.0F; /* in ms. */

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT", "MV", "MS"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  feat.p = compute_power(x,N);
  feat.zcr = compute_zcr(x,N,16000);
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate, float alfa1, float alfa2, float alfa3, float alfa4) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->alfa1=alfa1;
  vad_data->alfa2=alfa2;
  vad_data->alfa3=alfa3;//longitud frame
  vad_data->alfa4=alfa4;
  //vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  vad_data->frame_length = rate * alfa3 * 1e-3;
  vad_data->frame_length_init=vad_data->frame_length;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
  /* 
   * TODO: decide what to do with the last undecided frames
   */
  VAD_STATE state = vad_data->stateAnterior;
  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x) {

  /* 
   * TODO: You can change this, using your own features,
   * program finite state automaton, define conditions, etc.
   */

  Features f = compute_features(x, vad_data->frame_length);
  float longitudTrama;
  int contador=0;

  vad_data->last_feature = f.p; /* save feature, in case you want to show */
  switch (vad_data->state) {
  case ST_INIT:
    //printf("init \n");
    vad_data->state = ST_SILENCE;
    vad_data->stateAnterior= ST_SILENCE;
    vad_data->umbral3 = vad_data->alfa3; //limit de intents a Maybe
    vad_data->umbral2 = f.p + vad_data->alfa2; //alfa2=2 ()
    vad_data->umbral = f.p + vad_data->alfa1; //alfa1=5.1 (89.293%)
    vad_data->um1ini=vad_data->umbral;
    vad_data->um2ini=vad_data->umbral2;
    //printf("%f --- %f\n",um1ini,um2ini);
    
    break;
  case ST_SILENCE:
  contador=0;
  vad_data->frame_length=vad_data->frame_length_init;
    if (f.p > vad_data->umbral2)
      vad_data->state = ST_MAYBEVOICE;
    break;
  case ST_MAYBEVOICE:
    if (f.p < vad_data->umbral2){
      vad_data->state = ST_SILENCE;
      vad_data->stateAnterior = ST_SILENCE;
    }else if (f.p > vad_data->umbral){
      vad_data->state = ST_VOICE;
      vad_data->stateAnterior = ST_VOICE;
    }else{
      contador++;
      vad_data->frame_length=vad_data->frame_length/(contador*0.5);
    }
    break;
  case ST_MAYBESILENCE:
    if (f.p < vad_data->umbral2){ 
      vad_data->state = ST_SILENCE;
      vad_data->stateAnterior = ST_SILENCE;
    }else if (f.p > vad_data->umbral){
      vad_data->state = ST_VOICE;
      vad_data->stateAnterior = ST_VOICE;
    }else{
      contador++;
      vad_data->frame_length=vad_data->frame_length/(contador*0.5);
    }
    break;
  case ST_VOICE:
    vad_data->frame_length=vad_data->frame_length_init;
    contador=0;
    if (f.p < vad_data->umbral)
      vad_data->state = ST_MAYBESILENCE;
    break;
  case ST_UNDEF:
    break;
  }

  if(f.zcr>vad_data->alfa4 && f.p<vad_data->umbral){
    vad_data->umbral2 = vad_data->um2ini+1;
  }else{
    vad_data->umbral2 = vad_data->um2ini;
  }
  if(vad_data->stateAnterior == ST_SILENCE || vad_data->stateAnterior == ST_VOICE){
    return vad_data->stateAnterior;
  } else{
    //printf("stateAnteriorError\n");
    return ST_UNDEF;
  }
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->stateAnterior, vad_data->last_feature);
}
