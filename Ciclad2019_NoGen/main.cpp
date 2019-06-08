//
//  Copyright © 2018 [CICLAD TEAM]
//  
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.If not, see <http://www.gnu.org/licenses/>.
//

#include <cstdint>
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

//#define SHOW_CI

using namespace std;


int readfile(char *fn, std::queue<TRANSACTION> &T) {
  ifstream inFile(fn);
  if (!inFile) {
    cerr << "cannot open INPUT file!" << endl;
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
  return 0;
}

uint32_t __maxItem;

size_t numberOfCI = 1;
int main(int argc, char * argv[]) {
  std::queue<TRANSACTION> transactionStream, window;
  readfile(argv[1], transactionStream);

  const uint32_t nbr_items = strtoul(argv[2], 0, 10);
  //Attention : il faudrait aussi chercher le errno
  if (nbr_items == 0 || nbr_items == ULONG_MAX || nbr_items == ULLONG_MAX) {
    std::cout << "Cannot read number of items " << std::endl;
    exit(1);
  }
  const uint32_t window_size = strtoul(argv[3], 0, 10);
  //Attention : il faudrait aussi chercher le errno
  /*if (window_size == 0 || window_size == ULONG_MAX || window_size == ULLONG_MAX) {
    std::cout << "Cannot read window size " << std::endl;
    exit(1);
  }*/

  //item du "bottom"
  const size_t maxItem = nbr_items;// MAX_ITEM;
  std::cout << "nbr items " << maxItem << ", window size "<< window_size << std::endl;
  const size_t windowSize = window_size;// WINDOW_SIZE;
  vector<set<uint32_t>*> generatorContainer;
  concept bottomConcept;
  bottomConcept.id = 0;
  bottomConcept.lastitem = 0;
  bottomConcept.size = maxItem;
  bottomConcept.support = 0;
#ifdef STORE_ITEMSET
  bottomConcept.itemset = new vector<uint32_t>();
#endif

#ifdef REUSE_OBSOLETE
  available_positions_for_new_cis = (std::queue<size_t>**)malloc(maxItem * sizeof(std::queue<size_t>*));
#endif

  for (uint32_t i = 0; i < maxItem; ++i) {
#ifdef STORE_ITEMSET
    bottomConcept.itemset->push_back(i);
#endif
#ifdef REUSE_OBSOLETE
    available_positions_for_new_cis[i] = (std::queue<size_t>*)new std::queue<size_t>();
#endif
  }

  vector<concept*> conceptContainer;
  conceptContainer.push_back(&bottomConcept);
  vector<vector<concept*>>index;
  //init de l'index
  {
    for (size_t i = 0; i < maxItem; ++i) {
      vector<concept*> tmp;
      tmp.push_back(conceptContainer[0]);
      index.push_back(tmp);
    }
  }

  clock_t begin;
  clock_t end;

  begin = clock();

  int tran = 0;
  //cout << "start" << endl;
  for (size_t tID = 0; tID < windowSize; ++tID) {
    TRANSACTION current = transactionStream.front();
    window.push(current);
    updateCicladAdd(&current, &index, &conceptContainer, &generatorContainer);
    transactionStream.pop();
    trxid += 1;
  }
  std::cout << conceptContainer.size() << " " << numberOfCI << endl;

  while (!transactionStream.empty()) {
    //if not last in stream
    if (0 != windowSize) {
      updateCicladRmv(&window.front(), &index, &conceptContainer);
#ifdef STORE_ITEMSET
      free((&window.front())->itemset);
#endif
      window.pop();
      window.push(transactionStream.front());
    }
    updateCicladAdd(&transactionStream.front(), &index, &conceptContainer, &generatorContainer);

    transactionStream.pop();
    //TODO: this should be an argument
    if (trxid % 1000 == 0) {
      std::cout << "trx " << trxid << std::endl;
    }
    trxid += 1;
  }
  //cleaning
  while (!window.empty()) {
    free((&window.front())->itemset);
    window.pop();
  }

  end = clock();
  std::printf("processed transactions in %0.4f ms\n", (end - begin) / (double)CLOCKS_PER_SEC * 1000);
  std::cout << " number of ci : " << numberOfCI << "(" << conceptContainer.size() << ")" << flush;

#ifdef SHOW_CI
  uint32_t checkOfSize = 0;
  for (auto ci : conceptContainer) {
    if (ci && ci->id) {
      {
        printf(" CI #%d{", ci->id);
        vector<uint32_t>::iterator newciit = ci->itemset->begin();
        for (; newciit != ci->itemset->end(); ++newciit) {
          printf("%d,", *newciit);
        }
        printf("} -> ");
      }
      checkOfSize += 1;
    }
  }
  if (checkOfSize != numberOfCI) {
    std::printf("bad integrity, number of CI do not match : %d vs %d \n", checkOfSize, numberOfCI);
  }
#endif
  for (concept* ci : conceptContainer) {
    if (ci) {
#ifdef STORE_ITEMSET
      delete ci->itemset;
#endif
    }
  }

#ifdef _WIN32
  PROCESS_MEMORY_COUNTERS_EX info;
  GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&info, sizeof(info));
  cout << "WorkingSet " << info.WorkingSetSize / 1024 << "K, PeakWorkingSet " << info.PeakWorkingSetSize / 1024 << "K, PrivateSet " << info.PrivateUsage / 1024 << "K" << endl;
#endif
  return 0;
}