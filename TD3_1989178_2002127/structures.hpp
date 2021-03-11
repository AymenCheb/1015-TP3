// Solutionnaire du TD2 INF1015 hiver 2021
// Par Francois-R.Boyer@PolyMtl.ca
#pragma once
// Structures mémoires pour une collection de films.
#include <iostream>
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
	Film& operator[](int index) {
		return *this->elements[index];
	};
	template <typename PredicatUnaire>
	Film* chercherFilm(const PredicatUnaire& critere) {
		for (int i = 0; i < this->nElements; i++)
		{
			if (critere(this->elements[i])) {
				cout << "Un film satisfaisant le critere fournit a ete trouve: " << '\n';
				return this->elements[i];
			}
				
		}
		cout << "Aucun film satisfaisant le critere n'a ete trouve " << '\n';
		return nullptr;
	}
	
private:
	void changeDimension(int nouvelleCapacite);

	int capacite = 0, nElements = 0;
	Film** elements = nullptr; // Pointeur vers un tableau de Film*, chaque Film* pointant vers un Film.

	
};


template <typename typeDeListe>
class Liste {
public:
	int capacite, nElements;
	std::unique_ptr<std::shared_ptr<typeDeListe>[]> elements; // Pointeur vers un tableau de Acteur*, chaque Acteur* pointant vers un Acteur.
	Liste() {
		this->capacite = 1;
		this->nElements = 0;
		this->elements = make_unique<std::shared_ptr<typeDeListe>[]>(1);
	};
	Liste(int nElements) {
		this->capacite = nElements;
		this->nElements = nElements;
		this->elements = make_unique<std::shared_ptr<typeDeListe>[]>(nElements);
	};
	void copyListe(const Liste<typeDeListe>& nouvelleListe) {
		this->elements = make_unique<std::shared_ptr<typeDeListe>[]>(nouvelleListe.nElements);
		this->nElements = nouvelleListe.nElements;
		this->capacite = nouvelleListe.capacite;
		for (int i = 0; i < this->nElements; i++)
		{
			this->elements[i] = nouvelleListe.elements[i];
		};
	}
	Liste(const Liste<typeDeListe>& nouvelleListe) {
		this->copyListe(nouvelleListe);
	}
	Liste<typeDeListe> operator=(const Liste<typeDeListe>& nouvelleListe) {
		this->copyListe(nouvelleListe);
		return *this;
	}
};
using ListeActeurs = Liste<Acteur>;
struct Point { double x, y; };
struct Film
{
	Film(int nombreActeurs) {
		this->acteurs = ListeActeurs(nombreActeurs);
	}
	Film(const Film& nouveauFilm) {
		this->acteurs = ListeActeurs(nouveauFilm.acteurs.nElements);
		acteurs.capacite = nouveauFilm.acteurs.capacite;
		this->acteurs = nouveauFilm.acteurs;
		this->anneeSortie = nouveauFilm.anneeSortie;
		this->recette = nouveauFilm.recette;
		this->realisateur = nouveauFilm.realisateur;
		this->titre = nouveauFilm.titre;
	}
	std::string titre, realisateur; // Titre et nom du réalisateur (on suppose qu'il n'y a qu'un réalisateur).
	int anneeSortie, recette; // Année de sortie et recette globale du film en millions de dollars
	ListeActeurs acteurs;

};

struct Acteur
{
	std::string nom; int anneeNaissance; char sexe;
	void changerNom(string nouveauNom);
};
void Acteur::changerNom(string nouveauNom) {
	this->nom = nouveauNom;
}
shared_ptr<Acteur> ListeFilms::donnerActeur(int indexFilm, int indexActeur) {
	return this->elements[indexFilm]->acteurs.elements[indexActeur];
}

