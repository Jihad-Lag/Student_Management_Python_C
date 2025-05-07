import tkinter as tk
from tkinter import ttk, messagebox
from ctypes import (CDLL, Structure, c_char, c_int, c_float, POINTER, 
                    byref, c_void_p, cast, create_string_buffer, sizeof, c_char_p)
import os
import locale
import sys

# Set locale to handle special characters
locale.setlocale(locale.LC_ALL, '')

def safe_free(ptr):
    """Safely free memory only if it's a valid pointer"""
    if ptr and isinstance(ptr, (int, c_void_p, c_char_p)):
        try:
            free(cast(ptr, c_void_p))
        except Exception as e:
            print(f"Error freeing pointer: {e}")

# Load the C standard library for free()
try:
    libc = CDLL(None)
    free = libc.free
    free.argtypes = [c_void_p]
    free.restype = None
except Exception as e:
    print(f"Failed to load libc: {e}")
    sys.exit(1)

# Load the student management DLL
try:
    dll_path = os.path.abspath("student_management.dll")
    student_dll = CDLL(dll_path)
except Exception as e:
    messagebox.showerror("Error", f"Failed to load DLL: {e}")
    sys.exit(1)

# Define structures to match the C code
class Etudiant(Structure):
    _fields_ = [
        ("nom", c_char * 50),
        ("prenom", c_char * 50),
        ("CNE", c_int),
        ("notes", c_float * 4),
        ("moyenne", c_float),
        ("suivant", POINTER('Etudiant'))
    ]

class OperationResult(Structure):
    _fields_ = [
        ("success", c_int),
        ("message", c_char_p),
        ("etudiant", POINTER(Etudiant)),
        ("liste", POINTER(Etudiant))
    ]

# Define function prototypes with proper error handling
def setup_dll_functions():
    student_dll.lire_fichier_etudiants.argtypes = [c_char_p]
    student_dll.lire_fichier_etudiants.restype = OperationResult

    student_dll.mettre_a_jour_fichier.argtypes = [POINTER(Etudiant), c_char_p]
    student_dll.mettre_a_jour_fichier.restype = OperationResult

    student_dll.afficher_etudiant.argtypes = [POINTER(Etudiant)]
    student_dll.afficher_etudiant.restype = c_char_p

    student_dll.afficher_liste_etudiants.argtypes = [POINTER(Etudiant)]
    student_dll.afficher_liste_etudiants.restype = c_char_p

    student_dll.ajouter_etudiant.argtypes = [POINTER(POINTER(Etudiant)), c_char_p, c_char_p, c_char_p, c_int, POINTER(c_float)]
    student_dll.ajouter_etudiant.restype = OperationResult

    student_dll.supprimer_etudiant.argtypes = [POINTER(POINTER(Etudiant)), c_char_p, c_int]
    student_dll.supprimer_etudiant.restype = OperationResult

    student_dll.annuler_derniere_suppression.argtypes = [POINTER(POINTER(Etudiant)), c_char_p]
    student_dll.annuler_derniere_suppression.restype = OperationResult

    student_dll.chercher_etudiant.argtypes = [POINTER(Etudiant), c_int]
    student_dll.chercher_etudiant.restype = OperationResult

    student_dll.trier_etudiants_moyenne.argtypes = [POINTER(POINTER(Etudiant)), c_char_p, c_int]
    student_dll.trier_etudiants_moyenne.restype = OperationResult

    student_dll.ajouter_file_attente.argtypes = [c_char_p, c_char_p, c_int, POINTER(c_float)]
    student_dll.ajouter_file_attente.restype = OperationResult

    student_dll.retirer_file_attente.argtypes = []
    student_dll.retirer_file_attente.restype = OperationResult

    student_dll.afficher_file_attente.argtypes = []
    student_dll.afficher_file_attente.restype = c_char_p

    student_dll.inscrire_etudiant_file.argtypes = [POINTER(POINTER(Etudiant)), c_char_p]
    student_dll.inscrire_etudiant_file.restype = OperationResult

    student_dll.liberer_liste.argtypes = [POINTER(Etudiant)]
    student_dll.liberer_liste.restype = None

setup_dll_functions()

class StudentManagementApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Système de gestion des étudiants")
        self.root.geometry("1000x700")
        
        # Initialize variables
        self.current_file = None
        self.student_list = None
        self.current_sort = None  # None = unsorted, 1 = ascending, 0 = descending
        
        # Create UI
        self.setup_ui()
        
        # Try to auto-load a file
        self.auto_load_file()
    
    def setup_ui(self):
        """Set up the main user interface"""
        # Configure styles
        self.style = ttk.Style()
        self.style.configure("TFrame", background="#f0f0f0")
        self.style.configure("TLabel", background="#f0f0f0", foreground="#333333")
        self.style.configure("TButton", background="#4a90e2", foreground="white")
        self.style.configure("TEntry", fieldbackground="white")
        self.style.configure("ActiveSort.TButton", background="#2e7d32", foreground="white")
        
        # Main container
        self.main_container = ttk.Frame(self.root)
        self.main_container.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Notebook for tabs
        self.notebook = ttk.Notebook(self.main_container)
        self.notebook.pack(fill=tk.BOTH, expand=True)
        
        # Create tabs
        self.create_student_tab()
        self.create_queue_tab()
        self.create_view_tab()
        self.create_sort_tab()  # New dedicated sorting tab
    
    def create_sort_tab(self):
        """Create a dedicated tab for sorting operations"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="Tri des Étudiants")
        
        # Sorting controls frame
        sort_control_frame = ttk.Frame(tab)
        sort_control_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Sort buttons
        self.sort_asc_button = ttk.Button(sort_control_frame, text="Tri Croissant", 
                                        command=lambda: self.sort_students(1, display=True))
        self.sort_desc_button = ttk.Button(sort_control_frame, text="Tri Décroissant", 
                                         command=lambda: self.sort_students(0, display=True))
        self.sort_asc_button.pack(side=tk.LEFT, padx=5)
        self.sort_desc_button.pack(side=tk.LEFT, padx=5)
        
        # Sort status
        self.sort_status = ttk.Label(sort_control_frame, text="Non trié")
        self.sort_status.pack(side=tk.LEFT, padx=10)
        
        # Display frame for sorted results
        display_frame = ttk.LabelFrame(tab, text="Résultats du Tri", padding=10)
        display_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Text widget to display sorted students
        self.sorted_display = tk.Text(display_frame, height=30, width=80, state=tk.DISABLED)
        scrollbar = ttk.Scrollbar(display_frame, orient="vertical", command=self.sorted_display.yview)
        self.sorted_display.configure(yscrollcommand=scrollbar.set)
        
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.sorted_display.pack(fill=tk.BOTH, expand=True)
        
        # Status label
        self.sort_tab_status = ttk.Label(tab, text="")
        self.sort_tab_status.pack(pady=5)
    
    def create_student_tab(self):
        """Tab for student operations"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="Opérations étudiants")
        
        # Student form
        form_frame = ttk.LabelFrame(tab, text="Informations pour les étudiants", padding=10)
        form_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Form fields
        ttk.Label(form_frame, text="Prénom:").grid(row=0, column=0, padx=5, pady=5, sticky=tk.W)
        self.first_name_entry = ttk.Entry(form_frame)
        self.first_name_entry.grid(row=0, column=1, padx=5, pady=5)
        
        ttk.Label(form_frame, text="Nom:").grid(row=1, column=0, padx=5, pady=5, sticky=tk.W)
        self.last_name_entry = ttk.Entry(form_frame)
        self.last_name_entry.grid(row=1, column=1, padx=5, pady=5)
        
        ttk.Label(form_frame, text="CNE:").grid(row=2, column=0, padx=5, pady=5, sticky=tk.W)
        self.cne_entry = ttk.Entry(form_frame)
        self.cne_entry.grid(row=2, column=1, padx=5, pady=5)
        
        ttk.Label(form_frame, text="Notes:").grid(row=3, column=0, padx=5, pady=5, sticky=tk.W)
        self.note_entries = []
        for i in range(4):
            entry = ttk.Entry(form_frame, width=5)
            entry.grid(row=3, column=1+i, padx=2, pady=5)
            self.note_entries.append(entry)
        
        # Buttons
        button_frame = ttk.Frame(tab)
        button_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(button_frame, text="Ajouter Étudiant", command=self.add_student).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Supprimer Étudiant", command=self.delete_student).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Rechercher Étudiant", command=self.search_student).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Annuler Suppression", command=self.undo_delete).pack(side=tk.LEFT, padx=5)
        
        # Status
        self.student_status = ttk.Label(tab, text="")
        self.student_status.pack(pady=5)
        self.student_status.config(foreground="green")
    
    def create_queue_tab(self):
        """Tab for queue operations"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="File d'Attente")
        
        # Queue form
        form_frame = ttk.LabelFrame(tab, text="Ajouter à la File", padding=10)
        form_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Form fields
        ttk.Label(form_frame, text="Prénom:").grid(row=0, column=0, padx=5, pady=5, sticky=tk.W)
        self.queue_first_name = ttk.Entry(form_frame)
        self.queue_first_name.grid(row=0, column=1, padx=5, pady=5)
        
        ttk.Label(form_frame, text="Nom").grid(row=1, column=0, padx=5, pady=5, sticky=tk.W)
        self.queue_last_name = ttk.Entry(form_frame)
        self.queue_last_name.grid(row=1, column=1, padx=5, pady=5)
        
        ttk.Label(form_frame, text="CNE:").grid(row=2, column=0, padx=5, pady=5, sticky=tk.W)
        self.queue_cne = ttk.Entry(form_frame)
        self.queue_cne.grid(row=2, column=1, padx=5, pady=5)
        
        ttk.Label(form_frame, text="Notes:").grid(row=3, column=0, padx=5, pady=5, sticky=tk.W)
        self.queue_note_entries = []
        for i in range(4):
            entry = ttk.Entry(form_frame, width=5)
            entry.grid(row=3, column=1+i, padx=2, pady=5)
            self.queue_note_entries.append(entry)
        
        # Buttons
        button_frame = ttk.Frame(tab)
        button_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(button_frame, text="Ajouter à la File", command=self.add_to_queue).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Retirer de la File", command=self.remove_from_queue).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Inscrire depuis la File", command=self.enroll_from_queue).pack(side=tk.LEFT, padx=5)
        
        # Queue display
        display_frame = ttk.LabelFrame(tab, text="Affichage de la File", padding=10)
        display_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.queue_text = tk.Text(display_frame, height=10, width=60, state=tk.DISABLED)
        self.queue_text.pack(fill=tk.BOTH, expand=True)
        
        ttk.Button(display_frame, text="Actualiser la File", command=self.refresh_queue).pack(pady=5)
    
    def create_view_tab(self):
        """Tab for viewing students"""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="Voir les Étudiants")
        
        # Student list display
        display_frame = ttk.LabelFrame(tab, text="Liste des Étudiants", padding=10)
        display_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.student_text = tk.Text(display_frame, height=30, width=80, state=tk.DISABLED)
        self.student_text.pack(fill=tk.BOTH, expand=True)
        
        ttk.Button(display_frame, text="Actualiser la Liste", command=self.refresh_student_list).pack(pady=5)
    
    def safe_decode(self, text):
        """Safely decode text from C strings"""
        if not text:
            return ""
        try:
            return text.decode('utf-8')
        except UnicodeDecodeError:
            try:
                return text.decode('latin-1')
            except:
                try:
                    return text.decode(locale.getpreferredencoding())
                except:
                    return "Error decoding text"
    
    def validate_input(self, first_name, last_name, cne, notes):
        """Validate input fields"""
        if not first_name or not last_name:
            messagebox.showerror("Erreur", "Le prénom et le nom sont obligatoires")
            return False
            
        try:
            cne = int(cne)
            if cne <= 0:
                messagebox.showerror("Erreur", "Le CNE doit être un nombre positif")
                return False
        except ValueError:
            messagebox.showerror("Erreur", "Le CNE doit être un nombre valide")
            return False
            
        try:
            for note in notes:
                note_float = float(note)
                if note_float < 0 or note_float > 20:
                    messagebox.showerror("Erreur", "Les notes doivent être entre 0 et 20")
                    return False
        except ValueError:
            messagebox.showerror("Erreur", "Les notes doivent être des nombres valides")
            return False
            
        return True
    
    def auto_load_file(self):
        """Automatically try to load a student file"""
        try:
            # First clean up any existing data
            self.cleanup()
            
            # Look for files in the current directory
            current_dir = os.path.dirname(os.path.abspath(__file__))
            txt_files = [f for f in os.listdir(current_dir) if f.endswith('.txt')]
            
            if not txt_files:
                messagebox.showwarning("Attention", "Aucun fichier .txt trouvé dans le répertoire")
                return
            
            # Try to load each file until one succeeds
            for filename in txt_files:
                try:
                    full_path = os.path.join(current_dir, filename)
                    print(f"Trying to load file: {full_path}")
                    
                    # Create a buffer for the filename
                    filename_buffer = create_string_buffer(full_path.encode('utf-8'))
                    
                    # Call the DLL function
                    result = student_dll.lire_fichier_etudiants(filename_buffer)
                    
                    if result.success:
                        # Store the new list pointer
                        self.student_list = result.liste
                        
                        # Store the current file path (Python-managed)
                        self.current_file = create_string_buffer(full_path.encode('utf-8'))
                        
                        # Update UI
                        self.refresh_student_list()
                        print(f"Successfully loaded file: {full_path}")
                        
                        # Free the success message if it exists
                        if result.message:
                            safe_free(result.message)
                        return
                    else:
                        # Handle error message
                        if result.message:
                            error_msg = self.safe_decode(result.message)
                            print(f"Failed to load {filename}: {error_msg}")
                            safe_free(result.message)
                except Exception as e:
                    print(f"Error loading {filename}: {str(e)}")
                    continue
            
            # If we get here, no files could be loaded
            messagebox.showerror("Erreur", "Impossible de charger les fichiers du répertoire")
            
        except Exception as e:
            messagebox.showerror("Error", f"Échec du chargement des fichiers : {str(e)}")
            self.cleanup()
    
    def cleanup(self):
        """Clean up allocated resources"""
        if self.student_list:
            try:
                student_dll.liberer_liste(self.student_list)
            except Exception as e:
                print(f"Error freeing student list: {e}")
            self.student_list = None
        
        # No need to free current_file - it's Python-managed
        self.current_file = None
        self.current_sort = None
        self.update_sort_buttons()
    
    def update_sort_buttons(self):
        """Update the appearance of sort buttons based on current sort state"""
        self.sort_asc_button.config(style="TButton")
        self.sort_desc_button.config(style="TButton")
        self.sort_status.config(text="Non trié")
        
        if self.current_sort == 1:
            self.sort_asc_button.config(style="ActiveSort.TButton")
            self.sort_status.config(text="Trié par moyenne (croissant)")
        elif self.current_sort == 0:
            self.sort_desc_button.config(style="ActiveSort.TButton")
            self.sort_status.config(text="Trié par moyenne (décroissant)")
    
    def add_student(self):
        """Add a new student to the list"""
        if not self.current_file:
            messagebox.showerror("Erreur", "Veuillez charger un fichier d'abord")
            return
        
        # Get data from form
        first_name = self.first_name_entry.get()
        last_name = self.last_name_entry.get()
        cne = self.cne_entry.get()
        notes = [entry.get() for entry in self.note_entries]
        
        if not self.validate_input(first_name, last_name, cne, notes):
            return
        
        try:
            # Prepare data for C function
            first_name_buf = create_string_buffer(first_name.encode('utf-8'))
            last_name_buf = create_string_buffer(last_name.encode('utf-8'))
            cne_int = int(cne)
            notes_array = (c_float * 4)()
            for i in range(4):
                notes_array[i] = float(notes[i])
                
            # Call the DLL function
            result = student_dll.ajouter_etudiant(
                byref(self.student_list),
                self.current_file,
                first_name_buf,
                last_name_buf,
                cne_int,
                notes_array
            )
            
            # Handle the result
            if result.success:
                self.student_status.config(text="Étudiant ajouté avec succès")
                self.current_sort = None  # Adding a student invalidates the sort
                self.update_sort_buttons()
                self.refresh_student_list()
                # Clear form
                self.first_name_entry.delete(0, tk.END)
                self.last_name_entry.delete(0, tk.END)
                self.cne_entry.delete(0, tk.END)
                for entry in self.note_entries:
                    entry.delete(0, tk.END)
            else:
                if result.message:
                    error_msg = self.safe_decode(result.message)
                    messagebox.showerror("Error", error_msg)
                    safe_free(result.message)
                
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid input: {str(e)}")
        except Exception as e:
            messagebox.showerror("Error", f"Unexpected error: {str(e)}")
    
    def delete_student(self):
        """Delete a student from the list"""
        if not self.current_file:
            messagebox.showerror("Error", "Veuillez d'abord charger un fichier")
            return
            
        try:
            cne = int(self.cne_entry.get())
            result = student_dll.supprimer_etudiant(
                byref(self.student_list),
                self.current_file,
                cne
            )
            
            if result.success:
                self.student_status.config(text="L'étudiant a été supprimé avec succès")
                self.refresh_student_list()
                # Clear form
                self.cne_entry.delete(0, tk.END)
            else:
                if result.message:
                    error_msg = self.safe_decode(result.message)
                    messagebox.showerror("Error", error_msg)
                    safe_free(result.message)
                
        except ValueError:
            messagebox.showerror("Error", "Veuillez saisir un CNE valide")
    
    def search_student(self):
        """Search for a student by CNE"""
        if not self.current_file:
            messagebox.showerror("Error", "Veuillez d'abord charger un fichier")
            return
            
        try:
            cne = int(self.cne_entry.get())
            result = student_dll.chercher_etudiant(self.student_list, cne)
            
            if result.success:
                student_str = student_dll.afficher_etudiant(result.etudiant)
                if student_str:
                    try:
                        decoded_text = self.safe_decode(student_str)
                        messagebox.showinfo("Student Found", decoded_text)
                    finally:
                        safe_free(student_str)
            else:
                if result.message:
                    error_msg = self.safe_decode(result.message)
                    messagebox.showerror("Error", error_msg)
                    safe_free(result.message)
                
        except ValueError:
            messagebox.showerror("Error", "Veuillez saisir un CNE valide")
    
    def undo_delete(self):
        """Undo the last deletion"""
        if not self.current_file:
            messagebox.showerror("Error", "Veuillez d'abord charger un fichier")
            return
            
        result = student_dll.annuler_derniere_suppression(
            byref(self.student_list),
            self.current_file
        )
        
        if result.success:
            self.student_status.config(text="Dernière suppression annulée")
            self.refresh_student_list()
        else:
            if result.message:
                error_msg = self.safe_decode(result.message)
                messagebox.showerror("Error", error_msg)
                safe_free(result.message)
    
    def sort_students(self, order, display=False):
        """Sort students by average"""
        if self.current_sort == order:
            if display:
                self.sort_tab_status.config(text="Les étudiants sont déjà triés dans cet ordre")
            return
            
        if not self.current_file:
            messagebox.showerror("Error", "Veuillez d'abord charger un fichier")
            return
        
        sort_direction = "croissant" if order else "décroissant"
        status_text = f"Tri en cours ({sort_direction})..."
        
        if display:
            self.sort_tab_status.config(text=status_text)
        else:
            self.student_status.config(text=status_text)
            
        self.root.update()  # Force UI update
        
        try:
            result = student_dll.trier_etudiants_moyenne(
                byref(self.student_list),
                self.current_file,
                order
            )
            
            if result.success:
                self.current_sort = order
                self.update_sort_buttons()
                
                success_text = f"Tri {sort_direction} terminé avec succès"
                if display:
                    self.sort_tab_status.config(text=success_text, foreground="green")
                    self.display_sorted_students()
                else:
                    self.student_status.config(text=success_text, foreground="green")
                    self.refresh_student_list()
            else:
                if result.message:
                    error_msg = self.safe_decode(result.message)
                    if display:
                        self.sort_tab_status.config(text=error_msg, foreground="red")
                    messagebox.showerror("Error", error_msg)
                    safe_free(result.message)
        except Exception as e:
            error_text = f"Erreur de tri: {str(e)}"
            if display:
                self.sort_tab_status.config(text=error_text, foreground="red")
            messagebox.showerror("Error", error_text)
        finally:
            # Ensure any intermediate results are cleaned up
            if 'result' in locals() and hasattr(result, 'message'):
                safe_free(result.message)
    
    def display_sorted_students(self):
        """Display sorted students in the sorting tab"""
        if not self.student_list:
            self.sorted_display.config(state=tk.NORMAL)
            self.sorted_display.delete(1.0, tk.END)
            self.sorted_display.insert(1.0, "Aucune donnée étudiante chargée")
            self.sorted_display.config(state=tk.DISABLED)
            return
            
        list_str = student_dll.afficher_liste_etudiants(self.student_list)
        if list_str:
            try:
                decoded_text = self.safe_decode(list_str)
                
                # Process the text to show only names and averages
                lines = decoded_text.split('\n')
                filtered_lines = []
                for line in lines:
                    if "Nom:" in line or "Moyenne:" in line:
                        filtered_lines.append(line)
                
                filtered_text = '\n'.join(filtered_lines)
                
                self.sorted_display.config(state=tk.NORMAL)
                self.sorted_display.delete(1.0, tk.END)
                self.sorted_display.insert(1.0, filtered_text)
                self.sorted_display.config(state=tk.DISABLED)
            finally:
                safe_free(list_str)
        else:
            self.sorted_display.config(state=tk.NORMAL)
            self.sorted_display.delete(1.0, tk.END)
            self.sorted_display.insert(1.0, "Aucun étudiant dans la liste")
            self.sorted_display.config(state=tk.DISABLED)
    
    def add_to_queue(self):
        """Add a student to the waiting queue"""
        first_name = self.queue_first_name.get()
        last_name = self.queue_last_name.get()
        cne = self.queue_cne.get()
        notes = [entry.get() for entry in self.queue_note_entries]
        
        if not self.validate_input(first_name, last_name, cne, notes):
            return
            
        try:
            # Prepare data for C function
            first_name_buf = create_string_buffer(first_name.encode('utf-8'))
            last_name_buf = create_string_buffer(last_name.encode('utf-8'))
            cne_int = int(cne)
            notes_array = (c_float * 4)()
            for i in range(4):
                notes_array[i] = float(notes[i])
                
            # Call the DLL function
            result = student_dll.ajouter_file_attente(
                first_name_buf,
                last_name_buf,
                cne_int,
                notes_array
            )
            
            # Handle the result
            if result.success:
                messagebox.showinfo("Success", "Étudiant ajouté à la file d'attente")
                self.refresh_queue()
                # Clear form
                self.queue_first_name.delete(0, tk.END)
                self.queue_last_name.delete(0, tk.END)
                self.queue_cne.delete(0, tk.END)
                for entry in self.queue_note_entries:
                    entry.delete(0, tk.END)
            else:
                if result.message:
                    error_msg = self.safe_decode(result.message)
                    messagebox.showerror("Error", error_msg)
                    safe_free(result.message)
                
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid input: {str(e)}")
        except Exception as e:
            messagebox.showerror("Error", f"Unexpected error: {str(e)}")
    
    def remove_from_queue(self):
        """Remove a student from the waiting queue"""
        result = student_dll.retirer_file_attente()
        
        if result.success:
            messagebox.showinfo("Success", "Étudiant retiré de la file d'attente")
            self.refresh_queue()
        else:
            if result.message:
                error_msg = self.safe_decode(result.message)
                messagebox.showerror("Error", error_msg)
                safe_free(result.message)
    
    def enroll_from_queue(self):
        """Enroll a student from the queue to the main list"""
        if not self.current_file:
            messagebox.showerror("Error", "Veuillez d'abord charger un fichier")
            return
            
        result = student_dll.inscrire_etudiant_file(
            byref(self.student_list),
            self.current_file
        )
        
        if result.success:
            self.student_status.config(text="Étudiant inscrit depuis la file d'attente")
            self.current_sort = None  # Adding a student invalidates the sort
            self.update_sort_buttons()
            self.refresh_student_list()
            self.refresh_queue()
        else:
            if result.message:
                error_msg = self.safe_decode(result.message)
                messagebox.showerror("Error", error_msg)
                safe_free(result.message)
    
    def refresh_queue(self):
        """Refresh the queue display"""
        queue_str = student_dll.afficher_file_attente()
        if queue_str:
            try:
                decoded_text = self.safe_decode(queue_str)
                self.queue_text.config(state=tk.NORMAL)
                self.queue_text.delete(1.0, tk.END)
                self.queue_text.insert(1.0, decoded_text)
                self.queue_text.config(state=tk.DISABLED)
            finally:
                safe_free(queue_str)
        else:
            self.queue_text.config(state=tk.NORMAL)
            self.queue_text.delete(1.0, tk.END)
            self.queue_text.insert(1.0, "La file d'attente est vide")
            self.queue_text.config(state=tk.DISABLED)
    
    def refresh_student_list(self):
        """Refresh the student list display"""
        if not self.student_list:
            self.student_text.config(state=tk.NORMAL)
            self.student_text.delete(1.0, tk.END)
            self.student_text.insert(1.0, "Aucune donnée étudiante chargée")
            self.student_text.config(state=tk.DISABLED)
            return
            
        list_str = student_dll.afficher_liste_etudiants(self.student_list)
        if list_str:
            try:
                decoded_text = self.safe_decode(list_str)
                self.student_text.config(state=tk.NORMAL)
                self.student_text.delete(1.0, tk.END)
                self.student_text.insert(1.0, decoded_text)
                self.student_text.config(state=tk.DISABLED)
            finally:
                safe_free(list_str)
        else:
            self.student_text.config(state=tk.NORMAL)
            self.student_text.delete(1.0, tk.END)
            self.student_text.insert(1.0, "Aucun étudiant dans la liste")
            self.student_text.config(state=tk.DISABLED)

if __name__ == "__main__":
    root = tk.Tk()
    try:
        app = StudentManagementApp(root)
        root.mainloop()
    except Exception as e:
        messagebox.showerror("Fatal Error", f"Application crashed: {str(e)}")
        sys.exit(1)