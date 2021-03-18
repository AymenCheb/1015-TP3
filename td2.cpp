﻿// Solutionnaire du TD3 INF1015 hiver 2021
// Par Francois-R.Boyer@PolyMtl.ca

#pragma region "Includes"//{
#define _CRT_SECURE_NO_WARNINGS // On permet d'utiliser les fonctions de copies de chaînes qui sont considérées non sécuritaires.
#include "structures.hpp"      // Structures de données pour la collection de films en mémoire.

#include "bibliotheque_cours.hpp"
#include "verification_allocation.hpp" // Nos fonctions pour le rapport de fuites de mémoire.

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <limits>
#include <algorithm>
#include <sstream>
#include "cppitertools/range.hpp"
#include "gsl/span"
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

// Fonctions pour ajouter un Film à une ListeFilms.
//[
void ListeFilms::changeDimension(int nouvelleCapacite)
{
	Film** nouvelleListe = new Film*[nouvelleCapacite];
	
	if (elements != nullptr) {  // Noter que ce test n'est pas nécessaire puique nElements_ sera zéro si elements_ est nul, donc la boucle ne tentera pas de faire de copie, et on a le droit de faire delete sur un pointeur nul (ça ne fait rien).
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

//NOTE: Doit retourner un Acteur modifiable, sinon on ne peut pas l'utiliser pour modifier l'acteur tel que demandé dans le main, et on ne veut pas faire écrire deux versions.
shared_ptr<Acteur> ListeFilms::trouverActeur(const string& nomActeur) const
{
	for (const Film* film : enSpan()) {
		for (const shared_ptr<Acteur>& acteur : film->acteurs.enSpan()) {
			if (acteur->nom == nomActeur)
				return acteur;
		}
	}
	return nullptr;
}
//]

// Les fonctions pour lire le fichier et créer/allouer une ListeFilms.

shared_ptr<Acteur> lireActeur(istream& fichier, const ListeFilms& listeFilms)
{
	Acteur acteur = {};
	acteur.nom            = lireString(fichier);
	acteur.anneeNaissance = lireUint16 (fichier);
	acteur.sexe           = lireUint8  (fichier);

	shared_ptr<Acteur> acteurExistant = listeFilms.trouverActeur(acteur.nom);
	if (acteurExistant != nullptr)
		return acteurExistant;
	else {
		cout << "Création Acteur " << acteur.nom << endl;
		return make_shared<Acteur>(move(acteur));  // Le move n'est pas nécessaire mais permet de transférer le texte du nom sans le copier.
	}
}

Film* lireFilm(istream& fichier, ListeFilms& listeFilms)
{
	Film* film = new Film;
	film->titre       = lireString(fichier);
	film->realisateur = lireString(fichier);
	film->anneeSortie = lireUint16 (fichier);
	film->recette     = lireUint16 (fichier);
	auto nActeurs = lireUint8 (fichier);
	film->acteurs = ListeActeurs(nActeurs);  // On n'a pas fait de méthode pour changer la taille d'allocation, seulement un constructeur qui prend la capacité.  Pour que cette affectation fonctionne, il faut s'assurer qu'on a un operator= de move pour ListeActeurs.
	cout << "Création Film " << film->titre << endl;

	for ([[maybe_unused]] auto i : range(nActeurs)) {  // On peut aussi mettre nElements_ avant et faire un span, comme on le faisait au TD précédent.
		film->acteurs.ajouter(lireActeur(fichier, listeFilms));
	}

	return film;
}

// Fonction pour ajouter des livres dans un vecteur contenant des shared_ptr<Item>
void ajouterLivres(vector<shared_ptr<Item>> &bibliotheque, string nomFichier){
	ifstream fichierLivre(nomFichier);
	const int nombreDeColonnes = 5; // 5 est le nombre de colonnes que contient le fichier texte
	string ligne;
	string tempString;
	string oldString;
	string infosLivres[nombreDeColonnes]; // Ce tableau vide servira à copier les informations nécessaires pour créer les livres
	while (true) // Boucle while pour itérer sur tous les films. La condition de break sera toujours atteinte
	{
		 shared_ptr<Livre> livre = make_shared<Livre>(); // Crée une variable shared_ptr<Livre> pour chaque nouveau livre
		for (int i = 0; i < 5; i++) // Récupère les informations du fichier texte pour un livre
		{
			oldString = tempString;
			fichierLivre >> quoted(tempString);
			infosLivres[i] = tempString;
		}
		if (oldString == tempString) // Si la fin du fichier texte est atteinte, on continue de récupérer le dernier élement avec quoted, ce qui implique oldString == tempString
		{
			break;
		}
		livre->titre = infosLivres[0];
		livre->anneeSortie = stoi(infosLivres[1]);
		livre->Auteur = infosLivres[2];
		livre->MillionsDeCopiesVendues = stoi(infosLivres[3]);
		livre->nombreDePages = stoi(infosLivres[4]);
		bibliotheque.push_back(livre);
	}
	
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

// Fonction pour détruire une ListeFilms et tous les films qu'elle contient.
//[
//NOTE: La bonne manière serait que la liste sache si elle possède, plutôt qu'on le dise au moment de la destruction, et que ceci soit le destructeur.  Mais ça aurait complexifié le TD2 de demander une solution de ce genre, d'où le fait qu'on a dit de le mettre dans une méthode.
void ListeFilms::detruire(bool possedeLesFilms)
{
	if (possedeLesFilms)
		for (Film* film : enSpan())
			delete film;
	delete[] elements;
}
//]

// Pour que l'affichage de Film fonctionne avec <<, il faut aussi modifier l'affichage de l'acteur pour avoir un ostream; l'énoncé ne demande pas que ce soit un opérateur, mais tant qu'à y être...
ostream& operator<< (ostream& os, const Acteur& acteur)
{
	return os << "  " << acteur.nom << ", " << acteur.anneeNaissance << " " << acteur.sexe << endl;
}
ostream& operator<< (ostream& os, const Item& item) {
	item.afficher();
	return os << endl;
}


// Pas demandé dans l'énoncé de tout mettre les affichages avec surcharge, mais pourquoi pas.
ostream& operator<< (ostream& os, const ListeFilms& listeFilms)
{
	static const string ligneDeSeparation = //[
		"\033[32m────────────────────────────────────────\033[0m\n";
	os << ligneDeSeparation;
	for (const Film* film : listeFilms.enSpan()) {
		os << *film << ligneDeSeparation;
	}
	return os;
}
// Constructeur de FilmLivre par copie
FilmLivre::FilmLivre(const Film& film, const Livre& livre): Livre(livre), Film(film) {
	this->titre = film.titre;
	this->anneeSortie = film.anneeSortie;
}

// Constucteur par défaut de la classe Livre
Livre::Livre() {
	Auteur = "";
	anneeSortie = 0;
	nombreDePages = 0;
	MillionsDeCopiesVendues = 0;
}
// Constructeur par copie de la classe Livre
Livre::Livre(const Livre& autreLivre) {
	this->Auteur = autreLivre.Auteur;
	this->MillionsDeCopiesVendues = autreLivre.MillionsDeCopiesVendues;
	this->nombreDePages = autreLivre.nombreDePages;
}
// Fonction pour afficher FilmLivre
void FilmLivre::afficher() const {
	Film::afficher();
	cout << "Auteur: " << Auteur << '\n';
	cout << "Copies vendues: " << MillionsDeCopiesVendues << "M \n";
	cout << "Nombre de pages: " << nombreDePages << '\n';
}
// Fonction pour afficher Item
void Item::afficher() const {
	cout << " Titre: " << titre << endl;
	cout << " Annee de sortie: " << anneeSortie << endl;
}
// Fonction pour afficher Livre
void Livre::afficher() const {
	Item::afficher();
	cout << "Auteur: " << Auteur << '\n';
	cout << "Copies vendues: " << MillionsDeCopiesVendues << "M \n";
	cout << "Nombre de pages: " << nombreDePages << '\n';
}
// Fonction pour afficher Film
void Film::afficher() const {
	Item::afficher();
	cout << "  Réalisateur: " << this->realisateur << '\n';
	cout << "  Recette: " << this->recette << "M$" << '\n';

	cout << "Acteurs:" << '\n';
	for (const shared_ptr<Acteur>& acteur : this->acteurs.enSpan())
		cout << *acteur;
}
// Afficher la liste d'items
void afficherListeItems(vector<shared_ptr<Item>> bibilotheque) {
	static const string ligneDeSeparation = //[
		"\033[32m────────────────────────────────────────\033[0m\n";
	cout << ligneDeSeparation;
	for (int i = 0; i < bibilotheque.size(); i++)
	{
		cout << *bibilotheque[i] << endl;
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
	// Création bibliothèque 
	vector<shared_ptr<Item>> contenuBibliotheque;
	// Copie des films depuis ListeFilms
	for (int i = 0; i < listeFilms.size(); i++)
	{
		Film film = *listeFilms.retournerFilm(i);
		contenuBibliotheque.push_back(make_shared<Film>(film));
	}
	// Appel de la fonction pour ajouter les livres à la bibliothèque
	ajouterLivres(contenuBibliotheque, "livres.txt");

	// Création du FilmLivre the Hobbit
	Film* theHobbitFilm;
	theHobbitFilm = dynamic_cast<Film*>(contenuBibliotheque[4].get()); // Récupére le fim the Hobbit
	Livre* theHobbitLivre = dynamic_cast<Livre*>(contenuBibliotheque[9].get()); // Récupée le livre the Hobbit
	FilmLivre theHobbitFilmLivre(*theHobbitFilm, *theHobbitLivre);
	contenuBibliotheque.push_back(make_shared<FilmLivre>(theHobbitFilmLivre));

	// Afficher le contenu de la bibliothèque
	afficherListeItems(contenuBibliotheque);

	// Détruire tout avant de terminer le programme.
	listeFilms.detruire(true);
}
