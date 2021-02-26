// Solutionnaire du TD2 INF1015 hiver 2021
// Par Francois-R.Boyer@PolyMtl.ca
#pragma once
// Structures mémoires pour une collection de films.
#include <memory>
#include <string>
#include "gsl/span"
using gsl::span;
using namespace std;
struct Film; struct Acteur; // Permet d'utiliser les types alors qu'ils seront défini après.

class ListeFilms {
public:
	ListeFilms() = default;
	void ajouterFilm(Film* film);
	void enleverFilm(const Film* film);
	shared_ptr<Acteur> donnerActeur(int indexFilm, int indexActeur);
	Acteur* trouverActeur(const std::string& nomActeur) const;
	span<Film*> enSpan() const;
	int size() const { return nElements; }
	void detruire(bool possedeLesFilms = false);
	Film& operator[](int index);
	
private:
	void changeDimension(int nouvelleCapacite);

	int capacite = 0, nElements = 0;
	Film** elements = nullptr; // Pointeur vers un tableau de Film*, chaque Film* pointant vers un Film.

	
};


struct ListeActeurs {
	ListeActeurs();
	ListeActeurs(int nElements);
	int capacite, nElements;
	std::unique_ptr<std::shared_ptr<Acteur> []> elements; // Pointeur vers un tableau de Acteur*, chaque Acteur* pointant vers un Acteur.
};

ListeActeurs::ListeActeurs() {
	this->capacite = 1;
	this->nElements = 0;
	this->elements = make_unique<std::shared_ptr<Acteur>[]>(1);
}
ListeActeurs::ListeActeurs(int nombreElements) {
	this->capacite = nombreElements;
	this->nElements = nombreElements;
	this->elements = make_unique<std::shared_ptr<Acteur>[]>(nombreElements);
}

struct Film
{
	Film(int nombreActeurs);
	Film(Film&);
	std::string titre, realisateur; // Titre et nom du réalisateur (on suppose qu'il n'y a qu'un réalisateur).
	int anneeSortie, recette; // Année de sortie et recette globale du film en millions de dollars
	ListeActeurs acteurs;
	//Film& operator=(Film& nouveauFilm);

};
Film::Film(int nombreActeurs) {
	this->acteurs = ListeActeurs(nombreActeurs);
}

struct Acteur
{
	std::string nom; int anneeNaissance; char sexe;
	void changerNom(string nouveauNom);
	//ListeFilms joueDans;
};
void Acteur::changerNom(string nouveauNom) {
	this->nom = nouveauNom;
}
shared_ptr<Acteur> ListeFilms::donnerActeur(int indexFilm, int indexActeur) {
	return this->elements[indexFilm]->acteurs.elements[indexActeur];
}
