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

#include "CicladAdd.hpp"
#include <stdio.h>
#include <iostream>
#include <algorithm>

using namespace std;

extern uint32_t __maxItem;
extern size_t numberOfCI;

static uint32_t CID = 1;


//on rajoute une ligne de transaction et on compute les nouveaux CI
void updateCicladAdd(TRANSACTION *current_T, vector<vector<concept*>>*index, vector<concept*>*conceptContainer, vector<set<uint32_t>*>* const _genContainer) {


  /*****************************************
      Creation du Trie d'intersection

   chaque noeud contient :
      - id qui est l'item du noeud
      - le geniteur candidat
      - est pointé par certain CI
  ******************************************/


  //racine de l'arbre
  trie_node_add root;
  root.children = new map<uint32_t, size_t>();
  root.depth = 0;
  root.genitor = NULL;
  root.item = __maxItem;// ?
  root.nb_ref = 0;
  root.parent = 0;
  vector<trie_node_add> tnodes;
  tnodes.push_back(root);
  //on boucle sur les items de la transaction
  //pour rappel current_T->itemset[0] contient current_T.size()
  for (uint32_t pos = 0; pos < (current_T->itemset[0]); ++pos) {
    const uint32_t item = current_T->itemset[1 + pos];

    //On recupere les CI qui possedent l'item (l'ideal ?)
      //vector<concept*>* listeCI = &(*index)[item];

    for (concept* ci : (*index)[item]) {
      if (!ci) continue;

      size_t ref;
      //on recupere la derniere reference/noeud pour le CI courant
      // et on met a jour la reference, dernier noeud touché par le CI
        // et on expandPAth
      if (!ci->lastitem) {
        ref = 0;
      }
      else {
        ref = ci->lastitem;
      }
      expandPathAdd(item, ref, ci, &tnodes);//on etend les intersections partielles avec l'item courant
    }
  }


  /*******************************************

   Tri entre nouveaux noeuds et noeuds promus

   *******************************************/
   //boucler sur les noeuds pour creer les nouveaux ou les incrementations de support
  for (size_t i = 1; i < tnodes.size(); ++i) {
    trie_node_add node = tnodes[i];
    if (node.nb_ref != 0) {
      if (node.depth != node.genitor->size) {
        //Creer le CI
        concept* const newCi = (concept*)malloc(sizeof(concept));
        newCi->size = node.depth;
        //TODO : newCi.itemset=
        newCi->support = (node.genitor->support) + 1;
        // le referencer dans l'index
        //newCi->itemset = trie_path(&tnodes, &node);

        std::vector<uint32_t>* const path = trie_path(&tnodes, &node);
#ifdef STORE_ITEMSET
        newCi->itemset = path;
#endif

        newCi->positionsInIndex = (size_t*)malloc(sizeof(size_t) * (/*newCi->itemset*/path->size()));

#ifdef REUSE_OBSOLETE
        bool was_incremented = false;
        if (!available_id_for_new_cis->empty()) {
          const size_t oldId = available_id_for_new_cis->front();
          available_id_for_new_cis->pop();
          newCi->id = oldId;
        }
        else {

          newCi->id = CID++;
          was_incremented = true;
        }
#else
      newCi->id = CID++;// conceptContainer->size();// (*conceptContainer)[conceptContainer->size() - 1]->id + 1;
#endif
        for (size_t j = 0; j < /*newCi->itemset*/path->size(); ++j) {
          const uint32_t item = /*newCi->itemset*/path->at(j);

#ifdef REUSE_OBSOLETE
          if (!available_positions_for_new_cis[item]->empty()) {
            const size_t pos = available_positions_for_new_cis[item]->front();
            available_positions_for_new_cis[item]->pop();
            (*index)[item][pos] = newCi;
            newCi->positionsInIndex[j] = pos;
          }
          else {
            (*index)[item].push_back(newCi);
            const size_t len = (*index)[item].size() - 1;
            newCi->positionsInIndex[j] = len;
          }
#else
          (*index)[item].push_back(newCi);
          const size_t len = (*index)[item].size() - 1;
          newCi->positionsInIndex[j] = len;
#endif
        }

#ifdef REUSE_OBSOLETE
        if (was_incremented) {
          conceptContainer->push_back(newCi);
          //was_incremented = false;
        }
        else {
          (*conceptContainer)[newCi->id] = newCi;
        }
#else
        conceptContainer->push_back(newCi);
#endif
        numberOfCI++;
#ifndef STORE_ITEMSET
        delete path;
#endif
      }
      else if (node.genitor) {
        node.genitor->support++;
      }
    }
    delete tnodes[i].children;
  }
  for (auto ci : *conceptContainer) {
    if (ci) ci->lastitem = 0;
  }
  delete root.children;
}

void expandPathAdd(uint32_t item, size_t ref_pos, concept* ci, vector<trie_node_add>* tnodes) {//*
  trie_node_add* ref = &((*tnodes)[ref_pos]);
  std::map<uint32_t, size_t>::iterator it = ref->children->find(item);

  size_t ref_tmp_pos;
  if (it == ref->children->end()) {
    //on ajoute le nouveau noeud, s'il n'existe pas
    trie_node_add fils;
    fils.item = item;
    fils.parent = ref_pos;
    fils.depth = ref->depth + 1;
    fils.nb_ref = 0;
    //TODO: deallocate map 
    fils.children = new map<uint32_t, size_t>();
    ref->children->emplace(item, tnodes->size());
    tnodes->push_back(fils);

    (*tnodes)[tnodes->size() - 1].genitor = ci;
    ref_tmp_pos = tnodes->size() - 1;

  }
  else {
    ref_tmp_pos = it->second;
    if ((*tnodes)[it->second].genitor->support < ci->support) {
      (*tnodes)[it->second].genitor = ci;
    }
  }

  //nbr refs sert a savoir si il s'agit d'un prefixe correspondant a une intersection valide (si != 0)
  (*tnodes)[ref_pos].nb_ref -= 1;
  //ref->nb_ref -= 1;
  (*tnodes)[ref_tmp_pos].nb_ref += 1;
  ci->lastitem = ref_tmp_pos;
  //*/
}