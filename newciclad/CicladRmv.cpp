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

#include "CicladRmv.hpp"
#include <iostream>
#include <algorithm>

extern size_t numberOfCI;
extern uint32_t __maxItem;

void updateCicladRmv(TRANSACTION *current_T, vector<vector<concept*>>*index, vector<concept*>*conceptContainer) {

  /*****************************************
   Creation du Trie d'intersection

   chaque noeud contient :
   - id qui est l'item du noeud
   - le geniteur candidat
   - est pointé par certain CI
   ******************************************/


   //racine de l'arbre
  trie_node_rmv root;
  root.children = new map<uint32_t, size_t>();
  root.depth = 0;
  root.genitors = new vector<concept*>();
  root.minimalCandidates = new vector<concept*>();
  root.item = __maxItem;//est ce important ? peut on mettre iteger max ?
  root.nb_ref = 0;
  root.parent = 0;
  vector<trie_node_rmv> tnodes;
  tnodes.push_back(root);
  //on boucle sur les items de la transaction
  //pour rappel current_T->itemset[0] contient current_T.size()
  for (uint32_t pos = 0; pos < (current_T->itemset[0]); ++pos) {
    uint32_t item = current_T->itemset[1 + pos];

    //On recupere les CI qui possedent l'item (l'ideal ?)
    //vector<concept*>* listeCI = &(*index)[item];
    for (concept* ci : (*index)[item]) {
      if (!ci) {
        continue;
      }
#ifdef STORE_ITEMSET
      if (ci->id && ci->size != ci->itemset->size()) {
        exit(1);
      }
#endif            
      size_t ref;
      //on recupere la derniere reference/noeud pour le CI courant
      // et on met a jour la reference, dernier noeud touché par le CI
      // et on expandPAth

      if (ci->lastitem > tnodes.size()) {
        cout << "ci->lastitem > tnodes.size()" << endl;
        exit(1);
      }

      if (!ci->lastitem) {
        ref = 0;
      }
      else {
        ref = ci->lastitem;
      }
      expandPathRmv(item, ref, ci, &tnodes);//on etend les intersections partielles avec l'item courant
    }
  }


  /**************************************************

   Tri entre obsoletes noeuds et noeuds retrogradés

   **************************************************/
   //boucler sur les noeuds pour creer les obsoletes ou les decrementations de support
  const size_t tnodes_len = tnodes.size();
  for (size_t i = 1; i < tnodes_len; ++i) {
    trie_node_rmv* const node = &tnodes[i];
    if (node->nb_ref != 0) {
      bool stateModified = true;
      if (node->genitors && !node->genitors->empty()) {

        concept** it = &(*(node->genitors->begin()));
        const size_t genLen = node->genitors->size();
        for (int zzz = 0; zzz != genLen; ++zzz) {
          concept* const genitor = *(it + zzz);

          if (genitor->lastitem == i) {
            //degager puis
            std::vector<uint32_t>* obsolete = trie_path(&tnodes, /*&*/node);
            if (tnodes[i].minimalCandidates->at(0)->size != obsolete->size()) {
              cout << "tnodes[i].minimalCandidates->at(0)->size != obsolete->size() : " << tnodes[i].minimalCandidates->at(0)->size << " vs " << obsolete->size() << endl;
              exit(1);
            }
            for (size_t j = 0; j < obsolete->size(); ++j) {
              const uint32_t item = obsolete->at(j);
              const size_t position = tnodes[i].minimalCandidates->at(0)->positionsInIndex[j];
              (*index)[item].at(position) = 0;
#ifdef REUSE_OBSOLETE
              available_positions_for_new_cis[item]->push(position);
#endif
            }
            delete obsolete;
            free(tnodes[i].minimalCandidates->at(0)->positionsInIndex);
#ifdef STORE_ITEMSET
            free(tnodes[i].minimalCandidates->at(0)->itemset);
#endif
            (*conceptContainer)[tnodes[i].minimalCandidates->at(0)->id] = 0;
#ifdef REUSE_OBSOLETE
            available_id_for_new_cis->push(tnodes[i].minimalCandidates->at(0)->id);
#endif

            numberOfCI--;
            stateModified = false;
            break;
          }
        }
      }

      if (stateModified) {
        tnodes[i].minimalCandidates->at(0)->support--;
    }
  }
    delete tnodes[i].genitors;
    delete tnodes[i].minimalCandidates;
    delete tnodes[i].children;
}


  concept** it = &(*(conceptContainer->begin()));
  const size_t genLen = conceptContainer->size();
  for (int zzz = 0; zzz != genLen; ++zzz) {
    concept* const ci = *(it + zzz);
    if (ci) {
      ci->lastitem = 0;
    }
  }
  delete root.children;
  delete root.genitors;
  delete root.minimalCandidates;
}

void expandPathRmv(uint32_t item, size_t ref_pos, concept* ci, vector<trie_node_rmv>* tnodes) {//*
  trie_node_rmv* ref = &((*tnodes)[ref_pos]);
  std::map<uint32_t, size_t>::iterator it = ref->children->find(item);

  size_t ref_tmp_pos;
  if (it == ref->children->end()) {
    //on ajoute le nouveau noeud, s'il n'existe pas
    trie_node_rmv fils;
    fils.item = item;
    fils.parent = ref_pos;
    fils.depth = ref->depth + 1;
    fils.nb_ref = 0;
    //TODO: deallocate map
    fils.children = new map<uint32_t, size_t>();
    ref->children->emplace(item, tnodes->size());
    tnodes->push_back(fils);

    (*tnodes)[tnodes->size() - 1].minimalCandidates = new vector<concept*>();
    (*tnodes)[tnodes->size() - 1].minimalCandidates->push_back(ci);
    (*tnodes)[tnodes->size() - 1].genitors = new vector<concept*>();
    ref_tmp_pos = tnodes->size() - 1;

  }
  else {
    ref_tmp_pos = it->second;

    if (0 == (*tnodes)[ref_tmp_pos].minimalCandidates->size()) {
    }
    else {
      const uint32_t right = (*tnodes)[ref_tmp_pos].minimalCandidates->at(0)->support;
      const uint32_t left = ci->support;
      int ecart = left - right;
      switch (ecart) {
      case 0:
        (*tnodes)[ref_tmp_pos].minimalCandidates->push_back(ci);
        break;
      case 1:
        delete (*tnodes)[ref_tmp_pos].genitors;
        (*tnodes)[ref_tmp_pos].genitors = (*tnodes)[ref_tmp_pos].minimalCandidates;
        (*tnodes)[ref_tmp_pos].minimalCandidates = new vector<concept*>();
        (*tnodes)[ref_tmp_pos].minimalCandidates->push_back(ci);
        break;
      case -1:
        (*tnodes)[ref_tmp_pos].genitors->push_back(ci);
        break;
      default:
        if (ecart >= 2) {
          (*tnodes)[ref_tmp_pos].genitors->clear();
          (*tnodes)[ref_tmp_pos].minimalCandidates->clear();
          (*tnodes)[ref_tmp_pos].minimalCandidates->push_back(ci);
        }
        break;
      }
    }
  }

  //nbr refs sert a savoir si il s'agit d'un prefixe correspondant a une intersection valide (si != 0)
  (*tnodes)[ref_pos].nb_ref -= 1;
  (*tnodes)[ref_tmp_pos].nb_ref += 1;
  ci->lastitem = ref_tmp_pos;
}