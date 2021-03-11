// Solutionnaire du TD2 INF1015 hiver 2021
// Par Francois-R.Boyer@PolyMtl.ca

#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.

#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include "cppitertools/range.hpp"
#include "gsl/span"
#include "debogage_memoire.hpp"        // Ajout des numéros de ligne des "new" dans le rapport de fuites.  Doit être après les include du système, qui peuvent utiliser des "placement new" (non supporté par notre ajout de numéros de lignes).
#include <memory>
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

// Fonctions pour ajouter un Film à une ListeFilms.
//[
void ListeFilms::changeDimension(int nouvelleCapacite)
{
	Film** nouvelleListe = new Film*[nouvelleCapacite];
	
	if (elements != nullptr) {  // Noter que ce test n'est pas nécessaire puique nElements sera zéro si elements est nul, donc la boucle ne tentera pas de faire de copie, et on a le droit de faire delete sur un pointeur nul (ça ne fait rien).
		nElements = min(nouvelleCapacite, nElements);
		for (int i : range(nElements))
			nouvelleListe[i] = elements[i];
		delete[] elements;
	}
	
	elements = nouvelleListe;
	capacite = nouvelleCapacite;
}

void ListeFilms::ajouterFilm(Film* film)
{
	if (nElements == capacite)
		changeDimension(max(1, capacite * 2));
	elements[nElements++] = film;
}
//]

// Fonction pour enlever un Film d'une ListeFilms (enlever le pointeur) sans effacer le film; la fonction prenant en paramètre un pointeur vers le film à enlever.  L'ordre des films dans la liste n'a pas à être conservé.
//[
// On a juste fait une version const qui retourne un span non const.  C'est valide puisque c'est la struct qui est const et non ce qu'elle pointe.  Ça ne va peut-être pas bien dans l'idée qu'on ne devrait pas pouvoir modifier une liste const, mais il y aurais alors plusieurs fonctions à écrire en version const et non-const pour que ça fonctionne bien, et ce n'est pas le but du TD (il n'a pas encore vraiment de manière propre en C++ de définir les deux d'un coup).
span<Film*> ListeFilms::enSpan() const { return span(elements, nElements); }

void ListeFilms::enleverFilm(const Film* film)
{
	for (Film*& element : enSpan()) {  // Doit être une référence au pointeur pour pouvoir le modifier.
		if (element == film) {
			if (nElements > 1)
				element = elements[nElements - 1];
			nElements--;
			return;
		}
	}
}
//]

// Fonction pour trouver un Acteur par son nom dans une ListeFilms, qui retourne un pointeur vers l'acteur, ou nullptr si l'acteur n'est pas trouvé.  Devrait utiliser span.
//[
// Voir la NOTE ci-dessous pourquoi Acteur* n'est pas const.  Noter que c'est valide puisque c'est la struct uniquement qui est const dans le paramètre, et non ce qui est pointé par la struct.
span<shared_ptr<Acteur>> spanListeActeurs(const ListeActeurs& liste) {


	return span( liste.elements.get(),liste.nElements); }

//NOTE: Doit retourner un Acteur modifiable, sinon on ne peut pas l'utiliser pour modifier l'acteur tel que demandé dans le main, et on ne veut pas faire écrire deux versions aux étudiants dans le TD2.
Acteur* ListeFilms::trouverActeur(const string& nomActeur) const
{
	for (const Film* film : enSpan()) {
		for (shared_ptr<Acteur> acteur : spanListeActeurs(film->acteurs)) {
			if (acteur->nom == nomActeur)
				return acteur.get();
		}
	}
	return nullptr;
}
//]

// Les fonctions pour lire le fichier et créer/allouer une ListeFilms.


shared_ptr<Acteur> lireActeur(istream& fichier, ListeFilms& listeFilms)
{
	Acteur acteur = {};
	acteur.nom            = lireString(fichier);
	acteur.anneeNaissance = lireUint16 (fichier);
	acteur.sexe           = lireUint8  (fichier);

	Acteur* acteurExistant = listeFilms.trouverActeur(acteur.nom);
	if (acteurExistant != nullptr) {
		shared_ptr<Acteur> shared_ptr_acteurExistant = make_shared<Acteur>(*acteurExistant);
		return shared_ptr_acteurExistant;
	}
	else {
		cout << "Création Acteur " << acteur.nom << endl;
		shared_ptr<Acteur> shared_ptr_acteur = make_shared<Acteur>(acteur);
		return shared_ptr_acteur;
	}
	
}

Film* lireFilm(istream& fichier, ListeFilms& listeFilms)
{
	
	string titre = lireString(fichier);
	string realisateur = lireString(fichier);
	int anneeSortie = lireUint16(fichier);
	int recette = lireUint16(fichier);
	int nombreActeurs = lireUint8(fichier);

	Film* filmp = new Film(nombreActeurs);
	filmp->titre = titre;
	filmp->realisateur = realisateur;
	filmp->anneeSortie = anneeSortie;
	filmp->recette = recette;
	filmp->acteurs.nElements = nombreActeurs;

	 //NOTE: On aurait normalement fait le "new" au début de la fonction pour directement mettre les informations au bon endroit; on le fait ici pour que le code ci-dessus puisse être directement donné aux étudiants dans le TD2 sans qu'ils aient le "new" déjà écrit.  On aurait alors aussi un nom "film" pour le pointeur, pour suivre le guide de codage; on a mis un suffixe "p", contre le guide de codage, pour le différencier de "film".
	cout << "Création Film " << filmp->titre << endl;
	ListeActeurs listeActeurs = ListeActeurs(filmp->acteurs.nElements);
	for (shared_ptr<Acteur>& acteur : spanListeActeurs(filmp->acteurs)) {
		acteur = make_shared<Acteur>(*lireActeur(fichier, listeFilms)); 
	}
	return filmp;
}

ListeFilms creerListe(string nomFichier)
{
	ifstream fichier(nomFichier, ios::binary);
	fichier.exceptions(ios::failbit);
	
	int nElements = lireUint16(fichier);

	ListeFilms listeFilms;
	for ([[maybe_unused]] int i : range(nElements)) { //NOTE: On ne peut pas faire un span simple avec ListeFilms::enSpan car la liste est vide et on ajoute des éléments à mesure.
		listeFilms.ajouterFilm(lireFilm(fichier, listeFilms));
	}
	
	return listeFilms;
}

// Fonctions pour détruire un film (relâcher toute la mémoire associée à ce film, et les acteurs qui ne jouent plus dans aucun films de la collection).  Enlève aussi le film détruit des films dans lesquels jouent les acteurs.  Pour fins de débogage, les noms des acteurs sont affichés lors de leur destruction.
//[
void detruireActeur(Acteur* acteur)
{
	cout << "Destruction Acteur " << acteur->nom << endl;
	//acteur->joueDans.detruire();
	delete acteur;
}

void detruireFilm(Film* film)
{
	
	cout << "Destruction Film " << film->titre << endl;
	delete film;
}
//]

// Fonction pour détruire une ListeFilms et tous les films qu'elle contient.
//[
//NOTE: La bonne manière serait que la liste sache si elle possède, plutôt qu'on le dise au moment de la destruction, et que ceci soit le destructeur.  Mais ça aurait complexifié le TD2 de demander une solution de ce genre, d'où le fait qu'on a dit de le mettre dans une méthode.
void ListeFilms::detruire(bool possedeLesFilms)
{
	if (possedeLesFilms)
		for (Film* film : enSpan())
			detruireFilm(film);
	delete[] elements;
}
//]

void afficherActeur(const Acteur& acteur)
{
	cout << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}
ostream& operator<<(ostream& o, const Film& film) {
	
	cout << "Titre : " << film.titre << '\n' << "Realisateur : " << film.realisateur << "\n Annee : "<< film.anneeSortie << "\n Recette : " << film.recette << "M$" << "\n Acteurs : " << endl;
	for (const shared_ptr<Acteur> acteur : spanListeActeurs(film.acteurs))
		afficherActeur(*acteur);
	return o << "" << endl;
}
// Fonction pour afficher un film avec tous ces acteurs (en utilisant la fonction afficherActeur ci-dessus).
//[
void afficherFilm(const Film& film)
{
	cout << film;
}
//]

void afficherListeFilms(const ListeFilms& listeFilms)
{
	static const string ligneDeSeparation = //[
		"\033[32m────────────────────────────────────────\033[0m\n";
	cout << ligneDeSeparation;
	for (const Film* film : listeFilms.enSpan()) {
		afficherFilm(*film);
		cout << ligneDeSeparation;
	}
}

int main()
{
	#ifdef VERIFICATION_ALLOCATION_INCLUS
	bibliotheque_cours::VerifierFuitesAllocations verifierFuitesAllocations;
	#endif
	bibliotheque_cours::activerCouleursAnsi();  // Permet sous Windows les "ANSI escape code" pour changer de couleurs https://en.wikipedia.org/wiki/ANSI_escape_code ; les consoles Linux/Mac les supportent normalement par défaut.

	static const string ligneDeSeparation = "\n\033[35m════════════════════════════════════════\033[0m\n";

	ListeFilms listeFilms = creerListe("films.bin");
	
	cout << ligneDeSeparation << "Le premier film de la liste est:" << endl;
	// Le premier film de la liste.  Devrait être Alien.
	afficherFilm(*listeFilms.enSpan()[0]);

	cout << ligneDeSeparation << "Les films sont:" << endl;
	// Affiche la liste des films.  Il devrait y en avoir 7.
	afficherListeFilms(listeFilms);

	listeFilms.trouverActeur("Benedict Cumberbatch")->anneeNaissance = 1976;

	//cout << ligneDeSeparation << "Liste des films où Benedict Cumberbatch joue sont:" << endl;
	// Affiche la liste des films où Benedict Cumberbatch joue.  Il devrait y avoir Le Hobbit et Le jeu de l'imitation.
	//afficherFilmographieActeur(listeFilms, "Benedict Cumberbatch");
	//Creation de Skylien: 
	Film Skylien = listeFilms[0];
	Skylien.titre = "Skylien";
	Skylien.acteurs.elements[0] = listeFilms.donnerActeur(1, 0);
	Skylien.acteurs.elements[0].get()->changerNom("Daniel Wroughton Craig");
	cout << Skylien << listeFilms[0] << listeFilms[1];
	// Cherche un film avec un critère spécifique:
	cout << "Le film avec une recette de 955M$ est: " << endl;
	Film* ptrFilm = listeFilms.chercherFilm([](Film* Film) {return Film->recette == 955; });
	cout << *ptrFilm;
	// Fais la copie de ListeTextes:
	Liste<string> listeTextes(2);
	listeTextes.elements[0] = make_shared<string>("Un Alien de notre monde");
	listeTextes.elements[1] = make_shared<string>("Un humain random");
	Liste<string> listeTextes2 = listeTextes;
	//Modifie les listeTextes
	listeTextes.elements[0] = make_shared<string>("D'un monde alternatif vient un nouveau 1er string");
	*listeTextes.elements[1] += " de notre monde";
	//Afficher les textes
	cout << "La premiere phrase de la liste originale est: " << endl;
	cout << *listeTextes.elements[0] << endl;
	cout << "La premiere phrase de la liste 2 est: " << endl;
	cout << *listeTextes2.elements[0] << endl;

	cout << "La deuxieme phrase de la liste originale est: " << endl;
	cout << *listeTextes.elements[1] << endl;
	cout << "La deuxième phrase de la liste 2 est: " << endl;
	cout << *listeTextes2.elements[1] << endl;

	// Détruit et enlève le premier film de la liste (Alien).
	detruireFilm(listeFilms.enSpan()[0]);
	listeFilms.enleverFilm(listeFilms.enSpan()[0]);

	cout << ligneDeSeparation << "Les films sont maintenant:" << endl;
	afficherListeFilms(listeFilms);

	// Pour une couverture avec 0% de lignes non exécutées:
	listeFilms.enleverFilm(nullptr); // Enlever un film qui n'est pas dans la liste (clairement que nullptr n'y est pas).
	//afficherFilmographieActeur(listeFilms, "N'existe pas"); // Afficher les films d'un acteur qui n'existe pas.

	// Détruire tout avant de terminer le programme.
	listeFilms.detruire(true);
}
