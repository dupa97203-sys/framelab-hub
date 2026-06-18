import os
import urllib.request
import zipfile
import shutil

print("=== Pobieranie Dear ImGui przy użyciu Pythona (Poprawione) ===")
url = "https://github.com/ocornut/imgui/archive/refs/tags/v1.90.4.zip"
zip_path = "imgui.zip"

try:
    print(f"Pobieranie z: {url}")
    urllib.request.urlretrieve(url, zip_path)
    print("Pobieranie zakończone.")

    print("Rozpakowywanie...")
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall("imgui_temp")
    print("Rozpakowywanie zakończone.")

    # Ścieżka do rozpakowanego folderu
    temp_folder = os.path.join("imgui_temp", "imgui-1.90.4")
    
    # Utwórz katalog docelowy
    os.makedirs("imgui", exist_ok=True)
    os.makedirs(os.path.join("imgui", "backends"), exist_ok=True)

    # Kopiowanie rdzenia (wszystkie .h i .cpp z głównego folderu)
    print("Kopiowanie plików rdzenia...")
    for filename in os.listdir(temp_folder):
        if filename.endswith(".h") or filename.endswith(".cpp"):
            shutil.copy(os.path.join(temp_folder, filename), "imgui")

    # Kopiowanie backendów GLFW i OpenGL3
    print("Kopiowanie backendów GLFW + OpenGL3...")
    backend_src = os.path.join(temp_folder, "backends")
    backend_dest = os.path.join("imgui", "backends")
    
    backends_to_copy = [
        "imgui_impl_glfw.h", "imgui_impl_glfw.cpp",
        "imgui_impl_opengl3.h", "imgui_impl_opengl3.cpp",
        "imgui_impl_opengl3_loader.h"
    ]
    for filename in backends_to_copy:
        src_file = os.path.join(backend_src, filename)
        if os.path.exists(src_file):
            shutil.copy(src_file, backend_dest)

    # Czyszczenie
    print("Czyszczenie plików tymczasowych...")
    if os.path.exists(zip_path):
        os.remove(zip_path)
    if os.path.exists("imgui_temp"):
        shutil.rmtree("imgui_temp")
        
    print("=== Ukończono! Wszystkie pliki Dear ImGui znajdują się w folderze ./imgui ===")

except Exception as e:
    print(f"Błąd podczas instalacji: {e}")
