#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
    #ifdef BUILD_DLL
        #define EXPORT __declspec(dllexport)
    #else
        #define EXPORT __declspec(dllimport)
    #endif
#else
    #define EXPORT
#endif

#define MAX_STRING_LENGTH 50
#define MAX_BUFFER_SIZE 1024

typedef struct Etudiant {
    char nom[MAX_STRING_LENGTH];
    char prenom[MAX_STRING_LENGTH];
    int CNE;
    float notes[4];
    float moyenne;
    struct Etudiant *suivant;
} Etudiant;

typedef struct PileAnnulation {
    Etudiant *etudiant;
    struct PileAnnulation *precedent;
} PileAnnulation;

typedef struct FileAttente {
    Etudiant *debut;
    Etudiant *fin;
} FileAttente;

typedef struct OperationResult {
    int success;
    char *message;
    Etudiant *etudiant;  // For functions that return a student
    Etudiant *liste;     // For functions that return a list
} OperationResult;

// Global variables
PileAnnulation *pileSuppressions = NULL;
FileAttente fileAttente = {NULL, NULL};

// Utility functions
void calculer_moyenne(Etudiant *etudiant) {
    if (!etudiant) return;
    etudiant->moyenne = 0;
    for (int i = 0; i < 4; i++) {
        etudiant->moyenne += etudiant->notes[i];
    }
    etudiant->moyenne /= 4;
}

void empiler_suppression(Etudiant *etudiant) {
    if (!etudiant) return;
    
    PileAnnulation *nouveau = malloc(sizeof(PileAnnulation));
    if (!nouveau) return;

    Etudiant *copie = malloc(sizeof(Etudiant));
    if (!copie) {
        free(nouveau);
        return;
    }

    *copie = *etudiant;
    copie->suivant = NULL;

    nouveau->etudiant = copie;
    nouveau->precedent = pileSuppressions;
    pileSuppressions = nouveau;
}

Etudiant *depiler_suppression() {
    if (!pileSuppressions) return NULL;

    PileAnnulation *temp = pileSuppressions;
    Etudiant *etudiant = temp->etudiant;
    pileSuppressions = temp->precedent;

    free(temp);
    return etudiant;
}

void liberer_pile() {
    while (pileSuppressions) {
        PileAnnulation *temp = pileSuppressions;
        pileSuppressions = temp->precedent;
        free(temp->etudiant);
        free(temp);
    }
}

// Main functions
EXPORT OperationResult lire_fichier_etudiants(const char *filename) {
    OperationResult result = {0, NULL, NULL, NULL};
    FILE *file = fopen(filename, "r");
    if (!file) {
        result.message = strdup("Erreur d'ouverture du fichier");
        return result;
    }

    char nom[MAX_STRING_LENGTH], prenom[MAX_STRING_LENGTH];
    int CNE;
    float notes[4];
    Etudiant *tete = NULL;

    while (fscanf(file, "%49s %49s %d %f %f %f %f",
                nom, prenom, &CNE, &notes[0], &notes[1], &notes[2], &notes[3]) == 7) {
        // Validate input
        if (CNE <= 0) {
            continue;  // Skip invalid CNE
        }
        
        int valid_notes = 1;
        for (int i = 0; i < 4; i++) {
            if (notes[i] < 0 || notes[i] > 20) {
                valid_notes = 0;
                break;
            }
        }
        if (!valid_notes) {
            continue;  // Skip invalid notes
        }

        Etudiant *nouveau = malloc(sizeof(Etudiant));
        if (!nouveau) {
            fclose(file);
            result.message = strdup("Erreur d'allocation mémoire");
            return result;
        }

        // Clear the strings first
        memset(nouveau->nom, 0, sizeof(nouveau->nom));
        memset(nouveau->prenom, 0, sizeof(nouveau->prenom));

        // Copy strings with proper bounds checking
        strncpy(nouveau->nom, nom, sizeof(nouveau->nom) - 1);
        strncpy(nouveau->prenom, prenom, sizeof(nouveau->prenom) - 1);
        
        nouveau->CNE = CNE;
        memcpy(nouveau->notes, notes, sizeof(notes));
        calculer_moyenne(nouveau);

        nouveau->suivant = tete;
        tete = nouveau;
    }
    fclose(file);

    result.success = 1;
    result.message = strdup("Fichier chargé avec succès");
    result.liste = tete;
    return result;
}

EXPORT OperationResult mettre_a_jour_fichier(Etudiant *tete, const char *filename) {
    OperationResult result = {0, NULL, NULL, NULL};
    FILE *file = fopen(filename, "w");
    if (!file) {
        result.message = strdup("Erreur d'ouverture du fichier");
        return result;
    }

    Etudiant *current = tete;
    while (current) {
        fprintf(file, "%s %s %d %.2f %.2f %.2f %.2f\n",
                current->nom, current->prenom, current->CNE,
                current->notes[0], current->notes[1],
                current->notes[2], current->notes[3]);
        current = current->suivant;
    }
    fclose(file);

    result.success = 1;
    result.message = strdup("Fichier mis à jour avec succès");
    return result;
}

EXPORT char* afficher_etudiant(Etudiant *etudiant) {
    if (!etudiant) return NULL;
    
    char *buffer = malloc(MAX_BUFFER_SIZE);
    if (!buffer) return NULL;
    
    // Initialize buffer with zeros
    memset(buffer, 0, MAX_BUFFER_SIZE);
    
    // Use safer string formatting
    snprintf(buffer, MAX_BUFFER_SIZE, 
            "Nom: %s\nPrenom: %s\nCNE: %d\nNotes: %.2f, %.2f, %.2f, %.2f\nMoyenne: %.2f",
            etudiant->nom, etudiant->prenom, etudiant->CNE,
            etudiant->notes[0], etudiant->notes[1],
            etudiant->notes[2], etudiant->notes[3],
            etudiant->moyenne);
    
    return buffer;
}

EXPORT char* afficher_liste_etudiants(Etudiant *tete) {
    if (!tete) return strdup("Aucun etudiant dans la liste");
    
    // Calculate needed size
    size_t size = MAX_BUFFER_SIZE;
    Etudiant *current = tete;
    while (current) {
        size += MAX_BUFFER_SIZE;
        current = current->suivant;
    }
    
    char *buffer = malloc(size);
    if (!buffer) return NULL;
    
    // Initialize buffer with zeros
    memset(buffer, 0, size);
    
    buffer[0] = '\0';
    current = tete;
    int i = 1;
    while (current) {
        char *etudiant_str = afficher_etudiant(current);
        if (etudiant_str) {
            char temp[MAX_BUFFER_SIZE];
            snprintf(temp, sizeof(temp), "\nEtudiant %d:\n%s\n----------------\n", i++, etudiant_str);
            strncat(buffer, temp, size - strlen(buffer) - 1);
            free(etudiant_str);
        }
        current = current->suivant;
    }
    
    return buffer;
}

EXPORT OperationResult ajouter_etudiant(Etudiant **tete, const char *filename, 
                                      const char *nom, const char *prenom, 
                                      int CNE, const float *notes) {
    OperationResult result = {0, NULL, NULL, NULL};
    
    // Validate input
    if (!nom || !prenom || CNE <= 0) {
        result.message = strdup("Données invalides");
        return result;
    }
    
    for (int i = 0; i < 4; i++) {
        if (notes[i] < 0 || notes[i] > 20) {
            result.message = strdup("Notes invalides");
            return result;
        }
    }
    
    // Check if CNE already exists
    Etudiant *current = *tete;
    while (current) {
        if (current->CNE == CNE) {
            result.message = strdup("Un étudiant avec ce CNE existe déjà");
            return result;
        }
        current = current->suivant;
    }
    
    Etudiant *nouveau = malloc(sizeof(Etudiant));
    if (!nouveau) {
        result.message = strdup("Erreur d'allocation mémoire");
        return result;
    }

    strncpy(nouveau->nom, nom, MAX_STRING_LENGTH - 1);
    nouveau->nom[MAX_STRING_LENGTH - 1] = '\0';
    strncpy(nouveau->prenom, prenom, MAX_STRING_LENGTH - 1);
    nouveau->prenom[MAX_STRING_LENGTH - 1] = '\0';
    nouveau->CNE = CNE;
    memcpy(nouveau->notes, notes, sizeof(float)*4);
    calculer_moyenne(nouveau);
    nouveau->suivant = *tete;
    *tete = nouveau;

    OperationResult file_result = mettre_a_jour_fichier(*tete, filename);
    if (!file_result.success) {
        free(file_result.message);
        result.message = strdup("Étudiant ajouté mais erreur lors de la sauvegarde");
        return result;
    }
    free(file_result.message);

    result.success = 1;
    result.message = strdup("Étudiant ajouté avec succès");
    result.etudiant = nouveau;
    return result;
}

EXPORT OperationResult supprimer_etudiant(Etudiant **tete, const char *filename, int CNE) {
    OperationResult result = {0, NULL, NULL, NULL};
    
    if (!*tete) {
        result.message = strdup("Liste vide");
        return result;
    }
    
    Etudiant *current = *tete;
    Etudiant *previous = NULL;
    
    while (current) {
        if (current->CNE == CNE) {
            if (previous) {
                previous->suivant = current->suivant;
            } else {
                *tete = current->suivant;
            }
            
            empiler_suppression(current);
            
            OperationResult file_result = mettre_a_jour_fichier(*tete, filename);
            if (!file_result.success) {
                free(file_result.message);
                result.message = strdup("Étudiant supprimé mais erreur lors de la sauvegarde");
                return result;
            }
            free(file_result.message);
            
            result.success = 1;
            result.message = strdup("Étudiant supprimé avec succès");
            return result;
        }
        previous = current;
        current = current->suivant;
    }
    
    result.message = strdup("Étudiant non trouvé");
    return result;
}

EXPORT OperationResult annuler_derniere_suppression(Etudiant **tete, const char *filename) {
    OperationResult result = {0, NULL, NULL, NULL};
    
    Etudiant *etudiant = depiler_suppression();
    if (!etudiant) {
        result.message = strdup("Aucune suppression à annuler");
        return result;
    }
    
    etudiant->suivant = *tete;
    *tete = etudiant;
    
    OperationResult file_result = mettre_a_jour_fichier(*tete, filename);
    if (!file_result.success) {
        free(file_result.message);
        result.message = strdup("Suppression annulée mais erreur lors de la sauvegarde");
        return result;
    }
    free(file_result.message);
    
    result.success = 1;
    result.message = strdup("Dernière suppression annulée avec succès");
    return result;
}

EXPORT OperationResult chercher_etudiant(Etudiant *tete, int CNE) {
    OperationResult result = {0, NULL, NULL, NULL};
    
    Etudiant *current = tete;
    while (current) {
        if (current->CNE == CNE) {
            result.success = 1;
            result.message = strdup("Étudiant trouvé");
            result.etudiant = current;
            return result;
        }
        current = current->suivant;
    }
    
    result.message = strdup("Étudiant non trouvé");
    return result;
}

EXPORT OperationResult trier_etudiants_moyenne(Etudiant **tete, const char *filename, int ordre) {
    OperationResult result = {0, NULL, NULL, NULL};
    
    if (!*tete || !(*tete)->suivant) {
        result.success = 1;
        result.message = strdup("Liste déjà triée");
        return result;
    }
    
    int echange;
    Etudiant *ptr1;
    Etudiant *lptr = NULL;
    
    do {
        echange = 0;
        ptr1 = *tete;
        
        while (ptr1->suivant != lptr) {
            if ((ordre && ptr1->moyenne > ptr1->suivant->moyenne) ||
                (!ordre && ptr1->moyenne < ptr1->suivant->moyenne)) {
                // Swap data
                Etudiant temp = *ptr1;
                *ptr1 = *(ptr1->suivant);
                *(ptr1->suivant) = temp;
                
                // Keep the next pointers correct
                Etudiant *temp_next = ptr1->suivant->suivant;
                ptr1->suivant->suivant = ptr1->suivant;
                ptr1->suivant = temp_next;
                
                echange = 1;
            }
            ptr1 = ptr1->suivant;
        }
        lptr = ptr1;
    } while (echange);
    
    OperationResult file_result = mettre_a_jour_fichier(*tete, filename);
    if (!file_result.success) {
        free(file_result.message);
        result.message = strdup("Liste triée mais erreur lors de la sauvegarde");
        return result;
    }
    free(file_result.message);
    
    result.success = 1;
    result.message = strdup("Liste triée avec succès");
    return result;
}

EXPORT OperationResult ajouter_file_attente(const char *nom, const char *prenom, 
                                          int CNE, const float *notes) {
    OperationResult result = {0, NULL, NULL, NULL};
    
    // Validate input
    if (!nom || !prenom || CNE <= 0) {
        result.message = strdup("Données invalides");
        return result;
    }
    
    for (int i = 0; i < 4; i++) {
        if (notes[i] < 0 || notes[i] > 20) {
            result.message = strdup("Notes invalides");
            return result;
        }
    }
    
    Etudiant *nouveau = malloc(sizeof(Etudiant));
    if (!nouveau) {
        result.message = strdup("Erreur d'allocation mémoire");
        return result;
    }
    
    strncpy(nouveau->nom, nom, MAX_STRING_LENGTH - 1);
    nouveau->nom[MAX_STRING_LENGTH - 1] = '\0';
    strncpy(nouveau->prenom, prenom, MAX_STRING_LENGTH - 1);
    nouveau->prenom[MAX_STRING_LENGTH - 1] = '\0';
    nouveau->CNE = CNE;
    memcpy(nouveau->notes, notes, sizeof(float)*4);
    calculer_moyenne(nouveau);
    nouveau->suivant = NULL;
    
    if (!fileAttente.debut) {
        fileAttente.debut = nouveau;
        fileAttente.fin = nouveau;
    } else {
        fileAttente.fin->suivant = nouveau;
        fileAttente.fin = nouveau;
    }
    
    result.success = 1;
    result.message = strdup("Étudiant ajouté à la file d'attente");
    return result;
}

EXPORT OperationResult retirer_file_attente() {
    OperationResult result = {0, NULL, NULL, NULL};
    
    if (!fileAttente.debut) {
        result.message = strdup("File d'attente vide");
        return result;
    }
    
    Etudiant *temp = fileAttente.debut;
    fileAttente.debut = fileAttente.debut->suivant;
    
    if (!fileAttente.debut) {
        fileAttente.fin = NULL;
    }
    
    free(temp);
    
    result.success = 1;
    result.message = strdup("Étudiant retiré de la file d'attente");
    return result;
}

EXPORT char* afficher_file_attente() {
    if (!fileAttente.debut) {
        return strdup("File d'attente vide");
    }
    
    char *buffer = malloc(MAX_BUFFER_SIZE);
    if (!buffer) return NULL;
    
    // Initialize buffer with zeros
    memset(buffer, 0, MAX_BUFFER_SIZE);
    
    buffer[0] = '\0';
    Etudiant *current = fileAttente.debut;
    int i = 1;
    
    while (current) {
        char temp[MAX_BUFFER_SIZE];
        snprintf(temp, sizeof(temp), 
                "\nÉtudiant %d:\nNom: %s\nPrénom: %s\nCNE: %d\nNotes: %.2f, %.2f, %.2f, %.2f\nMoyenne: %.2f\n----------------\n",
                i++, current->nom, current->prenom, current->CNE,
                current->notes[0], current->notes[1],
                current->notes[2], current->notes[3],
                current->moyenne);
        strncat(buffer, temp, MAX_BUFFER_SIZE - strlen(buffer) - 1);
        current = current->suivant;
    }
    
    return buffer;
}

EXPORT OperationResult inscrire_etudiant_file(Etudiant **tete, const char *filename) {
    OperationResult result = {0, NULL, NULL, NULL};
    
    if (!fileAttente.debut) {
        result.message = strdup("File d'attente vide");
        return result;
    }
    
    Etudiant *etudiant = fileAttente.debut;
    fileAttente.debut = fileAttente.debut->suivant;
    
    if (!fileAttente.debut) {
        fileAttente.fin = NULL;
    }
    
    etudiant->suivant = *tete;
    *tete = etudiant;
    
    OperationResult file_result = mettre_a_jour_fichier(*tete, filename);
    if (!file_result.success) {
        free(file_result.message);
        result.message = strdup("Étudiant inscrit mais erreur lors de la sauvegarde");
        return result;
    }
    free(file_result.message);
    
    result.success = 1;
    result.message = strdup("Étudiant inscrit avec succès");
    return result;
}

EXPORT void liberer_liste(Etudiant *tete) {
    while (tete) {
        Etudiant *temp = tete;
        tete = tete->suivant;
        free(temp);
    }
}

#ifdef __cplusplus
}
#endif