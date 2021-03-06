#include <vector>
#include <iostream>
#include <assert.h>
#include <queue>
#include <algorithm>

#include "graphe.hpp"
#include "board.hpp"


using namespace RR;
// ------------- Struct Noeud ------------- //

void Noeud::setValues(const Robot& rbt, const int& ind) {
  info = rbt;
  indice = ind;
}

void Noeud::addLink(Noeud* n) {
  linked.push_back(n);
}

bool Noeud::equalTo(const Robot& rbt) const {
  if(info.location == rbt.location && info.status == rbt.status)
    return true;
  return false;
}

void Noeud::display(const bool& withLinks) const {
  std::cout << "Je suis le noeud numero " << indice <<  " avec pour coordonnées : "
          << std::endl
          << "(" << info.location.line << "," << info.location.column << ") "
          << std::endl
          << sizeof(*this) << " octets"<< std::endl;
  if(withLinks) {
    std::cout << "Je pointe sur (normalement 7) : " << linked.size() << " noeud(s)"<< std::endl << std::endl;
  }
}


// ------------- Classe Graphe ------------- //

Graphe::Graphe(const Robot& d,const Location& a): depart(d), arrivee(a) {}

Graphe::Graphe(const Robot& d, const Board& board, const Location& a): depart(d), arrivee(a){
  construitGraphe(board, true);
  std::vector<int> solutions = pluscourtChemin({});

  for(int j = solutions.size() - 1; j > -1; j--) {
    afficheMouvement(solutions[j]);
  }

}



void Graphe::construitGraphe(const Board& board, const bool& verbose) {

  //on crée le noeud vers lequel va pointer les déplacements détruit
  detruit = new Noeud();
  detruit->info.location = Location(-1,-1);
  detruit->info.status = Robot::Status::DEAD;
  detruit->indice = -1;

  std::cout<< "starting graph building"<< std::endl;

  //on crée le noeud initial
  Noeud* n0 = new Noeud();
  n0->setValues(depart, 0);
  noeuds.push_back(n0);
  nbsommet++;

  int visited = 0; //entier correspondant a l'indice du noeud étudié dans le tableau
  Robot transfer; //donnée pour copier celles du noeud en étude
  int tmp; //valeur tampon


  while(visited < nbsommet) {
    if( not(noeuds[visited]->info.location == arrivee) ) {
      for(unsigned int i = 0; i < 7; i++) {
        transfer = noeuds[visited]->info;
        board.play(transfer,moves[i]);

        if(transfer.status == Robot::Status::DEAD) {
          noeuds[visited]->addLink(detruit);
        }
        else  {
          tmp = existeDeja(transfer);
          if(tmp != -1) {
            noeuds[visited]->addLink(noeuds[tmp]);
          } else {
            Noeud* n = new Noeud;

            n->setValues(transfer, noeuds.size());
            noeuds.push_back(n);
            nbsommet++;
            noeuds[visited]->addLink(noeuds[nbsommet - 1]);
          }
        }
      }
      if(verbose) {
        noeuds[visited]->display(true);
      }
      assert(nbsommet <= 128); // on a calculé qu'il peut y avoir au maximum 128 position possible du robot sur un plateau de 32 cases
    }
    visited++;
  }
  std::cout<<std::endl;
  std::cout<<"Done, "<< nbsommet << " cells produced" << std::endl;
}

int Graphe::existeDeja(const Robot& rbt) const {
  for(int i = 0; i < nbsommet; i++) {
    if(noeuds[i]->equalTo(rbt)){
      return i;
    }
  }
  return -1;
}

std::vector<int> Graphe::pluscourtChemin(std::vector<int> cards) {
  int info_noeuds[128][3]; // au max 128 sommets dans le graphe
  int i;
  for(i = 0; i < nbsommet; i++) {
    info_noeuds[i][0] = 10000; //la distance parcouru
    info_noeuds[i][1] = -1; //la cellule parente
    info_noeuds[i][2] = -1; //le mouvement entre la cellule parente et celle ci
    //(indice dans le tableau move pour économiser de l'espace mémoire)
  }

  int tmp = existeDeja(depart); // on retrouve le noeud de départ dans le graphe
  if(tmp == -1)
    return {}; //si jamais la position n'existe pas dans le graphe on termine le programme
  std::queue<int> file; //file d'indice a étudier

  file.push(tmp); //on ajoute l'indice noeud de départ
  info_noeuds[tmp][0] = 0;
  info_noeuds[tmp][1] = tmp; //on met comme parent lui-meme car c'est le point de départ
  info_noeuds[tmp][2] = -1;

  int distD; // la distance parcouru jusqu'a la cellule étudié
  int distA; // la distance précédente de la cellule liée

  int indiceI; //indice de la cellule liée
  int indiceM; //indice du mouvements dans le tableau move

  bool withcards = true; //la fonction sera légèrement différente si on donne a la méthode un tableau cards rempli
  if(cards.empty())
    withcards = false;

  bool legal = true; //par defaut on a toujours le droit de jouer un coup

  while(!file.empty()) {
    tmp = file.front(); //on prend le dernier elem et on le retire de la file
    file.pop();
    distD = info_noeuds[tmp][0] + 1;
    indiceM = 0;
    for(auto j = noeuds[tmp]->linked.begin(); j != noeuds[tmp]->linked.end(); ++j) {
      indiceI = (*j)->indice; //on récupère l'indice dans le tableau linked

      if(withcards) { //on regarde si le coup qui est joué est "LEGAL" => on possède une carte non joué
        legal = false;
        std::vector<int> notplayed = cards;//tableau qui constituera les cartes restantes pour une cellule
        //d'abord on enlève les cartes déjà utilisé en remontant les coup joué par les parents
        // remarque : on ne tiendra pas compte du  nombre de carte joué ici, ceci sera vérifié lors du résultat en dehors de la méthode
        
        int parent = tmp;
        for(i = 0; i < info_noeuds[tmp][0]; i++) { //pour chaque parent on vérifie & on enlève la carte du tableau
          for (auto it = notplayed.begin(); it != notplayed.end(); ++it) {
            if(info_noeuds[parent][2] == *it) {
              notplayed.erase(it);
              break;
            } //on arrete car on souhaite enlever une seule carte et non plusieurs
          }
          parent = info_noeuds[parent][1]; //indice de la cellule parente
        }
        //puis on regarde si le coup est autorisé a être joué (la carte est présente dans notplayed)

        if( std::find(notplayed.begin(), notplayed.end(), indiceM) != notplayed.end() )
          legal = true;
        }

      if(indiceI != -1 && legal) { //si l'indice ne correspond pas a la cellule "detruit"
        distA = info_noeuds[indiceI][0];

        if(distD < distA) {
          info_noeuds[indiceI][0] = distD;
          info_noeuds[indiceI][1] = tmp;
          info_noeuds[indiceI][2] = indiceM;
          file.push(indiceI);
        }
      }
      indiceM++; // on incrémente le compteur pour les mouvements associés
    }
  }

  //on récupère la solution parmi 4 possible (au max)
  int solution; //indice de la solution (on prendra la première proposé au poid le plus faible)
  int poidmin = 100000;
  for(i = 0; i < 4; i++) {
    tmp = existeDeja(Robot(arrivee, positions[i]));
    if(tmp != -1 && info_noeuds[tmp][0] < poidmin) {
      solution = tmp;
      poidmin = info_noeuds[tmp][0];
    }
  }
  if( info_noeuds[solution][0] != 10000 ){
    std::cout<<"la fin a un poid de : " << info_noeuds[solution][0] << std::endl << std::endl;
  }
  else {
    std::cout<<"Aucun chemin trouvé avec ces cartes..."<<std::endl;
    return {};
  }

  //on rempli le tableau des mouvements dans l'ordre inverse
  std::vector<int> mouvements_opti;
  int s = solution;
  for(i = 0; i < info_noeuds[solution][0]; i++) {
    mouvements_opti.push_back(info_noeuds[s][2]);
    s = info_noeuds[s][1]; //indice de la cellule parente a la solution
  }

  return mouvements_opti;
}

void afficheMouvement(const int& indice) {
  switch(indice) {
    case 0:
      std::cout<<  "Avancer de 1" <<std::endl;
      break;
    case 1:
      std::cout<<  "Avancer de 2" <<std::endl;
      break;
    case 2:
      std::cout<<  "Avancer de 3" <<std::endl;
      break;
    case 3:
      std::cout<<  "Reculer" <<std::endl;
      break;
    case 4:
      std::cout<<  "Tourner a gauche" <<std::endl;
      break;
    case 5:
      std::cout<<  "Tourner à droite" <<std::endl;
      break;
    case 6:
      std::cout<<  "Faire demi-tour" <<std::endl;
      break;
  }
}



Graphe::~Graphe(){
  delete detruit;
  for(long unsigned int i = 0; i < noeuds.size(); i++) {
    delete noeuds[i];
  }
}
