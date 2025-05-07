#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Etudiants
{
    char nom[30];
    char prenom[30];
    int CNE;
    float notes[4];
    float moyenne;
    struct Etudiants *suivant;
} Etudiants;

typedef struct PileAnnulation
{
    Etudiants *etudiant;
    struct PileAnnulation *precedent;
} PileAnnulation;

typedef struct FileAttente {
    Etudiants *debut;
    Etudiants *fin;
} FileAttente;

PileAnnulation *pileSuppressions = NULL;
FileAttente fileAttente = {NULL, NULL};

// Function prototypes
void empiler_suppression(Etudiants *etudiant);
Etudiants *depiler_suppression(void);
void liberer_pile(void);
void inscrire_etudiant(Etudiants **liste, FileAttente *file, const char *filename);
void ajouter_file_attente(FileAttente *file, Etudiants *etudiant);
Etudiants* retirer_file_attente(FileAttente *file);
void afficher_file_attente(FileAttente *file);

// DES FUNCTIONS QUI PERMET DE  MODIFIER OU OUVRIR LE FICHIER .TXT

void mettre_a_jour_fichier(Etudiants *tete, const char *filename)
{
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Erreur d'ouverture du fichier pour écriture\n");
        return;
    }

    Etudiants *current = tete;
    while (current != NULL)
    {
        fprintf(file, "%s %s %d %.2f %.2f %.2f %.2f\n",
                current->nom, current->prenom, current->CNE,
                current->notes[0], current->notes[1],
                current->notes[2], current->notes[3]);
        current = current->suivant;
    }
    fclose(file);
}

void lire_fichier_etudiants(Etudiants **tete, const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        printf("Erreur d'ouverture du fichier\n");
        return;
    }

    char nom[30], prenom[30];
    int CNE;
    float notes[4];

    while (fscanf(file, "%29s %29s %d %f %f %f %f",
                  nom, prenom, &CNE,
                  &notes[0], &notes[1], &notes[2], &notes[3]) == 7)
    {
        Etudiants *nouveau = malloc(sizeof(Etudiants));
        if (nouveau == NULL)
        {
            printf("Erreur d'allocation memoire\n");
            fclose(file);
            return;
        }

        strcpy(nouveau->nom, nom);
        strcpy(nouveau->prenom, prenom);
        nouveau->CNE = CNE;
        memcpy(nouveau->notes, notes, sizeof(notes));

        nouveau->moyenne = 0;
        for (int i = 0; i < 4; i++)
        {
            nouveau->moyenne += nouveau->notes[i];
        }
        nouveau->moyenne /= 4;

        nouveau->suivant = *tete;
        *tete = nouveau;
    }
    fclose(file);
}
// DES FUNCTIONS QUI PERMET DE AFFICHER UN ETUDIANT OU TOUS

void afficher_etudiant(Etudiants *etudiant)
{
    printf("================================================\n");
    printf("Nom : %s\n", etudiant->nom);
    printf("Prénom : %s\n", etudiant->prenom);
    printf("CNE : %d\n", etudiant->CNE);
    printf("Notes :\n");
    for (int j = 0; j < 4; j++)
    {
        printf("- Note %d : %.2f\n", j + 1, etudiant->notes[j]);
    }
    printf("Moyenne : %.2f\n", etudiant->moyenne);
}

void afficher_les_etudiants(Etudiants *tete)
{
    Etudiants *current = tete;
    int i = 1;
    while (current != NULL)
    {
        printf("\nEtudiant %d :", i++);
        afficher_etudiant(current);
        current = current->suivant;
    }
}

// FUNCTION DE CHERCHER UN ETUDIANT
void chercher_un_etudiant(Etudiants *tete)
{
    if (tete == NULL)
    {
        printf("La liste est vide!\n");
        return;
    }

    int CNE;
    printf("Entrez le CNE que vous cherchez : ");
    if (scanf("%d", &CNE) != 1)
    {
        printf("CNE invalide\n");
        while (getchar() != '\n')
            ;
        return;
    }

    Etudiants *current = tete;
    int is_found = 0;

    while (current != NULL)
    {
        if (current->CNE == CNE)
        {
            is_found = 1;
            printf("\nÉtudiant trouvé :\n");
            afficher_etudiant(current);
            break;
        }
        current = current->suivant;
    }

    if (!is_found)
    {
        printf("Aucun étudiant trouvé avec le CNE %d\n", CNE);
    }
}
//  DES FUNCTIONS D'AJOUTER OU SUPPRIMER UN ETUDIANT / ANNULER LA SUPPRESSION
Etudiants *annuler_derniere_suppression(Etudiants *tete, const char *filename)
{
    Etudiants *etudiant = depiler_suppression();
    if (etudiant == NULL)
    {
        printf("Aucune suppression à annuler\n");
        return tete;
    }
    etudiant->suivant = tete;

    printf("Suppression annulée pour l'étudiant %s %s\n",
           etudiant->prenom, etudiant->nom);

    mettre_a_jour_fichier(tete, filename);
    return etudiant;
}

Etudiants *supprimer_un_etudiant(Etudiants *tete, const char *filename)
{
    if (tete == NULL)
    {
        printf("La liste est vide!\n");
        return NULL;
    }

    int CNE;
    printf("Entrez le CNE de l'étudiant à supprimer : ");
    if (scanf("%d", &CNE) != 1)
    {
        printf("CNE invalide\n");
        while (getchar() != '\n')
            ;
        return tete;
    }

    Etudiants *current = tete;
    Etudiants *previous = NULL;
    int found = 0;

    while (current != NULL)
    {
        empiler_suppression(current);
        if (current->CNE == CNE)
        {
            found = 1;
            if (previous == NULL)
            {
                Etudiants *new_head = current->suivant;
                free(current);
                printf("Étudiant avec CNE %d supprimé.\n", CNE);
                mettre_a_jour_fichier(new_head, filename);
                return new_head;
            }
            else
            {
                previous->suivant = current->suivant;
                free(current);
                printf("Étudiant avec CNE %d supprimé.\n", CNE);
                mettre_a_jour_fichier(tete, filename);
                return tete;
            }
        }
        previous = current;
        current = current->suivant;
    }

    if (!found)
    {
        printf("Aucun étudiant trouvé avec le CNE %d.\n", CNE);
    }
    return tete;
}

void ajouter_etudiant(Etudiants **tete, const char *filename)
{
    Etudiants *nouveau = malloc(sizeof(Etudiants));
    if (nouveau == NULL)
    {
        printf("Erreur d'allocation memoire\n");
        return;
    }

    printf("Entrez le nom de l'étudiant : ");
    scanf("%29s", nouveau->nom);
    printf("Entrez le prénom de l'étudiant : ");
    scanf("%29s", nouveau->prenom);
    printf("Entrez le CNE de l'étudiant : ");
    scanf("%d", &nouveau->CNE);

    nouveau->moyenne = 0;
    for (int j = 0; j < 4; j++)
    {
        printf("Entrez la note %d : ", j + 1);
        scanf("%f", &nouveau->notes[j]);
        nouveau->moyenne += nouveau->notes[j];
    }
    nouveau->moyenne /= 4;

    nouveau->suivant = *tete;
    *tete = nouveau;
    mettre_a_jour_fichier(*tete, filename);
}

void trier_les_etudiants_par_moyenne(Etudiants **tete, const char *filename)
{
    if (*tete == NULL)
    {
        printf("La liste est vide!\n");
        return;
    }

    int ordre;
    printf("Trier les moyennes : 1 (croissant) ou 0 (décroissant) ? ");
    scanf("%d", &ordre);

    int trie;
    Etudiants *current;
    do
    {
        trie = 0;
        current = *tete;
        Etudiants *previous = NULL;

        while (current != NULL && current->suivant != NULL)
        {
            int condition = ordre ? (current->moyenne > current->suivant->moyenne)
                                  : (current->moyenne < current->suivant->moyenne);

            if (condition)
            {

                Etudiants *temp = current->suivant;
                current->suivant = temp->suivant;
                temp->suivant = current;

                if (previous == NULL)
                {
                    *tete = temp;
                }
                else
                {
                    previous->suivant = temp;
                }

                previous = temp;
                trie = 1;
            }
            else
            {
                previous = current;
                current = current->suivant;
            }
        }
    } while (trie);

    printf("Étudiants triés par moyenne (%s) :\n", ordre ? "croissante" : "décroissante");

    current = *tete;
    int i = 1;
    while (current != NULL)
    {
        printf("---------------------");
        printf("\nEtudiant %d :\n", i++);
        printf(" - Nom : %s\n", current->nom);
        printf(" - Prénom : %s\n", current->prenom);
        printf(" - Moyenne : %.2f\n", current->moyenne);
        current = current->suivant;
    }

    mettre_a_jour_fichier(*tete, filename);
}

void empiler_suppression(Etudiants *etudiant)
{
    PileAnnulation *nouveau = malloc(sizeof(PileAnnulation));
    if (nouveau == NULL)
    {
        printf("Erreur d'allocation mémoire pour la pile\n");
        return;
    }

    // Allouer une copie de l'étudiant
    Etudiants *copie = malloc(sizeof(Etudiants));
    if (copie == NULL)
    {
        free(nouveau);
        printf("Erreur d'allocation mémoire pour la copie\n");
        return;
    }

    *copie = *etudiant; // Copie superficielle
    copie->suivant = NULL;

    nouveau->etudiant = copie;
    nouveau->precedent = pileSuppressions;
    pileSuppressions = nouveau;
}

Etudiants *depiler_suppression()
{
    if (pileSuppressions == NULL)
    {
        return NULL;
    }

    PileAnnulation *temp = pileSuppressions;
    Etudiants *etudiant = temp->etudiant;
    pileSuppressions = temp->precedent;

    free(temp);
    return etudiant;
}

void liberer_pile()
{
    while (pileSuppressions != NULL)
    {
        PileAnnulation *temp = pileSuppressions;
        pileSuppressions = temp->precedent;
        free(temp->etudiant);
        free(temp);
    }
}
void inscrire_etudiant(Etudiants **liste, FileAttente *file, const char *filename) {
    Etudiants *etudiant = retirer_file_attente(file);
    if (!etudiant) {
        printf("Aucun étudiant dans la file d'attente\n");
        return;
    }

    // Insert at head
    etudiant->suivant = *liste;
    *liste = etudiant;
    
    mettre_a_jour_fichier(*liste, filename);
    printf("%s %s inscrit avec succès\n", etudiant->nom, etudiant->prenom);
}
void ajouter_file_attente(FileAttente *file, Etudiants *etudiant) {
    Etudiants *nouveau = malloc(sizeof(Etudiants));
    if (!nouveau) {
        printf("Erreur mémoire\n");
        return;
    }
    
    *nouveau = *etudiant;
    nouveau->suivant = NULL;

    if (file->fin == NULL) {
        file->debut = file->fin = nouveau;
    } else {
        file->fin->suivant = nouveau;
        file->fin = nouveau;
    }
    printf("%s %s ajouté à la file d'attente\n", nouveau->nom, nouveau->prenom);
}

Etudiants* retirer_file_attente(FileAttente *file) {
    if (file->debut == NULL) return NULL;

    Etudiants *temp = file->debut;
    file->debut = file->debut->suivant;

    if (file->debut == NULL) {
        file->fin = NULL;
    }

    return temp;
}

void afficher_file_attente(FileAttente *file) {
    if (file->debut == NULL) {
        printf("La file d'attente est vide\n");
        return;
    }

    Etudiants *current = file->debut;
    int position = 1;
    while (current) {
        printf("%d. %s %s (CNE: %d)\n", 
               position++, current->nom, current->prenom, current->CNE);
        current = current->suivant;
    }
}

int main()
{
    Etudiants *liste = NULL;
    const char *filename = "etudiants.txt";
    lire_fichier_etudiants(&liste, filename);

    int choix;
    do
    {
        printf("\nMenu:\n");
        printf("1. Ajouter un étudiant\n");
        printf("4. Afficher les étudiants inscrits\n");
        printf("3. Chercher un étudiant\n");
        printf("4. Supprimer un étudiant\n");
        printf("5. Trier les étudiants par moyenne\n");
        printf("6. Annuler la dernière suppression\n");
        printf("7. Ajouter un etudiant à la file d'attente\n");
        printf("8. Inscrire les etudiants depuis la file\n");
        printf("9. Afficher la file d'attente\n");
        printf("0. Quitter\n");
        printf("Choix: ");
        scanf("%d", &choix);

        switch (choix)
        {
        case 1:
            ajouter_etudiant(&liste, filename);
            break;
        case 2:
            afficher_les_etudiants(liste);
            break;
        case 3:
            chercher_un_etudiant(liste);
            break;
        case 4:
            liste = supprimer_un_etudiant(liste, filename);
            break;
        case 5:
            trier_les_etudiants_par_moyenne(&liste, filename);
            break;
        case 6:
            liste = annuler_derniere_suppression(liste, filename);
            break;
        case 7:
        {
            Etudiants temp;
            printf("Nom: ");
            scanf("%29s", temp.nom);
            printf("Prénom: ");
            scanf("%29s", temp.prenom);
            printf("CNE: ");
            scanf("%d", &temp.CNE);
            ajouter_file_attente(&fileAttente, &temp);
            break;
        }
        case 8:
            inscrire_etudiant(&liste, &fileAttente, filename);
            break;
        case 9:
            afficher_file_attente(&fileAttente);
            break;
        case 0:
            liberer_pile();
            printf("Au revoir!\n");
            break;
        default:
            printf("Choix invalide!\n");
        }
    } while (choix != 0);

    Etudiants *temp;
    while (liste != NULL)
    {
        temp = liste;
        liste = liste->suivant;
        free(temp);
    }

    while (fileAttente.debut)
    {
        Etudiants *temp = fileAttente.debut;
        fileAttente.debut = fileAttente.debut->suivant;
        free(temp);
    }

    return 0;
}