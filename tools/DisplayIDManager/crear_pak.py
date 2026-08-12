from paklib import PAKFile
from pathlib import Path

def main():
    carpeta = Path("MiAtlasNuevo")
    salida = Path("item_atlas.pak")
    
    print("======================================")
    print("EMPEZANDO A FABRICAR EL PAK...")
    
    # Esto lee los 6 archivos que metiste en la carpeta[cite: 12]
    pak = PAKFile.from_directory(carpeta, "sprite")
    
    # Esto crea el archivo final[cite: 12]
    pak.write(salida)
    
    print(f"¡HECHO! Se han empaquetado {len(pak.sprites)} paginas del atlas.")
    print("Ya puedes cerrar esta ventana.")
    print("======================================")

if __name__ == "__main__":
    main()