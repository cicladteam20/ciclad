//
//  main.cpp
//  newCiclad
//
//  Created by Mickael Wajnberg on 18-06-22.
//  Copyright © 2018 Mickael Wajnberg. All rights reserved.
//

#include <cstdint>
//#include <cinttypes>
#include <cstdlib>
#include <iostream>
#include <vector> 
#include <queue>
#include <time.h>
#include "transaction.h"
#include "concept.h"
#include "trienode.h"
#include "CicladAdd.hpp"
#include "CicladRmv.hpp"

#include <fstream>
#include <sstream>

#ifdef _WIN32
  #include <windows.h>
  #include <psapi.h>
#endif

#define WINDOW_SIZE 1000
#define MAX_ITEM 10001
//#define SHOW_CI

using namespace std;


int readfile(char *fn, std::queue<TRANSACTION> &T) {
  //FILE *f;
  //f = fopen(fn, "r");
  //const uint32_t LINE_SIZE_MAX = 1000;
  //const uint32_t NEXT_CHUNK_SIZE = 1000;
  //char s[NEXT_CHUNK_SIZE];

  //std::queue<char*> chunks = std::queue<char*>();
  ifstream inFile(fn);
  if (!inFile) {
    cerr << "cannot open INPUT file!" << endl;
    //outFile.close();
    exit(1);
  }

  while (!inFile.eof()) {
    vector<uint32_t>* items = new vector<uint32_t>();
    items->push_back(0);
    string line;
    string token;
    std::getline(inFile, line);
    istringstream iss(line);
    while (getline(iss, token, ' ')) {
      items->push_back(stol(token));
    }
    

    TRANSACTION trx;
    trx.itemset = &(*(items->begin()));
    *(trx.itemset) = items->size() - 1;
    T.push(trx);
  }

  //


  //while (fgets(s, NEXT_CHUNK_SIZE, f) != NULL) {



  //  //cout << "asldjasld" << endl;
  //  uint32_t* itemset = (uint32_t*)malloc(sizeof(uint32_t) * LINE_SIZE_MAX);
  //  char* pch = strtok(s, " ");
  //  char* newpch = 0;
  //  uint32_t curr = 1;

  //  bool has_next = pch != 0;
  //  while (has_next) {
  //    if (curr > LINE_SIZE_MAX) {
  //      std::cout << "la ligne est trop grande... (pour l insant on gere pas ca) " << std::endl;
  //      exit(1);
  //    }
  //    uint32_t item = atol(pch);
  //    itemset[curr] = item;
  //    curr++;
  //    newpch = strtok(0, " ");
  //    if (newpch != 0) {
  //      //on a trouve un prochain token
  //      //sinon pas de prochain token
  //      //deux cas, soit finit par \0, soit non
  //      has_next = true;
  //      pch = newpch;
  //    }
  //    else {
  //      
  //    }
  //  }
    //

    /*uint32_t* fit_itemset = (uint32_t*)realloc(itemset, sizeof(uint32_t) * curr);
    if(!fit_itemset){
    cout << "probleme lors di decoupage de l itemset" << endl;
    free (itemset);
    exit (1);
    }
    itemset = fit_itemset;*/

   //itemset[0] = curr - 1;

    //TRANSACTION trx;
    //trx.itemset = itemset;
    //T.push(trx);
  //}
  //fclose(f);
  //delete f;
  return 0;
}

int findBottom(std::queue<TRANSACTION> T){return 0;};


size_t numberOfCI = 1;
int main(int argc, char * argv[]) {
    // insert code here...
    //char* path = "/Users/mickaelwajnberg/Documents/workspace/ciclad_Java/ciclad_java/testdb_gen2.txt";
    //pour chaque transaction le premier element est la taille de la transaction
    //TODO : faire une struct a la place
    std::queue<TRANSACTION> transactionStream,window;
    readfile(argv[1], transactionStream);
    

    //item du "bottom"
    const size_t maxItem = MAX_ITEM;//findBottom(transactionStream);
    const size_t windowSize = WINDOW_SIZE;
    vector<set<uint32_t>*> generatorContainer;
    //TODO : remplir concept proprement (dynamique)cf maxItem
    concept bottomConcept;
    bottomConcept.id=0;
    bottomConcept.lastitem=0;
    bottomConcept.size=maxItem;
    bottomConcept.support=0;
#ifdef CALC_GEN
    bottomConcept.generators=new vector<set<uint32_t>*>();
#endif
#ifdef STORE_ITEMSET
    bottomConcept.itemset=new vector<uint32_t>();
#endif

#ifdef CALC_GEN
    bottomConcept.genetorPositionReferences = new vector<vector<size_t>>();
#ifdef STRATIFIED_NDX
    bottomConcept.localGenInvertedIndex = new std::map<uint32_t, std::map<size_t, vector<size_t>*>*>();
#else
    bottomConcept.localGenInvertedIndex = new std::map<uint32_t, vector<size_t>*>();
#endif
    bottomConcept.actualGenerators = 0;
#endif
    //bottomConcept.generators->push_back(set<uint32_t>());
    //bottomConcept.localGenInvertedIndex->resize(MAX_ITEM);

#ifdef REUSE_OBSOLETE
    available_positions_for_new_cis = (std::queue<size_t>**)malloc(MAX_ITEM * sizeof(std::queue<size_t>*));
#endif

    for( uint32_t i=0 ; i< maxItem; ++i){
#ifdef CALC_GEN
        set<uint32_t>* const gen = new set<uint32_t>();
        gen->insert(i);
        add_gen_to_ci(&bottomConcept, gen);
        generatorContainer.push_back(gen);
        //bottomConcept.generators->push_back(gen);
#endif
#ifdef STORE_ITEMSET
        bottomConcept.itemset->push_back(i);
#endif
#ifdef REUSE_OBSOLETE
        available_positions_for_new_cis[i] = (std::queue<size_t>*)new std::queue<size_t>();
#endif
    }
    //bottomConcept.itemset=&bottomConcept.generators->at(0);
    //cout << available_positions_for_new_cis[0] << endl;
    //cout << available_positions_for_new_cis[41] << endl;


    vector<concept*> conceptContainer;
    
    conceptContainer.push_back(&bottomConcept);
    
    //index[3][8] -> renvoie le 8e CI pointer pour le 3e item
    vector<vector<concept*>>index;
    //init de l'index
    {
        for(size_t i=0; i<maxItem;++i){
            vector<concept*> tmp;
            tmp.push_back(conceptContainer[0]);
            index.push_back(tmp);
        }
    }

    clock_t begin;
    clock_t end;

    begin = clock();

    //int trxid = 0;
    //add only
    int tran = 0;
    //cout << "start" << endl;
    for(size_t tID = 0; tID < windowSize; ++tID){
        TRANSACTION current = transactionStream.front();
        window.push(current);

        /*printf("{");
        for (int ii = 1; ii != current.itemset[0]; ++ii) {
          printf("%d,", current.itemset[ii]);
        }
        printf("}\n");
        */
        //cout << "transaction number : "<<tran++<<endl;
        updateCicladAdd(&current, &index, &conceptContainer, &generatorContainer);
        transactionStream.pop();
        trxid += 1;
        //std::cout << conceptContainer.size() << " " << numberOfCI << endl;
        /*if (trxid > 5) {
          exit(2);
        }*/
    }
    std::cout << conceptContainer.size() << " " << numberOfCI << endl;

    while (!transactionStream.empty()) {
        //if not last in stream
        if (0 != windowSize) {
          //if (!window.empty()) {
          updateCicladRmv(&window.front(), &index, &conceptContainer);
          //cout << "transaction number : "<<tran++<<endl;
#ifdef STORE_ITEMSET
          free((&window.front())->itemset);
#endif
          window.pop();
          window.push(transactionStream.front());
          //}
        }
        updateCicladAdd(&transactionStream.front(), &index, &conceptContainer, &generatorContainer);
        // window.push(transactionStream.front());
        
        transactionStream.pop();
        if (trxid % 1000 == 0) {
          std::cout << "trx " << trxid << std::endl;
        }
        /*if (trxid == 250) {
          break;
        }*/
        trxid += 1;
    }
    //nettoyage
    while (!window.empty()) {
      free((&window.front())->itemset);
      window.pop();
    }
    
    end = clock();
    std::printf("processed transactions in %0.4f ms\n", (end - begin) / (double)CLOCKS_PER_SEC * 1000);
    std::cout << " number of ci : " << numberOfCI << "(" << conceptContainer.size() << ")" <<flush ;

#ifdef SHOW_CI
    uint32_t checkOfSize = 0;
#ifdef CALC_GEN
    uint32_t totalGenerators = 0;
    float avg_GeneratorsByCI = 0;
#endif
    for (auto ci : conceptContainer) {
      if (ci && ci->id) {
        /*for (auto gen : *(ci->generators)) {
          if (gen) {
            totalGenerators += 1;
          }
        }*/

        {
          printf(" CI #%d{", ci->id);
          vector<uint32_t>::iterator newciit = ci->itemset->begin();
          for (; newciit != ci->itemset->end(); ++newciit) {
          printf("%d,", *newciit);
          }
          printf("} -> ");
        }
#ifdef CALC_GEN
        vector<set<uint32_t>*>::iterator genit = ci->generators->begin();
        for (; genit != ci->generators->end(); ++genit) {
          set<uint32_t>* const ref = *genit;
          if (ref) {
            printf("{");
            set<uint32_t>::iterator newciit = ref->begin();
            for (; newciit != ref->end(); ++newciit) {
            printf("%d,", *newciit);
            }
            printf("}, ");
          }
        }
        printf("\n");

        if (ci->actualGenerators == 0) {
          cout << "" << endl;
        }
        totalGenerators += ci->actualGenerators;
#endif
        checkOfSize += 1;
      }
    }
    if (checkOfSize != numberOfCI) {
      std::printf("bad integrity, number of CI do not match : %d vs %d \n", checkOfSize, numberOfCI);
    }
#ifdef CALC_GEN
    avg_GeneratorsByCI = totalGenerators / (float)checkOfSize;
    std::printf("avg generators by ci : %0.2f (%d)", avg_GeneratorsByCI, totalGenerators);
#endif
#endif
    for (concept* ci : conceptContainer) {
      if (ci) {
#ifdef STORE_ITEMSET
        delete ci->itemset;
#endif
        //free(ci->positionsInIndex);
        //free(ci);
      }
    }

#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX info;
    GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&info, sizeof(info));
    cout << "WorkingSet " << info.WorkingSetSize / 1024 << "K, PeakWorkingSet " << info.PeakWorkingSetSize / 1024 << "K, PrivateSet " << info.PrivateUsage / 1024 << "K" << endl;
#endif

    return 0;
}
