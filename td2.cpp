#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>

#include "cppitertools/range.hpp"
#include "gsl/span"

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).

using namespace std;
using namespace iter;
using namespace gsl;

#pragma endregion//}

typedef uint8_t UInt8;
typedef uint16_t UInt16;

#pragma region "Fonctions de base pour lire le fichier binaire"//{

UInt8 lireUint8(istream& fichier)
{
	UInt8 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
UInt16 lireUint16(istream& fichier)
{
	UInt16 valeur = 0;
	fichier.read((char*)&valeur, sizeof(valeur));
	return valeur;
}
string lireString(istream& fichier)
{
	string texte;
	texte.resize(lireUint16(fichier));
	fichier.read((char*)&texte[0], streamsize(sizeof(texte[0])) * texte.length());
	return texte;
}

#pragma endregion//}

//TODO: Une fonction pour ajouter un Film à une ListeFilms, le film existant déjà; on veut uniquement ajouter le pointeur vers le film existant.  Cette fonction doit doubler la taille du tableau alloué, avec au minimum un élément, dans le cas où la capacité est insuffisante pour ajouter l'élément.  Il faut alors allouer un nouveau tableau plus grand, copier ce qu'il y avait dans l'ancien, et éliminer l'ancien trop petit.  Cette fonction ne doit copier aucun Film ni Acteur, elle doit copier uniquement des pointeurs.

void ajouterFilm(Film* filmToAdd, ListeFilms films) {
	
	if (films.nElements == films.capacite)						// Si il ne reste plus de capacite dans ListeFilms.elements:
	{
		Film** elements= new Film* [films.capacite * 2];		// Creer un nouveau tableau elements contenant des pointeurs de pointeurs de films avec le double de la capacite precedente
		for (int i = 0; i < films.nElements; i++)
		{
			elements[i] = films.elements[i];
			if (i == films.nElements -1)
			{
				films.elements[i + 1];
			}
		}
		delete(films.elements);									// Supprime l'ancien tableau
		films.elements = elements;								// Remplace l'ancien tableau par le nouveau
		//films.elements.push_back(filmToAdd);					// Ajoute le nouveau film
		films.nElements++;										// Met a jour le nombre d'elements  de ListeFilms
		films.capacite = films.capacite * 2;					// Met a jour la capacite de ListeFilms
	}
	else														// Si il reste de la capacite dans ListeFilms.elements:
	{
		//films.elements.push_back(filmToAdd);					// Ajoute le nouveau film
		films.elements [films.nElements + 1] = filmToAdd;
		films.nElements++;										// Met a jour le nombre d'elements de ListeFilms
	}
}
//TODO: Une fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; la fonction prenant en paramètre un pointeur vers le film à enlever.  L'ordre des films dans la liste n'a pas à être conservé.
void enleverFilm(Film* pointeurFilm, ListeFilms films) {
	bool removed = false;
	for (int i = 0; i < films.nElements; i++) { // Parcours ListeFilms.elements a la recherche du pointeur passe en parametre
		if (films.elements[i] == pointeurFilm) { 
			delete(films.elements[i]);
			removed = true;
		}
	}
	if (!removed) // Cette boucle indique si le film n a pas ete trouve dans la boucle
	{
		cout << "Le film a enlever n a pas ete trouve dans la liste" << endl;
	}
}
//TODO: Une fonction pour trouver un Acteur par son nom dans une ListeFilms, qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé.  Devrait utiliser span.
Acteur* trouverActeur(const ListeFilms listeFilms, string nom_acteur) {
	
	Film** films = listeFilms.elements;
	for (int i = 0; i < listeFilms.nElements; i++)
	{
		Film* film = films[i];
		if (film != NULL)
		{
			span<Acteur*> listeActeurs = span(film->acteurs.elements, film->acteurs.nElements);
			for (auto k : listeActeurs) {
				if (k->nom == nom_acteur)
				{
					return k;
				}
			}
		}
		
	}
	return nullptr; // Si on ne trouve pas l'acteur, on retourne nullptr
}
//TODO: Compléter les fonctions pour lire le fichier et créer/allouer une ListeFilms.  La ListeFilms devra être passée entre les fonctions, pour vérifier l'existence d'un Acteur avant de l'allouer à nouveau (cherché par nom en utilisant la fonction ci-dessus).
Acteur* lireActeur(istream& fichier, ListeFilms listeFilms)
{
	Acteur acteur = {};
	acteur.nom            = lireString(fichier);
	acteur.anneeNaissance = lireUint16 (fichier);
	acteur.sexe           = lireUint8  (fichier);
	Acteur* pointeurActeur = new Acteur;
	pointeurActeur = trouverActeur(listeFilms, acteur.nom); // Appel trouverActeur pour cherche un pointeur
	if (pointeurActeur != nullptr) { // Si on a trouvé un pointeur, on le retourne
		return pointeurActeur;
	}
	else {	// Sion a pas de pointeur existant, on doit ajouter l'acteur a la liste 
		pointeurActeur = &acteur;
		cout << acteur.nom << "n'a ete trouve dans aucun film, nouveua pointeur creer" << endl;
	}
	return pointeurActeur; //TODO: Retourner un pointeur soit vers un acteur existant ou un nouvel acteur ayant les bonnes informations, selon si l'acteur existait déjà.  Pour fins de débogage, affichez les noms des acteurs crées; vous ne devriez pas voir le même nom d'acteur affiché deux fois pour la création.
}

Film* lireFilm(istream& fichier, ListeFilms listeFilms)
{
	Film film = {};
	film.titre       = lireString(fichier);
	film.realisateur = lireString(fichier);
	film.anneeSortie = lireUint16 (fichier);
	film.recette     = lireUint16 (fichier);
	film.acteurs.nElements = lireUint8 (fichier);  //NOTE: Vous avez le droit d'allouer d'un coup le tableau pour les acteurs, sans faire de réallocation comme pour ListeFilms.  Vous pouvez aussi copier-coller les fonctions d'allocation de ListeFilms ci-dessus dans des nouvelles fonctions et faire un remplacement de Film par Acteur, pour réutiliser cette réallocation.
	static const int nombreActeurs = film.acteurs.nElements;
	film.acteurs.elements = new Acteur*[nombreActeurs];
	for (int i = 0; i < nombreActeurs; i++)
	{
		film.acteurs.elements[i] = NULL;
	}
	Film* ptrFilm = new Film;
	ptrFilm = &film;
	
	for (int i = 0; i < film.acteurs.nElements; i++) {
		 Acteur* ptrActeur = lireActeur(fichier, listeFilms); 
		 //TODO: Placer l'acteur au bon endroit dans les acteurs du film.
		 film.acteurs.elements[film.acteurs.nElements + 1] = ptrActeur;
		//TODO: Ajouter le film à la liste des films dans lesquels l'acteur joue.
		 ptrActeur->joueDans.elements[ptrActeur->joueDans.nElements +1] = ptrFilm; // Ajouter la fonction ajouterFilm au lieu de cette ligne
		 film.acteurs.nElements++;
		 ptrActeur->joueDans.nElements++;
	}
	return ptrFilm; //TODO: Retourner le pointeur vers le nouveau film.
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);
	
	int nElements = lireUint16(fichier);

	//TODO: Créer une liste de films vide.
	ListeFilms listeFilms = {};
	listeFilms.nElements = nElements;
	listeFilms.elements = new Film*[nElements * 2];
	for (int i = 0; i < nElements; i++)
	{
		listeFilms.elements[i] = NULL;
	}
	for (int i = 0; i < nElements; i++) {
		Film* ptrFilm = lireFilm(fichier, listeFilms); //TODO: Ajouter le film à la liste.
		listeFilms.nElements++;
		listeFilms.elements[listeFilms.nElements]= ptrFilm;
	}
	
	return listeFilms; //TODO: Retourner la liste de films.
}

//TODO: Une fonction pour détruire un film (relâcher toute la mémoire associée à ce film, et les acteurs qui ne jouent plus dans aucun films de la collection).  Noter qu'il faut enleve le film détruit des films dans lesquels jouent les acteurs.  Pour fins de débogage, affichez les noms des acteurs lors de leur destruction.
void deleteFilm(Film* ptrFilm) {
	int index = 0;
	for (int i = 0; i < ptrFilm->acteurs.nElements; i++)
	{
		cout << ptrFilm->acteurs.elements[i]->nom << endl;
		span<Film*> filmsActeur = span(ptrFilm->acteurs.elements[i]->joueDans.elements, ptrFilm->acteurs.elements[i]->joueDans.nElements);
		for (auto k : filmsActeur) {
			if (k == ptrFilm)
			{
				delete ptrFilm->acteurs.elements[i]->joueDans.elements[index];
				ptrFilm->acteurs.elements[i]->joueDans.nElements--;
				if (ptrFilm->acteurs.elements[i]->joueDans.nElements == 0)
				{
					delete ptrFilm->acteurs.elements[i];
				}
			}
			index++;
		}
		//delete ptrFilm->acteurs.elements[i]->joueDans[ptrFilm]; // Supprime le film de la liste joueDans de chaque acteur
	}
	delete ptrFilm;
}
//TODO: Une fonction pour détruire une ListeFilms et tous les films qu'elle contient.
void deleteListeFilms(ListeFilms listeFilms) {
	for (int i = 0; i < listeFilms.nElements; i++)
	{
		deleteFilm(listeFilms.elements[i]);
	}
	delete &listeFilms;
}
void afficherActeur(const Acteur& acteur)
{
	cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}

//TODO: Une fonction pour afficher un film avec tous ces acteurs (en utilisant la fonction afficherActeur ci-dessus).
void afficherFilm(const Film* ptrFilm) {
	cout << ptrFilm->titre << " realise par " << ptrFilm->realisateur << "sortie en " << ptrFilm->anneeSortie << " Pour une recette de " << ptrFilm->recette << endl;
	cout << "Avec les acteurs: " << endl;
	span<Acteur*> listeActeurs = span(ptrFilm->acteurs.elements, ptrFilm->acteurs.nElements);
	for (auto i : listeActeurs) {
		afficherActeur(*i);
	}
}
void afficherListeFilms(const ListeFilms& listeFilms)
{
	//TODO: Utiliser des caractères Unicode pour définir la ligne de séparation (différente des autres lignes de séparations dans ce progamme).
	static const string ligneDeSeparation = "separation--separation--separation--separation--separation--separation--separation--separation";
	cout << ligneDeSeparation << endl;
	//TODO: Changer le for pour utiliser un span.
	span<Film*> films = span(listeFilms.elements, listeFilms.nElements);
	for (auto i: films) {
		//TODO: Afficher le film.
		afficherFilm(i);
		cout << ligneDeSeparation << endl;
	}
}

void afficherFilmographieActeur(const ListeFilms& listeFilms, const string& nomActeur)
{
	//TODO: Utiliser votre fonction pour trouver l'acteur (au lieu de le mettre à nullptr).
	const Acteur* acteur = trouverActeur(listeFilms, nomActeur);
	if (acteur == nullptr)
		cout << "Aucun acteur de ce nom" << endl;
	else
		afficherListeFilms(acteur->joueDans);
}

int main()
{
	#ifdef VERIFICATION_ALLOCATION_INCLUS
	bibliotheque_cours::VerifierFuitesAllocations verifierFuitesAllocations;
	#endif
	bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

	int* fuite = new int; //TODO: Enlever cette ligne après avoir vérifié qu'il y a bien un "Fuite detectee" de "4 octets" affiché à la fin de l'exécution, qui réfère à cette ligne du programme.

	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	//TODO: Chaque TODO dans cette fonction devrait se faire en 1 ou 2 lignes, en appelant les fonctions écrites.

	//TODO: La ligne suivante devrait lire le fichier binaire en allouant la mémoire nécessaire.  Devrait afficher les noms de 20 acteurs sans doublons (par l'affichage pour fins de débogage dans votre fonction lireActeur).
	ListeFilms listeFilms = creerListe("films.bin");
	
	cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
	//TODO: Afficher le premier film de la liste.  Devrait être Alien.
	afficherFilm(listeFilms.elements[0]);
	cout << ligneDeSeparation << "Les films sont:" << endl;
	//TODO: Afficher la liste des films.  Il devrait y en avoir 7.
	afficherListeFilms(listeFilms);
	//TODO: Modifier l'année de naissance de Benedict Cumberbatch pour être 1976 (elle était 0 dans les données lues du fichier).  Vous ne pouvez pas supposer l'ordre des films et des acteurs dans les listes, il faut y aller par son nom.
	Acteur* ptrBenedict = trouverActeur(listeFilms, "Benedict Cumberbatch");
	ptrBenedict->anneeNaissance = 1978;
	cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
	//TODO: Afficher la liste des films où Benedict Cumberbatch joue.  Il devrait y avoir Le Hobbit et Le jeu de l'imitation.
	afficherFilmographieActeur(listeFilms, "Benedict Cumberbatch");
	//TODO: Détruire et enlever le premier film de la liste (Alien).  Ceci devrait "automatiquement" (par ce que font vos fonctions) détruire les acteurs Tom Skerritt et John Hurt, mais pas Sigourney Weaver puisqu'elle joue aussi dans Avatar.
	deleteFilm(listeFilms.elements[0]);
	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
	//TODO: Afficher la liste des films.
	afficherListeFilms(listeFilms);
	//TODO: Faire les appels qui manquent pour avoir 0% de lignes non exécutées dans le programme (aucune ligne rouge dans la couverture de code; c'est normal que les lignes de "new" et "delete" soient jaunes).  Vous avez aussi le droit d'effacer les lignes du programmes qui ne sont pas exécutée, si finalement vous pensez qu'elle ne sont pas utiles.
	
	//TODO: Détruire tout avant de terminer le programme.  L'objet verifierFuitesAllocations devrait afficher "Aucune fuite detectee." a la sortie du programme; il affichera "Fuite detectee:" avec la liste des blocs, s'il manque des delete.
}
